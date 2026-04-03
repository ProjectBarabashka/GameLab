# 🗺️ World Editor Documentation

## Starting the Editor

```bash
python editor/aethoria_editor3.py
```

The editor auto-detects `project_root` from its own file location
and loads `assets/map.json` on startup.

---

## Tab Reference

### 🗺 World Map

The main editing canvas.

**Tools:**
| Tool | Key | Description |
|------|-----|-------------|
| Paint | — | Draw tiles with selected brush |
| Erase | — | Replace tiles with GRASS |
| Fill | — | Flood fill a region |
| Select | ☝ | Click entity to select/inspect |
| Entity | 👾 | Place enemies, NPCs, objects, zones |
| Eyedropper | 💧 | Pick tile from canvas |

**Entity placement:**
1. Select entity kind (Enemy / NPC / Object / Zone) in the right panel
2. Choose type from the dropdown
3. Click **+ Add** button — cursor changes to `+`
4. Click anywhere on the map

**Keyboard shortcuts:**
| Shortcut | Action |
|----------|--------|
| `Ctrl+S` | Save + sync to scenes/ |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `N` | Switch to entity tool |
| Mouse wheel | Zoom |
| Middle drag | Pan |

**Right-click** on an entity opens the **Inspector** — edit position,
level, HP, name, and any custom props.

---

### 🎬 Assets (Animation Pipeline)

Convert videos/GIFs to spritesheets for the engine.

1. Drag or browse a `.mp4`, `.gif`, or `.png` file
2. Set target frame size (default: 64×64)
3. Set FPS (default: 12)
4. Click **Convert** — generates:
   - `assets/textures/sprites/<entity>_<action>.png`
   - Updates `assets/animations.json`

The system auto-detects entity name and action from the filename:
- `player_run.mp4` → entity=`player`, action=`run`
- `goblin_idle.gif` → entity=`goblin`, action=`idle`

---

### ⚔ Quests

Create and edit quest chains.

**Quest structure:**
```
Quest
├── id, name, level_req, type (kill/collect/escort)
├── giver_npc (NPC name)
├── objectives[]
│   ├── type: kill / collect
│   ├── target: enemy/item name
│   └── count
├── rewards
│   ├── gold_min / gold_max
│   ├── xp
│   └── items[]
└── dialogue
    ├── offer[]
    ├── progress[]
    └── complete[]
```

Saved to `assets/quests.json`.

---

### 💬 Dialogues

Edit NPC/enemy dialogue lines per event type.

**Supported dialogue keys:**
- `aggro` — enemy spots player
- `combat` — during fight
- `death` — enemy dies
- `idle` — ambient lines
- `greeting` — NPC says hello
- `trade` — shop NPC
- `farewell` — end conversation

Saved to `assets/dialogues.json`.

---

### ⭐ Prefabs

Ready-made entity templates for fast placement.

**Built-in categories:**
| Category | Count | Examples |
|----------|-------|---------|
| Enemies | 10 | Goblin, Wolf Alpha, Dragon Boss |
| NPCs | 8 | Vendor, Healer, Quest Giver |
| Objects | 12 | Chest, Portal, Altar, Waypoint |
| Zones | 6 | Safe Zone, Dungeon, Boss Arena |
| Portals | 5 | City Gate, Dungeon Entrance |

**To place a prefab:**
1. Select category on the left
2. Click prefab in the list — properties appear on the right
3. Click **🗺 Place on Map** — editor switches to map tab in entity mode
4. Click on the map to place

**Custom prefabs** can be saved, exported, and imported as JSON.

---

## Editor → Engine Data Flow

```
┌─────────────────────────────────────┐
│  Editor (Python)                    │
│                                     │
│  MapData.registry  ←──── user edits │
│       │                             │
│  to_dict() ────────────────────────►│
│       │                             │
│  assets/map.json  (primary save)    │
│       │                             │
│  _sync_to_scenes() ────────────────►│
│       │                             │
│  assets/scenes/aethoria_city.json   │
└──────────────────┬──────────────────┘
                   │
                   ▼
┌─────────────────────────────────────┐
│  Engine (C++)                       │
│                                     │
│  loadStartScene()                   │
│    → copies map.json → scenes/      │
│    → SceneManager::loadDefault()    │
│    → parseEntity() per entity       │
│    → applySceneData()               │
│    → spawnEnemies()                 │
└─────────────────────────────────────┘
```

---

## Adding a New Tile Type

**1. In editor** (`aethoria_editor3.py`, `TILES` dict):
```python
TILES["LAVA"] = {
    "name": "Lava",
    "color": "#ff4400",
    "walkable": False,
    "emoji": "🌋"
}
```

**2. In engine** (`src/scene_system.hpp`, `SceneTileType` enum + `tileFromStr`):
```cpp
enum class SceneTileType { ..., LAVA };

// in tileFromStr:
{"LAVA", {T::LAVA, false, sf::Color(255,68,0)}}
```

**3. In engine** (`src/main.cpp`, `drawMap()` switch):
```cpp
case SceneTileType::LAVA:
    // draw lava animation
    break;
```

---

## Adding a New Entity Kind

**1. Editor** — add to `EntityRegistry.KIND_PREFIX` and the entity panel UI.

**2. Engine** — add to `EntityType` enum in `entity_system.hpp`.

**3. Scene parser** — add a branch in `parseEntity()` in `scene_system.hpp`:
```cpp
else if (kind == "trap") {
    es.addEntityFull(EntityType::OBJECT, wx, wy, nm, ep);
}
```

**4. Renderer** — add a draw branch in `drawEntities()` in `main.cpp`.
