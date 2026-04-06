// quest_system.hpp — Система квестов AETHORIA: Eternal Realms
// Читает assets/quests.json (формат редактора), отслеживает прогресс,
// выдаёт награды, дедуплицирует objectives и IDs.
//
// Интеграция в Game:
//   QuestSystem questSystem;
//   questSystem.load("assets/quests.json");
//   // при убийстве:
//   auto results = questSystem.onKill(enemy.name);
//   for (auto& r : results) { player.xp += r.xpRewarded; player.gold += r.goldRewarded; }
//   // при взаимодействии с NPC квестодателем:
//   auto ir = questSystem.interact(npc.props.getStr("quest_id"), player.level);

#pragma once
#include "json_parser.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

// ════════════════════════════════════════════════════════════════
// ТИПЫ ЦЕЛЕЙ КВЕСТА
// ════════════════════════════════════════════════════════════════
enum class ObjectiveType {
    KILL,
    COLLECT,
    TALK,
    EXPLORE,
    DELIVER,
    ESCORT
};

inline std::string objectiveTypeStr(ObjectiveType t) {
    switch (t) {
        case ObjectiveType::KILL:    return "kill";
        case ObjectiveType::COLLECT: return "collect";
        case ObjectiveType::TALK:    return "talk";
        case ObjectiveType::EXPLORE: return "explore";
        case ObjectiveType::DELIVER: return "deliver";
        case ObjectiveType::ESCORT:  return "escort";
        default: return "kill";
    }
}

inline ObjectiveType objectiveTypeFromStr(const std::string& s) {
    if (s == "collect") return ObjectiveType::COLLECT;
    if (s == "talk")    return ObjectiveType::TALK;
    if (s == "explore") return ObjectiveType::EXPLORE;
    if (s == "deliver") return ObjectiveType::DELIVER;
    if (s == "escort")  return ObjectiveType::ESCORT;
    return ObjectiveType::KILL;
}

// ════════════════════════════════════════════════════════════════
// ЦЕЛЬ КВЕСТА
// ════════════════════════════════════════════════════════════════
struct QuestObjective {
    std::string    id;        // авто: "kill_GOBLIN"
    ObjectiveType  type;
    std::string    target;    // "GOBLIN", "Волчий клык", "Капитан Грэй" …
    int            required;  // нужно
    int            current;   // есть
    bool           completed;

    bool isComplete() const { return current >= required; }

    void progress(int n = 1) {
        current    = std::min(current + n, required);
        completed  = isComplete();
    }

    // Для отображения в HUD: "Убий GOBLIN: 3/5"
    std::string statusLine() const {
        return target + ": " + std::to_string(current) + "/" + std::to_string(required);
    }
};

// ════════════════════════════════════════════════════════════════
// НАГРАДА КВЕСТА
// ════════════════════════════════════════════════════════════════
struct QuestReward {
    int goldMin = 0;
    int goldMax = 0;
    int xp      = 0;
    std::vector<std::string> items;

    int rollGold() const {
        int range = std::max(1, goldMax - goldMin);
        return goldMin + rand() % range;
    }
};

// ════════════════════════════════════════════════════════════════
// ДИАЛОГ КВЕСТА
// ════════════════════════════════════════════════════════════════
struct QuestDialogue {
    std::vector<std::string> offer;    // NPC предлагает квест
    std::vector<std::string> progress; // квест активен, не выполнен
    std::vector<std::string> complete; // квест выполнен
};

// ════════════════════════════════════════════════════════════════
// СТАТУС КВЕСТА
// ════════════════════════════════════════════════════════════════
enum class QuestStatus {
    AVAILABLE,  // доступен, не взят
    ACTIVE,     // взят, в процессе
    COMPLETED,  // выполнен
    FAILED      // провален
};

inline std::string questStatusStr(QuestStatus s) {
    switch (s) {
        case QuestStatus::AVAILABLE: return "available";
        case QuestStatus::ACTIVE:    return "active";
        case QuestStatus::COMPLETED: return "completed";
        case QuestStatus::FAILED:    return "failed";
        default: return "available";
    }
}

