// trigger_system.hpp — Система триггеров и серверных сцен AETHORIA
// Триггерные зоны: ENTER / LEAVE / INTERACT / TIMER / CONDITION
// Серверные сцены: инстансы, данжи, переходы с spawn-offset
//
// Использование:
//   TriggerSystem triggers;
//   triggers.add({...});
//   // В game loop:
//   triggers.update(player.pos.x, player.pos.y, dt);

#pragma once
#include "json_parser.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

// ═══════════════════════════════════════════════════════════════
// ТИПЫ СОБЫТИЙ ТРИГГЕРА
// ═══════════════════════════════════════════════════════════════
enum class TriggerEvent {
    ENTER,       // игрок вошёл в зону
    LEAVE,       // игрок вышел из зоны
    INTERACT,    // игрок нажал [E] внутри зоны
    TIMER,       // таймер внутри зоны истёк
    CONDITION,   // внешнее условие выполнено (вызов triggerCondition)
};

inline std::string triggerEventStr(TriggerEvent e) {
    switch(e) {
        case TriggerEvent::ENTER:     return "enter";
        case TriggerEvent::LEAVE:     return "leave";
        case TriggerEvent::INTERACT:  return "interact";
        case TriggerEvent::TIMER:     return "timer";
        case TriggerEvent::CONDITION: return "condition";
        default: return "enter";
    }
}

inline TriggerEvent triggerEventFromStr(const std::string& s) {
    if (s == "leave")     return TriggerEvent::LEAVE;
    if (s == "interact")  return TriggerEvent::INTERACT;
    if (s == "timer")     return TriggerEvent::TIMER;
    if (s == "condition") return TriggerEvent::CONDITION;
    return TriggerEvent::ENTER;
}

// ═══════════════════════════════════════════════════════════════
// ФОРМА ЗОНЫ
// ═══════════════════════════════════════════════════════════════
enum class TriggerShape { CIRCLE, RECT };

// ═══════════════════════════════════════════════════════════════
// ДЕЙСТВИЕ ТРИГГЕРА
// ═══════════════════════════════════════════════════════════════
enum class TriggerAction {
    SCENE_SWITCH,    // переход в другую сцену
    SCENE_INSTANCE,  // войти в инстанс (данж)
    DIALOGUE,        // показать диалог/сообщение
    SOUND,           // воспроизвести звук
    PARTICLES,       // эффект частиц
    CUSTOM,          // callback
    NONE,
};

inline TriggerAction triggerActionFromStr(const std::string& s) {
    if (s == "scene_switch")   return TriggerAction::SCENE_SWITCH;
    if (s == "scene_instance") return TriggerAction::SCENE_INSTANCE;
    if (s == "dialogue")       return TriggerAction::DIALOGUE;
    if (s == "sound")          return TriggerAction::SOUND;
    if (s == "particles")      return TriggerAction::PARTICLES;
    if (s == "custom")         return TriggerAction::CUSTOM;
    return TriggerAction::NONE;
}

// ═══════════════════════════════════════════════════════════════
// ОПИСАНИЕ ТРИГГЕРА
// ═══════════════════════════════════════════════════════════════
struct Trigger {
    std::string  id;           // уникальный ID, напр. "portal_forest_enter"
    std::string  name;         // отображаемое имя
    bool         active = true;
    bool         oneShot = false;    // срабатывает только один раз
    bool         _fired  = false;    // уже сработал (для oneShot)

    // ── Зона ─────────────────────────────────────
    TriggerShape shape  = TriggerShape::CIRCLE;
    float        x      = 0.f;  // центр (пиксели)
    float        y      = 0.f;
    float        radius = 48.f; // для CIRCLE
    float        w      = 64.f; // для RECT
    float        h      = 64.f;

    // ── Событие ──────────────────────────────────
    TriggerEvent event  = TriggerEvent::ENTER;
    float        timerDuration = 3.f;  // для TIMER
    float        _timer = 0.f;
    bool         _playerInside = false;

    // ── Действие ─────────────────────────────────
    TriggerAction action = TriggerAction::NONE;
    std::string   targetScene;      // для SCENE_SWITCH / SCENE_INSTANCE
    float         spawnOffsetX = 0.f; // куда поставить игрока в целевой сцене (тайлы)
    float         spawnOffsetY = 0.f;
    std::string   dialogueText;     // для DIALOGUE
    std::string   soundName;        // для SOUND
    std::string   customTag;        // для CUSTOM

