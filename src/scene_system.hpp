// scene_system.hpp — Система сцен/локаций AETHORIA
// Каждая сцена = отдельный map.json + entities
// Переключение: saveScene() → loadScene(name) → rebuild world

#pragma once
#include "json_parser.hpp"
#include "entity_system.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>

// ═══════════════════════════════════════════════════════════════
// ОПИСАНИЕ СЦЕНЫ (метаданные, без тайлов)
// ═══════════════════════════════════════════════════════════════
struct SceneInfo {
    std::string id;           // "aethoria_city", "dark_forest" …
    std::string displayName;  // "Aethoria City"
    std::string mapFile;      // "assets/scenes/aethoria_city.json"
    float spawnX = 60.f;      // spawn в тайлах (переопределяется картой)
    float spawnY = 60.f;
    int   levelMin = 0;
    int   levelMax = 999;
    bool  pvp      = false;
    bool  safe     = false;
    std::string music;        // "assets/music/city.ogg"
    sf::Color ambientColor    = sf::Color(30,20,50);
};

// ═══════════════════════════════════════════════════════════════
// ЗАГРУЖЕННАЯ СЦЕНА (данные для движка)
// ═══════════════════════════════════════════════════════════════
const int SCENE_MAP_W = 512;
const int SCENE_MAP_H = 512;

enum class SceneTileType {
    GRASS, DIRT, STONE_FLOOR, ROAD, WALL, WATER, TREE,
    BUILDING_FLOOR, ROOF_RED, ROOF_BLUE, ROOF_GREEN,
    FOUNTAIN, SAND, SNOW, LAVA, BRIDGE, DUNGEON_FLOOR,
    PORTAL, CHEST, ALTAR
};

struct SceneTile {
    SceneTileType type     = SceneTileType::GRASS;
    bool          walkable = true;
    sf::Color     color    = sf::Color(60,140,50);
    int           variant  = 0;
    std::string   tileId;   // для кастомных тайлов: оригинальный ID из JSON (напр. "MY_TILE")
};

// Описание врага из карты (заполняется при парсинге, используется spawnEnemies)
struct SceneEnemyDef {
    int         tileX    = 0;
    int         tileY    = 0;
    std::string typeName = "GOBLIN";
    int         level    = 1;
    bool        boss     = false;
    int         hp       = 0;
    int         dmg      = 0;
};

// ═══════════════════════════════════════════════════════════════
// СВЯЗЬ ПОРТАЛ → СЦЕНА (хранится в portal_links.json)
// ═══════════════════════════════════════════════════════════════
struct PortalLink {
    std::string portalId;       // ID портала-сущности в этой сцене (из entitySystem)
    std::string fromScene;      // сцена-источник
    std::string toScene;        // сцена-назначение
    float       spawnX = 60.f;  // куда поставить игрока (в тайлах) в toScene
    float       spawnY = 60.f;
    std::string label;          // отображаемое имя ("→ Тёмный лес")
    bool        bidirectional = false; // создать обратный портал автоматически
};

struct SceneData {
    std::string            id;
    std::string            name;
    int                    width  = SCENE_MAP_W;
    int                    height = SCENE_MAP_H;
    SceneTile              tiles[SCENE_MAP_H][SCENE_MAP_W];
    float                  spawnX = 60.f * 32;
    float                  spawnY = 60.f * 32;
    std::vector<sf::Vector2i>  fountains;
    std::vector<SceneEnemyDef> enemyDefs;   // ← враги из JSON
    std::vector<PortalLink>    portalLinks; // ← связи порталов
    bool loaded = false;
};

