<div align="center">

```
 █████╗ ███████╗████████╗██╗  ██╗ ██████╗ ██████╗ ██╗ █████╗
██╔══██╗██╔════╝╚══██╔══╝██║  ██║██╔═══██╗██╔══██╗██║██╔══██╗
███████║█████╗     ██║   ███████║██║   ██║██████╔╝██║███████║
██╔══██║██╔══╝     ██║   ██╔══██║██║   ██║██╔══██╗██║██╔══██║
██║  ██║███████╗   ██║   ██║  ██║╚██████╔╝██║  ██║██║██║  ██║
╚═╝  ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝╚═╝  ╚═╝
                    E T E R N A L   R E A L M S
```

**A handcrafted MMORPG engine built from scratch in C++ with SFML**

[![License](https://img.shields.io/badge/License-Proprietary-red.svg)](#-license)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![SFML](https://img.shields.io/badge/SFML-2.6.1-green.svg)](https://www.sfml-dev.org/)
[![Python](https://img.shields.io/badge/Editor-Python%203.10-yellow.svg)](https://python.org)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)](#-building)

</div>

---

## ✨ Overview

**Aethoria: Eternal Realms** is a from-scratch MMORPG engine and world editor.
No Unity. No Unreal. Pure C++17 + SFML — every system written by hand.

The project includes a **full-featured map editor** built in Python/Tkinter
that exports directly to the engine's scene format, with automatic sync
so the engine always sees the latest changes.

---

## 🎮 Features

<table>
<tr>
<td width="50%">

### Engine
- ⚔️ Real-time combat with skills & combos
- 👾 AI enemy system (Idle → Patrol → Aggro → Combat)
- 🗺️ Tile-based world with 120×120 maps
- 🧙 Entity system: NPC, enemies, objects, portals
- 🎬 Spritesheet animation system
- 🔊 Spatial audio manager
- 🌐 Multi-scene world with transitions
- 💾 JSON-based save/load
- 🎨 Particle & floating text effects

</td>
<td width="50%">

### World Editor
- 🖌️ Full tile painting with flood fill
- 👺 Enemy & NPC placement
- 📦 Prefab system (60+ built-in templates)
- 🎭 Dialogue & quest editor
- 🎬 Animation pipeline (video → spritesheet)
- ⚙️ Game config editor
- 🌍 Multi-scene manager
- ↩️ Undo / Redo support
- 🔄 Auto-sync editor → engine

</td>
</tr>
</table>

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    AETHORIA ENGINE                          │
├──────────────┬──────────────┬──────────────┬───────────────┤
│ SceneSystem  │ EntitySystem │  Animation   │  AudioManager │
│              │              │   System     │               │
│ • Load/Save  │ • NPC        │ • Spritesheet│ • SFX         │
│ • Transition │ • Enemies    │ • Clips      │ • Music       │
│ • Multi-map  │ • Objects    │ • Players    │ • Spatial     │
│              │ • Portals    │              │               │
├──────────────┴──────────────┴──────────────┴───────────────┤
│                      GameEngine                             │
│  Player • Camera • Combat • Particles • UI • Save/Load     │
├─────────────────────────────────────────────────────────────┤
│                    JSON / Assets                            │
│  map.json  •  scenes/*.json  •  animations.json  •  ...    │
└─────────────────────────────────────────────────────────────┘
           ▲ auto-sync on save
┌─────────────────────────────────────────────────────────────┐
│                   WORLD EDITOR (Python)                     │
│  Map Tab • Prefabs • Quests • Dialogues • Locations • HUD  │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
aethoria/
├── 📂 src/
│   ├── main.cpp                 # GameEngine — entry point
│   ├── scene_system.hpp         # Scene load/save/switch
│   ├── entity_system.hpp        # Universal entity system
│   ├── animation_system.hpp     # Spritesheet animations
│   ├── audio_manager.hpp        # Sound & music
│   ├── prefab_system.hpp        # Object templates
│   ├── json_parser.hpp          # Lightweight JSON parser
│   └── aethoria_engine.hpp      # Engine constants & helpers
│
├── 📂 editor/
│   └── aethoria_editor3.py      # Full world editor (Tkinter)
│
├── 📂 assets/
│   ├── map.json                 # Main world map (editor output)
│   ├── animations.json          # Animation configs
│   ├── game_config.json         # Engine settings
│   ├── dialogues.json           # NPC dialogue trees
│   ├── quests.json              # Quest definitions
│   ├── ui_config.json           # UI layout & colors
│   ├── prefabs.json             # Custom prefab templates
│   ├── 📂 scenes/               # Scene files (read by engine)
│   │   └── aethoria_city.json
│   ├── 📂 textures/sprites/     # Character spritesheets (.png)
│   ├── 📂 sounds/               # Sound effects (.wav)
│   ├── 📂 music/                # Background music (.ogg)
│   └── 📂 fonts/                # Font files (.ttf)
│
├── CMakeLists.txt               # Build configuration
├── build.bat                    # Windows one-click build
├── build.sh                     # Linux/macOS build
├── LICENSE                      # Proprietary — read before use
└── README.md
```

---

## 🔨 Building

### Windows

**Requirements:** [CMake 3.16+](https://cmake.org/download/) · [SFML 2.6.1](https://www.sfml-dev.org/download/sfml/2.6.1/) · Visual Studio 2019/2022 or MinGW

```bat
REM 1. Extract SFML to C:\SFML-2.6.1
REM 2. Run:
build.bat
```

The script auto-detects Visual Studio 2022 → 2019 → MinGW in that order.

**Manual build:**
```bat
mkdir build && cd build
cmake .. -DSFML_DIR="C:\SFML-2.6.1\lib\cmake\SFML" -DCMAKE_PREFIX_PATH="C:\SFML-2.6.1"
cmake --build . --config Release
```

### Linux

```bash
sudo apt-get install cmake build-essential libsfml-dev
chmod +x build.sh && ./build.sh
```

### macOS

```bash
brew install cmake sfml
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(sysctl -n hw.ncpu)
```

---

## 🗺️ World Editor

```bash
# Python 3.10+ required
# Optional — for video-to-spritesheet conversion:
pip install pillow opencv-python

# Launch
python editor/aethoria_editor3.py
```

### Editor → Engine Sync

Every time you press **Ctrl+S** in the editor:

```
editor saves  ──►  assets/map.json
                         │
                         └──►  assets/scenes/<scene_id>.json  ──►  engine ✓
```

No manual copying. No restarts. Save and run.

### Editor Tabs

| Tab | Description |
|-----|-------------|
| 🗺 World Map | Tile painting, entity placement, collision editing |
| 🎬 Assets | Video → spritesheet pipeline (ffmpeg / OpenCV) |
| ⚔ Quests | Quest chain editor with objectives and rewards |
| 💬 Dialogues | NPC dialogue trees per enemy/NPC type |
| ⚙ Config | Game settings, spawn points, ambient lighting |
| 🔑 Login Screen | Login UI text and color customization |
| 🧙 Characters | Character select screen — classes and skins |
| 🎮 HUD/UI | In-game HUD layout configuration |
| 🌍 Locations | Multi-scene manager, scene metadata |
| ⭐ Prefabs | 60+ drag-and-drop entity templates |

---

## 🎮 Controls

| Key | Action |
|-----|--------|
| `W` `A` `S` `D` | Move |
| `1` `2` `3` `4` | Use skills |
| `7` | HP Potion |
| `8` | MP Potion |
| `E` | Interact with NPC / object |
| `C` | Character stats panel |
| `Tab` | Inventory |
| `M` | Minimap |
| `LMB` | Select target |
| `Scroll wheel` | Zoom camera |

---

## 🧩 Systems

<details>
<summary><b>⚔️ Scene System</b></summary>

Handles loading, saving, and switching between named scenes.
Each scene is a JSON file in `assets/scenes/`. Supports fade transitions,
per-scene spawn points, ambient color, and entity lists.
The editor auto-syncs to the correct scene file on every save.

</details>

<details>
<summary><b>🧙 Entity System</b></summary>

Universal entity type: `PLAYER` · `ENEMY` · `NPC` · `OBJECT` · `PORTAL` · `ITEM_DROP`.
Each entity has a unique `uint32_t` ID, world position, and a typed property bag
(`strings`, `floats`, `ints`, `bools`). Supports save/load to JSON.

</details>

<details>
<summary><b>🎬 Animation System</b></summary>

Loads spritesheet configs from `animations.json`. Supports per-entity,
per-action clips with configurable FPS, loop, and frame dimensions.
Smart alpha detection — automatically removes white/black backgrounds
from sprites that lack a proper alpha channel.

</details>

<details>
<summary><b>⭐ Prefab System</b></summary>

60+ built-in templates across 5 categories: enemies, NPCs, objects, zones, portals.
Custom prefabs saved to `prefabs.json`. One-click placement from editor panel
directly onto the world map with full property inheritance.

</details>

<details>
<summary><b>🔊 Audio Manager</b></summary>

Manages sound effects and background music via SFML Audio.
Supports spatialized playback (distance-based volume falloff),
pitch variation for natural-sounding repeated effects,
and smooth music transitions.

</details>

---

## 📋 Roadmap

- [x] Core engine (render, input, camera)
- [x] Entity system with JSON persistence
- [x] Scene system with transitions
- [x] World editor with 10 tabs
- [x] Prefab system (60+ templates)
- [x] Animation pipeline
- [x] Editor → engine auto-sync
- [ ] Autosave with backup rotation
- [ ] Hot-reload (engine detects file changes)
- [ ] Asset database with auto-discovery
- [ ] Portal placement tool in editor
- [ ] Region / instance system
- [ ] Multiplayer foundation

---

## 📜 License

**© 2025 PapaZ — All Rights Reserved**

This source code is published for **viewing and educational purposes only.**
Copying, modifying, distributing, or using any part of this code
in your own projects is **strictly prohibited** without written permission.

See [LICENSE](LICENSE) for the complete terms.

---

<div align="center">

*No engine. No framework. Just code.*

**Built entirely from scratch.**

</div>
