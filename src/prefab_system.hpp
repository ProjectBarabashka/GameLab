// prefab_system.hpp — Система префабов AETHORIA: Eternal Realms
// Шаблоны объектов, каталог, инстанцирование, переопределение параметров
// Совместимо с entity_system.hpp / json_parser.hpp

#pragma once
#include "entity_system.hpp"
#include "json_parser.hpp"

#include <map>
#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <algorithm>
#include <optional>

// ════════════════════════════════════════════════════════════════════
// КАТЕГОРИИ ПРЕФАБОВ
// ════════════════════════════════════════════════════════════════════
enum class PrefabCategory {
    ENEMY,      // враги
    NPC,        // мирные жители, торговцы, квестодатели
    OBJECT,     // интерактивные объекты (сундуки, бочки, алтари)
    PORTAL,     // порталы / точки перехода
    ITEM_DROP,  // дроп предметов
    BOSS,       // боссы (подтип ENEMY, выделен отдельно для удобства)
    TRAP,       // ловушки
    CUSTOM      // пользовательские
};

inline std::string prefabCategoryToStr(PrefabCategory c) {
    switch (c) {
        case PrefabCategory::ENEMY:     return "enemy";
        case PrefabCategory::NPC:       return "npc";
        case PrefabCategory::OBJECT:    return "object";
        case PrefabCategory::PORTAL:    return "portal";
        case PrefabCategory::ITEM_DROP: return "item_drop";
        case PrefabCategory::BOSS:      return "boss";
        case PrefabCategory::TRAP:      return "trap";
        default:                        return "custom";
    }
}

inline PrefabCategory strToPrefabCategory(const std::string& s) {
    if (s == "enemy")     return PrefabCategory::ENEMY;
    if (s == "npc")       return PrefabCategory::NPC;
    if (s == "object")    return PrefabCategory::OBJECT;
    if (s == "portal")    return PrefabCategory::PORTAL;
    if (s == "item_drop") return PrefabCategory::ITEM_DROP;
    if (s == "boss")      return PrefabCategory::BOSS;
    if (s == "trap")      return PrefabCategory::TRAP;
    return PrefabCategory::CUSTOM;
}

// ════════════════════════════════════════════════════════════════════
// СТРУКТУРА ПЕРЕОПРЕДЕЛЕНИЙ (Override)
// Позволяет при спавне изменить отдельные поля шаблона
// ════════════════════════════════════════════════════════════════════
struct PrefabOverride {
    std::map<std::string, std::string> strings;
    std::map<std::string, float>       floats;
    std::map<std::string, int>         ints;
    std::map<std::string, bool>        bools;
    std::optional<std::string>         nameOverride;

    // Цепочечный DSL для удобного заполнения
    PrefabOverride& str  (const std::string& k, const std::string& v){ strings[k]=v; return *this; }
    PrefabOverride& flt  (const std::string& k, float v)             { floats[k]=v;  return *this; }
    PrefabOverride& num  (const std::string& k, int v)               { ints[k]=v;    return *this; }
    PrefabOverride& flag (const std::string& k, bool v)              { bools[k]=v;   return *this; }
    PrefabOverride& name (const std::string& n)                      { nameOverride=n; return *this; }
};

// ════════════════════════════════════════════════════════════════════
// ШАБЛОН ПРЕФАБА
// ════════════════════════════════════════════════════════════════════
struct PrefabTemplate {
    // ── Идентификация ────────────────────────────────────────────
    std::string     id;           // уникальный ключ, напр. "goblin_warrior"
    std::string     displayName;  // для UI редактора: "Goblin Warrior"
    PrefabCategory  category;
    EntityType      entityType;

    // ── Визуал ───────────────────────────────────────────────────
    std::string     spriteKey;    // ключ в animations.json, напр. "goblin_warrior"
    std::string     iconPath;     // путь к иконке для UI редактора (PNG 32×32)
    int             tileWidth  = 32;
    int             tileHeight = 32;

    // ── Базовые свойства (все параметры по умолчанию) ────────────
    EntityProperties defaultProps;

    // ── Список разрешённых полей для переопределения ─────────────
    // Если пуст — разрешены все поля
    std::set<std::string> overridableFields;

    // ── Метаданные ───────────────────────────────────────────────
    std::string description;   // для редактора
    std::string author;        // кто создал
    int         version = 1;
    std::vector<std::string> tags; // для фильтрации: {"undead","melee","forest"}

    // ── Spawn-хук (опциональная логика после спавна) ─────────────
    // Устанавливается в C++ коде, не сериализуется
    std::function<void(Entity&)> onSpawn;

    // ── Проверка разрешённости поля ──────────────────────────────
    bool canOverride(const std::string& field) const {
        return overridableFields.empty() || overridableFields.count(field) > 0;
    }

