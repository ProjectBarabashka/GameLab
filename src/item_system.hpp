// item_system.hpp — AETHORIA: Предметы + Инвентарь (Приоритет 2 редмапа)
// Загружает items.json, управляет инвентарём, генерирует лут из drop-таблиц.
//
// Формат assets/items.json:
// [
//   { "id":"iron_sword", "name":"Железный меч", "icon":"⚔",
//     "type":"weapon", "rarity":1, "slot":"main_hand",
//     "stats":{"damage":15,"crit_chance":0.03},
//     "value":50, "stack":1, "description":"Обычный меч." }
// ]
//
// Формат assets/drop_tables.json:
// {
//   "GOBLIN": [
//     {"item_id":"goblin_tooth", "chance":0.6, "count_min":1, "count_max":3},
//     {"item_id":"iron_sword",   "chance":0.1, "count_min":1, "count_max":1}
//   ]
// }

#pragma once
#include "json_parser.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <random>

// ═══════════════════════════════════════════════════════════════
// РЕДКОСТЬ ПРЕДМЕТА
// ═══════════════════════════════════════════════════════════════
enum class ItemRarityLevel { COMMON=0, UNCOMMON=1, RARE=2, EPIC=3, LEGENDARY=4 };

inline std::string rarityName(ItemRarityLevel r) {
    switch (r) {
        case ItemRarityLevel::COMMON:    return "Обычный";
        case ItemRarityLevel::UNCOMMON:  return "Необычный";
        case ItemRarityLevel::RARE:      return "Редкий";
        case ItemRarityLevel::EPIC:      return "Эпический";
        case ItemRarityLevel::LEGENDARY: return "Легендарный";
        default: return "Обычный";
    }
}

// ═══════════════════════════════════════════════════════════════
// ОПРЕДЕЛЕНИЕ ПРЕДМЕТА
// ═══════════════════════════════════════════════════════════════
struct ItemDef {
    std::string     id;
    std::string     name;
    std::string     icon;
    std::string     type;       // "weapon","armor","helmet","boots","ring","consumable","quest","material"
    std::string     slot;       // "main_hand","off_hand","head","chest","legs","neck","ring1","ring2"
    ItemRarityLevel rarity = ItemRarityLevel::COMMON;
    int             maxStack  = 1;
    int             value     = 0; // стоимость у торговца
    std::string     description;

    // Статы (универсальное хранилище)
    std::map<std::string, float> stats;
    // stats ключи: "damage","defense","hp","mp","str","dex","int","vit","speed",
    //              "crit_chance","crit_mult","attack_speed","magic_power"

    // Какие классы могут использовать предмет (пусто = все)
    std::vector<std::string> allowedClasses;

    float getStat(const std::string& k) const {
        auto it = stats.find(k);
        return it != stats.end() ? it->second : 0.f;
    }

    // Может ли класс надеть предмет
    bool canBeUsedBy(const std::string& cls) const {
        if (allowedClasses.empty()) return true;
        for (const auto& c : allowedClasses)
            if (c == cls) return true;
        return false;
    }

    // Строка допустимых классов для тултипа
    std::string allowedClassesStr() const {
        if (allowedClasses.empty()) return "Все классы";
        std::string s;
        for (const auto& c : allowedClasses) {
            if (!s.empty()) s += ", ";
            s += c;
        }
        return s;
    }
};

// ═══════════════════════════════════════════════════════════════
// СТАК ПРЕДМЕТА В ИНВЕНТАРЕ
// ═══════════════════════════════════════════════════════════════
struct ItemStack {
    std::string itemId;
    int         count;
    ItemStack(const std::string& id, int n=1) : itemId(id), count(n) {}
};

// ═══════════════════════════════════════════════════════════════
// ИНВЕНТАРЬ
// ═══════════════════════════════════════════════════════════════
struct Inventory {
    std::vector<ItemStack> slots;
    int                    maxSlots = 20;
    int                    gold     = 0;

