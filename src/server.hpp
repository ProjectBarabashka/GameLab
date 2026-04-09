// server.hpp — AETHORIA: Минимальный игровой сервер (Приоритет 4 редмапа)
// TCP сервер на Winsock2 (Windows) / POSIX sockets (Linux/Mac).
// Принцип: НЕ переписывать движок — вынести game loop в server loop.
//
// Архитектура:
//   GameServer — принимает соединения, читает пакеты, обновляет ServerWorld
//   ServerWorld — упрощённая копия состояния мира (позиции, HP)
//   Клиент      — main.cpp (GameEngine) шлёт INPUT, получает STATE
//
// Протокол (простой бинарный / JSON-lines):
//   Клиент → Сервер:  { "type":"input", "pid":1, "dx":0.5, "dy":-1.0 }
//   Сервер → Клиент:  { "type":"state", "players":[...], "enemies":[...] }
//
// Запуск отдельным процессом:
//   GameServer server(7777);
//   server.run();   // блокирующий loop

#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using SocketFD = SOCKET;
  #define INVALID_SOCK INVALID_SOCKET
  #define CLOSE_SOCK(s) closesocket(s)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #include <fcntl.h>
  using SocketFD = int;
  #define INVALID_SOCK (-1)
  #define CLOSE_SOCK(s) close(s)
#endif

// ═══════════════════════════════════════════════════════════════
// ПАКЕТ ИГРОКА
// ═══════════════════════════════════════════════════════════════
struct PlayerState {
    int   id    = 0;
    float x     = 0.f, y     = 0.f;
    float hp    = 100.f, maxHp = 100.f;
    float mp    = 50.f,  maxMp = 50.f;
    int   level = 1;
    std::string name;
    std::string scene;
    bool  online = false;
};

// ═══════════════════════════════════════════════════════════════
// ВХОДЯЩИЙ ПАКЕТ (от клиента)
// ═══════════════════════════════════════════════════════════════
struct InputPacket {
    int         playerId = 0;
    float       dx = 0.f, dy = 0.f;
    std::string skill;
    std::string action;   // "attack", "interact", "use_item"
    std::string payload;  // JSON payload для сложных действий
};

// ═══════════════════════════════════════════════════════════════
// МИРОВОЕ СОСТОЯНИЕ СЕРВЕРА (упрощённое)
// ═══════════════════════════════════════════════════════════════
class ServerWorld {
public:
    std::mutex                         mtx;
    std::map<int, PlayerState>         players;
    int                                nextPlayerId = 1;

    // Callbacks движка (подключается снаружи)
    std::function<void(InputPacket&)>  onInput;

    int registerPlayer(const std::string& name) {
        std::lock_guard<std::mutex> lk(mtx);
        PlayerState ps;
        ps.id     = nextPlayerId++;
        ps.name   = name;
        ps.online = true;
        ps.x      = 60*32.f; ps.y = 60*32.f;
        players[ps.id] = ps;
        std::cout << "[Server] Игрок зарегистрирован: " << name
                  << " id=" << ps.id << "\n";
        return ps.id;
    }

    void disconnectPlayer(int id) {
        std::lock_guard<std::mutex> lk(mtx);
        if (players.count(id)) {
            players[id].online = false;
            std::cout << "[Server] Игрок отключён: id=" << id << "\n";
        }
    }

    void applyInput(const InputPacket& pkt) {
        {
            std::lock_guard<std::mutex> lk(mtx);
            auto it = players.find(pkt.playerId);
            if (it == players.end() || !it->second.online) return;
            auto& ps = it->second;
            // Простое движение (серверная сторона валидирует)
            float speed = 150.f;
            ps.x += pkt.dx * speed * 0.016f;  // ~60fps
            ps.y += pkt.dy * speed * 0.016f;
        }
        if (onInput) {
            InputPacket copy = pkt;
            onInput(copy);
        }
    }

    // Сериализация состояния в JSON-строку
    std::string serializeState() {
        std::lock_guard<std::mutex> lk(mtx);
        std::ostringstream ss;
        ss << "{\"type\":\"state\",\"players\":[";
        bool first = true;
        for (auto& [id, ps] : players) {
            if (!ps.online) continue;
            if (!first) ss << ","; first = false;
            ss << "{\"id\":" << ps.id
               << ",\"x\":"  << ps.x << ",\"y\":" << ps.y
               << ",\"hp\":" << ps.hp << ",\"maxHp\":" << ps.maxHp
               << ",\"mp\":" << ps.mp << ",\"maxMp\":" << ps.maxMp
               << ",\"lv\":" << ps.level
               << ",\"name\":\"" << ps.name << "\""
               << ",\"scene\":\"" << ps.scene << "\""
               << "}";
        }
        ss << "]}";
        return ss.str();
    }
};

// ═══════════════════════════════════════════════════════════════
// TCP СЕРВЕР
// ═══════════════════════════════════════════════════════════════
class GameServer {
public:
    explicit GameServer(int port = 7777)
        : port_(port), running_(false) {}

    ~GameServer() { stop(); }

    ServerWorld world;

    bool start() {
#ifdef _WIN32
        WSADATA wd;
        if (WSAStartup(MAKEWORD(2,2), &wd) != 0) {
            std::cerr << "[Server] WSAStartup failed\n"; return false;
        }
#endif
        listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listenFd_ == INVALID_SOCK) {
            std::cerr << "[Server] socket() failed\n"; return false;
        }

