// aethoria_engine.hpp - Основной игровой движок

#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <random>
#include "audio_manager.hpp"

// ============================================================
// CONSTANTS
// ============================================================
const int TILE_SIZE = 32;
const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 900;
const float BASE_SPEED = 150.0f;

// ============================================================
// ENUMERATIONS
// ============================================================
enum class GameScene {
    LOGIN,
    CHARACTER_SELECT,
    WORLD,
    INVENTORY,
    STATS,
    QUESTS,
    PAUSE_MENU,
    DEAD
};

enum class DamageType {
    PHYSICAL,
    FIRE,
    ICE,
    LIGHTNING,
    DARK,
    LIGHT
};

enum class ItemRarity {
    COMMON,
    UNCOMMON,
    RARE,
    EPIC,
    LEGENDARY
};

// ============================================================
// STRUCTURES
// ============================================================
struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const { return Vec2(x / s, y / s); }
    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const {
        float len = length();
        return len > 0 ? Vec2(x / len, y / len) : Vec2(0, 0);
    }
    float dot(const Vec2& v) const { return x * v.x + y * v.y; }
};

struct ParticleSystem {
    struct Particle {
        Vec2 pos, vel;
        sf::Color color;
        float lifetime, maxLifetime;
        float size;
        float rotation, rotationSpeed;
    };
    
    std::vector<Particle> particles;
    
    void emit(Vec2 position, int count, Vec2 velocity, 
              sf::Color color, float lifetime, float speed = 100.0f) {
        std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> angleDist(0, 2 * 3.14159f);
        std::uniform_real_distribution<float> speedDist(0.5f, 1.5f);
        
        for (int i = 0; i < count; i++) {
            Particle p;
            p.pos = position;
            
            float angle = angleDist(gen);
            float s = speed * speedDist(gen);
            p.vel = Vec2(std::cos(angle), std::sin(angle)) * s;
            
            p.color = color;
            p.lifetime = lifetime;
            p.maxLifetime = lifetime;
            p.size = 3.0f;
            p.rotation = angleDist(gen);
            p.rotationSpeed = angleDist(gen) * 360.0f;
            
            particles.push_back(p);
        }
    }
    
    void update(float dt) {
        for (auto& p : particles) {
            p.pos = p.pos + p.vel * dt;
            p.lifetime -= dt;
            p.rotation += p.rotationSpeed * dt;
            
            // Gravity effect
            p.vel.y += 50.0f * dt;
        }
        
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return p.lifetime <= 0; }),
            particles.end()
        );
    }
    
    void draw(sf::RenderWindow& window) {
        for (const auto& p : particles) {
            float alpha = (p.lifetime / p.maxLifetime) * 255;
            sf::Color c = p.color;
            c.a = static_cast<uint8_t>(alpha);
            
            sf::CircleShape circle(p.size);
            circle.setPosition(p.pos.x - p.size, p.pos.y - p.size);
            circle.setFillColor(c);
            circle.setRotation(p.rotation);
            window.draw(circle);
        }
    }
};

struct LightSource {
    Vec2 position;
    float radius;
    sf::Color color;
    float intensity;
    bool isPlayer;
    
    LightSource(Vec2 pos, float rad, sf::Color col, float inten, bool isP = false)
        : position(pos), radius(rad), color(col), intensity(inten), isPlayer(isP) {}
};

struct Effect {
    std::string name;
    float duration;
    std::string type; // "poison", "stun", "burn", etc
    int potency;
};

struct Inventory {
    std::vector<std::string> items;
    int gold;
    int capacity;
    
    Inventory() : gold(0), capacity(20) {}
    
    bool addItem(const std::string& item) {
        if (items.size() >= capacity) return false;
        items.push_back(item);
        return true;
    }
};

// ============================================================
// GAME ENGINE CLASS
// ============================================================
class AethoriaEngine {
private:
    // Window
    sf::RenderWindow window;
    GameScene currentScene;
    sf::Clock gameClock;
    float deltaTime;
    float gameTime;
    
