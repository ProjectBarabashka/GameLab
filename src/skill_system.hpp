// skill_system.hpp — AETHORIA: Система навыков (Приоритет 2 редмапа)
// Загружает skills.json, хранит кулдауны, вычисляет урон по формуле,
// интегрируется с EventBus через SkillUsedEvent.
//
// Формат assets/skills.json:
// [
//   { "id":"slash", "name":"Разящий удар", "icon":"⚔",
//     "type":"active", "mana":10, "cooldown":2.0,
//     "damage_formula":"STR*2.5+50", "target":"single",
//     "range":80, "aoe_radius":0,
//     "effects":[{"type":"stun","duration":1.0}],
//     "description":"Мощный удар." }
// ]

#pragma once
#include "json_parser.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <cmath>

// ═══════════════════════════════════════════════════════════════
// СТАТЫ КАСТЕРА (передаются при вычислении урона)
// ═══════════════════════════════════════════════════════════════
struct CasterStats {
    float STR = 10, DEX = 8, INT = 6, VIT = 9;
    float level = 1;
    float critChance = 0.05f;   // 5%
    float critMult   = 1.5f;
};

// ═══════════════════════════════════════════════════════════════
// ЭФФЕКТ СКИЛЛА (стан, яд, замедление и т.д.)
// ═══════════════════════════════════════════════════════════════
struct SkillEffect {
    std::string type;       // "stun", "poison", "slow", "burn", "heal"
    float       duration;
    float       potency;    // урон в секунду для дот, % замедления для slow и т.д.
};

// ═══════════════════════════════════════════════════════════════
// ОПИСАНИЕ СКИЛЛА (из JSON, шаблон)
// ═══════════════════════════════════════════════════════════════
struct SkillDef {
    std::string id;
    std::string name;
    std::string icon;
    std::string type;          // "active" | "passive"
    std::string target;        // "single" | "aoe" | "self"
    int         manaCost  = 0;
    float       cooldown  = 1.f;
    float       range     = 80.f;
    float       aoeRadius = 0.f;
    std::string damageFormula; // "STR*2.5+50"
    std::string description;
    std::vector<SkillEffect> effects;
};

// ═══════════════════════════════════════════════════════════════
// RUNTIME ЭКЗЕМПЛЯР СКИЛЛА (кулдаун у конкретного персонажа)
// ═══════════════════════════════════════════════════════════════
struct SkillInstance {
    std::string defId;          // ссылка на SkillDef
    float       currentCooldown = 0.f;
    int         level           = 1;    // уровень скилла (1-10)
    bool        unlocked        = true;

    bool isReady() const { return currentCooldown <= 0.f; }
};

// ═══════════════════════════════════════════════════════════════
// РЕЗУЛЬТАТ ПРИМЕНЕНИЯ СКИЛЛА
// ═══════════════════════════════════════════════════════════════
struct SkillResult {
    bool        success  = false;
    std::string reason;          // причина отказа
    float       damage   = 0.f;
    bool        isCrit   = false;
    std::vector<SkillEffect> appliedEffects;
};