    // ── Применение override к свойствам ──────────────────────────
    EntityProperties applyOverride(const PrefabOverride& ov) const {
        EntityProperties props = defaultProps;
        for (auto& [k, v] : ov.strings) if (canOverride(k)) props.strings[k] = v;
        for (auto& [k, v] : ov.floats)  if (canOverride(k)) props.floats[k]  = v;
        for (auto& [k, v] : ov.ints)    if (canOverride(k)) props.ints[k]    = v;
        for (auto& [k, v] : ov.bools)   if (canOverride(k)) props.bools[k]   = v;
        return props;
    }
};

// ════════════════════════════════════════════════════════════════════
// РЕЗУЛЬТАТ СПАВНА
// ════════════════════════════════════════════════════════════════════
struct SpawnResult {
    bool      success = false;
    uint32_t  entityId = 0;
    std::string prefabId;
    std::string errorMsg;

    explicit operator bool() const { return success; }
};

// ════════════════════════════════════════════════════════════════════
// КАТАЛОГ ПРЕФАБОВ
// ════════════════════════════════════════════════════════════════════
class PrefabCatalog {
private:
    std::map<std::string, PrefabTemplate> templates;  // id → шаблон
    std::string catalogPath;

    // ── JSON helpers ─────────────────────────────────────────────
    static std::string escJ(const std::string& s) {
        std::string o;
        for (char c : s) {
            if      (c == '"')  o += "\\\"";
            else if (c == '\\') o += "\\\\";
            else if (c == '\n') o += "\\n";
            else if (c == '\t') o += "\\t";
            else                o += c;
        }
        return o;
    }

    static void writePropsJSON(std::ofstream& f, const EntityProperties& p, int indent) {
        std::string pad(indent, ' ');

        // strings
        f << pad << "\"strings\": {";
        bool first = true;
        for (auto& [k, v] : p.strings) {
            if (!first) f << ", "; first = false;
            f << "\"" << escJ(k) << "\": \"" << escJ(v) << "\"";
        }
        f << "},\n";

        // floats
        f << pad << "\"floats\": {";
        first = true;
        for (auto& [k, v] : p.floats) {
            if (!first) f << ", "; first = false;
            f << "\"" << escJ(k) << "\": " << v;
        }
        f << "},\n";

        // ints
        f << pad << "\"ints\": {";
        first = true;
        for (auto& [k, v] : p.ints) {
            if (!first) f << ", "; first = false;
            f << "\"" << escJ(k) << "\": " << v;
        }
        f << "},\n";

        // bools
        f << pad << "\"bools\": {";
        first = true;
        for (auto& [k, v] : p.bools) {
            if (!first) f << ", "; first = false;
            f << "\"" << escJ(k) << "\": " << (v ? "true" : "false");
        }
        f << "}\n";
    }

    static EntityProperties parsePropsJSON(const SimpleJSON::Value* propsNode) {
        EntityProperties props;
        if (!propsNode) return props;

        if (auto s = propsNode->get("strings"))
            for (auto& [k, v] : s->objectVal) if (v) props.strings[k] = v->asString();
        if (auto s = propsNode->get("floats"))
            for (auto& [k, v] : s->objectVal) if (v) props.floats[k]  = (float)v->asDouble();
        if (auto s = propsNode->get("ints"))
            for (auto& [k, v] : s->objectVal) if (v) props.ints[k]    = v->asInt();
        if (auto s = propsNode->get("bools"))
            for (auto& [k, v] : s->objectVal) if (v) props.bools[k]   = v->asBool();

        return props;
    }

public:
    explicit PrefabCatalog(const std::string& path = "assets/prefabs.json")
        : catalogPath(path) {}

    // ════════════════════════════════════════════════════════════
    // РЕГИСТРАЦИЯ И ПОЛУЧЕНИЕ
    // ════════════════════════════════════════════════════════════

    void registerPrefab(PrefabTemplate tmpl) {
        std::string key = tmpl.id;
        templates[key] = std::move(tmpl);
        std::cout << "[Prefab] Зарегистрирован: " << key << "\n";
    }

    PrefabTemplate* get(const std::string& id) {
        auto it = templates.find(id);
        return it != templates.end() ? &it->second : nullptr;
    }

    const PrefabTemplate* get(const std::string& id) const {
        auto it = templates.find(id);
        return it != templates.end() ? &it->second : nullptr;
    }

    bool has(const std::string& id) const { return templates.count(id) > 0; }

    void remove(const std::string& id) { templates.erase(id); }

    // Переименование (для редактора)
    bool rename(const std::string& oldId, const std::string& newId) {
        if (!has(oldId) || has(newId)) return false;
        auto node = templates.extract(oldId);
        node.key() = newId;
        node.mapped().id = newId;
        templates.insert(std::move(node));
        return true;
    }

    // ── Клонирование ─────────────────────────────────────────────