    // View and camera
    sf::View gameView;
    float zoomLevel;
    
    // Audio
    std::unique_ptr<AudioManager> audioManager;
    
    // Rendering
    std::vector<LightSource> lightSources;
    ParticleSystem particleSystem;
    
    // Game state (simplified)
    struct Player {
        Vec2 position;
        Vec2 velocity;
        float hp, maxHp;
        float mp, maxMp;
        float speed;
        int level;
        int experience;
        std::string name;
        std::string className;
        
        // Stats
        int strength, agility, intelligence, vitality;
        int attackPower, defense;
        
        // Combat
        float attackCooldown, attackTimer;
        std::vector<std::string> skills;
        
        // Inventory
        Inventory inventory;
        
        Player() : position(0, 0), velocity(0, 0),
                   hp(100), maxHp(100), mp(50), maxMp(50),
                   speed(BASE_SPEED), level(1), experience(0),
                   strength(10), agility(8), intelligence(6), vitality(9),
                   attackPower(15), defense(5), attackCooldown(0.5f), attackTimer(0) {}
    } player;
    
    struct Enemy {
        Vec2 position;
        Vec2 velocity;
        float hp, maxHp;
        int level;
        int damage;
        float speed;
        float attackCooldown, attackTimer;
        std::string name;
        bool isBoss;
        
        Enemy() : position(0, 0), velocity(0, 0),
                  hp(30), maxHp(30), level(1), damage(5),
                  speed(80.0f), attackCooldown(1.5f), attackTimer(0),
                  isBoss(false) {}
    };
    
    std::vector<Enemy> enemies;
    std::map<int, AnimationPlayer> enemyAnimations; 
    Enemy* selectedTarget;
    
