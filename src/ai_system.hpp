// ai_system.hpp — AETHORIA: AI State Machine (Приоритет 3 редмапа)
// Выносит логику AI из main.cpp в отдельную систему.
// FSM состояния: IDLE → PATROL → AGGRO → COMBAT → RETURN
// Можно настраивать через JSON или редактор.

#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cmath>
#include <iostream>

// ═══════════════════════════════════════════════════════════════
// СОСТОЯНИЯ AI
// ═══════════════════════════════════════════════════════════════
enum class AIState {
    IDLE,      // стоит, смотрит по сторонам
    PATROL,    // ходит по patrol-точкам
    AGGRO,     // заметил игрока, идёт к нему
    COMBAT,    // рядом с игроком, атакует
    RETURN,    // возвращается на spawn-позицию
    DEAD
};

inline std::string aiStateStr(AIState s) {
    switch (s) {
        case AIState::IDLE:    return "IDLE";
        case AIState::PATROL:  return "PATROL";
        case AIState::AGGRO:   return "AGGRO";
        case AIState::COMBAT:  return "COMBAT";
        case AIState::RETURN:  return "RETURN";
        case AIState::DEAD:    return "DEAD";
        default: return "IDLE";
    }
}

// ═══════════════════════════════════════════════════════════════
// ТОЧКА ПАТРУЛЯ
// ═══════════════════════════════════════════════════════════════
struct PatrolPoint {
    float x, y;
    float waitTime = 0.f;  // сколько ждать на этой точке
};

// ═══════════════════════════════════════════════════════════════
// ПАРАМЕТРЫ AI (настраиваются из JSON / редактора)
// ═══════════════════════════════════════════════════════════════
struct AIParams {
    float aggroRange    = 200.f;   // дистанция обнаружения игрока
    float deaggroRange  = 450.f;   // дистанция потери агрессии
    float combatRange   = 40.f;    // дистанция начала атаки
    float returnRange   = 30.f;    // считаем "вернулись", если в этом радиусе от спавна
    float patrolSpeed   = 60.f;
    float aggroSpeed    = 100.f;
    float combatSpeed   = 80.f;
    float attackCooldown = 1.5f;
    bool  canPatrol     = true;
    bool  isBoss        = false;
    bool  fleeAtLowHp   = false;
    float fleeHpThreshold = 0.15f; // 15% HP
};

// ═══════════════════════════════════════════════════════════════
// AI КОМПОНЕНТ (одна сущность)
// ═══════════════════════════════════════════════════════════════
struct AIComponent {
    AIState    state     = AIState::IDLE;
    AIState    prevState = AIState::IDLE;
    AIParams   params;

    // Позиция спавна (точка возврата)
    float spawnX = 0.f, spawnY = 0.f;

    // Патруль
    std::vector<PatrolPoint> patrolPath;
    int   patrolIdx    = 0;
    float patrolWait   = 0.f;

    // Таймеры
    float idleTimer    = 0.f;
    float idleDuration = 2.f;  // сколько стоять перед следующим патрулем
    float attackTimer  = 0.f;
    float stateTimer   = 0.f;  // сколько в текущем состоянии

    // Результат тика (заполняется update)
    float velX = 0.f, velY = 0.f;  // желаемая скорость
    bool  shouldAttack = false;

    // Callback при смене состояния (опционально)
    std::function<void(AIState from, AIState to)> onStateChange;

    void setState(AIState s) {
        if (s == state) return;
        prevState = state;
        state = s;
        stateTimer = 0.f;
        if (onStateChange) onStateChange(prevState, s);
    }
};

