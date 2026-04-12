<div align="center">

<img src="logo.ico" width="110"/>

# AETHORIA: Eternal Realms

**MMORPG Engine (C++17 + SFML) with integrated Python Editor**

</div>

---

## 🌍 Language / Язык

**🇷🇺 Русский** | [🇺🇸 English](#english)

---

# 🇷🇺 Русская версия

## ✨ Обзор

**Aethoria: Eternal Realms** — это самописный 2D MMORPG-движок с полноценным пайплайном разработки.

Проект включает:

* движок на C++17 (SFML)
* встроенную игровую логику (AI, combat, entities)
* редактор на Python (Tkinter)
* data-driven систему (JSON)

Без использования сторонних игровых движков.

---

## 🎮 Движок

* Реалтайм боевая система
* AI: `Idle → Patrol → Aggro → Combat`
* Тайловый мир
* Entity system (NPC, враги, порталы)
* Animation system (spritesheets)
* Audio (music + SFX)
* Scene system
* Загрузка/сохранение через JSON

---

## 🛠 Редактор (editor/)

Функциональность:

* Редактор карт
* Размещение NPC и врагов
* Префабы
* Квесты и диалоги
* Конфигурация
* Undo / Redo

Все изменения сохраняются в JSON и сразу читаются движком.

---

## 🔄 Pipeline

```
Editor → JSON → Engine
```

Без промежуточных форматов.

---

## 📦 Assets

```
assets/
├── animations/
├── fonts/
├── maps/
├── music/
├── scenes/
├── sounds/
├── textures/
├── dialogues.json
├── items.json
├── quests.json
├── prefabs.json
├── game_config.json
├── server_config.json
└── ui_config.json
```

---

## 🏗 Архитектура

```
ENGINE (C++)
├── Core
├── Scene System
├── Entity System
├── Animation
└── Audio

EDITOR (Python)
├── Map Editor
├── Quests
├── Dialogues
└── Config

DATA
└── JSON
```

---

## 📁 Структура проекта

```
.
├── assets/
├── build/
├── docs/
├── editor/
├── src/
├── saves/
├── CMakeLists.txt
├── build.sh
├── build.bat
├── logo.ico
├── resources.rc
├── resources.res
```

---

## 🔨 Сборка

### 🐧 Linux / 🍎 macOS

```bash
./build.sh
```

Скрипт:

* проверяет `cmake` и компилятор
* проверяет SFML (через pkg-config)
* собирает проект (multi-core)
* после сборки предлагает запуск

---

### 🪟 Windows

```bat
build.bat
```

---

## ⚙️ Особенности сборки

CMake:

* рекурсивно собирает `src/*.cpp`
* подключает SFML:

  * graphics
  * window
  * audio
  * system
* использует `resources.rc` (иконка exe)

Post-build:

```
assets → build/
SFML DLL → build/
```

👉 после сборки проект сразу запускается

---

## ▶️ Запуск

```
./build/AETHORIA
```

---

## 🗺 Редактор

```
python editor/aethoria_editor3.py
```

---

## 🎮 Управление

| Кнопка | Действие    |
| ------ | ----------- |
| WASD   | движение    |
| 1–4    | способности |

---

# 🇺🇸 English

## ✨ Overview

**Aethoria: Eternal Realms** is a custom 2D MMORPG engine with a full development pipeline.

Includes:

* C++17 engine (SFML)
* gameplay systems (AI, combat, entities)
* Python editor (Tkinter)
* JSON-based data system

No external game engines.

---

## 🎮 Engine

* Real-time combat
* AI: `Idle → Patrol → Aggro → Combat`
* Tile-based world
* Entity system
* Animation system
* Audio system
* Scene system
* JSON save/load

---

## 🛠 Editor

Located in `editor/`

* Map editor
* NPC / enemy placement
* Prefabs
* Quests & dialogues
* Config editor
* Undo / Redo

---

## 🔄 Pipeline

```
Editor → JSON → Engine
```

---

## 🔨 Build

### Linux / macOS

```
./build.sh
```

### Windows

```
build.bat
```

---

## ▶️ Run

```
./build/AETHORIA
```

---

## 🗺 Editor

```
python editor/aethoria_editor3.py
```

---

## 📄 License

See LICENSE

---
