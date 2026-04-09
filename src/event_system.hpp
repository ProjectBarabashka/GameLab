// event_system.hpp — AETHORIA: EventBus (Приоритет 1 редмапа)
// Даёт миру "жизнь": квесты, триггеры, UI, звук — всё реагирует на события
// через подписки, без жёсткой связи между системами.
//
// Использование:
//   eventBus.on<EnemyKilledEvent>([&](const EnemyKilledEvent& e){
//       questSystem.onKill(e.enemyName);
//   });
//   // где-то в update:
//   eventBus.emit(EnemyKilledEvent{"GOBLIN", 3, player.pos});

#pragma once
#include <functional>
#include <vector>
#include <map>
#include <typeindex>
#include <memory>
#include <string>

// ═══════════════════════════════════════════════════════════════
// БАЗОВЫЙ ТИП СОБЫТИЯ
// ═══════════════════════════════════════════════════════════════
struct GameEvent {
    virtual ~GameEvent() = default;
};

// ═══════════════════════════════════════════════════════════════
// КОНКРЕТНЫЕ СОБЫТИЯ
// ═══════════════════════════════════════════════════════════════

// Убийство врага
struct EnemyKilledEvent : GameEvent {
    std::string enemyName;   // "GOBLIN", "Forest Goblin Lv.3" и т.д.
    int         enemyLevel;
    float       posX, posY;
    int         goldDrop;
    int         xpReward;
};

// Игрок подобрал предмет
struct ItemPickupEvent : GameEvent {
    std::string itemId;
    std::string itemName;
    int         count;
};

// Игрок вошёл в триггер-зону
struct TriggerZoneEvent : GameEvent {
    std::string triggerId;
    bool        entering;    // true=enter, false=leave
};

// Квест завершён
struct QuestCompletedEvent : GameEvent {
    std::string questId;
    std::string questName;
    int         xpReward;
    int         goldReward;
};

// Квест принят
struct QuestAcceptedEvent : GameEvent {
    std::string questId;
    std::string giverNpc;
};

// Игрок повысил уровень
struct LevelUpEvent : GameEvent {
    int oldLevel;
    int newLevel;
};

// Игрок умер
struct PlayerDeadEvent : GameEvent {
    float posX, posY;
    std::string killedBy;
};

// Использование скилла
struct SkillUsedEvent : GameEvent {
    std::string skillId;
    std::string casterId;    // "player" или enemy ID
    float targetX, targetY;
    int   damage;
};

// Диалог NPC
struct NpcDialogueEvent : GameEvent {
    std::string npcName;
    std::string message;
    float       duration;
};

// Сцена переключилась
struct SceneSwitchedEvent : GameEvent {
    std::string fromScene;
    std::string toScene;
};

// Предмет из инвентаря использован
struct ItemUsedEvent : GameEvent {
    std::string itemId;
    bool        consumed;    // true = предмет расходуется
};

// Произвольное событие с тегом (для редактора)
struct CustomEvent : GameEvent {
    std::string tag;
    std::map<std::string, std::string> data;
};

// ═══════════════════════════════════════════════════════════════
// ШИНА СОБЫТИЙ
// ═══════════════════════════════════════════════════════════════
class EventBus {
public:
    using HandlerID = size_t;

    // ── Подписка ─────────────────────────────────────────────
    // Возвращает ID хендлера для отписки
    template<typename E>
    HandlerID on(std::function<void(const E&)> handler) {
        auto key = std::type_index(typeid(E));
        HandlerID id = nextId_++;
        handlers_[key].push_back({id, [h = std::move(handler)](const GameEvent& e) {
            h(static_cast<const E&>(e));
        }});
        return id;
    }

    // ── Отписка по ID ──────────────────────────────────────
    void off(HandlerID id) {
        for (auto& [key, vec] : handlers_) {
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                [id](const Handler& h){ return h.id == id; }),
                vec.end());
        }
    }

    // ── Публикация события ─────────────────────────────────
    template<typename E>
    void emit(const E& event) {
        auto key = std::type_index(typeid(E));
        auto it = handlers_.find(key);
        if (it == handlers_.end()) return;
        // Копируем список — handler может отписаться внутри
        auto handlers_copy = it->second;
        for (auto& h : handlers_copy) {
            h.fn(event);
        }
    }

    // ── Отложенная очередь (safe для update loop) ──────────
    template<typename E>
    void enqueue(const E& event) {
        pending_.push_back(std::make_shared<E>(event));
    }

    // Вызывай раз в конце update() для flush очереди
    void flush() {
        auto tmp = std::move(pending_);
        for (auto& e : tmp) {
            auto key = std::type_index(typeid(*e));
            auto it = handlers_.find(key);
            if (it == handlers_.end()) continue;
            auto hcopy = it->second;
            for (auto& h : hcopy) h.fn(*e);
        }
    }

    void clearAll() { handlers_.clear(); pending_.clear(); }

private:
    struct Handler {
        HandlerID id;
        std::function<void(const GameEvent&)> fn;
    };
    std::map<std::type_index, std::vector<Handler>> handlers_;
    std::vector<std::shared_ptr<GameEvent>> pending_;
    HandlerID nextId_ = 1;
};

// ═══════════════════════════════════════════════════════════════
// КАК ПОДКЛЮЧИТЬ В main.cpp:
// ═══════════════════════════════════════════════════════════════
//
// 1. #include "event_system.hpp"
//
// 2. В GameEngine добавить поле:
//    EventBus eventBus;
//
// 3. В initEventHandlers() (вызови из init):
//    eventBus.on<EnemyKilledEvent>([&](const EnemyKilledEvent& ev){
//        auto results = questSystem.onKill(ev.enemyName);
//        for (auto& r : results) { player.xp += r.xpRewarded; player.gold += r.goldRewarded; }
//    });
//    eventBus.on<LevelUpEvent>([&](const LevelUpEvent& ev){
//        audioManager->playSound("levelup");
//        addFloatingText(player.pos, "LEVEL UP!", sf::Color::Yellow);
//    });
//    eventBus.on<QuestCompletedEvent>([&](const QuestCompletedEvent& ev){
//        questNotifyText = "Квест выполнен: " + ev.questName;
//        questNotifyTimer = 4.f;
//    });
//
// 4. В onEnemyKilled(Enemy& e) ВМЕСТО прямых вызовов:
//    EnemyKilledEvent ev;
//    ev.enemyName  = getEntityName(e.type);
//    ev.enemyLevel = e.level;
//    ev.posX = e.pos.x; ev.posY = e.pos.y;
//    ev.goldDrop  = gold;
//    ev.xpReward  = xp;
//    eventBus.emit(ev);
//
// 5. В конце update():
//    eventBus.flush();