// ═══════════════════════════════════════════════════════════════
// AI СИСТЕМА
// ═══════════════════════════════════════════════════════════════
class AISystem {
public:
    // Обновить одного AI-агента
    // posX/posY     — текущая позиция агента
    // targetX/targetY — позиция игрока/цели
    // hp / maxHp    — HP агента (для flee логики)
    // dt            — delta time
    void update(AIComponent& ai,
                float posX, float posY,
                float targetX, float targetY,
                float hp, float maxHp,
                float dt)
    {
        if (ai.state == AIState::DEAD) { ai.velX = ai.velY = 0.f; return; }

        ai.stateTimer += dt;
        ai.attackTimer = std::max(0.f, ai.attackTimer - dt);
        ai.shouldAttack = false;

        float distToTarget = _dist(posX, posY, targetX, targetY);
        float distToSpawn  = _dist(posX, posY, ai.spawnX, ai.spawnY);
        float hpPct        = (maxHp > 0.f) ? hp / maxHp : 1.f;

        switch (ai.state) {
        case AIState::IDLE:
            _tickIdle(ai, posX, posY, targetX, targetY, distToTarget, dt);
            break;
        case AIState::PATROL:
            _tickPatrol(ai, posX, posY, targetX, targetY, distToTarget, dt);
            break;
        case AIState::AGGRO:
            _tickAggro(ai, posX, posY, targetX, targetY, distToTarget, hpPct, dt);
            break;
        case AIState::COMBAT:
            _tickCombat(ai, posX, posY, targetX, targetY, distToTarget, hpPct, dt);
            break;
        case AIState::RETURN:
            _tickReturn(ai, posX, posY, distToTarget, distToSpawn, dt);
            break;
        default: break;
        }
    }

    // Инициализировать стандартный патруль вокруг спавна (4 точки)
    static void initDefaultPatrol(AIComponent& ai, float spawnX, float spawnY, float radius = 64.f) {
        ai.spawnX = spawnX;
        ai.spawnY = spawnY;
        ai.patrolPath = {
            {spawnX + radius,  spawnY,          1.5f},
            {spawnX + radius,  spawnY + radius,  0.f},
            {spawnX,           spawnY + radius,  1.5f},
            {spawnX,           spawnY,           0.f},
        };
        ai.patrolIdx = 0;
    }

    // Убить агента
    static void kill(AIComponent& ai) { ai.setState(AIState::DEAD); ai.velX = ai.velY = 0.f; }

    // Принудительный агрро (например, от AoE скилла)
    static void forceAggro(AIComponent& ai) {
        if (ai.state != AIState::DEAD)
            ai.setState(AIState::AGGRO);
    }

private:
    static float _dist(float ax, float ay, float bx, float by) {
        float dx = ax-bx, dy = ay-by;
        return std::sqrt(dx*dx+dy*dy);
    }

    static void _moveTowards(AIComponent& ai, float fromX, float fromY,
                              float toX, float toY, float speed) {
        float dx = toX - fromX, dy = toY - fromY;
        float d = std::sqrt(dx*dx+dy*dy);
        if (d > 2.f) {
            ai.velX = (dx/d) * speed;
            ai.velY = (dy/d) * speed;
        } else {
            ai.velX = ai.velY = 0.f;
        }
    }

    void _tickIdle(AIComponent& ai, float px, float py,
                   float tx, float ty, float dist, float dt)
    {
        ai.velX = ai.velY = 0.f;
        ai.idleTimer += dt;

        // Проверяем агрро
        if (dist < ai.params.aggroRange) {
            ai.setState(AIState::AGGRO);
            return;
        }

        // Переходим в патруль
        if (ai.params.canPatrol && ai.idleTimer >= ai.idleDuration && !ai.patrolPath.empty()) {
            ai.idleTimer = 0.f;
            ai.setState(AIState::PATROL);
        }
    }

    void _tickPatrol(AIComponent& ai, float px, float py,
                     float tx, float ty, float dist, float dt)
    {
        // Агрро
        if (dist < ai.params.aggroRange) {
            ai.setState(AIState::AGGRO);
            return;
        }

        if (ai.patrolPath.empty()) { ai.setState(AIState::IDLE); return; }

        auto& pt = ai.patrolPath[ai.patrolIdx];
        float dPt = _dist(px, py, pt.x, pt.y);

        if (dPt < 16.f) {
            ai.velX = ai.velY = 0.f;
            // Ждём на точке
            if (pt.waitTime > 0.f) {
                ai.patrolWait += dt;
                if (ai.patrolWait >= pt.waitTime) {
                    ai.patrolWait = 0.f;
                    ai.patrolIdx = (ai.patrolIdx + 1) % (int)ai.patrolPath.size();
                }
            } else {
                ai.patrolIdx = (ai.patrolIdx + 1) % (int)ai.patrolPath.size();
            }
        } else {
            _moveTowards(ai, px, py, pt.x, pt.y, ai.params.patrolSpeed);
        }
    }

