// entity_system.hpp - Система сущностей AETHORIA
// NPC, объекты, враги, порталы — единый тип, уникальный ID

#pragma once
#include "json_parser.hpp"
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdint>

// ═══════════════════════════════════════════════════════════════
// ТИП СУЩНОСТИ
// ═══════════════════════════════════════════════════════════════
enum class EntityType {
    PLAYER,
    ENEMY,
    NPC,
    OBJECT,    // сундук, бочка, алтарь и т.д.
    PORTAL,
    ITEM_DROP
};

inline std::string entityTypeToStr(EntityType t) {
    switch (t) {
        case EntityType::PLAYER:    return "PLAYER";
        case EntityType::ENEMY:     return "ENEMY";
        case EntityType::NPC:       return "NPC";
        case EntityType::OBJECT:    return "OBJECT";
        case EntityType::PORTAL:    return "PORTAL";
        case EntityType::ITEM_DROP: return "ITEM_DROP";
        default:                    return "NPC";
    }
}

inline EntityType strToEntityType(const std::string& s) {
    if (s == "PLAYER")    return EntityType::PLAYER;
    if (s == "ENEMY")     return EntityType::ENEMY;
    if (s == "NPC")       return EntityType::NPC;
    if (s == "OBJECT")    return EntityType::OBJECT;
    if (s == "PORTAL")    return EntityType::PORTAL;
    if (s == "ITEM_DROP") return EntityType::ITEM_DROP;
    return EntityType::NPC;
}

// ═══════════════════════════════════════════════════════════════
// СВОЙСТВА СУЩНОСТИ — универсальное key-value хранилище
// ═══════════════════════════════════════════════════════════════
struct EntityProperties {
    std::map<std::string, std::string> strings;
    std::map<std::string, float>       floats;
    std::map<std::string, int>         ints;
    std::map<std::string, bool>        bools;

    void setStr  (const std::string& k, const std::string& v) { strings[k] = v; }
    void setFloat(const std::string& k, float v)              { floats[k]  = v; }
    void setInt  (const std::string& k, int v)                { ints[k]    = v; }
    void setBool (const std::string& k, bool v)               { bools[k]   = v; }

    std::string getStr  (const std::string& k, const std::string& d = "") const { auto i=strings.find(k); return i!=strings.end()?i->second:d; }
    float       getFloat(const std::string& k, float d = 0.f)             const { auto i=floats.find(k);  return i!=floats.end() ?i->second:d; }
    int         getInt  (const std::string& k, int d = 0)                 const { auto i=ints.find(k);    return i!=ints.end()   ?i->second:d; }
    bool        getBool (const std::string& k, bool d = false)            const { auto i=bools.find(k);   return i!=bools.end()  ?i->second:d; }

    bool hasStr  (const std::string& k) const { return strings.count(k) > 0; }
    bool hasFloat(const std::string& k) const { return floats.count(k)  > 0; }
    bool hasInt  (const std::string& k) const { return ints.count(k)    > 0; }
    bool hasBool (const std::string& k) const { return bools.count(k)   > 0; }
};

// ═══════════════════════════════════════════════════════════════
// БАЗОВАЯ СУЩНОСТЬ
// ═══════════════════════════════════════════════════════════════
struct Entity {
    uint32_t         id;
    EntityType       type;
    float            x, y;      // мировые координаты
    std::string      name;
    bool             active;
    EntityProperties props;

    Entity() : id(0), type(EntityType::NPC), x(0.f), y(0.f), active(true) {}

    // Быстрый доступ к стандартным свойствам
    int   getLevel()  const { return props.getInt("level", 1); }
    float getHp()     const { return props.getFloat("hp", 0.f); }
    float getMaxHp()  const { return props.getFloat("maxHp", 0.f); }
    bool  isBoss()    const { return props.getBool("boss", false); }
    std::string getSubtype() const { return props.getStr("subtype", ""); }
};