// ═══════════════════════════════════════════════════════════════
// СИСТЕМА НАВЫКОВ
// ═══════════════════════════════════════════════════════════════
class SkillSystem {
public:
    // ── Загрузка из JSON ─────────────────────────────────────
    bool loadDefs(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "[SkillSystem] Файл не найден: " << path << "\n";
            _createDefaultDefs();
            return false;
        }
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isArray()) {
            std::cerr << "[SkillSystem] Невалидный JSON: " << path << "\n";
            return false;
        }
        defs_.clear();
        for (size_t i = 0; i < root->arrayVal.size(); i++) {
            auto j = root->get(i); if (!j) continue;
            SkillDef d;
            if (auto v = j->get("id"))              d.id            = v->asString();
            if (auto v = j->get("name"))            d.name          = v->asString();
            if (auto v = j->get("icon"))            d.icon          = v->asString();
            if (auto v = j->get("type"))            d.type          = v->asString();
            if (auto v = j->get("target"))          d.target        = v->asString();
            if (auto v = j->get("mana"))            d.manaCost      = v->asInt();
            if (auto v = j->get("cooldown"))        d.cooldown      = (float)v->asDouble();
            if (auto v = j->get("range"))           d.range         = (float)v->asDouble();
            if (auto v = j->get("aoe_radius"))      d.aoeRadius     = (float)v->asDouble();
            if (auto v = j->get("damage_formula"))  d.damageFormula = v->asString();
            if (auto v = j->get("description"))     d.description   = v->asString();
            if (auto fx = j->get("effects")) {
                for (size_t k = 0; k < fx->arrayVal.size(); k++) {
                    auto e = fx->get(k); if (!e) continue;
                    SkillEffect ef;
                    if (auto v = e->get("type"))     ef.type     = v->asString();
                    if (auto v = e->get("duration")) ef.duration = (float)v->asDouble();
                    if (auto v = e->get("potency"))  ef.potency  = (float)v->asDouble();
                    d.effects.push_back(ef);
                }
            }
            if (!d.id.empty()) defs_[d.id] = d;
        }
        std::cout << "[SkillSystem] Загружено скиллов: " << defs_.size()
                  << " из " << path << "\n";
        return !defs_.empty();
    }

    // ── Сохранение (для редактора) ───────────────────────────
    bool saveDefs(const std::string& path) const {
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << "[\n";
        bool first = true;
        for (auto& [id, d] : defs_) {
            if (!first) f << ",\n"; first = false;
            f << "  {\n";
            f << "    \"id\":\""             << _esc(d.id)            << "\",\n";
            f << "    \"name\":\""           << _esc(d.name)          << "\",\n";
            f << "    \"icon\":\""           << _esc(d.icon)          << "\",\n";
            f << "    \"type\":\""           << _esc(d.type)          << "\",\n";
            f << "    \"target\":\""         << _esc(d.target)        << "\",\n";
            f << "    \"mana\":"             << d.manaCost             << ",\n";
            f << "    \"cooldown\":"         << d.cooldown             << ",\n";
            f << "    \"range\":"            << d.range                << ",\n";
            f << "    \"aoe_radius\":"       << d.aoeRadius            << ",\n";
            f << "    \"damage_formula\":\"" << _esc(d.damageFormula)  << "\",\n";
            f << "    \"description\":\""   << _esc(d.description)    << "\",\n";
            f << "    \"effects\":[";
            bool fe = true;
            for (auto& ef : d.effects) {
                if (!fe) f << ","; fe = false;
                f << "{\"type\":\"" << _esc(ef.type) << "\""
                  << ",\"duration\":" << ef.duration
                  << ",\"potency\":" << ef.potency << "}";
            }
            f << "]\n  }";
        }
        f << "\n]\n";
        return true;
    }

    // ── Получить определение ────────────────────────────────
    const SkillDef* getDef(const std::string& id) const {
        auto it = defs_.find(id);
        return it != defs_.end() ? &it->second : nullptr;
    }

    const std::map<std::string, SkillDef>& allDefs() const { return defs_; }

    // ── Создать набор скиллов для класса ────────────────────
    // skillIds — список ID из класса
    std::vector<SkillInstance> createSkillSet(const std::vector<std::string>& skillIds) {
        std::vector<SkillInstance> set;
        for (auto& id : skillIds) {
            if (defs_.count(id)) {
                SkillInstance si;
                si.defId = id;
                set.push_back(si);
            }
        }
        return set;
    }

    // ── Обновление кулдаунов ────────────────────────────────
    void update(std::vector<SkillInstance>& skills, float dt) {
        for (auto& s : skills)
            if (s.currentCooldown > 0.f)
                s.currentCooldown = std::max(0.f, s.currentCooldown - dt);
    }

    // ── Применение скилла ──────────────────────────────────
    // idx — индекс в наборе skills
    // stats — статы кастера (STR, DEX, INT, VIT)
    // mana — текущая мана (изменится)
    SkillResult use(std::vector<SkillInstance>& skills, int idx,
                    const CasterStats& stats, float& mana)
    {
        SkillResult res;
        if (idx < 0 || idx >= (int)skills.size()) {
            res.reason = "Скилл не существует"; return res;
        }
        SkillInstance& si = skills[idx];
        if (!si.unlocked)    { res.reason = "Скилл не изучен"; return res; }
        if (!si.isReady())   { res.reason = "Кулдаун"; return res; }

        const SkillDef* def = getDef(si.defId);
        if (!def)            { res.reason = "Определение не найдено"; return res; }
        if (def->type == "passive") { res.reason = "Пассивный скилл"; return res; }
        if (mana < def->manaCost) { res.reason = "Недостаточно маны"; return res; }

        mana -= def->manaCost;
        si.currentCooldown = def->cooldown;

        // Вычисляем урон по формуле
        res.damage = _evalFormula(def->damageFormula, stats, si.level);

        // Крит
        float roll = (float)(rand() % 1000) / 1000.f;
        if (roll < stats.critChance) {
            res.isCrit = true;
            res.damage *= stats.critMult;
        }

        res.appliedEffects = def->effects;
        res.success = true;
        return res;
    }

    // Удобная версия по skillId напрямую
    SkillResult useById(std::vector<SkillInstance>& skills, const std::string& id,
                        const CasterStats& stats, float& mana)
    {
        for (int i = 0; i < (int)skills.size(); i++)
            if (skills[i].defId == id)
                return use(skills, i, stats, mana);
        SkillResult r; r.reason = "Скилл не найден: " + id; return r;
    }

    // Пассивный бонус (вызывай при level-up/equip)
    float passiveBonus(const std::vector<SkillInstance>& skills,
                       const std::string& defId,
                       const CasterStats& stats) const
    {
        const SkillDef* def = getDef(defId);
        if (!def || def->type != "passive") return 0.f;
        for (auto& si : skills)
            if (si.defId == defId && si.unlocked)
                return _evalFormula(def->damageFormula, stats, si.level);
        return 0.f;
    }

