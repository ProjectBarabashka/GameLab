// main.cpp - AETHORIA: Eternal Realms (объединённая версия: сцены + интерактивность)
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <iostream>
#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>
#include <locale>
#include <clocale>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#endif

// ════════════════════════════════════════════════════════════════
// СИСТЕМЫ
// ════════════════════════════════════════════════════════════════
#include "animation_system.hpp"
#include "anim_state_machine.hpp"
#include "audio_manager.hpp"        // FIX: добавлен include для AudioManager
#include "entity_system.hpp"
#include "scene_system.hpp"
#include "trigger_system.hpp"
#include "prefab_system.hpp"   // Этап 1: каталог префабов
#include "quest_system.hpp"    // Этап 1: система квестов
// ── Редмап: новые системы ───────────────────────────────────
#include "event_system.hpp"    // Приоритет 1: EventBus
#include "skill_system.hpp"    // Приоритет 2: Скиллы
#include "item_system.hpp"     // Приоритет 2: Предметы + инвентарь
#include "ai_system.hpp"       // Приоритет 3: AI FSM

// ============================================================
// CONSTANTS
// ============================================================
const int TILE         = 32;
const int MAP_W        = 512;
const int MAP_H        = 512;
const int WINDOW_W     = 1400;
const int WINDOW_H     = 900;
const int CITY_CX      = 60;
const int CITY_CY      = 60;
const int CITY_RADIUS  = 18;
const int SAFE_RADIUS  = 22;
const float AGGRO_RANGE   = 200.0f;
const float DEAGGRO_RANGE = 450.0f;

// ============================================================
// ENUMS
// ============================================================
enum class TileType  { GRASS, DIRT, STONE_FLOOR, ROAD, WALL, WATER, TREE,
                       BUILDING_FLOOR, ROOF_RED, ROOF_BLUE, ROOF_GREEN, FOUNTAIN };
enum class EnemyState{ IDLE, PATROL, AGGRO, COMBAT, RETURN };
enum class ParticleT { BLOOD, MAGIC, HEAL, CRITICAL };
enum class GameState { LOGIN, CHARACTER_SELECT, PLAYING, DEAD };
enum class EnemyType { GOBLIN, TROLL, BANDIT, WOLF, SKELETON };

// ============================================================
// HELPERS
// ============================================================
struct V2 {
    float x=0,y=0;
    V2(float x=0,float y=0):x(x),y(y){}
    V2 operator+(const V2& o) const{return{x+o.x,y+o.y};}
    V2 operator-(const V2& o) const{return{x-o.x,y-o.y};}
    V2 operator*(float s)     const{return{x*s,y*s};}
    float len()  const{return std::sqrt(x*x+y*y);}
    V2    norm() const{float l=len();return l>0?V2(x/l,y/l):V2();}
};
inline float dist(V2 a,V2 b){return(a-b).len();}
inline sf::Color lerp(sf::Color a,sf::Color b,float t){
    return sf::Color(
        (uint8_t)(a.r+(b.r-a.r)*t),
        (uint8_t)(a.g+(b.g-a.g)*t),
        (uint8_t)(a.b+(b.b-a.b)*t),
        (uint8_t)(a.a+(b.a-a.a)*t)
    );
}

// ============================================================
// STRUCTURES
// ============================================================
struct Particle {
    V2 pos,vel; float life,maxLife,size; sf::Color color;
};
struct FloatingText {
    V2 pos; std::string text; float life; sf::Color color; bool critical=false;
};
struct Skill {
    std::string name,description; int manaCost,damage; float cooldown,currentCooldown;
};
struct Item {
    std::string name,type; int rarity,damage,defense,hpBoost;
};

// ════════════════════════════════════════════════════════════════
// СТАТУС-ЭФФЕКТЫ (яд, стан, замедление, поджог)
// ════════════════════════════════════════════════════════════════
enum class StatusType { POISON, STUN, SLOW, BURN };

struct ActiveStatus {
    StatusType type;
    float      duration;   // оставшееся время
    float      potency;    // урон/с для DoT, % замедления для slow
    float      tickTimer;  // таймер тика (DoT)
};

// ════════════════════════════════════════════════════════════════
// ENEMY
// ════════════════════════════════════════════════════════════════
struct Enemy {
    V2          pos, spawnPos;
    float       hp, maxHp;
    int         level, damage, gold;
    float       speed;
    float       attackCD, currentAttackCD;
    std::string name;
    bool        boss = false;
    EnemyState  state = EnemyState::IDLE;
    EnemyType   type  = EnemyType::GOBLIN;
    float       aiTimer = 0;
    V2          patrolTarget;
    bool        aggroedByHit = false;
    float       animTime = 0;
    AnimationPlayer* animPlayer = nullptr;
    AnimStateMachine stateMachine;
    bool        deathProcessed = false;
    AIComponent ai;    // Приоритет 3: AI FSM компонент

    // Статус-эффекты
    std::vector<ActiveStatus> statuses;
    float stunTimer  = 0.f;   // суммарное время стана
    float slowFactor = 1.f;   // множитель скорости (1=нет слоу, 0.5=50% замедление)

    // Применить статус-эффект
    void applyStatus(StatusType t, float duration, float potency) {
        for (auto& s : statuses) {
            if (s.type == t) { s.duration = std::max(s.duration, duration); return; }
        }
        statuses.push_back({t, duration, potency, 0.5f});
    }

    bool isStunned() const { return stunTimer > 0.f; }
};

// ════════════════════════════════════════════════════════════════
// PLAYER
// ════════════════════════════════════════════════════════════════
struct Player {
    V2    pos;
    float hp,maxHp,mp,maxMp;
    int   level,xp,xpNext;
    int   str,agi,intel,vit;
    int   baseAtk,baseDef;
    int   gold,kills;
    std::string name,className;
    float speed,attackCD,currentAttackCD,comboTimer;
    int   combo;
    std::vector<Skill> skills;
    std::vector<Item>  inventory;   // legacy инвентарь (для HUD и старых функций)
    Inventory          bag;         // FIX: новый Inventory для ItemSystem::giveToInventory
    std::vector<SkillInstance> skillInstances;  // SkillSystem runtime instances
    int   hpPotions=5, mpPotions=3;
    float animTime=0;
    bool  moving=false;
    int   facing=0;

    // ── Экипировка (slot → itemId, "" = пусто) ────────────────
    std::map<std::string, std::string> equipped;
    // Кэш суммарных бонусов от экипировки (пересчитывается при надевании/снятии)
    std::map<std::string, float> equipBonuses;
    AnimationPlayer* animPlayer = nullptr;
    AnimStateMachine stateMachine;
};

// ============================================================
// DRAW HELPERS (unchanged from main2)
// ============================================================
void drawRoundedRect(sf::RenderWindow& w, float x, float y, float ww, float hh,
                     sf::Color fill, sf::Color outline=sf::Color::Transparent, float ot=0) {
    sf::RectangleShape r({ww-4,hh}); r.setPosition(x+2,y);
    r.setFillColor(fill); w.draw(r);
    sf::RectangleShape r2({ww,hh-4}); r2.setPosition(x,y+2);
    r2.setFillColor(fill); w.draw(r2);
    if(outline!=sf::Color::Transparent){
        r.setFillColor(sf::Color::Transparent);
        r.setOutlineColor(outline); r.setOutlineThickness(ot); w.draw(r);
        r2.setFillColor(sf::Color::Transparent);
        r2.setOutlineColor(outline); r2.setOutlineThickness(ot); w.draw(r2);
    }
}

void drawChibiTree(sf::RenderWindow& w, float x, float y, int variant) {
    sf::RectangleShape trunk({6,12}); trunk.setOrigin(3,0);
    trunk.setPosition(x+TILE/2, y+TILE-12);
    trunk.setFillColor(variant%2==0 ? sf::Color(110,70,40) : sf::Color(90,55,30));
    w.draw(trunk);
    sf::CircleShape foliage(10); foliage.setOrigin(10,10);
    foliage.setPosition(x+TILE/2, y+TILE/2);
    sf::Color leafCol = variant%3==0 ? sf::Color(30,120,30) : 
                        variant%3==1 ? sf::Color(50,140,50) : sf::Color(35,110,35);
    foliage.setFillColor(leafCol);
    w.draw(foliage);
}

void drawFountain(sf::RenderWindow& w, float x, float y, float time) {
    sf::CircleShape base(14,30); base.setOrigin(14,14);
    base.setPosition(x+TILE/2, y+TILE/2);
    base.setFillColor(sf::Color(180,180,160));
    w.draw(base);
    sf::CircleShape water(8,20); water.setOrigin(8,8);
    water.setPosition(x+TILE/2, y+TILE/2-2);
    water.setFillColor(sf::Color(int(40+std::sin(time)*30),int(110+std::sin(time)*50),220));
    w.draw(water);
    for(int i=0;i<4;i++){
        float a=(time+i*3.14159f/2)*0.7f;
        float dx=std::cos(a)*6, dy=-std::abs(std::sin(a*2))*8;
        sf::CircleShape drop(2,10); drop.setOrigin(2,2);
        drop.setPosition(x+TILE/2+dx, y+TILE/2+dy);
        drop.setFillColor(sf::Color(150,200,255,100));
        w.draw(drop);
    }
}

void drawBuilding(sf::RenderWindow& w, float x, float y, int bw, int bh,
                  sf::Color roofCol, int variant) {
    sf::RectangleShape wall({(float)bw*TILE,(float)bh*TILE});
    wall.setPosition(x,y); wall.setFillColor(sf::Color(140,120,100));
    wall.setOutlineColor(sf::Color(100,80,60)); wall.setOutlineThickness(2);
    w.draw(wall);
    sf::RectangleShape roof({(float)bw*TILE,(float)bh*TILE*0.4f});
    roof.setPosition(x,y-bh*TILE*0.4f);
    roof.setFillColor(roofCol);
    w.draw(roof);
    sf::RectangleShape door({TILE*0.5f,TILE*0.8f});
    door.setPosition(x+bw*TILE/2-door.getSize().x/2, y+bh*TILE-door.getSize().y);
    door.setFillColor(sf::Color(80,50,20));
    w.draw(door);
}

void drawWorldBar(sf::RenderWindow& w, float x, float y, float ww, float hh, float ratio, sf::Color col) {
    sf::RectangleShape bg({ww,hh}); bg.setPosition(x,y); bg.setFillColor(sf::Color(30,30,30));
    w.draw(bg);
    sf::RectangleShape bar({ww*ratio,hh}); bar.setPosition(x,y); bar.setFillColor(col);
    w.draw(bar);
}

void drawChibiEnemy(sf::RenderWindow& w, const Enemy& e, bool targeted, float gameTime) {
    float bob = std::sin(gameTime*4)*2;
    sf::CircleShape body(10); body.setOrigin(10,10);
    body.setPosition(e.pos.x, e.pos.y+bob);
    sf::Color bodyCol;
    if(e.type==EnemyType::GOBLIN)       bodyCol=sf::Color(60,180,60);
    else if(e.type==EnemyType::TROLL)   bodyCol=sf::Color(100,160,100);
    else if(e.type==EnemyType::BANDIT)  bodyCol=sf::Color(140,80,50);
    else if(e.type==EnemyType::WOLF)    bodyCol=sf::Color(150,100,60);
    else                                bodyCol=sf::Color(120,120,120);
    body.setFillColor(bodyCol);
    w.draw(body);
    sf::CircleShape eye(2.5f); eye.setOrigin(2.5f,2.5f);
    eye.setFillColor(sf::Color::White);
    eye.setPosition(e.pos.x-4, e.pos.y-3+bob); w.draw(eye);
    eye.setPosition(e.pos.x+4, e.pos.y-3+bob); w.draw(eye);
}

// ============================================================
// GAME ENGINE
// ============================================================
struct Star { float x,y,speed,brightness; };

// ════════════════════════════════════════════════════════════════
// LOOT DROP — Этап 3
// ════════════════════════════════════════════════════════════════
struct LootDrop {
    V2          pos;
    std::string name;
    int         rarity;   // 0=common 1=uncommon 2=rare 3=epic
    float       life;     // время до исчезновения (30 сек)
    bool        isGold;
    int         goldAmt;
    bool        pickedUp = false;
};

static sf::Color rarityColor(int r) {
    switch(r) {
        case 1: return sf::Color(30, 180, 80);    // uncommon зелёный
        case 2: return sf::Color(60, 120, 220);   // rare синий
        case 3: return sf::Color(150, 50, 220);   // epic фиолетовый
        default: return sf::Color(160, 155, 150); // common серый
    }
}

class GameEngine {
public:
    sf::RenderWindow window;
    sf::Clock clk;

    GameState gameState = GameState::LOGIN;
    int selectedSlot = 0, selectedClass = 0, selectedSkin = 0;

    sf::String loginText, passText, charNameText;
    bool activeFocus = true, activeField = 0, nameFocused = false;
    int activeSlot = -1;
    std::vector<Star> stars;
    float starTimer = 0;

    struct CharSlot {
        std::string name;
        int classIdx, skinIdx, level;
        bool available;
    };
    CharSlot charSlots[3];

    struct EnemyDef {
        int tileX=0, tileY=0;
        std::string typeName="GOBLIN";
        int level=1; bool boss=false; int hp=0; int dmg=0;
    };
    std::vector<EnemyDef> mapEnemyDefs;

    // ════════════════════════════════════════════════════════════════
    // worldMap теперь типа SceneTile
    // ════════════════════════════════════════════════════════════════
    SceneTile worldMap[MAP_H][MAP_W];

    Player player;
    std::vector<Enemy> enemies;
    Enemy* targetEnemy = nullptr;

    std::vector<LootDrop> lootDrops;   // Этап 3: лут на земле

    std::vector<Particle> particles;
    std::vector<FloatingText> floatingTexts;
    sf::View camera;
    float gameTime=0;

    sf::Font font;
    bool fontLoaded=false;

    AnimationManager* animManager = nullptr;
    AudioManager*     audioManager = nullptr;   // FIX: был используется но не объявлен
    EntitySystem entitySystem;
    uint32_t playerEntityId = 0;

    // ── Текстуры кастомных тайлов (из custom_tiles.json) ──────────
    std::map<std::string, sf::Texture> customTileTextures;
    std::map<std::string, sf::Sprite>  customTileSprite;
    bool customTilesTexturesLoaded = false;

    // ── Этап 1: Prefab + Quest системы ───────────────────────────
    PrefabCatalog prefabCatalog;
    QuestSystem   questSystem;
    // Последние строки квест-диалога (для HUD-уведомлений)
    std::string   questNotification;
    float         questNotifyTimer = 0.f;

    // ── Редмап: новые системы ─────────────────────────────────
    EventBus    eventBus;      // Приоритет 1: шина событий
    SkillSystem skillSystem;   // Приоритет 2: скиллы
    ItemSystem  itemSystem;    // Приоритет 2: предметы
    AISystem    aiSystem;      // Приоритет 3: AI FSM

    // Система сцен
    SceneManager sceneManager;
    float sceneTransitionTimer = 0.f;
    bool  sceneTransitioning   = false;
    std::string pendingSceneId;
    float pendingSpawnX = -1.f;  // spawn offset при переходе через портал (-1 = использовать spawn сцены)
    float pendingSpawnY = -1.f;

    // Система триггеров
    TriggerSystem triggerSystem;

    bool showStats=false, showInventory=false, showMap=false;

    // ── UI состояние инвентаря/экипировки ────────────────────
    int  hoveredInvSlot  = -1;   // слот под курсором мыши
    int  selectedInvSlot = -1;   // выбранный слот (для экипировки)
    bool showCharacter   = false; // окно персонажа с экипировкой (C)

    // Переменные для диалогов и взаимодействия
    uint32_t  interactTarget = 0;
    std::string dialogText;
    float     dialogTimer  = 0.f;
    float     interactRange = 60.f;

    struct BuildingInfo {
        int tx,ty,bw,bh; sf::Color roofCol; int variant;
    };
    std::vector<BuildingInfo> buildings;
    std::vector<sf::Vector2i> fountains;

    struct GameConfig {
        float playerSpawnX  = CITY_CX * TILE;
        float playerSpawnY  = (CITY_CY - 5) * TILE;
        int   enemyCount    = 8;
        float respawnTime   = 60.f;
        bool  bossEnabled   = true;
        float particleScale = 1.0f;
        sf::Color ambientColor = sf::Color(30, 20, 50);
        bool  shadersEnabled = false;
        float brightness     = 1.0f;
        float saturation     = 1.0f;
        bool layerGround   = true;
        bool layerObjects  = true;
        bool layerEntities = true;
        bool layerEffects  = true;
    } gameConfig;

    static std::string getEntityName(EnemyType t) {
        switch(t) {
            case EnemyType::GOBLIN:   return "goblin";
            case EnemyType::TROLL:    return "troll";
            case EnemyType::BANDIT:   return "bandit";
            case EnemyType::WOLF:     return "wolf";
            case EnemyType::SKELETON: return "skeleton";
            default:                  return "goblin";
        }
    }

    void loadGameConfig() {
        std::ifstream f("assets/game_config.json");
        if (!f.is_open()) { std::cout << "[INFO] game_config.json не найден\n"; return; }
        std::stringstream buf; buf << f.rdbuf();
        auto json = SimpleJSON::Parser::parse(buf.str());
        if (!json || !json->isObject()) return;
        if (auto sp = json->get("player_spawn")) {
            if (auto x = sp->get("x")) gameConfig.playerSpawnX = (float)x->asDouble() * TILE;
            if (auto y = sp->get("y")) gameConfig.playerSpawnY = (float)y->asDouble() * TILE;
        }
        if (auto ec = json->get("enemy_count"))   gameConfig.enemyCount  = std::max(1, ec->asInt());
        if (auto rt = json->get("respawn_time"))  gameConfig.respawnTime = (float)rt->asDouble();
        if (auto be = json->get("boss_enabled"))  gameConfig.bossEnabled = be->asBool();
        if (auto ps = json->get("particle_scale")) gameConfig.particleScale = (float)ps->asDouble();
        if (auto sh = json->get("shaders")) {
            if (auto en = sh->get("enabled"))    gameConfig.shadersEnabled = en->asBool();
            if (auto br = sh->get("brightness")) gameConfig.brightness     = (float)br->asDouble();
            if (auto sa = sh->get("saturation")) gameConfig.saturation     = (float)sa->asDouble();
        }
        if (auto lt = json->get("lights")) {
            if (auto amb = lt->get("ambient")) {
                int r=30,g=20,b=50;
                if (auto rv=amb->get("r")) r=rv->asInt();
                if (auto gv=amb->get("g")) g=gv->asInt();
                if (auto bv=amb->get("b")) b=bv->asInt();
                gameConfig.ambientColor = sf::Color(r,g,b);
            }
        }
        if (auto lay = json->get("layers")) {
            if (auto v=lay->get("ground"))   gameConfig.layerGround   = v->asBool();
            if (auto v=lay->get("objects"))  gameConfig.layerObjects  = v->asBool();
            if (auto v=lay->get("entities")) gameConfig.layerEntities = v->asBool();
            if (auto v=lay->get("effects"))  gameConfig.layerEffects  = v->asBool();
        }
        std::cout << "[OK] game_config.json загружен: spawn=("
            << gameConfig.playerSpawnX << "," << gameConfig.playerSpawnY
            << ") enemies=" << gameConfig.enemyCount << "\n";
    }
    static sf::String U(const std::string& s) {
        return sf::String::fromUtf8(s.begin(), s.end());
    }
    GameEngine() : window(sf::VideoMode(WINDOW_W,WINDOW_H),"AETHORIA: Eternal Realms",sf::Style::Default) {
        window.setFramerateLimit(60);
        camera.setSize(WINDOW_W,WINDOW_H);

        // Иконка окна — генерируем программно (логотип 32×32)
        {
            sf::Image icon;
            icon.create(32, 32, sf::Color(0,0,0,0));
            // Фон — тёмно-фиолетовый
            for (unsigned y=0;y<32;y++) for (unsigned x=0;x<32;x++)
                icon.setPixel(x, y, sf::Color(15, 10, 35));
            // Золотой символ ⚔ — рисуем два пересекающихся прямоугольника
            for (int i=4;i<28;i++) {
                icon.setPixel(i, 16, sf::Color(220,170,50));
                icon.setPixel(16, i, sf::Color(220,170,50));
            }
            // Ромб вокруг
            for (int i=0;i<8;i++) {
                icon.setPixel(16-i,   8+i,  sf::Color(180,100,220));
                icon.setPixel(16+i,   8+i,  sf::Color(180,100,220));
                icon.setPixel(16-i,  24-i,  sf::Color(180,100,220));
                icon.setPixel(16+i,  24-i,  sf::Color(180,100,220));
            }
            window.setIcon(32, 32, icon.getPixelsPtr());
        }
        std::mt19937 rng2(123);
        for(int i=0;i<200;i++){
            Star s;
            s.x=(float)(rng2()%WINDOW_W);
            s.y=(float)(rng2()%WINDOW_H);
            s.speed=0.1f+(rng2()%30)*0.01f;
            s.brightness=0.3f+(rng2()%70)*0.01f;
            stars.push_back(s);
        }
        charSlots[0]={"", -1, 0, 1, true};
        charSlots[1]={"", -1, 0, 1, true};
        charSlots[2]={"", -1, 0, 1, true};
        loadCharSlots();
        loadGameConfig();
        loadAssets();
        initSceneManager();
        initPlayer();
        initSkills();
        loadStartScene();
    }

