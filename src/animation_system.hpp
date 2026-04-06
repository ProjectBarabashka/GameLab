// animation_system.hpp - Полная система загрузки и воспроизведения анимаций
// ИСПРАВЛЕНО: facing/flip корректно работает с любым origin
// ИСПРАВЛЕНО: setPosition учитывает направление при отражении спрайта

#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "json_parser.hpp"


// ════════════════════════════════════════════════════════════════
// СТРУКТУРЫ АНИМАЦИИ
// ════════════════════════════════════════════════════════════════
struct AnimationFrame {
    sf::IntRect rect;  // Координаты в спрайтшите
    float duration;    // Длительность кадра в секундах
};

struct AnimationClip {
    std::string name;
    std::string sheetPath;
    std::vector<AnimationFrame> frames;
    float fps;
    bool loop;
    int frameWidth, frameHeight;
};

// ════════════════════════════════════════════════════════════════
// МЕНЕДЖЕР АНИМАЦИЙ
// ════════════════════════════════════════════════════════════════
class AnimationManager {
private:
    std::map<std::string, std::map<std::string, AnimationClip>> animations;
    std::map<std::string, sf::Texture> textures;
    std::string assetsDir;
    
public:
    AnimationManager(const std::string& assetsDirPath = "assets") 
        : assetsDir(assetsDirPath) {}
    
    bool loadAnimationsJSON(const std::string& jsonPath) {
        std::ifstream file(jsonPath);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Не удалось открыть JSON: " << jsonPath << std::endl;
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonContent = buffer.str();
        file.close();
        
        auto json = SimpleJSON::Parser::parse(jsonContent);
        if (!json || !json->isObject()) {
            std::cerr << "[ERROR] JSON не является объектом" << std::endl;
            return false;
        }
        
        for (auto& [entity, entityVal] : json->objectVal) {
            if (!entityVal->isObject()) continue;
            
            for (auto& [action, actionVal] : entityVal->objectVal) {
                if (!actionVal->isObject()) continue;
                
                AnimationClip clip;
                clip.name = action;
                
                if (auto sheet = actionVal->get("sheet")) {
                    clip.sheetPath = assetsDir + "/" + sheet->asString();
                }
                if (auto w = actionVal->get("width"))  { clip.frameWidth  = w->asInt(); }
                if (auto h = actionVal->get("height")) { clip.frameHeight = h->asInt(); }
                if (auto f = actionVal->get("fps"))    { clip.fps = (float)f->asDouble(); }
                if (auto l = actionVal->get("loop"))   { clip.loop = l->asBool(); }
                
                int frames = 0, cols = 0, rows = 0;
                if (auto fr = actionVal->get("frames")) frames = fr->asInt();
                if (auto c  = actionVal->get("cols"))   cols   = c->asInt();
                if (auto r  = actionVal->get("rows"))   rows   = r->asInt();
                
                generateFrames(clip, frames, cols, rows, clip.frameWidth, clip.frameHeight);
                
                std::string texKey = entity + "_" + action;
                if (textures.find(texKey) == textures.end()) {
                    sf::Image img;
                    if (img.loadFromFile(clip.sheetPath)) {
                        auto sz = img.getSize();
                        bool hasRealAlpha = false;
                        if (sz.x > 0 && sz.y > 0) {
                            struct { unsigned x, y; } pts[] = {
                                {0, 0}, {sz.x-1, 0}, {0, sz.y-1}, {sz.x-1, sz.y-1},
                                {sz.x/4, sz.y/4}, {sz.x*3/4, sz.y/4},
                                {sz.x/4, sz.y*3/4}, {sz.x/2, sz.y/2}
                            };
                            for (auto& p : pts) {
                                if (img.getPixel(p.x, p.y).a < 200) {
                                    hasRealAlpha = true;
                                    break;
                                }
                            }
                        }
                        if (!hasRealAlpha) {
                            img.createMaskFromColor(sf::Color(255, 255, 255), 0);
                            for (unsigned py = 0; py < sz.y; py++) {
                                for (unsigned px = 0; px < sz.x; px++) {
                                    auto c = img.getPixel(px, py);
                                    if (c.r > 230 && c.g > 230 && c.b > 230)
                                        img.setPixel(px, py, sf::Color(c.r, c.g, c.b, 0));
                                }
                            }
                        }
                        sf::Texture tex;
                        tex.loadFromImage(img);
                        tex.setSmooth(false);
                        textures[texKey] = tex;
                    } else {
                        std::cerr << "[WARNING] Не удалось загрузить текстуру: " << clip.sheetPath << std::endl;
                    }
                }
                
                animations[entity][action] = clip;
                std::cout << "[OK] Загружена анимация: " << entity << "/" << action 
                         << " (" << frames << " кадров)" << std::endl;
            }
        }
        
        return !animations.empty();
    }
    
