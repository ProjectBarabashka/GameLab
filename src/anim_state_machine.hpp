// anim_state_machine.hpp — Этап 2: State Machine для анимаций AETHORIA
// Подключается дополнительно к animation_system.hpp
// Решает: анимации зависят от состояния, нет дёрганий при переключении,
//         нет одновременного воспроизведения idle+walk и т.д.

#pragma once
#include "animation_system.hpp"
#include <string>
#include <map>
#include <functional>

// ════════════════════════════════════════════════════════════════
// СОСТОЯНИЯ АНИМАЦИИ (State Machine)
// ════════════════════════════════════════════════════════════════
enum class AnimState {
    IDLE,
    WALK,
    RUN,
    ATTACK,
    CAST,
    HURT,
    DEAD,
    CUSTOM   // для спец. анимаций (праздник, сидит и т.д.)
};

inline std::string animStateToClipName(AnimState s) {
    switch (s) {
        case AnimState::IDLE:   return "idle";
        case AnimState::WALK:   return "walk";
        case AnimState::RUN:    return "run";
        case AnimState::ATTACK: return "attack";
        case AnimState::CAST:   return "cast";
        case AnimState::HURT:   return "hurt";
        case AnimState::DEAD:   return "dead";
        default:                return "idle";
    }
}

// ════════════════════════════════════════════════════════════════
// AnimStateMachine — обёртка над AnimationPlayer с State Machine
// Использование:
//   AnimStateMachine sm(animPlayer, "wolf");
//   sm.setState(AnimState::WALK);  // переключится только если нужно
//   sm.update(dt);
//   sm.draw(window);
// ════════════════════════════════════════════════════════════════
class AnimStateMachine {
public:
    AnimationPlayer* player;
    std::string      entityKey;   // "player", "wolf", "goblin" …

    AnimState  currentState  = AnimState::IDLE;
    AnimState  previousState = AnimState::IDLE;
    bool       stateDirty    = true;  // true = нужно переключить анимацию

    // Колбэк: вызывается при завершении non-loop анимации
    std::function<void(AnimState)> onAnimFinished;

    // ── Переопределение клипов (если в JSON другое имя) ──────────
    // Например: setClipOverride(AnimState::WALK, "run") — если walk=run
    std::map<AnimState, std::string> clipOverrides;

    AnimStateMachine() : player(nullptr) {}
    AnimStateMachine(AnimationPlayer* p, const std::string& key)
        : player(p), entityKey(key) {}

    // ── Задать состояние ─────────────────────────────────────────
    // Смена состояния происходит только если оно действительно изменилось
    // (или если forceRestart=true)
    void setState(AnimState newState, bool forceRestart = false) {
        if (newState == currentState && !forceRestart) return;
        previousState = currentState;
        currentState  = newState;
        stateDirty    = true;
    }

    // Удобные методы для основных переходов
    void setIdle()   { setState(AnimState::IDLE);   }
    void setWalk()   { setState(AnimState::WALK);   }
    void setRun()    { setState(AnimState::RUN);    }
    void setAttack() { setState(AnimState::ATTACK, true); } // атака всегда с начала
    void setHurt()   { setState(AnimState::HURT,   true); }
    void setDead()   { setState(AnimState::DEAD,   true); }

    // ── Обновление ───────────────────────────────────────────────
    void update(float dt) {
        if (!player) return;

        // Применяем переключение анимации если состояние изменилось
        if (stateDirty) {
            stateDirty = false;
            std::string clipName = _resolveClip(currentState);

            // Фолбэк: если walk нет — используем idle (типично для мобов)
            if (!player->playAnimation(entityKey, clipName)) {
                if (clipName != "idle")
                    player->playAnimation(entityKey, "idle");
            }
        }

        player->update(dt);

        // Отслеживаем завершение non-loop анимаций
        if (player->isFinished()) {
            if (onAnimFinished) onAnimFinished(currentState);
            // Автовозврат: после attack/hurt/dead → idle
            if (currentState == AnimState::ATTACK ||
                currentState == AnimState::HURT) {
                setState(AnimState::IDLE);
            }
            // dead — остаётся в dead, не возвращается
        }
    }