    // Клонирует prefab sourceId под новым именем newId.
    // Возвращает false если источник не найден или newId уже занят.
    // Пример: clone("goblin_warrior", "goblin_warrior_elite")
    bool clone(const std::string& sourceId, const std::string& newId) {
        if (!has(sourceId)) {
            std::cerr << "[Prefab] clone: источник не найден: " << sourceId << "\n";
            return false;
        }
        if (has(newId)) {
            std::cerr << "[Prefab] clone: ID уже занят: " << newId << "\n";
            return false;
        }
        PrefabTemplate t = templates.at(sourceId);
        t.id = newId;
        // Сбрасываем onSpawn-хук — не копируется, задаётся отдельно
        t.onSpawn = nullptr;
        templates[newId] = std::move(t);
        std::cout << "[Prefab] Клонирован: " << sourceId << " → " << newId << "\n";
        return true;
    }

    // Клонирует с автоматическим числовым суффиксом.
    // "goblin_warrior" → "goblin_warrior_02", "_03" и т.д.
    // Возвращает новый ID или "" при ошибке.
    // Пример: std::string id = catalog.cloneNumbered("goblin_warrior");
    std::string cloneNumbered(const std::string& sourceId) {
        if (!has(sourceId)) {
            std::cerr << "[Prefab] cloneNumbered: источник не найден: " << sourceId << "\n";
            return "";
        }
        char buf[8];
        for (int n = 2; n <= 99; n++) {
            std::snprintf(buf, sizeof(buf), "_%02d", n);
            std::string newId = sourceId + buf;
            if (!has(newId)) {
                clone(sourceId, newId);
                return newId;
            }
        }
        std::cerr << "[Prefab] cloneNumbered: все суффиксы заняты для " << sourceId << "\n";
        return "";
    }

    // Проверяет каталог на дубликаты по displayName (разные ID, одно имя).
    // Безопасна: только читает, ничего не изменяет.
    // Возвращает число найденных дублей.
    int validateDuplicates() const {
        std::map<std::string, std::vector<std::string>> byName;
        for (auto& [id, t] : templates)
            byName[t.displayName].push_back(id);
        int dupeCount = 0;
        for (auto& [name, ids] : byName) {
            if (ids.size() > 1) {
                dupeCount++;
                std::cerr << "[Prefab] ДУБЛЬ displayName \"" << name << "\": ";
                for (auto& id : ids) std::cerr << "\"" << id << "\" ";
                std::cerr << "\n";
            }
        }
        if (dupeCount == 0)
            std::cout << "[Prefab] Дублей нет — каталог чистый (" << templates.size() << " записей)\n";
        else
            std::cerr << "[Prefab] Найдено дублей: " << dupeCount << "\n";
        return dupeCount;
    }

    size_t count() const { return templates.size(); }
    void   clear() { templates.clear(); }

    // ════════════════════════════════════════════════════════════
    // ФИЛЬТРАЦИЯ И ПОИСК
    // ════════════════════════════════════════════════════════════

    // Все ID
    std::vector<std::string> getAllIds() const {
        std::vector<std::string> ids;
        ids.reserve(templates.size());
        for (auto& [id, _] : templates) ids.push_back(id);
        return ids;
    }

    // По категории
    std::vector<const PrefabTemplate*> getByCategory(PrefabCategory cat) const {
        std::vector<const PrefabTemplate*> out;
        for (auto& [_, t] : templates)
            if (t.category == cat) out.push_back(&t);
        return out;
    }

    // По тегу
    std::vector<const PrefabTemplate*> getByTag(const std::string& tag) const {
        std::vector<const PrefabTemplate*> out;
        for (auto& [_, t] : templates) {
            for (auto& tg : t.tags)
                if (tg == tag) { out.push_back(&t); break; }
        }
        return out;
    }

    // Текстовый поиск по id / displayName
    std::vector<const PrefabTemplate*> search(const std::string& query) const {
        std::string q = query;
        std::transform(q.begin(), q.end(), q.begin(), ::tolower);
        std::vector<const PrefabTemplate*> out;
        for (auto& [_, t] : templates) {
            std::string idL = t.id, dnL = t.displayName;
            std::transform(idL.begin(), idL.end(), idL.begin(), ::tolower);
            std::transform(dnL.begin(), dnL.end(), dnL.begin(), ::tolower);
            if (idL.find(q) != std::string::npos || dnL.find(q) != std::string::npos)
                out.push_back(&t);
        }
        return out;
    }

    // Все уникальные категории, представленные в каталоге
    std::vector<PrefabCategory> getCategories() const {
        std::set<PrefabCategory> s;
        for (auto& [_, t] : templates) s.insert(t.category);
        return std::vector<PrefabCategory>(s.begin(), s.end());
    }

    // ════════════════════════════════════════════════════════════
    // СПАВН СУЩНОСТИ
    // ════════════════════════════════════════════════════════════

