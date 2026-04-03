// audio_manager.hpp - Управление звуками и музыкой

#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <vector>
#include <string>
#include <memory>

class AudioManager {
private:
    std::map<std::string, std::unique_ptr<sf::SoundBuffer>> soundBuffers;
    std::vector<sf::Sound> activeSounds;
    
    // Background music
    sf::Music backgroundMusic;
    float musicVolume;
    float sfxVolume;
    
    // Sound effect metadata
    struct SoundEffect {
        std::string name;
        std::string filePath;
        float volume;
        float pitch;
        bool loop;
    };
    
    std::vector<SoundEffect> soundEffects;
    
public:
    AudioManager() : musicVolume(70.0f), sfxVolume(80.0f) {
        initializeSoundEffects();
    }
    
    ~AudioManager() {
        stopAllSounds();
        if (backgroundMusic.getStatus() == sf::Music::Playing) {
            backgroundMusic.stop();
        }
    }
    
    void initializeSoundEffects() {
        // Define all game sounds
        soundEffects.push_back({"slash", "assets/sounds/slash.wav", 0.8f, 1.0f, false});
        soundEffects.push_back({"fireball", "assets/sounds/fireball.wav", 0.9f, 1.0f, false});
        soundEffects.push_back({"heal", "assets/sounds/heal.wav", 0.7f, 1.0f, false});
        soundEffects.push_back({"damage", "assets/sounds/damage.wav", 0.8f, 1.0f, false});
        soundEffects.push_back({"critical", "assets/sounds/critical.wav", 0.9f, 1.2f, false});
        soundEffects.push_back({"levelup", "assets/sounds/levelup.wav", 0.8f, 1.0f, false});
        soundEffects.push_back({"death", "assets/sounds/death.wav", 0.7f, 0.9f, false});
        soundEffects.push_back({"pickup", "assets/sounds/pickup.wav", 0.6f, 1.0f, false});
        soundEffects.push_back({"ui_click", "assets/sounds/ui_click.wav", 0.5f, 1.0f, false});
    }
    
    bool loadSound(const std::string& name, const std::string& filePath) {
        auto buffer = std::make_unique<sf::SoundBuffer>();
        if (!buffer->loadFromFile(filePath)) {
            return false;
        }
        soundBuffers[name] = std::move(buffer);
        return true;
    }
    
    bool loadBackgroundMusic(const std::string& filePath) {
        if (!backgroundMusic.openFromFile(filePath)) {
            return false;
        }
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(musicVolume);
        return true;
    }
    
    void playSound(const std::string& soundName, float volume = 1.0f, float pitch = 1.0f) {
        auto it = soundBuffers.find(soundName);
        if (it == soundBuffers.end()) {
            return; // Sound not loaded
        }
        
        sf::Sound sound(*it->second);
        sound.setVolume(sfxVolume * volume);
        sound.setPitch(pitch);
        sound.play();
        
        activeSounds.push_back(sound);
    }
    
    void playMusicTrack(const std::string& filePath) {
        if (backgroundMusic.getStatus() == sf::Music::Playing) {
            backgroundMusic.stop();
        }
        loadBackgroundMusic(filePath);
        backgroundMusic.play();
    }
    
    void playMusicFadeIn(const std::string& filePath, float duration = 2.0f) {
        playMusicTrack(filePath);
        backgroundMusic.setVolume(0.0f);
    }
    
    void playMusicFadeOut(float duration = 2.0f) {
        // This would need to be called repeatedly with delta time
        if (backgroundMusic.getStatus() == sf::Music::Playing) {
            backgroundMusic.stop();
        }
    }
    
    void stopAllSounds() {
        for (auto& sound : activeSounds) {
            if (sound.getStatus() == sf::Sound::Playing) {
                sound.stop();
            }
        }
        activeSounds.clear();
    }
    
    void pauseMusic() {
        if (backgroundMusic.getStatus() == sf::Music::Playing) {
            backgroundMusic.pause();
        }
    }
    
    void resumeMusic() {
        if (backgroundMusic.getStatus() == sf::Music::Paused) {
            backgroundMusic.play();
        }
    }
    
    void setMusicVolume(float volume) {
        musicVolume = std::max(0.0f, std::min(100.0f, volume));
        backgroundMusic.setVolume(musicVolume);
    }
    
    void setSFXVolume(float volume) {
        sfxVolume = std::max(0.0f, std::min(100.0f, volume));
    }
    
    float getMusicVolume() const { return musicVolume; }
    float getSFXVolume() const { return sfxVolume; }
    
    void update(float deltaTime) {
        // Remove finished sounds
        activeSounds.erase(
            std::remove_if(activeSounds.begin(), activeSounds.end(),
                [](const sf::Sound& sound) { return sound.getStatus() != sf::Sound::Playing; }),
            activeSounds.end()
        );
    }
    
    // Advanced audio effects
    void playSoundWithVariation(const std::string& soundName, float pitchVariation = 0.1f) {
        float pitch = 1.0f + ((rand() % 200 - 100) / 1000.0f) * pitchVariation;
        playSound(soundName, 1.0f, pitch);
    }
    
    void playSoundSpatialized(const std::string& soundName, float x, float y, 
                             float centerX, float centerY, float maxDistance = 500.0f) {
        float dist = std::hypot(x - centerX, y - centerY);
        if (dist > maxDistance) return;
        
        float volume = 1.0f - (dist / maxDistance);
        float pan = (x - centerX) / 200.0f;
        
        playSound(soundName, volume);
        
        // Pan would need to be applied to individual sound instance
    }
};