    // Добавить предмет (с учётом стакования)
    bool add(const std::string& itemId, int count, int maxStack) {
        // Попытка добавить к существующему стаку
        if (maxStack > 1) {
            for (auto& s : slots) {
                if (s.itemId == itemId && s.count < maxStack) {
                    int space = maxStack - s.count;
                    int add = std::min(count, space);
                    s.count += add;
                    count -= add;
                    if (count <= 0) return true;
                }
            }
        }
        // Новые слоты
        while (count > 0 && (int)slots.size() < maxSlots) {
            int take = std::min(count, maxStack);
            slots.push_back(ItemStack(itemId, take));
            count -= take;
        }
        return count <= 0;
    }

    // Убрать предмет
    bool remove(const std::string& itemId, int count = 1) {
        for (auto it = slots.begin(); it != slots.end(); ) {
            if (it->itemId == itemId) {
                if (it->count <= count) {
                    count -= it->count;
                    it = slots.erase(it);
                } else {
                    it->count -= count;
                    count = 0;
                    break;
                }
            } else ++it;
        }
        return count <= 0;
    }

    // Количество предмета в инвентаре
    int countOf(const std::string& itemId) const {
        int total = 0;
        for (auto& s : slots) if (s.itemId == itemId) total += s.count;
        return total;
    }

    bool has(const std::string& itemId, int n=1) const { return countOf(itemId) >= n; }
    bool isFull() const { return (int)slots.size() >= maxSlots; }
    int  freeSlots() const { return maxSlots - (int)slots.size(); }
};

// ═══════════════════════════════════════════════════════════════
// ДРОП-ТАБЛИЦА
// ═══════════════════════════════════════════════════════════════
struct DropEntry {
    std::string itemId;
    float       chance;     // 0.0 – 1.0
    int         countMin = 1;
    int         countMax = 1;
};