// ════════════════════════════════════════════════════════════════
// КВЕСТ
// ════════════════════════════════════════════════════════════════
struct Quest {
    std::string  id;
    std::string  name;
    std::string  description;
    int          levelReq = 1;
    std::string  type;      // "kill", "collect", "talk" …
    std::string  giverNpc;  // Имя NPC (совпадает с Entity.name)
    QuestStatus  status = QuestStatus::AVAILABLE;

    std::vector<QuestObjective> objectives;
    QuestReward                 reward;
    QuestDialogue               dialogue;

    bool isComplete() const {
        if (objectives.empty()) return false;
        return std::all_of(objectives.begin(), objectives.end(),
            [](const QuestObjective& o) { return o.isComplete(); });
    }

    int completedObjectiveCount() const {
        return (int)std::count_if(objectives.begin(), objectives.end(),
            [](const QuestObjective& o) { return o.isComplete(); });
    }

    // Краткий статус для HUD-лога
    std::string summary() const {
        return "[" + id + "] " + name + " (" +
               std::to_string(completedObjectiveCount()) + "/" +
               std::to_string((int)objectives.size()) + " целей)";
    }
};

// ════════════════════════════════════════════════════════════════
// РЕЗУЛЬТАТ СОБЫТИЯ (убийство / сбор / диалог)
// ════════════════════════════════════════════════════════════════
struct QuestEventResult {
    bool        questCompleted = false;
    std::string questId;
    std::string questName;
    int         goldRewarded   = 0;
    int         xpRewarded     = 0;
    std::vector<std::string> itemsRewarded;
};

// ════════════════════════════════════════════════════════════════
// РЕЗУЛЬТАТ ВЗАИМОДЕЙСТВИЯ С NPC
// ════════════════════════════════════════════════════════════════
struct QuestInteractResult {
    std::vector<std::string> lines;
    bool questAssigned = false;
    bool questComplete = false;
    int  goldRewarded  = 0;
    int  xpRewarded    = 0;
    std::vector<std::string> items;
    // Пустая линия — нет диалога
    bool empty() const { return lines.empty(); }
    // Первая строка для однострочного диалога
    std::string firstLine() const { return lines.empty() ? "" : lines[0]; }
    // Все строки через \n
    std::string allLines() const {
        std::string r;
        for (size_t i = 0; i < lines.size(); i++) {
            if (i) r += "\n";
            r += lines[i];
        }
        return r;
    }
};

// ════════════════════════════════════════════════════════════════
// СИСТЕМА КВЕСТОВ
// ════════════════════════════════════════════════════════════════
class QuestSystem {
private:
    std::vector<Quest>           quests;
    std::map<std::string,size_t> index;     // id → позиция в quests
    std::string                  filePath;

    // ── Вспомогательные ──────────────────────────────────────────

    void rebuildIndex() {
        index.clear();
        for (size_t i = 0; i < quests.size(); i++)
            index[quests[i].id] = i;
    }

    // Проверяет завершение квеста и возвращает результат если да
    std::vector<QuestEventResult> tryComplete(Quest& q) {
        std::vector<QuestEventResult> res;
        if (q.status == QuestStatus::ACTIVE && q.isComplete()) {
            q.status = QuestStatus::COMPLETED;
            QuestEventResult r;
            r.questCompleted  = true;
            r.questId         = q.id;
            r.questName       = q.name;
            r.goldRewarded    = q.reward.rollGold();
            r.xpRewarded      = q.reward.xp;
            r.itemsRewarded   = q.reward.items;
            res.push_back(std::move(r));
            std::cout << "[Quest] ВЫПОЛНЕН: " << q.name
                      << " (+" << res[0].xpRewarded << " XP, +"
                      << res[0].goldRewarded << " gold)\n";
        }
        return res;
    }

