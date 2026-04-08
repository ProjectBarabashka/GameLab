<div align="center">

<img src="docs/images/logo.png" width="420"/>

<br/>

# 🔥 AETHORIA: Eternal Realms 🔥

### ⚔️ Custom MMORPG Engine • C++17 • SFML ⚔️

<p align="center">
  <img src="https://img.shields.io/badge/ENGINE-C++17-blue?style=for-the-badge"/>
  <img src="https://img.shields.io/badge/RENDER-SFML-green?style=for-the-badge"/>
  <img src="https://img.shields.io/badge/EDITOR-Python-orange?style=for-the-badge"/>
</p>

---

### 🌑 Славянская мифология • 东方幻想 • Технологический СССР 🌑

</div>

---

## 🌍 Language / Язык

**🇷🇺 Русский** • [🇺🇸 English](#english)

---

# 🇷🇺 РУССКАЯ ВЕРСИЯ

---

## ⚔️ О ПРОЕКТЕ

**Aethoria: Eternal Realms** — это кастомный MMORPG-движок, созданный с нуля.

🔥 Без готовых решений
🔥 Полный контроль над системой
🔥 Своя архитектура + свой toolchain

Проект вдохновлён смесью:

* славянской мифологии
* восточной (китайской) эстетики
* индустриального духа СССР

---

## 🧠 ФИЛОСОФИЯ

```id="philo"
DATA → JSON → ENGINE → WORLD
```

* данные управляют игрой
* логика отделена от контента
* редактор = часть движка

---

## 🎮 ДВИЖОК

### ⚔️ Core

* Реалтайм бой
* AI (FSM поведение)
* Entity system
* Scene system
* Эффекты и анимации
* Аудио система

---

### 🌍 Мир

* Тайловая карта
* NPC / враги / порталы
* JSON сцены
* Динамическая загрузка

---

### 🔥 Pipeline

* JSON-first архитектура
* Ассеты подгружаются автоматически
* Нет жёстко захардкоженных данных

---

## 🛠 РЕДАКТОР (Python)

### 🎨 Возможности

* Карта (рисование + flood fill)
* NPC / враги
* Префабы
* Квесты и диалоги
* Конфиги

---

### ⚡ Особенности

* Undo / Redo
* Быстрая итерация
* Прямая интеграция с движком

---

## 🎨 ВИЗУАЛЫ

### 🧍 Спрайты

![sprites](assets/textures/sprites_preview.png)

---

### 🗺 Мир

![world](assets/maps/world_preview.png)

---

## 📦 ASSET SYSTEM

```id="assets-tree"
assets/
├── animations/
├── fonts/
├── maps/
├── music/
├── scenes/
├── sounds/
├── textures/
├── *.json
```

---

## 🏗 АРХИТЕКТУРА

```id="arch"
ENGINE (C++)
├── Core
├── Scene
├── Entity
├── Animation
└── Audio

EDITOR (Python)
├── Map
├── Quests
├── Dialogues
└── Config
```

---

## 📁 СТРУКТУРА

```id="tree"
project_root/
├── assets/
├── build/
├── docs/images/logo.png
├── editor/
├── saves/
├── src/
├── CMakeLists.txt
├── build.sh
└── README.md
```

---

## 🔨 СБОРКА

### 🐧 Linux / 🍎 macOS

```id="build1"
./build.sh
```

---

### 🪟 Windows

```id="build2"
build.bat
```

---

## ▶️ ЗАПУСК

```id="run"
./build/AETHORIA
```

---

## 🗺 РЕДАКТОР

```id="editor"
python editor/aethoria_editor3.py
```

---

## 🔄 WORKFLOW

```id="flow"
EDITOR → JSON → ENGINE
```

---

## 🚀 ROADMAP

### 🥇 Core

* Event System
* Skill System
* Item System

---

### 🥈 Multiplayer

* Server
* Sync
* Zones

---

### 🥉 MMO

* Accounts
* Database
* Persistence

---

### 🔥 Advanced

* Visual Scripting
* Behavior Trees
* World Streaming

---

## ⚡ СТАТУС

🟢 Активная разработка
🟡 Архитектура готова
🔴 MMO системы в процессе

---

---

# 🇺🇸 ENGLISH

---

## ⚔️ OVERVIEW

Custom MMORPG engine built from scratch.

Inspired by:

* Slavic mythology
* Eastern fantasy
* Industrial USSR aesthetics

---

## 🎮 FEATURES

* Real-time combat
* AI system
* Tile world
* Entity system
* JSON pipeline
* Integrated editor

---

## 🎨 VISUALS

![sprites](assets/textures/sprites_preview.png)

![world](assets/maps/world_preview.png)

---

## 🔨 BUILD

```id="b1"
./build.sh
```

or

```id="b2"
build.bat
```

---

## ▶️ RUN

```id="b3"
./build/AETHORIA
```

---

## 🗺 EDITOR

```id="b4"
python editor/aethoria_editor3.py
```

---

## 📄 LICENSE

See LICENSE