    ~GameEngine() {
        if (animManager) delete animManager;
        if (audioManager) delete audioManager;          // FIX: освобождаем audioManager
        if (player.animPlayer) delete player.animPlayer;
        for (auto& enemy : enemies) if (enemy.animPlayer) delete enemy.animPlayer;
    }

    void loadAssets() {
#ifdef _WIN32
        SetConsoleOutputCP(65001);
        SetConsoleCP(65001);
#endif
        std::setlocale(LC_ALL,"");
        const char* fontPaths[]={
            "assets/fonts/arial.ttf","assets/fonts/cinzel.ttf",
            "C:/Windows/Fonts/arial.ttf","C:/Windows/Fonts/arialuni.ttf",
            "C:/Windows/Fonts/calibri.ttf","C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/tahoma.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",nullptr};
        for(int i=0;fontPaths[i];i++)
            if(font.loadFromFile(fontPaths[i])){fontLoaded=true;break;}
        animManager = new AnimationManager("assets");
        if (!animManager->loadAnimationsJSON("assets/animations.json"))
            std::cerr << "[WARNING] Не удалось загрузить animations.json" << std::endl;
        else
            std::cout << "[OK] Система анимаций инициализирована успешно!" << std::endl;
        audioManager = new AudioManager();             // FIX: инициализируем audioManager
        // AudioManager::initializeSoundEffects() вызывается в конструкторе — загружаем все звуки
        {
            // Загружаем звуки через публичный API
            const char* sounds[][2] = {
                {"slash",    "assets/sounds/slash.wav"},
                {"fireball", "assets/sounds/fireball.wav"},
                {"heal",     "assets/sounds/heal.wav"},
                {"damage",   "assets/sounds/damage.wav"},
                {"critical", "assets/sounds/critical.wav"},
                {"levelup",  "assets/sounds/levelup.wav"},
                {"death",    "assets/sounds/death.wav"},
                {"pickup",   "assets/sounds/pickup.wav"},
                {"ui_click", "assets/sounds/ui_click.wav"},
                {nullptr, nullptr}
            };
            for (int i = 0; sounds[i][0]; i++)
                audioManager->loadSound(sounds[i][0], sounds[i][1]);
        }
        loadCustomTileTextures();                      // FIX: загружаем кастомные тайлы
    }

    // ── Загрузка текстур кастомных тайлов из custom_tiles.json ───
    void loadCustomTileTextures() {
        if (customTilesTexturesLoaded) return;
        customTilesTexturesLoaded = true;
        std::ifstream f("assets/custom_tiles.json");
        if (!f.is_open()) { std::cout << "[INFO] custom_tiles.json не найден\n"; return; }
        std::stringstream buf; buf << f.rdbuf();
        auto root = SimpleJSON::Parser::parse(buf.str());
        if (!root || !root->isArray()) return;

        int loaded = 0;
        // ── Шаг 1: загружаем все текстуры в map (без создания спрайтов!)
        // Нельзя создавать sf::Sprite пока вставляем в map — каждая вставка
        // может реаллоцировать память и указатель внутри спрайта станет висячим.
        for (size_t i = 0; i < root->arrayVal.size(); i++) {
            auto j = root->get(i);
            if (!j) continue;
            std::string tid = j->get("id")      ? j->get("id")->asString()      : "";
            std::string tex = j->get("texture") ? j->get("texture")->asString() : "";
            if (tid.empty() || tex.empty()) continue;
            // Ищем файл по нескольким стандартным путям
            std::vector<std::string> candidates = {
                tex,
                "assets/textures/tiles/" + tex,
                "assets/tiles/" + tex,
                "assets/" + tex,
                "assets/textures/" + tex,
            };
            for (auto& path : candidates) {
                sf::Texture t;
                if (t.loadFromFile(path)) {
                    t.setRepeated(false);
                    t.setSmooth(false);
                    customTileTextures[tid] = std::move(t);
                    loaded++;
                    std::cout << "[OK] Текстура тайла '" << tid << "': " << path << "\n";
                    break;
                }
            }
            if (!customTileTextures.count(tid))
                std::cout << "[WARN] Текстура тайла '" << tid << "' не найдена: " << tex << "\n";
        }

        // ── Шаг 2: создаём спрайты только ПОСЛЕ того как map не будет расти.
        // Теперь адреса текстур стабильны — sf::Sprite безопасно хранит указатель.
        customTileSprite.clear();
        for (auto& [tid, tex] : customTileTextures) {
            customTileSprite[tid] = sf::Sprite(tex);
        }

        std::cout << "[OK] Загружено текстур кастомных тайлов: " << loaded << "\n";
    }

    std::string savePath(){
        std::string dir="saves";
#ifdef _WIN32
        _mkdir(dir.c_str());
#else
        mkdir(dir.c_str(),0755);
#endif
        std::string login=loginText.toAnsiString();
        for(size_t j=0;j<login.size();j++){ char&c=login[j]; if(c==47||c==58||c==42||c==60||c==62||c==124) c=95; }
        return dir+"/"+login+".dat";
    }

    void saveGame(){
        std::ofstream f(savePath(),std::ios::binary);
        if(f.is_open()){
            f.write((char*)&player.pos,sizeof(V2)); f.write((char*)&player.hp,sizeof(float));
            f.write((char*)&player.mp,sizeof(float)); f.write((char*)&player.level,sizeof(int));
            f.write((char*)&player.gold,sizeof(int));
            f.close();
        }
        std::string entityFile = savePath() + "_entities.json";
        if (Entity* pe = entitySystem.getEntity(playerEntityId)) {
            pe->x = player.pos.x; pe->y = player.pos.y;
            pe->props.setFloat("hp",    player.hp);
            pe->props.setInt  ("level", player.level);
            pe->props.setInt  ("gold",  player.gold);
        }
        entitySystem.save(entityFile);
    }

    void loadGame(){
        std::ifstream f(savePath(),std::ios::binary);
        if(f.is_open()){
            f.read((char*)&player.pos,sizeof(V2)); f.read((char*)&player.hp,sizeof(float));
            f.read((char*)&player.mp,sizeof(float)); f.read((char*)&player.level,sizeof(int));
            f.read((char*)&player.gold,sizeof(int));
            f.close();
        }
        std::string entityFile = savePath() + "_entities.json";
        entitySystem.load(entityFile);
    }

    void loadCharSlots() {}

    // ════════════════════════════════════════════════════════════════
    // СИСТЕМА СЦЕН
    // ════════════════════════════════════════════════════════════════
    void initSceneManager() {
        sceneManager.onSceneLoaded = [this](const SceneData& sd, EntitySystem& es) {
            applySceneData(sd);
            spawnEnemies();
        };
        sceneManager.onSceneUnload = [this]() {
            enemies.clear();
            fountains.clear();
            mapEnemyDefs.clear();
            // FIX: критический баг — очищаем NPC/OBJECT/PORTAL сущности при смене сцены.
            // Враги и игрок управляются отдельно, их не трогаем тут.
            auto& all = entitySystem.getAll();
            all.erase(std::remove_if(all.begin(), all.end(), [](const Entity& e){
                return e.type == EntityType::NPC ||
                       e.type == EntityType::OBJECT ||
                       e.type == EntityType::PORTAL ||
                       e.type == EntityType::ITEM_DROP ||
                       e.type == EntityType::ENEMY;
            }), all.end());
        };
        sceneManager.loadFromConfig("assets/game_config.json");
        if (sceneManager.sceneOrder().empty()) {
            auto reg = [&](const std::string& name){
                SceneInfo si;
                si.id          = SceneUtils::toId(name);
                si.displayName = name;
                si.mapFile     = SceneUtils::sceneFile(si.id);
                sceneManager.registerScene(si);
            };
            reg("Aethoria City");
            reg("Dark Forest");
            reg("Goblin Caves");
            reg("Dragon Lair");
            reg("Frozen Tundra");
        }
        std::cout << "[SceneManager] Зарегистрировано сцен: "
                  << sceneManager.sceneOrder().size() << "\n";

        // ── Загружаем связи порталов ──────────────────────────────
        sceneManager.loadPortalLinks("assets/portal_links.json");

        // ── Инициализируем систему триггеров ──────────────────────
        // Колбэк: переход в сцену с заданным spawn-offset
        triggerSystem.onSceneSwitch = [this](const std::string& sceneId,
                                             float spawnX, float spawnY) {
            pendingSpawnX = spawnX * TILE;   // тайлы → пиксели
            pendingSpawnY = spawnY * TILE;
            switchToScene(sceneId);
        };
        // Колбэк: диалог/сообщение
        triggerSystem.onDialogue = [this](const std::string& text, float dur) {
            dialogText  = text;
            dialogTimer = dur;
        };
        // Колбэк: частицы на позиции триггера
        triggerSystem.onParticles = [this](float x, float y) {
            spawnParticles({x, y}, ParticleT::MAGIC, 15);
        };
        // Колбэк: проверка уровня
        triggerSystem.onLevelCheck = [this](int req, const std::string& msg) -> bool {
            if (player.level < req) {
                dialogText  = msg;
                dialogTimer = 3.f;
                return false;
            }
            return true;
        };
        // Колбэк: вход в инстанс (серверную сцену)
        triggerSystem.onInstanceEnter = [this](const std::string& instanceId) {
            switchToScene(instanceId);
        };

        // Загружаем триггеры и серверные сцены из JSON
        triggerSystem.loadFromFile("assets/triggers.json");
    }

    void loadStartScene() {
        // map.json больше не используется — редактор сохраняет прямо в assets/scenes/
        if (!sceneManager.loadDefault(entitySystem)) {
            buildMap();    // fallback если нет ни одного файла сцены
            spawnEnemies();
        }

        // ── Этап 1: Prefab catalog ────────────────────────────────
        AethoriaPrefabFactory::registerDefaults(prefabCatalog);
        // Загружаем пользовательские префабы поверх (если файл есть)
        prefabCatalog.load("assets/prefabs.json");
        prefabCatalog.validateDuplicates();

        // ── Этап 1: Quest system ──────────────────────────────────
        if (!questSystem.load("assets/quests.json")) {
            std::cout << "[Quest] Квестов нет — создаём файл по умолчанию\n";
        }
        questSystem.printStatus();

        // ── Редмап Приоритет 2: Скиллы и предметы ────────────────
        skillSystem.loadDefs("assets/skills.json");
        itemSystem.loadItems("assets/items.json");
        itemSystem.loadDropTables("assets/drop_tables.json");

        // ── Редмап Приоритет 1: EventBus wire-up ─────────────────
        _initEventHandlers();
    }

    void _initEventHandlers() {
        // Убийство врага → квест + лут + XP/gold
        eventBus.on<EnemyKilledEvent>([&](const EnemyKilledEvent& ev) {
            // Квесты
            auto qResults = questSystem.onKill(ev.enemyName);
            for (auto& r : qResults) {
                player.xp   += r.xpRewarded;
                player.gold += r.goldRewarded;
                if (r.questCompleted) {
                    questNotification = "✓ Квест: " + r.questName;
                    questNotifyTimer  = 4.f;
                }
            }
            // XP + gold от врага напрямую
            // (Предметный лут кладётся в bag при подборе с земли — через spawnItemLoot)
            player.xp   += ev.xpReward;
            player.gold += ev.goldDrop;
            checkLevelUp();
        });

        // Подбор предмета → квест
        eventBus.on<ItemPickupEvent>([&](const ItemPickupEvent& ev) {
            questSystem.onCollect(ev.itemName, ev.count);
        });

        // Level-up уведомление
        eventBus.on<LevelUpEvent>([&](const LevelUpEvent& ev) {
            questNotification = "⬆ УРОВЕНЬ " + std::to_string(ev.newLevel) + "!";
            questNotifyTimer  = 5.f;
            if (audioManager) audioManager->playSound("levelup");
        });

        // NPC диалог через EventBus
        eventBus.on<NpcDialogueEvent>([&](const NpcDialogueEvent& ev) {
            dialogText  = ev.message;
            dialogTimer = ev.duration;
        });

        // Квест принят
        eventBus.on<QuestAcceptedEvent>([&](const QuestAcceptedEvent& ev) {
            questNotification = "! Задание принято";
            questNotifyTimer  = 3.f;
        });
    }

    void applySceneData(const SceneData& sd) {
        // Копируем тайлы напрямую (worldMap теперь SceneTile)
        for (int y=0; y<MAP_H && y<sd.height; y++) {
            for (int x=0; x<MAP_W && x<sd.width; x++) {
                worldMap[y][x] = sd.tiles[y][x];
            }
        }
        gameConfig.playerSpawnX = sd.spawnX;
        gameConfig.playerSpawnY = sd.spawnY;
        player.pos = {sd.spawnX, sd.spawnY};
        camera.setCenter(player.pos.x, player.pos.y);
        fountains = sd.fountains;

        // Читаем врагов напрямую из SceneData.enemyDefs (заполненных parseEntity)
        // Больше не зависим от entitySystem — нет дублирования и нет random fallback
        mapEnemyDefs.clear();
        for (auto& def : sd.enemyDefs) {
            EnemyDef d;
            d.tileX    = def.tileX;
            d.tileY    = def.tileY;
            d.typeName = def.typeName;
            d.level    = def.level;
            d.boss     = def.boss;
            d.hp       = def.hp;
            d.dmg      = def.dmg;
            mapEnemyDefs.push_back(d);
        }

        std::cout << "[Scene] Применено: тайлы, spawn=("
                  << sd.spawnX << "," << sd.spawnY
                  << ") фонтанов=" << fountains.size()
                  << " врагов=" << mapEnemyDefs.size() << "\n";

        // ── Перестраиваем триггеры порталов для новой сцены ──────────
        // Удаляем старые portal-триггеры (по префиксу "portal_link_")
        {
            auto& all = triggerSystem.all();
            all.erase(std::remove_if(all.begin(), all.end(), [](const Trigger& t){
                return t.id.size() >= 12 &&
                       t.id.substr(0, 12) == "portal_link_";
            }), all.end());
        }
        // Создаём новые триггеры на основе portal_links для текущей сцены
        for (auto& pl : sd.portalLinks) {
            // Ищем сущность-портал по имени / ID в entitySystem
            for (auto& ent : entitySystem.getAll()) {
                if (ent.type != EntityType::PORTAL) continue;
                // Совпадение по portal_id в props или по имени
                std::string eid = ent.props.getStr("portal_id", "");
                if (eid.empty()) eid = ent.name;
                if (eid != pl.portalId) continue;

                Trigger t;
                t.id           = "portal_link_" + pl.portalId;
                t.name         = pl.label.empty() ? ("→ " + pl.toScene) : pl.label;
                t.shape        = TriggerShape::CIRCLE;
                t.x            = ent.x;
                t.y            = ent.y;
                t.radius       = 40.f;
                t.event        = TriggerEvent::ENTER;
                t.action       = TriggerAction::SCENE_SWITCH;
                t.targetScene  = pl.toScene;
                t.spawnOffsetX = pl.spawnX;
                t.spawnOffsetY = pl.spawnY;
                triggerSystem.add(t);
                std::cout << "[Trigger] Портал " << pl.portalId
                          << " → " << pl.toScene << " зарегистрирован\n";
            }
        }
        // Дополнительно: порталы у которых target_zone задан напрямую в props
        for (auto& ent : entitySystem.getAll()) {
            if (ent.type != EntityType::PORTAL) continue;
            std::string dest = ent.props.getStr("target_zone", "");
            if (dest.empty()) continue;
            std::string tid = "portal_direct_" + std::to_string(ent.id);
            // Не дублируем если уже есть portal_link
            if (triggerSystem.find(tid)) continue;
            // Проверяем нет ли уже portal_link для этого портала
            std::string pid = ent.props.getStr("portal_id", ent.name);
            if (triggerSystem.find("portal_link_" + pid)) continue;
            Trigger t;
            t.id          = tid;
            t.name        = "→ " + dest;
            t.shape       = TriggerShape::CIRCLE;
            t.x           = ent.x;
            t.y           = ent.y;
            t.radius      = 40.f;
            t.event       = TriggerEvent::ENTER;
            t.action      = TriggerAction::SCENE_SWITCH;
            t.targetScene = dest;
            triggerSystem.add(t);
        }
    }

    void switchToScene(const std::string& sceneId) {
        if (sceneTransitioning) return;
        pendingSceneId       = sceneId;
        sceneTransitioning   = true;
        sceneTransitionTimer = 0.5f;
        std::cout << "[Scene] Переключение → " << sceneId << "\n";
    }

    // Перегрузка с явным spawn-offset (вызывается из triggerSystem.onSceneSwitch)
    void switchToScene(const std::string& sceneId, float spawnPxX, float spawnPxY) {
        pendingSpawnX = spawnPxX;
        pendingSpawnY = spawnPxY;
        switchToScene(sceneId);
    }

    void updateSceneTransition(float dt) {
        if (!sceneTransitioning) return;
        sceneTransitionTimer -= dt;
        if (sceneTransitionTimer <= 0.f) {
            sceneTransitioning = false;
            sceneManager.switchScene(pendingSceneId, entitySystem,
                worldMap, MAP_W, MAP_H,
                player.pos.x, player.pos.y);
            // Применяем кастомный spawn-offset если задан порталом
            if (pendingSpawnX >= 0.f && pendingSpawnY >= 0.f) {
                player.pos = {pendingSpawnX, pendingSpawnY};
                camera.setCenter(player.pos.x, player.pos.y);
                entitySystem.setPosition(playerEntityId, player.pos.x, player.pos.y);
            }
            pendingSpawnX = -1.f;
            pendingSpawnY = -1.f;
            pendingSceneId.clear();
        }
    }

    void drawSceneTransition() {
        if (!sceneTransitioning && sceneTransitionTimer<=0.f) return;
        float alpha = std::min(1.f, 1.f - sceneTransitionTimer/0.5f);
        sf::RectangleShape overlay({(float)WINDOW_W,(float)WINDOW_H});
        overlay.setFillColor(sf::Color(0,0,0,uint8_t(alpha*255)));
        window.setView(window.getDefaultView());
        window.draw(overlay);
    }

    static void ensureScenesDir() {
#ifdef _WIN32
        std::system("if not exist \"assets\\scenes\" mkdir \"assets\\scenes\"");
#else
        std::system("mkdir -p \"assets/scenes\"");
#endif
    }

    // ════════════════════════════════════════════════════════════════
    // ЗАГРУЗКА КАРТЫ ИЗ map.json (fallback, не используется при сценах)
    // ════════════════════════════════════════════════════════════════
    bool loadMapFromJSON() {
        std::cout << "[INFO] loadMapFromJSON не используется при активных сценах.\n";
        return false;
    }

    void buildMap() {
        // Fallback, если нет сцен и map.json
        for(int y=0;y<MAP_H;y++) for(int x=0;x<MAP_W;x++) {
            worldMap[y][x].type     = SceneTileType::GRASS;
            worldMap[y][x].walkable = true;
            worldMap[y][x].color    = sf::Color(60,140,50);
            worldMap[y][x].variant  = 0;
        }
        for(int x=25;x<35;x++) for(int y=25;y<35;y++) worldMap[y][x].type = SceneTileType::STONE_FLOOR;
        for(int x=60;x<65;x++) for(int y=10;y<20;y++) {
            worldMap[y][x].type = SceneTileType::WATER;
            worldMap[y][x].walkable = false;
        }
        for(int i=0;i<20;i++){
            int tx=20+rand()%80, ty=20+rand()%80;
            worldMap[ty][tx].type = SceneTileType::TREE;
            worldMap[ty][tx].walkable = false;
        }
        int bx=(CITY_CX-5),by=(CITY_CY-5);
        for(int y=by;y<by+10;y++) for(int x=bx;x<bx+10;x++)
            worldMap[y][x].type = SceneTileType::BUILDING_FLOOR;
        fountains.push_back({CITY_CX,CITY_CY});
        fountains.push_back({CITY_CX+10,CITY_CY-8});
    }

    void initPlayer(){
        player.pos={gameConfig.playerSpawnX, gameConfig.playerSpawnY};
        player.hp=player.maxHp=100; player.mp=player.maxMp=80;
        player.level=1; player.xp=0; player.xpNext=100;
        player.str=10; player.agi=8; player.intel=6; player.vit=12;
        player.baseAtk=15; player.baseDef=5;
        player.gold=50; player.kills=0;
        player.name="Player"; player.className="Warrior";
        player.speed=200;
        player.attackCD=0.8f; player.currentAttackCD=0;
        if (animManager) {
            player.animPlayer = new AnimationPlayer(animManager);
            player.animPlayer->playAnimation("player", "idle");
            player.animPlayer->setScale(1.2f, 1.2f);
            player.stateMachine = AnimStateMachine(player.animPlayer, "player");
            player.stateMachine.setClipOverride(AnimState::RUN,  "run");
            player.stateMachine.setClipOverride(AnimState::WALK, "walk");
            std::cout << "[OK] Анимация игрока инициализирована" << std::endl;
        }
        EntityProperties pp;
        pp.setInt("level",   player.level);
        pp.setFloat("hp",    player.hp);
        pp.setFloat("maxHp", player.maxHp);
        pp.setStr("class",   player.className);
        playerEntityId = entitySystem.addEntityFull(EntityType::PLAYER, player.pos.x, player.pos.y, player.name, pp);
    }