    AnimationClip* getAnimation(const std::string& entity, const std::string& action) {
        auto entityIt = animations.find(entity);
        if (entityIt == animations.end()) return nullptr;
        auto actionIt = entityIt->second.find(action);
        if (actionIt == entityIt->second.end()) return nullptr;
        return &actionIt->second;
    }
    
    sf::Texture* getTexture(const std::string& entity, const std::string& action) {
        std::string key = entity + "_" + action;
        auto it = textures.find(key);
        return it != textures.end() ? &it->second : nullptr;
    }
    
private:
    void generateFrames(AnimationClip& clip, int totalFrames, int cols, int rows, 
                       int frameW, int frameH) {
        float frameDuration = clip.fps > 0 ? 1.0f / clip.fps : 0.1f;
        int frameIndex = 0;
        
        for (int row = 0; row < rows && frameIndex < totalFrames; row++) {
            for (int col = 0; col < cols && frameIndex < totalFrames; col++) {
                AnimationFrame frame;
                frame.rect = sf::IntRect(col * frameW, row * frameH, frameW, frameH);
                frame.duration = frameDuration;
                clip.frames.push_back(frame);
                frameIndex++;
            }
        }
    }
};

// ════════════════════════════════════════════════════════════════
// ПЛЕЕР АНИМАЦИЙ
// ИСПРАВЛЕНО: правильный flip спрайта - origin ставится в центр,
//             при отражении по X offset компенсируется через position.
// ════════════════════════════════════════════════════════════════
class AnimationPlayer {
private:
    AnimationManager* manager;
    sf::Sprite sprite;
    
    std::string currentEntity;
    std::string currentAction;
    AnimationClip* currentClip;
    
    int currentFrame;
    float frameTimer;
    bool isPlaying;
    bool finished;
    
    // Направление и масштаб (отдельно от sprite.scale)
    bool  _facingLeft = false;
    float _scaleX     = 1.0f;
    float _scaleY     = 1.0f;

    // Мировая позиция (центр персонажа)
    float _worldX = 0.f;
    float _worldY = 0.f;
    
public:
    AnimationPlayer(AnimationManager* mgr = nullptr) 
        : manager(mgr), currentClip(nullptr), currentFrame(0), 
          frameTimer(0), isPlaying(false), finished(false) {
        sprite.setOrigin(0, 0);
    }
    
    // ── Воспроизведение анимации ───────────────────────────────────
    bool playAnimation(const std::string& entity, const std::string& action) {
        if (!manager) return false;
        
        // Если та же самая анимация уже играет — не перезапускаем
        if (currentEntity == entity && currentAction == action && isPlaying && !finished)
            return true;
        
        auto clip = manager->getAnimation(entity, action);
        auto tex  = manager->getTexture(entity, action);
        
        if (!clip || !tex) {
            std::cerr << "[ERROR] Анимация не найдена: " << entity << "/" << action << std::endl;
            return false;
        }
        
        currentEntity = entity;
        currentAction = action;
        currentClip   = clip;
        currentFrame  = 0;
        frameTimer    = 0;
        isPlaying     = true;
        finished      = false;
        
        sprite.setTexture(*tex);
        updateFrame();
        
        // Origin — центр кадра (для корректного позиционирования и flip)
        sprite.setOrigin(clip->frameWidth  / 2.0f,
                         clip->frameHeight / 2.0f);
        
        _applyTransform();
        return true;
    }
    