private:
    std::map<std::string, SkillDef> defs_;

    // Простой интерпретатор формул: STR*2.5+50, INT*3+level*5 и т.д.
    float _evalFormula(const std::string& formula,
                       const CasterStats& stats, int skillLevel) const
    {
        if (formula.empty() || formula == "0") return 0.f;
        // Подстановка переменных
        std::string expr = formula;
        auto replace = [&](const std::string& from, float val) {
            std::string sv = std::to_string(val);
            // Убираем лишние нули
            size_t dot = sv.find('.');
            if (dot != std::string::npos)
                sv = sv.substr(0, dot + 3);
            std::string out;
            size_t pos = 0;
            while ((pos = expr.find(from, pos)) != std::string::npos) {
                expr.replace(pos, from.size(), sv);
                pos += sv.size();
            }
        };
        replace("STR",   stats.STR);
        replace("DEX",   stats.DEX);
        replace("INT",   stats.INT);
        replace("VIT",   stats.VIT);
        replace("level", (float)skillLevel);

        // Простой парсинг: X*Y+Z, X+Y и т.д.
        float result = 0.f;
        try {
            // Сначала умножения, потом сложения/вычитания
            result = _parseExpr(expr);
        } catch (...) {
            result = 0.f;
        }
        return result;
    }

    float _parseExpr(const std::string& s) const {
        float sum = 0.f; float sign = 1.f;
        size_t i = 0;
        while (i <= s.size()) {
            size_t j = i;
            while (j < s.size() && s[j] != '+' && s[j] != '-') j++;
            std::string term = s.substr(i, j - i);
            // Умножение/деление
            float val = _parseTerm(term);
            sum += sign * val;
            if (j < s.size()) sign = (s[j] == '+') ? 1.f : -1.f;
            i = j + 1;
        }
        return sum;
    }

    float _parseTerm(const std::string& s) const {
        float prod = 1.f; bool first = true;
        size_t i = 0;
        while (i <= s.size()) {
            size_t j = i;
            while (j < s.size() && s[j] != '*' && s[j] != '/') j++;
            std::string factor = s.substr(i, j - i);
            if (!factor.empty()) {
                float v = 0.f;
                try { v = std::stof(factor); } catch (...) {}
                if (first) { prod = v; first = false; }
                else prod *= v;
            }
            if (j >= s.size()) break;
            i = j + 1;
        }
        return prod;
    }

    static std::string _esc(const std::string& s) {
        std::string o;
        for (char c : s) {
            if (c == '"') o += "\\\"";
            else if (c == '\\') o += "\\\\";
            else o += c;
        }
        return o;
    }

    void _createDefaultDefs() {
        // Создаём минимальный набор скиллов по умолчанию
        std::vector<SkillDef> defaults = {
            {"slash",    "Разящий удар",   "⚔",  "active", "single",  10, 2.f,  80.f, 0.f,   "STR*2.5+50",  "Мощный удар по одной цели."},
            {"fireball", "Огненный шар",   "🔥", "active", "aoe",     20, 1.5f, 200.f,80.f,  "INT*3.5+80",  "Взрыв в AoE."},
            {"heal",     "Лечение",        "💚", "active", "self",    15, 8.f,  0.f,  0.f,   "INT*2+VIT*3", "Восстанавливает HP."},
            {"dash",     "Рывок",          "💨", "active", "self",    10, 6.f,  0.f,  0.f,   "0",           "Рывок в направлении движения."},
        };
        for (auto& d : defaults) defs_[d.id] = d;
    }
};

// ═══════════════════════════════════════════════════════════════
// КАК ПОДКЛЮЧИТЬ В main.cpp:
// ═══════════════════════════════════════════════════════════════
//
// 1. #include "skill_system.hpp"
//
// 2. В GameEngine:
//    SkillSystem skillSystem;
//    std::vector<SkillInstance> playerSkills;
//
// 3. В initGame():
//    skillSystem.loadDefs("assets/skills.json");
//    playerSkills = skillSystem.createSkillSet({"slash","fireball","heal","dash"});
//
// 4. В update():
//    skillSystem.update(playerSkills, dt);
//
// 5. В useSkill(int index):
//    CasterStats stats;
//    stats.STR = player.str; stats.DEX = player.dex;
//    stats.INT = player.intl; stats.VIT = player.vit;
//    auto result = skillSystem.use(playerSkills, index, stats, player.mp);
//    if (result.success && selectedTarget) {
//        selectedTarget->hp -= result.damage;
//        if (result.isCrit) addFloatingText(selectedTarget->pos, "КРИТ!", sf::Color::Yellow);
//        SkillUsedEvent ev;
//        ev.skillId = playerSkills[index].defId;
//        ev.damage  = (int)result.damage;
//        eventBus.emit(ev);
//    }