    // ── Минимальный уровень игрока ────────────────
    int  levelRequirement = 0;
    std::string blockedMessage = "Слишком опасно! Прокачайся сначала.";

    // ── Коллбэк (только для CUSTOM, не сериализуется) ──
    std::function<void(const Trigger&)> callback;

    // ── Проверка коллизии ─────────────────────────
    bool contains(float px, float py) const {
        if (shape == TriggerShape::CIRCLE) {
            float dx = px - x, dy = py - y;
            return dx*dx + dy*dy <= radius*radius;
        } else {
            return px >= x - w/2.f && px <= x + w/2.f &&
                   py >= y - h/2.f && py <= y + h/2.f;
        }
    }
};

// ═══════════════════════════════════════════════════════════════
// ОПИСАНИЕ СЕРВЕРНОЙ СЦЕНЫ / ИНСТАНСА
// ═══════════════════════════════════════════════════════════════
struct ServerScene {
    std::string id;           // "dungeon_goblin"
    std::string displayName;  // "Гоблинские пещеры"
    std::string mapFile;      // "assets/scenes/dungeon_goblin.json"
    int         maxPlayers   = 5;
    int         levelReq     = 1;
    int         levelMax     = 999;
    float       cooldownSecs = 60.f;
    bool        pvp          = false;
    bool        instanced    = false;  // true = у каждого игрока своя копия
    std::string bossType;    // "GOBLIN_KING"
    std::string exitScene;   // куда возвращаться при выходе
    float       exitSpawnX   = 60.f;
    float       exitSpawnY   = 60.f;
    // Runtime
    float _cooldownRemaining = 0.f;
};

// ═══════════════════════════════════════════════════════════════
// СИСТЕМА ТРИГГЕРОВ
// ═══════════════════════════════════════════════════════════════
class TriggerSystem {
public:
    // Коллбэки — движок подключает их
    std::function<void(const std::string& sceneId, float spawnX, float spawnY)> onSceneSwitch;
    std::function<void(const std::string& instanceId)>                           onInstanceEnter;
    std::function<void(const std::string& text, float duration)>                 onDialogue;
    std::function<void(const std::string& soundName)>                            onSound;
    std::function<void(float x, float y)>                                        onParticles;
    std::function<bool(int levelRequired, const std::string& msg)>               onLevelCheck;

    // ── Добавление / удаление ────────────────────
    void add(Trigger t) {
        triggers_.push_back(std::move(t));
    }

    void remove(const std::string& id) {
        triggers_.erase(std::remove_if(triggers_.begin(), triggers_.end(),
            [&](const Trigger& t){ return t.id == id; }), triggers_.end());
    }

    void clear() { triggers_.clear(); }

    Trigger* find(const std::string& id) {
        for (auto& t : triggers_) if (t.id == id) return &t;
        return nullptr;
    }

    // Активировать/деактивировать по ID
    void setActive(const std::string& id, bool v) {
        if (auto* t = find(id)) t->active = v;
    }

    // Внешнее условие (для TriggerEvent::CONDITION)
    void triggerCondition(const std::string& id) {
        auto* t = find(id);
        if (!t || !t->active) return;
        if (t->event == TriggerEvent::CONDITION)
            fire(*t);
    }

    // ── Обновление (вызывать каждый кадр) ────────
    // playerX/playerY — позиция игрока в пикселях
    // playerLevel     — текущий уровень игрока
    void update(float playerX, float playerY, float dt, int playerLevel = 1) {
        for (auto& t : triggers_) {
            if (!t.active) continue;
            if (t.oneShot && t._fired) continue;

            bool inside = t.contains(playerX, playerY);

            switch (t.event) {
            case TriggerEvent::ENTER:
                if (inside && !t._playerInside) {
                    t._playerInside = true;
                    fire(t, playerLevel);
                } else if (!inside) {
                    t._playerInside = false;
                }
                break;

            case TriggerEvent::LEAVE:
                if (!inside && t._playerInside) {
                    t._playerInside = false;
                    fire(t, playerLevel);
                } else if (inside) {
                    t._playerInside = true;
                }
                break;

            case TriggerEvent::TIMER:
                if (inside) {
                    if (!t._playerInside) {
                        t._playerInside = true;
                        t._timer = 0.f;
                    }
                    t._timer += dt;
                    if (t._timer >= t.timerDuration) {
                        t._timer = 0.f;
                        fire(t, playerLevel);
                    }
                } else {
                    t._playerInside = false;
                    t._timer = 0.f;
                }
                break;

            case TriggerEvent::INTERACT:
                // Только отмечаем "внутри" — fire вызывается извне через tryInteract()
                t._playerInside = inside;
                break;

            case TriggerEvent::CONDITION:
                // Fire только через triggerCondition()
                break;
            }
        }
    }