        int opt = 1;
#ifdef _WIN32
        setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
        setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons((uint16_t)port_);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "[Server] bind() failed на порту " << port_ << "\n";
            return false;
        }
        if (listen(listenFd_, 10) < 0) {
            std::cerr << "[Server] listen() failed\n"; return false;
        }

        running_ = true;
        acceptThread_ = std::thread(&GameServer::_acceptLoop, this);
        std::cout << "[Server] Слушаю порт " << port_ << "\n";
        return true;
    }

    void stop() {
        running_ = false;
        CLOSE_SOCK(listenFd_);
        if (acceptThread_.joinable()) acceptThread_.join();
        // Закрываем клиентов
        std::lock_guard<std::mutex> lk(clientsMtx_);
        for (auto fd : clients_) CLOSE_SOCK(fd);
        clients_.clear();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // Рассылаем состояние всем клиентам (вызывай из game loop каждые N мс)
    void broadcast() {
        std::string state = world.serializeState() + "\n";
        std::lock_guard<std::mutex> lk(clientsMtx_);
        for (auto it = clients_.begin(); it != clients_.end(); ) {
            if (!_send(*it, state)) {
                CLOSE_SOCK(*it);
                it = clients_.erase(it);
            } else ++it;
        }
    }

    bool isRunning() const { return running_; }
    int  playerCount() {
        std::lock_guard<std::mutex> lk(clientsMtx_);
        return (int)clients_.size();
    }

private:
    int           port_;
    SocketFD      listenFd_ = INVALID_SOCK;
    std::atomic<bool> running_{false};
    std::thread   acceptThread_;
    std::vector<SocketFD> clients_;
    std::mutex    clientsMtx_;

    void _acceptLoop() {
        while (running_) {
            sockaddr_in clientAddr{};
#ifdef _WIN32
            int addrLen = sizeof(clientAddr);
#else
            socklen_t addrLen = sizeof(clientAddr);
#endif
            SocketFD fd = accept(listenFd_, (sockaddr*)&clientAddr, &addrLen);
            if (fd == INVALID_SOCK) {
                if (running_) std::cerr << "[Server] accept() error\n";
                break;
            }
            {
                std::lock_guard<std::mutex> lk(clientsMtx_);
                clients_.push_back(fd);
            }
            std::cout << "[Server] Новое подключение. Всего: "
                      << clients_.size() << "\n";
            // Каждый клиент в отдельном потоке
            std::thread([this, fd]{ _clientLoop(fd); }).detach();
        }
    }

    void _clientLoop(SocketFD fd) {
        int playerId = 0;
        char buf[4096];
        std::string partial;

        while (running_) {
#ifdef _WIN32
            int n = recv(fd, buf, sizeof(buf)-1, 0);
#else
            int n = read(fd, buf, sizeof(buf)-1);
#endif
            if (n <= 0) break;
            buf[n] = '\0';
            partial += buf;

            // Обрабатываем строки (разделитель \n)
            size_t pos;
            while ((pos = partial.find('\n')) != std::string::npos) {
                std::string line = partial.substr(0, pos);
                partial = partial.substr(pos + 1);
                _handlePacket(fd, line, playerId);
            }
        }

        if (playerId > 0) world.disconnectPlayer(playerId);
        {
            std::lock_guard<std::mutex> lk(clientsMtx_);
            clients_.erase(std::remove(clients_.begin(), clients_.end(), fd),
                           clients_.end());
        }
        CLOSE_SOCK(fd);
    }

    void _handlePacket(SocketFD fd, const std::string& line, int& playerId) {
        // Простой мини-парсер без зависимостей
        // Формат: {"type":"login","name":"Hero"}
        //         {"type":"input","dx":1.0,"dy":0.0}
        auto extract = [&](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\":";
            size_t p = line.find(search);
            if (p == std::string::npos) return "";
            p += search.size();
            // string value
            if (line[p] == '"') {
                p++;
                size_t end = line.find('"', p);
                return end != std::string::npos ? line.substr(p, end-p) : "";
            }
            // number value
            size_t end = line.find_first_of(",}", p);
            return end != std::string::npos ? line.substr(p, end-p) : "";
        };

        std::string type = extract("type");

        if (type == "login") {
            std::string name = extract("name");
            if (name.empty()) name = "Player";
            playerId = world.registerPlayer(name);
            std::string resp = "{\"type\":\"welcome\",\"id\":" +
                               std::to_string(playerId) + "}\n";
            _send(fd, resp);
        }
        else if (type == "input" && playerId > 0) {
            InputPacket pkt;
            pkt.playerId = playerId;
            std::string dxs = extract("dx"), dys = extract("dy");
            try { pkt.dx = std::stof(dxs); } catch (...) {}
            try { pkt.dy = std::stof(dys); } catch (...) {}
            pkt.skill  = extract("skill");
            pkt.action = extract("action");
            world.applyInput(pkt);
        }
        else if (type == "ping") {
            _send(fd, "{\"type\":\"pong\"}\n");
        }
    }

    bool _send(SocketFD fd, const std::string& msg) {
#ifdef _WIN32
        int n = send(fd, msg.c_str(), (int)msg.size(), 0);
#else
        int n = write(fd, msg.c_str(), msg.size());
#endif
        return n > 0;
    }
};

// ═══════════════════════════════════════════════════════════════
// КАК ИСПОЛЬЗОВАТЬ:
// ═══════════════════════════════════════════════════════════════
//
// // server_main.cpp — отдельный исполняемый файл
// #include "server.hpp"
// int main() {
//     GameServer server(7777);
//     if (!server.start()) return 1;
//     // Game loop сервера
//     while (server.isRunning()) {
//         // тут логика мира...
//         server.broadcast();
//         std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
//     }
//     return 0;
// }
//
// // Подключение коллбэков:
// server.world.onInput = [&](InputPacket& pkt) {
//     // пересылай в твой GameEngine::applyRemoteInput(pkt)
// };