    SpawnResult spawn(EntitySystem& es,
                      const std::string& prefabId,
                      float x, float y,
                      const PrefabOverride& ov = {}) const
    {
        SpawnResult res;
        res.prefabId = prefabId;

        const PrefabTemplate* tmpl = get(prefabId);
        if (!tmpl) {
            res.errorMsg = "Префаб не найден: " + prefabId;
            std::cerr << "[Prefab] " << res.errorMsg << "\n";
            return res;
        }

        // Применяем overrides
        EntityProperties props = tmpl->applyOverride(ov);

        // Имя: override > шаблонное displayName
        std::string name = ov.nameOverride.value_or(tmpl->displayName);

        // Сохраняем id префаба для обратной связи
        props.setStr("prefab_id", prefabId);

        // Спрайт из шаблона (если не переопределён)
        if (!tmpl->spriteKey.empty() && !props.hasStr("sprite_key"))
            props.setStr("sprite_key", tmpl->spriteKey);

        // Размер тайла
        if (!props.hasInt("tile_w")) props.setInt("tile_w", tmpl->tileWidth);
        if (!props.hasInt("tile_h")) props.setInt("tile_h", tmpl->tileHeight);

        // Создаём сущность
        uint32_t id = es.addEntityFull(tmpl->entityType, x, y, name, props);

        // Вызываем spawn-хук (если задан)
        if (tmpl->onSpawn) {
            Entity* e = es.getEntity(id);
            if (e) tmpl->onSpawn(*e);
        }

        res.success  = true;
        res.entityId = id;

        std::cout << "[Prefab] Заспавнен " << prefabId
                  << " id=" << id
                  << " pos=(" << x << "," << y << ")\n";
        return res;
    }

    // Пакетный спавн по списку точек
    std::vector<SpawnResult> spawnBatch(EntitySystem& es,
                                        const std::string& prefabId,
                                        const std::vector<std::pair<float,float>>& positions,
                                        const PrefabOverride& ov = {}) const
    {
        std::vector<SpawnResult> results;
        results.reserve(positions.size());
        for (auto& [x, y] : positions)
            results.push_back(spawn(es, prefabId, x, y, ov));
        return results;
    }

    // ════════════════════════════════════════════════════════════
    // СОХРАНЕНИЕ / ЗАГРУЗКА
    // ════════════════════════════════════════════════════════════

    bool save(const std::string& path = "") const {
        const std::string& p = path.empty() ? catalogPath : path;
        std::ofstream f(p);
        if (!f.is_open()) {
            std::cerr << "[Prefab] Не удалось открыть для записи: " << p << "\n";
            return false;
        }

        f << "{\n  \"prefabs\": {\n";
        bool firstPrefab = true;
        for (auto& [id, t] : templates) {
            if (!firstPrefab) f << ",\n";
            firstPrefab = false;

            f << "    \"" << escJ(id) << "\": {\n";
            f << "      \"display_name\": \""  << escJ(t.displayName)                  << "\",\n";
            f << "      \"category\": \""       << prefabCategoryToStr(t.category)      << "\",\n";
            f << "      \"entity_type\": \""    << entityTypeToStr(t.entityType)        << "\",\n";
            f << "      \"sprite_key\": \""     << escJ(t.spriteKey)                   << "\",\n";
            f << "      \"icon_path\": \""      << escJ(t.iconPath)                    << "\",\n";
            f << "      \"tile_width\": "       << t.tileWidth                         << ",\n";
            f << "      \"tile_height\": "      << t.tileHeight                        << ",\n";
            f << "      \"description\": \""    << escJ(t.description)                 << "\",\n";
            f << "      \"author\": \""         << escJ(t.author)                      << "\",\n";
            f << "      \"version\": "          << t.version                           << ",\n";

            // Tags
            f << "      \"tags\": [";
            for (size_t i = 0; i < t.tags.size(); i++) {
                if (i) f << ", ";
                f << "\"" << escJ(t.tags[i]) << "\"";
            }
            f << "],\n";

            // Overridable fields
            f << "      \"overridable\": [";
            bool firstOv = true;
            for (auto& field : t.overridableFields) {
                if (!firstOv) f << ", "; firstOv = false;
                f << "\"" << escJ(field) << "\"";
            }
            f << "],\n";

            // Default props
            f << "      \"default_props\": {\n";
            writePropsJSON(f, t.defaultProps, 8);
            f << "      }\n";

            f << "    }";
        }
        f << "\n  }\n}\n";

        std::cout << "[Prefab] Сохранено " << templates.size()
                  << " префабов → " << p << "\n";
        return true;
    }

    bool load(const std::string& path = "") {
        const std::string& p = path.empty() ? catalogPath : path;
        std::ifstream f(p);
        if (!f.is_open()) {
            std::cerr << "[Prefab] Файл каталога не найден: " << p << "\n";
            return false;
        }
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isObject()) {
            std::cerr << "[Prefab] Невалидный JSON: " << p << "\n";
            return false;
        }
        auto prefabsNode = root->get("prefabs");
        if (!prefabsNode || !prefabsNode->isObject()) return false;