    // Вызывать из обработчика [E] — возвращает id сработавшего триггера или ""
    std::string tryInteract(float playerX, float playerY, int playerLevel = 1) {
        for (auto& t : triggers_) {
            if (!t.active) continue;
            if (t.oneShot && t._fired) continue;
            if (t.event != TriggerEvent::INTERACT) continue;
            if (t.contains(playerX, playerY)) {
                fire(t, playerLevel);
                return t.id;
            }
        }
        return "";
    }

    // Таймеры кулдаунов серверных сцен
    void updateCooldowns(float dt) {
        for (auto& [id, ss] : serverScenes_)
            if (ss._cooldownRemaining > 0.f)
                ss._cooldownRemaining = std::max(0.f, ss._cooldownRemaining - dt);
    }

    // ── Серверные сцены ───────────────────────────
    void registerServerScene(const ServerScene& ss) {
        serverScenes_[ss.id] = ss;
    }

    ServerScene* getServerScene(const std::string& id) {
        auto it = serverScenes_.find(id);
        return it != serverScenes_.end() ? &it->second : nullptr;
    }

    bool enterServerScene(const std::string& id, int playerLevel) {
        auto* ss = getServerScene(id);
        if (!ss) { std::cerr << "[Trigger] ServerScene не найдена: " << id << "\n"; return false; }
        if (ss->_cooldownRemaining > 0.f) {
            if (onDialogue) onDialogue("Данж на кулдауне: " +
                std::to_string(int(ss->_cooldownRemaining)) + " сек.", 3.f);
            return false;
        }
        if (playerLevel < ss->levelReq) {
            if (onDialogue) onDialogue("Требуется уровень " +
                std::to_string(ss->levelReq) + ".", 3.f);
            return false;
        }
        ss->_cooldownRemaining = ss->cooldownSecs;
        if (onInstanceEnter) onInstanceEnter(id);
        return true;
    }

    // ── Загрузка из JSON ──────────────────────────
    // Формат: assets/triggers.json
    bool loadFromFile(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isObject()) return false;

        // triggers []
        if (auto arr = root->get("triggers")) {
            for (size_t i = 0; i < arr->arrayVal.size(); i++) {
                auto j = arr->get(i);
                if (!j) continue;
                Trigger t;
                if (auto v = j->get("id"))      t.id      = v->asString();
                if (auto v = j->get("name"))    t.name    = v->asString();
                if (auto v = j->get("active"))  t.active  = v->asBool();
                if (auto v = j->get("one_shot"))t.oneShot = v->asBool();
                if (auto v = j->get("x"))       t.x       = (float)v->asDouble();
                if (auto v = j->get("y"))       t.y       = (float)v->asDouble();
                if (auto v = j->get("radius"))  t.radius  = (float)v->asDouble();
                if (auto v = j->get("w"))       t.w       = (float)v->asDouble();
                if (auto v = j->get("h"))       t.h       = (float)v->asDouble();
                if (auto v = j->get("shape"))   t.shape   = (v->asString()=="rect") ? TriggerShape::RECT : TriggerShape::CIRCLE;
                if (auto v = j->get("event"))   t.event   = triggerEventFromStr(v->asString());
                if (auto v = j->get("timer_duration")) t.timerDuration = (float)v->asDouble();
                if (auto v = j->get("action"))  t.action  = triggerActionFromStr(v->asString());
                if (auto v = j->get("target_scene"))   t.targetScene  = v->asString();
                if (auto v = j->get("spawn_offset_x")) t.spawnOffsetX = (float)v->asDouble();
                if (auto v = j->get("spawn_offset_y")) t.spawnOffsetY = (float)v->asDouble();
                if (auto v = j->get("dialogue"))       t.dialogueText = v->asString();
                if (auto v = j->get("sound"))          t.soundName    = v->asString();
                if (auto v = j->get("level_req"))      t.levelRequirement = v->asInt();
                if (auto v = j->get("blocked_msg"))    t.blockedMessage   = v->asString();
                if (auto v = j->get("custom_tag"))     t.customTag        = v->asString();
                triggers_.push_back(std::move(t));
            }
        }