    // Нормализует строку в верхний регистр для сравнения
    static std::string toUpper(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), ::toupper);
        return r;
    }

    // Проверяет совпадение цели: "GOBLIN" совпадёт с "Goblin Warrior", "Forest Goblin" и т.д.
    static bool matchTarget(const std::string& target, const std::string& input) {
        std::string t = toUpper(target), i = toUpper(input);
        return i.find(t) != std::string::npos || t.find(i) != std::string::npos;
    }

    // Загружает массив строк из JSON
    static void loadStrArray(const SimpleJSON::Value* node, const std::string& key,
                              std::vector<std::string>& out)
    {
        if (!node) return;
        auto arr = node->get(key);
        if (!arr || !arr->isArray()) return;
        for (size_t k = 0; k < arr->arrayVal.size(); k++) {
            auto v = arr->get(k);
            if (v) out.push_back(v->asString());
        }
    }

    static std::string escJ(const std::string& s) {
        std::string o;
        for (char c : s) {
            if      (c == '"')  o += "\\\"";
            else if (c == '\\') o += "\\\\";
            else if (c == '\n') o += "\\n";
            else                o += c;
        }
        return o;
    }

public:
    explicit QuestSystem(const std::string& path = "assets/quests.json")
        : filePath(path) {}

    // ════════════════════════════════════════════════════════════
    // ЗАГРУЗКА / СОХРАНЕНИЕ
    // ════════════════════════════════════════════════════════════

    bool load(const std::string& path = "") {
        const std::string& p = path.empty() ? filePath : path;
        if (!path.empty()) filePath = path;

        std::ifstream f(p);
        if (!f.is_open()) {
            std::cerr << "[Quest] Файл не найден: " << p
                      << " — квесты не загружены\n";
            return false;
        }
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isArray()) {
            std::cerr << "[Quest] Невалидный JSON (ожидается массив): " << p << "\n";
            return false;
        }

        quests.clear();
        index.clear();
        int skipped = 0;

        for (size_t i = 0; i < root->arrayVal.size(); i++) {
            auto qn = root->get(i);
            if (!qn || !qn->isObject()) continue;

            Quest q;
            if (auto v = qn->get("id"))          q.id          = v->asString();
            if (auto v = qn->get("name"))         q.name        = v->asString();
            if (auto v = qn->get("description"))  q.description = v->asString();
            if (auto v = qn->get("level_req"))    q.levelReq    = v->asInt();
            if (auto v = qn->get("type"))         q.type        = v->asString();
            if (auto v = qn->get("giver_npc"))    q.giverNpc    = v->asString();

            // Восстановление статуса (при загрузке сохранения)
            if (auto v = qn->get("status")) {
                std::string st = v->asString();
                if      (st == "active")    q.status = QuestStatus::ACTIVE;
                else if (st == "completed") q.status = QuestStatus::COMPLETED;
                else if (st == "failed")    q.status = QuestStatus::FAILED;
                else                        q.status = QuestStatus::AVAILABLE;
            }

            // ── Objectives — с дедупликацией ─────────────────────
            if (auto objs = qn->get("objectives")) {
                std::set<std::string> seen;
                for (size_t j = 0; j < objs->arrayVal.size(); j++) {
                    auto on = objs->get(j);
                    if (!on) continue;
                    QuestObjective obj;
                    std::string otype, otarget;
                    if (auto v = on->get("type"))    otype    = v->asString();
                    if (auto v = on->get("target"))  otarget  = v->asString();
                    if (auto v = on->get("count"))   obj.required = std::max(1, v->asInt());
                    if (auto v = on->get("current")) obj.current  = v->asInt();
                    obj.type      = objectiveTypeFromStr(otype);
                    obj.target    = otarget;
                    obj.id        = otype + "_" + otarget;
                    obj.completed = (obj.current >= obj.required);

                    // Ключ дедупликации: тип + цель
                    std::string dedupeKey = otype + "|" + toUpper(otarget);
                    if (seen.count(dedupeKey)) {
                        std::cerr << "[Quest] ДУБЛЬ objective пропущен ["
                                  << q.id << "]: " << dedupeKey << "\n";
                        skipped++;
                        continue;
                    }
                    seen.insert(dedupeKey);
                    q.objectives.push_back(std::move(obj));
                }
            }

            // ── Rewards ───────────────────────────────────────────
            if (auto rw = qn->get("rewards")) {
                if (auto v = rw->get("gold_min")) q.reward.goldMin = v->asInt();
                if (auto v = rw->get("gold_max")) q.reward.goldMax = v->asInt();
                if (auto v = rw->get("xp"))       q.reward.xp      = v->asInt();
                loadStrArray(rw.get(), "items", q.reward.items);
            }

            // ── Dialogue ──────────────────────────────────────────
            if (auto dlg = qn->get("dialogue")) {
                loadStrArray(dlg.get(), "offer",    q.dialogue.offer);
                loadStrArray(dlg.get(), "progress", q.dialogue.progress);
                loadStrArray(dlg.get(), "complete", q.dialogue.complete);
            }

            // Дедупликация квестов по ID
            if (q.id.empty()) {
                std::cerr << "[Quest] Квест без ID пропущен (индекс " << i << ")\n";
                skipped++;
                continue;
            }
            if (index.count(q.id)) {
                std::cerr << "[Quest] ДУБЛЬ квеста пропущен: " << q.id << "\n";
                skipped++;
                continue;
            }

            index[q.id] = quests.size();
            quests.push_back(std::move(q));
        }

        std::cout << "[Quest] Загружено " << quests.size() << " квестов"
                  << (skipped > 0 ? " (пропущено дублей: " + std::to_string(skipped) + ")" : "")
                  << " из " << p << "\n";
        return !quests.empty();
    }

    // Сохраняет прогресс (статусы + current для objectives)
    bool save(const std::string& path = "") const {
        const std::string& p = path.empty() ? filePath : path;
        std::ofstream f(p);
        if (!f.is_open()) {
            std::cerr << "[Quest] Не удалось сохранить: " << p << "\n";
            return false;
        }
        f << "[\n";
        for (size_t qi = 0; qi < quests.size(); qi++) {
            const auto& q = quests[qi];
            f << "  {\n";
            f << "    \"id\": \""          << escJ(q.id)          << "\",\n";
            f << "    \"name\": \""        << escJ(q.name)        << "\",\n";
            f << "    \"level_req\": "     << q.levelReq          << ",\n";
            f << "    \"type\": \""        << escJ(q.type)        << "\",\n";
            f << "    \"giver_npc\": \""   << escJ(q.giverNpc)    << "\",\n";
            f << "    \"description\": \"" << escJ(q.description) << "\",\n";
            f << "    \"status\": \""      << questStatusStr(q.status) << "\",\n";
            f << "    \"objectives\": [\n";
            for (size_t oi = 0; oi < q.objectives.size(); oi++) {
                const auto& obj = q.objectives[oi];
                f << "      {\"type\":\""   << objectiveTypeStr(obj.type) << "\""
                  << ",\"target\":\""       << escJ(obj.target) << "\""
                  << ",\"count\":"          << obj.required
                  << ",\"current\":"        << obj.current << "}";
                if (oi + 1 < q.objectives.size()) f << ",";
                f << "\n";
            }
            f << "    ],\n";
            f << "    \"rewards\": {\"gold_min\":" << q.reward.goldMin
              << ",\"gold_max\":" << q.reward.goldMax
              << ",\"xp\":" << q.reward.xp << ",\"items\":[";
            for (size_t ii = 0; ii < q.reward.items.size(); ii++) {
                if (ii) f << ",";
                f << "\"" << escJ(q.reward.items[ii]) << "\"";
            }
            f << "]},\n";
            // Dialogue (сохраняем первую строку каждого)
            f << "    \"active\": " << (q.status == QuestStatus::ACTIVE ? "true" : "false") << "\n";
            f << "  }";
            if (qi + 1 < quests.size()) f << ",";
            f << "\n";
        }
        f << "]\n";
        std::cout << "[Quest] Прогресс сохранён → " << p << "\n";
        return true;
    }

    // ════════════════════════════════════════════════════════════
    // ВЗАИМОДЕЙСТВИЕ С NPC
    // ════════════════════════════════════════════════════════════

    // Вызывается когда игрок нажимает [E] на NPC квестодателе.
    // questId — props.getStr("quest_id") у NPC.
    QuestInteractResult interact(const std::string& questId, int playerLevel) {
        QuestInteractResult result;

        // Нет quest_id — стандартный NPC
        if (questId.empty()) {
            result.lines = {"Привет, путник!"};
            return result;
        }

        Quest* q = getById(questId);
        if (!q) {
            result.lines = {"У меня нет для тебя задания."};
            return result;
        }

        switch (q->status) {
            case QuestStatus::AVAILABLE:
                if (playerLevel < q->levelReq) {
                    result.lines = {
                        "Тебе нужен " + std::to_string(q->levelReq) + " уровень.",
                        "Возвращайся, когда повзрослеешь."
                    };
                } else {
                    q->status = QuestStatus::ACTIVE;
                    result.questAssigned = true;
                    result.lines = !q->dialogue.offer.empty() ?
                        q->dialogue.offer :
                        std::vector<std::string>{"Задание принято: " + q->name,
                                                  q->description};
                }
                break;

            case QuestStatus::ACTIVE:
                if (q->isComplete()) {
                    // Выдаём награду
                    q->status = QuestStatus::COMPLETED;
                    result.questComplete  = true;
                    result.goldRewarded   = q->reward.rollGold();
                    result.xpRewarded     = q->reward.xp;
                    result.items          = q->reward.items;
                    result.lines = !q->dialogue.complete.empty() ?
                        q->dialogue.complete :
                        std::vector<std::string>{"Превосходно! Задание выполнено!"};
                    std::cout << "[Quest] ЗАВЕРШЁН через NPC: " << q->name
                              << " (+" << result.xpRewarded << " XP, +"
                              << result.goldRewarded << " gold)\n";
                } else {
                    // Показываем прогресс
                    result.lines = !q->dialogue.progress.empty() ?
                        q->dialogue.progress :
                        std::vector<std::string>{"Продолжай, у тебя получится!"};
                    // Добавляем строку прогресса для каждой незаконченной цели
                    for (auto& obj : q->objectives) {
                        if (!obj.isComplete()) {
                            result.lines.push_back("  → " + obj.statusLine());
                        }
                    }
                }
                break;

            case QuestStatus::COMPLETED:
                result.lines = {"Ты уже выполнил это задание.",
                                 "Удачи в следующих приключениях!"};
                break;

            case QuestStatus::FAILED:
                result.lines = {"Это задание было провалено."};
                break;
        }
        return result;
    }

    // ════════════════════════════════════════════════════════════
    // СОБЫТИЯ ПРОГРЕССА
    // ════════════════════════════════════════════════════════════

    // Убийство врага. enemyName: "GOBLIN", "Forest Goblin Lv.3" и т.д.
    std::vector<QuestEventResult> onKill(const std::string& enemyName) {
        std::vector<QuestEventResult> all;
        for (auto& q : quests) {
            if (q.status != QuestStatus::ACTIVE) continue;
            bool progressed = false;
            for (auto& obj : q.objectives) {
                if (obj.type != ObjectiveType::KILL || obj.completed) continue;
                if (matchTarget(obj.target, enemyName)) {
                    obj.progress();
                    progressed = true;
                    std::cout << "[Quest] " << q.name
                              << " — " << obj.statusLine() << "\n";
                }
            }
            if (progressed) {
                auto r = tryComplete(q);
                all.insert(all.end(), r.begin(), r.end());
            }
        }
        return all;
    }

    // Сбор предмета. itemName: "Волчий клык", "iron_ore" и т.д.
    std::vector<QuestEventResult> onCollect(const std::string& itemName, int count = 1) {
        std::vector<QuestEventResult> all;
        for (auto& q : quests) {
            if (q.status != QuestStatus::ACTIVE) continue;
            bool progressed = false;
            for (auto& obj : q.objectives) {
                if (obj.type != ObjectiveType::COLLECT || obj.completed) continue;
                if (matchTarget(obj.target, itemName)) {
                    for (int c = 0; c < count; c++) obj.progress();
                    progressed = true;
                    std::cout << "[Quest] " << q.name
                              << " — " << obj.statusLine() << "\n";
                }
            }
            if (progressed) {
                auto r = tryComplete(q);
                all.insert(all.end(), r.begin(), r.end());
            }
        }
        return all;
    }

    // Разговор с NPC
    std::vector<QuestEventResult> onTalk(const std::string& npcName) {
        std::vector<QuestEventResult> all;
        for (auto& q : quests) {
            if (q.status != QuestStatus::ACTIVE) continue;
            bool progressed = false;
            for (auto& obj : q.objectives) {
                if (obj.type != ObjectiveType::TALK || obj.completed) continue;
                if (matchTarget(obj.target, npcName)) {
                    obj.progress();
                    progressed = true;
                }
            }
            if (progressed) {
                auto r = tryComplete(q);
                all.insert(all.end(), r.begin(), r.end());
            }
        }
        return all;
    }

    // Исследование зоны
    std::vector<QuestEventResult> onExplore(const std::string& zoneName) {
        std::vector<QuestEventResult> all;
        for (auto& q : quests) {
            if (q.status != QuestStatus::ACTIVE) continue;
            bool progressed = false;
            for (auto& obj : q.objectives) {
                if (obj.type != ObjectiveType::EXPLORE || obj.completed) continue;
                if (matchTarget(obj.target, zoneName)) {
                    obj.progress();
                    progressed = true;
                }
            }
            if (progressed) {
                auto r = tryComplete(q);
                all.insert(all.end(), r.begin(), r.end());
            }
        }
        return all;
    }

    // ════════════════════════════════════════════════════════════
    // ПОЛУЧЕНИЕ ДАННЫХ
    // ════════════════════════════════════════════════════════════

    Quest* getById(const std::string& id) {
        auto it = index.find(id);
        return it != index.end() ? &quests[it->second] : nullptr;
    }

    const Quest* getById(const std::string& id) const {
        auto it = index.find(id);
        return it != index.end() ? &quests[it->second] : nullptr;
    }

    // Найти квест по имени NPC-квестодателя
    Quest* getByNpc(const std::string& npcName) {
        for (auto& q : quests)
            if (q.giverNpc == npcName) return &q;
        return nullptr;
    }

    std::vector<Quest*> getActive() {
        std::vector<Quest*> r;
        for (auto& q : quests)
            if (q.status == QuestStatus::ACTIVE) r.push_back(&q);
        return r;
    }

    std::vector<Quest*> getAvailable() {
        std::vector<Quest*> r;
        for (auto& q : quests)
            if (q.status == QuestStatus::AVAILABLE) r.push_back(&q);
        return r;
    }

    std::vector<Quest*> getCompleted() {
        std::vector<Quest*> r;
        for (auto& q : quests)
            if (q.status == QuestStatus::COMPLETED) r.push_back(&q);
        return r;
    }

    bool hasActiveQuests() const {
        return std::any_of(quests.begin(), quests.end(),
            [](const Quest& q){ return q.status == QuestStatus::ACTIVE; });
    }

    size_t activeCount()    const {
        return std::count_if(quests.begin(), quests.end(),
            [](const Quest& q){ return q.status == QuestStatus::ACTIVE; });
    }

    size_t completedCount() const {
        return std::count_if(quests.begin(), quests.end(),
            [](const Quest& q){ return q.status == QuestStatus::COMPLETED; });
    }

    size_t totalCount()     const { return quests.size(); }

    // ════════════════════════════════════════════════════════════
    // ОТЛАДКА
    // ════════════════════════════════════════════════════════════

    void printStatus() const {
        std::cout << "╔══════════════════════ QUEST LOG ══════════════════════╗\n";
        std::cout << "║ Всего: " << quests.size()
                  << "  Активных: "  << activeCount()
                  << "  Выполнено: " << completedCount() << "\n";
        for (auto& q : quests) {
            char mark = ' ';
            if      (q.status == QuestStatus::ACTIVE)    mark = '*';
            else if (q.status == QuestStatus::COMPLETED) mark = '+';
            else if (q.status == QuestStatus::FAILED)    mark = 'x';
            std::cout << "║ [" << mark << "] " << q.id << " — " << q.name << "\n";
            for (auto& obj : q.objectives) {
                std::cout << "║    " << objectiveTypeStr(obj.type)
                          << " " << obj.statusLine();
                if (obj.completed) std::cout << " ✓";
                std::cout << "\n";
            }
        }
        std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    }
};
