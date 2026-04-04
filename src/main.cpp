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
#endif

// ════════════════════════════════════════════════════════════════
// СИСТЕМЫ
// ════════════════════════════════════════════════════════════════
#include "animation_system.hpp"
#include "entity_system.hpp"
#include "scene_system.hpp"

// ============================================================
// CONSTANTS
// ============================================================
const int TILE         = 32;
const int MAP_W        = 120;
const int MAP_H        = 120;
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
    std::vector<Item>  inventory;
    int   hpPotions=5, mpPotions=3;
    float animTime=0;
    bool  moving=false;
    int   facing=0;
    AnimationPlayer* animPlayer = nullptr;
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

    std::vector<Particle> particles;
    std::vector<FloatingText> floatingTexts;
    sf::View camera;
    float gameTime=0;

    sf::Font font;
    bool fontLoaded=false;

    AnimationManager* animManager = nullptr;
    EntitySystem entitySystem;
    uint32_t playerEntityId = 0;

    // Система сцен
    SceneManager sceneManager;
    float sceneTransitionTimer = 0.f;
    bool  sceneTransitioning   = false;
    std::string pendingSceneId;

    bool showStats=false, showInventory=false, showMap=false;

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

    GameEngine() : window(sf::VideoMode(WINDOW_W,WINDOW_H),"AETHORIA: Eternal Realms",sf::Style::Default) {
        window.setFramerateLimit(60);
        camera.setSize(WINDOW_W,WINDOW_H);
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
        if (player.animPlayer) delete player.animPlayer;
        for (auto& enemy : enemies) if (enemy.animPlayer) delete enemy.animPlayer;
    }

    void loadAssets() {
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
    }

    void loadStartScene() {
        // Всегда синхронизируем map.json → scenes/aethoria_city.json
        // чтобы изменения из редактора (сущности, тайлы) отражались в движке
        std::ifstream test("assets/map.json");
        if (test.is_open()) {
            test.close();
            ensureScenesDir();
            std::string target = SceneUtils::sceneFile("aethoria_city");
            // Копируем map.json в scenes/ всегда — редактор мог его обновить
            std::ifstream src("assets/map.json", std::ios::binary);
            std::ofstream dst(target, std::ios::binary);
            if (src && dst) {
                dst << src.rdbuf();
                std::cout << "[SceneManager] map.json → " << target << " (sync)\n";
            }
        }
        if (!sceneManager.loadDefault(entitySystem)) {
            buildMap();    // fallback если нет ни одного файла сцены
            spawnEnemies();
        }
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
    }

    void switchToScene(const std::string& sceneId) {
        if (sceneTransitioning) return;
        pendingSceneId       = sceneId;
        sceneTransitioning   = true;
        sceneTransitionTimer = 0.5f;
        std::cout << "[Scene] Переключение → " << sceneId << "\n";
    }

    void updateSceneTransition(float dt) {
        if (!sceneTransitioning) return;
        sceneTransitionTimer -= dt;
        if (sceneTransitionTimer <= 0.f) {
            sceneTransitioning = false;
            sceneManager.switchScene(pendingSceneId, entitySystem,
                worldMap, MAP_W, MAP_H,
                player.pos.x, player.pos.y);
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
    }

    void spawnEnemies(){
        enemies.clear();
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
                    }
                }
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
                }
            }
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
        float bestDist = interactRange * interactRange;
        Entity* target = nullptr;
        for (auto& e : entitySystem.getAll()) {
            if (!e.active) continue;
            if (e.type==EntityType::ENEMY || e.type==EntityType::PLAYER) continue;
            float dx=player.pos.x-e.x, dy=player.pos.y-e.y;
            float d2=dx*dx+dy*dy;
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
                dialogText += "У меня есть задание для храброго авантюриста...";
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
            std::string dest=target->props.getStr("target_zone","");
            if (!dest.empty()) {
                switchToScene(dest);
            } else {
                dialogText = "Телепортация: " + (dest.empty()?"неизвестно":dest);
                dialogTimer = 2.f;
            }
            spawnParticles({player.pos.x,player.pos.y},ParticleT::MAGIC,20);
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
        txt.setString(dialogText);
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
            case sf::Keyboard::C:showStats=!showStats;break;
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
    }

    void updatePlayer(float dt){
        V2 dir;
        bool wasMoving=player.moving;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)){dir.y-=1;player.facing=1;}
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)){dir.y+=1;player.facing=0;}
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)){dir.x-=1;player.facing=2;}
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)){dir.x+=1;player.facing=3;}
        player.moving=(dir.len()>0);
        if(player.moving){
            V2 newPos=player.pos+dir.norm()*player.speed*dt;
            int tx=(int)(newPos.x/TILE),ty=(int)(newPos.y/TILE);
            if(tx>=0&&ty>=0&&tx<MAP_W&&ty<MAP_H&&worldMap[ty][tx].walkable)
                player.pos=newPos;
            player.animTime+=dt;
        }
        if(player.currentAttackCD>0) player.currentAttackCD-=dt;
        if(player.comboTimer>0) player.comboTimer-=dt; else player.combo=0;
        for(auto& s:player.skills) if(s.currentCooldown>0) s.currentCooldown-=dt;
        player.hp=std::min(player.maxHp,player.hp+3.f*dt);
        player.mp=std::min(player.maxMp,player.mp+2.f*dt);
        camera.setCenter(player.pos.x,player.pos.y);
        entitySystem.setPosition(playerEntityId, player.pos.x, player.pos.y);
        if (player.animPlayer) {
            if (player.moving) {
                if (player.animPlayer->getCurrentAction() != "run")
                    player.animPlayer->playAnimation("player", "run");
            } else {
                if (player.animPlayer->getCurrentAction() != "idle")
                    player.animPlayer->playAnimation("player", "idle");
            }
            player.animPlayer->update(dt);
            player.animPlayer->setPosition(player.pos.x, player.pos.y);
            float baseScale = 1.2f;
            if (player.facing == 2) player.animPlayer->setScale(-baseScale, baseScale);
            else player.animPlayer->setScale(baseScale, baseScale);
        }
    }

    void updateEnemies(float dt){
        for(auto& e:enemies){
            if(e.hp<=0) continue;
            e.animTime+=dt;
            float dPlayer=dist(e.pos,player.pos);
            switch(e.state){
            case EnemyState::IDLE:
            case EnemyState::PATROL:
                if(dPlayer<AGGRO_RANGE||e.aggroedByHit){e.state=EnemyState::AGGRO;e.aggroedByHit=false;}
                else{
                    e.aiTimer-=dt;
                    if(e.aiTimer<=0){
                        e.aiTimer=2.f+(float)(rand()%3);
                        e.patrolTarget=e.spawnPos+V2((rand()%160)-80.f,(rand()%160)-80.f);
                        e.state=EnemyState::PATROL;
                    }
                    if(e.state==EnemyState::PATROL){
                        V2 toDest=e.patrolTarget-e.pos;
                        if(toDest.len()<20.f){e.state=EnemyState::IDLE;e.aiTimer=1.f;}
                        else {
                            V2 move=toDest.norm()*e.speed*dt;
                            e.pos=e.pos+move;
                            if(e.animPlayer) e.animPlayer->updateFacingFromVelocity(move.x);
                        }
                    }
                }
                break;
            case EnemyState::AGGRO:{
                V2 toPlayer=player.pos-e.pos;
                float d=toPlayer.len();
                if(d<DEAGGRO_RANGE){
                    if(d>40.f){
                        V2 move=toPlayer.norm()*e.speed*dt;
                        e.pos=e.pos+move;
                        if(e.animPlayer) e.animPlayer->updateFacingFromVelocity(move.x);
                    }
                    else e.state=EnemyState::COMBAT;
                } else e.state=EnemyState::PATROL;
                break;}
            case EnemyState::COMBAT:{
                float d=dist(e.pos,player.pos);
                if(d>DEAGGRO_RANGE) e.state=EnemyState::PATROL;
                else{
                    e.currentAttackCD-=dt;
                    if(e.currentAttackCD<=0){
                        player.hp-=e.damage*0.8f;
                        e.currentAttackCD=1.2f;
                        spawnParticles(player.pos,ParticleT::BLOOD,8);
                    }
                }
                break;}
            case EnemyState::RETURN:
                if(dist(e.pos,e.spawnPos)<10.f) {e.pos=e.spawnPos;e.state=EnemyState::IDLE;}
                else {
                    V2 move=(e.spawnPos-e.pos).norm()*e.speed*dt;
                    e.pos=e.pos+move;
                    if(e.animPlayer) e.animPlayer->updateFacingFromVelocity(move.x);
                }
                break;
            default:break;
            }
            if (e.animPlayer) {
                // Переключаем анимацию в зависимости от состояния
                std::string entityName = getEntityName(e.type);
                bool isMoving = (e.state == EnemyState::PATROL ||
                                 e.state == EnemyState::AGGRO  ||
                                 e.state == EnemyState::RETURN);
                std::string wantedAnim = isMoving ? "walk" : "idle";
                // Фолбэк: если walk нет — оставляем idle
                if (wantedAnim == "walk" && e.animPlayer->getCurrentAction() != "walk") {
                    if (!e.animPlayer->playAnimation(entityName, "walk"))
                        e.animPlayer->playAnimation(entityName, "idle");
                } else if (wantedAnim == "idle" && e.animPlayer->getCurrentAction() != "idle") {
                    e.animPlayer->playAnimation(entityName, "idle");
                }
                e.animPlayer->update(dt);
                e.animPlayer->setPosition(e.pos.x, e.pos.y);
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
        player.mp-=skill.manaCost; skill.currentCooldown=skill.cooldown;
        if(targetEnemy&&targetEnemy->hp>0){
            int dmg=skill.damage+(rand()%10)-5;
            targetEnemy->hp-=dmg;
            targetEnemy->aggroedByHit=true;
            floatingTexts.push_back({targetEnemy->pos, std::to_string(dmg), 1.5f, sf::Color(200,50,50), false});
            spawnParticles(targetEnemy->pos, ParticleT::MAGIC, 5);
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

    void tryLogin(){ gameState=GameState::CHARACTER_SELECT; }
    void enterWorld(){ gameState=GameState::PLAYING; }

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
        drawEnemies();
        if(gameConfig.layerEffects)  drawParticles();
        if(gameConfig.layerEntities) drawPlayer();
        drawFloatTexts();
        window.setView(window.getDefaultView());
        drawHUD();
        drawSceneTransition();
        if(dialogTimer>0.f) drawDialog();
        if(showStats)     drawStatsPanel();
        if(showInventory) drawInventoryPanel();
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
            label.setString("~ Aethoria City ~");
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
                    nm.setString(e.name);
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
                        hint.setString("[E]");
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
                    hint.setString("[E] Открыть");
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
                float pulse = 0.85f + 0.15f*std::sin(gameTime*3.f);
                sf::CircleShape outer(r*1.3f*pulse,30);
                outer.setOrigin(r*1.3f*pulse,r*1.3f*pulse);
                outer.setPosition(wx,wy);
                outer.setFillColor(sf::Color(120,40,220,
                    uint8_t(120+50*std::sin(gameTime*2))));
                outer.setOutlineColor(sf::Color(200,140,255,200));
                outer.setOutlineThickness(3);
                window.draw(outer);
                sf::CircleShape inner(r*0.6f*pulse,20);
                inner.setOrigin(r*0.6f*pulse,r*0.6f*pulse);
                inner.setPosition(wx,wy);
                inner.setFillColor(sf::Color(220,180,255,
                    uint8_t(180+60*std::sin(gameTime*5))));
                window.draw(inner);
                if(fontLoaded) {
                    sf::Text lbl; lbl.setFont(font);
                    std::string dest=e.props.getStr("target_zone","");
                    lbl.setString(dest.empty()?"Портал":dest);
                    lbl.setCharacterSize(11);
                    lbl.setFillColor(sf::Color(220,180,255,220));
                    auto b=lbl.getLocalBounds();
                    lbl.setOrigin(b.width/2,b.height);
                    lbl.setPosition(wx,wy-r*1.8f);
                    window.draw(lbl);
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
                    hint.setString("[E] "+e.name);
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
            sf::Text txt; txt.setFont(font); txt.setString(ft.text);
            txt.setCharacterSize(20); txt.setFillColor(ft.color);
            auto b=txt.getLocalBounds();
            txt.setOrigin(b.width/2,b.height/2);
            txt.setPosition(ft.pos.x,ft.pos.y);
            window.draw(txt);
        }
    }

    void drawHUD(){
        if(!fontLoaded) return;
        drawWorldBar(window, 20,20,200,20,player.hp/player.maxHp,sf::Color(200,50,50));
        drawWorldBar(window, 20,50,200,20,player.mp/player.maxMp,sf::Color(50,100,200));
        sf::Text levelTxt; levelTxt.setFont(font); levelTxt.setString("Lvl "+std::to_string(player.level));
        levelTxt.setCharacterSize(16); levelTxt.setFillColor(sf::Color(200,200,100));
        levelTxt.setPosition(20,85);
        window.draw(levelTxt);
    }

    void drawStatsPanel(){
        if(!fontLoaded) return;
        drawRoundedRect(window,WINDOW_W-280,20,260,300,sf::Color(30,20,40),sf::Color(100,80,150),2);
        sf::Text title; title.setFont(font); title.setString("Stats");
        title.setCharacterSize(18); title.setFillColor(sf::Color(200,200,100));
        title.setPosition(WINDOW_W-270,30); window.draw(title);
    }

    void drawInventoryPanel(){
        if(!fontLoaded) return;
        drawRoundedRect(window,20,WINDOW_H-280,260,260,sf::Color(30,20,40),sf::Color(100,80,150),2);
        sf::Text title; title.setFont(font); title.setString("Inventory");
        title.setCharacterSize(18); title.setFillColor(sf::Color(200,200,100));
        title.setPosition(30,WINDOW_H-270); window.draw(title);
    }

    void drawMinimap(){
        drawRoundedRect(window,WINDOW_W-180,WINDOW_H-180,160,160,sf::Color(40,30,50,150),sf::Color(100,80,150),1);
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
    GameEngine engine;
    engine.run();
    return 0;
}