        // server_scenes []
        if (auto arr = root->get("server_scenes")) {
            for (size_t i = 0; i < arr->arrayVal.size(); i++) {
                auto j = arr->get(i);
                if (!j) continue;
                ServerScene ss;
                if (auto v = j->get("id"))          ss.id          = v->asString();
                if (auto v = j->get("name"))        ss.displayName = v->asString();
                if (auto v = j->get("map_file"))    ss.mapFile     = v->asString();
                if (auto v = j->get("max_players")) ss.maxPlayers  = v->asInt();
                if (auto v = j->get("level_req"))   ss.levelReq    = v->asInt();
                if (auto v = j->get("level_max"))   ss.levelMax    = v->asInt();
                if (auto v = j->get("cooldown"))    ss.cooldownSecs= (float)v->asDouble();
                if (auto v = j->get("pvp"))         ss.pvp         = v->asBool();
                if (auto v = j->get("instanced"))   ss.instanced   = v->asBool();
                if (auto v = j->get("boss"))        ss.bossType    = v->asString();
                if (auto v = j->get("exit_scene"))  ss.exitScene   = v->asString();
                if (auto v = j->get("exit_x"))      ss.exitSpawnX  = (float)v->asDouble();
                if (auto v = j->get("exit_y"))      ss.exitSpawnY  = (float)v->asDouble();
                serverScenes_[ss.id] = ss;
            }
        }