// ═══════════════════════════════════════════════════════════════
// КОНВЕРТЕР: строка → SceneTile
// ═══════════════════════════════════════════════════════════════
namespace SceneUtils {
    inline SceneTile tileFromStr(const std::string& s) {
        using T = SceneTileType;
        struct Desc { T type; bool w; sf::Color c; };
        static const std::map<std::string,Desc> tab = {
            {"GRASS",         {T::GRASS,         true,  sf::Color(60,140,50)}},
            {"DIRT",          {T::DIRT,           true,  sf::Color(110,75,45)}},
            {"STONE_FLOOR",   {T::STONE_FLOOR,   true,  sf::Color(130,122,110)}},
            {"ROAD",          {T::ROAD,           true,  sf::Color(160,150,135)}},
            {"WALL",          {T::WALL,           false, sf::Color(120,105,90)}},
            {"WATER",         {T::WATER,          false, sf::Color(50,110,190)}},
            {"TREE",          {T::TREE,           false, sf::Color(30,80,20)}},
            {"BUILDING_FLOOR",{T::BUILDING_FLOOR, false, sf::Color(100,90,78)}},
            {"FOUNTAIN",      {T::FOUNTAIN,       true,  sf::Color(80,140,200)}},
            {"ROOF_RED",      {T::ROOF_RED,       false, sf::Color(160,60,50)}},
            {"ROOF_BLUE",     {T::ROOF_BLUE,      false, sf::Color(50,80,180)}},
            {"ROOF_GREEN",    {T::ROOF_GREEN,     false, sf::Color(50,140,60)}},
            {"SAND",          {T::SAND,           true,  sf::Color(190,165,90)}},
            {"SNOW",          {T::SNOW,           true,  sf::Color(220,230,245)}},
            {"LAVA",          {T::LAVA,           false, sf::Color(200,60,10)}},
            {"BRIDGE",        {T::BRIDGE,         true,  sf::Color(130,100,60)}},
            {"DUNGEON_FLOOR", {T::DUNGEON_FLOOR,  true,  sf::Color(55,50,75)}},
            {"PORTAL",        {T::PORTAL,         true,  sf::Color(150,60,220)}},
            {"CHEST",         {T::CHEST,          false, sf::Color(190,130,30)}},
            {"ALTAR",         {T::ALTAR,          false, sf::Color(130,80,50)}},
        };
        auto it = tab.find(s);
        if (it != tab.end()) {
            SceneTile t;
            t.type = it->second.type; t.walkable = it->second.w;
            t.color = it->second.c;
            return t;
        }

        // ── Кастомные тайлы из assets/custom_tiles.json ──────────
        // Парсим цвет из sfml-строки "R,G,B" или hex "#rrggbb"
        {
            static std::map<std::string, SceneTile> customCache;
            static bool customLoaded = false;
            if (!customLoaded) {
                customLoaded = true;
                std::ifstream cf("assets/custom_tiles.json");
                if (cf.is_open()) {
                    std::stringstream buf; buf << cf.rdbuf();
                    auto root = SimpleJSON::Parser::parse(buf.str());
                    if (root && root->isArray()) {
                        for (size_t i = 0; i < root->arrayVal.size(); i++) {
                            auto j = root->get(i);
                            if (!j) continue;
                            std::string tid = j->get("id") ? j->get("id")->asString() : "";
                            if (tid.empty()) continue;
                            SceneTile ct;
                            // walkable
                            ct.walkable = j->get("walkable") ? j->get("walkable")->asBool() : true;
                            // texture path (stored as string in variant=99 signal)
                            ct.variant = 99;  // сигнал: кастомный тайл с текстурой
                            // цвет из "sfml": "R,G,B"
                            sf::Color col(128,128,128);
                            std::string sfmlStr = j->get("sfml") ? j->get("sfml")->asString() : "";
                            if (!sfmlStr.empty()) {
                                int r=128,g=128,b=128;
                                sscanf(sfmlStr.c_str(), "%d,%d,%d", &r, &g, &b);
                                col = sf::Color(
                                    (uint8_t)std::clamp(r,0,255),
                                    (uint8_t)std::clamp(g,0,255),
                                    (uint8_t)std::clamp(b,0,255));
                            }
                            ct.color = col;
                            // Тип — кастомные используют DUNGEON_FLOOR как базовый
                            ct.type = SceneTileType::DUNGEON_FLOOR;
                            customCache[tid] = ct;
                        }
                        std::cout << "[SceneUtils] Загружено custom_tiles: "
                                  << customCache.size() << "\n";
                    }
                }
            }
            auto cit = customCache.find(s);
            if (cit != customCache.end()) {
                SceneTile result = cit->second;
                result.tileId = s;  // сохраняем ID для рендеринга текстуры
                return result;
            }
        }

        // Неизвестный тайл → трава (без краша)
        SceneTile fallback;
        fallback.color = sf::Color(60,140,50);
        std::cout << "[WARN] Неизвестный тайл '" << s << "' → GRASS\n";
        return fallback;
    }