        int loaded = 0;
        for (auto& [id, node] : prefabsNode->objectVal) {
            if (!node || !node->isObject()) continue;
            PrefabTemplate t;
            t.id = id;

            if (auto v = node->get("display_name")) t.displayName = v->asString();
            if (auto v = node->get("category"))     t.category    = strToPrefabCategory(v->asString());
            if (auto v = node->get("entity_type"))  t.entityType  = strToEntityType(v->asString());
            if (auto v = node->get("sprite_key"))   t.spriteKey   = v->asString();
            if (auto v = node->get("icon_path"))    t.iconPath    = v->asString();
            if (auto v = node->get("tile_width"))   t.tileWidth   = v->asInt();
            if (auto v = node->get("tile_height"))  t.tileHeight  = v->asInt();
            if (auto v = node->get("description"))  t.description = v->asString();
            if (auto v = node->get("author"))       t.author      = v->asString();
            if (auto v = node->get("version"))      t.version     = v->asInt();

            if (auto tags = node->get("tags"))
                for (auto& tagNode : tags->arrayVal)
                    if (tagNode) t.tags.push_back(tagNode->asString());

            if (auto ov = node->get("overridable"))
                for (auto& fieldNode : ov->arrayVal)
                    if (fieldNode) t.overridableFields.insert(fieldNode->asString());

            if (auto props = node->get("default_props"))
                t.defaultProps = parsePropsJSON(props.get());

            templates[id] = std::move(t);
            loaded++;
        }

        std::cout << "[Prefab] Загружено " << loaded
                  << " префабов из " << p << "\n";
        return loaded > 0;
    }

    // ════════════════════════════════════════════════════════════
    // ОТЛАДКА
    // ════════════════════════════════════════════════════════════

    void printCatalog() const {
        std::cout << "╔══════════════════════ PREFAB CATALOG ══════════════════════╗\n";
        std::cout << "║ Всего шаблонов: " << templates.size() << "\n";
        for (auto& [id, t] : templates) {
            std::cout << "║  [" << prefabCategoryToStr(t.category) << "] "
                      << id << " → " << t.displayName << "\n";
            std::cout << "║    sprite=" << t.spriteKey
                      << "  overridable=" << t.overridableFields.size() << " полей\n";
        }
        std::cout << "╚═════════════════════════════════════════════════════════════╝\n";
    }
};