    // UI elements
    bool showDebugInfo;
    bool showInventory;
    bool showStats;
    
public:
    AethoriaEngine() 
        : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "AETHORIA: Eternal Realms"),
          currentScene(GameScene::LOGIN),
          deltaTime(0.0f),
          gameTime(0.0f),
          zoomLevel(1.0f),
          selectedTarget(nullptr),
          showDebugInfo(false),
          showInventory(false),
          showStats(false) {
        
        window.setFramerateLimit(60);
        window.setVerticalSyncEnabled(true);
        
        // Initialize audio
        audioManager = std::make_unique<AudioManager>();
        
        // Initialize game view
        gameView.setSize(WINDOW_WIDTH, WINDOW_HEIGHT);
        gameView.setCenter(player.position.x, player.position.y);
        window.setView(gameView);
        
        // Load game content
        initializeGame();
    }
    
    void initializeGame() {
        // Initialize player
        player.name = "Hero";
        player.className = "Warrior";
        player.position = Vec2(400, 300);
        
        // Add some skills
        player.skills = {"Slash", "Fireball", "Heal", "Dash"};
        
        // Add light source at player
        lightSources.emplace_back(player.position, 200.0f, sf::Color(200, 180, 100), 0.8f, true);
        
        // Spawn some enemies
        spawnEnemies();
        
        // Load audio files (these would need to exist in assets folder)
        audioManager->loadSound("slash", "assets/sounds/slash.wav");
        audioManager->loadSound("damage", "assets/sounds/damage.wav");
        audioManager->loadSound("levelup", "assets/sounds/levelup.wav");
        audioManager->loadBackgroundMusic("assets/music/ambient.ogg");
    }
    
    void spawnEnemies() {
        std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> xDist(100, 800);
        std::uniform_real_distribution<float> yDist(100, 600);
        
        for (int i = 0; i < 8; i++) {
            Enemy enemy;
            enemy.position = Vec2(xDist(gen), yDist(gen));
            enemy.maxHp = 40 + i * 10;
            enemy.hp = enemy.maxHp;
            enemy.level = 1 + i / 2;
            enemy.damage = 8 + i * 2;
            enemy.name = "Forest Goblin Lv." + std::to_string(enemy.level);
            enemies.push_back(enemy);
        }
    }
    
    bool isRunning() {
        return window.isOpen();
    }
    
    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            if (event.type == sf::Event::KeyPressed) {
                handleKeyPress(event.key.code);
            }
            
            if (event.type == sf::Event::MouseButtonPressed) {
                handleMouseClick(event.mouseButton.x, event.mouseButton.y);
            }
        }
    }
    
    void handleKeyPress(sf::Keyboard::Key key) {
        switch (key) {
            case sf::Keyboard::Escape:
                currentScene = GameScene::PAUSE_MENU;
                break;
            case sf::Keyboard::C:
                showStats = !showStats;
                break;
            case sf::Keyboard::B:
                showInventory = !showInventory;
                break;
            case sf::Keyboard::F1:
                showDebugInfo = !showDebugInfo;
                break;
            case sf::Keyboard::Num1:
                useSkill(0);
                break;
            case sf::Keyboard::Num2:
                useSkill(1);
                break;
            case sf::Keyboard::Num3:
                useSkill(2);
                break;
            default:
                break;
        }
    }
    
    void handleMouseClick(int x, int y) {
        // Convert screen coordinates to world coordinates
        sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(x, y));
        
        // Check if clicked on any enemy
        for (auto& enemy : enemies) {
            if ((enemy.position - Vec2(worldPos.x, worldPos.y)).length() < 20) {
                selectedTarget = &enemy;
                return;
            }
        }
    }
    
    void useSkill(int skillIndex) {
        if (skillIndex >= player.skills.size() || !selectedTarget) return;
        
        float damage = player.attackPower + (player.strength * 0.5f);
        selectedTarget->hp -= damage;
        
        // Play sound
        audioManager->playSound("slash", 0.8f);
        
        // Spawn particles
        particleSystem.emit(selectedTarget->position, 15,
                          Vec2(0, -50), sf::Color::Red, 0.3f, 150.0f);
        
        if (selectedTarget->hp <= 0) {
            selectedTarget = nullptr;
        }
    }
    
    void update() {
        deltaTime = gameClock.restart().asSeconds();
        gameTime += deltaTime;
        
        // Clamp deltaTime to prevent large jumps
        if (deltaTime > 0.05f) deltaTime = 0.05f;
        
        // Update player movement
        updatePlayerMovement();
        
        // Update enemies
        updateEnemies();
        
        // Update systems
        particleSystem.update(deltaTime);
        audioManager->update(deltaTime);
        
        // Update lights
        if (!lightSources.empty()) {
            lightSources[0].position = player.position;
        }
    }
    
    void updatePlayerMovement() {
        Vec2 moveDir(0, 0);
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) moveDir.y -= 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) moveDir.y += 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) moveDir.x -= 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) moveDir.x += 1;
        
        if (moveDir.length() > 0) {
            moveDir = moveDir.normalized();
            player.velocity = moveDir * player.speed;
        } else {
            player.velocity = Vec2(0, 0);
        }
        
        player.position = player.position + player.velocity * deltaTime;
        
        // Update camera
        gameView.setCenter(player.position.x, player.position.y);
        window.setView(gameView);
    }
    
   void updateEnemies() {
    for (size_t i = 0; i < enemies.size(); i++) {
        auto& enemy = enemies[i];
        
        // 1. Движение к игроку
        Vec2 dirToPlayer = (player.position - enemy.position);
        float distToPlayer = dirToPlayer.length();
        
        if (distToPlayer > 5.0f) { // Двигаемся, только если игрок не вплотную
            enemy.velocity = dirToPlayer.normalized() * enemy.speed;
            enemy.position = enemy.position + enemy.velocity * deltaTime;
        } else {
            enemy.velocity = Vec2(0, 0);
        }
        
        // 2. ПОВОРОТ (Зеркалирование спрайта)
        // Если игрок слева от моба (разница по X отрицательная)
        if (player.position.x < enemy.position.x - 2.0f) {
            // Разворачиваем влево
            enemyAnimations[i].getSprite().setScale(-1.0f, 1.0f);
        } 
        else if (player.position.x > enemy.position.x + 2.0f) {
            // Смотрим вправо
            enemyAnimations[i].getSprite().setScale(1.0f, 1.0f);
        }

        // 3. ОБНОВЛЕНИЕ АНИМАЦИИ И ПОЗИЦИИ
        enemyAnimations[i].update(deltaTime);
        enemyAnimations[i].setPosition(enemy.position.x, enemy.position.y);

        // 4. ЛОГИКА АТАКИ
        if (distToPlayer < 30.0f && enemy.attackTimer <= 0) {
            player.hp -= enemy.damage;
            enemy.attackTimer = enemy.attackCooldown;
            audioManager->playSound("damage", 0.7f);
        } else if (enemy.attackTimer > 0) {
            enemy.attackTimer -= deltaTime;
        }
    }
}

      void render() {
        window.clear(sf::Color(20, 15, 35));
        
        // Draw world
        drawWorld();
        drawEnemies();
        drawPlayer();
        particleSystem.draw(window);
        drawLighting();
        
        // Draw UI
        window.setView(window.getDefaultView());
        drawUI();
        
        if (showDebugInfo) {
            drawDebugInfo();
        }
        
        window.display();
    }
    
   void drawWorld() {
        // Draw grid
        sf::Color gridColor(40, 35, 60);
        
        for (int x = -WINDOW_WIDTH; x < WINDOW_WIDTH * 2; x += TILE_SIZE) {
            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(x, -WINDOW_HEIGHT), gridColor),
                sf::Vertex(sf::Vector2f(x, WINDOW_HEIGHT * 2), gridColor)
            };
            window.draw(line, 2, sf::Lines);
        }
        
        for (int y = -WINDOW_HEIGHT; y < WINDOW_HEIGHT * 2; y += TILE_SIZE) {
            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(-WINDOW_WIDTH, y), gridColor),
                sf::Vertex(sf::Vector2f(WINDOW_WIDTH * 2, y), gridColor)
            };
            window.draw(line, 2, sf::Lines);
        }
    }
    
    void drawPlayer() {
        sf::CircleShape playerShape(12);
        playerShape.setPosition(player.position.x - 12, player.position.y - 12);
        playerShape.setFillColor(sf::Color(100, 200, 255));
        playerShape.setOutlineColor(sf::Color(200, 220, 255));
        playerShape.setOutlineThickness(2);
        window.draw(playerShape);
        
        // Draw HP bar
        drawHealthBar(player.position.x - 15, player.position.y - 30,
                     player.hp / player.maxHp, sf::Color(200, 50, 50));
    }
    
    void drawEnemies() {
    for (size_t i = 0; i < enemies.size(); i++) {
        auto& enemy = enemies[i];
        
        // 1. Рисуем анимацию вместо кружочка
        // enemyAnimations[i] — это тот самый AnimationPlayer, который мы добавили в map
        enemyAnimations[i].draw(window);

        // 2. Рисуем полоску ХП (оставляем как было)
        drawHealthBar(enemy.position.x - 15, enemy.position.y - 25,
                     enemy.hp / enemy.maxHp, sf::Color(200, 50, 50));
                     
        // 3. Если моб выбран (таргет), можно оставить кружочек-подсветку под ногами
        if (&enemy == selectedTarget) {
            sf::CircleShape targetRing(15);
            targetRing.setOrigin(15, 15);
            targetRing.setPosition(enemy.position.x, enemy.position.y);
            targetRing.setFillColor(sf::Color::Transparent);
            targetRing.setOutlineColor(sf::Color::Red);
            targetRing.setOutlineThickness(2);
            window.draw(targetRing);
        }
    }
}
    
    void drawHealthBar(float x, float y, float percent, sf::Color color) {
        sf::RectangleShape bg(sf::Vector2f(30, 4));
        bg.setPosition(x, y);
        bg.setFillColor(sf::Color(50, 50, 50));
        window.draw(bg);
        
        sf::RectangleShape bar(sf::Vector2f(30 * std::max(0.0f, percent), 4));
        bar.setPosition(x, y);
        bar.setFillColor(color);
        window.draw(bar);
    }
    
    void drawLighting() {
        // Simple lighting effect with circles
        for (const auto& light : lightSources) {
            sf::CircleShape lightCircle(light.radius);
            lightCircle.setPosition(light.position.x - light.radius,
                                   light.position.y - light.radius);
            lightCircle.setFillColor(sf::Color::Transparent);
            
            // Draw light glow
            for (float r = light.radius; r > 0; r -= 10) {
                sf::CircleShape glow(r);
                glow.setPosition(light.position.x - r, light.position.y - r);
                sf::Color glowColor = light.color;
                glowColor.a = static_cast<uint8_t>(20 * light.intensity);
                glow.setFillColor(glowColor);
                window.draw(glow);
            }
        }
    }
    
    void drawUI() {
        // Draw HP/MP bars
        drawBarUI("HP", 10, 10, player.hp, player.maxHp, sf::Color(200, 50, 50));
        drawBarUI("MP", 10, 35, player.mp, player.maxMp, sf::Color(50, 100, 200));
        
        // Draw skill buttons
        drawSkillBar();
        
        // Draw crosshair
        if (selectedTarget) {
            drawCrosshair(WINDOW_WIDTH - 60, WINDOW_HEIGHT - 60,
                         sf::Color(200, 50, 50));
        }
    }
    
    void drawBarUI(const std::string& label, float x, float y, 
                   float current, float maximum, sf::Color color) {
        // Background
        sf::RectangleShape bg(sf::Vector2f(200, 20));
        bg.setPosition(x, y);
        bg.setFillColor(sf::Color(50, 50, 50, 200));
        window.draw(bg);
        
        // Bar
        float percent = std::max(0.0f, current / maximum);
        sf::RectangleShape bar(sf::Vector2f(200 * percent, 20));
        bar.setPosition(x, y);
        bar.setFillColor(color);
        window.draw(bar);
    }
    
    void drawSkillBar() {
        const float skillSize = 40;
        const float startX = WINDOW_WIDTH - 250;
        const float startY = WINDOW_HEIGHT - 60;
        
        for (size_t i = 0; i < std::min(player.skills.size(), size_t(4)); i++) {
            sf::RectangleShape skillBtn(sf::Vector2f(skillSize, skillSize));
            skillBtn.setPosition(startX + i * (skillSize + 5), startY);
            skillBtn.setFillColor(sf::Color(60, 40, 100, 200));
            skillBtn.setOutlineColor(sf::Color(100, 80, 150));
            skillBtn.setOutlineThickness(1);
            window.draw(skillBtn);
        }
    }
    
    void drawCrosshair(float x, float y, sf::Color color) {
        const float size = 20;
        
        // Horizontal line
        sf::Vertex hline[] = {
            sf::Vertex(sf::Vector2f(x - size, y), color),
            sf::Vertex(sf::Vector2f(x + size, y), color)
        };
        window.draw(hline, 2, sf::Lines);
        
        // Vertical line
        sf::Vertex vline[] = {
            sf::Vertex(sf::Vector2f(x, y - size), color),
            sf::Vertex(sf::Vector2f(x, y + size), color)
        };
        window.draw(vline, 2, sf::Lines);
    }
    
    void drawDebugInfo() {
        // Would display FPS, player position, enemy count, etc.
    }
};