    void initSkills(){
        player.skills.push_back({"Slash",     "Basic melee attack",    0,  25, 0.5f, 0});
        player.skills.push_back({"Fireball",  "Ranged fire attack",   30,  40, 1.5f, 0});
        player.skills.push_back({"Shield",    "Defense buff",         20,   0, 2.0f, 0});
        player.skills.push_back({"Heal",      "Restore HP",           40,   0, 3.0f, 0});

        // SkillSystem instances (привязаны к def IDs из skills.json)
        for (auto& [id, def] : skillSystem.allDefs()) {
            SkillInstance si;
            si.defId   = id;
            si.unlocked = true;
            si.level   = 1;
            player.skillInstances.push_back(si);
        }
        // Fallback если skills.json не загружен — создаём базовые
        if (player.skillInstances.empty()) {
            for (const std::string& id : {"slash","fireball","shield","heal"}) {
                SkillInstance si; si.defId = id; si.unlocked = true;
                player.skillInstances.push_back(si);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════
    // СИСТЕМА ЭКИПИРОВКИ
    // ═══════════════════════════════════════════════════════════

    // Пересчитать суммарные бонусы от всей надетой экипировки
    void recalcEquipBonuses() {
        player.equipBonuses.clear();
        for (auto& [slot, itemId] : player.equipped) {
            if (itemId.empty()) continue;
            const ItemDef* def = itemSystem.get(itemId);
            if (!def) continue;
            for (auto& [k, v] : def->stats)
                player.equipBonuses[k] += v;
        }
        // Применяем к базовым статам (пересчитываем maxHp/maxMp с бонусами)
        float bonusHp  = player.equipBonuses.count("hp")  ? player.equipBonuses["hp"]  : 0.f;
        float bonusMp  = player.equipBonuses.count("mp")  ? player.equipBonuses["mp"]  : 0.f;
        // maxHp/maxMp = base + bonus (base у игрока фиксированный до добавления)
        // Просто обновляем верхнюю планку — HP/MP не обрезаем до новых значений
        // (базовое maxHp=100 хранится в player, бонусы отдельно)
    }

    // Надеть предмет из инвентаря (по индексу слота bag)
    bool equipItem(int invSlotIdx) {
        if (invSlotIdx < 0 || invSlotIdx >= (int)player.bag.slots.size()) return false;
        const std::string& itemId = player.bag.slots[invSlotIdx].itemId;
        const ItemDef* def = itemSystem.get(itemId);
        if (!def) return false;
        // Только надеваемые типы
        if (def->type == "consumable" || def->type == "material" || def->type == "quest")
            return false;

        // ── Проверка ограничения по классу ─────────────────────
        if (!def->canBeUsedBy(player.className)) {
            floatingTexts.push_back({
                player.pos + V2(0, -40),
                "Только для: " + def->allowedClassesStr(),
                2.5f, sf::Color(255, 80, 80), false
            });
            if (audioManager) audioManager->playSound("ui_click");
            return false;
        }

        // Определяем слот
        std::string slot = def->slot;
        if (slot.empty()) {
            // Автоопределение по типу
            if      (def->type == "weapon")  slot = "main_hand";
            else if (def->type == "armor")   slot = "chest";
            else if (def->type == "helmet")  slot = "head";
            else if (def->type == "boots")   slot = "legs";
            else if (def->type == "ring")    slot = "ring1";
            else if (def->type == "amulet")  slot = "neck";
            else if (def->type == "shield")  slot = "off_hand";
            else return false;
        }

        // Если слот занят — снимаем и возвращаем в инвентарь
        auto it = player.equipped.find(slot);
        if (it != player.equipped.end() && !it->second.empty()) {
            std::string old = it->second;
            player.equipped[slot] = "";
            itemSystem.giveToInventory(player.bag, old, 1);
        }

        // Надеваем: убираем из инвентаря, ставим в слот
        player.bag.slots[invSlotIdx].count--;
        if (player.bag.slots[invSlotIdx].count <= 0)
            player.bag.slots.erase(player.bag.slots.begin() + invSlotIdx);

        player.equipped[slot] = itemId;
        recalcEquipBonuses();

        selectedInvSlot = -1;
        if (audioManager) audioManager->playSound("ui_click");
        floatingTexts.push_back({player.pos + V2(0,-30), def->name, 2.f, sf::Color(220,185,80), false});
        return true;
    }

    // Снять предмет из слота экипировки
    void unequipSlot(const std::string& slot) {
        auto it = player.equipped.find(slot);
        if (it == player.equipped.end() || it->second.empty()) return;
        std::string itemId = it->second;
        it->second = "";
        itemSystem.giveToInventory(player.bag, itemId, 1);
        recalcEquipBonuses();
        if (audioManager) audioManager->playSound("ui_click");
    }

    // Итоговый стат с учётом экипировки
    float getTotalStat(const std::string& key) const {
        float bonus = 0.f;
        auto it = player.equipBonuses.find(key);
        if (it != player.equipBonuses.end()) bonus = it->second;
        if (key == "damage")  return player.baseAtk + player.str * 2 + bonus;
        if (key == "defense") return player.baseDef + player.vit + bonus;
        if (key == "hp")      return player.maxHp + bonus;
        if (key == "mp")      return player.maxMp + bonus;
        return bonus;
    }

    void spawnEnemies(){
        enemies.clear();
        // FIX: очищаем старые ENEMY-сущности из entitySystem перед спауном
        {
            auto& all = entitySystem.getAll();
            all.erase(std::remove_if(all.begin(), all.end(), [](const Entity& e){
                return e.type == EntityType::ENEMY;
            }), all.end());
        }
        if (!mapEnemyDefs.empty()) {
            for (auto& def : mapEnemyDefs) {
                Enemy e;
                e.pos = e.spawnPos = {
                    (float)def.tileX * TILE + TILE / 2.f,
                    (float)def.tileY * TILE + TILE / 2.f
                };
                if      (def.typeName=="TROLL")    e.type = EnemyType::TROLL;
                else if (def.typeName=="BANDIT")   e.type = EnemyType::BANDIT;
                else if (def.typeName=="WOLF")     e.type = EnemyType::WOLF;
                else if (def.typeName=="SKELETON") e.type = EnemyType::SKELETON;
                else                               e.type = EnemyType::GOBLIN;
                e.level  = std::max(1, def.level);
                e.boss   = def.boss && gameConfig.bossEnabled;
                e.maxHp  = def.hp  > 0 ? def.hp  : (20 + e.level * 5);
                e.hp     = e.maxHp;
                e.damage = def.dmg > 0 ? def.dmg : (5  + e.level * 2);
                e.gold   = 10 + e.level * 5;
                e.speed  = 100 + e.level * 10;
                e.name   = def.typeName;
                if (animManager) {
                    e.animPlayer = new AnimationPlayer(animManager);
                    std::string ename = getEntityName(e.type);
                    if (!e.animPlayer->playAnimation(ename, "idle")) {
                        delete e.animPlayer; e.animPlayer = nullptr;
                    } else {
                        float sc = e.boss ? 1.2f : 0.8f;
                        e.animPlayer->setScale(sc, sc);
                        e.stateMachine = AnimStateMachine(e.animPlayer, ename);
                    }
                }
                // ── AI FSM инициализация ──────────────────────────────
                e.ai.params.aggroRange    = e.boss ? 280.f : AGGRO_RANGE;
                e.ai.params.deaggroRange  = DEAGGRO_RANGE;
                e.ai.params.combatRange   = e.boss ? 60.f : 40.f;
                e.ai.params.patrolSpeed   = e.speed * 0.55f;
                e.ai.params.aggroSpeed    = e.speed;
                e.ai.params.combatSpeed   = e.speed * 0.8f;
                e.ai.params.attackCooldown = e.attackCD > 0 ? e.attackCD : 1.2f;
                e.ai.params.isBoss        = e.boss;
                e.ai.params.fleeAtLowHp   = false;
                AISystem::initDefaultPatrol(e.ai, e.pos.x, e.pos.y, e.boss ? 96.f : 64.f);
                enemies.push_back(std::move(e));
            }
            for (auto& en : enemies) {
                EntityProperties ep;
                ep.setStr  ("subtype", en.name);
                ep.setInt  ("level",   en.level);
                ep.setFloat("hp",      en.hp);
                ep.setFloat("maxHp",   en.maxHp);
                ep.setInt  ("damage",  en.damage);
                ep.setInt  ("gold",    en.gold);
                ep.setBool ("boss",    en.boss);
                entitySystem.addEntityFull(EntityType::ENEMY, en.pos.x, en.pos.y, en.name, ep);
            }
            std::cout << "[OK] Враги из map.json: " << enemies.size() << "\n";
            return;
        }
        // Fallback random spawn
        std::vector<std::string> names={"Goblin","Troll","Bandit","Wolf","Skeleton"};
        int count = gameConfig.enemyCount;
        for(int i=0;i<count;i++){
            Enemy e;
            e.pos={float(30+rand()%60)*TILE, float(30+rand()%60)*TILE};
            e.spawnPos=e.pos;
            e.type=(EnemyType)(rand()%5);
            e.level=1+(rand()%3);
            e.hp=e.maxHp=20+e.level*5;
            e.damage=5+e.level*2;
            e.gold=10+e.level*5;
            e.speed=100+e.level*10;
            e.name=names[(int)e.type % 5];
            e.boss = gameConfig.bossEnabled && (rand()%8==0);
            if (animManager) {
                e.animPlayer = new AnimationPlayer(animManager);
                std::string entityName = getEntityName(e.type);
                if (!e.animPlayer->playAnimation(entityName, "idle")) {
                    delete e.animPlayer; e.animPlayer = nullptr;
                } else {
                    float sc = e.boss ? 1.2f : 0.8f;
                    e.animPlayer->setScale(sc, sc);
                    e.stateMachine = AnimStateMachine(e.animPlayer, entityName);
                }
            }
            // ── AI FSM инициализация ──────────────────────────────
            e.ai.params.aggroRange    = e.boss ? 280.f : AGGRO_RANGE;
            e.ai.params.deaggroRange  = DEAGGRO_RANGE;
            e.ai.params.combatRange   = e.boss ? 60.f : 40.f;
            e.ai.params.patrolSpeed   = e.speed * 0.55f;
            e.ai.params.aggroSpeed    = e.speed;
            e.ai.params.combatSpeed   = e.speed * 0.8f;
            e.ai.params.attackCooldown = 1.2f;
            e.ai.params.isBoss        = e.boss;
            AISystem::initDefaultPatrol(e.ai, e.pos.x, e.pos.y, e.boss ? 96.f : 64.f);
            enemies.push_back(e);
        }
        for (auto& en : enemies) {
            EntityProperties ep;
            ep.setStr  ("subtype", en.name);
            ep.setInt  ("level",   en.level);
            ep.setFloat("hp",      en.hp);
            ep.setFloat("maxHp",   en.maxHp);
            ep.setInt  ("damage",  en.damage);
            ep.setInt  ("gold",    en.gold);
            ep.setBool ("boss",    en.boss);
            entitySystem.addEntityFull(EntityType::ENEMY, en.pos.x, en.pos.y, en.name, ep);
        }
    }

    // ════════════════════════════════════════════════════════════════
    // ВЗАИМОДЕЙСТВИЕ [E] и диалоги
    // ════════════════════════════════════════════════════════════════
    void tryInteract() {
        // ── Сначала проверяем INTERACT-триггеры ──────────────────────
        std::string triggerId = triggerSystem.tryInteract(
            player.pos.x, player.pos.y, player.level);
        if (!triggerId.empty()) return;   // триггер обработан

        float bestDist = interactRange * interactRange;
        Entity* target = nullptr;
        for (auto& e : entitySystem.getAll()) {
            if (!e.active) continue;
            // FIX: исключаем ENEMY и PLAYER — и особо проверяем ID игрока
            if (e.type == EntityType::ENEMY || e.type == EntityType::PLAYER) continue;
            if (e.id == playerEntityId) continue;   // FIX: явно пропускаем сущность игрока
            float dx = player.pos.x - e.x, dy = player.pos.y - e.y;
            float d2 = dx*dx + dy*dy;
            if(d2 < bestDist) { bestDist=d2; target=&e; }
        }
        if(!target) return;
        switch(target->type) {
        case EntityType::NPC: {
            std::string sub=target->props.getStr("subtype","");
            dialogText = target->name + ": ";
            if(sub=="HEALER") {
                player.hp=player.maxHp; player.mp=player.maxMp;
                dialogText += "Свет Аэтории исцелит тебя!";
                spawnParticles(player.pos, ParticleT::HEAL, 12);
            } else if(sub=="VENDOR") {
                dialogText += "Добро пожаловать! Лучшие товары в Аэтории!";
            } else if(sub=="QUEST") {
                // Этап 1: полноценный квест-диалог через QuestSystem
                std::string qid = target->props.getStr("quest_id", "");
                auto ir = questSystem.interact(qid, player.level);
                dialogText = target->name + ": " + ir.allLines();
                if (ir.questAssigned) {
                    questNotification = "Квест принят!";
                    questNotifyTimer   = 5.f;
                }
                if (ir.questComplete) {
                    player.gold += ir.goldRewarded;
                    player.xp   += ir.xpRewarded;
                    for (auto& itm : ir.items)
                        player.inventory.push_back({itm, "quest_reward", 1, 0, 0, 0});
                    spawnParticles(player.pos, ParticleT::HEAL, 20);
                    questNotification = "Квест выполнен! +" +
                        std::to_string(ir.xpRewarded) + " XP  +" +
                        std::to_string(ir.goldRewarded) + "g";
                    questNotifyTimer = 6.f;
                    questSystem.onTalk(target->name);
                }
            } else if(sub=="BLACKSMITH") {
                dialogText += "Нужно оружие? Сталь Борга — лучшая в мире!";
            } else if(sub=="INNKEEPER") {
                dialogText += "Добро пожаловать в «Золотую чарку»!";
            } else {
                dialogText += "Привет, путник!";
            }
            dialogTimer = 4.f;
            break;
        }
        case EntityType::OBJECT: {
            bool opened=target->props.getBool("opened",false);
            if(!opened) {
                target->props.setBool("opened",true);
                int gold=target->props.getInt("gold_min",5)+
                         rand()%(std::max(1,target->props.getInt("gold_max",20)-
                                          target->props.getInt("gold_min",5)+1));
                player.gold+=gold;
                spawnParticles({target->x,target->y}, ParticleT::MAGIC, 10);
                dialogText = "Найдено: " + std::to_string(gold) + " монет!";
                std::string item=target->props.getStr("item","");
                if(!item.empty()) dialogText += " + " + item;
                dialogTimer = 3.f;
            }
            break;
        }
        case EntityType::PORTAL: {
            // Сначала смотрим portal_links по portal_id сущности
            std::string pid  = target->props.getStr("portal_id", target->name);
            const PortalLink* link = sceneManager.getLinkByPortalId(pid);
            if (link) {
                pendingSpawnX = link->spawnX * TILE;
                pendingSpawnY = link->spawnY * TILE;
                spawnParticles({player.pos.x, player.pos.y}, ParticleT::MAGIC, 20);
                switchToScene(link->toScene);
            } else {
                // Фолбэк: target_zone напрямую в props
                std::string dest = target->props.getStr("target_zone","");
                if (!dest.empty()) {
                    switchToScene(dest);
                } else {
                    dialogText  = "Портал не настроен. Нет цели.";
                    dialogTimer = 2.f;
                }
                spawnParticles({player.pos.x,player.pos.y},ParticleT::MAGIC,20);
            }
            break;
        }
        case EntityType::ITEM_DROP: {
            int gold=target->props.getInt("gold",0);
            if(gold>0) player.gold+=gold;
            std::string item=target->props.getStr("item","");
            dialogText = "Подобрано: "+(item.empty()?"предмет":item);
            if(gold>0) dialogText += " (+" + std::to_string(gold) + "g)";
            dialogTimer = 2.f;
            target->active=false;
            entitySystem.purgeInactive();
            break;
        }
        default: break;
        }
    }

    void drawDialog() {
        if(!fontLoaded || dialogText.empty()) return;
        float alpha=std::min(1.f, dialogTimer)*255.f;
        if(dialogTimer<0.6f) alpha=(dialogTimer/0.6f)*255.f;
        float pw=500, ph=64;
        float px=(WINDOW_W-pw)/2, py=WINDOW_H-ph-24;
        drawRoundedRect(window,px,py,pw,ph,
            sf::Color(20,10,35,uint8_t(alpha*0.92f)),
            sf::Color(140,80,220,uint8_t(alpha)),2);
        sf::Text txt; txt.setFont(font);
        txt.setString(U(dialogText));
        txt.setCharacterSize(15);
        txt.setFillColor(sf::Color(230,210,255,uint8_t(alpha)));
        auto b=txt.getLocalBounds();
        txt.setOrigin(b.width/2,b.height/2);
        txt.setPosition(px+pw/2, py+ph/2);
        window.draw(txt);
    }

    // ════════════════════════════════════════════════════════════════
    // ХЕЛПЕРЫ СОЗДАНИЯ СУЩНОСТЕЙ
    // ════════════════════════════════════════════════════════════════
    uint32_t spawnNpc(float wx, float wy, const std::string& name,
                      const std::string& subtype = "villager",
                      const std::string& dialog  = "") {
        EntityProperties p;
        p.setStr ("subtype",      subtype);
        p.setStr ("dialog",       dialog);
        p.setBool("interactable", true);
        return entitySystem.addEntityFull(EntityType::NPC, wx, wy, name, p);
    }

    uint32_t spawnObject(float wx, float wy, const std::string& name,
                         const std::string& subtype = "chest",
                         int gold = 0, const std::string& item = "") {
        EntityProperties p;
        p.setStr ("subtype", subtype);
        p.setInt ("gold",    gold);
        p.setStr ("item",    item);
        p.setBool("opened",  false);
        return entitySystem.addEntityFull(EntityType::OBJECT, wx, wy, name, p);
    }

    uint32_t spawnPortal(float wx, float wy, const std::string& name,
                         const std::string& destination, float radius = 40.f) {
        EntityProperties p;
        p.setStr  ("destination", destination);
        p.setFloat("radius",      radius);
        return entitySystem.addEntityFull(EntityType::PORTAL, wx, wy, name, p);
    }

    uint32_t spawnItemDrop(float wx, float wy,
                           const std::string& itemName, int gold = 0) {
        EntityProperties p;
        p.setStr("item", itemName);
        p.setInt("gold", gold);
        return entitySystem.addEntityFull(EntityType::ITEM_DROP, wx, wy, itemName, p);
    }

    void run(){
        while(window.isOpen()){
            float dt=std::min(clk.restart().asSeconds(),0.05f);
            starTimer+=dt;
            handleEvents();
            if(gameState==GameState::PLAYING) update(dt);
            render();
        }
    }

    void handleEvents(){
        sf::Event ev;
        while(window.pollEvent(ev)){
            if(ev.type==sf::Event::Closed) window.close();
            if(ev.type==sf::Event::KeyPressed) onKey(ev.key.code);
            if(ev.type==sf::Event::TextEntered) onTextEntered(ev.text.unicode);
            if(ev.type==sf::Event::MouseButtonPressed)
                onMouseClick(ev.mouseButton.x,ev.mouseButton.y);
            if(ev.type==sf::Event::MouseWheelScrolled&&gameState==GameState::PLAYING){
                float z=ev.mouseWheelScroll.delta>0?0.9f:1.1f;
                camera.zoom(z);
            }
        }
    }

    void onTextEntered(sf::Uint32 c){
        if(gameState==GameState::LOGIN){
            if(c==8){
                if(activeField==0&&loginText.getSize()>0) loginText.erase(loginText.getSize()-1);
                else if(activeField==1&&passText.getSize()>0) passText.erase(passText.getSize()-1);
            } else if(c>=32&&c<127){
                if(activeField==0&&loginText.getSize()<20) { sf::String tmp; tmp+=static_cast<sf::Uint32>(c); loginText+=tmp; }
                else if(activeField==1&&passText.getSize()<20) { sf::String tmp; tmp+=static_cast<sf::Uint32>(c); passText+=tmp; }
            }
        } else if(gameState==GameState::CHARACTER_SELECT&&nameFocused){
            if(c==8){if(charNameText.getSize()>0) charNameText.erase(charNameText.getSize()-1);}
            else if(c>=32&&c<127&&charNameText.getSize()<14) { sf::String tmp; tmp+=static_cast<sf::Uint32>(c); charNameText+=tmp; }
        }
    }

    void onKey(sf::Keyboard::Key k){
        if(gameState==GameState::LOGIN){
            if(k==sf::Keyboard::Tab) activeField=(activeField+1)%2;
            if(k==sf::Keyboard::Return) tryLogin();
            return;
        }
        if(gameState==GameState::CHARACTER_SELECT){
            if(k==sf::Keyboard::Left)  selectedSlot=(selectedSlot+2)%3;
            if(k==sf::Keyboard::Right) selectedSlot=(selectedSlot+1)%3;
            if(k==sf::Keyboard::Up)    selectedClass=(selectedClass+3)%4;
            if(k==sf::Keyboard::Down)  selectedClass=(selectedClass+1)%4;
            if(k==sf::Keyboard::Return) enterWorld();
            if(k==sf::Keyboard::Escape){ gameState=GameState::LOGIN; }
            return;
        }
        switch(k){
            case sf::Keyboard::Num1:useSkill(0);break;
            case sf::Keyboard::Num2:useSkill(1);break;
            case sf::Keyboard::Num3:useSkill(2);break;
            case sf::Keyboard::Num4:useSkill(3);break;
            case sf::Keyboard::Num7:
                if(player.hpPotions>0){player.hp=std::min(player.maxHp,player.hp+60.f);player.hpPotions--;spawnParticles(player.pos,ParticleT::HEAL,10);}break;
            case sf::Keyboard::Num8:
                if(player.mpPotions>0){player.mp=std::min(player.maxMp,player.mp+50.f);player.mpPotions--;}break;
            case sf::Keyboard::C:showCharacter=!showCharacter;break;
            case sf::Keyboard::Tab:showInventory=!showInventory;break;
            case sf::Keyboard::M:showMap=!showMap;break;
            case sf::Keyboard::E:tryInteract();break;
            default:break;
        }
    }

    void onMouseClick(int mx,int my){
        if(gameState==GameState::LOGIN){
            if(mx>=140&&mx<=420){
                if(my>=270&&my<=310) activeField=0;
                else if(my>=330&&my<=370) activeField=1;
            }
            if(mx>=140&&mx<=420&&my>=390&&my<=430) tryLogin();
            return;
        }
        if(gameState==GameState::CHARACTER_SELECT){
            float slotW=280,gap=30,startX=(WINDOW_W-3*slotW-2*gap)/2;
            for(int i=0;i<3;i++){
                float sx=startX+i*(slotW+gap);
                if(mx>=sx&&mx<=sx+slotW&&my>=90&&my<=550) selectedSlot=i;
            }
            float cx=startX+selectedSlot*(slotW+gap);
            float panelY=550+16;
            for(int c=0;c<4;c++){
                if(mx>=cx+c*66&&mx<=cx+c*66+60&&my>=panelY+18&&my<=panelY+42){
                    selectedClass=c;
                    charSlots[selectedSlot].classIdx=c;
                }
            }
            for(int s=0;s<3;s++){
                if(mx>=cx+s*86&&mx<=cx+s*86+80&&my>=panelY+74&&my<=panelY+98){
                    selectedSkin=s;
                    charSlots[selectedSlot].skinIdx=s;
                }
            }
            nameFocused=(mx>=cx&&mx<=cx+260&&my>=panelY+128&&my<=panelY+158);
            if(mx>=WINDOW_W/2-280&&mx<=WINDOW_W/2-160&&my>=WINDOW_H-60&&my<=WINDOW_H-16)
                gameState=GameState::LOGIN;
            if(mx>=WINDOW_W/2-100&&mx<=WINDOW_W/2+100&&my>=WINDOW_H-60&&my<=WINDOW_H-16)
                enterWorld();
            return;
        }
        sf::Vector2f wp=window.mapPixelToCoords(sf::Vector2i(mx,my),camera);

        // ── Клик по инвентарю ────────────────────────────────────
        if (showInventory) {
            float px = 16.f, py = WINDOW_H - 340.f; // matches drawInventoryPanel
            const float SZ = 48.f, GAP = 6.f;
            float gridX = px + 12.f, gridY = py + 34.f;
            const int COLS = 5;
            // Всего предметов в bag
            int totalItems = (int)player.bag.slots.size();
            for (int i = 0; i < totalItems; i++) {
                int col = i % COLS, row = i / COLS;
                float sx = gridX + col * (SZ + GAP);
                float sy = gridY + row * (SZ + GAP);
                if (mx >= sx && mx <= sx+SZ && my >= sy && my <= sy+SZ) {
                    if (selectedInvSlot == i) {
                        // Повторный клик — надеваем/используем
                        equipItem(i);
                        selectedInvSlot = -1;
                    } else {
                        selectedInvSlot = i;
                    }
                    return;
                }
            }
            // Клик по кнопке "Надеть" под сеткой предметов
            if (selectedInvSlot >= 0 && selectedInvSlot < (int)player.bag.slots.size()) {
                const ItemDef* def = itemSystem.get(player.bag.slots[selectedInvSlot].itemId);
                bool canEquip = def && def->type != "consumable" && def->type != "material" && def->type != "quest";
                if (canEquip) {
                    // Вычисляем позицию кнопки (та же логика что в drawInventoryPanel)
                    const int ROWS2 = 4;
                    float infoY2 = gridY + ROWS2 * (SZ + GAP) + 4.f;
                    // Пропускаем имя (16) + описание (14) + статы (12 каждый)
                    infoY2 += 16.f; // имя
                    if (!def->description.empty()) infoY2 += 14.f;
                    for (auto& [k,v] : def->stats) if (v != 0.f) infoY2 += 12.f;
                    float bx = px + 12, by = infoY2, bw = 100.f, bh = 22.f;
                    if (mx >= bx && mx <= bx+bw && my >= by && my <= by+bh) {
                        equipItem(selectedInvSlot);
                        selectedInvSlot = -1;
                        return;
                    }
                }
            }
            // Клик вне слотов — сбрасываем выделение
            float panelX2 = px + 320.f, panelY2 = WINDOW_H - 16.f;
            if (mx >= px && mx <= panelX2 && my >= py && my <= panelY2) {
                selectedInvSlot = -1;
                return;
            }
        }

        // ── Клик по окну персонажа — снять вещь ─────────────────
        if (showCharacter) {
            const float CW = 620.f, CH = 520.f;
            const float CX2 = WINDOW_W / 2.f - CW / 2.f;
            const float CY2 = WINDOW_H / 2.f - CH / 2.f;
            const float SS2 = 54.f, SG2 = 8.f;
            const float LC2 = CX2 + 12.f;
            const float RC2 = CX2 + CW - 12.f - SS2;

            struct SlotPos { const char* slot; float x, y; };
            SlotPos spos[] = {
                {"main_hand", LC2, CY2 + 58.f},
                {"ring1",     LC2, CY2 + 58.f + (SS2+SG2)},
                {"ring2",     LC2, CY2 + 58.f + (SS2+SG2)*2},
                {"legs",      LC2, CY2 + 58.f + (SS2+SG2)*3},
                {"off_hand",  RC2, CY2 + 58.f},
                {"head",      RC2, CY2 + 58.f + (SS2+SG2)},
                {"chest",     RC2, CY2 + 58.f + (SS2+SG2)*2},
                {"neck",      RC2, CY2 + 58.f + (SS2+SG2)*3},
            };
            for (auto& sp : spos) {
                if (mx >= sp.x && mx <= sp.x+SS2 && my >= sp.y && my <= sp.y+SS2) {
                    unequipSlot(sp.slot);
                    return;
                }
            }
        }

        for(auto& e:enemies){
            if(e.hp<=0) continue;
            if(dist({wp.x,wp.y},e.pos)<24.f){targetEnemy=&e;return;}
        }
        targetEnemy=nullptr;
    }

    void update(float dt){
        gameTime+=dt;
        updateSceneTransition(dt);
        updatePlayer(dt);
        updateEnemies(dt);
        updateParticles(dt);
        updateFloatTexts(dt);
        if (dialogTimer>0.f) dialogTimer-=dt;
        if (questNotifyTimer>0.f) questNotifyTimer-=dt;

        // ── Этап 1: Обработка смерти врагов (XP/gold/квесты) ─────
        for (auto& e : enemies) {
            if (e.hp <= 0 && !e.deathProcessed) {
                e.deathProcessed = true;
                onEnemyKilled(e);
            }
        }

        // ── Триггеры: автоматические (ENTER/LEAVE/TIMER) ──────────
        triggerSystem.update(player.pos.x, player.pos.y, dt, player.level);
        triggerSystem.updateCooldowns(dt);

        // ── Этап 3: Лут ───────────────────────────────────────────
        updateLoot(dt);
        enemies.erase(std::remove_if(enemies.begin(),enemies.end(),
            [this](const Enemy& e){
                bool dead = e.hp<=0 && e.state!=EnemyState::IDLE;
                if (dead) {
                    auto ents = entitySystem.getByType(EntityType::ENEMY);
                    for (auto* en : ents) {
                        float dx = en->x - e.pos.x, dy = en->y - e.pos.y;
                        if (dx*dx+dy*dy < 4.f) { entitySystem.deactivate(en->id); break; }
                    }
                }
                return dead;
            }),enemies.end());
        entitySystem.purgeInactive();

        // ── Редмап Приоритет 1: flush EventBus ───────────────────
        eventBus.flush();
    }

    void updatePlayer(float dt){
    V2 dir;
    bool wasMoving = player.moving;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)) dir.y -= 1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)) dir.y += 1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)) dir.x -= 1;
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)) dir.x += 1;

    // Сохраняем сырой X ДО нормализации — нужен для facing
    float rawDirX = dir.x;

    player.moving = (dir.len() > 0);
    if(player.moving){
        V2 newPos = player.pos + dir.norm() * player.speed * dt;
        int tx = (int)(newPos.x / TILE), ty = (int)(newPos.y / TILE);
        if(tx >= 0 && ty >= 0 && tx < MAP_W && ty < MAP_H && worldMap[ty][tx].walkable)
            player.pos = newPos;
        player.animTime += dt;
    }

    if(player.currentAttackCD > 0) player.currentAttackCD -= dt;
    if(player.comboTimer > 0)      player.comboTimer -= dt;
    else                           player.combo = 0;

    for(auto& s : player.skills)
        if(s.currentCooldown > 0) s.currentCooldown -= dt;

    // SkillSystem cooldown update
    skillSystem.update(player.skillInstances, dt);

    // Пассивная регенерация
    player.hp = std::min(player.maxHp, player.hp + 3.f * dt);
    player.mp = std::min(player.maxMp, player.mp + 2.f * dt);

    camera.setCenter(player.pos.x, player.pos.y);
    entitySystem.setPosition(playerEntityId, player.pos.x, player.pos.y);

    if (player.animPlayer) {
        // AnimStateMachine управляет переходами
        if (player.moving)
            player.stateMachine.setState(AnimState::RUN);
        else
            player.stateMachine.setState(AnimState::IDLE);

        player.stateMachine.updateFacingFromVelocity(rawDirX);
        player.stateMachine.update(dt);
        player.stateMachine.setPosition(player.pos.x, player.pos.y);
    }
}

    void updateEnemies(float dt){
    for(auto& e : enemies){
        if(e.hp <= 0) continue;
        e.animTime += dt;

        // ── Статус-эффекты ────────────────────────────────────────
        e.stunTimer  = 0.f;
        e.slowFactor = 1.f;
        float poisonDmg = 0.f;
        for (auto it = e.statuses.begin(); it != e.statuses.end(); ) {
            it->duration -= dt;
            switch (it->type) {
                case StatusType::STUN:
                    e.stunTimer = std::max(e.stunTimer, it->duration);
                    break;
                case StatusType::SLOW:
                    e.slowFactor = std::min(e.slowFactor, 1.f - it->potency);
                    break;
                case StatusType::POISON:
                case StatusType::BURN:
                    it->tickTimer -= dt;
                    if (it->tickTimer <= 0.f) {
                        it->tickTimer = 0.5f;
                        poisonDmg += it->potency * 0.5f;
                    }
                    break;
                default: break;
            }
            if (it->duration <= 0.f) it = e.statuses.erase(it);
            else ++it;
        }
        if (poisonDmg > 0.f) {
            e.hp -= poisonDmg;
            floatingTexts.push_back({e.pos + V2(0, -16),
                "-" + std::to_string((int)poisonDmg), 1.f,
                sf::Color(80,200,60), false});
            if (e.hp <= 0.f && !e.deathProcessed) { e.deathProcessed=true; onEnemyKilled(e); }
        }
        if (e.hp <= 0) continue;

        // ── AISystem update ───────────────────────────────────────
        if (!e.isStunned()) {
            // Применяем замедление к скоростям
            float savedAggroSpeed  = e.ai.params.aggroSpeed;
            float savedPatrolSpeed = e.ai.params.patrolSpeed;
            float savedCombatSpeed = e.ai.params.combatSpeed;
            e.ai.params.aggroSpeed  *= e.slowFactor;
            e.ai.params.patrolSpeed *= e.slowFactor;
            e.ai.params.combatSpeed *= e.slowFactor;

            aiSystem.update(e.ai, e.pos.x, e.pos.y,
                            player.pos.x, player.pos.y,
                            e.hp, e.maxHp, dt);

            e.ai.params.aggroSpeed  = savedAggroSpeed;
            e.ai.params.patrolSpeed = savedPatrolSpeed;
            e.ai.params.combatSpeed = savedCombatSpeed;

            // Применяем движение
            e.pos.x += e.ai.velX * dt;
            e.pos.y += e.ai.velY * dt;

            // Проверяем walkable
            int tx = (int)(e.pos.x / TILE), ty = (int)(e.pos.y / TILE);
            if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H ||
                !worldMap[ty][tx].walkable) {
                e.pos.x -= e.ai.velX * dt;
                e.pos.y -= e.ai.velY * dt;
            }

            // AI атака → наносим урон игроку
            if (e.ai.shouldAttack) {
                int dmg = std::max(1, (int)(e.damage * (0.8f + (rand()%40)*0.01f)));
                // Митигация урона базовой защитой
                dmg = std::max(1, dmg - player.baseDef / 2);
                player.hp -= (float)dmg;
                player.hp  = std::max(0.f, player.hp);
                spawnParticles(player.pos, ParticleT::BLOOD, 6);
                floatingTexts.push_back({player.pos + V2(0,-30),
                    "-" + std::to_string(dmg), 1.2f, sf::Color(220,80,60), false});
                if (audioManager) audioManager->playSound("damage");
            }

            // Синхронизируем EnemyState из AIState (для анимации)
            switch (e.ai.state) {
                case AIState::IDLE:    e.state = EnemyState::IDLE;    break;
                case AIState::PATROL:  e.state = EnemyState::PATROL;  break;
                case AIState::AGGRO:   e.state = EnemyState::AGGRO;   break;
                case AIState::COMBAT:  e.state = EnemyState::COMBAT;  break;
                case AIState::RETURN:  e.state = EnemyState::RETURN;  break;
                default: break;
            }
        }

        // Агрро от попадания (принудительно переключаем AI)
        if (e.aggroedByHit && e.ai.state != AIState::DEAD) {
            AISystem::forceAggro(e.ai);
            e.aggroedByHit = false;
        }

        // ── Анимации через StateMachine ───────────────────────────
        if(e.animPlayer){
            bool isMoving = (e.state == EnemyState::PATROL ||
                             e.state == EnemyState::AGGRO  ||
                             e.state == EnemyState::RETURN);
            if (e.ai.velX != 0.f || e.ai.velY != 0.f)
                e.stateMachine.updateFacingFromVelocity(e.ai.velX);
            e.stateMachine.setState(isMoving ? AnimState::WALK : AnimState::IDLE);
            e.stateMachine.update(dt);
            e.stateMachine.setPosition(e.pos.x, e.pos.y);
        }
    }
}

    void updateParticles(float dt){
        for(auto& p:particles){
            p.pos=p.pos+p.vel*dt; p.life-=dt;
            p.vel.y+=300*dt;
        }
        particles.erase(std::remove_if(particles.begin(),particles.end(),
            [](const Particle& p){return p.life<=0;}),particles.end());
    }

    void updateFloatTexts(float dt){
        for(auto& ft:floatingTexts){
            ft.pos.y-=60*dt;
            ft.life-=dt;
        }
        floatingTexts.erase(std::remove_if(floatingTexts.begin(),floatingTexts.end(),
            [](const FloatingText& ft){return ft.life<=0;}),floatingTexts.end());
    }

    void useSkill(int idx){
        if(idx<0||idx>=(int)player.skills.size()) return;
        auto& skill=player.skills[idx];
        if(skill.currentCooldown>0||player.mp<skill.manaCost) return;
        player.mp-=skill.manaCost;
        skill.currentCooldown=skill.cooldown;

        // ── Пробуем через SkillSystem (если скилл там зарегистрирован) ──
        // Маппинг legacy skill names → skillSystem IDs
        static const std::map<std::string,std::string> skillIdMap = {
            {"Slash","slash"},{"Fireball","fireball"},
            {"Shield","shield"},{"Heal","heal"}
        };
        auto sit = skillIdMap.find(skill.name);
        if (sit != skillIdMap.end()) {
            if (skill.name == "Heal") {
                // Особая обработка хила
                float healAmt = 20.f + player.intel * 2.f + player.level * 5.f;
                player.hp = std::min(player.maxHp, player.hp + healAmt);
                spawnParticles(player.pos, ParticleT::HEAL, 12);
                floatingTexts.push_back({player.pos+V2(0,-40),
                    "+" + std::to_string((int)healAmt) + " HP",
                    2.f, sf::Color(80,240,100), false});
                if (audioManager) audioManager->playSound("heal");
                player.stateMachine.setAttack();
                return;
            }
            if (skill.name == "Shield") {
                player.baseDef += 5;
                floatingTexts.push_back({player.pos+V2(0,-40),
                    "Щит! +5 DEF", 2.f, sf::Color(100,180,255), false});
                // Убираем через 4 сек (простая версия)
                spawnParticles(player.pos, ParticleT::MAGIC, 8);
                player.stateMachine.setAttack();
                return;
            }

            // Атакующие скиллы
            if (targetEnemy && targetEnemy->hp > 0) {
                CasterStats cs;
                cs.STR = (float)player.str; cs.DEX = (float)player.agi;
                cs.INT = (float)player.intel; cs.VIT = (float)player.vit;
                cs.level = (float)player.level;
                cs.critChance = 0.05f + player.agi * 0.005f;

                auto result = skillSystem.useById(player.skillInstances, sit->second, cs, player.mp);
                // Если skillSystem не знает этот скилл — fallback
                if (!result.success) {
                    int fdmg = skill.damage + player.str * 2 + player.level * 2 + rand()%10 - 5;
                    result.success = true; result.damage = (float)fdmg;
                    result.isCrit  = (rand()%100 < 5);
                }

                int dmg = result.damage > 0 ? (int)result.damage
                                            : (skill.damage + rand()%10 - 5);
                // Бонус к урону от уровня
                dmg += player.level * 2;
                if (result.isCrit) dmg = (int)(dmg * 1.5f);

                targetEnemy->hp -= (float)dmg;
                targetEnemy->aggroedByHit = true;
                AISystem::forceAggro(targetEnemy->ai);

                // Применяем эффекты из SkillSystem
                for (auto& ef : result.appliedEffects) {
                    if (ef.type == "stun")
                        targetEnemy->applyStatus(StatusType::STUN, ef.duration, 0.f);
                    else if (ef.type == "poison")
                        targetEnemy->applyStatus(StatusType::POISON, ef.duration, ef.potency);
                    else if (ef.type == "slow")
                        targetEnemy->applyStatus(StatusType::SLOW, ef.duration, ef.potency);
                    else if (ef.type == "burn")
                        targetEnemy->applyStatus(StatusType::BURN, ef.duration, ef.potency);
                }

                // Визуал
                sf::Color dmgCol = result.isCrit ? sf::Color(255,200,30)
                    : (skill.name=="Fireball" ? sf::Color(255,120,40) : sf::Color(200,50,50));
                std::string dmgStr = (result.isCrit ? "КРИТ! " : "") + std::to_string(dmg);
                floatingTexts.push_back({targetEnemy->pos+V2(0,-30),
                    dmgStr, 1.8f, dmgCol, result.isCrit});
                spawnParticles(targetEnemy->pos,
                    skill.name=="Fireball" ? ParticleT::MAGIC : ParticleT::BLOOD, 8);

                // Звук
                if (audioManager) {
                    if      (skill.name=="Fireball") audioManager->playSound("fireball");
                    else if (result.isCrit)          audioManager->playSound("critical");
                    else                             audioManager->playSound("slash");
                }

                // EventBus
                SkillUsedEvent sv;
                sv.skillId = sit->second; sv.casterId = "player";
                sv.targetX = targetEnemy->pos.x; sv.targetY = targetEnemy->pos.y;
                sv.damage = dmg;
                eventBus.emit(sv);
            }
        } else {
            // Legacy fallback
            if(targetEnemy&&targetEnemy->hp>0){
                int dmg=skill.damage+(rand()%10)-5;
                targetEnemy->hp-=dmg;
                targetEnemy->aggroedByHit=true;
                floatingTexts.push_back({targetEnemy->pos, std::to_string(dmg), 1.5f, sf::Color(200,50,50), false});
                spawnParticles(targetEnemy->pos, ParticleT::MAGIC, 5);
            }
        }
        player.stateMachine.setAttack();
    }

    // ════════════════════════════════════════════════════════════
    // ЭТАП 1: Обработка убийства врага
    // XP, gold, kills, quest progress, level-up check
    // ════════════════════════════════════════════════════════════
    void onEnemyKilled(const Enemy& e) {
        // ── 1. Очки опыта ─────────────────────────────────────────
        int xpGain = 10 + e.level * 5 + (e.boss ? 100 : 0);
        player.xp += xpGain;
        player.kills++;

        // Floating XP text
        floatingTexts.push_back({e.pos + V2(0, -20),
            "+" + std::to_string(xpGain) + " XP", 2.f, sf::Color(120, 220, 120), false});

        // ── 2. Золото ─────────────────────────────────────────────
        int goldGain = e.gold > 0 ? e.gold : (5 + e.level * 3);
        if (e.boss) goldGain *= 3;
        player.gold += goldGain;
        floatingTexts.push_back({e.pos + V2(0, -36),
            "+" + std::to_string(goldGain) + "g", 2.f, sf::Color(255, 210, 60), false});

        // ── 3. Квест-прогресс + лут через EventBus ───────────────
        {
            EnemyKilledEvent ev;
            ev.enemyName  = e.name;
            ev.enemyLevel = e.level;
            ev.posX       = e.pos.x;
            ev.posY       = e.pos.y;
            ev.goldDrop   = goldGain;
            ev.xpReward   = xpGain;
            eventBus.enqueue(ev);  // обработается в flush() в конце update()
        }
        // Прямой quest.onKill (синхронно, для обратной совместимости)
        auto results = questSystem.onKill(e.name);
        for (auto& r : results) {
            if (r.questCompleted) {
                questNotification = "✓ Квест: " + r.questName;
                questNotifyTimer = 6.f;
                spawnParticles(player.pos, ParticleT::HEAL, 25);
            }
        }

        // ── 4. Level-up ───────────────────────────────────────────
        checkLevelUp();

        // ── 5. Этап 3: Лут (золото + предметы из ItemSystem) ─────
        spawnLoot(e.pos, e.level, e.boss);              // золото на земле
        spawnItemLoot(e.pos, e.name, e.level, e.boss);  // предметы на земле
    }

    void checkLevelUp() {
        while (player.xp >= player.xpNext) {
            player.xp -= player.xpNext;
            int oldLv = player.level;
            player.level++;
            player.xpNext = 100 + player.level * 50;
            player.maxHp += 10; player.hp = player.maxHp;
            player.maxMp += 5;  player.mp = player.maxMp;
            player.baseAtk += 2; player.baseDef += 1;
            floatingTexts.push_back({player.pos + V2(0, -50),
                "LEVEL UP! Lv." + std::to_string(player.level),
                3.f, sf::Color(255, 220, 50), true});
            spawnParticles(player.pos, ParticleT::CRITICAL, 30);
            LevelUpEvent ev; ev.oldLevel = oldLv; ev.newLevel = player.level;
            eventBus.emit(ev);
        }
    }

    void spawnParticles(V2 pos, ParticleT type, int count){
        for(int i=0;i<count;i++){
            Particle p;
            p.pos=pos+V2((rand()%20)-10, (rand()%20)-10);
            float angle=2*3.14159f*rand()/RAND_MAX;
            p.vel=V2(std::cos(angle),std::sin(angle))*float(100+rand()%100);
            p.life=0.5f+float(rand()%5)*0.1f;
            p.maxLife=p.life;
            p.size=2+rand()%4;
            if(type==ParticleT::BLOOD)      p.color=sf::Color(200,50,50);
            else if(type==ParticleT::MAGIC) p.color=sf::Color(100,150,255);
            else if(type==ParticleT::HEAL)  p.color=sf::Color(100,255,100);
            else                            p.color=sf::Color(255,200,50);
            particles.push_back(p);
        }
    }

    // ── Этап 3: Лут ───────────────────────────────────────────
    // itemId хранится в LootDrop::name когда isGold==false
    void spawnLoot(V2 pos, int enemyLevel, bool boss) {
        // Золото — всегда (визуальный пикап, в bag не кладём — золото в player.gold)
        {
            LootDrop g;
            g.pos     = pos + V2((float)(rand()%24)-12.f, (float)(rand()%24)-12.f);
            g.isGold  = true;
            g.goldAmt = (5 + enemyLevel * 3) * (boss ? 5 : 1) + rand() % 10;
            g.name    = std::to_string(g.goldAmt) + "g";
            g.rarity  = 0;
            g.life    = 30.f;
            lootDrops.push_back(g);
        }
    }

    // Спавн предметного лута из ItemSystem
    void spawnItemLoot(V2 pos, const std::string& enemyType, int enemyLevel, bool boss) {
        auto drops = itemSystem.rollLoot(enemyType, enemyLevel);

        // Если дроп-таблицы нет — генерируем по шансу из встроенных предметов
        if (drops.empty()) {
            int roll = rand() % 100;
            int threshold = boss ? 10 : std::max(5, 50 - enemyLevel * 3);
            if (roll < threshold) {
                std::vector<std::string> byRarity[4];
                for (auto& [id, def] : itemSystem.all()) {
                    if (def.type == "consumable" || def.type == "material" || def.type == "quest")
                        byRarity[0].push_back(id);
                    else
                        byRarity[std::min(3, (int)def.rarity)].push_back(id);
                }
                int rar = 0;
                if (boss || roll < 5)                  rar = 3;
                else if (roll < 15 || enemyLevel >= 3) rar = 2;
                else if (roll < 30)                    rar = 1;

                for (int r = rar; r >= 0; r--) {
                    if (!byRarity[r].empty()) {
                        const std::string& id = byRarity[r][rand() % byRarity[r].size()];
                        drops.push_back(ItemStack(id, 1));
                        break;
                    }
                }
            }
        }

        for (auto& stack : drops) {
            if (stack.itemId.empty()) continue;
            const ItemDef* def = itemSystem.get(stack.itemId);
            if (!def) continue;

            LootDrop d;
            d.pos     = pos + V2((float)(rand()%32)-16.f, (float)(rand()%32)-16.f);
            d.isGold  = false;
            d.name    = stack.itemId;  // ВАЖНО: itemId, не display-name
            d.rarity  = (int)def->rarity;
            d.life    = 30.f;
            d.goldAmt = stack.count;   // переиспользуем поле для кол-ва
            lootDrops.push_back(d);
        }
    }

    void updateLoot(float dt) {
        for (auto& l : lootDrops) {
            if (l.pickedUp) continue;
            l.life -= dt;
            if (l.life <= 0) { l.pickedUp = true; continue; }
            float dx = player.pos.x - l.pos.x, dy = player.pos.y - l.pos.y;
            if (dx*dx + dy*dy < 24.f*24.f) {
                l.pickedUp = true;
                if (l.isGold) {
                    player.gold += l.goldAmt;
                    floatingTexts.push_back({player.pos+V2(0,-24),
                        "+"+std::to_string(l.goldAmt)+"g", 1.5f, sf::Color(255,215,0), false});
                } else {
                    // l.name == itemId из ItemSystem
                    int qty = l.goldAmt > 0 ? l.goldAmt : 1;
                    bool added = itemSystem.giveToInventory(player.bag, l.name, qty);
                    const ItemDef* def = itemSystem.get(l.name);
                    std::string displayName = def ? def->name : l.name;
                    std::string pickupMsg = (qty > 1)
                        ? displayName + " x" + std::to_string(qty)
                        : displayName;
                    if (!added) pickupMsg = "Inv full!";
                    floatingTexts.push_back({player.pos+V2(0,-24),
                        pickupMsg, 1.8f, rarityColor(l.rarity), false});
                    if (added) {
                        spawnParticles(player.pos, ParticleT::MAGIC, 6);
                        ItemPickupEvent ipev;
                        ipev.itemId   = l.name;
                        ipev.itemName = displayName;
                        ipev.count    = qty;
                        eventBus.enqueue(ipev);
                    }
                }
            }
        }
        lootDrops.erase(std::remove_if(lootDrops.begin(), lootDrops.end(),
            [](const LootDrop& l){ return l.pickedUp; }), lootDrops.end());
    }

    void drawLoot() {
        for (auto& l : lootDrops) {
            float pulse = 0.7f + 0.3f * std::sin(gameTime * 4.f + l.pos.x);
            sf::Color col = rarityColor(l.rarity);

            if (l.isGold) {
                // Монета
                sf::CircleShape coin(6);
                coin.setOrigin(6,6);
                coin.setPosition(l.pos.x, l.pos.y);
                coin.setFillColor(sf::Color(220, 185, 30, uint8_t(200*pulse)));
                coin.setOutlineColor(sf::Color(255,220,60));
                coin.setOutlineThickness(1.f);
                window.draw(coin);
            } else {
                // Предмет — ромб
                sf::ConvexShape gem;
                gem.setPointCount(4);
                float s = 7.f + l.rarity * 2.f;
                gem.setPoint(0, {0, -s});
                gem.setPoint(1, {s*0.6f, 0});
                gem.setPoint(2, {0, s});
                gem.setPoint(3, {-s*0.6f, 0});
                gem.setOrigin(0, 0);
                gem.setPosition(l.pos.x, l.pos.y);
                col.a = uint8_t(180 * pulse);
                gem.setFillColor(col);
                gem.setOutlineColor(sf::Color(255,255,255,100));
                gem.setOutlineThickness(1.f);
                window.draw(gem);
            }
        }
    }

    void tryLogin(){ gameState=GameState::CHARACTER_SELECT; }
    void enterWorld() {
        // Применяем выбранный класс и имя персонажа
        const char* classNames[] = {"Warrior", "Mage", "Rogue", "Paladin"};
        int ci = charSlots[selectedSlot].classIdx;
        if (ci >= 0 && ci < 4) player.className = classNames[ci];
        if (!charSlots[selectedSlot].name.empty())
            player.name = charSlots[selectedSlot].name;
        // Обновляем entitySystem
        if (auto* e = entitySystem.getEntity(playerEntityId)) {
            e->name = player.name;
            e->props.setStr("class", player.className);
        }
        gameState = GameState::PLAYING;
    }

    // =========================================================
    void render(){
        if(gameState==GameState::LOGIN){
            renderLoginScreen(); window.display(); return;
        }
        if(gameState==GameState::CHARACTER_SELECT){
            renderCharSelect(); window.display(); return;
        }
        window.clear(gameConfig.ambientColor);
        window.setView(camera);
        if(gameConfig.layerGround)   drawMap();
        if(gameConfig.layerObjects)  drawEntities();
        drawTriggerZones();
        drawLoot();           // Этап 3: лут лежит под ногами
        drawEnemies();
        if(gameConfig.layerEffects)  drawParticles();
        if(gameConfig.layerEntities) drawPlayer();
        drawFloatTexts();
        window.setView(window.getDefaultView());
        drawHUD();
        drawSceneTransition();
        if(dialogTimer>0.f) drawDialog();
        if(showInventory) drawInventoryPanel();
        if(showCharacter) drawCharacterPanel();
        if(showMap)       drawMinimap();
        window.display();
    }

    void drawMap(){
        sf::Vector2f cc=camera.getCenter();
        int sx=std::max(0,(int)((cc.x-WINDOW_W/2)/TILE)-2);
        int sy=std::max(0,(int)((cc.y-WINDOW_H/2)/TILE)-2);
        int ex=std::min(MAP_W,(int)((cc.x+WINDOW_W/2)/TILE)+3);
        int ey=std::min(MAP_H,(int)((cc.y+WINDOW_H/2)/TILE)+3);
        sf::RectangleShape tile({(float)TILE,(float)TILE});

        for(int y=sy;y<ey;y++){
            for(int x=sx;x<ex;x++){
                auto& t = worldMap[y][x];
                float wx=x*TILE, wy=y*TILE;
                tile.setPosition(wx,wy);

                // ── Кастомный тайл (текстура из custom_tiles.json) ───────
                // Проверяем ДО switch по типу — кастомные не нужно конвертировать
                if (!t.tileId.empty()) {
                    auto cit = customTileSprite.find(t.tileId);
                    if (cit != customTileSprite.end()) {
                        auto& sp = cit->second;
                        auto tr  = sp.getTextureRect();
                        sp.setPosition(wx, wy);
                        sp.setScale(
                            (float)TILE / std::max(1, tr.width),
                            (float)TILE / std::max(1, tr.height));
                        window.draw(sp);
                    } else {
                        // Текстура не загрузилась — рисуем цвет-заглушку из custom_tiles.json
                        tile.setFillColor(t.color);
                        tile.setOutlineThickness(0);
                        window.draw(tile);
                    }
                    continue;  // переходим к следующему тайлу
                }

                // Преобразование SceneTileType в TileType для визуализации
                TileType tt;
                switch(t.type){
                    case SceneTileType::GRASS:          tt = TileType::GRASS; break;
                    case SceneTileType::DIRT:           tt = TileType::DIRT; break;
                    case SceneTileType::STONE_FLOOR:    tt = TileType::STONE_FLOOR; break;
                    case SceneTileType::ROAD:           tt = TileType::ROAD; break;
                    case SceneTileType::WALL:           tt = TileType::WALL; break;
                    case SceneTileType::WATER:          tt = TileType::WATER; break;
                    case SceneTileType::TREE:           tt = TileType::TREE; break;
                    case SceneTileType::BUILDING_FLOOR: tt = TileType::BUILDING_FLOOR; break;
                    case SceneTileType::FOUNTAIN:       tt = TileType::FOUNTAIN; break;
                    case SceneTileType::ROOF_RED:       tt = TileType::ROOF_RED; break;
                    case SceneTileType::ROOF_BLUE:      tt = TileType::ROOF_BLUE; break;
                    case SceneTileType::ROOF_GREEN:     tt = TileType::ROOF_GREEN; break;
                    default:                            tt = TileType::GRASS; break;
                }

                switch(tt){
                case TileType::GRASS:{
                    sf::Color g = t.color;
                    int v=(x*3+y*7+t.variant*31)%8-4;
                    g.g=std::clamp(g.g+v,0,255);
                    tile.setFillColor(g); tile.setOutlineThickness(0); window.draw(tile);
                    if((x+y)%4==0){
                        sf::RectangleShape blade({2,5});
                        blade.setFillColor(sf::Color(70,170,55));
                        blade.setPosition(wx+6,wy+18); window.draw(blade);
                        blade.setPosition(wx+18,wy+12); window.draw(blade);
                        blade.setPosition(wx+25,wy+20); window.draw(blade);
                    }
                    break;}
                case TileType::STONE_FLOOR:{
                    tile.setFillColor(sf::Color(130,122,110)); window.draw(tile);
                    sf::RectangleShape line({(float)TILE,1});
                    line.setFillColor(sf::Color(110,100,90,120));
                    line.setPosition(wx,wy+TILE/2); window.draw(line);
                    sf::RectangleShape line2({1,(float)TILE});
                    line2.setFillColor(sf::Color(110,100,90,120));
                    int offset=(y%2==0)?0:TILE/2;
                    line2.setPosition(wx+offset,wy); window.draw(line2);
                    break;}
                case TileType::ROAD:{
                    tile.setFillColor(sf::Color(170,158,138)); window.draw(tile);
                    sf::RectangleShape edge({(float)TILE,2});
                    edge.setFillColor(sf::Color(145,133,113,150));
                    edge.setPosition(wx,wy); window.draw(edge);
                    edge.setPosition(wx,wy+TILE-2); window.draw(edge);
                    break;}
                case TileType::WALL:{
                    sf::Color wc=t.variant==1?sf::Color(90,75,60):sf::Color(120,108,95);
                    tile.setFillColor(wc); window.draw(tile);
                    sf::RectangleShape hi({(float)TILE,4});
                    hi.setFillColor(sf::Color(160,148,130,140));
                    hi.setPosition(wx,wy); window.draw(hi);
                    sf::RectangleShape j({(float)TILE,1});
                    j.setFillColor(sf::Color(80,65,50,160));
                    j.setPosition(wx,wy+TILE/3); window.draw(j);
                    j.setPosition(wx,wy+TILE*2/3); window.draw(j);
                    sf::RectangleShape jv({1,(float)TILE/3});
                    jv.setFillColor(sf::Color(80,65,50,140));
                    int boff=(y%2==0)?TILE/4:TILE*3/4;
                    jv.setPosition(wx+boff,wy); window.draw(jv);
                    jv.setPosition(wx+boff,wy+TILE/3); window.draw(jv);
                    break;}
                case TileType::WATER:{
                    float wave=std::sin(gameTime*2+(x+y)*0.4f)*0.12f;
                    sf::Color wc(int(40+wave*20),int(110+wave*30),int(200+wave*20));
                    tile.setFillColor(wc); window.draw(tile);
                    float sw=std::sin(gameTime*3+x*0.7f);
                    if(sw>0.6f){
                        sf::RectangleShape shim({8,2});
                        shim.setFillColor(sf::Color(180,230,255,int(sw*150)));
                        shim.setPosition(wx+4+sw*6,wy+TILE/2+std::sin(gameTime+y)*4);
                        window.draw(shim);
                    }
                    break;}
                case TileType::TREE:
                    tile.setFillColor(sf::Color(40,90,30)); window.draw(tile);
                    drawChibiTree(window,wx,wy,t.variant);
                    break;
                case TileType::BUILDING_FLOOR:
                    tile.setFillColor(sf::Color(100,90,78)); window.draw(tile);
                    break;
                case TileType::FOUNTAIN:
                    tile.setFillColor(sf::Color(120,115,105)); window.draw(tile);
                    break;
                default:
                    // Неизвестный встроенный тип — цвет из SceneTile
                    tile.setFillColor(t.color); window.draw(tile);
                    break;
                }
            }
        }

        // Draw buildings
        for(auto& b:buildings){
            float wx=b.tx*TILE, wy=b.ty*TILE;
            if(std::abs(wx-cc.x)<WINDOW_W&&std::abs(wy-cc.y)<WINDOW_H)
                drawBuilding(window,wx,wy,b.bw,b.bh,b.roofCol,b.variant);
        }

        // Draw fountains
        for(auto& f:fountains){
            float wx=f.x*TILE, wy=f.y*TILE;
            if(std::abs(wx-cc.x)<WINDOW_W&&std::abs(wy-cc.y)<WINDOW_H)
                drawFountain(window,wx,wy,gameTime);
        }

        // City name sign
        if(fontLoaded){
            sf::Text label; label.setFont(font);
            label.setString(U("~ Aethoria City ~"));
            label.setCharacterSize(15);
            label.setFillColor(sf::Color(240,220,160,200));
            label.setStyle(sf::Text::Bold);
            auto lb=label.getLocalBounds();
            sf::RectangleShape sign({lb.width+20,26});
            sign.setFillColor(sf::Color(60,40,20,200));
            sign.setOutlineColor(sf::Color(180,140,80));
            sign.setOutlineThickness(2);
            sign.setPosition(CITY_CX*TILE-lb.width/2-10,(CITY_CY-CITY_RADIUS-3)*TILE-4);
            window.draw(sign);
            label.setPosition(CITY_CX*TILE-lb.width/2,(CITY_CY-CITY_RADIUS-3)*TILE);
            window.draw(label);
        }
    }

    void drawEntities() {
        sf::Vector2f cc = camera.getCenter();
        float cullW = WINDOW_W / 2.f + TILE * 2;
        float cullH = WINDOW_H / 2.f + TILE * 2;
        for (auto& e : entitySystem.getAll()) {
            if (!e.active) continue;
            if (e.type == EntityType::ENEMY || e.type == EntityType::PLAYER) continue;
            if (std::abs(e.x - cc.x) > cullW || std::abs(e.y - cc.y) > cullH) continue;
            float wx = e.x, wy = e.y;
            float r = (float)TILE * 0.4f;
            sf::CircleShape shadow(r * 0.7f, 16);
            shadow.setOrigin(r*0.7f, r*0.35f);
            shadow.setScale(1.f, 0.3f);
            shadow.setPosition(wx, wy + r * 0.5f);
            shadow.setFillColor(sf::Color(0,0,0,50));
            window.draw(shadow);
            switch (e.type) {
            case EntityType::NPC: {
                sf::CircleShape body(r, 20);
                body.setOrigin(r, r);
                body.setPosition(wx, wy);
                std::string sub = e.props.getStr("subtype","");
                sf::Color nc(80,140,220);
                if(sub=="VENDOR")     nc=sf::Color(200,160,40);
                else if(sub=="QUEST") nc=sf::Color(220,60,60);
                else if(sub=="HEALER")nc=sf::Color(60,200,120);
                else if(sub=="GUARD") nc=sf::Color(80,80,200);
                else if(sub=="BLACKSMITH")nc=sf::Color(120,120,130);
                body.setFillColor(nc);
                body.setOutlineColor(sf::Color(255,255,255,180));
                body.setOutlineThickness(2);
                window.draw(body);
                sf::CircleShape head(r*0.55f, 16);
                head.setOrigin(r*0.55f, r*0.55f);
                head.setPosition(wx, wy - r*1.1f);
                head.setFillColor(sf::Color(255,220,180));
                head.setOutlineColor(sf::Color(180,140,100));
                head.setOutlineThickness(1);
                window.draw(head);
                if(sub=="QUEST" && fontLoaded) {
                    sf::Text mark; mark.setFont(font);
                    mark.setString("!");
                    mark.setCharacterSize(18);
                    mark.setFillColor(sf::Color(255,220,0));
                    mark.setStyle(sf::Text::Bold);
                    auto b=mark.getLocalBounds();
                    mark.setOrigin(b.width/2,b.height/2);
                    mark.setPosition(wx, wy-r*2.2f);
                    window.draw(mark);
                }
                if(fontLoaded) {
                    sf::Text nm; nm.setFont(font);
                    nm.setString(U(e.name));
                    nm.setCharacterSize(11);
                    nm.setFillColor(sf::Color(240,230,180,230));
                    auto b=nm.getLocalBounds();
                    nm.setOrigin(b.width/2, b.height);
                    nm.setPosition(wx, wy - r*1.9f - 2);
                    window.draw(nm);
                }
                float dx=player.pos.x-wx, dy=player.pos.y-wy;
                if(dx*dx+dy*dy < interactRange*interactRange) {
                    sf::CircleShape ring(r+8,20);
                    ring.setOrigin(r+8,r+8);
                    ring.setPosition(wx,wy);
                    ring.setFillColor(sf::Color::Transparent);
                    ring.setOutlineColor(sf::Color(255,220,60,
                        uint8_t(180+70*std::sin(gameTime*4))));
                    ring.setOutlineThickness(2);
                    window.draw(ring);
                    if(fontLoaded) {
                        sf::Text hint; hint.setFont(font);
                        hint.setString(U("[E]"));
                        hint.setCharacterSize(12);
                        hint.setFillColor(sf::Color(255,220,60));
                        auto b=hint.getLocalBounds();
                        hint.setOrigin(b.width/2,b.height);
                        hint.setPosition(wx, wy-r*2.5f);
                        window.draw(hint);
                    }
                }
                break;
            }
            case EntityType::OBJECT: {
                std::string sub = e.props.getStr("subtype","chest");
                bool opened = e.props.getBool("opened",false);
                sf::Color oc = opened ? sf::Color(80,60,30) : sf::Color(180,130,40);
                sf::RectangleShape box({r*1.6f, r*1.2f});
                box.setOrigin(r*0.8f, r*0.6f);
                box.setPosition(wx, wy);
                box.setFillColor(oc);
                box.setOutlineColor(sf::Color(240,200,80));
                box.setOutlineThickness(opened?0:2);
                window.draw(box);
                if(!opened) {
                    sf::CircleShape lock(r*0.18f,8);
                    lock.setOrigin(r*0.18f,r*0.18f);
                    lock.setPosition(wx, wy-r*0.05f);
                    lock.setFillColor(sf::Color(240,210,60));
                    window.draw(lock);
                }
                float dx=player.pos.x-wx, dy=player.pos.y-wy;
                if(!opened && dx*dx+dy*dy < interactRange*interactRange && fontLoaded) {
                    sf::Text hint; hint.setFont(font);
                    hint.setString(U("[E] Открыть"));
                    hint.setCharacterSize(11);
                    hint.setFillColor(sf::Color(255,220,60));
                    auto b=hint.getLocalBounds();
                    hint.setOrigin(b.width/2,b.height);
                    hint.setPosition(wx,wy-r*1.8f);
                    window.draw(hint);
                }
                break;
            }
            case EntityType::PORTAL: {
                float pulse  = 0.85f + 0.15f * std::sin(gameTime * 3.f);
                float spin   = gameTime * 1.5f;
                // Внешнее кольцо — пульсирующее
                sf::CircleShape outer(r * 1.3f * pulse, 30);
                outer.setOrigin(r*1.3f*pulse, r*1.3f*pulse);
                outer.setPosition(wx, wy);
                outer.setFillColor(sf::Color(120, 40, 220,
                    uint8_t(120 + 50*std::sin(gameTime*2))));
                outer.setOutlineColor(sf::Color(200, 140, 255, 200));
                outer.setOutlineThickness(3);
                window.draw(outer);
                // Внутреннее ядро
                sf::CircleShape inner(r * 0.6f * pulse, 20);
                inner.setOrigin(r*0.6f*pulse, r*0.6f*pulse);
                inner.setPosition(wx, wy);
                inner.setFillColor(sf::Color(220, 180, 255,
                    uint8_t(180 + 60*std::sin(gameTime*5))));
                window.draw(inner);
                // Вращающиеся орбитальные точки
                for (int oi = 0; oi < 4; oi++) {
                    float a = spin + oi * 3.14159f * 0.5f;
                    float ox = std::cos(a) * r * 1.5f;
                    float oy = std::sin(a) * r * 0.6f;  // приплюснуто — эллипс
                    sf::CircleShape orb(3, 8);
                    orb.setOrigin(3,3);
                    orb.setPosition(wx + ox, wy + oy);
                    orb.setFillColor(sf::Color(220, 160, 255,
                        uint8_t(160 + 60*std::sin(gameTime*3 + oi))));
                    window.draw(orb);
                }
                // Метка из portal_links или target_zone
                if (fontLoaded) {
                    std::string pid  = e.props.getStr("portal_id", e.name);
                    const PortalLink* link = sceneManager.getLinkByPortalId(pid);
                    std::string dest = link ? link->label
                                           : e.props.getStr("target_zone","");
                    if (dest.empty()) dest = "Портал";
                    sf::Text lbl; lbl.setFont(font);
                    lbl.setString(U(dest));
                    lbl.setCharacterSize(11);
                    lbl.setFillColor(sf::Color(220, 180, 255, 220));
                    auto b = lbl.getLocalBounds();
                    lbl.setOrigin(b.width/2, b.height);
                    lbl.setPosition(wx, wy - r*1.9f);
                    window.draw(lbl);
                }
                // Подсказка [E] при близости
                float dx = player.pos.x - wx, dy = player.pos.y - wy;
                if (dx*dx + dy*dy < interactRange*interactRange && fontLoaded) {
                    sf::Text hint; hint.setFont(font);
                    hint.setString("[E]");
                    hint.setCharacterSize(13);
                    hint.setFillColor(sf::Color(255, 220, 60,
                        uint8_t(200 + 55*std::sin(gameTime*5))));
                    hint.setStyle(sf::Text::Bold);
                    auto b = hint.getLocalBounds();
                    hint.setOrigin(b.width/2, b.height);
                    hint.setPosition(wx, wy - r*2.6f);
                    window.draw(hint);
                }
                break;
            }
            case EntityType::ITEM_DROP: {
                float bob = std::sin(gameTime*4.f+wx*0.1f)*3.f;
                sf::CircleShape gem(r*0.5f,6);
                gem.setOrigin(r*0.5f,r*0.5f);
                gem.setPosition(wx,wy+bob);
                gem.setFillColor(sf::Color(255,220,60));
                gem.setOutlineColor(sf::Color(200,160,20));
                gem.setOutlineThickness(2);
                window.draw(gem);
                float dx=player.pos.x-wx, dy=player.pos.y-wy;
                if(dx*dx+dy*dy < interactRange*interactRange && fontLoaded) {
                    sf::Text hint; hint.setFont(font);
                    hint.setString(U("[E] "+e.name));
                    hint.setCharacterSize(11);
                    hint.setFillColor(sf::Color(255,220,60));
                    auto b=hint.getLocalBounds();
                    hint.setOrigin(b.width/2,b.height);
                    hint.setPosition(wx,wy-r*1.5f+bob);
                    window.draw(hint);
                }
                break;
            }
            default: break;
            }
        }
    }

    // Визуализация триггерных зон (только не-портальные, полупрозрачно)
    void drawTriggerZones() {
        sf::Vector2f cc = camera.getCenter();
        float cullW = WINDOW_W / 2.f + 128.f;
        float cullH = WINDOW_H / 2.f + 128.f;
        for (auto& t : triggerSystem.all()) {
            if (!t.active) continue;
            // Порталы уже рисуются как entity — не дублируем
            if (t.id.size() >= 7 && t.id.substr(0,7) == "portal_") continue;

            float tx = t.x, ty = t.y;
            if (std::abs(tx - cc.x) > cullW || std::abs(ty - cc.y) > cullH) continue;

            // Цвет по типу действия
            sf::Color zoneColor;
            switch (t.action) {
                case TriggerAction::SCENE_SWITCH:   zoneColor = sf::Color(100,200,255,35); break;
                case TriggerAction::SCENE_INSTANCE: zoneColor = sf::Color(255,100,50,35);  break;
                case TriggerAction::DIALOGUE:       zoneColor = sf::Color(255,220,60,30);  break;
                default:                            zoneColor = sf::Color(180,180,255,25); break;
            }
            sf::Color borderColor = zoneColor;
            borderColor.a = uint8_t(60 + 30*std::sin(gameTime*2.5f));

            if (t.shape == TriggerShape::CIRCLE) {
                float r = t.radius;
                sf::CircleShape zone(r, 24);
                zone.setOrigin(r, r);
                zone.setPosition(tx, ty);
                zone.setFillColor(zoneColor);
                zone.setOutlineColor(borderColor);
                zone.setOutlineThickness(1.5f);
                window.draw(zone);
            } else {
                sf::RectangleShape zone({t.w, t.h});
                zone.setOrigin(t.w/2.f, t.h/2.f);
                zone.setPosition(tx, ty);
                zone.setFillColor(zoneColor);
                zone.setOutlineColor(borderColor);
                zone.setOutlineThickness(1.5f);
                window.draw(zone);
            }

            // Метка внутри зоны
            if (fontLoaded && !t.name.empty()) {
                sf::Text lbl; lbl.setFont(font);
                lbl.setString(U(t.name));
                lbl.setCharacterSize(10);
                lbl.setFillColor(sf::Color(200,200,255,
                    uint8_t(140 + 60*std::sin(gameTime*2))));
                auto b = lbl.getLocalBounds();
                lbl.setOrigin(b.width/2, b.height/2);
                lbl.setPosition(tx, ty);
                window.draw(lbl);
            }

            // Подсказка [E] для INTERACT-триггеров
            float dx = player.pos.x - tx, dy = player.pos.y - ty;
            bool nearPlayer = (t.shape == TriggerShape::CIRCLE)
                ? (dx*dx+dy*dy < (t.radius+16)*(t.radius+16))
                : (std::abs(dx) < t.w/2+16 && std::abs(dy) < t.h/2+16);
            if (nearPlayer && t.event == TriggerEvent::INTERACT && fontLoaded) {
                sf::Text hint; hint.setFont(font);
                hint.setString(U("[E] " + t.name));
                hint.setCharacterSize(12);
                hint.setFillColor(sf::Color(255,220,60,
                    uint8_t(200 + 55*std::sin(gameTime*5))));
                hint.setStyle(sf::Text::Bold);
                auto b = hint.getLocalBounds();
                hint.setOrigin(b.width/2, b.height);
                hint.setPosition(tx, ty - t.radius - 14.f);
                window.draw(hint);
            }
        }
    }

    void drawPlayer(){
        float cx=player.pos.x, cy=player.pos.y;
        if(player.animPlayer){
            float bob = player.moving ? std::sin(player.animTime*9.f)*4.f : 0.f;
            float squashX = player.moving ? 1.f+std::cos(player.animTime*9.f)*0.04f : 1.f;
            sf::CircleShape shadow(14,20);
            shadow.setOrigin(14,5);
            shadow.setScale(squashX,0.35f);
            shadow.setPosition(cx,cy+24);
            shadow.setFillColor(sf::Color(0,0,0,player.moving?50:75));
            window.draw(shadow);
            player.animPlayer->draw(window);
        }
        float bob2=player.moving?std::sin(player.animTime*10)*3.f:0.f;
        drawWorldBar(window, cx-22,cy-52+bob2,44,6,player.hp/player.maxHp,sf::Color(220,50,50));
    }

    void drawEnemies(){
        for(auto& e:enemies){
            if(e.hp<=0) continue;
            bool targeted=(targetEnemy==&e);
            float r=e.boss?18.f:12.f;
            sf::CircleShape shadow(r*0.8f,20);
            shadow.setOrigin(r*0.8f,r*0.4f);
            shadow.setScale(1,0.3f);
            shadow.setPosition(e.pos.x,e.pos.y+r*0.5f);
            shadow.setFillColor(sf::Color(0,0,0,60));
            window.draw(shadow);
            if(e.animPlayer){
                e.animPlayer->draw(window);
            } else {
                drawChibiEnemy(window,e,targeted,gameTime);
            }
            drawWorldBar(window, e.pos.x-22,e.pos.y-r-16,44,5,
                e.hp/e.maxHp,e.boss?sf::Color(255,120,0):sf::Color(200,50,50));
            if(targeted){
                sf::CircleShape sel(r+6,20);
                sel.setOrigin(r+6,r+6);
                sel.setPosition(e.pos.x,e.pos.y);
                sel.setFillColor(sf::Color::Transparent);
                sel.setOutlineColor(sf::Color(100,200,100,200));
                sel.setOutlineThickness(2);
                window.draw(sel);
            }

            // ── Иконки статус-эффектов над врагом ────────────────
            if (!e.statuses.empty()) {
                float ix = e.pos.x - e.statuses.size() * 8.f;
                float iy = e.pos.y - r - 24.f;
                for (auto& st : e.statuses) {
                    sf::CircleShape ic(5, 6);
                    ic.setOrigin(5, 5);
                    ic.setPosition(ix, iy);
                    switch (st.type) {
                        case StatusType::POISON: ic.setFillColor(sf::Color(60,200,60)); break;
                        case StatusType::STUN:   ic.setFillColor(sf::Color(240,220,40)); break;
                        case StatusType::SLOW:   ic.setFillColor(sf::Color(80,160,220)); break;
                        case StatusType::BURN:   ic.setFillColor(sf::Color(255,120,40)); break;
                    }
                    ic.setOutlineColor(sf::Color(0,0,0,120));
                    ic.setOutlineThickness(1.f);
                    window.draw(ic);
                    ix += 14.f;
                }
            }

            // ── Имя врага (боссы и targeted) ─────────────────────
            if (fontLoaded && (e.boss || targeted)) {
                sf::Text nm; nm.setFont(font);
                nm.setString(U(e.name + (e.boss ? " [BOSS]" : "")));
                nm.setCharacterSize(e.boss ? 13 : 11);
                nm.setFillColor(e.boss ? sf::Color(255,160,40,220) : sf::Color(220,200,200,180));
                auto nb = nm.getLocalBounds();
                nm.setOrigin(nb.width/2, nb.height);
                nm.setPosition(e.pos.x, e.pos.y - r - 20.f);
                window.draw(nm);
            }
        }
    }

    void drawParticles(){
        for(auto& p:particles){
            sf::CircleShape circle(p.size);
            circle.setOrigin(p.size,p.size);
            circle.setPosition(p.pos.x,p.pos.y);
            sf::Color c=p.color;
            c.a=uint8_t(255.f*p.life/p.maxLife);
            circle.setFillColor(c);
            window.draw(circle);
        }
    }

    void drawFloatTexts(){
        if(!fontLoaded) return;
        for(auto& ft:floatingTexts){
            sf::Text txt; txt.setFont(font); txt.setString(U(ft.text));
            txt.setCharacterSize(20); txt.setFillColor(ft.color);
            auto b=txt.getLocalBounds();
            txt.setOrigin(b.width/2,b.height/2);
            txt.setPosition(ft.pos.x,ft.pos.y);
            window.draw(txt);
        }
    }

    void drawHUD(){
        if(!fontLoaded) return;

        // ── HP / MP бары ─────────────────────────────────────────
        // Фон
        drawRoundedRect(window, 16, 16, 220, 80,
            sf::Color(15, 10, 28, 200), sf::Color(80, 60, 120), 1);

        // HP бар
        drawWorldBar(window, 26, 24, 200, 18, player.hp/player.maxHp, sf::Color(200,50,50));
        {
            sf::Text t; t.setFont(font);
            t.setString(U("HP  " + std::to_string((int)player.hp) + "/" + std::to_string((int)player.maxHp)));
            t.setCharacterSize(12); t.setFillColor(sf::Color(240,200,200));
            t.setPosition(28, 26); window.draw(t);
        }
        // MP бар
        drawWorldBar(window, 26, 50, 200, 18, player.mp/player.maxMp, sf::Color(50,100,200));
        {
            sf::Text t; t.setFont(font);
            t.setString(U("MP  " + std::to_string((int)player.mp) + "/" + std::to_string((int)player.maxMp)));
            t.setCharacterSize(12); t.setFillColor(sf::Color(200,210,240));
            t.setPosition(28, 52); window.draw(t);
        }
        // XP бар
        drawWorldBar(window, 26, 74, 200, 8,
            player.xpNext>0 ? (float)player.xp/player.xpNext : 0.f, sf::Color(80,180,255));

        // Level + Gold
        sf::Text lvlTxt; lvlTxt.setFont(font);
        lvlTxt.setString(U("Lv." + std::to_string(player.level) +
                           "   +" + std::to_string(player.xp) + "xp"));
        lvlTxt.setCharacterSize(11); lvlTxt.setFillColor(sf::Color(160,200,120));
        lvlTxt.setPosition(26, 85); window.draw(lvlTxt);

        // ── Панель скиллов (нижняя центральная) ──────────────────
        const int NSLOTS = (int)player.skills.size();
        const float SLOT_SZ = 52.f, SLOT_GAP = 6.f;
        float totalW = NSLOTS * SLOT_SZ + (NSLOTS - 1) * SLOT_GAP;
        float barX = (WINDOW_W - totalW) / 2.f;
        float barY = WINDOW_H - SLOT_SZ - 24.f;

        // Фон панели
        drawRoundedRect(window, barX - 10, barY - 8, totalW + 20, SLOT_SZ + 16,
            sf::Color(15, 10, 28, 200), sf::Color(80, 60, 120), 1);

        for (int i = 0; i < NSLOTS; i++) {
            auto& sk = player.skills[i];
            float sx = barX + i * (SLOT_SZ + SLOT_GAP);
            bool onCD = sk.currentCooldown > 0;
            bool noMana = player.mp < sk.manaCost;

            // Фон слота
            sf::RectangleShape slotBg({SLOT_SZ, SLOT_SZ});
            slotBg.setPosition(sx, barY);
            slotBg.setFillColor(sf::Color(25, 18, 42));
            slotBg.setOutlineColor(onCD || noMana ? sf::Color(60,50,80) : sf::Color(140,100,200));
            slotBg.setOutlineThickness(2.f);
            window.draw(slotBg);

            // Имя скилла
            sf::Text nm; nm.setFont(font);
            nm.setString(U(sk.name.size()>5 ? sk.name.substr(0,5) : sk.name));
            nm.setCharacterSize(11); nm.setFillColor(sf::Color(200,180,240));
            nm.setStyle(sf::Text::Bold);
            auto nb = nm.getLocalBounds();
            nm.setOrigin(nb.width/2, 0);
            nm.setPosition(sx + SLOT_SZ/2, barY + 4);
            window.draw(nm);

            // Мана и кулдаун текст
            sf::Text sub; sub.setFont(font);
            sub.setString(U(std::to_string(sk.manaCost) + "mp"));
            sub.setCharacterSize(10);
            sub.setFillColor(noMana ? sf::Color(160,80,80) : sf::Color(100,140,220));
            auto sb = sub.getLocalBounds();
            sub.setOrigin(sb.width/2, 0);
            sub.setPosition(sx + SLOT_SZ/2, barY + 22);
            window.draw(sub);

            // Кулдаун оверлей
            if (onCD) {
                float cdPct = sk.currentCooldown / sk.cooldown;
                sf::RectangleShape cdBar({SLOT_SZ, SLOT_SZ * cdPct});
                cdBar.setPosition(sx, barY + SLOT_SZ * (1.f - cdPct));
                cdBar.setFillColor(sf::Color(0, 0, 0, 150));
                window.draw(cdBar);

                sf::Text cdTxt; cdTxt.setFont(font);
                cdTxt.setString(std::to_string((int)std::ceil(sk.currentCooldown)));
                cdTxt.setCharacterSize(18); cdTxt.setStyle(sf::Text::Bold);
                cdTxt.setFillColor(sf::Color(220, 200, 80));
                auto ct = cdTxt.getLocalBounds();
                cdTxt.setOrigin(ct.width/2, ct.height/2);
                cdTxt.setPosition(sx + SLOT_SZ/2, barY + SLOT_SZ/2);
                window.draw(cdTxt);
            }

            // Хоткей [1-4]
            sf::Text hk; hk.setFont(font);
            hk.setString("[" + std::to_string(i+1) + "]");
            hk.setCharacterSize(10); hk.setFillColor(sf::Color(120, 110, 150));
            hk.setPosition(sx + 3, barY + SLOT_SZ - 13);
            window.draw(hk);
        }

        // ── Target frame ─────────────────────────────────────────
        if (targetEnemy && targetEnemy->hp > 0) {
            float tfX = WINDOW_W - 230.f, tfY = 16.f;
            drawRoundedRect(window, tfX, tfY, 214, 56,
                sf::Color(15, 10, 28, 200), sf::Color(200,60,60), 1);
            sf::Text tnm; tnm.setFont(font);
            std::string tname = targetEnemy->name + " Lv." + std::to_string(targetEnemy->level);
            if (targetEnemy->boss) tname += " [BOSS]";
            tnm.setString(U(tname));
            tnm.setCharacterSize(13); tnm.setFillColor(sf::Color(240, 180, 180));
            tnm.setPosition(tfX + 10, tfY + 8);
            window.draw(tnm);
            drawWorldBar(window, tfX + 10, tfY + 30, 194, 14,
                targetEnemy->hp / targetEnemy->maxHp,
                targetEnemy->boss ? sf::Color(255,120,0) : sf::Color(200,50,50));
            sf::Text thp; thp.setFont(font);
            thp.setString(U(std::to_string((int)targetEnemy->hp) + " / " +
                            std::to_string((int)targetEnemy->maxHp)));
            thp.setCharacterSize(11); thp.setFillColor(sf::Color(220,180,180));
            thp.setPosition(tfX + 12, tfY + 31);
            window.draw(thp);

            // Статус-эффекты цели
            float stX = tfX + 10; float stY = tfY + 48;
            for (auto& st : targetEnemy->statuses) {
                sf::Color sc; std::string sl;
                switch (st.type) {
                    case StatusType::POISON: sc=sf::Color(60,200,60);  sl="ЯД";   break;
                    case StatusType::STUN:   sc=sf::Color(240,220,40); sl="СТАН"; break;
                    case StatusType::SLOW:   sc=sf::Color(80,160,220); sl="СЛО";  break;
                    case StatusType::BURN:   sc=sf::Color(255,120,40); sl="ОГН";  break;
                }
                sf::RectangleShape sb({28.f, 14.f});
                sb.setPosition(stX, stY);
                sb.setFillColor(sf::Color(sc.r/3, sc.g/3, sc.b/3, 200));
                sb.setOutlineColor(sc); sb.setOutlineThickness(1.f);
                window.draw(sb);
                sf::Text st2; st2.setFont(font);
                st2.setString(sl); st2.setCharacterSize(8);
                st2.setFillColor(sc);
                st2.setPosition(stX + 2, stY + 2);
                window.draw(st2);
                stX += 32.f;
            }
        }

        // ── Активные квесты ───────────────────────────────────────
        auto activeQs = questSystem.getActive();
        float qy = 110.f;
        for (auto* q : activeQs) {
            sf::Text qt; qt.setFont(font);
            qt.setString(U("● " + q->name));
            qt.setCharacterSize(13); qt.setFillColor(sf::Color(220,200,100));
            qt.setPosition(20, qy); window.draw(qt); qy += 16.f;
            for (auto& obj : q->objectives) {
                if (!obj.isComplete()) {
                    sf::Text ot; ot.setFont(font);
                    ot.setString(U("  " + obj.statusLine()));
                    ot.setCharacterSize(12); ot.setFillColor(sf::Color(160,160,180));
                    ot.setPosition(20, qy); window.draw(ot); qy += 14.f;
                }
            }
        }

        // ── Квест-уведомление ─────────────────────────────────────
        if (questNotifyTimer > 0.f && !questNotification.empty()) {
            float alpha = std::min(1.f, questNotifyTimer / 1.5f);
            sf::RectangleShape bg({440, 56});
            bg.setPosition(WINDOW_W/2 - 220, 20);
            bg.setFillColor(sf::Color(20, 40, 20, (uint8_t)(200*alpha)));
            bg.setOutlineColor(sf::Color(80, 200, 80, (uint8_t)(255*alpha)));
            bg.setOutlineThickness(1.5f);
            window.draw(bg);
            sf::Text nt; nt.setFont(font);
            nt.setString(U(questNotification));
            nt.setCharacterSize(14);
            nt.setFillColor(sf::Color(120, 255, 120, (uint8_t)(255*alpha)));
            auto nb = nt.getLocalBounds();
            nt.setOrigin(nb.width/2, nb.height/2);
            nt.setPosition(WINDOW_W/2, 48);
            window.draw(nt);
        }

        // ── Подсказки управления (нижний левый) ──────────────────
        sf::Text hint; hint.setFont(font);
        hint.setString(U("[C] Персонаж  [Tab] Инв  [M] Карта  [E] Взаим  [7][8] Зелья"));
        hint.setCharacterSize(11); hint.setFillColor(sf::Color(80, 70, 100));
        hint.setPosition(16, WINDOW_H - 16);
        window.draw(hint);
    }

    void drawStatsPanel(){
        if(!fontLoaded) return;
        float px = WINDOW_W - 290.f, py = 16.f, pw = 274.f, ph = 380.f;
        drawRoundedRect(window, px, py, pw, ph,
            sf::Color(20, 12, 35, 230), sf::Color(120, 80, 180), 2);

        sf::Text title; title.setFont(font);
        title.setString(U("[ " + player.name + " ]"));
        title.setCharacterSize(16); title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(220, 185, 80));
        auto tb = title.getLocalBounds();
        title.setOrigin(tb.width/2, 0);
        title.setPosition(px + pw/2, py + 10);
        window.draw(title);

        sf::Text cls; cls.setFont(font);
        cls.setString(U(player.className + " — Уровень " + std::to_string(player.level)));
        cls.setCharacterSize(13); cls.setFillColor(sf::Color(160, 140, 200));
        auto cb = cls.getLocalBounds();
        cls.setOrigin(cb.width/2, 0);
        cls.setPosition(px + pw/2, py + 32);
        window.draw(cls);

        sf::RectangleShape div({pw - 20.f, 1.f});
        div.setPosition(px + 10, py + 52);
        div.setFillColor(sf::Color(80, 60, 120));
        window.draw(div);

        struct StatLine { std::string label; std::string value; sf::Color col; };
        std::vector<StatLine> stats = {
            {"❤ HP",  std::to_string((int)player.hp) + " / " + std::to_string((int)getTotalStat("hp")), sf::Color(220,80,80)},
            {"💧 MP",  std::to_string((int)player.mp) + " / " + std::to_string((int)getTotalStat("mp")), sf::Color(80,120,220)},
            {"⚔ Атака", std::to_string((int)getTotalStat("damage")), sf::Color(200,160,60)},
            {"🛡 Защита", std::to_string((int)getTotalStat("defense")), sf::Color(100,160,220)},
            {"STR",  std::to_string(player.str),  sf::Color(220,100,60)},
            {"AGI",  std::to_string(player.agi),  sf::Color(80,220,120)},
            {"INT",  std::to_string(player.intel), sf::Color(120,100,240)},
            {"VIT",  std::to_string(player.vit),  sf::Color(100,200,160)},
            {"💀 Убийств", std::to_string(player.kills), sf::Color(180,100,100)},
            {"💰 Золото",  std::to_string(player.gold),  sf::Color(220,185,40)},
            {"XP",   std::to_string(player.xp) + " / " + std::to_string(player.xpNext), sf::Color(80,200,255)},
        };

        float sy = py + 60.f;
        for (auto& sl : stats) {
            sf::Text lbl, val;
            lbl.setFont(font); lbl.setString(U(sl.label));
            lbl.setCharacterSize(13); lbl.setFillColor(sf::Color(140,130,160));
            lbl.setPosition(px + 14, sy);
            window.draw(lbl);

            val.setFont(font); val.setString(U(sl.value));
            val.setCharacterSize(13); val.setFillColor(sl.col);
            auto vb = val.getLocalBounds();
            val.setPosition(px + pw - 14 - vb.width, sy);
            window.draw(val);
            sy += 24.f;
        }

        sf::RectangleShape div2({pw - 20.f, 1.f});
        div2.setPosition(px + 10, sy + 2);
        div2.setFillColor(sf::Color(80, 60, 120));
        window.draw(div2);
        sy += 8.f;

        sf::Text pot; pot.setFont(font);
        pot.setString(U("🧪 HP Зелья: " + std::to_string(player.hpPotions) +
                        "   MP: " + std::to_string(player.mpPotions)));
        pot.setCharacterSize(12); pot.setFillColor(sf::Color(160,200,120));
        pot.setPosition(px + 14, sy);
        window.draw(pot);
    }

    // ═══════════════════════════════════════════════════════════
    // ОКНО ПЕРСОНАЖА С ЭКИПИРОВКОЙ  [C]
    // ═══════════════════════════════════════════════════════════
    void drawCharacterPanel() {
        if (!fontLoaded) return;

        // ── Размер и позиция окна ────────────────────────────────
        const float CW = 620.f, CH = 520.f;
        const float CX = WINDOW_W / 2.f - CW / 2.f;
        const float CY = WINDOW_H / 2.f - CH / 2.f;

        // Затемнение фона
        sf::RectangleShape overlay({(float)WINDOW_W, (float)WINDOW_H});
        overlay.setFillColor(sf::Color(0, 0, 0, 120));
        window.draw(overlay);

        // Фон панели
        drawRoundedRect(window, CX, CY, CW, CH,
            sf::Color(14, 9, 26, 252), sf::Color(130, 85, 195), 2);

        // Заголовок
        sf::Text title; title.setFont(font);
        title.setString(U("[ " + player.name + " | " + player.className + " | Ур." + std::to_string(player.level) + " ]"));
        title.setCharacterSize(16); title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(220, 185, 80));
        auto tb = title.getLocalBounds(); title.setOrigin(tb.width/2, 0);
        title.setPosition(CX + CW/2, CY + 10);
        window.draw(title);

        // Подсказка
        sf::Text hint; hint.setFont(font);
        hint.setString(U("[C] закрыть   клик по слоту — снять вещь"));
        hint.setCharacterSize(10); hint.setFillColor(sf::Color(90, 80, 120));
        auto hb = hint.getLocalBounds(); hint.setOrigin(hb.width/2, 0);
        hint.setPosition(CX + CW/2, CY + 30);
        window.draw(hint);

        // Разделитель
        sf::RectangleShape div1({CW - 16.f, 1.f});
        div1.setPosition(CX + 8, CY + 46);
        div1.setFillColor(sf::Color(70, 50, 110));
        window.draw(div1);

        // ─────────────────────────────────────────────────────────
        // ЛЕВАЯ КОЛОНКА: слоты экипировки (4 слева от манекена)
        // ─────────────────────────────────────────────────────────
        const float SS = 54.f;   // размер слота
        const float SG = 8.f;    // зазор между слотами
        const float LC = CX + 12.f;   // X левой колонки
        const float RC = CX + CW - 12.f - SS; // X правой колонки

        // Слоты: left column, right column, bottom row
        struct SlotDef { std::string slot, label; float x, y; };
        std::vector<SlotDef> slots = {
            // Левая колонка (4 слота)
            {"main_hand", "Оружие",   LC,      CY + 58.f},
            {"ring1",     "Кольцо 1", LC,      CY + 58.f + (SS+SG)},
            {"ring2",     "Кольцо 2", LC,      CY + 58.f + (SS+SG)*2},
            {"legs",      "Сапоги",   LC,      CY + 58.f + (SS+SG)*3},
            // Правая колонка (4 слота)
            {"off_hand",  "Щит",      RC,      CY + 58.f},
            {"head",      "Голова",   RC,      CY + 58.f + (SS+SG)},
            {"chest",     "Броня",    RC,      CY + 58.f + (SS+SG)*2},
            {"neck",      "Шея/Ам.",  RC,      CY + 58.f + (SS+SG)*3},
        };

        for (auto& sd : slots) {
            std::string eqId;
            auto it = player.equipped.find(sd.slot);
            if (it != player.equipped.end()) eqId = it->second;
            bool filled = !eqId.empty();

            // Рамка слота — заполненный выглядит заметно ярче
            sf::RectangleShape bg({SS, SS});
            bg.setPosition(sd.x, sd.y);
            bg.setFillColor(filled ? sf::Color(65, 40, 100) : sf::Color(18, 12, 32));
            bg.setOutlineColor(filled ? sf::Color(230, 180, 255) : sf::Color(60, 48, 90));
            bg.setOutlineThickness(filled ? 2.5f : 1.f);
            window.draw(bg);

            if (filled) {
                const ItemDef* def = itemSystem.get(eqId);
                sf::Color rarCol = def ? rarityColor((int)def->rarity) : sf::Color(160,155,150);

                // Иконка (emoji или аббревиатура)
                std::string ico = def && !def->icon.empty() ? def->icon
                    : (eqId.size()>=2 ? eqId.substr(0,2) : eqId);
                sf::Text it2; it2.setFont(font); it2.setString(U(ico));
                it2.setCharacterSize(22); it2.setFillColor(rarCol); it2.setStyle(sf::Text::Bold);
                auto ib = it2.getLocalBounds(); it2.setOrigin(ib.width/2, ib.height/2);
                it2.setPosition(sd.x + SS/2, sd.y + SS/2 - 4);
                window.draw(it2);

                // Имя предмета под слотом — UTF-8 безопасное усечение
                std::string nm = def ? def->name : eqId;
                // Считаем символы (не байты) для кириллицы
                auto utf8chars = [](const std::string& s) -> int {
                    int n = 0;
                    for (unsigned char c : s) if ((c & 0xC0) != 0x80) n++;
                    return n;
                };
                auto utf8trunc = [](const std::string& s, int maxC) -> std::string {
                    int chars = 0; size_t i = 0;
                    while (i < s.size() && chars < maxC) {
                        unsigned char c = (unsigned char)s[i];
                        if      ((c & 0x80) == 0)    i += 1;
                        else if ((c & 0xE0) == 0xC0) i += 2;
                        else if ((c & 0xF0) == 0xE0) i += 3;
                        else                          i += 4;
                        chars++;
                    }
                    return s.substr(0, std::min(i, s.size()));
                };
                if (utf8chars(nm) > 9) nm = utf8trunc(nm, 8) + "~";
                sf::Text nt; nt.setFont(font); nt.setString(U(nm));
                nt.setCharacterSize(8); nt.setFillColor(sf::Color(rarCol.r, rarCol.g, rarCol.b, 200));
                auto nb2 = nt.getLocalBounds(); nt.setOrigin(nb2.width/2, 0);
                nt.setPosition(sd.x + SS/2, sd.y + SS + 2);
                window.draw(nt);

                // Подсказка «клик — снять» в углу слота
                sf::Text rmv; rmv.setFont(font); rmv.setString("x");
                rmv.setCharacterSize(9); rmv.setFillColor(sf::Color(200, 100, 100, 180));
                rmv.setPosition(sd.x + SS - 11, sd.y + 2);
                window.draw(rmv);
            } else {
                // Лейбл пустого слота
                sf::Text lbl; lbl.setFont(font); lbl.setString(U(sd.label));
                lbl.setCharacterSize(9); lbl.setFillColor(sf::Color(65, 55, 95));
                auto lb = lbl.getLocalBounds(); lbl.setOrigin(lb.width/2, lb.height/2);
                lbl.setPosition(sd.x + SS/2, sd.y + SS/2);
                window.draw(lbl);
            }
        }

        // ─────────────────────────────────────────────────────────
        // ЦЕНТР: манекен — реальный idle-спрайт персонажа
        // ─────────────────────────────────────────────────────────
        const float DOLL_X = CX + CW/2.f;
        const float DOLL_Y = CY + 260.f;    // центр по Y

        // Тёмный фон под манекеном
        sf::RectangleShape dollBg({130.f, 280.f});
        dollBg.setOrigin(65, 140);
        dollBg.setPosition(DOLL_X, DOLL_Y);
        dollBg.setFillColor(sf::Color(20, 14, 36));
        dollBg.setOutlineColor(sf::Color(70, 50, 110));
        dollBg.setOutlineThickness(1.f);
        window.draw(dollBg);

        if (player.animPlayer) {
            // Создаём временный AnimationPlayer и показываем первый кадр idle
            // (использует уже загруженную текстуру)
            AnimationPlayer dollPlayer(animManager);
            std::string entityKey = "player";  // или по className
            bool loaded = dollPlayer.playAnimation(entityKey, "idle");
            if (!loaded) {
                // fallback — попробуем другие ключи
                for (auto& key : {"warrior","mage","rogue","paladin","hero"}) {
                    if (dollPlayer.playAnimation(key, "idle")) { loaded = true; break; }
                }
            }

            if (loaded) {
                // Стоп на первом кадре, масштаб крупнее для манекена
                dollPlayer.stop();
                dollPlayer.setScale(2.8f, 2.8f);
                dollPlayer.setFacing(false);   // смотрит вправо (лицом к игроку)
                dollPlayer.setPosition(DOLL_X, DOLL_Y);
                dollPlayer.draw(window);
            } else {
                // Если анимации нет — рисуем стилизованный силуэт
                // Тело
                sf::RectangleShape torso({38.f, 50.f}); torso.setOrigin(19, 0);
                torso.setPosition(DOLL_X, DOLL_Y - 50);
                torso.setFillColor(sf::Color(55, 45, 75));
                torso.setOutlineColor(sf::Color(90, 70, 130)); torso.setOutlineThickness(1);
                window.draw(torso);
                // Голова
                sf::CircleShape head(20); head.setOrigin(20, 20);
                head.setPosition(DOLL_X, DOLL_Y - 72);
                head.setFillColor(sf::Color(190, 160, 120));
                window.draw(head);
                // Ноги
                sf::RectangleShape legs({38.f, 38.f}); legs.setOrigin(19, 0);
                legs.setPosition(DOLL_X, DOLL_Y);
                legs.setFillColor(sf::Color(45, 36, 62));
                legs.setOutlineColor(sf::Color(80, 65, 110)); legs.setOutlineThickness(1);
                window.draw(legs);
            }
        }

        // Имя класса под манекеном
        sf::Text clsLbl; clsLbl.setFont(font);
        clsLbl.setString(U(player.className));
        clsLbl.setCharacterSize(11); clsLbl.setFillColor(sf::Color(160, 140, 200));
        auto clb = clsLbl.getLocalBounds(); clsLbl.setOrigin(clb.width/2, 0);
        clsLbl.setPosition(DOLL_X, CY + CH - 44);
        window.draw(clsLbl);

        // ─────────────────────────────────────────────────────────
        // НИЖНЯЯ СТРОКА: XP-бар + золото
        // ─────────────────────────────────────────────────────────
        float xpRatio = player.xpNext > 0 ? (float)player.xp / player.xpNext : 0.f;
        drawWorldBar(window, CX + 10, CY + CH - 24, CW - 20, 12, xpRatio, sf::Color(80, 180, 255));
        sf::Text xpTxt; xpTxt.setFont(font);
        xpTxt.setString(U("XP: " + std::to_string(player.xp) + " / " + std::to_string(player.xpNext)
            + "   |   Золото: " + std::to_string(player.gold)));
        xpTxt.setCharacterSize(10); xpTxt.setFillColor(sf::Color(120, 170, 220));
        auto xb = xpTxt.getLocalBounds(); xpTxt.setOrigin(xb.width/2, 0);
        xpTxt.setPosition(CX + CW/2, CY + CH - 24);
        window.draw(xpTxt);

        // ─────────────────────────────────────────────────────────
        // ПРАВЕЕ ЦЕНТРА: итоговые статы (в правой половине между
        // правой колонкой слотов и правым краем правой колонки)
        // ─────────────────────────────────────────────────────────
        // Статы рисуем между левой и правой колонками,
        // справа от манекена
        float statX = CX + CW/2.f + 80.f;
        float statY = CY + 58.f;

        sf::Text stHdr; stHdr.setFont(font);
        stHdr.setString(U("Характеристики"));
        stHdr.setCharacterSize(12); stHdr.setStyle(sf::Text::Bold);
        stHdr.setFillColor(sf::Color(170, 150, 215));
        stHdr.setPosition(statX, statY); window.draw(stHdr);
        statY += 20.f;

        sf::RectangleShape sdiv({RC - statX - 4, 1.f});
        sdiv.setPosition(statX, statY); sdiv.setFillColor(sf::Color(60,45,90));
        window.draw(sdiv);
        statY += 6.f;

        auto bonusStr = [&](const std::string& key) -> std::string {
            auto it = player.equipBonuses.find(key);
            if (it == player.equipBonuses.end() || it->second == 0.f) return "";
            return " +" + std::to_string((int)it->second);
        };

        struct SL { std::string lbl, val; sf::Color col; };
        std::vector<SL> statLines = {
            {"❤ HP",    std::to_string((int)player.hp)+"/"+std::to_string((int)getTotalStat("hp"))+bonusStr("hp"),    sf::Color(220,80,80)},
            {"💧 MP",   std::to_string((int)player.mp)+"/"+std::to_string((int)getTotalStat("mp"))+bonusStr("mp"),    sf::Color(80,120,220)},
            {"⚔ Атака", std::to_string((int)getTotalStat("damage"))+bonusStr("damage"),  sf::Color(220,170,50)},
            {"🛡 Защита",std::to_string((int)getTotalStat("defense"))+bonusStr("defense"),sf::Color(90,155,220)},
            {"STR",     std::to_string(player.str),   sf::Color(220,100,60)},
            {"AGI",     std::to_string(player.agi),   sf::Color(80,220,120)},
            {"INT",     std::to_string(player.intel),  sf::Color(120,100,240)},
            {"VIT",     std::to_string(player.vit),   sf::Color(100,200,160)},
            {"Убийств", std::to_string(player.kills),  sf::Color(180,100,100)},
        };

        float colW = RC - statX - 4;
        for (auto& sl : statLines) {
            sf::Text lv; lv.setFont(font); lv.setString(U(sl.lbl));
            lv.setCharacterSize(11); lv.setFillColor(sf::Color(110,100,145));
            lv.setPosition(statX, statY); window.draw(lv);

            sf::Text vv; vv.setFont(font); vv.setString(U(sl.val));
            vv.setCharacterSize(11); vv.setFillColor(sl.col);
            auto vb = vv.getLocalBounds();
            vv.setPosition(statX + colW - vb.width, statY);
            window.draw(vv);
            statY += 19.f;
        }
    }

    void drawInventoryPanel(){
        if(!fontLoaded) return;
        float px = 16.f, py = WINDOW_H - 340.f, pw = 340.f, ph = 324.f;
        drawRoundedRect(window, px, py, pw, ph,
            sf::Color(20, 12, 35, 230), sf::Color(120, 80, 180), 2);

        sf::Text title; title.setFont(font);
        title.setString(U("[ Инвентарь ]  Tab-закрыть"));
        title.setCharacterSize(14); title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(220, 185, 80));
        title.setPosition(px + 12, py + 8);
        window.draw(title);

        // Сетка 5×4 слотов
        const int COLS = 5, ROWS = 4;
        const float SZ = 48.f, GAP = 6.f;
        float gridX = px + 12.f, gridY = py + 34.f;

        // Все предметы из bag
        std::vector<std::pair<std::string, int>> items; // name, count
        std::vector<std::string> itemIds;
        for (auto& s : player.bag.slots) {
            const ItemDef* def = itemSystem.get(s.itemId);
            items.push_back({def ? def->name : s.itemId, s.count});
            itemIds.push_back(s.itemId);
        }

        int slot = 0;
        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLS; col++) {
                float sx = gridX + col * (SZ + GAP);
                float sy = gridY + row * (SZ + GAP);

                bool isSelected = (slot == selectedInvSlot);

                sf::RectangleShape bg({SZ, SZ});
                bg.setPosition(sx, sy);
                bg.setFillColor(sf::Color(30, 20, 50));
                bg.setOutlineColor(isSelected ? sf::Color(255,220,50) : sf::Color(70, 55, 100));
                bg.setOutlineThickness(isSelected ? 2.5f : 1.f);
                window.draw(bg);

                if (slot < (int)items.size()) {
                    auto& itm = items[slot];
                    const ItemDef* def = itemSystem.get(itemIds[slot]);
                    int rar = def ? (int)def->rarity : 0;
                    sf::Color rarBorder = rarityColor(rar);

                    sf::RectangleShape filled({SZ, SZ});
                    filled.setPosition(sx, sy);
                    filled.setFillColor(sf::Color(40+rar*8, 28+rar*4, 60+rar*6));
                    filled.setOutlineColor(isSelected ? sf::Color(255,220,50) : rarBorder);
                    filled.setOutlineThickness(isSelected ? 2.5f : 2.f);
                    window.draw(filled);

                    // Иконка предмета (emoji или аббревиатура)
                    std::string icon_str;
                    if (def && !def->icon.empty()) icon_str = def->icon;
                    else {
                        icon_str = itm.first.size() >= 2 ? itm.first.substr(0,2) : itm.first;
                    }
                    sf::Text icon; icon.setFont(font);
                    icon.setString(U(icon_str));
                    icon.setCharacterSize(18);
                    icon.setFillColor(rarBorder);
                    icon.setStyle(sf::Text::Bold);
                    auto ib = icon.getLocalBounds();
                    icon.setOrigin(ib.width/2, ib.height/2);
                    icon.setPosition(sx + SZ/2, sy + SZ/2 - 4);
                    window.draw(icon);

                    // Стак
                    if (itm.second > 1) {
                        sf::Text cnt; cnt.setFont(font);
                        cnt.setString(std::to_string(itm.second));
                        cnt.setCharacterSize(10);
                        cnt.setFillColor(sf::Color(220,220,220));
                        cnt.setPosition(sx + SZ - 14, sy + SZ - 14);
                        window.draw(cnt);
                    }

                    // Пометка «надет»
                    if (def) {
                        std::string checkSlot = def->slot;
                        if (checkSlot.empty()) {
                            if      (def->type=="weapon")  checkSlot="main_hand";
                            else if (def->type=="armor")   checkSlot="chest";
                            else if (def->type=="helmet")  checkSlot="head";
                            else if (def->type=="boots")   checkSlot="legs";
                            else if (def->type=="ring")    checkSlot="ring1";
                        }
                        auto eqIt = player.equipped.find(checkSlot);
                        bool worn = (eqIt != player.equipped.end() && eqIt->second == itemIds[slot]);
                        if (worn) {
                            sf::Text eq; eq.setFont(font); eq.setString("E");
                            eq.setCharacterSize(9); eq.setFillColor(sf::Color(100,255,120));
                            eq.setPosition(sx+2, sy+2);
                            window.draw(eq);
                        }
                    }
                }
                slot++;
            }
        }

        // Подсказка выбранного предмета + кнопка Надеть
        float infoY = gridY + ROWS * (SZ + GAP) + 4.f;
        if (selectedInvSlot >= 0 && selectedInvSlot < (int)player.bag.slots.size()) {
            const std::string& sid = player.bag.slots[selectedInvSlot].itemId;
            const ItemDef* def = itemSystem.get(sid);
            bool canEquip = def && def->type != "consumable" && def->type != "material" && def->type != "quest";

            // Имя предмета + описание
            if (def) {
                sf::Text nm; nm.setFont(font);
                sf::Color rarC = rarityColor((int)def->rarity);
                nm.setString(U(def->name));
                nm.setCharacterSize(12); nm.setStyle(sf::Text::Bold);
                nm.setFillColor(rarC);
                nm.setPosition(px + 12, infoY);
                window.draw(nm);
                infoY += 16.f;
                if (!def->description.empty()) {
                    sf::Text ds; ds.setFont(font);
                    ds.setString(U(def->description));
                    ds.setCharacterSize(10); ds.setFillColor(sf::Color(160,155,180));
                    ds.setPosition(px + 12, infoY);
                    window.draw(ds);
                    infoY += 14.f;
                }
                // Статы
                for (auto& [k,v] : def->stats) {
                    if (v == 0.f) continue;
                    sf::Text st; st.setFont(font);
                    st.setString(U("  +" + std::to_string((int)v) + " " + k));
                    st.setCharacterSize(10); st.setFillColor(sf::Color(120,220,120));
                    st.setPosition(px + 12, infoY);
                    window.draw(st);
                    infoY += 12.f;
                }

                // Ограничение по классу
                if (!def->allowedClasses.empty()) {
                    bool canUse = def->canBeUsedBy(player.className);
                    sf::Text clsT; clsT.setFont(font);
                    clsT.setString(U(canUse
                        ? ("✓ " + def->allowedClassesStr())
                        : ("✗ Только: " + def->allowedClassesStr())));
                    clsT.setCharacterSize(10);
                    clsT.setFillColor(canUse ? sf::Color(100,220,120) : sf::Color(255,90,90));
                    clsT.setStyle(canUse ? sf::Text::Regular : sf::Text::Bold);
                    clsT.setPosition(px + 12, infoY);
                    window.draw(clsT);
                    infoY += 13.f;
                }
            }

            // Кнопка Надеть (только для экипируемых)
            if (canEquip) {
                bool classOk = def->canBeUsedBy(player.className);
                float bx = px + 12, by = infoY, bw = 100.f, bh = 22.f;
                sf::RectangleShape btn({bw, bh});
                btn.setPosition(bx, by);
                btn.setFillColor(classOk ? sf::Color(60, 40, 100) : sf::Color(50, 30, 50));
                btn.setOutlineColor(classOk ? sf::Color(160, 110, 220) : sf::Color(100, 60, 80));
                btn.setOutlineThickness(1.5f);
                window.draw(btn);
                sf::Text bt; bt.setFont(font);
                bt.setString(U(classOk ? "Надеть [ЛКМ]" : "Не ваш класс"));
                bt.setCharacterSize(10);
                bt.setFillColor(classOk ? sf::Color(220, 200, 255) : sf::Color(160, 100, 120));
                auto bb = bt.getLocalBounds(); bt.setOrigin(bb.width/2, bb.height/2);
                bt.setPosition(bx + bw/2, by + bh/2);
                window.draw(bt);
                infoY += bh + 4.f;
            }
        }

        // Золото и зелья
        sf::Text goldTxt; goldTxt.setFont(font);
        goldTxt.setString(U("💰 " + std::to_string(player.gold) + " золота"));
        goldTxt.setCharacterSize(13); goldTxt.setFillColor(sf::Color(220, 185, 40));
        goldTxt.setPosition(px + 14, infoY);
        window.draw(goldTxt);

        sf::Text potTxt; potTxt.setFont(font);
        potTxt.setString(U("🧪 HP:" + std::to_string(player.hpPotions) +
                           "  MP:" + std::to_string(player.mpPotions) +
                           "  [7][8]"));
        potTxt.setCharacterSize(12); potTxt.setFillColor(sf::Color(120, 200, 120));
        potTxt.setPosition(px + 14, infoY + 18.f);
        window.draw(potTxt);
    }

    void drawMinimap(){
        const float MW = 180.f, MH = 180.f;
        float mx = WINDOW_W - MW - 16.f, my = WINDOW_H - MH - 16.f;
        drawRoundedRect(window, mx, my, MW, MH,
            sf::Color(15, 10, 28, 200), sf::Color(100, 75, 150), 1);

        // Масштаб: сколько тайлов в мини-карте
        const float VIEW_TILES = 60.f;  // 60×60 тайлов видно на миникарте
        float scale = MW / VIEW_TILES;

        // Центр обзора = позиция игрока в тайлах
        float camTX = player.pos.x / TILE;
        float camTY = player.pos.y / TILE;
        float halfT = VIEW_TILES / 2.f;

        // Рисуем тайлы
        int tStartX = std::max(0,  (int)(camTX - halfT));
        int tStartY = std::max(0,  (int)(camTY - halfT));
        int tEndX   = std::min(MAP_W, (int)(camTX + halfT) + 1);
        int tEndY   = std::min(MAP_H, (int)(camTY + halfT) + 1);

        for (int ty = tStartY; ty < tEndY; ty++) {
            for (int tx = tStartX; tx < tEndX; tx++) {
                float sx = mx + (tx - camTX + halfT) * scale;
                float sy = my + (ty - camTY + halfT) * scale;
                if (sx < mx || sy < my || sx >= mx+MW || sy >= my+MH) continue;
                sf::RectangleShape dot({scale + 0.5f, scale + 0.5f});
                dot.setPosition(sx, sy);
                sf::Color col;
                switch (worldMap[ty][tx].type) {
                    case SceneTileType::WATER:        col = sf::Color(40,100,200); break;
                    case SceneTileType::WALL:         col = sf::Color(90,80,70);  break;
                    case SceneTileType::TREE:         col = sf::Color(30,100,30); break;
                    case SceneTileType::ROAD:         col = sf::Color(160,145,120); break;
                    case SceneTileType::STONE_FLOOR:  col = sf::Color(110,100,90); break;
                    case SceneTileType::BUILDING_FLOOR: col = sf::Color(100,85,70); break;
                    case SceneTileType::GRASS:        col = sf::Color(50,120,40); break;
                    default:                          col = sf::Color(60,130,50); break;
                }
                dot.setFillColor(col);
                window.draw(dot);
            }
        }

        // Враги — красные точки
        for (auto& e : enemies) {
            if (e.hp <= 0) continue;
            float ex = mx + (e.pos.x/TILE - camTX + halfT) * scale;
            float ey = my + (e.pos.y/TILE - camTY + halfT) * scale;
            if (ex < mx || ey < my || ex >= mx+MW || ey >= my+MH) continue;
            sf::RectangleShape dot({3.f, 3.f});
            dot.setPosition(ex - 1.f, ey - 1.f);
            dot.setFillColor(e.boss ? sf::Color(255,120,0) : sf::Color(220,50,50));
            window.draw(dot);
        }

        // Игрок — белый/жёлтый блип
        float ppx = mx + halfT * scale;
        float ppy = my + halfT * scale;
        sf::CircleShape pip(4, 6);
        pip.setOrigin(4, 4);
        pip.setPosition(ppx, ppy);
        pip.setFillColor(sf::Color(255, 220, 60));
        pip.setOutlineColor(sf::Color(255,255,255,180));
        pip.setOutlineThickness(1.f);
        window.draw(pip);

        // Метка
        if (fontLoaded) {
            sf::Text lbl; lbl.setFont(font);
            lbl.setString(U("Карта [M]"));
            lbl.setCharacterSize(10);
            lbl.setFillColor(sf::Color(160, 140, 200, 180));
            lbl.setPosition(mx + 5, my + 3);
            window.draw(lbl);
        }
    }

    void renderLoginScreen(){
        window.clear(sf::Color(10,5,25));
        for(auto& s:stars){
            sf::CircleShape star(s.brightness>0.7f?2.f:1.f);
            star.setFillColor(sf::Color(255,255,255,uint8_t(200*s.brightness)));
            star.setPosition(s.x,s.y); window.draw(star);
        }
        if(!fontLoaded) return;
        sf::Text title; title.setFont(font);
        title.setString("AETHORIA: Eternal Realms");
        title.setCharacterSize(42); title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(220,170,80));
        auto tb=title.getLocalBounds();
        title.setOrigin(tb.width/2,tb.height/2);
        title.setPosition(WINDOW_W/2,100); window.draw(title);
        sf::Text sub; sub.setFont(font);
        sub.setString("Enter the world of endless adventure");
        sub.setCharacterSize(16); sub.setFillColor(sf::Color(140,120,160));
        auto sb=sub.getLocalBounds(); sub.setOrigin(sb.width/2,0);
        sub.setPosition(WINDOW_W/2,150); window.draw(sub);
        float px=WINDOW_W/2-160.f, py=220.f, pw=320.f, ph=260.f;
        drawRoundedRect(window,px,py,pw,ph,sf::Color(20,15,35,230),sf::Color(80,60,120),2);
        {
            sf::Text lbl; lbl.setFont(font); lbl.setString("Login:");
            lbl.setCharacterSize(13); lbl.setFillColor(sf::Color(160,140,200));
            lbl.setPosition(px+20,py+30); window.draw(lbl);
            sf::RectangleShape box({pw-40,36}); box.setPosition(px+20,py+50);
            box.setFillColor(sf::Color(15,10,30));
            box.setOutlineColor(activeField==0?sf::Color(160,100,220):sf::Color(60,50,90));
            box.setOutlineThickness(activeField==0?2:1); window.draw(box);
            std::string shown=loginText.toAnsiString()+(activeField==0?"|":"");
            sf::Text vt; vt.setFont(font); vt.setString(shown);
            vt.setCharacterSize(16); vt.setFillColor(sf::Color(220,210,240));
            vt.setPosition(px+26,py+56); window.draw(vt);
        }
        {
            sf::Text lbl; lbl.setFont(font); lbl.setString("Password:");
            lbl.setCharacterSize(13); lbl.setFillColor(sf::Color(160,140,200));
            lbl.setPosition(px+20,py+100); window.draw(lbl);
            sf::RectangleShape box({pw-40,36}); box.setPosition(px+20,py+120);
            box.setFillColor(sf::Color(15,10,30));
            box.setOutlineColor(activeField==1?sf::Color(160,100,220):sf::Color(60,50,90));
            box.setOutlineThickness(activeField==1?2:1); window.draw(box);
            std::string stars2(passText.getSize(),'*');
            if(activeField==1) stars2+='|';
            sf::Text vt; vt.setFont(font); vt.setString(stars2);
            vt.setCharacterSize(16); vt.setFillColor(sf::Color(220,210,240));
            vt.setPosition(px+26,py+126); window.draw(vt);
        }
        {
            float bx=px+80,by=py+175,bw=160,bh=40;
            sf::RectangleShape btn({bw,bh}); btn.setPosition(bx,by);
            btn.setFillColor(sf::Color(100,60,160));
            btn.setOutlineColor(sf::Color(180,120,255)); btn.setOutlineThickness(2);
            window.draw(btn);
            sf::Text bt; bt.setFont(font); bt.setString("ENTER WORLD");
            bt.setCharacterSize(15); bt.setStyle(sf::Text::Bold);
            bt.setFillColor(sf::Color(220,200,255));
            auto bb=bt.getLocalBounds(); bt.setOrigin(bb.width/2,bb.height/2);
            bt.setPosition(bx+bw/2,by+bh/2); window.draw(bt);
        }
        sf::Text hint; hint.setFont(font);
        hint.setString("Tab - switch field    Enter - login");
        hint.setCharacterSize(12); hint.setFillColor(sf::Color(100,90,120));
        auto hb=hint.getLocalBounds(); hint.setOrigin(hb.width/2,0);
        hint.setPosition(WINDOW_W/2,py+ph+16); window.draw(hint);
    }

    void renderCharSelect(){
        window.clear(sf::Color(15,10,30));
        if(!fontLoaded) return;
        sf::Text title; title.setFont(font); title.setString("Select Your Character");
        title.setCharacterSize(32); title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(200,170,100));
        auto tb=title.getLocalBounds(); title.setOrigin(tb.width/2,0);
        title.setPosition(WINDOW_W/2,20); window.draw(title);
        const char* classNames[]={"Warrior","Mage","Rogue","Paladin"};
        sf::Color classColors[]={sf::Color(200,80,60),sf::Color(80,120,220),
                                  sf::Color(60,200,100),sf::Color(220,180,60)};
        const char* skinNames[]={"Classic","Dark","Gold"};
        float slotW=280,gap=30;
        float startX=(WINDOW_W-3*slotW-2*gap)/2;
        for(int i=0;i<3;i++){
            float sx=startX+i*(slotW+gap), sy=80;
            bool sel=(i==selectedSlot);
            drawRoundedRect(window,sx,sy,slotW,460,
                sel?sf::Color(35,25,55):sf::Color(22,15,38),
                sel?sf::Color(160,100,220):sf::Color(60,50,80),sel?2:1);
            float cx2=sx+slotW/2, cy2=sy+150;
            int ci=charSlots[i].classIdx<0?0:charSlots[i].classIdx;
            sf::Color cc=classColors[ci];
            sf::CircleShape body(40); body.setOrigin(40,40);
            body.setFillColor(cc); body.setPosition(cx2,cy2);
            window.draw(body);
            sf::CircleShape head(28); head.setOrigin(28,28);
            head.setFillColor(sf::Color(240,210,180)); head.setPosition(cx2,cy2-52);
            window.draw(head);
            sf::CircleShape eye(5); eye.setFillColor(sf::Color(30,30,30));
            eye.setOrigin(5,5);
            eye.setPosition(cx2-10,cy2-56); window.draw(eye);
            eye.setPosition(cx2+10,cy2-56); window.draw(eye);
            std::string slotName = charSlots[i].name.empty() ?
                "[Empty Slot " + std::to_string(i+1) + "]" : charSlots[i].name;
            sf::Text sn; sn.setFont(font); sn.setString(slotName);
            sn.setCharacterSize(15);
            sn.setFillColor(charSlots[i].name.empty()?sf::Color(100,90,120):sf::Color(220,210,240));
            auto snb=sn.getLocalBounds(); sn.setOrigin(snb.width/2,0);
            sn.setPosition(cx2,sy+240); window.draw(sn);
            sf::Text cls; cls.setFont(font);
            cls.setString(classNames[ci]);
            cls.setCharacterSize(13); cls.setFillColor(cc);
            auto clb=cls.getLocalBounds(); cls.setOrigin(clb.width/2,0);
            cls.setPosition(cx2,sy+260); window.draw(cls);
            if(sel){
                float panelY=sy+295;
                sf::Text clsLbl; clsLbl.setFont(font); clsLbl.setString("Class:");
                clsLbl.setCharacterSize(12); clsLbl.setFillColor(sf::Color(140,130,160));
                clsLbl.setPosition(sx+10,panelY); window.draw(clsLbl);
                for(int c=0;c<4;c++){
                    sf::RectangleShape cb({60,24}); cb.setPosition(sx+10+c*66,panelY+16);
                    cb.setFillColor(c==ci?classColors[c]:sf::Color(30,22,45));
                    cb.setOutlineColor(classColors[c]); cb.setOutlineThickness(1);
                    window.draw(cb);
                    sf::Text cl; cl.setFont(font); cl.setString(std::string(classNames[c]).substr(0,3));
                    cl.setCharacterSize(10); cl.setFillColor(sf::Color(220,210,240));
                    cl.setPosition(sx+15+c*66,panelY+20); window.draw(cl);
                }
                sf::Text skinLbl; skinLbl.setFont(font); skinLbl.setString("Skin:");
                skinLbl.setCharacterSize(12); skinLbl.setFillColor(sf::Color(140,130,160));
                skinLbl.setPosition(sx+10,panelY+50); window.draw(skinLbl);
                for(int s=0;s<3;s++){
                    sf::RectangleShape sb2({80,22}); sb2.setPosition(sx+10+s*86,panelY+66);
                    sb2.setFillColor(s==charSlots[i].skinIdx?sf::Color(60,40,90):sf::Color(25,18,40));
                    sb2.setOutlineColor(sf::Color(100,80,140)); sb2.setOutlineThickness(1);
                    window.draw(sb2);
                    sf::Text sl2; sl2.setFont(font); sl2.setString(skinNames[s]);
                    sl2.setCharacterSize(10); sl2.setFillColor(sf::Color(200,190,220));
                    sl2.setPosition(sx+14+s*86,panelY+70); window.draw(sl2);
                }
                sf::Text nameLbl; nameLbl.setFont(font); nameLbl.setString("Name:");
                nameLbl.setCharacterSize(12); nameLbl.setFillColor(sf::Color(140,130,160));
                nameLbl.setPosition(sx+10,panelY+100); window.draw(nameLbl);
                sf::RectangleShape nameBox({260,28}); nameBox.setPosition(sx+10,panelY+116);
                nameBox.setFillColor(sf::Color(15,10,28));
                nameBox.setOutlineColor(nameFocused?sf::Color(160,100,220):sf::Color(60,50,80));
                nameBox.setOutlineThickness(nameFocused?2:1); window.draw(nameBox);
                std::string nameStr=charNameText.toAnsiString()+(nameFocused?"|":"");
                sf::Text nt; nt.setFont(font); nt.setString(nameStr);
                nt.setCharacterSize(14); nt.setFillColor(sf::Color(220,210,240));
                nt.setPosition(sx+14,panelY+120); window.draw(nt);
            }
        }
        float bY=WINDOW_H-55;
        {
            sf::RectangleShape btn({120,38}); btn.setPosition(WINDOW_W/2-280,bY);
            btn.setFillColor(sf::Color(60,40,80));
            btn.setOutlineColor(sf::Color(120,80,160)); btn.setOutlineThickness(2);
            window.draw(btn);
            sf::Text bt; bt.setFont(font); bt.setString("< Back");
            bt.setCharacterSize(14); bt.setFillColor(sf::Color(180,160,210));
            auto bb=bt.getLocalBounds(); bt.setOrigin(bb.width/2,bb.height/2);
            bt.setPosition(WINDOW_W/2-280+60,bY+19); window.draw(bt);
        }
        {
            sf::RectangleShape btn({200,38}); btn.setPosition(WINDOW_W/2-100,bY);
            btn.setFillColor(sf::Color(100,60,160));
            btn.setOutlineColor(sf::Color(180,120,255)); btn.setOutlineThickness(2);
            window.draw(btn);
            sf::Text bt; bt.setFont(font); bt.setString("ENTER WORLD");
            bt.setCharacterSize(15); bt.setStyle(sf::Text::Bold);
            bt.setFillColor(sf::Color(220,200,255));
            auto bb=bt.getLocalBounds(); bt.setOrigin(bb.width/2,bb.height/2);
            bt.setPosition(WINDOW_W/2,bY+19); window.draw(bt);
        }
        sf::Text hint; hint.setFont(font);
        hint.setString("Arrow keys - navigate    Enter - play    Esc - back");
        hint.setCharacterSize(12); hint.setFillColor(sf::Color(80,70,100));
        auto hb=hint.getLocalBounds(); hint.setOrigin(hb.width/2,0);
        hint.setPosition(WINDOW_W/2,WINDOW_H-18); window.draw(hint);
    }
};

int main(){
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    SetConsoleTitleA("AETHORIA: Eternal Realms — Engine");
#endif
    std::setlocale(LC_ALL, ".UTF-8");
    // FIX: MAP 512×512 — создаём GameEngine в куче, а не на стеке (worldMap ~1MB)
    auto engine = std::make_unique<GameEngine>();
    engine->run();
    return 0;
}