// ════════════════════════════════════════════════════════════════════
// ФАБРИКА СТАНДАРТНЫХ ПРЕФАБОВ AETHORIA
// Вызывать один раз при старте — регистрирует базовые шаблоны
// ════════════════════════════════════════════════════════════════════
class AethoriaPrefabFactory {
public:
    static void registerDefaults(PrefabCatalog& catalog) {

        // ── ВРАГИ ──────────────────────────────────────────────────
        {
            PrefabTemplate t;
            t.id          = "goblin_warrior";
            t.displayName = "Goblin Warrior";
            t.category    = PrefabCategory::ENEMY;
            t.entityType  = EntityType::ENEMY;
            t.spriteKey   = "goblin_warrior";
            t.iconPath    = "assets/icons/goblin_warrior.png";
            t.description = "Базовый ближний враг. Патрулирует, атакует при сближении.";
            t.tags        = {"melee", "humanoid", "forest", "common"};
            t.overridableFields = {"level", "hp", "maxHp", "name", "exp_reward", "damage"};

            t.defaultProps.setInt  ("level",       1);
            t.defaultProps.setFloat("hp",          50.f);
            t.defaultProps.setFloat("maxHp",       50.f);
            t.defaultProps.setInt  ("damage",      8);
            t.defaultProps.setInt  ("exp_reward",  15);
            t.defaultProps.setInt  ("gold_reward", 3);
            t.defaultProps.setFloat("speed",       80.f);
            t.defaultProps.setFloat("attack_range",40.f);
            t.defaultProps.setFloat("detect_range",200.f);
            t.defaultProps.setStr  ("ai_type",    "melee_patrol");
            t.defaultProps.setStr  ("drop_table", "goblin_drops");
            t.defaultProps.setStr  ("faction",    "goblins");
            t.defaultProps.setBool ("boss",        false);
            t.defaultProps.setBool ("can_patrol",  true);
            t.defaultProps.setBool ("respawns",    true);
            t.defaultProps.setFloat("respawn_time",30.f);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "goblin_shaman";
            t.displayName = "Goblin Shaman";
            t.category    = PrefabCategory::ENEMY;
            t.entityType  = EntityType::ENEMY;
            t.spriteKey   = "goblin_shaman";
            t.description = "Дальний маг. Держится на расстоянии, кастует молнии.";
            t.tags        = {"ranged", "magic", "humanoid", "forest"};
            t.overridableFields = {"level", "hp", "maxHp", "name"};

            t.defaultProps.setInt  ("level",        3);
            t.defaultProps.setFloat("hp",           35.f);
            t.defaultProps.setFloat("maxHp",        35.f);
            t.defaultProps.setInt  ("damage",       14);
            t.defaultProps.setInt  ("exp_reward",   30);
            t.defaultProps.setFloat("speed",        60.f);
            t.defaultProps.setFloat("attack_range", 200.f);
            t.defaultProps.setFloat("detect_range", 250.f);
            t.defaultProps.setStr  ("ai_type",     "ranged_flee");
            t.defaultProps.setStr  ("skill_1",     "lightning_bolt");
            t.defaultProps.setStr  ("drop_table",  "shaman_drops");
            t.defaultProps.setStr  ("faction",     "goblins");
            t.defaultProps.setBool ("boss",         false);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "skeleton_archer";
            t.displayName = "Skeleton Archer";
            t.category    = PrefabCategory::ENEMY;
            t.entityType  = EntityType::ENEMY;
            t.spriteKey   = "skeleton_archer";
            t.description = "Дистанционный враг. Стреляет костяными стрелами из укрытия.";
            t.tags        = {"ranged", "undead", "crypt"};
            t.overridableFields = {"level", "hp", "maxHp", "name", "damage"};

            t.defaultProps.setInt  ("level",        5);
            t.defaultProps.setFloat("hp",           45.f);
            t.defaultProps.setFloat("maxHp",        45.f);
            t.defaultProps.setInt  ("damage",       18);
            t.defaultProps.setInt  ("exp_reward",   40);
            t.defaultProps.setFloat("speed",        65.f);
            t.defaultProps.setFloat("attack_range", 300.f);
            t.defaultProps.setStr  ("ai_type",     "ranged_stationary");
            t.defaultProps.setStr  ("drop_table",  "undead_drops");
            t.defaultProps.setStr  ("faction",     "undead");
            t.defaultProps.setBool ("undead",       true);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "dark_knight";
            t.displayName = "Dark Knight";
            t.category    = PrefabCategory::ENEMY;
            t.entityType  = EntityType::ENEMY;
            t.spriteKey   = "dark_knight";
            t.description = "Тяжёлый ближник. Высокий HP, блокирует атаки.";
            t.tags        = {"melee", "armored", "undead", "elite"};
            t.overridableFields = {"level", "hp", "maxHp", "name"};

            t.defaultProps.setInt  ("level",        8);
            t.defaultProps.setFloat("hp",           120.f);
            t.defaultProps.setFloat("maxHp",        120.f);
            t.defaultProps.setInt  ("defense",      25);
            t.defaultProps.setInt  ("damage",       30);
            t.defaultProps.setInt  ("exp_reward",   100);
            t.defaultProps.setFloat("speed",        50.f);
            t.defaultProps.setFloat("attack_range", 55.f);
            t.defaultProps.setStr  ("ai_type",     "melee_heavy");
            t.defaultProps.setStr  ("drop_table",  "elite_drops");
            t.defaultProps.setBool ("can_block",    true);
            t.defaultProps.setBool ("undead",       true);
            catalog.registerPrefab(std::move(t));
        }

        // ── БОССЫ ──────────────────────────────────────────────────
        {
            PrefabTemplate t;
            t.id          = "shadow_lord";
            t.displayName = "Shadow Lord";
            t.category    = PrefabCategory::BOSS;
            t.entityType  = EntityType::ENEMY;
            t.spriteKey   = "shadow_lord";
            t.description = "Финальный босс первого акта. Три фазы, стихийные атаки.";
            t.tags        = {"boss", "magic", "undead", "act1"};
            t.overridableFields = {};  // боссы не переопределяются

            t.defaultProps.setInt  ("level",         20);
            t.defaultProps.setFloat("hp",            800.f);
            t.defaultProps.setFloat("maxHp",         800.f);
            t.defaultProps.setInt  ("damage",        60);
            t.defaultProps.setInt  ("defense",       30);
            t.defaultProps.setInt  ("exp_reward",    1000);
            t.defaultProps.setInt  ("gold_reward",   200);
            t.defaultProps.setFloat("speed",         90.f);
            t.defaultProps.setFloat("attack_range",  80.f);
            t.defaultProps.setStr  ("ai_type",      "boss_multiphase");
            t.defaultProps.setStr  ("drop_table",   "boss_act1_drops");
            t.defaultProps.setStr  ("phase_skill_1","shadow_bolt");
            t.defaultProps.setStr  ("phase_skill_2","void_rift");
            t.defaultProps.setStr  ("phase_skill_3","dark_nova");
            t.defaultProps.setInt  ("phase_count",   3);
            t.defaultProps.setBool ("boss",          true);
            t.defaultProps.setBool ("respawns",      false);
            catalog.registerPrefab(std::move(t));
        }

        // ── NPC ────────────────────────────────────────────────────
        {
            PrefabTemplate t;
            t.id          = "merchant_general";
            t.displayName = "General Merchant";
            t.category    = PrefabCategory::NPC;
            t.entityType  = EntityType::NPC;
            t.spriteKey   = "merchant";
            t.description = "Торговец. Открывает магазин при взаимодействии.";
            t.tags        = {"merchant", "town", "shop"};
            t.overridableFields = {"name", "shop_id", "greeting"};

            t.defaultProps.setStr  ("shop_id",   "general_store");
            t.defaultProps.setStr  ("greeting",  "Чем могу помочь, путник?");
            t.defaultProps.setStr  ("subtype",   "merchant");
            t.defaultProps.setFloat("interact_range", 60.f);
            t.defaultProps.setBool ("hostile",   false);
            t.defaultProps.setBool ("can_move",  false);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "quest_giver";
            t.displayName = "Quest Giver NPC";
            t.category    = PrefabCategory::NPC;
            t.entityType  = EntityType::NPC;
            t.spriteKey   = "villager";
            t.description = "NPC выдающий квесты. Иконка! над головой.";
            t.tags        = {"quest", "town"};
            t.overridableFields = {"name", "quest_id", "dialogue_id", "greeting"};

            t.defaultProps.setStr  ("quest_id",    "");
            t.defaultProps.setStr  ("dialogue_id", "");
            t.defaultProps.setStr  ("greeting",    "У меня есть для тебя задание...");
            t.defaultProps.setStr  ("subtype",     "quest_giver");
            t.defaultProps.setFloat("interact_range", 60.f);
            t.defaultProps.setBool ("has_quest",   true);
            t.defaultProps.setBool ("hostile",     false);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "blacksmith";
            t.displayName = "Blacksmith";
            t.category    = PrefabCategory::NPC;
            t.entityType  = EntityType::NPC;
            t.spriteKey   = "blacksmith";
            t.description = "Кузнец. Крафт и улучшение снаряжения.";
            t.tags        = {"craft", "town", "upgrade"};
            t.overridableFields = {"name", "greeting"};

            t.defaultProps.setStr  ("subtype",     "blacksmith");
            t.defaultProps.setStr  ("shop_id",     "blacksmith_shop");
            t.defaultProps.setStr  ("greeting",    "Нужна помощь с оружием?");
            t.defaultProps.setFloat("interact_range", 70.f);
            t.defaultProps.setBool ("can_upgrade", true);
            t.defaultProps.setBool ("can_craft",   true);
            catalog.registerPrefab(std::move(t));
        }

        // ── ОБЪЕКТЫ ────────────────────────────────────────────────
        {
            PrefabTemplate t;
            t.id          = "chest_wooden";
            t.displayName = "Wooden Chest";
            t.category    = PrefabCategory::OBJECT;
            t.entityType  = EntityType::OBJECT;
            t.spriteKey   = "chest_wooden";
            t.description = "Деревянный сундук. Содержит обычный лут.";
            t.tags        = {"container", "loot", "interactable"};
            t.overridableFields = {"loot_table", "gold_min", "gold_max", "is_locked"};

            t.defaultProps.setStr  ("subtype",    "chest");
            t.defaultProps.setStr  ("loot_table", "chest_common");
            t.defaultProps.setInt  ("gold_min",   2);
            t.defaultProps.setInt  ("gold_max",   15);
            t.defaultProps.setFloat("interact_range", 50.f);
            t.defaultProps.setBool ("is_locked",  false);
            t.defaultProps.setBool ("opened",     false);
            t.defaultProps.setBool ("destroyable",false);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "chest_iron";
            t.displayName = "Iron Chest";
            t.category    = PrefabCategory::OBJECT;
            t.entityType  = EntityType::OBJECT;
            t.spriteKey   = "chest_iron";
            t.description = "Железный сундук. Редкий лут, может быть заперт.";
            t.tags        = {"container", "loot", "interactable", "locked"};
            t.overridableFields = {"loot_table", "key_id", "is_locked"};

            t.defaultProps.setStr  ("subtype",    "chest");
            t.defaultProps.setStr  ("loot_table", "chest_rare");
            t.defaultProps.setStr  ("key_id",     "iron_key");
            t.defaultProps.setInt  ("gold_min",   20);
            t.defaultProps.setInt  ("gold_max",   80);
            t.defaultProps.setFloat("interact_range", 50.f);
            t.defaultProps.setBool ("is_locked",  true);
            t.defaultProps.setBool ("opened",     false);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "altar_healing";
            t.displayName = "Healing Altar";
            t.category    = PrefabCategory::OBJECT;
            t.entityType  = EntityType::OBJECT;
            t.spriteKey   = "altar_healing";
            t.description = "Алтарь исцеления. Восстанавливает HP раз в N секунд.";
            t.tags        = {"altar", "healing", "interactable"};
            t.overridableFields = {"heal_amount", "cooldown"};

            t.defaultProps.setStr  ("subtype",        "altar");
            t.defaultProps.setFloat("heal_amount",    0.30f);  // 30% от maxHP
            t.defaultProps.setFloat("cooldown",       120.f);  // 2 минуты
            t.defaultProps.setFloat("interact_range", 55.f);
            t.defaultProps.setBool ("active",         true);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "barrel_explosive";
            t.displayName = "Explosive Barrel";
            t.category    = PrefabCategory::OBJECT;
            t.entityType  = EntityType::OBJECT;
            t.spriteKey   = "barrel_explosive";
            t.description = "Взрывная бочка. AoE-урон при разрушении.";
            t.tags        = {"trap", "explosive", "environment"};
            t.overridableFields = {"damage", "radius"};

            t.defaultProps.setStr  ("subtype",        "barrel");
            t.defaultProps.setFloat("hp",             20.f);
            t.defaultProps.setFloat("maxHp",          20.f);
            t.defaultProps.setInt  ("damage",         80);
            t.defaultProps.setFloat("radius",         120.f);
            t.defaultProps.setBool ("destroyable",    true);
            t.defaultProps.setBool ("explodes",       true);
            catalog.registerPrefab(std::move(t));
        }

        // ── ПОРТАЛЫ ────────────────────────────────────────────────
        {
            PrefabTemplate t;
            t.id          = "portal_zone";
            t.displayName = "Zone Portal";
            t.category    = PrefabCategory::PORTAL;
            t.entityType  = EntityType::PORTAL;
            t.spriteKey   = "portal_blue";
            t.description = "Портал перехода между зонами.";
            t.tags        = {"portal", "transition"};
            t.overridableFields = {"target_zone", "target_x", "target_y", "name", "level_req"};

            t.defaultProps.setStr  ("subtype",        "zone_portal");
            t.defaultProps.setStr  ("target_zone",    "");
            t.defaultProps.setFloat("target_x",       500.f);
            t.defaultProps.setFloat("target_y",       500.f);
            t.defaultProps.setInt  ("level_req",      0);
            t.defaultProps.setFloat("interact_range", 60.f);
            t.defaultProps.setBool ("active",         true);
            t.defaultProps.setBool ("needs_key",      false);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "portal_dungeon";
            t.displayName = "Dungeon Entrance";
            t.category    = PrefabCategory::PORTAL;
            t.entityType  = EntityType::PORTAL;
            t.spriteKey   = "portal_dungeon";
            t.description = "Вход в подземелье. Требует минимального уровня.";
            t.tags        = {"portal", "dungeon", "entrance"};
            t.overridableFields = {"target_zone", "level_req", "name"};

            t.defaultProps.setStr  ("subtype",        "dungeon_entrance");
            t.defaultProps.setStr  ("target_zone",    "dungeon_01");
            t.defaultProps.setInt  ("level_req",      5);
            t.defaultProps.setFloat("interact_range", 70.f);
            t.defaultProps.setBool ("active",         true);
            t.defaultProps.setBool ("needs_key",      false);
            catalog.registerPrefab(std::move(t));
        }

        // ── ЛОВУШКИ ────────────────────────────────────────────────
        {
            PrefabTemplate t;
            t.id          = "trap_spike";
            t.displayName = "Spike Trap";
            t.category    = PrefabCategory::TRAP;
            t.entityType  = EntityType::OBJECT;
            t.spriteKey   = "trap_spike";
            t.description = "Шипы из пола. Срабатывают при наступлении.";
            t.tags        = {"trap", "physical", "dungeon"};
            t.overridableFields = {"damage", "cooldown"};

            t.defaultProps.setStr  ("subtype",    "trap");
            t.defaultProps.setStr  ("trap_type",  "spike");
            t.defaultProps.setInt  ("damage",     25);
            t.defaultProps.setFloat("trigger_range", 20.f);
            t.defaultProps.setFloat("cooldown",   2.f);
            t.defaultProps.setBool ("visible",    false);  // скрытая
            t.defaultProps.setBool ("active",     true);
            catalog.registerPrefab(std::move(t));
        }
        {
            PrefabTemplate t;
            t.id          = "trap_fire";
            t.displayName = "Fire Trap";
            t.category    = PrefabCategory::TRAP;
            t.entityType  = EntityType::OBJECT;
            t.spriteKey   = "trap_fire";
            t.description = "Огненная струя. Постоянно активна по таймеру.";
            t.tags        = {"trap", "fire", "dungeon"};
            t.overridableFields = {"damage", "active_time", "inactive_time"};

            t.defaultProps.setStr  ("subtype",       "trap");
            t.defaultProps.setStr  ("trap_type",     "fire");
            t.defaultProps.setInt  ("damage",        15);  // урон в секунду
            t.defaultProps.setFloat("active_time",   2.f);
            t.defaultProps.setFloat("inactive_time", 1.5f);
            t.defaultProps.setFloat("width",         16.f);
            t.defaultProps.setFloat("length",        96.f);
            t.defaultProps.setBool ("active",        true);
            catalog.registerPrefab(std::move(t));
        }

        std::cout << "[Prefab] AethoriaPrefabFactory: зарегистрировано "
                  << catalog.count() << " префабов по умолчанию\n";
    }
};