    // Slugify display name → file id: "Dark Forest" → "dark_forest"
    inline std::string toId(const std::string& name) {
        std::string r;
        for (char c : name) {
            if (std::isalnum((unsigned char)c)) r += std::tolower((unsigned char)c);
            else if (!r.empty() && r.back() != '_') r += '_';
        }
        while (!r.empty() && r.back()=='_') r.pop_back();
        return r;
    }

    inline std::string sceneFile(const std::string& id) {
        return "assets/scenes/" + id + ".json";
    }
}

// ═══════════════════════════════════════════════════════════════
// МЕНЕДЖЕР СЦЕН
// ═══════════════════════════════════════════════════════════════
class SceneManager {
public:
    // Callbacks — движок подключает их при инициализации
    std::function<void(const SceneData&, EntitySystem&)> onSceneLoaded;
    std::function<void()>                                 onSceneUnload;

    // ── Регистрация сцен ──────────────────────────────────────
    void registerScene(const SceneInfo& info) {
        registry_[info.id] = info;
        // Сохраняем порядок
        if (std::find(order_.begin(),order_.end(),info.id)==order_.end())
            order_.push_back(info.id);
    }

    // ── Загрузка / сохранение связей порталов ────────────────
    // Формат portal_links.json:
    // [{"portal_id":"p01","from":"aethoria_city","to":"dark_forest",
    //   "spawn_x":10,"spawn_y":20,"label":"→ Лес","bidirectional":false}, ...]
    bool loadPortalLinks(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isArray()) return false;
        portalLinks_.clear();
        for (size_t i = 0; i < root->arrayVal.size(); i++) {
            auto j = root->get(i);
            if (!j) continue;
            PortalLink pl;
            if (auto v = j->get("portal_id"))    pl.portalId      = v->asString();
            if (auto v = j->get("from"))          pl.fromScene     = v->asString();
            if (auto v = j->get("to"))            pl.toScene       = v->asString();
            if (auto v = j->get("spawn_x"))       pl.spawnX        = (float)v->asDouble();
            if (auto v = j->get("spawn_y"))       pl.spawnY        = (float)v->asDouble();
            if (auto v = j->get("label"))         pl.label         = v->asString();
            if (auto v = j->get("bidirectional")) pl.bidirectional = v->asBool();
            portalLinks_.push_back(pl);
            // Если двунаправленный — создаём обратную связь автоматически
            if (pl.bidirectional) {
                PortalLink rev;
                rev.portalId  = pl.portalId + "_back";
                rev.fromScene = pl.toScene;
                rev.toScene   = pl.fromScene;
                rev.spawnX    = pl.spawnX;
                rev.spawnY    = pl.spawnY;
                rev.label     = "↩ " + pl.fromScene;
                portalLinks_.push_back(rev);
            }
        }
        _applyPortalLinksToScene(current_);
        std::cout << "[SceneManager] Portal links: " << portalLinks_.size()
                  << " из " << path << "\n";
        return true;
    }

    bool savePortalLinks(const std::string& path) const {
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
        f << "[\n";
        bool firstWritten = false;
        for (auto& pl : portalLinks_) {
            // Пропускаем авто-сгенерированные обратные (_back)
            if (pl.portalId.size() >= 5 &&
                pl.portalId.compare(pl.portalId.size()-5, 5, "_back") == 0)
                continue;
            if (firstWritten) f << ",\n";
            firstWritten = true;
            f << "  {\n";
            f << "    \"portal_id\": \""    << esc(pl.portalId)  << "\",\n";
            f << "    \"from\": \""          << esc(pl.fromScene) << "\",\n";
            f << "    \"to\": \""            << esc(pl.toScene)   << "\",\n";
            f << "    \"spawn_x\": "         << pl.spawnX         << ",\n";
            f << "    \"spawn_y\": "         << pl.spawnY         << ",\n";
            f << "    \"label\": \""         << esc(pl.label)     << "\",\n";
            f << "    \"bidirectional\": "   << (pl.bidirectional ? "true":"false") << "\n";
            f << "  }";
        }
        f << "\n]\n";
        std::cout << "[SceneManager] Portal links сохранены → " << path << "\n";
        return true;
    }

    void setPortalLink(const PortalLink& pl) {
        for (auto& ex : portalLinks_) {
            if (ex.portalId == pl.portalId) { ex = pl; return; }
        }
        portalLinks_.push_back(pl);
    }

    std::vector<PortalLink> getLinksForScene(const std::string& sceneId) const {
        std::vector<PortalLink> r;
        for (auto& pl : portalLinks_)
            if (pl.fromScene == sceneId) r.push_back(pl);
        return r;
    }

    const PortalLink* getLinkByPortalId(const std::string& id) const {
        for (auto& pl : portalLinks_)
            if (pl.portalId == id) return &pl;
        return nullptr;
    }

    const std::vector<PortalLink>& allPortalLinks() const { return portalLinks_; }
    std::vector<PortalLink>&       allPortalLinks()       { return portalLinks_; }

    // Загрузить список сцен из game_config.json → "location.zones"
    void loadFromConfig(const std::string& configPath) {
        std::ifstream f(configPath);
        if (!f.is_open()) return;
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root) return;
        auto loc = root->get("location");
        if (!loc) return;
        auto zones = loc->get("zones");
        if (!zones || !zones->isArray()) return;

        std::string active;
        if (auto az = loc->get("active_zone")) active = az->asString();

        for (size_t i = 0; i < zones->arrayVal.size(); i++) {
            auto z = zones->get(i);
            if (!z) continue;
            std::string name = z->asString();
            std::string id   = SceneUtils::toId(name);
            if (registry_.count(id)) continue;   // уже зарегистрирована
            SceneInfo info;
            info.id          = id;
            info.displayName = name;
            info.mapFile     = SceneUtils::sceneFile(id);
            registerScene(info);
        }

        if (!active.empty()) {
            std::string aid = SceneUtils::toId(active);
            if (registry_.count(aid)) defaultSceneId_ = aid;
        }
    }

    // ── Список всех сцен ──────────────────────────────────────
    const std::vector<std::string>& sceneOrder() const { return order_; }

    SceneInfo* getInfo(const std::string& id) {
        auto it = registry_.find(id);
        return it != registry_.end() ? &it->second : nullptr;
    }

    std::string currentId() const { return currentId_; }
    std::string defaultId() const { return defaultSceneId_; }

    bool isLoaded() const { return current_.loaded; }
    SceneData& current() { return current_; }

    // ── Загрузка сцены ────────────────────────────────────────
    bool loadScene(const std::string& id, EntitySystem& es) {
        auto it = registry_.find(id);
        if (it == registry_.end()) {
            std::cerr << "[SceneManager] Сцена не найдена: " << id << "\n";
            return false;
        }

        // FIX: НЕ очищаем весь entitySystem — сохраняем PLAYER сущность.
        // onSceneUnload (в GameEngine) уже удалит NPC/OBJECT/PORTAL/ENEMY.
        // es.clear() здесь приводило к потере playerEntityId → NPC внутри игрока.
        // Выгружаем текущую (коллбэк очищает NPC/enemy/portal из entitySystem)
        if (current_.loaded && onSceneUnload) onSceneUnload();
        // Оставляем только PLAYER (на случай если onSceneUnload не убрал всё):
        {
            auto& all = es.getAll();
            all.erase(std::remove_if(all.begin(), all.end(), [](const Entity& e){
                return e.type != EntityType::PLAYER;
            }), all.end());
        }

        const SceneInfo& info = it->second;
        std::string path = info.mapFile;

        std::ifstream f(path);
        if (!f.is_open()) {
            // Файл не создан — генерируем пустую сцену
            std::cout << "[SceneManager] Файл не найден: " << path
                      << " — создаю пустую сцену\n";
            buildEmpty(current_, info);
            currentId_ = id;
            current_.loaded = true;
            if (onSceneLoaded) onSceneLoaded(current_, es);
            return true;
        }

        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isObject()) {
            std::cerr << "[SceneManager] Невалидный JSON: " << path << "\n";
            return false;
        }

        // Базовые поля
        current_.id   = id;
        current_.name = root->get("name") ? root->get("name")->asString() : info.displayName;
        current_.width  = root->get("width")  ? root->get("width")->asInt()  : SCENE_MAP_W;
        current_.height = root->get("height") ? root->get("height")->asInt() : SCENE_MAP_H;

        // Тайлы
        auto tilesNode = root->get("tiles");
        // Сначала GRASS всюду
        for (int y=0;y<SCENE_MAP_H;y++)
            for (int x=0;x<SCENE_MAP_W;x++)
                current_.tiles[y][x] = SceneTile{};

        if (tilesNode && tilesNode->isArray()) {
            for (size_t y=0; y<tilesNode->arrayVal.size() && (int)y<SCENE_MAP_H; y++) {
                auto row = tilesNode->get(y);
                if (!row || !row->isArray()) continue;
                for (size_t x=0; x<row->arrayVal.size() && (int)x<SCENE_MAP_W; x++) {
                    auto cell = row->get(x);
                    if (cell && cell->isString())
                        current_.tiles[y][x] = SceneUtils::tileFromStr(cell->asString());
                }
            }
        }

        // Коллизии (override walkable)
        if (auto coll = root->get("collision")) {
            if (coll->isArray()) {
                for (size_t y=0; y<coll->arrayVal.size() && (int)y<SCENE_MAP_H; y++) {
                    auto row = coll->get(y);
                    if (!row||!row->isArray()) continue;
                    for (size_t x=0; x<row->arrayVal.size() && (int)x<SCENE_MAP_W; x++) {
                        auto cell = row->get(x);
                        if (cell) current_.tiles[y][x].walkable = (cell->asInt()==0);
                    }
                }
            }
        }

        // Spawn из metadata
        current_.spawnX = info.spawnX * 32.f;
        current_.spawnY = info.spawnY * 32.f;
        if (auto meta = root->get("metadata")) {
            if (auto sx = meta->get("player_spawn_x"))
                current_.spawnX = (float)sx->asDouble() * 32.f;
            if (auto sy = meta->get("player_spawn_y"))
                current_.spawnY = (float)sy->asDouble() * 32.f;
        }

        // Сущности
        current_.fountains.clear();
        current_.enemyDefs.clear();   // ← сбрасываем перед загрузкой
        if (auto ents = root->get("entities")) {
            for (size_t i=0; i<ents->arrayVal.size(); i++) {
                auto e = ents->get(i);
                if (!e) continue;
                std::string kind = e->get("kind") ? e->get("kind")->asString() : "";
                int ex = e->get("x") ? e->get("x")->asInt() : 0;
                int ey = e->get("y") ? e->get("y")->asInt() : 0;
                parseEntity(kind, ex, ey, e, es, current_);
            }
        }
        if (current_.fountains.empty())
            current_.fountains.push_back({current_.width/2, current_.height/2});

        currentId_      = id;
        current_.loaded = true;
        _applyPortalLinksToScene(current_);
        std::cout << "[SceneManager] Загружена сцена: " << info.displayName
                  << "  (" << current_.width << "x" << current_.height
                  << "  fontains=" << current_.fountains.size() << ")\n";

        if (onSceneLoaded) onSceneLoaded(current_, es);
        return true;
    }

    // Загрузить дефолтную / первую сцену
    bool loadDefault(EntitySystem& es) {
        if (!defaultSceneId_.empty() && registry_.count(defaultSceneId_))
            return loadScene(defaultSceneId_, es);
        if (!order_.empty())
            return loadScene(order_.front(), es);
        return false;
    }

    // ── Сохранение сцены ──────────────────────────────────────
    // Сохраняет текущий worldMap (передаётся снаружи) + сущности
    bool saveScene(const std::string& id,
                   const SceneTile tiles[SCENE_MAP_H][SCENE_MAP_W],
                   int w, int h, float spawnX, float spawnY,
                   const EntitySystem& es)
    {
        auto it = registry_.find(id);
        if (it == registry_.end()) return false;

        std::string path = it->second.mapFile;
        // Создаём папку если надо
        std::string dir = path.substr(0, path.rfind('/'));
        ensureDir(dir);

        std::ofstream f(path);
        if (!f.is_open()) {
            std::cerr << "[SceneManager] Не открыть для записи: " << path << "\n";
            return false;
        }

        f << "{\n";
        f << "  \"version\": \"4.0\",\n";
        f << "  \"name\": \"" << it->second.displayName << "\",\n";
        f << "  \"width\": " << w << ",\n";
        f << "  \"height\": " << h << ",\n";
        f << "  \"metadata\": {\n";
        f << "    \"player_spawn_x\": " << (int)(spawnX/32) << ",\n";
        f << "    \"player_spawn_y\": " << (int)(spawnY/32) << "\n";
        f << "  },\n";

        // tiles
        f << "  \"tiles\": [\n";
        for (int y=0;y<h;y++) {
            f << "    [";
            for (int x=0;x<w;x++) {
                const auto& st = tiles[y][x];
                // Кастомный тайл — сохраняем его оригинальный ID, а не тип
                std::string tstr = st.tileId.empty() ? tileTypeToStr(st.type) : st.tileId;
                f << "\"" << tstr << "\"";
                if (x<w-1) f << ",";
            }
            f << "]";
            if (y<h-1) f << ",";
            f << "\n";
        }
        f << "  ],\n";

        // collision
        f << "  \"collision\": [\n";
        for (int y=0;y<h;y++) {
            f << "    [";
            for (int x=0;x<w;x++) {
                f << (tiles[y][x].walkable ? 0 : 1);
                if (x<w-1) f << ",";
            }
            f << "]";
            if (y<h-1) f << ",";
            f << "\n";
        }
        f << "  ],\n";

        // entities  FIX: убрано дублирование ключа "entities" (было 2 строки — невалидный JSON)
        f << "  \"entities\": [\n";

const auto& all = es.getAll();
bool firstEntity = true;

for (size_t i = 0; i < all.size(); i++) {
    const auto& e = all[i];

    // ❗ НЕ сохраняем игрока
    if (!e.active || e.type == EntityType::PLAYER)
        continue;

    if (!firstEntity) f << ",";
    firstEntity = false;

    f << "\n    {";
    f << "\"kind\":\"" << entityKindStr(e.type) << "\",";
    f << "\"type\":\"" << e.props.getStr("subtype", entityKindStr(e.type)) << "\",";
    f << "\"x\":" << (int)(e.x/32) << ",";
    f << "\"y\":" << (int)(e.y/32) << ",";
    f << "\"name\":\"" << escStr(e.name) << "\",";
    f << "\"props\":{";

    bool first = true;
    for (auto& [k,v]:e.props.strings) { if(!first)f<<","; first=false; f<<"\"" <<k<<"\":\"" <<escStr(v)<<"\""; }
    for (auto& [k,v]:e.props.ints)    { if(!first)f<<","; first=false; f<<"\"" <<k<<"\":" <<v; }
    for (auto& [k,v]:e.props.floats)  { if(!first)f<<","; first=false; f<<"\"" <<k<<"\":" <<v; }
    for (auto& [k,v]:e.props.bools)   { if(!first)f<<","; first=false; f<<"\"" <<k<<"\":" <<(v?"true":"false"); }

    f << "}}";
}

f << "\n  ]\n}\n";

std::cout << "[SceneManager] Сохранена сцена: " << path << "\n";
return true;
    }

    // ── Переключение с плавным переходом ─────────────────────
    // Вернёт true если переключение началось; следи за switchPending
    bool switchScene(const std::string& toId, EntitySystem& es,
                     const SceneTile curTiles[SCENE_MAP_H][SCENE_MAP_W],
                     int w, int h, float spawnX, float spawnY)
    {
        if (!registry_.count(toId)) {
            std::cerr << "[SceneManager] switchScene: неизвестная сцена " << toId << "\n";
            return false;
        }
        // Сначала сохраняем текущую
        if (!currentId_.empty())
            saveScene(currentId_, curTiles, w, h, spawnX, spawnY, es);
        // Загружаем новую
        return loadScene(toId, es);
    }

