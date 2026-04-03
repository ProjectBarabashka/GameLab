<div align="center">
█████╗ ███████╗████████╗██╗ ██╗ ██████╗ ██████╗ ██╗ █████╗
██╔══██╗██╔════╝╚══██╔══╝██║ ██║██╔═══██╗██╔══██╗██║██╔══██╗
███████║█████╗ ██║ ███████║██║ ██║██████╔╝██║███████║
██╔══██║██╔══╝ ██║ ██╔══██║██║ ██║██╔══██╗██║██╔══██║
██║ ██║███████╗ ██║ ██║ ██║╚██████╔╝██║ ██║██║██║ ██║
╚═╝ ╚═╝╚══════╝ ╚═╝ ╚═╝ ╚═╝ ╚═════╝ ╚═╝ ╚═╝╚═╝╚═╝ ╚═╝
ETERNAL REALMS
✦ ВЕЧНЫЕ ЗЕМЛИ ✦

**Ручной движок MMORPG с нуля на C++ и SFML**

[![License](https://img.shields.io/badge/License-Proprietary-red.svg)](#-лицензия)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![SFML](https://img.shields.io/badge/SFML-2.6.1-green.svg)](https://www.sfml-dev.org/)
[![Python](https://img.shields.io/badge/Editor-Python%203.10-yellow.svg)](https://python.org)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)](#-сборка)

---

## 🌐 Language / Язык

<div align="center">

[![English](https://img.shields.io/badge/English-🇬🇧-blue.svg)](#english-)
[![Русский](https://img.shields.io/badge/Русский-🇷🇺-red.svg)](#russian-)

</div>

---

## <a name="english-"></a>🇬🇧 ENGLISH

### ✨ Overview

**Aethoria: Eternal Realms** is a from-scratch MMORPG engine and world editor.
No Unity. No Unreal. Pure C++17 + SFML — every system written by hand.

The project includes a **full-featured map editor** built in Python/Tkinter
that exports directly to the engine's scene format, with automatic sync
so the engine always sees the latest changes.

### 🎮 Features

<table>
<tr>
<td width="50%">

#### Engine
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

#### World Editor
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

### 🏗️ Architecture
┌─────────────────────────────────────────────────────────────┐
│ AETHORIA ENGINE │
├──────────────┬──────────────┬──────────────┬───────────────┤
│ SceneSystem │ EntitySystem │ Animation │ AudioManager │
│ │ │ System │ │
│ • Load/Save │ • NPC │ • Spritesheet│ • SFX │
│ • Transition │ • Enemies │ • Clips │ • Music │
│ • Multi-map │ • Objects │ • Players │ • Spatial │
│ │ • Portals │ │ │
├──────────────┴──────────────┴──────────────┴───────────────┤
│ GameEngine │
│ Player • Camera • Combat • Particles • UI • Save/Load │
├─────────────────────────────────────────────────────────────┤
│ JSON / Assets │
│ map.json • scenes/*.json • animations.json • ... │
└─────────────────────────────────────────────────────────────┘
▲ auto-sync on save
┌─────────────────────────────────────────────────────────────┐
│ WORLD EDITOR (Python) │
│ Map Tab • Prefabs • Quests • Dialogues • Locations • HUD │
└─────────────────────────────────────────────────────────────┘

---

### 🔨 Building

#### Windows
```bat
build.bat
Linux
bash
chmod +x build.sh && ./build.sh
macOS
bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(sysctl -n hw.ncpu)
🎮 Controls
Key	Action
W A S D	Move
1 2 3 4	Skills
7	HP Potion
8	MP Potion
E	Interact
C	Character stats
Tab	Inventory
M	Minimap
LMB	Select target
Scroll	Zoom
<a name="russian-"></a>🇷🇺 РУССКИЙ
✨ Обзор
Aethoria: Вечные Земли — это полностью самодельный MMORPG движок и редактор миров.
Никакого Unity. Никакого Unreal. Чистый C++17 + SFML — каждая система написана вручную.

Проект включает полноценный картографический редактор на Python/Tkinter,
который экспортирует данные напрямую в формат сцен движка с автоматической синхронизацией.

🎮 Возможности
<table> <tr> <td width="50%">
Движок
⚔️ Боевая система в реальном времени (скиллы и комбо)

👾 ИИ врагов (Бездействие → Патруль → Агро → Бой)

🗺️ Мир на тайлах (карты 120×120)

🧙 Сущности: NPC, враги, объекты, порталы

🎬 Система спрайтовой анимации

🔊 Пространственный звук

🌐 Многосценовый мир с переходами

💾 Сохранение/загрузка в JSON

🎨 Частицы и парящий текст

</td> <td width="50%">
Редактор миров
🖌️ Полная покраска тайлов (заливка области)

👺 Расстановка врагов и NPC

📦 Система префабов (60+ шаблонов)

🎭 Редактор диалогов и квестов

🎬 Конвертер видео → спрайт-листы

⚙️ Редактор настроек игры

🌍 Менеджер нескольких сцен

↩️ Отмена / Повтор действий

🔄 Автосинхронизация → движок

</td> </tr> </table>
🏗️ Архитектура
┌─────────────────────────────────────────────────────────────┐
│                      ДВИЖОК AETHORIA                        │
├──────────────┬──────────────┬──────────────┬───────────────┤
│ SceneSystem  │ EntitySystem │  Animation   │  AudioManager │
│              │              │   System     │               │
│ • Загрузка   │ • NPC        │ • Спрайт-лист│ • Эффекты     │
│ • Переходы   │ • Враги      │ • Клипы      │ • Музыка      │
│ • Мульти-карта│ • Объекты   │ • Персонажи  │ • Пространство│
│              │ • Порталы    │              │               │
├──────────────┴──────────────┴──────────────┴───────────────┤
│                      GameEngine                             │
│  Игрок • Камера • Бой • Частицы • UI • Сохранение/Загрузка │
├─────────────────────────────────────────────────────────────┤
│                    JSON / Ресурсы                           │
│  map.json  •  scenes/*.json  •  animations.json  •  ...    │
└─────────────────────────────────────────────────────────────┘
           ▲ авто-синхронизация при сохранении
┌─────────────────────────────────────────────────────────────┐
│              РЕДАКТОР МИРОВ (Python)                        │
│  Карта • Префабы • Квесты • Диалоги • Локации • HUD         │
└─────────────────────────────────────────────────────────────┘
🔨 Сборка
Windows
build.bat
Linux
chmod +x build.sh && ./build.sh
macOS
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(sysctl -n hw.ncpu)
🎮 Управление
Клавиша	Действие
W A S D	Движение
1 2 3 4	Скиллы
7	Зелье HP
8	Зелье MP
E	Взаимодействие
C	Характеристики
Tab	Инвентарь
M	Мини-карта
ЛКМ	Выбор цели
Колесо	Масштаб
🗺️ Редактор миров
# Требуется Python 3.10+
pip install pillow opencv-python  # опционально

# Запуск
python editor/aethoria_editor3.py
Вкладки редактора
Вкладка	Описание
🗺 Карта мира	Покраска тайлов, размещение сущностей, коллизии
🎬 Ресурсы	Видео → спрайт-листы (ffmpeg / OpenCV)
⚔ Квесты	Редактор цепочек квестов с наградами
💬 Диалоги	Диалоговые деревья NPC
⚙ Конфиг	Настройки игры, точки спавна, освещение
🔑 Экран входа	Текст и цвета UI входа
🧙 Персонажи	Классы и облики персонажей
🎮 HUD/UI	Настройка внутриигрового интерфейса
🌍 Локации	Менеджер сцен, метаданные
⭐ Префабы	60+ шаблонов сущностей
🧩 Системы
<details> <summary><b>⚔️ Система сцен</b></summary>
Загрузка, сохранение и переключение между именованными сценами.
Каждая сцена — JSON в assets/scenes/. Поддержка переходов с затуханием,
точек спавна, цвета окружения и списков сущностей.
Авто-синхронизация при каждом сохранении.

</details><details> <summary><b>🧙 Система сущностей</b></summary>
Универсальные типы: PLAYER · ENEMY · NPC · OBJECT · PORTAL · ITEM_DROP.
У каждой сущности уникальный uint32_t ID, позиция и свойства в JSON.

</details><details> <summary><b>🎬 Анимационная система</b></summary>
Загрузка спрайт-листов из animations.json. Поддержка клипов с настраиваемым FPS,
цикличностью и размерами кадров. Умное детектирование альфа-канала.

</details><details> <summary><b>⭐ Система префабов</b></summary>
60+ встроенных шаблонов в 5 категориях. Пользовательские префабы сохраняются в prefabs.json.
Однокликовая расстановка с наследованием свойств.

</details><details> <summary><b>🔊 Аудио-менеджер</b></summary>
Управление звуками и музыкой через SFML Audio.
Пространственное воспроизведение, вариации высоты тона, плавные переходы.

</details>
📋 План развития
Ядро движка (рендер, ввод, камера)

Система сущностей с JSON

Система сцен с переходами

Редактор миров (10 вкладок)

Система префабов (60+ шаблонов)

Конвейер анимаций

Авто-синхронизация редактора

Автосохранение с ротацией бэкапов

Горячая перезагрузка (обнаружение изменений)

База ресурсов с авто-обнаружением

Инструмент расстановки порталов

Система регионов / инстансов

Основа для многопользовательской игры

📜 Лицензия
© 2025 PapaZ — Все права защищены

Этот исходный код опубликован только для ознакомления и обучения.
Копирование, изменение, распространение или использование любой части кода
в собственных проектах строго запрещено без письменного разрешения.

См. LICENSE для полных условий.
