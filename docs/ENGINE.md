# ⚙️ Engine Documentation

## Table of Contents
- [Scene System](#scene-system)
- [Entity System](#entity-system)
- [Animation System](#animation-system)
- [Audio Manager](#audio-manager)
- [Game Config](#game-config)

---

## Scene System

**File:** `src/scene_system.hpp`

The scene system manages the entire world — loading, saving, switching between locations.

### Scene File Format

Each scene is stored in `assets/scenes/<id>.json`:

```json
{
  "version": "4.0",
  "name": "Aethoria City",
  "width": 120,
  "height": 120,
  "metadata": {
    "player_spawn_x": 60,
    "player_spawn_y": 55,
    "scene_id": "aethoria_city"
  },
  "tiles": [["GRASS", "GRASS", ...], ...],
  "collision": [[0, 0, 1, ...], ...],
  "entities": [
    { "id": "n0001", "kind": "npc",   "type": "VENDOR", "x": 52, "y": 56, "props": {...} },
    { "id": "e0001", "kind": "enemy", "type": "GOBLIN", "x": 27, "y": 78, "props": {...} }
  ]
}
```

### Entity kinds in scene JSON

| `kind` | Loaded as | Notes |
|--------|-----------|-------|
| `enemy` | `SceneEnemyDef` → `Enemy` | Spawned by `spawnEnemies()` |
| `npc` | `EntityType::NPC` | Interactive characters |
| `object` | `EntityType::OBJECT` | Chests, barrels, altars |
| `portal` | `EntityType::PORTAL` | Zone transitions |
| `zone` | Spawn point / fountains | Not an entity, modifies scene metadata |
| `fountain` | Decorative fountain | Added to `fountains` list |

### Switching Scenes

```cpp
// In GameEngine:
switchToScene("dark_forest");  // triggers 0.5s fade transition
```

The scene transition:
1. Starts a 0.5s black fade overlay
2. Calls `SceneManager::switchScene()` which clears all entities
3. Loads the new scene file
4. Fires `onSceneLoaded` callback → `applySceneData()` + `spawnEnemies()`

---

## Entity System

**File:** `src/entity_system.hpp`

### Entity Structure

```cpp
struct Entity {
    uint32_t         id;      // unique, auto-incremented
    EntityType       type;    // PLAYER/ENEMY/NPC/OBJECT/PORTAL/ITEM_DROP
    float            x, y;   // world coordinates (pixels)
    std::string      name;
    bool             active;
    EntityProperties props;  // typed key-value store
};
```

### EntityProperties

All entity data is stored in a typed bag — no hardcoded fields:

```cpp
EntityProperties props;
props.setStr  ("name",    "Goblin Scout");
props.setInt  ("level",   3);
props.setFloat("hp",      75.f);
props.setBool ("boss",    false);

// Reading:
std::string name = props.getStr("name", "Unknown");
int level        = props.getInt("level", 1);
```

### Common props by entity kind

**Enemy:**
| Key | Type | Description |
|-----|------|-------------|
| `subtype` | string | `GOBLIN`, `TROLL`, `WOLF`, etc. |
| `level` | int | Enemy level |
| `hp` | float | Max HP |
| `dmg` | int | Damage per hit |
| `boss` | bool | Boss flag (larger, more HP) |
| `gold` | int | Gold dropped on death |

**NPC:**
| Key | Type | Description |
|-----|------|-------------|
| `subtype` | string | `VENDOR`, `QUEST`, `HEALER`, etc. |
| `name` | string | Display name |
| `quest_id` | string | Linked quest ID |
| `dialogue_id` | string | Dialogue tree key |

**Object:**
| Key | Type | Description |
|-----|------|-------------|
| `subtype` | string | `CHEST`, `PORTAL`, `SIGN`, etc. |
| `opened` | bool | Already looted? |
| `gold_min/max` | int | Loot range |
| `item` | string | Item name in loot |

---

## Animation System

**File:** `src/animation_system.hpp`

### animations.json format

```json
{
  "player": {
    "idle": {
      "sheet": "textures/sprites/player_idle.png",
      "frames": 8,
      "cols":   4,
      "rows":   2,
      "width":  64,
      "height": 64,
      "fps":    12,
      "loop":   true
    },
    "run": { ... }
  },
  "goblin": {
    "idle": { ... }
  }
}
```

### Using animations in code

```cpp
// Setup (once):
AnimationManager manager("assets");
manager.loadAnimationsJSON("assets/animations.json");

// Per entity:
AnimationPlayer player(&manager);
player.playAnimation("player", "idle");
player.setScale(1.5f, 1.5f);

// Game loop:
player.update(deltaTime);
player.setPosition(x, y);
player.draw(window);
```

### Transparency handling

The system automatically detects if a spritesheet has a real alpha channel.
If not, it removes white backgrounds using color key masking:

```
Has alpha? ──► use as-is
No alpha?  ──► remove white pixels (r>230 && g>230 && b>230) → alpha=0
```

---

## Audio Manager

**File:** `src/audio_manager.hpp`

### Playing sounds

```cpp
AudioManager audio;
audio.loadSound("slash", "assets/sounds/slash.wav");

// Simple playback:
audio.playSound("slash");

// With pitch variation (natural feel):
audio.playSoundWithVariation("slash", 0.15f);

// Spatialized (distance-based volume):
audio.playSoundSpatialized("slash", enemyX, enemyY, playerX, playerY, 400.f);
```

### Background music

```cpp
audio.playMusicTrack("assets/music/city_theme.ogg");
audio.setMusicVolume(70.f);  // 0-100
audio.pauseMusic();
audio.resumeMusic();
```

---

## Game Config

**File:** `assets/game_config.json`

```json
{
  "player_spawn": { "x": 60, "y": 55 },
  "enemy_count":  8,
  "respawn_time": 60.0,
  "boss_enabled": true,
  "particle_scale": 1.0,
  "lights": {
    "ambient": { "r": 30, "g": 20, "b": 50 }
  },
  "shaders": {
    "enabled": false,
    "brightness": 1.0,
    "saturation": 1.0
  },
  "layers": {
    "ground":   true,
    "objects":  true,
    "entities": true,
    "effects":  true
  }
}
```

All values are loaded at startup by `GameEngine::loadGameConfig()`.
Modify via the **⚙ Config** tab in the editor or edit JSON directly.