    void draw(sf::RenderWindow& window) {
        if (player) player->draw(window);
    }

    // Проксируем часто используемые методы AnimationPlayer
    void setPosition(float x, float y) {
        if (player) player->setPosition(x, y);
    }
    void setFacing(bool left) {
        if (player) player->setFacing(left);
    }
    void updateFacingFromVelocity(float vx) {
        if (player) player->updateFacingFromVelocity(vx);
    }
    void setScale(float x, float y) {
        if (player) player->setScale(x, y);
    }

    AnimState getState() const { return currentState; }
    bool isDead()        const { return currentState == AnimState::DEAD; }
    bool isAttacking()   const { return currentState == AnimState::ATTACK; }

    // ── Переопределить клип для состояния ────────────────────────
    void setClipOverride(AnimState s, const std::string& clipName) {
        clipOverrides[s] = clipName;
    }

private:
    std::string _resolveClip(AnimState s) const {
        auto it = clipOverrides.find(s);
        if (it != clipOverrides.end()) return it->second;
        return animStateToClipName(s);
    }
};

// ════════════════════════════════════════════════════════════════
// Вспомогательная функция: определить AnimState врага из EnemyState
// Подключается после определения enum class EnemyState в main.cpp
// ════════════════════════════════════════════════════════════════

// Используй так в updateEnemies():
//
//   AnimState animS = enemyStateToAnimState(e.state, e.hp <= 0);
//   e.stateMachine.setState(animS);
//   e.stateMachine.updateFacingFromVelocity(moveVelocity.x);
//   e.stateMachine.update(dt);
//   e.stateMachine.setPosition(e.pos.x, e.pos.y);
//
// Нужно добавить поле в Enemy:
//   AnimStateMachine stateMachine;
// И инициализировать при спавне:
//   e.stateMachine = AnimStateMachine(e.animPlayer, getEntityName(e.type));

// (Inline функция — не может ссылаться на EnemyState до его объявления,
//  поэтому определяется как шаблон или в .cpp. Используй напрямую.)

// ════════════════════════════════════════════════════════════════
// КАК ПОДКЛЮЧИТЬ В main.cpp:
// ════════════════════════════════════════════════════════════════
//
// 1. #include "anim_state_machine.hpp"  (в начале main.cpp)
//
// 2. В struct Enemy добавить:
//    AnimStateMachine stateMachine;
//
// 3. В struct Player добавить:
//    AnimStateMachine stateMachine;
//
// 4. В initPlayer(), после создания animPlayer:
//    player.stateMachine = AnimStateMachine(player.animPlayer, "player");
//    player.stateMachine.setClipOverride(AnimState::RUN, "run");
//
// 5. В spawnEnemies(), после создания e.animPlayer:
//    e.stateMachine = AnimStateMachine(e.animPlayer, getEntityName(e.type));
//    // Для врагов у которых нет "walk" — фолбэк на "idle"
//
// 6. В updatePlayer(), заменить блок анимации:
//    if (player.moving)
//        player.stateMachine.setState(AnimState::RUN);
//    else
//        player.stateMachine.setState(AnimState::IDLE);
//    player.stateMachine.updateFacingFromVelocity(rawDirX);
//    player.stateMachine.update(dt);
//    player.stateMachine.setPosition(player.pos.x, player.pos.y);
//
// 7. В updateEnemies(), заменить блок анимации в конце цикла:
//    bool isMoving = (e.state == EnemyState::PATROL ||
//                     e.state == EnemyState::AGGRO  ||
//                     e.state == EnemyState::RETURN);
//    e.stateMachine.setState(isMoving ? AnimState::WALK : AnimState::IDLE);
//    e.stateMachine.update(dt);
//    e.stateMachine.setPosition(e.pos.x, e.pos.y);
//    // facing уже обновлён выше через updateFacingFromVelocity()
//
// ════════════════════════════════════════════════════════════════
