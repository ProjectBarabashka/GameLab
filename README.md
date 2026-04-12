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

**MMORPG Engine (C++17 + SFML) with integrated Python Editor**

</div>

---

## 🌍 Language / Язык

**🇷🇺 Русский** | [🇺🇸 English](#english)

---

# 🇷🇺 Русская версия

## ✨ Обзор

**Aethoria: Eternal Realms** — это полноценный MMORPG-движок, написанный с нуля на C++17 с использованием SFML.

Ключевая идея проекта:

* собственная архитектура
* data-driven подход (JSON)
* встроенный инструмент разработки (редактор)

Проект не использует сторонние игровые движки.

---

## 🎮 Возможности

### ⚔ Движок

* Реалтайм боевая система
* AI: Idle → Patrol → Aggro → Combat
* Тайловая карта
* Entity system (NPC, враги, порталы)
* Система анимаций (spritesheets)
* Аудио (музыка + SFX)
* Scene system
* Загрузка/сохранение через JSON
* Визуальные эффекты

---

### 🛠 Редактор (Python / Tkinter)

* Редактор карт
* Размещение NPC и врагов
* Префабы
* Квесты и диалоги
* Конфигурация игры
* Undo / Redo
* Прямая синхронизация

---

## 🎨 Asset Pipeline

Проект использует полностью **JSON-ориентированную систему ассетов**.

### 📦 Структура

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

### 🗺 Сцены

Сцены задаются через JSON и управляют:

* логикой зоны
* PvP
* ограничениями уровня
* параметрами окружения

---

### 🔄 Интеграция

* редактор генерирует JSON
* движок читает напрямую
* без промежуточных форматов

---

## 🏗 Архитектура

```
ENGINE (C++)
├── Scene System
├── Entity System
├── Animation
├── Audio
└── Core

EDITOR (Python)
├── Map Editor
├── Quests
├── Dialogues
└── Config

DATA
└── JSON + Assets
```

---

## 📁 Структура проекта

```
project_root/
├── assets/
├── build/
├── docs/
├── editor/
├── saves/
├── src/
├── CMakeLists.txt
├── build.sh
├── build.bat
└── README.md
```

---

## 🔨 Сборка

### 🐧 Linux / 🍎 macOS

```
./build.sh
```

Что делает скрипт:

* проверяет cmake и компилятор
* проверяет SFML
* собирает проект
* предлагает сразу запустить игру или редактор

---

### 🪟 Windows

```
build.bat
```

---

## ⚙️ Особенности сборки

* CMake автоматически подключает SFML
* после сборки:

  * копируются **assets/**
  * копируются **SFML DLL**

👉 проект запускается без ручной настройки

---

## ▶️ Запуск

После сборки:

```
./build/AETHORIA
```

или через build.sh (интерактивно)

---

## 🗺 Редактор

```
python editor/aethoria_editor3.py
```

---

## 🔄 Workflow

```
Редактор → JSON → Движок
```

* Ctrl+S в редакторе
* данные сразу доступны игре

---

## 🎮 Управление

| Кнопка | Действие    |
| ------ | ----------- |
| WASD   | движение    |
| 1–4    | способности |

---

---

# 🇺🇸 English

## ✨ Overview

**Aethoria: Eternal Realms** is a custom-built MMORPG engine written in C++17 using SFML.

Core principles:

* custom architecture
* data-driven design (JSON)
* integrated toolchain (editor)

---

## 🎮 Features

### Engine

* Real-time combat
* AI system
* Tile-based world
* Entity system
* Animation system
* Audio system
* Scene system
* JSON save/load

---

### Editor

* Map editor
* NPC placement
* Prefabs
* Quests & dialogues
* Config editor
* Undo/Redo
* Auto-sync

---

## 🎨 Assets

```
assets/
├── animations/
├── fonts/
├── maps/
├── music/
├── scenes/
├── sounds/
├── textures/
└── *.json
```

* JSON-based pipeline
* Sprite animations
* Scene definitions

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