    void _tickAggro(AIComponent& ai, float px, float py,
                    float tx, float ty, float dist, float hpPct, float dt)
    {
        // Flee если мало HP и разрешено
        if (ai.params.fleeAtLowHp && hpPct < ai.params.fleeHpThreshold) {
            // Бежим от игрока
            float dx = px - tx, dy = py - ty;
            float d = std::sqrt(dx*dx+dy*dy);
            if (d > 0.f) {
                ai.velX = (dx/d) * ai.params.aggroSpeed * 1.3f;
                ai.velY = (dy/d) * ai.params.aggroSpeed * 1.3f;
            }
            return;
        }

        // Деаггро
        if (dist > ai.params.deaggroRange) {
            ai.setState(AIState::RETURN);
            return;
        }

        // Переходим в combat
        if (dist < ai.params.combatRange) {
            ai.setState(AIState::COMBAT);
            return;
        }

        _moveTowards(ai, px, py, tx, ty, ai.params.aggroSpeed);
    }

    void _tickCombat(AIComponent& ai, float px, float py,
                     float tx, float ty, float dist, float hpPct, float dt)
    {
        // Цель ушла — преследуем
        if (dist > ai.params.combatRange * 1.5f) {
            ai.setState(AIState::AGGRO);
            return;
        }

        // Деаггро
        if (dist > ai.params.deaggroRange) {
            ai.setState(AIState::RETURN);
            return;
        }

        // Атакуем
        if (ai.attackTimer <= 0.f) {
            ai.shouldAttack = true;
            ai.attackTimer  = ai.params.attackCooldown;
        }

        // Держим дистанцию (слегка)
        if (dist < ai.params.combatRange * 0.5f) {
            // Отступаем
            float dx = px-tx, dy = py-ty;
            float d = std::sqrt(dx*dx+dy*dy);
            if (d > 0.f) {
                ai.velX = (dx/d)*30.f;
                ai.velY = (dy/d)*30.f;
            }
        } else {
            _moveTowards(ai, px, py, tx, ty, ai.params.combatSpeed);
        }
    }

    void _tickReturn(AIComponent& ai, float px, float py,
                     float distToTarget, float distToSpawn, float dt)
    {
        // Если игрок снова близко — агрро
        if (distToTarget < ai.params.aggroRange * 0.7f) {
            ai.setState(AIState::AGGRO);
            return;
        }

        // Дошли до спавна
        if (distToSpawn < ai.params.returnRange) {
            ai.velX = ai.velY = 0.f;
            ai.setState(AIState::IDLE);
            return;
        }

        _moveTowards(ai, px, py, ai.spawnX, ai.spawnY, ai.params.aggroSpeed);
    }
};

// ═══════════════════════════════════════════════════════════════
// КАК ПОДКЛЮЧИТЬ В main.cpp:
// ═══════════════════════════════════════════════════════════════
//
// 1. #include "ai_system.hpp"
//
// 2. В struct Enemy добавить:
//    AIComponent ai;
//
// 3. При спавне врага:
//    e.ai.params.aggroRange   = AGGRO_RANGE;
//    e.ai.params.deaggroRange = DEAGGRO_RANGE;
//    e.ai.params.attackCooldown = e.attackCooldown;
//    AISystem::initDefaultPatrol(e.ai, e.pos.x, e.pos.y);
//
// 4. В updateEnemies(), ВМЕСТО switch(e.state):
//    aiSystem.update(e.ai, e.pos.x, e.pos.y,
//                    player.pos.x, player.pos.y,
//                    e.hp, e.maxHp, dt);
//    e.pos.x += e.ai.velX * dt;
//    e.pos.y += e.ai.velY * dt;
//    if (e.ai.shouldAttack) {
//        player.hp -= e.damage;
//        audioManager->playSound("damage");
//    }
//    // Обновляем EnemyState из AIState для совместимости:
//    switch (e.ai.state) {
//        case AIState::IDLE:    e.state = EnemyState::IDLE;   break;
//        case AIState::PATROL:  e.state = EnemyState::PATROL; break;
//        case AIState::AGGRO:
//        case AIState::COMBAT:  e.state = EnemyState::AGGRO;  break;
//        case AIState::RETURN:  e.state = EnemyState::RETURN; break;
//        default: break;
//    }