// ═══════════════════════════════════════════════════════════════
// СИСТЕМА ПРЕДМЕТОВ
// ═══════════════════════════════════════════════════════════════
class ItemSystem {
public:
    // ── Загрузка предметов ──────────────────────────────────
    bool loadItems(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "[ItemSystem] items.json не найден: " << path << "\n";
            _createDefaultItems();
            return false;
        }
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isArray()) return false;
        items_.clear();
        for (size_t i = 0; i < root->arrayVal.size(); i++) {
            auto j = root->get(i); if (!j) continue;
            ItemDef d;
            if (auto v = j->get("id"))          d.id          = v->asString();
            if (auto v = j->get("name"))        d.name        = v->asString();
            if (auto v = j->get("icon"))        d.icon        = v->asString();
            if (auto v = j->get("type"))        d.type        = v->asString();
            if (auto v = j->get("slot"))        d.slot        = v->asString();
            if (auto v = j->get("rarity"))      d.rarity      = (ItemRarityLevel)std::min(4,v->asInt());
            if (auto v = j->get("stack"))       d.maxStack    = std::max(1,v->asInt());
            if (auto v = j->get("value"))       d.value       = v->asInt();
            if (auto v = j->get("description")) d.description = v->asString();
            if (auto s = j->get("stats")) {
                for (auto& [k,v] : s->objectVal)
                    if (v) d.stats[k] = (float)v->asDouble();
            }
            // Ограничение по классу: "allowed_classes":["Warrior","Mage"]
            if (auto ac = j->get("allowed_classes")) {
                d.allowedClasses.clear();
                if (ac->isArray()) {
                    for (size_t k2 = 0; k2 < ac->arrayVal.size(); k2++) {
                        auto cl = ac->get(k2);
                        if (cl && !cl->asString().empty())
                            d.allowedClasses.push_back(cl->asString());
                    }
                }
            }
            if (!d.id.empty()) items_[d.id] = d;
        }
        std::cout << "[ItemSystem] Загружено предметов: " << items_.size()
                  << " из " << path << "\n";
        return true;
    }

    // ── Загрузка дроп-таблиц ────────────────────────────────
    bool loadDropTables(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "[ItemSystem] drop_tables.json не найден: " << path << "\n";
            return false;
        }
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isObject()) return false;
        dropTables_.clear();
        for (auto& [enemy, arr] : root->objectVal) {
            if (!arr || !arr->isArray()) continue;
            for (size_t i = 0; i < arr->arrayVal.size(); i++) {
                auto j = arr->get(i); if (!j) continue;
                DropEntry de;
                if (auto v = j->get("item_id"))   de.itemId   = v->asString();
                if (auto v = j->get("chance"))     de.chance   = (float)v->asDouble();
                if (auto v = j->get("count_min"))  de.countMin = v->asInt();
                if (auto v = j->get("count_max"))  de.countMax = v->asInt();
                dropTables_[enemy].push_back(de);
            }
        }
        std::cout << "[ItemSystem] Drop tables: " << dropTables_.size()
                  << " врагов из " << path << "\n";
        return true;
    }

    // ── Сохранение предметов (для редактора) ────────────────
    bool saveItems(const std::string& path) const {
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << "[\n"; bool first = true;
        for (auto& [id, d] : items_) {
            if (!first) f << ",\n"; first = false;
            f << "  {";
            f << "\"id\":\""          << _esc(d.id)          << "\",";
            f << "\"name\":\""        << _esc(d.name)        << "\",";
            f << "\"icon\":\""        << _esc(d.icon)        << "\",";
            f << "\"type\":\""        << _esc(d.type)        << "\",";
            f << "\"slot\":\""        << _esc(d.slot)        << "\",";
            f << "\"rarity\":"        << (int)d.rarity       << ",";
            f << "\"stack\":"         << d.maxStack          << ",";
            f << "\"value\":"         << d.value             << ",";
            f << "\"description\":\"" << _esc(d.description) << "\",";
            f << "\"allowed_classes\":[";
            { bool fc = true;
              for (const auto& c : d.allowedClasses) {
                if (!fc) f << ","; fc = false;
                f << "\"" << _esc(c) << "\"";
              }
            }
            f << "],";
            f << "\"stats\":{";
            bool fs = true;
            for (auto& [k,v] : d.stats) {
                if (!fs) f << ","; fs = false;
                f << "\"" << _esc(k) << "\":" << v;
            }
            f << "}}";
        }
        f << "\n]\n";
        return true;
    }

    // ── Получить предмет ────────────────────────────────────
    const ItemDef* get(const std::string& id) const {
        auto it = items_.find(id);
        return it != items_.end() ? &it->second : nullptr;
    }

    const std::map<std::string, ItemDef>& all() const { return items_; }

    // ── Генерация лута ──────────────────────────────────────
    // enemyType — "GOBLIN", "TROLL" и т.д.
    // Возвращает список {itemId, count}
    std::vector<ItemStack> rollLoot(const std::string& enemyType, int enemyLevel = 1) {
        std::vector<ItemStack> drops;
        auto it = dropTables_.find(enemyType);
        if (it == dropTables_.end()) return drops;

        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> chance(0.f, 1.f);
        std::uniform_int_distribution<int> lvlBonus(0, std::max(0, enemyLevel - 1));

        for (auto& entry : it->second) {
            if (chance(rng) < entry.chance) {
                int cnt = entry.countMin;
                if (entry.countMax > entry.countMin) {
                    std::uniform_int_distribution<int> cntDist(entry.countMin, entry.countMax);
                    cnt = cntDist(rng);
                }
                drops.push_back(ItemStack(entry.itemId, cnt));
            }
        }
        return drops;
    }

    // ── Добавить в инвентарь с учётом предмета ──────────────
    bool giveToInventory(Inventory& inv, const std::string& itemId, int count = 1) {
        const ItemDef* def = get(itemId);
        int maxStack = def ? def->maxStack : 1;
        return inv.add(itemId, count, maxStack);
    }

    // Всё лут сразу в инвентарь
    void lootToInventory(Inventory& inv, const std::vector<ItemStack>& loot) {
        for (auto& l : loot)
            giveToInventory(inv, l.itemId, l.count);
    }