private:
    std::map<std::string,SceneInfo>  registry_;
    std::vector<std::string>         order_;
    std::string                      currentId_;
    std::string                      defaultSceneId_ = "aethoria_city";
    SceneData                        current_;
    std::vector<PortalLink>          portalLinks_;

    // Применяет связи порталов к загруженной сцене (props сущностей)
    void _applyPortalLinksToScene(SceneData& sd) {
        sd.portalLinks.clear();
        for (auto& pl : portalLinks_)
            if (pl.fromScene == sd.id)
                sd.portalLinks.push_back(pl);
    }

    void buildEmpty(SceneData& sd, const SceneInfo& info) {
        sd.id = info.id; sd.name = info.displayName;
        sd.width = SCENE_MAP_W; sd.height = SCENE_MAP_H;
        for (int y=0;y<SCENE_MAP_H;y++)
            for (int x=0;x<SCENE_MAP_W;x++)
                sd.tiles[y][x] = SceneTile{};
        sd.spawnX = info.spawnX*32.f; sd.spawnY = info.spawnY*32.f;
        sd.fountains.push_back({SCENE_MAP_W/2, SCENE_MAP_H/2});
    }

    void parseEntity(const std::string& kind, int ex, int ey,
                     const std::shared_ptr<SimpleJSON::Value>& e,
                     EntitySystem& es, SceneData& sd)
    {
        float wx = ex*32.f+16.f, wy = ey*32.f+16.f;
        std::string nm   = e->get("name") ? e->get("name")->asString() : kind;
        std::string type = e->get("type") ? e->get("type")->asString() : "";

        EntityProperties ep;
        ep.setStr("subtype", type.empty() ? kind : type);
        if (auto props = e->get("props")) {
            for (auto& [k,v]:props->objectVal) {
                if (!v) continue;
                if      (v->isBool())   ep.setBool(k, v->asBool());
                else if (v->isNumber()) {
                    // Try to detect int vs float
                    double d=v->asDouble();
                    if(d==(int)d) ep.setInt(k,(int)d);
                    else          ep.setFloat(k,(float)d);
                }
                else if (v->isString()) ep.setStr(k, v->asString());
            }
        }

        if      (kind=="enemy") {
            // Сохраняем в SceneData.enemyDefs — spawnEnemies заберёт отсюда
            SceneEnemyDef def;
            def.tileX    = ex;
            def.tileY    = ey;
            def.typeName = type.empty() ? "GOBLIN" : type;
            def.level    = ep.getInt("level", 1);
            def.boss     = ep.getBool("boss", false);
            def.hp       = ep.getInt("hp", 0);
            def.dmg      = ep.getInt("dmg", 0);
            // Поддержка dmg/damage — редактор пишет "dmg", проверяем оба
            if (def.dmg == 0) def.dmg = ep.getInt("damage", 0);
            sd.enemyDefs.push_back(def);
        }
        else if (kind=="npc")     { ep.setBool("interactable",true); es.addEntityFull(EntityType::NPC,    wx,wy,nm,ep); }
        else if (kind=="portal")  { es.addEntityFull(EntityType::PORTAL,  wx,wy,nm,ep); }
        else if (kind=="object")  { ep.setBool("opened",ep.getBool("opened",false)); es.addEntityFull(EntityType::OBJECT,  wx,wy,nm,ep); }
        else if (kind=="fountain"){ sd.fountains.push_back({ex,ey}); }
        else if (kind=="zone") {
            // Зоны не в entity system — просто фонтаны/спавн
            // Используем центр тайла: ex*32 + 16
            if (type=="SPAWN") { sd.spawnX = ex*32.f; sd.spawnY = ey*32.f; }
            else if (type=="FOUNTAIN"||type=="SAFE") sd.fountains.push_back({ex,ey});
        }
    }

    static std::string tileTypeToStr(SceneTileType t) {
        using T=SceneTileType;
        switch(t) {
            case T::GRASS:         return "GRASS";
            case T::DIRT:          return "DIRT";
            case T::STONE_FLOOR:   return "STONE_FLOOR";
            case T::ROAD:          return "ROAD";
            case T::WALL:          return "WALL";
            case T::WATER:         return "WATER";
            case T::TREE:          return "TREE";
            case T::BUILDING_FLOOR:return "BUILDING_FLOOR";
            case T::FOUNTAIN:      return "FOUNTAIN";
            case T::ROOF_RED:      return "ROOF_RED";
            case T::ROOF_BLUE:     return "ROOF_BLUE";
            case T::ROOF_GREEN:    return "ROOF_GREEN";
            case T::SAND:          return "SAND";
            case T::SNOW:          return "SNOW";
            case T::LAVA:          return "LAVA";
            case T::BRIDGE:        return "BRIDGE";
            case T::DUNGEON_FLOOR: return "DUNGEON_FLOOR";
            case T::PORTAL:        return "PORTAL";
            case T::CHEST:         return "CHEST";
            case T::ALTAR:         return "ALTAR";
            default:               return "GRASS";
        }
    }

    static std::string entityKindStr(EntityType t) {
        switch(t) {
			case EntityType::PLAYER:   return "player";
            case EntityType::NPC:      return "npc";
            case EntityType::OBJECT:   return "object";
            case EntityType::PORTAL:   return "portal";
            case EntityType::ITEM_DROP:return "item_drop";
            case EntityType::ENEMY:    return "enemy";
            default:                   return "object";
        }
    }

    static std::string escStr(const std::string& s) {
        std::string o;
        for (char c:s) {
            if(c=='"') o+="\\\"";
            else if(c=='\\') o+="\\\\";
            else if(c=='\n') o+="\\n";
            else o+=c;
        }
        return o;
    }

    static void ensureDir(const std::string& path) {
#ifdef _WIN32
        std::string cmd = "if not exist \"" + path + "\" mkdir \"" + path + "\"";
#else
        std::string cmd = "mkdir -p \"" + path + "\"";
#endif
        std::system(cmd.c_str());
    }
};