        std::cout << "[TriggerSystem] Загружено: " << triggers_.size()
                  << " триггеров, " << serverScenes_.size() << " серверных сцен из " << path << "\n";
        return true;
    }

    // ── Сохранение в JSON ─────────────────────────
    bool saveToFile(const std::string& path) const {
        std::ofstream f(path);
        if (!f.is_open()) return false;

        auto esc = [](const std::string& s) {
            std::string o;
            for (char c : s) {
                if (c=='"') o += "\\\"";
                else if (c=='\\') o += "\\\\";
                else o += c;
            }
            return o;
        };

        f << "{\n  \"triggers\": [\n";
        for (size_t i = 0; i < triggers_.size(); i++) {
            const auto& t = triggers_[i];
            f << "    {\n";
            f << "      \"id\": \""     << esc(t.id)   << "\",\n";
            f << "      \"name\": \""   << esc(t.name) << "\",\n";
            f << "      \"active\": "   << (t.active  ? "true":"false") << ",\n";
            f << "      \"one_shot\": " << (t.oneShot ? "true":"false") << ",\n";
            f << "      \"shape\": \""  << (t.shape==TriggerShape::RECT?"rect":"circle") << "\",\n";
            f << "      \"x\": "        << t.x      << ", \"y\": " << t.y << ",\n";
            f << "      \"radius\": "   << t.radius << ", \"w\": " << t.w << ", \"h\": " << t.h << ",\n";
            f << "      \"event\": \""  << triggerEventStr(t.event) << "\",\n";
            f << "      \"timer_duration\": " << t.timerDuration << ",\n";
            f << "      \"action\": \"";
            switch(t.action) {
                case TriggerAction::SCENE_SWITCH:   f << "scene_switch";   break;
                case TriggerAction::SCENE_INSTANCE: f << "scene_instance"; break;
                case TriggerAction::DIALOGUE:       f << "dialogue";       break;
                case TriggerAction::SOUND:          f << "sound";          break;
                case TriggerAction::PARTICLES:      f << "particles";      break;
                case TriggerAction::CUSTOM:         f << "custom";         break;
                default:                            f << "none";           break;
            }
            f << "\",\n";
            f << "      \"target_scene\": \""   << esc(t.targetScene)   << "\",\n";
            f << "      \"spawn_offset_x\": "   << t.spawnOffsetX       << ",\n";
            f << "      \"spawn_offset_y\": "   << t.spawnOffsetY       << ",\n";
            f << "      \"dialogue\": \""        << esc(t.dialogueText)  << "\",\n";
            f << "      \"sound\": \""           << esc(t.soundName)     << "\",\n";
            f << "      \"level_req\": "         << t.levelRequirement   << ",\n";
            f << "      \"blocked_msg\": \""     << esc(t.blockedMessage)<< "\",\n";
            f << "      \"custom_tag\": \""      << esc(t.customTag)     << "\"\n";
            f << "    }";
            if (i+1 < triggers_.size()) f << ",";
            f << "\n";
        }
        f << "  ],\n  \"server_scenes\": [\n";
        bool first = true;
        for (auto& [id, ss] : serverScenes_) {
            if (!first) f << ",\n";
            first = false;
            f << "    {\n";
            f << "      \"id\": \""         << esc(ss.id)         << "\",\n";
            f << "      \"name\": \""       << esc(ss.displayName)<< "\",\n";
            f << "      \"map_file\": \""   << esc(ss.mapFile)    << "\",\n";
            f << "      \"max_players\": "  << ss.maxPlayers      << ",\n";
            f << "      \"level_req\": "    << ss.levelReq        << ",\n";
            f << "      \"level_max\": "    << ss.levelMax        << ",\n";
            f << "      \"cooldown\": "     << ss.cooldownSecs    << ",\n";
            f << "      \"pvp\": "          << (ss.pvp      ? "true":"false") << ",\n";
            f << "      \"instanced\": "    << (ss.instanced? "true":"false") << ",\n";
            f << "      \"boss\": \""       << esc(ss.bossType)   << "\",\n";
            f << "      \"exit_scene\": \"" << esc(ss.exitScene)  << "\",\n";
            f << "      \"exit_x\": "       << ss.exitSpawnX      << ",\n";
            f << "      \"exit_y\": "       << ss.exitSpawnY      << "\n";
            f << "    }";
        }
        f << "\n  ]\n}\n";
        std::cout << "[TriggerSystem] Сохранено → " << path << "\n";
        return true;
    }

    // ── Отладочный вывод активных триггеров ──────
    void debugDraw(/* sf::RenderWindow& window — подключи если нужно */) const {
        for (const auto& t : triggers_) {
            if (!t.active) continue;
            std::cout << "[T] " << t.id
                      << " @(" << t.x << "," << t.y << ")"
                      << " inside=" << t._playerInside << "\n";
        }
    }

    // ── Все триггеры (для редактора/рендера) ─────
    const std::vector<Trigger>& all() const { return triggers_; }
    std::vector<Trigger>&       all()       { return triggers_; }

    const std::map<std::string,ServerScene>& serverScenes() const { return serverScenes_; }
    std::map<std::string,ServerScene>&       serverScenes()       { return serverScenes_; }

private:
    std::vector<Trigger>              triggers_;
    std::map<std::string,ServerScene> serverScenes_;

    void fire(Trigger& t, int playerLevel = 1) {
        // Проверяем уровень
        if (t.levelRequirement > 0 && playerLevel < t.levelRequirement) {
            if (onLevelCheck) onLevelCheck(t.levelRequirement, t.blockedMessage);
            else if (onDialogue) onDialogue(t.blockedMessage, 3.f);
            return;
        }
        if (t.oneShot) t._fired = true;

        switch (t.action) {
        case TriggerAction::SCENE_SWITCH:
            if (onSceneSwitch && !t.targetScene.empty())
                onSceneSwitch(t.targetScene, t.spawnOffsetX, t.spawnOffsetY);
            break;
        case TriggerAction::SCENE_INSTANCE:
            enterServerScene(t.targetScene, playerLevel);
            break;
        case TriggerAction::DIALOGUE:
            if (onDialogue && !t.dialogueText.empty())
                onDialogue(t.dialogueText, 4.f);
            break;
        case TriggerAction::SOUND:
            if (onSound && !t.soundName.empty())
                onSound(t.soundName);
            break;
        case TriggerAction::PARTICLES:
            if (onParticles) onParticles(t.x, t.y);
            break;
        case TriggerAction::CUSTOM:
            if (t.callback) t.callback(t);
            break;
        default: break;
        }
        std::cout << "[Trigger] " << t.id << " fired → " << t.targetScene << "\n";
    }
};