private:
    std::map<std::string, ItemDef>            items_;
    std::map<std::string, std::vector<DropEntry>> dropTables_;

    static std::string _esc(const std::string& s) {
        std::string o;
        for (char c : s) {
            if (c=='"') o+="\\\"";
            else if(c=='\\') o+="\\\\";
            else o+=c;
        }
        return o;
    }

    void _createDefaultItems() {
        // Вспомогательная лямбда — добавить предмет с ограничением по классу
        auto add = [&](const std::string& id, const std::string& name,
                       const std::string& type, int rarity,
                       std::map<std::string,float> stats, int value,
                       std::vector<std::string> classes = {}) {
            ItemDef d;
            d.id = id; d.name = name; d.type = type;
            d.rarity = (ItemRarityLevel)rarity;
            d.stats = stats; d.value = value;
            d.allowedClasses = classes;
            // Автоопределение слота
            if      (type == "weapon")  d.slot = "main_hand";
            else if (type == "armor")   d.slot = "chest";
            else if (type == "helmet")  d.slot = "head";
            else if (type == "boots")   d.slot = "legs";
            else if (type == "ring")    d.slot = "ring1";
            else if (type == "amulet")  d.slot = "neck";
            else if (type == "shield")  d.slot = "off_hand";
            items_[id] = d;
        };

        // Оружие — ограничено по классу
        add("iron_sword",    "Железный меч",   "weapon", 0, {{"damage",15}},       50, {"Warrior","Paladin"});
        add("staff_oak",     "Дубовый посох",  "weapon", 0, {{"magic_power",20}},  60, {"Mage"});
        add("daggers_iron",  "Железные клинки","weapon", 0, {{"damage",12},{"crit_chance",0.05f}}, 55, {"Rogue"});
        add("holy_hammer",   "Священный молот","weapon", 0, {{"damage",13},{"hp",10}}, 65, {"Paladin"});

        // Броня — ограничена по классу
        add("plate_chest",   "Латный нагрудник","armor", 1, {{"defense",18}},      80, {"Warrior","Paladin"});
        add("leather_armor", "Кожаная броня",   "armor", 0, {{"defense",10}},      40, {"Warrior","Rogue","Paladin"});
        add("robe_mage",     "Мантия мага",     "armor", 0, {{"defense",5},{"mp",20}}, 45, {"Mage"});

        // Расходники и материалы — без ограничений
        add("health_potion", "Зелье здоровья",  "consumable", 0, {{"hp_restore",100}}, 20);
        add("mana_potion",   "Зелье маны",      "consumable", 0, {{"mp_restore",80}},  20);
        add("goblin_tooth",  "Зуб гоблина",     "material",   0, {},                    5);
        add("gold_coin",     "Золотая монета",  "material",   0, {},                    1);
    }
};

// ═══════════════════════════════════════════════════════════════
// КАК ПОДКЛЮЧИТЬ В main.cpp:
// ═══════════════════════════════════════════════════════════════
//
// 1. #include "item_system.hpp"
//
// 2. В GameEngine:
//    ItemSystem itemSystem;
//    Inventory  playerInventory;
//
// 3. В initGame():
//    itemSystem.loadItems("assets/items.json");
//    itemSystem.loadDropTables("assets/drop_tables.json");
//    playerInventory.maxSlots = 20;
//
// 4. В onEnemyKilled(Enemy& e):
//    auto loot = itemSystem.rollLoot(getEntityName(e.type), e.level);
//    itemSystem.lootToInventory(playerInventory, loot);
//    for (auto& l : loot)
//        addFloatingText(e.pos, "+" + l.itemId, sf::Color::Yellow);
//
// 5. При использовании зелья:
//    if (playerInventory.has("health_potion")) {
//        playerInventory.remove("health_potion");
//        player.hp = std::min(player.maxHp, player.hp + 100.f);
//    }
