# Changelog

All notable changes to Aethoria: Eternal Realms are documented here.

---

## [Unreleased]

### Planned
- Autosave with backup rotation
- File watcher / hot-reload for scenes
- Asset database with auto-discovery
- Portal placement tool in editor
- Region and instance system

---

## [0.4.0] — 2025

### Added
- **Prefab System** — 60+ built-in entity templates across 5 categories
- **PrefabSystemTab** in editor with search, categories, and one-click placement
- **Multi-scene support** — `SceneManager` with named scenes, transitions, metadata
- **Locations tab** in editor — manage all scenes from one panel
- **Animation pipeline** — video/GIF → spritesheet with auto-alpha detection
- **Entity inspector** — right-click any entity on map to edit its properties

### Fixed
- `PrefabSystemTab` crashed on startup with `AttributeError: _list_frame`
  — `_select_cat()` was called before `_build_center()` created `_list_frame`
- NPC placement from Prefabs panel did not work
  — `placing_entity` was never set when placing from prefab panel
- Enemy spawned inside player on game start
  — `parseEntity()` silently ignored all `enemy` entries from JSON,
    causing `spawnEnemies()` to fall back to random positions
- `animations.json` had invalid JSON (unclosed `player` block, `wolf` as
  separate root object) — engine could not load any animations
- Editor saved to `map.json` but engine read `scenes/aethoria_city.json`
  — these were always out of sync; editor now auto-syncs on every save

---

## [0.3.0] — 2025

### Added
- **Quest Editor** tab with full quest chain support
- **Dialogue Editor** with per-NPC, per-event dialogue lines
- **HUD/UI Editor** tab
- **Character Select** editor tab with class and skin configuration
- `EntitySystem::save()` and `EntitySystem::load()` — JSON persistence
- `AudioManager` — spatial sound, pitch variation, music fade
- Minimap in editor

### Changed
- `worldMap` array type changed from `TileType` to `SceneTile`
  for richer per-tile data (color, variant, walkable override)

---

## [0.2.0] — 2025

### Added
- Full tile map editor with paint, erase, fill, eyedropper tools
- Entity placement: enemies, NPCs, objects, zones
- Undo/Redo stack (Ctrl+Z / Ctrl+Y)
- Collision layer editor and visualization
- Spawn point placement
- `EntityRegistry` — unified entity storage with typed ID generation
- `AnimationSystem` — spritesheet loader and per-entity animation player

### Fixed
- Camera pan and zoom now respect tile boundaries
- Flood fill no longer escapes map bounds

---

## [0.1.0] — 2025

### Added
- Initial project structure
- Basic tile rendering with SFML
- Player movement (WASD) with collision
- Simple enemy AI (IDLE → AGGRO → COMBAT → RETURN)
- Login screen and character select screens
- Skill system (4 slots, cooldowns, mana cost)
- Particle system (blood, magic, heal, critical)
- Floating damage numbers
- HP/MP regeneration
- Basic save/load (binary format)
- `json_parser.hpp` — lightweight zero-dependency JSON parser