// ═══════════════════════════════════════════════════════════════
// СИСТЕМА СУЩНОСТЕЙ
// ═══════════════════════════════════════════════════════════════
class EntitySystem {
private:
    std::vector<Entity> entities;
    uint32_t            nextId = 1;

    static std::string escapeStr(const std::string& s) {
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

public:
    // ── Добавление ────────────────────────────────────────────
    uint32_t addEntity(EntityType type, float x, float y,
                       const std::string& name = "")
    {
        Entity e;
        e.id     = nextId++;
        e.type   = type;
        e.x      = x;
        e.y      = y;
        e.name   = name;
        e.active = true;
        entities.push_back(std::move(e));
        return entities.back().id;
    }

    // Добавление с готовыми свойствами
    uint32_t addEntityFull(EntityType type, float x, float y,
                           const std::string& name,
                           const EntityProperties& props)
    {
        uint32_t id = addEntity(type, x, y, name);
        Entity* e = getEntity(id);
        if (e) e->props = props;
        return id;
    }

    // ── Удаление ──────────────────────────────────────────────
    void removeEntity(uint32_t id) {
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                [id](const Entity& e){ return e.id == id; }),
            entities.end()
        );
    }

    void deactivate(uint32_t id) {
        if (Entity* e = getEntity(id)) e->active = false;
    }

    void purgeInactive() {
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                [](const Entity& e){ return !e.active; }),
            entities.end()
        );
    }

    // ── Поиск ─────────────────────────────────────────────────
    Entity* getEntity(uint32_t id) {
        for (auto& e : entities) if (e.id == id) return &e;
        return nullptr;
    }

    const Entity* getEntity(uint32_t id) const {
        for (const auto& e : entities) if (e.id == id) return &e;
        return nullptr;
    }

    std::vector<Entity*> getByType(EntityType type) {
        std::vector<Entity*> r;
        for (auto& e : entities) if (e.type == type && e.active) r.push_back(&e);
        return r;
    }

    std::vector<Entity*> getActive() {
        std::vector<Entity*> r;
        for (auto& e : entities) if (e.active) r.push_back(&e);
        return r;
    }

    // Ближайшая сущность заданного типа в радиусе
    Entity* getNearby(float x, float y, float radius, EntityType type) {
        Entity* closest = nullptr;
        float   best    = radius * radius;
        for (auto& e : entities) {
            if (!e.active || e.type != type) continue;
            float dx = e.x - x, dy = e.y - y;
            float d2 = dx*dx + dy*dy;
            if (d2 < best) { best = d2; closest = &e; }
        }
        return closest;
    }

    // ── Вся коллекция ─────────────────────────────────────────
    std::vector<Entity>&       getAll()       { return entities; }
    const std::vector<Entity>& getAll() const { return entities; }

    size_t count() const { return entities.size(); }
    size_t countByType(EntityType t) const {
        size_t n = 0;
        for (const auto& e : entities) if (e.type == t && e.active) n++;
        return n;
    }

    void clear() { entities.clear(); nextId = 1; }

    // ── Обновление позиции ────────────────────────────────────
    void setPosition(uint32_t id, float x, float y) {
        if (Entity* e = getEntity(id)) { e->x = x; e->y = y; }
    }

    // ── Сохранение ────────────────────────────────────────────
    bool save(const std::string& filepath) const {
        std::ofstream f(filepath);
        if (!f.is_open()) {
            std::cerr << "[ERROR] EntitySystem::save — не открыть: " << filepath << "\n";
            return false;
        }
        f << "{\n";
        f << "  \"next_id\": " << nextId << ",\n";
        f << "  \"entities\": [\n";
        for (size_t i = 0; i < entities.size(); i++) {
            const auto& e = entities[i];
            f << "    {\n";
            f << "      \"id\": "      << e.id                          << ",\n";
            f << "      \"type\": \""  << entityTypeToStr(e.type)        << "\",\n";
            f << "      \"x\": "       << e.x                           << ",\n";
            f << "      \"y\": "       << e.y                           << ",\n";
            f << "      \"name\": \""  << escapeStr(e.name)              << "\",\n";
            f << "      \"active\": "  << (e.active ? "true" : "false") << ",\n";
            f << "      \"props\": {\n";
            // strings
            f << "        \"strings\": {";
            bool first = true;
            for (auto& [k,v] : e.props.strings) {
                if (!first) f << ","; first = false;
                f << "\"" << escapeStr(k) << "\":\"" << escapeStr(v) << "\"";
            }
            f << "},\n";
            // floats
            f << "        \"floats\": {";
            first = true;
            for (auto& [k,v] : e.props.floats) {
                if (!first) f << ","; first = false;
                f << "\"" << escapeStr(k) << "\":" << v;
            }
            f << "},\n";
            // ints
            f << "        \"ints\": {";
            first = true;
            for (auto& [k,v] : e.props.ints) {
                if (!first) f << ","; first = false;
                f << "\"" << escapeStr(k) << "\":" << v;
            }
            f << "},\n";
            // bools
            f << "        \"bools\": {";
            first = true;
            for (auto& [k,v] : e.props.bools) {
                if (!first) f << ","; first = false;
                f << "\"" << escapeStr(k) << "\":" << (v?"true":"false");
            }
            f << "}\n";
            f << "      }\n";
            f << "    }";
            if (i + 1 < entities.size()) f << ",";
            f << "\n";
        }
        f << "  ]\n}\n";
        std::cout << "[OK] EntitySystem: сохранено " << entities.size()
                  << " сущностей → " << filepath << "\n";
        return true;
    }

    // ── Загрузка ──────────────────────────────────────────────
    bool load(const std::string& filepath) {
        std::ifstream f(filepath);
        if (!f.is_open()) {
            std::cerr << "[INFO] EntitySystem::load — файл не найден: " << filepath << "\n";
            return false;
        }
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isObject()) {
            std::cerr << "[ERROR] EntitySystem::load — невалидный JSON: " << filepath << "\n";
            return false;
        }
        clear();
        if (auto nid = root->get("next_id")) nextId = (uint32_t)nid->asInt();
        auto arr = root->get("entities");
        if (!arr || !arr->isArray()) return false;
        for (size_t i = 0; i < arr->arrayVal.size(); i++) {
            auto ej = arr->get(i);
            if (!ej) continue;
            Entity e;
            if (auto v = ej->get("id"))     e.id     = (uint32_t)v->asInt();
            if (auto v = ej->get("type"))   e.type   = strToEntityType(v->asString());
            if (auto v = ej->get("x"))      e.x      = (float)v->asDouble();
            if (auto v = ej->get("y"))      e.y      = (float)v->asDouble();
            if (auto v = ej->get("name"))   e.name   = v->asString();
            if (auto v = ej->get("active")) e.active = v->asBool();
            if (auto p = ej->get("props")) {
                if (auto s = p->get("strings"))
                    for (auto& [k,v] : s->objectVal) if (v) e.props.strings[k] = v->asString();
                if (auto s = p->get("floats"))
                    for (auto& [k,v] : s->objectVal) if (v) e.props.floats[k]  = (float)v->asDouble();
                if (auto s = p->get("ints"))
                    for (auto& [k,v] : s->objectVal) if (v) e.props.ints[k]    = v->asInt();
                if (auto s = p->get("bools"))
                    for (auto& [k,v] : s->objectVal) if (v) e.props.bools[k]   = v->asBool();
            }
            entities.push_back(std::move(e));
        }
        // Восстанавливаем nextId если он меньше фактических ID
        for (const auto& e : entities)
            if (e.id >= nextId) nextId = e.id + 1;
        std::cout << "[OK] EntitySystem: загружено " << entities.size()
                  << " сущностей из " << filepath << "\n";
        return true;
    }
};