    // ── Обновление ──────────────────────────────────────────────────
    void update(float deltaTime) {
        if (!isPlaying || !currentClip || currentClip->frames.empty()) return;
        
        frameTimer += deltaTime;
        float frameDuration = currentClip->frames[currentFrame].duration;
        
        while (frameTimer >= frameDuration && !finished) {
            frameTimer -= frameDuration;
            currentFrame++;
            
            if (currentFrame >= (int)currentClip->frames.size()) {
                if (currentClip->loop) {
                    currentFrame = 0;
                } else {
                    finished   = true;
                    isPlaying  = false;
                    currentFrame = (int)(currentClip->frames.size() - 1);
                }
            }
        }
        
        updateFrame();
    }
    
    // ── Рисование ──────────────────────────────────────────────────
    void draw(sf::RenderWindow& window) {
        if (currentClip)
            window.draw(sprite);
    }
    
    // ── Getters ────────────────────────────────────────────────────
    sf::Sprite& getSprite() { return sprite; }
    bool isFinished()    const { return finished; }
    bool isAnimating()   const { return isPlaying; }
    int  getCurrentFrame()const { return currentFrame; }
    std::string getCurrentAction() const { return currentAction; }
    std::string getCurrentEntity() const { return currentEntity; }
    
    // ── Управление ──────────────────────────────────────────────────
    void stop()   { isPlaying = false; currentFrame = 0; frameTimer = 0; }
    void pause()  { isPlaying = false; }
    void resume() { if (currentClip) isPlaying = true; }
    
    // ── Масштаб ──────────────────────────────────────────────────
    // Вызывай с положительными значениями — flip управляется через setFacing()
    void setScale(float x, float y) {
        _scaleX = std::abs(x);
        _scaleY = std::abs(y);
        _applyTransform();
    }

    // ── Направление взгляда ────────────────────────────────────
    // facingLeft=true  → смотрит влево (спрайт зеркален по X)
    // facingLeft=false → смотрит вправо
    void setFacing(bool facingLeft) {
        if (_facingLeft != facingLeft) {
            _facingLeft = facingLeft;
            _applyTransform();
        }
    }

    bool isFacingLeft() const { return _facingLeft; }

    // Вызывай каждый кадр из логики движения
    // При vx ≈ 0 направление сохраняется (порог 0.5 пикс/с)
    void updateFacingFromVelocity(float vx) {
        if      (vx < -0.5f) setFacing(true);
        else if (vx >  0.5f) setFacing(false);
    }
    
    // ── Позиция (центр персонажа в мире) ──────────────────────
    // Работает правильно при любом facing — внутри компенсируется offset
    void setPosition(float x, float y) {
        _worldX = x;
        _worldY = y;
        _applyTransform();
    }
    
    void setOrigin(float x, float y) {
        sprite.setOrigin(x, y);
    }
    
private:
    void updateFrame() {
        if (currentClip && currentFrame < (int)currentClip->frames.size())
            sprite.setTextureRect(currentClip->frames[currentFrame].rect);
    }
    
    // Применяет масштаб + flip + позицию одновременно.
    // При отражении по X SFML зеркалит вокруг origin, а не вокруг позиции,
    // поэтому дополнительного сдвига не нужно — origin уже в центре кадра.
    void _applyTransform() {
        float sx = _facingLeft ? -_scaleX : _scaleX;
        sprite.setScale(sx, _scaleY);
        sprite.setPosition(_worldX, _worldY);
    }
};
