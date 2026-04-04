#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Papaz&KuponaLoa — Game Dev Studio v1.3 — Ultimate Game Dev Studio
══════════════════════════════════════════════════════
  ■ World Map Editor        — тайлы, враги, НПС, зоны
  ■ Asset Pipeline          — MP4/GIF/PNG → Sprite Sheet → JSON
  ■ Quest Editor            — цели, награды, квест-диалоги
  ■ Dialogue & NPC Editor   — реплики мобам/НПС, управление наградами

Требования: pip install Pillow opencv-python
ffmpeg должен быть установлен (brew/apt/choco install ffmpeg)
══════════════════════════════════════════════════════
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog, simpledialog
import json, os, sys, math, re, shutil, copy, random, subprocess, tempfile, threading
from pathlib import Path

try:
    from PIL import Image, ImageTk, ImageDraw, ImageFilter, ImageFont, ImageEnhance
    PILLOW = True
except ImportError:
    PILLOW = False

try:
    import cv2
    CV2 = True
except ImportError:
    CV2 = False

# ══════════════════════════════════════════════════════════════
# КОНСТАНТЫ
# ══════════════════════════════════════════════════════════════
TILE        = 32
MAP_W       = 120
MAP_H       = 120
CITY_CX     = 60
CITY_CY     = 60
CITY_RADIUS = 18
SAFE_RADIUS = 22

APP_TITLE   = "Papaz&KuponaLoa — Game Dev Studio v1.3"
VERSION     = "1.3.0"

# ── Палитра  стиль ──────────────────────────────
C = {
    "bg":       "#080714",
    "panel":    "#0e0c20",
    "panel2":   "#13112a",
    "panel3":   "#1a1740",
    "border":   "#2d2260",
    "gold":     "#f0c040",
    "gold2":    "#ffe880",
    "text":     "#e0d8ff",
    "muted":    "#6655aa",
    "accent":   "#8c3df5",
    "accent2":  "#b270ff",
    "green":    "#3dde7a",
    "red":      "#ff4455",
    "blue":     "#44aaff",
    "orange":   "#ff9933",
    "cyan":     "#33eeff",
    "pink":     "#ff55cc",
    "canvas":   "#05040f",
    "tab_bg":   "#100e22",
    "tab_sel":  "#1f1a45",
    "input_bg": "#12102a",
    "success":  "#22cc66",
    "warning":  "#ffaa22",
    "danger":   "#ff3344",
}

# ── Тайлы ────────────────────────────────────────────────────
TILES = {
    "GRASS":          {"name": "Трава",          "color": "#3c8c2a", "walkable": True,  "sfml": "55,140,45"},
    "DIRT":           {"name": "Грунт",           "color": "#7a5030", "walkable": True,  "sfml": "110,75,45"},
    "STONE_FLOOR":    {"name": "Камень",          "color": "#7a7265", "walkable": True,  "sfml": "130,122,110"},
    "ROAD":           {"name": "Дорога",          "color": "#9a8e78", "walkable": True,  "sfml": "160,150,135"},
    "WALL":           {"name": "Стена",           "color": "#5a4c3a", "walkable": False, "sfml": "120,105,90"},
    "WATER":          {"name": "Вода",            "color": "#2855cc", "walkable": False, "sfml": "50,110,190"},
    "TREE":           {"name": "Дерево",          "color": "#1a6018", "walkable": False, "sfml": "30,80,20"},
    "BUILDING_FLOOR": {"name": "Пол здания",      "color": "#5a4e3c", "walkable": False, "sfml": "100,90,78"},
    "FOUNTAIN":       {"name": "Фонтан",          "color": "#3a7acc", "walkable": True,  "sfml": "80,140,200"},
    "ROOF_RED":       {"name": "Крыша красная",   "color": "#aa2222", "walkable": False, "sfml": "160,60,50"},
    "ROOF_BLUE":      {"name": "Крыша синяя",     "color": "#2244cc", "walkable": False, "sfml": "50,80,180"},
    "ROOF_GREEN":     {"name": "Крыша зелёная",   "color": "#228844", "walkable": False, "sfml": "50,140,60"},
    "SAND":           {"name": "Песок",           "color": "#c8a858", "walkable": True,  "sfml": "190,165,90"},
    "SNOW":           {"name": "Снег",            "color": "#ddeeff", "walkable": True,  "sfml": "220,230,245"},
    "LAVA":           {"name": "Лава",            "color": "#cc3300", "walkable": False, "sfml": "200,60,10"},
    "BRIDGE":         {"name": "Мост",            "color": "#8a6a40", "walkable": True,  "sfml": "130,100,60"},
    "DUNGEON_FLOOR":  {"name": "Подземелье",      "color": "#3a3550", "walkable": True,  "sfml": "55,50,75"},
    "PORTAL":         {"name": "Портал",          "color": "#aa44ff", "walkable": True,  "sfml": "150,60,220"},
    "CHEST":          {"name": "Сундук",          "color": "#cc8822", "walkable": False, "sfml": "190,130,30"},
    "ALTAR":          {"name": "Алтарь",          "color": "#885533", "walkable": False, "sfml": "130,80,50"},
}

TILE_GROUPS = {
    "🌿 Природа":    ["GRASS","DIRT","TREE","WATER","SAND","SNOW","LAVA"],
    "🏙 Город":      ["STONE_FLOOR","ROAD","WALL","BUILDING_FLOOR","FOUNTAIN","ROOF_RED","ROOF_BLUE","ROOF_GREEN"],
    "⚔ Подземелье": ["DUNGEON_FLOOR","ALTAR","CHEST","PORTAL","BRIDGE"],
}

# ── Враги ─────────────────────────────────────────────────────
ENEMIES = {
    "GOBLIN":   {"name": "Гоблин",       "color": "#4a8a40", "emoji": "👺", "hp": 45,  "dmg": 7,  "speed": 75,  "gold": 35,
                 "anim": {"idle":"goblin_idle","walk":"goblin_walk","attack":"goblin_attack","death":"goblin_death"}},
    "TROLL":    {"name": "Тролль",       "color": "#7a6050", "emoji": "🧌", "hp": 90,  "dmg": 15, "speed": 60,  "gold": 50,
                 "anim": {"idle":"troll_idle","walk":"troll_walk","attack":"troll_attack","death":"troll_death"}},
    "BANDIT":   {"name": "Бандит",       "color": "#aa7030", "emoji": "🗡",  "hp": 55,  "dmg": 10, "speed": 80,  "gold": 40,
                 "anim": {"idle":"bandit_idle","walk":"bandit_walk","attack":"bandit_attack","death":"bandit_death"}},
    "WOLF":     {"name": "Тёмный волк",  "color": "#505068", "emoji": "🐺", "hp": 40,  "dmg": 9,  "speed": 95,  "gold": 30,
                 "anim": {"idle":"wolf_idle","walk":"wolf_walk","attack":"wolf_attack","death":"wolf_death"}},
    "SKELETON": {"name": "Скелет",       "color": "#ccccaa", "emoji": "💀", "hp": 50,  "dmg": 8,  "speed": 70,  "gold": 38,
                 "anim": {"idle":"skeleton_idle","walk":"skeleton_walk","attack":"skeleton_attack","death":"skeleton_death"}},
    "ORC":      {"name": "Орк",          "color": "#558844", "emoji": "👹", "hp": 120, "dmg": 20, "speed": 65,  "gold": 65,
                 "anim": {"idle":"orc_idle","walk":"orc_walk","attack":"orc_attack","death":"orc_death"}},
    "DRAGON":   {"name": "Дракон",       "color": "#cc3322", "emoji": "🐉", "hp": 500, "dmg": 50, "speed": 90,  "gold": 300,
                 "anim": {"idle":"dragon_idle","walk":"dragon_walk","attack":"dragon_attack","death":"dragon_death"}},
    "SPIDER":   {"name": "Паук",         "color": "#442244", "emoji": "🕷",  "hp": 30,  "dmg": 6,  "speed": 100, "gold": 25,
                 "anim": {"idle":"spider_idle","walk":"spider_walk","attack":"spider_attack","death":"spider_death"}},
    "VAMPIRE":  {"name": "Вампир",       "color": "#660033", "emoji": "🧛", "hp": 150, "dmg": 25, "speed": 85,  "gold": 100,
                 "anim": {"idle":"vampire_idle","walk":"vampire_walk","attack":"vampire_attack","death":"vampire_death"}},
    "GOLEM":    {"name": "Голем",        "color": "#888888", "emoji": "🗿", "hp": 300, "dmg": 35, "speed": 45,  "gold": 150,
                 "anim": {"idle":"golem_idle","walk":"golem_walk","attack":"golem_attack","death":"golem_death"}},
}

# ── НПС ───────────────────────────────────────────────────────
NPCS = {
    "VENDOR":       {"name": "Торговец",       "color": "#cc9933", "emoji": "🛒",
                     "anim": {"idle":"vendor_idle","talk":"vendor_talk"}},
    "QUEST":        {"name": "Квестодатель",   "color": "#dd3333", "emoji": "❗",
                     "anim": {"idle":"quest_idle","talk":"quest_talk"}},
    "GUARD":        {"name": "Стражник",       "color": "#3355aa", "emoji": "💂",
                     "anim": {"idle":"guard_idle","attack":"guard_attack"}},
    "HEALER":       {"name": "Целитель",       "color": "#33aa55", "emoji": "⚕",
                     "anim": {"idle":"healer_idle","talk":"healer_talk"}},
    "INNKEEPER":    {"name": "Трактирщик",     "color": "#aa5522", "emoji": "🏨",
                     "anim": {"idle":"innkeeper_idle","talk":"innkeeper_talk"}},
    "BLACKSMITH":   {"name": "Кузнец",         "color": "#556677", "emoji": "⚒",
                     "anim": {"idle":"blacksmith_idle","work":"blacksmith_work"}},
    "MAGE_TRAINER": {"name": "Маг-наставник",  "color": "#8844cc", "emoji": "🧙",
                     "anim": {"idle":"mage_idle","cast":"mage_cast"}},
    "BANKER":       {"name": "Банкир",         "color": "#aaaa22", "emoji": "💰",
                     "anim": {"idle":"banker_idle","talk":"banker_talk"}},
    "PORTAL_KEEPER":{"name": "Хранитель порт.", "color":"#aa44ff",  "emoji": "🌀",
                     "anim": {"idle":"portal_idle"}},
    "FISHERMAN":    {"name": "Рыбак",          "color": "#4488aa", "emoji": "🎣",
                     "anim": {"idle":"fisherman_idle"}},
}

ZONES = {
    "SPAWN":      {"name": "Точка спауна",    "color": "#ff44aa", "alpha": 80},
    "SAFE":       {"name": "Безопасная зона", "color": "#44bb66", "alpha": 70},
    "PVP":        {"name": "PvP зона",        "color": "#ee4444", "alpha": 75},
    "BOSS":       {"name": "Зона босса",      "color": "#cc44ff", "alpha": 85},
    "EVENT":      {"name": "Ивент",           "color": "#44ccff", "alpha": 70},
    "DUNGEON":    {"name": "Вход в данж",     "color": "#885500", "alpha": 80},
    "TOWN":       {"name": "Город",           "color": "#4488ff", "alpha": 60},
    "WILDERNESS": {"name": "Дикая природа",   "color": "#228844", "alpha": 60},
}

OBJECTS = {
    "CHEST":            {"name": "Сундук",           "color": "#cc8822", "emoji": "📦"},
    "ALTAR":            {"name": "Алтарь",           "color": "#885533", "emoji": "🗽"},
    "PORTAL":           {"name": "Портал",           "color": "#aa44ff", "emoji": "🌀"},
    "TORCH":            {"name": "Факел",            "color": "#ff8822", "emoji": "🔥"},
    "SIGN":             {"name": "Знак",             "color": "#886644", "emoji": "🪧"},
    "TREE_GIANT":       {"name": "Гигантское дерево","color": "#114411", "emoji": "🌳"},
    "BOSS_SPAWN":       {"name": "Точка босса",      "color": "#cc0000", "emoji": "💀"},
    "DUNGEON_ENTRANCE": {"name": "Вход данж",        "color": "#442200", "emoji": "🚪"},
    "WAYPOINT":         {"name": "Путевая точка",    "color": "#4488ff", "emoji": "📍"},
    "RESOURCE":         {"name": "Ресурс",           "color": "#44aa44", "emoji": "💎"},
}

# ── Шаблоны диалогов по умолчанию ─────────────────────────────
DEFAULT_DIALOGUES = {
    "GOBLIN": {
        "aggro":   ["Гы-гы! Я убью тебя!", "Смерть, смерть, смерть!", "Хозяин будет доволен!"],
        "combat":  ["Умри! Умри!", "Я тебя поймал!", "Никуда не денешься!"],
        "death":   ["Нет... невозможно...", "Уй... за мной придут другие...", "*хрипит*"],
        "idle":    ["*бормочет что-то*", "*шатается*", "*чешет голову*"],
    },
    "TROLL": {
        "aggro":   ["ТРОЛЛЬ СОКРУШИТЬ!", "Маленький человек вкусный!", "РА-А-А!"],
        "combat":  ["Тролль БИТЬ!", "Никуда не уйдёшь!", "РАЗОРВУ!"],
        "death":   ["Тролль... устал...", "*грохот*"],
        "idle":    ["*громкое сопение*", "*скребёт землю*"],
    },
    "WOLF": {
        "aggro":   ["*злобный рык*", "*оскаливает зубы*", "*рычит*"],
        "combat":  ["*кусает*", "*воет*"],
        "death":   ["*жалобный вой*", "*скулит*"],
        "idle":    ["*нюхает воздух*", "*рычит тихо*"],
    },
    "DRAGON": {
        "aggro":   ["Смертный! Ты осмелился нарушить мой покой?", "Твои кости пополнят мою коллекцию!", "ОГОНЬ И ПЕПЕЛ!"],
        "combat":  ["Чувствуешь жар?", "Я УНИЧТОЖУ ТЕБЯ!", "Беги! Или оставайся и умри!"],
        "death":   ["Невозможно... тысячи лет... и вот так...", "Мой огонь... гаснет...", "Я... вернусь..."],
        "idle":    ["*глубокий рык*", "*пускает кольца дыма*"],
    },
    "VENDOR": {
        "greeting": ["Добро пожаловать! Лучшие товары в Аэтории!", "Чем могу помочь, путник?", "Есть что-нибудь интересное для тебя!"],
        "trade":    ["Что хочешь купить?", "Смотри, не пожалеешь!", "Цены честные, товар качественный!"],
        "farewell": ["Возвращайся!", "Удачи, путник!", "Буду рад видеть снова!"],
        "idle":     ["*раскладывает товары*", "*считает монеты*"],
    },
    "QUEST": {
        "greeting": ["О, путник! Как раз тебя искал!", "У меня есть задание для храброго авантюриста.", "Наконец-то ты пришёл!"],
        "offer":    ["Мне нужна твоя помощь...", "Это опасное задание, но ты справишься.", "Времени мало!"],
        "progress": ["Ты ещё не выполнил задание...", "Продолжай, у тебя всё получится!", "Я верю в тебя."],
        "complete": ["Невероятно! Ты справился!", "Аэтория в долгу перед тобой!", "Держи свою заслуженную награду!"],
        "idle":     ["*нервно оглядывается*", "*перебирает бумаги*"],
    },
    "HEALER": {
        "greeting": ["Свет Аэтории да пребудет с тобой!", "Нуждаешься в исцелении?", "Добро пожаловать в храм."],
        "trade":    ["Могу вылечить твои раны.", "Зелья тут лучшие в округе.", "Доверься свету!"],
        "farewell": ["Иди с миром.", "Свет да хранит тебя!", "Возвращайся если понадоблюсь."],
        "idle":     ["*читает молитву*", "*смешивает зелья*"],
    },
    "GUARD": {
        "greeting": ["Стой! Кто идёт?", "Проходи, нарушений не допускай.", "Добро пожаловать в город."],
        "warning":  ["Нарушители законов будут наказаны!", "В городе порядок!", "Следи за собой, путник."],
        "farewell": ["Проходи.", "Следующий!", "Не задерживайся."],
        "idle":     ["*патрулирует*", "*звенит доспехами*", "*зевает*"],
    },
    "BLACKSMITH": {
        "greeting": ["Нужно оружие? Ты пришёл куда надо!", "Сталь Борга — лучшая в мире!", "Что изготовить?"],
        "trade":    ["Вот моё лучшее оружие.", "Могу улучшить твоё снаряжение.", "Качество гарантирую!"],
        "farewell": ["Удачи в бою!", "Возвращайся с трофеями!", "Это оружие тебя не подведёт!"],
        "idle":     ["*стучит молотом*", "*раздувает меха*", "*точит клинок*"],
    },
    "INNKEEPER": {
        "greeting": ["Добро пожаловать в «Золотую чарку»!", "Усаживайся, путник!", "Что будешь — пиво или эль?"],
        "trade":    ["Вот наше меню.", "Комната на ночь — 10 монет.", "Кормлю до отвала!"],
        "farewell": ["Возвращайся!", "Двери всегда открыты!", "Удачи в пути!"],
        "idle":     ["*протирает кружки*", "*напевает*", "*считает монеты*"],
    },
}

# ── Шаблоны квестов по умолчанию ─────────────────────────────
QUEST_TYPES = {
    "kill":    {"name": "Убийство",     "icon": "⚔", "color": "#ff4455"},
    "collect": {"name": "Сбор",         "icon": "📦", "color": "#ffaa22"},
    "escort":  {"name": "Сопровождение","icon": "🛡", "color": "#44aaff"},
    "explore": {"name": "Исследование", "icon": "🗺", "color": "#44dd88"},
    "deliver": {"name": "Доставка",     "icon": "📜", "color": "#cc88ff"},
    "talk":    {"name": "Разговор",     "icon": "💬", "color": "#ffff44"},
}

ITEM_TYPES = {
    "weapon":  ["Деревянный меч","Железный меч","Стальной меч","Зачарованный клинок","Легендарная сабля"],
    "armor":   ["Кожаный нагрудник","Кольчуга","Рыцарские доспехи","Мифриловая броня"],
    "potion":  ["Зелье здоровья (малое)","Зелье здоровья (среднее)","Зелье маны","Зелье скорости"],
    "material":["Гоблинское ухо","Волчий клык","Кость скелета","Драконья чешуя","Кристалл маны"],
    "key":     ["Ключ от данжа","Ключ от сокровищницы","Ключ от руин"],
}

# ── Анимационные пресеты ───────────────────────────────────────
ANIM_ACTIONS = ["idle","walk","run","attack","attack2","cast","hurt","death","interact","emote"]
ANIM_FRAME_SIZES = [32, 48, 64, 96, 128, 192, 256]
ANIM_FPS_PRESETS = [6, 8, 10, 12, 15, 24, 30]

ENTITY_KEYWORDS = {
    "player": ["player","hero","protagonist","char","character"],
    "goblin": ["goblin","gob"],
    "troll": ["troll"],
    "bandit": ["bandit","rogue","thief"],
    "wolf": ["wolf","hound","dog"],
    "skeleton": ["skeleton","bones","undead"],
    "orc": ["orc","ork"],
    "dragon": ["dragon","wyvern","drake"],
    "spider": ["spider","arachnid"],
    "vampire": ["vampire","vamp","dracula"],
    "golem": ["golem","construct"],
    "vendor": ["vendor","merchant","shop","trader"],
    "guard": ["guard","soldier","warrior"],
    "healer": ["healer","priest","cleric"],
    "npc": ["npc","townfolk","villager","civilian"],
}

ACTION_KEYWORDS = {
    "idle":    ["idle","stand","wait","rest"],
    "walk":    ["walk","move","run","jog"],
    "attack":  ["attack","hit","slash","strike","swing","fight"],
    "attack2": ["attack2","cast","skill","spell","magic","special"],
    "hurt":    ["hurt","damage","pain","hit","wound"],
    "death":   ["death","die","dead","defeat","kill"],
    "interact":["interact","talk","use","open"],
}

try:
    from aethoria_mmo_tabs import ClassesTab, ItemsTab, ServerTab
    MMO_TABS = True
except ImportError:
    MMO_TABS = False


# ══════════════════════════════════════════════════════════════
# ГЕНЕРАЦИЯ КАРТЫ
# ══════════════════════════════════════════════════════════════
def generate_default_map():
    rng = random.Random(42)
    tiles = [["GRASS"] * MAP_W for _ in range(MAP_H)]

    for y in range(MAP_H):
        for x in range(MAP_W):
            dx = x - CITY_CX
            dy = y - CITY_CY
            d = math.sqrt(dx*dx + dy*dy)
            if d > CITY_RADIUS + 4 and rng.randint(0,10) == 0:
                tiles[y][x] = "TREE"

    for dy in range(-CITY_RADIUS, CITY_RADIUS+1):
        for dx in range(-CITY_RADIUS, CITY_RADIUS+1):
            tx = CITY_CX + dx
            ty = CITY_CY + dy
            if tx < 0 or ty < 0 or tx >= MAP_W or ty >= MAP_H:
                continue
            d = math.sqrt(dx*dx + dy*dy)
            on_border = (int(d + 0.5) == CITY_RADIUS)
            gate = ((dy == CITY_RADIUS or dy == -CITY_RADIUS) and abs(dx) <= 2) or \
                   ((dx == CITY_RADIUS or dx == -CITY_RADIUS) and abs(dy) <= 2)
            if on_border and not gate:
                tiles[ty][tx] = "WALL"
            elif not on_border or gate:
                tiles[ty][tx] = "STONE_FLOOR"

    for dx in range(-CITY_RADIUS, CITY_RADIUS+1):
        tx = CITY_CX + dx
        if 0 <= tx < MAP_W: tiles[CITY_CY][tx] = "ROAD"
    for dy in range(-CITY_RADIUS, CITY_RADIUS+1):
        ty = CITY_CY + dy
        if 0 <= ty < MAP_H: tiles[ty][CITY_CX] = "ROAD"

    for i in range(CITY_RADIUS, MAP_H//2):
        for tx, ty in [(CITY_CX, CITY_CY-i),(CITY_CX, CITY_CY+i),(CITY_CX-i, CITY_CY),(CITY_CX+i, CITY_CY)]:
            if 0 <= tx < MAP_W and 0 <= ty < MAP_H: tiles[ty][tx] = "ROAD"

    def place_tower(cx, cy):
        for dy2 in range(-2,3):
            for dx2 in range(-2,3):
                ttx = cx+dx2; tty = cy+dy2
                if 0 <= ttx < MAP_W and 0 <= tty < MAP_H: tiles[tty][ttx] = "WALL"

    place_tower(CITY_CX-CITY_RADIUS+1, CITY_CY-CITY_RADIUS+1)
    place_tower(CITY_CX+CITY_RADIUS-1, CITY_CY-CITY_RADIUS+1)
    place_tower(CITY_CX-CITY_RADIUS+1, CITY_CY+CITY_RADIUS-1)
    place_tower(CITY_CX+CITY_RADIUS-1, CITY_CY+CITY_RADIUS-1)

    buildings = [
        (CITY_CX-14,CITY_CY-14,4,3),(CITY_CX-14,CITY_CY-10,3,3),
        (CITY_CX+10,CITY_CY-14,4,3),(CITY_CX+10,CITY_CY-10,3,3),
        (CITY_CX-14,CITY_CY+8, 4,3),(CITY_CX-14,CITY_CY+12,3,3),
        (CITY_CX+10,CITY_CY+8, 4,3),(CITY_CX+10,CITY_CY+12,3,3),
        (CITY_CX-3, CITY_CY-7, 6,5),
    ]
    for (btx,bty,bw,bh) in buildings:
        for dy2 in range(bh):
            for dx2 in range(bw):
                ttx=btx+dx2; tty=bty+dy2
                if 0<=ttx<MAP_W and 0<=tty<MAP_H: tiles[tty][ttx]="BUILDING_FLOOR"

    for dy2 in range(-1,2):
        for dx2 in range(-1,2):
            tiles[CITY_CY+dy2][CITY_CX+dx2]="FOUNTAIN"

    for dy in range(5):
        for dx in range(7):
            tx=CITY_CX+28+dx; ty=CITY_CY+22+dy
            if tx<MAP_W and ty<MAP_H: tiles[ty][tx]="WATER"

    return tiles

# ══════════════════════════════════════════════════════════════
# ENTITY REGISTRY — единый реестр сущностей с уникальными ID
# ══════════════════════════════════════════════════════════════
class EntityRegistry:
    """
    kind: enemy | npc | object | portal | chest | trigger | zone
    Каждая сущность: {id, kind, type, x, y, props}
    """
    KIND_PREFIX = {
        "enemy":"e","npc":"n","object":"o",
        "portal":"p","chest":"c","trigger":"t","zone":"z",
    }
    def __init__(self):
        self._entities = []
        self._counters = {}

    def _gen_id(self, kind):
        prefix = self.KIND_PREFIX.get(kind, "x")
        n = self._counters.get(prefix, 0) + 1
        self._counters[prefix] = n
        return f"{prefix}{n:04d}"

    def _sync_counter(self, eid):
        if not eid or len(eid) < 2: return
        try: self._counters[eid[0]] = max(self._counters.get(eid[0],0), int(eid[1:]))
        except ValueError: pass

    def add(self, kind, etype, x, y, props=None, eid=None):
        if eid is None: eid = self._gen_id(kind)
        else: self._sync_counter(eid)
        e = {"id":eid,"kind":kind,"type":etype,"x":x,"y":y,"props":dict(props or {})}
        self._entities.append(e)
        return e

    def remove(self, eid):
        before = len(self._entities)
        self._entities = [e for e in self._entities if e.get("id") != eid]
        return len(self._entities) < before

    def remove_at(self, x, y):
        before = len(self._entities)
        self._entities = [e for e in self._entities if not(e["x"]==x and e["y"]==y)]
        return before - len(self._entities)

    def find_by_id(self, eid):
        return next((e for e in self._entities if e.get("id")==eid), None)

    def find_at(self, x, y):
        return [e for e in self._entities if e["x"]==x and e["y"]==y]

    def find_by_kind(self, kind):
        return [e for e in self._entities if e.get("kind")==kind]

    def find_by_type(self, etype):
        return [e for e in self._entities if e.get("type")==etype]

    def update_props(self, eid, props):
        e = self.find_by_id(eid)
        if e: e["props"].update(props)

    def move(self, eid, x, y):
        e = self.find_by_id(eid)
        if e: e["x"]=x; e["y"]=y

    def all(self): return self._entities

    def count(self, kind=None):
        if kind: return sum(1 for e in self._entities if e.get("kind")==kind)
        return len(self._entities)

    def clear(self):
        self._entities.clear(); self._counters.clear()

    def to_list(self):
        return [dict(e) for e in self._entities]

    def from_list(self, data):
        self._entities.clear(); self._counters.clear()
        for item in data:
            if not isinstance(item, dict): continue
            self.add(item.get("kind","object"), item.get("type",""),
                     item.get("x",0), item.get("y",0),
                     item.get("props",{}), item.get("id",None))


def _default_object_props(otype):
    if otype=="CHEST":    return {"loot_table":"common","gold_min":10,"gold_max":50,"opened":False}
    if otype=="PORTAL":   return {"target_zone":"","target_x":0,"target_y":0,"color":"#aa44ff"}
    if otype=="SIGN":     return {"text":"Надпись на знаке"}
    if otype=="TORCH":    return {"light_radius":3,"color":"#ff8822"}
    if otype=="BOSS_SPAWN":       return {"boss_type":"DRAGON","respawn_time":300}
    if otype=="DUNGEON_ENTRANCE": return {"target_zone":"Goblin Caves","level_req":1}
    if otype=="WAYPOINT": return {"name":"Путевая точка","cost":0}
    if otype=="RESOURCE": return {"resource_type":"ore","respawn_time":60}
    return {}


def generate_default_entities():
    entities = []
    npcs_data = [
        {"type":"VENDOR",      "x":CITY_CX-8,"y":CITY_CY-4,"kind":"npc","props":{"name":"Торговец Алар","quest_id":None}},
        {"type":"QUEST",       "x":CITY_CX+3,"y":CITY_CY-8,"kind":"npc","props":{"name":"Капитан Грэй","quest_id":"q001"}},
        {"type":"HEALER",      "x":CITY_CX-5,"y":CITY_CY+4,"kind":"npc","props":{"name":"Жрица Элара","quest_id":None}},
        {"type":"BLACKSMITH",  "x":CITY_CX+6,"y":CITY_CY+3,"kind":"npc","props":{"name":"Кузнец Борг","quest_id":None}},
        {"type":"INNKEEPER",   "x":CITY_CX-2,"y":CITY_CY+6,"kind":"npc","props":{"name":"Трактирщик Дун","quest_id":None}},
        {"type":"GUARD",       "x":CITY_CX-CITY_RADIUS+3,"y":CITY_CY,"kind":"npc","props":{"name":"Стражник","quest_id":None}},
        {"type":"GUARD",       "x":CITY_CX+CITY_RADIUS-3,"y":CITY_CY,"kind":"npc","props":{"name":"Стражник","quest_id":None}},
    ]
    entities.extend(npcs_data)
    entities.append({"type":"SPAWN","x":CITY_CX,"y":CITY_CY-4,"kind":"zone","props":{"radius":3}})
    entities.append({"type":"SAFE", "x":CITY_CX,"y":CITY_CY,  "kind":"zone","props":{"radius":CITY_RADIUS}})

    rng = random.Random(99)
    types = ["GOBLIN","TROLL","BANDIT","WOLF","SKELETON"]
    spawned = 0
    while spawned < 20:
        tx = rng.randint(2, MAP_W-3)
        ty = rng.randint(2, MAP_H-3)
        px = tx*TILE+TILE//2; py = ty*TILE+TILE//2
        d = math.sqrt((px-CITY_CX*TILE)**2+(py-CITY_CY*TILE)**2)
        if d < SAFE_RADIUS*TILE: continue
        et = types[spawned%5]
        lvl = 1+spawned//4
        boss = (spawned==9 or spawned==19)
        entities.append({
            "type":et,"x":tx,"y":ty,"kind":"enemy",
            "props":{"level":lvl,"boss":boss,"hp":(30+lvl*15)*(4 if boss else 1),"dmg":(5+lvl*2)*(2 if boss else 1)}
        })
        spawned += 1
    return entities

def make_default_quests():
    return [
        {
            "id": "q001", "name": "Первые шаги", "level_req": 1,
            "type": "kill", "giver_npc": "Капитан Грэй",
            "description": "Гоблины атакуют деревни вокруг Аэториума. Уничтожь их!",
            "objectives": [{"type":"kill","target":"GOBLIN","count":5,"current":0}],
            "rewards": {"gold_min":50,"gold_max":100,"xp":200,"items":[]},
            "dialogue": {
                "offer":    ["О, путник! Как раз тебя искал!","Гоблины терроризируют наши форпосты.","Убей 5 гоблинов — и ты получишь награду!"],
                "progress": ["Ты ещё не добил их всех...","Продолжай, у тебя всё получится!"],
                "complete": ["Превосходно! Ты справился!","Аэтория благодарит тебя!","Держи заслуженную награду."]
            },
            "active": True
        },
        {
            "id": "q002", "name": "Зелья целителя", "level_req": 2,
            "type": "collect", "giver_npc": "Жрица Элара",
            "description": "Мне нужны волчьи клыки для приготовления зелий. Найди их.",
            "objectives": [{"type":"collect","target":"Волчий клык","count":3,"current":0}],
            "rewards": {"gold_min":30,"gold_max":60,"xp":150,"items":["Зелье здоровья (среднее)"]},
            "dialogue": {
                "offer":    ["Путник, мне нужна твоя помощь.","Запасы зелий заканчиваются.","Принеси мне 3 волчьих клыка."],
                "progress": ["Клыков пока недостаточно...","Волки обитают в лесу к северу от города."],
                "complete": ["Благодарю тебя!","Аэтория нуждается в таких людях как ты.","Вот твоя награда."]
            },
            "active": True
        }
    ]

def make_default_dialogues():
    return copy.deepcopy(DEFAULT_DIALOGUES)


# ══════════════════════════════════════════════════════════════
# ПОИСК ПРОЕКТА
# ══════════════════════════════════════════════════════════════
def find_project_root():
    start = Path(sys.argv[0]).resolve().parent
    for path in [start, *start.parents]:
        hits = sum(1 for m in ["CMakeLists.txt","src","assets","build.bat","main.cpp"] if (path/m).exists())
        if hits >= 2: return path
        if (path/"src"/"main.cpp").exists(): return path
    return start


# ══════════════════════════════════════════════════════════════
# ДАННЫЕ КАРТЫ
# ══════════════════════════════════════════════════════════════
class MapData:
    def __init__(self):
        self.width    = MAP_W; self.height = MAP_H
        self.name     = "Aethoria World"
        self.tiles    = generate_default_map()
        self.filepath = None
        self.metadata = {
            "city_cx":CITY_CX,"city_cy":CITY_CY,"city_radius":CITY_RADIUS,
            "player_spawn_x":CITY_CX,"player_spawn_y":CITY_CY-5,
        }
        self.registry = EntityRegistry()
        self.registry.from_list(generate_default_entities())

    @property
    def entities(self):    return self.registry._entities
    @entities.setter
    def entities(self, v): self.registry.from_list(v)

    def get(self, x, y):
        if 0<=x<self.width and 0<=y<self.height: return self.tiles[y][x]
        return None

    def set(self, x, y, tile):
        if 0<=x<self.width and 0<=y<self.height: self.tiles[y][x]=tile

    def flood_fill(self, x, y, tile):
        target = self.get(x,y)
        if target == tile or target is None: return
        stack = [(x,y)]; visited = set()
        while stack:
            cx,cy = stack.pop()
            if (cx,cy) in visited: continue
            if not (0<=cx<self.width and 0<=cy<self.height): continue
            if self.tiles[cy][cx] != target: continue
            visited.add((cx,cy)); self.tiles[cy][cx] = tile
            stack+=[(cx+1,cy),(cx-1,cy),(cx,cy+1),(cx,cy-1)]

    def entities_at(self, x, y):
        return self.registry.find_at(x, y)

    def to_dict(self):
        collision = [
            [0 if TILES.get(self.tiles[y][x],{}).get("walkable",True) else 1
             for x in range(self.width)]
            for y in range(self.height)
        ]
        return {"version":"4.0","name":self.name,"width":self.width,"height":self.height,
                "metadata":self.metadata,"tiles":self.tiles,
                "collision":collision,"entities":self.registry.to_list()}

    def from_dict(self, d):
        self.width=d.get("width",MAP_W); self.height=d.get("height",MAP_H)
        self.name=d.get("name","World"); self.tiles=d.get("tiles",generate_default_map())
        self.metadata=d.get("metadata",{})
        self.registry.from_list(d.get("entities",[]))

# ══════════════════════════════════════════════════════════════
class AnimationPipeline:
    """Конвертирует MP4/GIF/PNG → Sprite Sheet + JSON конфиг для движка"""

    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.output_dir = project_root / "assets" / "textures" / "sprites"
        self.config_path = project_root / "assets" / "animations.json"
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def detect_entity_action(self, filename: str):
        """Определяет сущность и действие из имени файла"""
        stem = Path(filename).stem.lower()
        stem_clean = re.sub(r'[^a-z0-9_]', '_', stem)
        parts = stem_clean.split('_')

        entity = "unknown"
        action = "idle"

        # Определяем сущность
        for ent_key, keywords in ENTITY_KEYWORDS.items():
            for kw in keywords:
                if kw in stem_clean:
                    entity = ent_key
                    break

        # Определяем действие
        for act_key, keywords in ACTION_KEYWORDS.items():
            for kw in keywords:
                if kw in stem_clean:
                    action = act_key
                    break

        return entity, action

    def extract_frames_ffmpeg(self, video_path: str, out_dir: Path,
                               target_w: int = 64, target_h: int = 64,
                               fps: int = 12) -> list:
        """Извлекает кадры из MP4 через ffmpeg"""
        out_dir.mkdir(parents=True, exist_ok=True)
        out_pattern = str(out_dir / "frame_%04d.png")
        scale_filter = f"scale={target_w}:{target_h}:force_original_aspect_ratio=decrease,pad={target_w}:{target_h}:(ow-iw)/2:(oh-ih)/2:color=black@0"
        cmd = ["ffmpeg", "-y", "-i", video_path, "-vf", f"fps={fps},{scale_filter}", out_pattern]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"ffmpeg error: {result.stderr[-500:]}")
        frames = sorted(out_dir.glob("frame_*.png"))
        return frames

    def extract_frames_cv2(self, video_path: str, out_dir: Path,
                            target_w: int = 64, target_h: int = 64,
                            fps: int = 12) -> list:
        """Извлекает кадры через OpenCV"""
        cap = cv2.VideoCapture(video_path)
        if not cap.isOpened():
            raise RuntimeError(f"Не удалось открыть видео: {video_path}")

        src_fps = cap.get(cv2.CAP_PROP_FPS) or 24
        frame_interval = max(1, int(src_fps / fps))
        out_dir.mkdir(parents=True, exist_ok=True)
        frames = []; idx = 0; saved = 0

        while True:
            ret, frame = cap.read()
            if not ret: break
            if idx % frame_interval == 0:
                # Конвертим в RGBA
                rgba = cv2.cvtColor(frame, cv2.COLOR_BGR2BGRA)
                # Убираем чёрный фон (auto-alpha)
                black_mask = (frame[:,:,0]<15) & (frame[:,:,1]<15) & (frame[:,:,2]<15)
                rgba[black_mask, 3] = 0
                # Ресайз
                resized = cv2.resize(rgba, (target_w, target_h), interpolation=cv2.INTER_AREA)
                fname = out_dir / f"frame_{saved:04d}.png"
                cv2.imwrite(str(fname), resized)
                frames.append(fname); saved += 1
            idx += 1
        cap.release()
        return frames

    def extract_frames_gif(self, gif_path: str, out_dir: Path,
                            target_w: int = 64, target_h: int = 64) -> list:
        """Извлекает кадры из GIF через Pillow"""
        if not PILLOW:
            raise RuntimeError("Pillow не установлен")
        out_dir.mkdir(parents=True, exist_ok=True)
        img = Image.open(gif_path)
        frames = []
        try:
            while True:
                frame = img.convert("RGBA").resize((target_w, target_h), Image.LANCZOS)
                fname = out_dir / f"frame_{len(frames):04d}.png"
                frame.save(str(fname))
                frames.append(fname)
                img.seek(img.tell() + 1)
        except EOFError:
            pass
        return frames

    def build_spritesheet(self, frames: list, target_w: int, target_h: int,
                           max_cols: int = 8) -> Image.Image:
        """Создаёт спрайт-шит из списка кадров"""
        n = len(frames)
        if n == 0:
            raise ValueError("Нет кадров для спрайт-шита")
        cols = min(n, max_cols)
        rows = math.ceil(n / cols)
        sheet = Image.new("RGBA", (cols * target_w, rows * target_h), (0,0,0,0))
        for i, fp in enumerate(frames):
            frame_img = Image.open(str(fp)).convert("RGBA")
            frame_img = frame_img.resize((target_w, target_h), Image.LANCZOS)
            col = i % cols; row = i // cols
            sheet.paste(frame_img, (col*target_w, row*target_h), frame_img)
        return sheet

    def load_animations_config(self) -> dict:
        if self.config_path.exists():
            try:
                return json.loads(self.config_path.read_text(encoding="utf-8"))
            except: pass
        return {}

    def save_animations_config(self, config: dict):
        self.config_path.write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding="utf-8")

    def import_file(self, filepath: str, entity: str = None, action: str = None,
                    target_w: int = 64, target_h: int = 64, fps: int = 12,
                    progress_cb=None) -> dict:
        """Главная функция импорта. Возвращает конфиг анимации."""
        path = Path(filepath)
        if not path.exists():
            raise FileNotFoundError(f"Файл не найден: {filepath}")

        # Авто-определение если не задано
        if not entity or not action:
            auto_entity, auto_action = self.detect_entity_action(path.name)
            entity = entity or auto_entity
            action = action or auto_action

        if progress_cb: progress_cb(10, "Извлекаем кадры...")

        # Временная папка для кадров
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            frames_dir = tmp_path / "frames"
            ext = path.suffix.lower()

            try:
                if ext in (".mp4", ".avi", ".mov", ".mkv", ".webm"):
                    if CV2:
                        frames = self.extract_frames_cv2(str(path), frames_dir, target_w, target_h, fps)
                    else:
                        frames = self.extract_frames_ffmpeg(str(path), frames_dir, target_w, target_h, fps)
                elif ext == ".gif":
                    frames = self.extract_frames_gif(str(path), frames_dir, target_w, target_h)
                elif ext in (".png", ".jpg", ".jpeg", ".webp"):
                    # Одиночный кадр или уже спрайт-шит
                    if PILLOW:
                        img = Image.open(str(path)).convert("RGBA")
                        frames = [path]  # используем как есть
                    else:
                        raise RuntimeError("Pillow не установлен")
                else:
                    raise ValueError(f"Неподдерживаемый формат: {ext}")

                if not frames:
                    raise RuntimeError("Нет кадров после извлечения")

                if progress_cb: progress_cb(50, f"Создаём спрайт-шит ({len(frames)} кадров)...")

                if ext in (".png",".jpg",".jpeg",".webp") and len(frames)==1:
                    # Используем изображение напрямую
                    sheet_img = Image.open(str(frames[0])).convert("RGBA")
                    n_frames = 1
                else:
                    sheet_img = self.build_spritesheet(frames, target_w, target_h)
                    n_frames = len(frames)

                if progress_cb: progress_cb(75, "Сохраняем спрайт-шит...")

                # Сохраняем
                sheet_name = f"{entity}_{action}.png"
                sheet_path = self.output_dir / sheet_name
                sheet_img.save(str(sheet_path), "PNG")

                if progress_cb: progress_cb(90, "Обновляем конфиг анимаций...")

                # Обновляем animations.json
                config = self.load_animations_config()
                if entity not in config:
                    config[entity] = {}
                config[entity][action] = {
                    "sheet": sheet_name,
                    "frames": n_frames,
                    "cols": min(n_frames, 8),
                    "rows": math.ceil(n_frames / min(n_frames, 8)),
                    "width": target_w,
                    "height": target_h,
                    "fps": fps,
                    "loop": action not in ("death",),
                    "source": path.name,
                }
                self.save_animations_config(config)

                if progress_cb: progress_cb(100, "Готово!")
                return config[entity][action]

            except Exception as e:
                raise RuntimeError(f"Ошибка импорта: {e}") from e

    def get_preview_image(self, entity: str, action: str, max_size: int = 256) -> "ImageTk.PhotoImage | None":
        """Возвращает превью спрайт-шита для отображения в UI"""
        if not PILLOW: return None
        sheet_path = self.output_dir / f"{entity}_{action}.png"
        if not sheet_path.exists(): return None
        img = Image.open(str(sheet_path)).convert("RGBA")
        img.thumbnail((max_size, max_size), Image.LANCZOS)
        # Шахматный фон
        bg = Image.new("RGBA", img.size)
        draw = ImageDraw.Draw(bg)
        sq = 8
        for yi in range(0, img.height, sq):
            for xi in range(0, img.width, sq):
                color = (40,40,60) if (xi//sq + yi//sq)%2==0 else (30,30,50)
                draw.rectangle([xi,yi,xi+sq,yi+sq], fill=color)
        bg.paste(img, mask=img)
        return ImageTk.PhotoImage(bg)


# ══════════════════════════════════════════════════════════════
# СТИЛИ — единая функция применения стилей ttk
# ══════════════════════════════════════════════════════════════
def apply_styles(root):
    style = ttk.Style(root)
    style.theme_use("clam")
    # Notebook
    style.configure("Dark.TNotebook", background=C["bg"], borderwidth=0)
    style.configure("Dark.TNotebook.Tab",
        background=C["tab_bg"], foreground=C["muted"],
        padding=[16,8], font=("Segoe UI",10,"bold"), borderwidth=0)
    style.map("Dark.TNotebook.Tab",
        background=[("selected",C["tab_sel"]),("active",C["panel3"])],
        foreground=[("selected",C["gold"]),("active",C["text"])])
    # Frame
    style.configure("Dark.TFrame", background=C["bg"])
    style.configure("Panel.TFrame", background=C["panel"])
    style.configure("Panel2.TFrame", background=C["panel2"])
    # Label
    style.configure("Dark.TLabel", background=C["bg"], foreground=C["text"], font=("Segoe UI",9))
    style.configure("Gold.TLabel", background=C["panel"], foreground=C["gold"], font=("Segoe UI",10,"bold"))
    style.configure("Title.TLabel", background=C["bg"], foreground=C["gold2"], font=("Segoe UI",14,"bold"))
    style.configure("Muted.TLabel", background=C["panel"], foreground=C["muted"], font=("Segoe UI",8))
    # Scrollbar
    style.configure("Dark.Vertical.TScrollbar",
        background=C["panel2"], troughcolor=C["bg"], arrowcolor=C["muted"],
        borderwidth=0, width=8)
    style.configure("Dark.Horizontal.TScrollbar",
        background=C["panel2"], troughcolor=C["bg"], arrowcolor=C["muted"],
        borderwidth=0, width=8)
    # Treeview
    style.configure("Dark.Treeview",
        background=C["panel2"], foreground=C["text"], fieldbackground=C["panel2"],
        borderwidth=0, font=("Segoe UI",9))
    style.configure("Dark.Treeview.Heading",
        background=C["panel3"], foreground=C["gold"], font=("Segoe UI",9,"bold"), borderwidth=0)
    style.map("Dark.Treeview",
        background=[("selected",C["accent"])], foreground=[("selected","white")])
    # Entry / Combobox
    style.configure("Dark.TEntry",
        fieldbackground=C["input_bg"], foreground=C["text"],
        insertcolor=C["gold"], borderwidth=1, relief="flat")
    style.configure("Dark.TCombobox",
        fieldbackground=C["input_bg"], background=C["panel2"],
        foreground=C["text"], arrowcolor=C["gold"])
    # Progressbar
    style.configure("Gold.Horizontal.TProgressbar",
        background=C["gold"], troughcolor=C["panel2"], borderwidth=0)
    # Separator
    style.configure("Dark.TSeparator", background=C["border"])
    # Scale
    style.configure("Dark.Horizontal.TScale",
        background=C["panel"], troughcolor=C["panel3"],
        sliderlength=16, sliderthickness=16)
    # Checkbutton
    style.configure("Dark.TCheckbutton",
        background=C["panel"], foreground=C["text"], font=("Segoe UI",9))
    style.map("Dark.TCheckbutton",
        background=[("active",C["panel2"])], foreground=[("active",C["gold"])])


def btn(parent, text, cmd, color=None, fg=None, font=None, width=None, height=None, padx=8, pady=4):
    """Создаёт стилизованную кнопку"""
    kw = dict(
        text=text, command=cmd,
        bg=color or C["panel3"], fg=fg or C["text"],
        font=font or ("Segoe UI",9,"bold"),
        relief="flat", bd=0, cursor="hand2",
        activebackground=C["accent"], activeforeground="white",
        padx=padx, pady=pady,
    )
    if width: kw["width"] = width
    if height: kw["height"] = height
    b = tk.Button(parent, **kw)
    b.bind("<Enter>", lambda e: b.config(bg=C["accent"]))
    b.bind("<Leave>", lambda e: b.config(bg=color or C["panel3"]))
    return b

def lbl(parent, text, color=None, font=None, bg=None, anchor="w"):
    return tk.Label(parent, text=text, fg=color or C["text"],
                    bg=bg or C["panel"], font=font or ("Segoe UI",9), anchor=anchor)

def sep(parent):
    return ttk.Separator(parent, orient="horizontal", style="Dark.TSeparator")

def scrolled_text(parent, height=6, width=40):
    fr = tk.Frame(parent, bg=C["input_bg"], bd=1, relief="flat")
    t = tk.Text(fr, height=height, width=width, bg=C["input_bg"], fg=C["text"],
                insertbackground=C["gold"], relief="flat", wrap="word",
                font=("Segoe UI",9), padx=6, pady=6)
    sb = ttk.Scrollbar(fr, orient="vertical", command=t.yview, style="Dark.Vertical.TScrollbar")
    t.configure(yscrollcommand=sb.set)
    sb.pack(side="right", fill="y"); t.pack(side="left", fill="both", expand=True)
    return fr, t


# ══════════════════════════════════════════════════════════════
# ТАБ 1: КАРТА МИРА
# ══════════════════════════════════════════════════════════════
class MapTab:
    def __init__(self, notebook, mapdata: MapData, project_root: Path = None):
        self.mapdata = mapdata
        self.project_root = project_root
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="🗺 Карта мира")

        # Состояние
        self.selected_tile = "GRASS"
        self.tool = "paint"
        self.zoom = 0.25
        self.pan_x = 0; self.pan_y = 0
        self.dragging = False; self.pan_start = None
        self.last_painted = None
        self.undo_stack = []; self.redo_stack = []
        self.modified = False
        self.grid_visible = True
        self.brush_size = 1
        self.selected_entity = None
        self.placing_entity = None
        self.selected_entity_type = "GOBLIN"
        self.selected_npc_type = "VENDOR"
        self.selected_zone_type = "SPAWN"
        self.selected_object_type = "CHEST"
        self.hover_x = -1; self.hover_y = -1
        self._tile_cache = {}

        self._build_ui()
        self.frame.after(200, self._safe_start)

    def _safe_start(self):
        try:
            self._fit_map()
            self.frame.after(100, self._goto_city)
            self.frame.after(300, self._update_minimap)
            self.frame.after(400, self._update_stats)
        except: pass

    def _build_ui(self):
        # Левая панель
        left = tk.Frame(self.frame, bg=C["panel"], width=200)
        left.pack(side="left", fill="y"); left.pack_propagate(False)
        self._build_left(left)

        # Канвас
        mid = tk.Frame(self.frame, bg=C["bg"])
        mid.pack(side="left", fill="both", expand=True)
        self._build_canvas(mid)

        # Правая панель
        right = tk.Frame(self.frame, bg=C["panel"], width=200)
        right.pack(side="right", fill="y"); right.pack_propagate(False)
        self._build_right(right)

    def _build_left(self, p):
        # Инструменты
        lbl(p,"🔧 ИНСТРУМЕНТЫ",C["gold"],("Segoe UI",9,"bold"),C["panel"]).pack(fill="x",padx=8,pady=(8,4))
        tool_fr = tk.Frame(p, bg=C["panel"]); tool_fr.pack(fill="x", padx=4)
        tools = [("🖌","paint"),("⬛","erase"),("🪣","fill"),("☝","select"),("👾","entity"),("💧","eyedropper")]
        self._tool_btns = {}
        for icon, t in tools:
            b = btn(tool_fr, icon, lambda t=t: self._set_tool(t), C["panel3"], C["text"], ("Segoe UI",13), width=3)
            b.pack(side="left", padx=1, pady=2)
            self._tool_btns[t] = b
        self._update_tool_buttons()

        # Размер кисти
        lbl(p,"Размер кисти:",C["muted"],("Segoe UI",8),C["panel"]).pack(fill="x",padx=8,pady=(6,0))
        self.brush_var = tk.IntVar(value=1)
        ttk.Scale(p, from_=1, to=5, variable=self.brush_var, orient="horizontal",
                  style="Dark.Horizontal.TScale",
                  command=lambda v: setattr(self,"brush_size",int(float(v)))).pack(fill="x",padx=8,pady=(0,4))

        sep(p).pack(fill="x", padx=8, pady=4)

        # Тайлы
        lbl(p,"🧱 ТАЙЛЫ",C["gold"],("Segoe UI",9,"bold"),C["panel"]).pack(fill="x",padx=8,pady=(0,4))
        tile_scroll = tk.Frame(p, bg=C["panel"]); tile_scroll.pack(fill="both", expand=True, padx=4)
        canvas_t = tk.Canvas(tile_scroll, bg=C["panel"], highlightthickness=0)
        sb = ttk.Scrollbar(tile_scroll, orient="vertical", command=canvas_t.yview,
                           style="Dark.Vertical.TScrollbar")
        canvas_t.configure(yscrollcommand=sb.set)
        sb.pack(side="right", fill="y"); canvas_t.pack(side="left", fill="both", expand=True)
        inner = tk.Frame(canvas_t, bg=C["panel"])
        canvas_t.create_window((0,0), window=inner, anchor="nw")
        inner.bind("<Configure>", lambda e: canvas_t.configure(scrollregion=canvas_t.bbox("all")))
        self._tile_buttons = {}
        for grp, tile_list in TILE_GROUPS.items():
            lbl(inner,grp,C["muted"],("Segoe UI",8),C["panel"]).pack(fill="x",padx=4,pady=(4,1))
            for tk_name in tile_list:
                info = TILES[tk_name]
                fr = tk.Frame(inner, bg=C["panel"]); fr.pack(fill="x", padx=4, pady=1)
                dot = tk.Canvas(fr, width=14, height=14, bg=C["panel"], highlightthickness=0)
                dot.pack(side="left", padx=(2,4))
                dot.create_rectangle(2,2,12,12, fill=info["color"], outline="")
                b = btn(fr, info["name"], lambda t=tk_name: self._select_tile(t),
                       C["panel2"], C["text"], ("Segoe UI",8), padx=4, pady=2)
                b.pack(side="left", fill="x", expand=True)
                self._tile_buttons[tk_name] = (fr, b, dot)
        inner.bind("<MouseWheel>", lambda e: canvas_t.yview_scroll(-1*(e.delta//120),"units"))

    def _build_canvas(self, p):
        # Статус
        self.status_var = tk.StringVar(value="Готов")
        status = tk.Label(p, textvariable=self.status_var, bg=C["panel2"], fg=C["muted"],
                         font=("Segoe UI",8), anchor="w", padx=8, pady=3)
        status.pack(fill="x")

        # Тулбар
        tb = tk.Frame(p, bg=C["panel2"], pady=3)
        tb.pack(fill="x")
        btn(tb,"Новая",self._new_map,padx=6,pady=3).pack(side="left",padx=2)
        btn(tb,"Открыть",self._open,padx=6,pady=3).pack(side="left",padx=2)
        btn(tb,"💾 Сохранить",self._save,C["accent"],padx=6,pady=3).pack(side="left",padx=2)
        btn(tb,"Сохранить как",self._save_as,padx=6,pady=3).pack(side="left",padx=2)
        btn(tb,"Экспорт C++",self._export_cpp,C["panel3"],C["orange"],padx=6,pady=3).pack(side="left",padx=2)
        tk.Label(tb,text="│",bg=C["panel2"],fg=C["border"],padx=4).pack(side="left")
        btn(tb,"↩ Undo",self._undo,padx=6,pady=3).pack(side="left",padx=2)
        btn(tb,"↪ Redo",self._redo,padx=6,pady=3).pack(side="left",padx=2)
        tk.Label(tb,text="│",bg=C["panel2"],fg=C["border"],padx=4).pack(side="left")
        self.grid_var = tk.BooleanVar(value=True)
        tk.Checkbutton(tb, text="Сетка", variable=self.grid_var,
                       command=lambda: self._toggle_grid(),
                       bg=C["panel2"], fg=C["text"], selectcolor=C["panel3"],
                       activebackground=C["panel2"], font=("Segoe UI",9)).pack(side="left",padx=4)
        btn(tb,"Город",self._goto_city,padx=6,pady=3).pack(side="left",padx=2)
        btn(tb,"↔ Всё",self._fit_map,padx=6,pady=3).pack(side="left",padx=2)

        # Канвас
        cf = tk.Frame(p, bg=C["canvas"]); cf.pack(fill="both", expand=True)
        self.canvas = tk.Canvas(cf, bg=C["canvas"], highlightthickness=0, cursor="crosshair")
        hbar = ttk.Scrollbar(cf, orient="horizontal", command=self.canvas.xview, style="Dark.Horizontal.TScrollbar")
        vbar = ttk.Scrollbar(cf, orient="vertical",   command=self.canvas.yview, style="Dark.Vertical.TScrollbar")
        self.canvas.configure(xscrollcommand=hbar.set, yscrollcommand=vbar.set)
        hbar.pack(side="bottom", fill="x"); vbar.pack(side="right", fill="y")
        self.canvas.pack(side="left", fill="both", expand=True)

        # Координаты
        self.coord_var = tk.StringVar(value="")
        tk.Label(p, textvariable=self.coord_var, bg=C["panel2"], fg=C["muted"],
                font=("Consolas",8), anchor="e", padx=8).pack(fill="x")

        # Привязки мыши
        self.canvas.bind("<ButtonPress-1>",   self._mouse_down)
        self.canvas.bind("<B1-Motion>",       self._mouse_drag)
        self.canvas.bind("<ButtonRelease-1>", self._mouse_up)
        self.canvas.bind("<ButtonPress-3>",   self._right_press)
        self.canvas.bind("<Double-Button-1>", self._double_click)
        self.canvas.bind("<B3-Motion>",       self._pan_move)
        self.canvas.bind("<ButtonRelease-3>", self._pan_end)
        self.canvas.bind("<ButtonPress-2>",   self._pan_start)
        self.canvas.bind("<B2-Motion>",       self._pan_move)
        self.canvas.bind("<ButtonRelease-2>", self._pan_end)
        self.canvas.bind("<MouseWheel>",      self._scroll_zoom)
        self.canvas.bind("<Button-4>",        lambda e: self._scroll_zoom_step(1))
        self.canvas.bind("<Button-5>",        lambda e: self._scroll_zoom_step(-1))
        self.canvas.bind("<Motion>",          self._mouse_hover)

    def _build_right(self, p):
        lbl(p,"📊 СТАТИСТИКА",C["gold"],("Segoe UI",9,"bold"),C["panel"]).pack(fill="x",padx=8,pady=(8,4))
        self.stats_var = tk.StringVar(value="Загрузка...")
        tk.Label(p, textvariable=self.stats_var, bg=C["panel"], fg=C["text"],
                font=("Consolas",8), justify="left", anchor="nw",
                padx=8, pady=4).pack(fill="x")
        sep(p).pack(fill="x", padx=8, pady=4)

        # Миникарта
        lbl(p,"🗺 Мини-карта",C["gold"],("Segoe UI",9,"bold"),C["panel"]).pack(fill="x",padx=8,pady=(0,4))
        self.minimap = tk.Canvas(p, width=180, height=180, bg=C["canvas"], highlightthickness=1,
                                 highlightbackground=C["border"])
        self.minimap.pack(padx=10, pady=4)
        self.minimap.bind("<Button-1>", self._minimap_click)

        sep(p).pack(fill="x", padx=8, pady=4)

        # Сущности
        lbl(p,"👾 СУЩНОСТИ",C["gold"],("Segoe UI",9,"bold"),C["panel"]).pack(fill="x",padx=8,pady=(0,4))

        lbl(p,"Враги:",C["muted"],("Segoe UI",8),C["panel"]).pack(fill="x",padx=8)
        self.enemy_combo = ttk.Combobox(p, values=list(ENEMIES.keys()), state="readonly",
                                        style="Dark.TCombobox", font=("Segoe UI",9))
        self.enemy_combo.set("GOBLIN"); self.enemy_combo.pack(fill="x",padx=8,pady=2)
        self.enemy_combo.bind("<<ComboboxSelected>>",
                              lambda e: setattr(self,"selected_entity_type",self.enemy_combo.get()))
        btn(p,"+ Добавить врага",lambda: self._start_place("enemy"),C["red"],padx=6,pady=3).pack(fill="x",padx=8,pady=2)

        lbl(p,"НПС:",C["muted"],("Segoe UI",8),C["panel"]).pack(fill="x",padx=8)
        self.npc_combo = ttk.Combobox(p, values=list(NPCS.keys()), state="readonly",
                                      style="Dark.TCombobox", font=("Segoe UI",9))
        self.npc_combo.set("VENDOR"); self.npc_combo.pack(fill="x",padx=8,pady=2)
        self.npc_combo.bind("<<ComboboxSelected>>",
                            lambda e: setattr(self,"selected_npc_type",self.npc_combo.get()))
        btn(p,"+ Добавить НПС",lambda: self._start_place("npc"),C["blue"],padx=6,pady=3).pack(fill="x",padx=8,pady=2)

        lbl(p,"Зоны:",C["muted"],("Segoe UI",8),C["panel"]).pack(fill="x",padx=8)
        self.zone_combo = ttk.Combobox(p, values=list(ZONES.keys()), state="readonly",
                                       style="Dark.TCombobox", font=("Segoe UI",9))
        self.zone_combo.set("SPAWN"); self.zone_combo.pack(fill="x",padx=8,pady=2)
        btn(p,"+ Добавить зону",lambda: self._start_place("zone"),C["accent"],padx=6,pady=3).pack(fill="x",padx=8,pady=2)

        lbl(p,"Объекты:",C["muted"],("Segoe UI",8),C["panel"]).pack(fill="x",padx=8)
        self.object_combo = ttk.Combobox(p, values=list(OBJECTS.keys()), state="readonly",
                                         style="Dark.TCombobox", font=("Segoe UI",9))
        self.object_combo.set("CHEST"); self.object_combo.pack(fill="x",padx=8,pady=2)
        self.object_combo.bind("<<ComboboxSelected>>",
            lambda e: setattr(self,"selected_object_type",self.object_combo.get()))
        btn(p,"+ Добавить объект",lambda: self._start_place("object"),
            C["cyan"],padx=6,pady=3).pack(fill="x",padx=8,pady=2)

        sep(p).pack(fill="x", padx=8, pady=4)
        btn(p,"🗑 Очистить всё",self._clear_all,C["danger"],padx=6,pady=3).pack(fill="x",padx=8,pady=2)
        btn(p,"🎲 Заполнить тайлом",self._fill_all,padx=6,pady=3).pack(fill="x",padx=8,pady=2)

    # ── Инструменты ─────────────────────────────────────────────
    def _set_tool(self, t):
        self.tool = t
        if t != "entity": self.placing_entity = None
        self._update_tool_buttons()
        self._update_status()

    def _update_tool_buttons(self):
        for t, b in self._tool_btns.items():
            b.config(bg=C["accent"] if t==self.tool else C["panel3"])

    def _select_tile(self, t):
        self.selected_tile = t
        self._update_status()

    def _start_place(self, kind):
        self._set_tool("entity")
        if kind == "enemy":
            self.selected_entity_type = self.enemy_combo.get()
            et_type = self.selected_entity_type
        elif kind == "npc":
            self.selected_npc_type = self.npc_combo.get()
            et_type = self.selected_npc_type
        elif kind == "object":
            self.selected_object_type = self.object_combo.get()
            et_type = self.selected_object_type
        else:
            et_type = self.zone_combo.get()
        self.placing_entity = (et_type, kind)
        self.canvas.config(cursor="plus")
        self._update_status(f"👆 Кликни на карту — {et_type}")

    # ── Canvas coords ────────────────────────────────────────────
    def _canvas_to_tile(self, cx, cy):
        s = TILE * self.zoom
        tx = int((cx - self.pan_x) / s)
        ty = int((cy - self.pan_y) / s)
        return tx, ty

    def _tile_to_canvas(self, tx, ty):
        s = TILE * self.zoom
        return tx*s + self.pan_x, ty*s + self.pan_y

    # ── Mouse ────────────────────────────────────────────────────
    def _mouse_hover(self, e):
        tx, ty = self._canvas_to_tile(e.x, e.y)
        if 0<=tx<MAP_W and 0<=ty<MAP_H:
            tile = self.mapdata.get(tx,ty)
            self.coord_var.set(f"X:{tx} Y:{ty} | {TILES.get(tile,{}).get('name',tile)}")
        self.hover_x = tx; self.hover_y = ty

    def _mouse_down(self, e):
        tx, ty = self._canvas_to_tile(e.x, e.y)
        if self.placing_entity:
            self._place_entity(tx, ty)
            return
        if self.tool == "select":
            ents = self.mapdata.entities_at(tx, ty)
            self.selected_entity = ents[0] if ents else None
            return
        if self.tool == "eyedropper":
            t = self.mapdata.get(tx, ty)
            if t: self._select_tile(t)
            return
        if self.tool == "fill":
            self._push_undo()
            self.mapdata.flood_fill(tx, ty, self.selected_tile)
            self._draw_full_map(); self._update_minimap()
            return
        self._push_undo()
        self._paint(tx, ty)

    def _mouse_drag(self, e):
        tx, ty = self._canvas_to_tile(e.x, e.y)
        if self.last_painted == (tx,ty): return
        self.last_painted = (tx,ty)
        if self.tool in ("paint","erase"): self._paint(tx,ty)

    def _mouse_up(self, e): self.last_painted = None

    def _paint(self, tx, ty):
        bs = self.brush_size
        for dy in range(-bs+1, bs):
            for dx in range(-bs+1, bs):
                x,y = tx+dx, ty+dy
                if 0<=x<MAP_W and 0<=y<MAP_H:
                    tile = "GRASS" if self.tool=="erase" else self.selected_tile
                    self.mapdata.set(x,y,tile)
                    self._draw_tile(x,y)
        self._update_minimap()
        self.modified = True
        self._update_status()

    def _right_press(self, e):
        tx,ty = self._canvas_to_tile(e.x,e.y)
        ents = self.mapdata.registry.find_at(tx,ty)
        if ents:
            self.selected_entity = ents[0]; self._draw_full_map()
            self._open_inspector(ents[0])
        else:
            self._pan_start(e)

    def _double_click(self, e):
        tx,ty = self._canvas_to_tile(e.x,e.y)
        ents = self.mapdata.registry.find_at(tx,ty)
        if ents:
            self.selected_entity = ents[0]
            self._open_inspector(ents[0])

    def _open_inspector(self, entity):
        win = tk.Toplevel(self.frame.winfo_toplevel())
        win.title(f"[{entity.get('id','')}] {entity.get('kind','').upper()} › {entity.get('type','')}")
        win.geometry("400x480"); win.configure(bg=C["panel"]); win.grab_set()
        tk.Label(win, text=f"  {entity.get('kind','').upper()} › {entity.get('type','')}",
                 bg=C["accent"],fg="white",font=("Segoe UI",10,"bold"),anchor="w",padx=8,pady=6
                 ).pack(fill="x")
        tk.Label(win, text=f"  ID: {entity.get('id','')}",
                 bg=C["panel2"],fg=C["gold"],font=("Consolas",9),anchor="w",padx=8,pady=3
                 ).pack(fill="x")
        frm=tk.Frame(win,bg=C["panel"]); frm.pack(fill="both",expand=True,padx=12,pady=8)
        frm.columnconfigure(1,weight=1)
        xv=tk.StringVar(value=str(entity["x"]))
        yv=tk.StringVar(value=str(entity["y"]))
        prop_vars={}; row_i=0
        def mk_row(label,var,ri):
            tk.Label(frm,text=label,bg=C["panel"],fg=C["muted"],font=("Segoe UI",9),anchor="w"
                     ).grid(row=ri,column=0,sticky="w",pady=2,padx=4)
            tk.Entry(frm,textvariable=var,bg=C["input_bg"],fg=C["text"],
                     font=("Consolas",9),insertbackground=C["text"],relief="flat",bd=4
                     ).grid(row=ri,column=1,sticky="ew",pady=2,padx=4)
        mk_row("X:",xv,row_i); row_i+=1
        mk_row("Y:",yv,row_i); row_i+=1
        for k,v in entity.get("props",{}).items():
            pv=tk.StringVar(value="" if v is None else str(v))
            mk_row(f"{k}:",pv,row_i); prop_vars[k]=pv; row_i+=1
        bf=tk.Frame(win,bg=C["panel2"]); bf.pack(fill="x",side="bottom",pady=6)
        def _apply():
            try: entity["x"]=int(xv.get())
            except: pass
            try: entity["y"]=int(yv.get())
            except: pass
            for k,pv in prop_vars.items():
                raw=pv.get(); old_val=entity["props"].get(k)
                if isinstance(old_val,bool): entity["props"][k]=raw.lower() in("true","1","yes")
                elif isinstance(old_val,int):
                    try: entity["props"][k]=int(raw)
                    except: pass
                elif isinstance(old_val,float):
                    try: entity["props"][k]=float(raw)
                    except: pass
                else: entity["props"][k]=raw
            self.modified=True; self._draw_full_map(); self._update_stats(); win.destroy()
        def _delete():
            eid=entity.get("id","")
            if messagebox.askyesno("Удалить",f"Удалить {eid}?",parent=win):
                self.mapdata.registry.remove(eid); self.selected_entity=None
                self.modified=True; self._draw_full_map(); self._update_stats()
                self._update_minimap(); win.destroy()
        btn(bf,"✅ Применить",_apply,C["accent"],padx=10,pady=4).pack(side="left",padx=8)
        btn(bf,"🗑 Удалить",_delete,C["danger"],padx=10,pady=4).pack(side="left",padx=4)
        btn(bf,"✖ Отмена",win.destroy,padx=10,pady=4).pack(side="right",padx=8)

    def _pan_start(self, e): self.dragging=True; self.pan_start=(e.x-self.pan_x, e.y-self.pan_y)
    def _pan_move(self, e):
        if self.dragging and self.pan_start:
            self.pan_x = e.x - self.pan_start[0]
            self.pan_y = e.y - self.pan_start[1]
            self._draw_full_map()
    def _pan_end(self, e): self.dragging=False; self.pan_start=None

    def _scroll_zoom(self, e):
        factor = 1.2 if e.delta > 0 else 1/1.2
        self._set_zoom(self.zoom*factor, e.x, e.y)
    def _scroll_zoom_step(self, d):
        factor = 1.2 if d > 0 else 1/1.2
        w=self.canvas.winfo_width(); h=self.canvas.winfo_height()
        self._set_zoom(self.zoom*factor, w//2, h//2)

    def _set_zoom(self, z, cx=None, cy=None):
        z = max(0.05, min(4.0, z))
        if cx is not None:
            mx,my = self._canvas_to_tile(cx,cy)
            self.zoom = z
            nx,ny = self._tile_to_canvas(mx,my)
            self.pan_x += cx-nx; self.pan_y += cy-ny
        else:
            self.zoom = z
        self._draw_full_map()
        self._update_status()

    def _fit_map(self):
        w=self.canvas.winfo_width() or 900
        h=self.canvas.winfo_height() or 600
        zx=(w-20)/(MAP_W*TILE); zy=(h-20)/(MAP_H*TILE)
        self.zoom=min(zx,zy)
        mw=MAP_W*TILE*self.zoom; mh=MAP_H*TILE*self.zoom
        self.pan_x=(w-mw)/2; self.pan_y=(h-mh)/2
        self._draw_full_map()

    def _goto_city(self):
        w=self.canvas.winfo_width() or 900
        h=self.canvas.winfo_height() or 600
        self.pan_x = w/2 - CITY_CX*TILE*self.zoom
        self.pan_y = h/2 - CITY_CY*TILE*self.zoom
        self._draw_full_map()

    def _toggle_grid(self):
        self.grid_visible = self.grid_var.get()
        self._draw_full_map()

    # ── Рисование ────────────────────────────────────────────────
    def _draw_full_map(self):
        self.canvas.delete("all")
        s = TILE * self.zoom
        w = self.canvas.winfo_width() or 900
        h = self.canvas.winfo_height() or 600

        x0 = max(0, int(-self.pan_x/s))
        y0 = max(0, int(-self.pan_y/s))
        x1 = min(MAP_W, int((w-self.pan_x)/s)+2)
        y1 = min(MAP_H, int((h-self.pan_y)/s)+2)

        for ty in range(y0, y1):
            for tx in range(x0, x1):
                self._draw_tile(tx, ty, batch=True)

        # Зоны (полупрозрачные — эмуляция через stipple)
        for e in self.mapdata.entities:
            if e.get("kind") == "zone":
                zdata = ZONES.get(e["type"], {})
                color = zdata.get("color","#ffffff")
                r = e.get("props",{}).get("radius",3)
                cx2,cy2 = self._tile_to_canvas(e["x"],e["y"])
                self.canvas.create_oval(
                    cx2-r*s, cy2-r*s, cx2+r*s, cy2+r*s,
                    outline=color, width=2, fill=color, stipple="gray25",
                    tags="zone")
                if s > 8:
                    self.canvas.create_text(cx2, cy2, text=zdata.get("name",""),
                                           fill="white", font=("Segoe UI",int(max(7,s*0.4))), tags="zone")

        # Сущности
        for e in self.mapdata.entities:
            if e.get("kind") == "zone": continue
            self._draw_entity(e, s)

        # Сетка
        if self.grid_visible and s >= 4:
            alpha_col = "#1a1832" if s<10 else "#2a2255"
            for tx in range(x0, x1):
                cx2 = tx*s + self.pan_x
                self.canvas.create_line(cx2,y0*s+self.pan_y, cx2,y1*s+self.pan_y, fill=alpha_col, tags="grid")
            for ty in range(y0, y1):
                cy2 = ty*s + self.pan_y
                self.canvas.create_line(x0*s+self.pan_x,cy2, x1*s+self.pan_x,cy2, fill=alpha_col, tags="grid")

        # Highlight города
        if s > 4:
            ccx,ccy = self._tile_to_canvas(CITY_CX, CITY_CY)
            cr = CITY_RADIUS*s
            self.canvas.create_oval(ccx-cr,ccy-cr,ccx+cr,ccy+cr, outline="#f0c04044", width=1)

    def _draw_tile(self, tx, ty, batch=False):
        s = TILE * self.zoom
        cx2 = tx*s + self.pan_x
        cy2 = ty*s + self.pan_y
        tile = self.mapdata.get(tx, ty) or "GRASS"
        color = TILES.get(tile, TILES["GRASS"])["color"]

        # Небольшое затемнение по шахматному
        if (tx+ty)%2==0:
            r,g,b = int(color[1:3],16),int(color[3:5],16),int(color[5:7],16)
            r=max(0,r-8);g=max(0,g-8);b=max(0,b-8)
            color=f"#{r:02x}{g:02x}{b:02x}"

        if not batch:
            self.canvas.delete(f"tile_{tx}_{ty}")
        self.canvas.create_rectangle(cx2, cy2, cx2+s, cy2+s, fill=color, outline="", tags=f"tile_{tx}_{ty}")

    def _draw_entity(self, e, s):
        tx=e["x"]; ty=e["y"]
        cx2=tx*s+self.pan_x+s/2; cy2=ty*s+self.pan_y+s/2
        kind=e.get("kind")
        if kind=="enemy":
            info=ENEMIES.get(e["type"],{}); color=info.get("color","#ff0000")
            boss=e.get("props",{}).get("boss",False)
            r=s*0.45 if boss else s*0.35
            self.canvas.create_oval(cx2-r,cy2-r,cx2+r,cy2+r,
                fill=color, outline=C["gold"] if boss else "#333", width=2 if boss else 1)
            if s>=12:
                self.canvas.create_text(cx2,cy2, text=info.get("emoji","?"),
                    font=("",max(8,int(s*0.5))), fill="white")
        elif kind=="npc":
            info=NPCS.get(e["type"],{}); color=info.get("color","#44aaff")
            r=s*0.38
            self.canvas.create_oval(cx2-r,cy2-r,cx2+r,cy2+r, fill=color, outline="#fff", width=1)
            if s>=12:
                self.canvas.create_text(cx2,cy2, text=info.get("emoji","?"),
                    font=("",max(8,int(s*0.45))), fill="white")
        elif kind=="object":
            info=OBJECTS.get(e["type"],{}); color=info.get("color","#cc8822")
            r=s*0.3
            self.canvas.create_rectangle(cx2-r,cy2-r,cx2+r,cy2+r,
                fill=color, outline="#fff", width=1)
            if s>=10:
                self.canvas.create_text(cx2,cy2, text=info.get("emoji","📦"),
                    font=("",max(7,int(s*0.4))), fill="white")
        if e is self.selected_entity:
            self.canvas.create_rectangle(tx*s+self.pan_x, ty*s+self.pan_y,
                                         tx*s+self.pan_x+s, ty*s+self.pan_y+s,
                                         outline=C["gold"], width=2, dash=(4,2))
            if s>=10 and e.get("id"):
                self.canvas.create_text(tx*s+self.pan_x+2, ty*s+self.pan_y+2,
                    text=e["id"], anchor="nw",
                    fill=C["gold"], font=("Consolas",max(6,int(s*0.28))))

    # ── Entity placement ──────────────────────────────────────────
    def _place_entity(self, tx, ty):
        if not (0<=tx<MAP_W and 0<=ty<MAP_H): return
        et_type, kind = self.placing_entity
        # Если размещение из системы префабов — используем их props как основу
        pfb_props = getattr(self, "_prefab_props", None) or {}
        if kind=="enemy":
            info=ENEMIES.get(et_type,{})
            base = {"level":1,"boss":False,
                    "hp":info.get("hp",50),"dmg":info.get("dmg",10)}
            base.update(pfb_props)
            self.mapdata.registry.add("enemy", et_type, tx, ty, props=base)
        elif kind=="npc":
            default_name = pfb_props.get("name") or NPCS.get(et_type,{}).get("name","НПС")
            name=(simpledialog.askstring("НПС","Имя персонажа:",
                parent=self.frame.winfo_toplevel(), initialvalue=default_name)
                or default_name)
            base = {"name":name,"quest_id":None,"dialogue_id":None}
            base.update(pfb_props)
            base["name"] = name   # имя из диалога всегда приоритетнее
            self.mapdata.registry.add("npc", et_type, tx, ty, props=base)
        elif kind=="object":
            base = _default_object_props(et_type)
            base.update(pfb_props)
            self.mapdata.registry.add("object", et_type, tx, ty, props=base)
        elif kind=="zone":
            base = {"radius":ZONES.get(et_type,{}).get("alpha",50)//10+3}
            base.update(pfb_props)
            self.mapdata.registry.add("zone", et_type, tx, ty, props=base)
        self._prefab_props = None
        self.placing_entity=None; self.canvas.config(cursor="crosshair")
        self.modified=True; self._draw_full_map(); self._update_stats(); self._update_status()

    # ── Minimap ───────────────────────────────────────────────────
    def _update_minimap(self):
        if not PILLOW: return
        try:
            mw=MAP_W; mh=MAP_H; scale=1
            img=Image.new("RGB",(mw*scale,mh*scale))
            for ty in range(mh):
                for tx in range(mw):
                    tile=self.mapdata.get(tx,ty) or "GRASS"
                    hx=TILES.get(tile,TILES["GRASS"])["color"].lstrip("#")
                    r,g,b=int(hx[:2],16),int(hx[2:4],16),int(hx[4:],16)
                    img.putpixel((tx*scale,ty*scale),(r,g,b))
            img=img.resize((180,180),Image.NEAREST)
            self._minimap_img=ImageTk.PhotoImage(img)
            self.minimap.delete("all")
            self.minimap.create_image(0,0,anchor="nw",image=self._minimap_img)
            # Viewport rect
            s=self.zoom*TILE; cw=self.canvas.winfo_width() or 900; ch=self.canvas.winfo_height() or 600
            rx0=int(-self.pan_x/s*(180/mw)); ry0=int(-self.pan_y/s*(180/mh))
            rx1=int((cw-self.pan_x)/s*(180/mw)); ry1=int((ch-self.pan_y)/s*(180/mh))
            rx0=max(0,rx0);ry0=max(0,ry0);rx1=min(180,rx1);ry1=min(180,ry1)
            self.minimap.create_rectangle(rx0,ry0,rx1,ry1,outline=C["gold"],width=1)
            # Город
            self.minimap.create_oval(int(CITY_CX/mw*180)-3,int(CITY_CY/mh*180)-3,
                                    int(CITY_CX/mw*180)+3,int(CITY_CY/mh*180)+3,
                                    fill=C["gold"],outline="")
        except: pass

    def _minimap_click(self, e):
        tx=int(e.x/180*MAP_W); ty=int(e.y/180*MAP_H)
        w=self.canvas.winfo_width() or 900; h=self.canvas.winfo_height() or 600
        self.pan_x=w/2-tx*TILE*self.zoom; self.pan_y=h/2-ty*TILE*self.zoom
        self._draw_full_map(); self._update_minimap()

    # ── Undo/Redo ─────────────────────────────────────────────────
    def _push_undo(self):
        self.undo_stack.append(copy.deepcopy(self.mapdata.tiles))
        if len(self.undo_stack)>50: self.undo_stack.pop(0)
        self.redo_stack.clear()

    def _undo(self):
        if not self.undo_stack: return
        self.redo_stack.append(copy.deepcopy(self.mapdata.tiles))
        self.mapdata.tiles=self.undo_stack.pop()
        self._draw_full_map(); self._update_minimap(); self._update_stats()

    def _redo(self):
        if not self.redo_stack: return
        self.undo_stack.append(copy.deepcopy(self.mapdata.tiles))
        self.mapdata.tiles=self.redo_stack.pop()
        self._draw_full_map(); self._update_minimap(); self._update_stats()

    # ── File ops ──────────────────────────────────────────────────
    def _new_map(self):
        if not messagebox.askyesno("Новая карта","Создать новую карту?"): return
        self.mapdata.tiles=generate_default_map()
        self.mapdata.registry.from_list(generate_default_entities())
        self.modified=False; self._draw_full_map(); self._update_minimap(); self._update_stats()

    def _save(self):
        # Если путь не задан — пробуем project_root/assets/map.json, потом диалог
        if not self.mapdata.filepath:
            if self.project_root:
                default_path = self.project_root / "assets" / "map.json"
                default_path.parent.mkdir(parents=True, exist_ok=True)
                self.mapdata.filepath = str(default_path)
            else:
                p = filedialog.asksaveasfilename(defaultextension=".json",
                                                  filetypes=[("JSON", "*.json")])
                if not p: return
                self.mapdata.filepath = p
        try:
            data = self.mapdata.to_dict()
            Path(self.mapdata.filepath).write_text(
                json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8"
            )
            self.modified = False
            # Синхронизируем в scenes/ чтобы движок видел актуальные данные
            self._sync_to_scenes(data)
        except Exception as ex:
            messagebox.showerror("Ошибка сохранения", str(ex))

    def _sync_to_scenes(self, data=None):
        """Синхронизирует карту → assets/scenes/<id>.json (формат движка).
        Вызывается автоматически при каждом сохранении."""
        if not self.project_root:
            return
        if data is None:
            data = self.mapdata.to_dict()
        try:
            scenes_dir = self.project_root / "assets" / "scenes"
            scenes_dir.mkdir(parents=True, exist_ok=True)
            # Определяем scene_id из metadata или имени файла карты
            scene_id = data.get("metadata", {}).get("scene_id", "")
            if not scene_id and self.mapdata.filepath:
                stem = Path(self.mapdata.filepath).stem.lower().replace(" ", "_")
                scene_id = stem
            if not scene_id:
                scene_id = "aethoria_city"
            scene_file = scenes_dir / f"{scene_id}.json"
            scene_file.write_text(
                json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8"
            )
            n = len(self.mapdata.registry.all())
            self._update_status(
                f"💾 Сохранено → {Path(self.mapdata.filepath).name}"
                f"  +  scenes/{scene_id}.json  ({n} сущностей)"
            )
        except Exception as ex:
            self._update_status(f"💾 Сохранено (sync failed: {ex})")
            print(f"[WARN] _sync_to_scenes: {ex}")

    def _open(self):
        p=filedialog.askopenfilename(filetypes=[("JSON","*.json"),("Все","*.*")])
        if not p: return
        try:
            d=json.loads(Path(p).read_text(encoding="utf-8"))
            self.mapdata.from_dict(d); self.mapdata.filepath=p
            self.modified=False
            self._draw_full_map(); self._update_minimap(); self._update_stats()
            self._update_status(f"📂 Загружено: {Path(p).name}")
        except Exception as ex:
            messagebox.showerror("Ошибка загрузки",str(ex))

    def _save_as(self):
        """Сохранить в другой файл (смена canonical path)"""
        p = filedialog.asksaveasfilename(
            defaultextension=".json",
            filetypes=[("JSON", "*.json")],
            initialfile="map.json"
        )
        if not p: return
        self.mapdata.filepath = p
        self._save()

    def _export_cpp(self):
        p=filedialog.asksaveasfilename(defaultextension=".cpp",filetypes=[("C++","*.cpp"),("Все","*.*")])
        if not p: return
        Path(p).write_text(self.mapdata.generate_cpp(), encoding="utf-8")
        messagebox.showinfo("Экспорт",f"C++ код сохранён: {p}")

    # ── Utils ─────────────────────────────────────────────────────
    def _clear_all(self):
        if messagebox.askyesno("Очистить","Очистить все объекты?"):
            self.mapdata.registry.clear(); self._draw_full_map(); self._update_minimap(); self._update_stats()

    def _fill_all(self):
        if messagebox.askyesno("Заполнить",f"Заполнить всё '{TILES[self.selected_tile]['name']}'?"):
            self._push_undo()
            for y in range(MAP_H):
                for x in range(MAP_W):
                    self.mapdata.tiles[y][x]=self.selected_tile
            self._draw_full_map(); self._update_minimap()

    def _update_stats(self):
        tile_counts={}
        for row in self.mapdata.tiles:
            for t in row: tile_counts[t]=tile_counts.get(t,0)+1
        enemies=sum(1 for e in self.mapdata.entities if e.get("kind")=="enemy")
        npcs=sum(1 for e in self.mapdata.entities if e.get("kind")=="npc")
        zones=sum(1 for e in self.mapdata.entities if e.get("kind")=="zone")
        bosses=sum(1 for e in self.mapdata.entities if e.get("kind")=="enemy" and e.get("props",{}).get("boss"))
        top_tiles=sorted(tile_counts.items(),key=lambda x:-x[1])[:5]
        top_str="\n".join(f"  {TILES.get(t,{}).get('name',t)[:10]:10s} {c}" for t,c in top_tiles)
        self.stats_var.set(
            f"Размер: {MAP_W}×{MAP_H}\n"
            f"Врагов: {enemies} (боссов: {bosses})\n"
            f"НПС:    {npcs}\n"
            f"Зон:    {zones}\n"
            f"Zoom:   {int(self.zoom*100)}%\n\nТоп тайлов:\n{top_str}"
        )

    def _update_status(self, msg=None):
        if msg: self.status_var.set(msg); return
        tools={"paint":"🖌 Рисовать","erase":"⬛ Ластик","fill":"🪣 Заливка",
               "select":"☝ Выбрать","entity":"👾 Объект","eyedropper":"💧 Пипетка"}
        tile=TILES.get(self.selected_tile,{}).get("name",self.selected_tile)
        self.status_var.set(f"{tools.get(self.tool,self.tool)}  |  {tile}{'  [*]' if self.modified else ''}")


# ══════════════════════════════════════════════════════════════
# ТАБ 2: ASSET PIPELINE — АВТОМАТИЧЕСКИЙ ИМПОРТ АНИМАЦИЙ
# ══════════════════════════════════════════════════════════════
class AssetPipelineTab:
    def __init__(self, notebook, project_root: Path):
        self.project_root = project_root
        self.pipeline = AnimationPipeline(project_root)
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="🎬 Ассеты")
        self._imported_list = []
        self._preview_img = None
        self._build_ui()
        self._refresh_list()

    def _build_ui(self):
        # Заголовок
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=12)
        hdr.pack(fill="x")
        tk.Label(hdr, text="🎬 PIPELINE АНИМАЦИЙ", bg=C["panel"], fg=C["gold2"],
                font=("Segoe UI",14,"bold"), padx=20).pack(side="left")
        tk.Label(hdr, text="MP4 / GIF / PNG → Sprite Sheet + JSON → Движок AETHORIA",
                bg=C["panel"], fg=C["muted"], font=("Segoe UI",9), padx=8).pack(side="left")

        # Основной split
        main = tk.Frame(self.frame, bg=C["bg"])
        main.pack(fill="both", expand=True, padx=12, pady=8)

        # Левая панель — импорт
        left = tk.Frame(main, bg=C["panel"], width=420)
        left.pack(side="left", fill="y", padx=(0,8))
        left.pack_propagate(False)
        self._build_import_panel(left)

        # Правая панель — список и превью
        right = tk.Frame(main, bg=C["bg"])
        right.pack(side="left", fill="both", expand=True)
        self._build_right_panel(right)

    def _build_import_panel(self, p):
        # Зона перетаскивания
        drop_fr = tk.Frame(p, bg=C["panel2"], bd=2, relief="groove", cursor="hand2")
        drop_fr.pack(fill="x", padx=12, pady=(12,8))
        tk.Label(drop_fr, text="📥", bg=C["panel2"], fg=C["accent2"],
                font=("Segoe UI",36), pady=10).pack()
        tk.Label(drop_fr, text="Перетащи файл сюда\nили нажми кнопку ниже",
                bg=C["panel2"], fg=C["muted"], font=("Segoe UI",10)).pack(pady=(0,8))
        tk.Label(drop_fr, text="MP4 · AVI · MOV · GIF · PNG · WEBP",
                bg=C["panel2"], fg=C["border"], font=("Segoe UI",8)).pack(pady=(0,10))

        btn(p, "📂 Выбрать файл(ы)", self._browse_files, C["accent"], "white",
            ("Segoe UI",10,"bold"), padx=10, pady=6).pack(fill="x", padx=12, pady=(0,12))

        sep(p).pack(fill="x", padx=12, pady=4)

        # Настройки
        lbl(p,"⚙ НАСТРОЙКИ ИМПОРТА",C["gold"],("Segoe UI",9,"bold"),C["panel"]).pack(fill="x",padx=12,pady=(8,4))

        def row(label, widget_fn):
            fr = tk.Frame(p, bg=C["panel"]); fr.pack(fill="x", padx=12, pady=2)
            tk.Label(fr, text=label, bg=C["panel"], fg=C["text"],
                    font=("Segoe UI",9), width=16, anchor="w").pack(side="left")
            w = widget_fn(fr); w.pack(side="left", fill="x", expand=True, padx=(4,0))
            return w

        # Имя сущности
        self.entity_var = tk.StringVar(value="auto")
        row("Сущность:", lambda f: tk.Entry(f, textvariable=self.entity_var,
            bg=C["input_bg"], fg=C["text"], insertbackground=C["gold"],
            relief="flat", font=("Segoe UI",9)))

        # Действие
        self.action_var = tk.StringVar(value="auto")
        def act_widget(f):
            vals = ["auto"] + ANIM_ACTIONS
            w = ttk.Combobox(f, textvariable=self.action_var, values=vals,
                            style="Dark.TCombobox", font=("Segoe UI",9))
            return w
        row("Действие:", act_widget)

        # Размер кадра
        self.frame_w = tk.StringVar(value="64")
        self.frame_h = tk.StringVar(value="64")
        fr_size = tk.Frame(p, bg=C["panel"]); fr_size.pack(fill="x", padx=12, pady=2)
        tk.Label(fr_size, text="Размер кадра:", bg=C["panel"], fg=C["text"],
                font=("Segoe UI",9), width=16, anchor="w").pack(side="left")
        ttk.Combobox(fr_size, textvariable=self.frame_w,
                    values=[str(s) for s in ANIM_FRAME_SIZES],
                    style="Dark.TCombobox", font=("Segoe UI",9), width=6).pack(side="left",padx=2)
        tk.Label(fr_size, text="×", bg=C["panel"], fg=C["muted"]).pack(side="left")
        ttk.Combobox(fr_size, textvariable=self.frame_h,
                    values=[str(s) for s in ANIM_FRAME_SIZES],
                    style="Dark.TCombobox", font=("Segoe UI",9), width=6).pack(side="left",padx=2)

        # FPS
        self.fps_var = tk.StringVar(value="12")
        def fps_widget(f):
            return ttk.Combobox(f, textvariable=self.fps_var,
                values=[str(x) for x in ANIM_FPS_PRESETS],
                style="Dark.TCombobox", font=("Segoe UI",9))
        row("FPS:", fps_widget)

        sep(p).pack(fill="x", padx=12, pady=8)

        # Прогресс
        self.prog_var = tk.StringVar(value="")
        self.prog_lbl = tk.Label(p, textvariable=self.prog_var, bg=C["panel"],
                                fg=C["cyan"], font=("Segoe UI",9))
        self.prog_lbl.pack(fill="x", padx=12)

        self.progressbar = ttk.Progressbar(p, orient="horizontal", mode="determinate",
                                           style="Gold.Horizontal.TProgressbar")
        self.progressbar.pack(fill="x", padx=12, pady=4)

        # Лог
        log_fr, self.log_text = scrolled_text(p, height=8)
        log_fr.pack(fill="both", expand=True, padx=12, pady=4)

        # Кнопки
        btn_fr = tk.Frame(p, bg=C["panel"]); btn_fr.pack(fill="x", padx=12, pady=8)
        btn(btn_fr,"🗑 Очистить лог",
            lambda: self.log_text.delete("1.0","end"),
            padx=6,pady=4).pack(side="right",padx=4)
        btn(btn_fr,"📁 Открыть папку спрайтов",
            self._open_sprites_dir, C["panel3"], C["blue"],
            padx=6,pady=4).pack(side="right",padx=4)

    def _build_right_panel(self, p):
        # Превью
        prev_fr = tk.Frame(p, bg=C["panel"], bd=1, relief="flat")
        prev_fr.pack(fill="x", pady=(0,8))
        tk.Label(prev_fr, text="👁 ПРЕВЬЮ СПРАЙТ-ШИТА", bg=C["panel"], fg=C["gold"],
                font=("Segoe UI",9,"bold"), padx=12, pady=6).pack(anchor="w")
        self.preview_canvas = tk.Canvas(prev_fr, bg=C["canvas"], height=200,
                                       highlightthickness=0)
        self.preview_canvas.pack(fill="x", padx=12, pady=(0,8))
        self.preview_info = tk.Label(prev_fr, text="Нет превью", bg=C["panel"],
                                    fg=C["muted"], font=("Segoe UI",8), padx=12, pady=4)
        self.preview_info.pack(anchor="w")

        # Список импортированных
        lbl_fr = tk.Frame(p, bg=C["panel"]); lbl_fr.pack(fill="x")
        tk.Label(lbl_fr, text="📋 ИМПОРТИРОВАННЫЕ АНИМАЦИИ", bg=C["panel"],
                fg=C["gold"], font=("Segoe UI",9,"bold"), padx=12, pady=6).pack(side="left")
        btn(lbl_fr,"♻ Обновить",self._refresh_list,padx=6,pady=3).pack(side="right",padx=8)

        cols = ("entity","action","frames","size","fps","file")
        self.anim_tree = ttk.Treeview(p, columns=cols, show="headings",
                                      style="Dark.Treeview", height=15)
        hdr_map = [("entity","Сущность",80),("action","Действие",70),
                   ("frames","Кадров",60),("size","Размер",70),
                   ("fps","FPS",40),("file","Файл",200)]
        for col,text,w in hdr_map:
            self.anim_tree.heading(col,text=text)
            self.anim_tree.column(col,width=w,anchor="center" if col not in ("file","entity") else "w")

        sb = ttk.Scrollbar(p, orient="vertical", command=self.anim_tree.yview,
                          style="Dark.Vertical.TScrollbar")
        self.anim_tree.configure(yscrollcommand=sb.set)

        tr_fr = tk.Frame(p, bg=C["bg"]); tr_fr.pack(fill="both", expand=True)
        self.anim_tree.pack(in_=tr_fr, side="left", fill="both", expand=True)
        sb.pack(in_=tr_fr, side="right", fill="y")
        self.anim_tree.bind("<<TreeviewSelect>>", self._on_anim_select)

        # Кнопки таблицы
        tb_btn = tk.Frame(p, bg=C["bg"], pady=4); tb_btn.pack(fill="x")
        btn(tb_btn,"🗑 Удалить",self._delete_anim,C["danger"],padx=8,pady=4).pack(side="right",padx=8)
        btn(tb_btn,"📋 Копировать JSON",self._copy_json,padx=8,pady=4).pack(side="right",padx=4)

    def _browse_files(self):
        files = filedialog.askopenfilenames(
            title="Выбрать файлы анимаций",
            filetypes=[
                ("Видео/Изображения","*.mp4 *.avi *.mov *.gif *.png *.webp *.jpg"),
                ("Видео","*.mp4 *.avi *.mov *.mkv *.webm"),
                ("GIF","*.gif"),
                ("Изображения","*.png *.jpg *.jpeg *.webp"),
                ("Все","*.*"),
            ]
        )
        if files:
            threading.Thread(target=self._import_files_thread, args=(list(files),), daemon=True).start()

    def _import_files_thread(self, files):
        for i, filepath in enumerate(files):
            try:
                self._log(f"\n{'─'*50}")
                self._log(f"📥 Импорт: {Path(filepath).name}")

                entity_raw = self.entity_var.get().strip()
                action_raw = self.action_var.get().strip()
                entity = None if entity_raw in ("auto","") else entity_raw
                action = None if action_raw in ("auto","") else action_raw

                try: w = int(self.frame_w.get())
                except: w = 64
                try: h = int(self.frame_h.get())
                except: h = 64
                try: fps = int(self.fps_var.get())
                except: fps = 12

                def progress_cb(pct, msg):
                    self.frame.after(0, lambda: self._set_progress(pct, msg))
                    self.frame.after(0, lambda: self._log(f"  {msg}"))

                config = self.pipeline.import_file(
                    filepath, entity=entity, action=action,
                    target_w=w, target_h=h, fps=fps,
                    progress_cb=progress_cb
                )

                cfg_entity = entity or self.pipeline.detect_entity_action(Path(filepath).name)[0]
                cfg_action = action or self.pipeline.detect_entity_action(Path(filepath).name)[1]

                self._log(f"  ✅ Готово! Кадров: {config['frames']}")
                self._log(f"  📁 {self.pipeline.output_dir / config['sheet']}")
                self.frame.after(0, self._refresh_list)
                self.frame.after(0, lambda e=cfg_entity, a=cfg_action: self._show_preview(e, a))

            except Exception as ex:
                error_msg = str(ex)
                self._log(f"  ❌ Ошибка: {error_msg}")
                self.frame.after(0, lambda msg=error_msg: self._set_progress(0, f"Ошибка: {msg}"))

        

    def _log(self, msg):
        def do():
            self.log_text.insert("end", msg + "\n")
            self.log_text.see("end")
        try: self.frame.after(0, do)
        except: pass

    def _set_progress(self, pct, msg):
        try:
            self.progressbar["value"] = pct
            self.prog_var.set(msg)
        except: pass

    def _refresh_list(self):
        for item in self.anim_tree.get_children():
            self.anim_tree.delete(item)
        config = self.pipeline.load_animations_config()
        for entity, actions in sorted(config.items()):
            for action, info in sorted(actions.items()):
                self.anim_tree.insert("", "end", values=(
                    entity, action,
                    info.get("frames",1),
                    f"{info.get('width',64)}×{info.get('height',64)}",
                    info.get("fps",12),
                    info.get("sheet","?"),
                ))

    def _on_anim_select(self, e):
        sel = self.anim_tree.selection()
        if not sel: return
        vals = self.anim_tree.item(sel[0], "values")
        if vals:
            entity, action = vals[0], vals[1]
            self._show_preview(entity, action)

    def _show_preview(self, entity, action):
        img = self.pipeline.get_preview_image(entity, action, 400)
        if img:
            self._preview_img = img
            self.preview_canvas.delete("all")
            cw = self.preview_canvas.winfo_width() or 400
            self.preview_canvas.create_image(cw//2, 100, image=img, anchor="center")
            config = self.pipeline.load_animations_config()
            info = config.get(entity,{}).get(action,{})
            self.preview_info.config(
                text=f"{entity} / {action}  |  {info.get('frames',0)} кадров  |  "
                     f"{info.get('width',0)}×{info.get('height',0)}  |  {info.get('fps',0)} FPS",
                fg=C["cyan"]
            )

    def _delete_anim(self):
        sel = self.anim_tree.selection()
        if not sel: return
        vals = self.anim_tree.item(sel[0], "values")
        entity, action = vals[0], vals[1]
        if not messagebox.askyesno("Удалить",f"Удалить {entity}/{action}?"): return
        config = self.pipeline.load_animations_config()
        if entity in config and action in config[entity]:
            sheet = config[entity][action].get("sheet","")
            del config[entity][action]
            if not config[entity]: del config[entity]
            self.pipeline.save_animations_config(config)
            sp = self.pipeline.output_dir / sheet
            if sp.exists(): sp.unlink()
        self._refresh_list()

    def _copy_json(self):
        sel = self.anim_tree.selection()
        if not sel: return
        vals = self.anim_tree.item(sel[0], "values")
        entity, action = vals[0], vals[1]
        config = self.pipeline.load_animations_config()
        info = config.get(entity,{}).get(action,{})
        text = json.dumps({entity: {action: info}}, ensure_ascii=False, indent=2)
        self.frame.winfo_toplevel().clipboard_clear()
        self.frame.winfo_toplevel().clipboard_append(text)

    def _open_sprites_dir(self):
        path = str(self.pipeline.output_dir)
        if sys.platform == "win32": os.startfile(path)
        elif sys.platform == "darwin": subprocess.run(["open", path])
        else: subprocess.run(["xdg-open", path])


# ══════════════════════════════════════════════════════════════
# ТАБ 3: РЕДАКТОР КВЕСТОВ
# ══════════════════════════════════════════════════════════════
class QuestEditorTab:
    def __init__(self, notebook, project_root: Path):
        self.project_root = project_root
        self.quests_path = project_root / "assets" / "quests.json"
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="⚔ Квесты")
        self.quests = make_default_quests()
        self._load_quests()
        self.selected_quest = None
        self._build_ui()
        self._refresh_list()

    def _load_quests(self):
        if self.quests_path.exists():
            try: self.quests = json.loads(self.quests_path.read_text(encoding="utf-8"))
            except: pass

    def _save_quests(self):
        self.quests_path.parent.mkdir(parents=True, exist_ok=True)
        self.quests_path.write_text(json.dumps(self.quests, ensure_ascii=False, indent=2), encoding="utf-8")

    def _build_ui(self):
        # Заголовок
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=10)
        hdr.pack(fill="x")
        tk.Label(hdr, text="⚔ РЕДАКТОР КВЕСТОВ", bg=C["panel"], fg=C["gold2"],
                font=("Segoe UI",14,"bold"), padx=20).pack(side="left")
        btn(hdr,"+ Новый квест",self._new_quest,C["green"],"black",("Segoe UI",10,"bold"),padx=10,pady=4).pack(side="right",padx=12)
        btn(hdr,"💾 Сохранить всё",self._save_all,C["accent"],padx=10,pady=4).pack(side="right",padx=4)

        # Split
        main = tk.Frame(self.frame, bg=C["bg"])
        main.pack(fill="both", expand=True)

        # Список квестов (слева)
        left = tk.Frame(main, bg=C["panel"], width=240)
        left.pack(side="left", fill="y"); left.pack_propagate(False)

        lbl(left,"📋 КВЕСТЫ",C["gold"],("Segoe UI",9,"bold"),C["panel"]).pack(fill="x",padx=8,pady=(8,4))

        # Поиск
        search_var = tk.StringVar()
        search_entry = tk.Entry(left, textvariable=search_var, bg=C["input_bg"],
                               fg=C["text"], insertbackground=C["gold"],
                               relief="flat", font=("Segoe UI",9))
        search_entry.pack(fill="x", padx=8, pady=(0,4))
        tk.Label(left, text="🔍 Поиск", bg=C["panel"], fg=C["muted"],
                font=("Segoe UI",8)).pack(anchor="w", padx=8)
        search_var.trace_add("write", lambda *a: self._filter_list(search_var.get()))

        # Список
        self.quest_lb = tk.Listbox(left, bg=C["panel2"], fg=C["text"],
                                   selectbackground=C["accent"], selectforeground="white",
                                   font=("Segoe UI",9), relief="flat",
                                   activestyle="none", bd=0)
        sb = ttk.Scrollbar(left, orient="vertical", command=self.quest_lb.yview,
                           style="Dark.Vertical.TScrollbar")
        self.quest_lb.configure(yscrollcommand=sb.set)
        lb_fr = tk.Frame(left, bg=C["panel"]); lb_fr.pack(fill="both", expand=True, padx=4)
        sb.pack(in_=lb_fr, side="right", fill="y")
        self.quest_lb.pack(in_=lb_fr, side="left", fill="both", expand=True)
        self.quest_lb.bind("<<ListboxSelect>>", self._on_quest_select)

        # Кнопки списка
        lb_btns = tk.Frame(left, bg=C["panel"]); lb_btns.pack(fill="x", padx=4, pady=4)
        btn(lb_btns,"📋 Копировать",self._duplicate_quest,padx=4,pady=3).pack(side="left",padx=2)
        btn(lb_btns,"🗑 Удалить",self._delete_quest,C["danger"],padx=4,pady=3).pack(side="right",padx=2)

        # Правая панель — редактор квеста
        right = tk.Frame(main, bg=C["bg"])
        right.pack(side="left", fill="both", expand=True, padx=8, pady=8)

        # Notebook внутри правой панели
        self.quest_nb = ttk.Notebook(right, style="Dark.TNotebook")
        self.quest_nb.pack(fill="both", expand=True)

        self._build_properties_tab()
        self._build_objectives_tab()
        self._build_rewards_tab()
        self._build_dialogue_tab()

    def _build_properties_tab(self):
        tab = tk.Frame(self.quest_nb, bg=C["bg"])
        self.quest_nb.add(tab, text="📝 Свойства")

        scroll_canvas = tk.Canvas(tab, bg=C["bg"], highlightthickness=0)
        sb = ttk.Scrollbar(tab, orient="vertical", command=scroll_canvas.yview,
                           style="Dark.Vertical.TScrollbar")
        scroll_canvas.configure(yscrollcommand=sb.set)
        sb.pack(side="right", fill="y")
        scroll_canvas.pack(side="left", fill="both", expand=True)
        inner = tk.Frame(scroll_canvas, bg=C["bg"])
        scroll_canvas.create_window((0,0), window=inner, anchor="nw")
        inner.bind("<Configure>", lambda e: scroll_canvas.configure(scrollregion=scroll_canvas.bbox("all")))

        def field(label, var_name, default="", multiline=False, choices=None, color_indicator=False):
            fr = tk.Frame(inner, bg=C["panel"], pady=6)
            fr.pack(fill="x", padx=12, pady=3)
            tk.Label(fr, text=label, bg=C["panel"], fg=C["muted"],
                    font=("Segoe UI",8,"bold"), padx=12, pady=4, width=16, anchor="w").pack(anchor="w")
            if multiline:
                txt_fr, txt = scrolled_text(fr, height=4)
                txt_fr.pack(fill="x", padx=12, pady=(0,6))
                setattr(self, var_name, txt)
            elif choices:
                var = tk.StringVar(value=default)
                setattr(self, var_name+"_var", var)
                cb = ttk.Combobox(fr, textvariable=var, values=choices, state="readonly",
                                 style="Dark.TCombobox", font=("Segoe UI",9))
                cb.pack(fill="x", padx=12, pady=(0,6))
                cb.bind("<<ComboboxSelected>>", lambda e: self._mark_modified())
                setattr(self, var_name, cb)
            else:
                var = tk.StringVar(value=default)
                setattr(self, var_name+"_var", var)
                e = tk.Entry(fr, textvariable=var, bg=C["input_bg"], fg=C["text"],
                           insertbackground=C["gold"], relief="flat", font=("Segoe UI",9))
                e.pack(fill="x", padx=12, pady=(0,6))
                var.trace_add("write", lambda *a: self._mark_modified())
                setattr(self, var_name+"_entry", e)
            return fr

        field("ID квеста:", "q_id", "q001")
        field("Название:", "q_name", "Новый квест")
        field("Описание:", "q_desc", "", multiline=True)
        field("Уровень:", "q_level", "1")
        field("Тип:", "q_type", "kill", choices=list(QUEST_TYPES.keys()))
        field("Квестодатель НПС:", "q_giver", "")
        field("Активен:", "q_active", "да", choices=["да","нет"])

        # Тип визуально
        type_preview = tk.Frame(inner, bg=C["bg"]); type_preview.pack(fill="x", padx=12, pady=4)
        for qt, info in QUEST_TYPES.items():
            fr = tk.Frame(type_preview, bg=C["panel3"])
            fr.pack(side="left", padx=3, pady=2)
            tk.Label(fr, text=f"{info['icon']} {info['name']}", bg=C["panel3"],
                    fg=info["color"], font=("Segoe UI",8,"bold"), padx=6, pady=4).pack()

    def _build_objectives_tab(self):
        tab = tk.Frame(self.quest_nb, bg=C["bg"])
        self.quest_nb.add(tab, text="🎯 Цели")

        top = tk.Frame(tab, bg=C["bg"]); top.pack(fill="x", padx=8, pady=8)
        btn(top,"+ Добавить цель",self._add_objective,C["green"],"black",padx=8,pady=4).pack(side="left",padx=4)
        btn(top,"🗑 Удалить",self._del_objective,C["danger"],padx=8,pady=4).pack(side="left",padx=4)

        cols = ("type","target","count","desc")
        self.obj_tree = ttk.Treeview(tab, columns=cols, show="headings",
                                    style="Dark.Treeview", height=8)
        for col,text,w in [("type","Тип",80),("target","Цель",150),("count","Количество",80),("desc","Описание",250)]:
            self.obj_tree.heading(col,text=text)
            self.obj_tree.column(col,width=w,anchor="center")
        sb = ttk.Scrollbar(tab, orient="vertical", command=self.obj_tree.yview,
                          style="Dark.Vertical.TScrollbar")
        self.obj_tree.configure(yscrollcommand=sb.set)
        tr_fr = tk.Frame(tab, bg=C["bg"]); tr_fr.pack(fill="both", expand=False, padx=8)
        self.obj_tree.pack(in_=tr_fr, side="left", fill="both", expand=True)
        sb.pack(in_=tr_fr, side="right", fill="y")

        # Форма добавления
        add_fr = tk.Frame(tab, bg=C["panel"], bd=1); add_fr.pack(fill="x", padx=8, pady=8)
        tk.Label(add_fr, text="➕ Новая цель", bg=C["panel"], fg=C["gold"],
                font=("Segoe UI",9,"bold"), padx=12, pady=6).pack(anchor="w")

        form = tk.Frame(add_fr, bg=C["panel"]); form.pack(fill="x", padx=12, pady=(0,8))

        tk.Label(form, text="Тип:", bg=C["panel"], fg=C["text"],
                font=("Segoe UI",9)).grid(row=0,column=0,padx=4,pady=3,sticky="w")
        self.new_obj_type = tk.StringVar(value="kill")
        ttk.Combobox(form, textvariable=self.new_obj_type,
                    values=["kill","collect","escort","explore","deliver","talk"],
                    style="Dark.TCombobox", width=12).grid(row=0,column=1,padx=4,pady=3)

        tk.Label(form, text="Цель:", bg=C["panel"], fg=C["text"],
                font=("Segoe UI",9)).grid(row=0,column=2,padx=4,pady=3,sticky="w")
        self.new_obj_target = tk.StringVar(value="GOBLIN")
        all_targets = list(ENEMIES.keys()) + list(ITEM_TYPES["material"]) + list(ITEM_TYPES["key"])
        ttk.Combobox(form, textvariable=self.new_obj_target,
                    values=all_targets,
                    style="Dark.TCombobox", width=18).grid(row=0,column=3,padx=4,pady=3)

        tk.Label(form, text="Кол-во:", bg=C["panel"], fg=C["text"],
                font=("Segoe UI",9)).grid(row=0,column=4,padx=4,pady=3,sticky="w")
        self.new_obj_count = tk.StringVar(value="5")
        tk.Entry(form, textvariable=self.new_obj_count, bg=C["input_bg"],
                fg=C["text"], insertbackground=C["gold"], relief="flat",
                width=6, font=("Segoe UI",9)).grid(row=0,column=5,padx=4,pady=3)

        btn(form,"Добавить",self._confirm_add_objective,C["green"],"black",
            padx=6,pady=3).grid(row=0,column=6,padx=8,pady=3)

    def _build_rewards_tab(self):
        tab = tk.Frame(self.quest_nb, bg=C["bg"])
        self.quest_nb.add(tab, text="🏆 Награда")

        # Золото и XP
        top = tk.Frame(tab, bg=C["panel"]); top.pack(fill="x", padx=8, pady=8)
        tk.Label(top, text="💰 ЗОЛОТО И ОПЫТ", bg=C["panel"], fg=C["gold"],
                font=("Segoe UI",10,"bold"), padx=12, pady=6).pack(anchor="w")

        gold_fr = tk.Frame(top, bg=C["panel"]); gold_fr.pack(fill="x", padx=12)

        def num_field(parent, label, var_name, default=0):
            fr2 = tk.Frame(parent, bg=C["panel"]); fr2.pack(side="left", padx=8)
            tk.Label(fr2, text=label, bg=C["panel"], fg=C["muted"],
                    font=("Segoe UI",8)).pack()
            var = tk.IntVar(value=default)
            setattr(self, var_name, var)
            tk.Spinbox(fr2, textvariable=var, from_=0, to=99999,
                      bg=C["input_bg"], fg=C["gold"], insertbackground=C["gold"],
                      relief="flat", font=("Segoe UI",11,"bold"),
                      width=8, bd=0).pack()

        num_field(gold_fr, "Золото (мин.)", "rew_gold_min", 50)
        num_field(gold_fr, "Золото (макс.)", "rew_gold_max", 100)
        num_field(gold_fr, "Опыт (XP)", "rew_xp", 200)

        # Предметы
        item_fr = tk.Frame(tab, bg=C["panel"]); item_fr.pack(fill="x", padx=8, pady=4)
        tk.Label(item_fr, text="🎒 ПРЕДМЕТЫ НАГРАДЫ", bg=C["panel"], fg=C["gold"],
                font=("Segoe UI",10,"bold"), padx=12, pady=6).pack(anchor="w")

        item_ctrl = tk.Frame(item_fr, bg=C["panel"]); item_ctrl.pack(fill="x", padx=12)
        self.rew_item_var = tk.StringVar()
        all_items = []
        for lst in ITEM_TYPES.values(): all_items.extend(lst)
        ttk.Combobox(item_ctrl, textvariable=self.rew_item_var, values=all_items,
                    style="Dark.TCombobox", width=30, font=("Segoe UI",9)).pack(side="left",padx=4)
        btn(item_ctrl,"+ Добавить",self._add_reward_item,C["green"],"black",padx=6,pady=3).pack(side="left",padx=4)
        btn(item_ctrl,"🗑 Удалить",self._del_reward_item,C["danger"],padx=6,pady=3).pack(side="left",padx=2)

        self.rew_items_lb = tk.Listbox(item_fr, bg=C["panel2"], fg=C["text"],
                                      selectbackground=C["accent"],
                                      font=("Segoe UI",9), height=6, relief="flat")
        self.rew_items_lb.pack(fill="x", padx=12, pady=6)

        # Дополнительно
        extra = tk.Frame(tab, bg=C["panel"]); extra.pack(fill="x", padx=8, pady=4)
        tk.Label(extra, text="⚡ СПЕЦИАЛЬНЫЕ НАГРАДЫ", bg=C["panel"], fg=C["gold"],
                font=("Segoe UI",10,"bold"), padx=12, pady=6).pack(anchor="w")
        self.rew_special = tk.StringVar()
        tk.Entry(extra, textvariable=self.rew_special, bg=C["input_bg"], fg=C["text"],
                insertbackground=C["gold"], relief="flat", font=("Segoe UI",9),
                ).pack(fill="x", padx=12, pady=(0,8))
        tk.Label(extra, text="Например: unlock_area:dungeon_1, give_title:Герой, unlock_skill:fireball",
                bg=C["panel"], fg=C["muted"], font=("Segoe UI",8), padx=12).pack(anchor="w")

    def _build_dialogue_tab(self):
        tab = tk.Frame(self.quest_nb, bg=C["bg"])
        self.quest_nb.add(tab, text="💬 Диалог")

        hdr2 = tk.Label(tab, text="Диалог квестодателя — 3 фазы: Предложение / В процессе / Завершение",
                       bg=C["bg"], fg=C["muted"], font=("Segoe UI",9), pady=8)
        hdr2.pack(fill="x", padx=12)

        phases = [
            ("offer",    "📜 ПРЕДЛОЖЕНИЕ КВЕСТА",   C["blue"]),
            ("progress", "⏳ В ПРОЦЕССЕ",            C["orange"]),
            ("complete", "✅ ЗАВЕРШЕНИЕ",             C["green"]),
        ]
        self.dlg_widgets = {}
        for phase_id, phase_name, color in phases:
            fr = tk.Frame(tab, bg=C["panel"]); fr.pack(fill="x", padx=8, pady=4)
            tk.Label(fr, text=phase_name, bg=C["panel"], fg=color,
                    font=("Segoe UI",9,"bold"), padx=12, pady=6).pack(anchor="w")
            # Список реплик
            inner = tk.Frame(fr, bg=C["panel"]); inner.pack(fill="x", padx=12)
            lb = tk.Listbox(inner, bg=C["panel2"], fg=C["text"],
                           selectbackground=C["accent"], height=3,
                           font=("Segoe UI",9), relief="flat")
            ctrl = tk.Frame(inner, bg=C["panel"]); ctrl.pack(side="right", fill="y", padx=(4,0))

            btn(ctrl,"+ Добавить",lambda p=phase_id: self._add_dlg_line(p),
               C["panel3"],C["cyan"],("Segoe UI",8),padx=4,pady=2).pack(fill="x",pady=1)
            btn(ctrl,"✏ Изменить",lambda p=phase_id: self._edit_dlg_line(p),
               C["panel3"],C["text"],("Segoe UI",8),padx=4,pady=2).pack(fill="x",pady=1)
            btn(ctrl,"🗑 Удалить",lambda p=phase_id: self._del_dlg_line(p),
               C["panel3"],C["red"],("Segoe UI",8),padx=4,pady=2).pack(fill="x",pady=1)
            lb.pack(side="left", fill="both", expand=True)
            self.dlg_widgets[phase_id] = lb

    # ── Quest CRUD ────────────────────────────────────────────────
    def _new_quest(self):
        qid = f"q{len(self.quests)+1:03d}"
        q = {
            "id": qid, "name": f"Новый квест {qid}", "level_req": 1,
            "type": "kill", "giver_npc": "",
            "description": "Описание квеста...",
            "objectives": [], "rewards": {"gold_min":10,"gold_max":50,"xp":100,"items":[]},
            "dialogue": {"offer":["..."],"progress":["..."],"complete":["Отлично!"]},
            "active": True
        }
        self.quests.append(q)
        self._refresh_list()
        self.quest_lb.selection_clear(0,"end")
        self.quest_lb.selection_set("end")
        self._on_quest_select(None)

    def _duplicate_quest(self):
        idx = self._selected_idx()
        if idx is None: return
        q = copy.deepcopy(self.quests[idx])
        q["id"] += "_copy"; q["name"] += " (копия)"
        self.quests.insert(idx+1, q)
        self._refresh_list()

    def _delete_quest(self):
        idx = self._selected_idx()
        if idx is None: return
        if messagebox.askyesno("Удалить",f"Удалить квест '{self.quests[idx]['name']}'?"):
            self.quests.pop(idx); self._refresh_list(); self.selected_quest=None

    def _selected_idx(self):
        sel = self.quest_lb.curselection()
        return sel[0] if sel else None

    def _filter_list(self, query):
        self._refresh_list(query)

    def _refresh_list(self, filter_text=""):
        self.quest_lb.delete(0,"end")
        for q in self.quests:
            if filter_text.lower() in q["name"].lower() or filter_text.lower() in q["id"].lower():
                qt = QUEST_TYPES.get(q.get("type","kill"),{})
                icon = qt.get("icon","⚔")
                active = "✅" if q.get("active",True) else "⭕"
                self.quest_lb.insert("end", f"{active} {icon} [{q['id']}] {q['name']}")

    def _on_quest_select(self, e):
        idx = self._selected_idx()
        if idx is None: return
        # Находим квест по позиции в отфильтрованном списке
        visible = [q for q in self.quests]
        if idx >= len(visible): return
        q = visible[idx]
        self.selected_quest = q
        self._load_quest_into_form(q)

    def _load_quest_into_form(self, q):
        # Properties
        def set_var(name, val):
            try:
                var = getattr(self, name+"_var", None)
                if var: var.set(val)
                widget = getattr(self, name, None)
                if hasattr(widget,"delete"):  # Text widget
                    widget.delete("1.0","end")
                    widget.insert("1.0", val)
            except: pass

        set_var("q_id", q.get("id",""))
        set_var("q_name", q.get("name",""))
        set_var("q_type", q.get("type","kill"))
        set_var("q_level", str(q.get("level_req",1)))
        set_var("q_giver", q.get("giver_npc",""))
        set_var("q_active", "да" if q.get("active",True) else "нет")

        desc_w = getattr(self, "q_desc", None)
        if desc_w:
            desc_w.delete("1.0","end")
            desc_w.insert("1.0", q.get("description",""))

        # Objectives
        for item in self.obj_tree.get_children():
            self.obj_tree.delete(item)
        for obj in q.get("objectives",[]):
            self.obj_tree.insert("", "end", values=(
                obj.get("type","kill"), obj.get("target",""),
                obj.get("count",1), f"{obj.get('type','kill')} {obj.get('target','')} ×{obj.get('count',1)}"
            ))

        # Rewards
        rew = q.get("rewards",{})
        try: self.rew_gold_min.set(rew.get("gold_min",0))
        except: pass
        try: self.rew_gold_max.set(rew.get("gold_max",0))
        except: pass
        try: self.rew_xp.set(rew.get("xp",0))
        except: pass
        try:
            self.rew_items_lb.delete(0,"end")
            for item in rew.get("items",[]):
                self.rew_items_lb.insert("end", item)
        except: pass

        # Dialogue
        dlg = q.get("dialogue",{})
        for phase, lb in self.dlg_widgets.items():
            lb.delete(0,"end")
            for line in dlg.get(phase,[]):
                lb.insert("end", line)

    def _mark_modified(self):
        if not self.selected_quest: return
        # Синхронизируем из формы в выбранный квест
        q = self.selected_quest
        try: q["id"] = self.q_id_var.get()
        except: pass
        try: q["name"] = self.q_name_var.get()
        except: pass
        try: q["type"] = self.q_type_var.get()
        except: pass
        try: q["level_req"] = int(self.q_level_var.get() or 1)
        except: pass
        try: q["giver_npc"] = self.q_giver_var.get()
        except: pass
        try: q["active"] = self.q_active_var.get() == "да"
        except: pass
        try: q["description"] = self.q_desc.get("1.0","end").strip()
        except: pass

    def _save_all(self):
        if self.selected_quest: self._mark_modified()
        self._save_quests()
        messagebox.showinfo("Сохранено",f"✅ {len(self.quests)} квестов сохранено в:\n{self.quests_path}")

    # ── Objectives ────────────────────────────────────────────────
    def _add_objective(self):
        if not self.selected_quest: return
        self._confirm_add_objective()

    def _confirm_add_objective(self):
        if not self.selected_quest: return
        obj_type = self.new_obj_type.get()
        target   = self.new_obj_target.get()
        try: count = int(self.new_obj_count.get())
        except: count = 1
        obj = {"type":obj_type,"target":target,"count":count,"current":0}
        self.selected_quest.setdefault("objectives",[]).append(obj)
        self.obj_tree.insert("","end", values=(obj_type,target,count,f"{obj_type} {target} ×{count}"))

    def _del_objective(self):
        sel = self.obj_tree.selection()
        if not sel or not self.selected_quest: return
        idx = self.obj_tree.index(sel[0])
        if idx < len(self.selected_quest.get("objectives",[])):
            self.selected_quest["objectives"].pop(idx)
        self.obj_tree.delete(sel[0])

    # ── Rewards ───────────────────────────────────────────────────
    def _add_reward_item(self):
        if not self.selected_quest: return
        item = self.rew_item_var.get().strip()
        if not item: return
        self.selected_quest.setdefault("rewards",{}).setdefault("items",[]).append(item)
        self.rew_items_lb.insert("end", item)

    def _del_reward_item(self):
        sel = self.rew_items_lb.curselection()
        if not sel or not self.selected_quest: return
        self.rew_items_lb.delete(sel[0])
        items = self.selected_quest.get("rewards",{}).get("items",[])
        if sel[0] < len(items): items.pop(sel[0])

    # ── Dialogue ──────────────────────────────────────────────────
    def _add_dlg_line(self, phase):
        if not self.selected_quest: return
        line = simpledialog.askstring("Реплика", f"Введи реплику для фазы «{phase}»:",
                                      parent=self.frame.winfo_toplevel())
        if line:
            self.selected_quest.setdefault("dialogue",{}).setdefault(phase,[]).append(line)
            self.dlg_widgets[phase].insert("end", line)

    def _edit_dlg_line(self, phase):
        if not self.selected_quest: return
        lb = self.dlg_widgets[phase]
        sel = lb.curselection()
        if not sel: return
        old = lb.get(sel[0])
        new = simpledialog.askstring("Изменить реплику", "Реплика:", initialvalue=old,
                                     parent=self.frame.winfo_toplevel())
        if new:
            lb.delete(sel[0]); lb.insert(sel[0], new)
            lines = self.selected_quest.get("dialogue",{}).get(phase,[])
            if sel[0] < len(lines): lines[sel[0]] = new

    def _del_dlg_line(self, phase):
        if not self.selected_quest: return
        lb = self.dlg_widgets[phase]
        sel = lb.curselection()
        if not sel: return
        lb.delete(sel[0])
        lines = self.selected_quest.get("dialogue",{}).get(phase,[])
        if sel[0] < len(lines): lines.pop(sel[0])


# ══════════════════════════════════════════════════════════════
# ТАБ 4: ДИАЛОГИ НПС И МОБОВ
# ══════════════════════════════════════════════════════════════
class DialogueEditorTab:
    def __init__(self, notebook, project_root: Path):
        self.project_root = project_root
        self.dlg_path = project_root / "assets" / "dialogues.json"
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="💬 Диалоги")
        self.dialogues = make_default_dialogues()
        self._load_dialogues()
        self.selected_entity_key = None
        self._build_ui()
        self._refresh_entity_list()

    def _load_dialogues(self):
        if self.dlg_path.exists():
            try: self.dialogues = json.loads(self.dlg_path.read_text(encoding="utf-8"))
            except: pass

    def _save_dialogues(self):
        self.dlg_path.parent.mkdir(parents=True, exist_ok=True)
        self.dlg_path.write_text(json.dumps(self.dialogues, ensure_ascii=False, indent=2), encoding="utf-8")

    def _build_ui(self):
        # Заголовок
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=10)
        hdr.pack(fill="x")
        tk.Label(hdr, text="💬 РЕДАКТОР ДИАЛОГОВ", bg=C["panel"], fg=C["gold2"],
                font=("Segoe UI",14,"bold"), padx=20).pack(side="left")
        tk.Label(hdr, text="Назначай реплики мобам и НПС, управляй каждой фразой",
                bg=C["panel"], fg=C["muted"], font=("Segoe UI",9), padx=8).pack(side="left")
        btn(hdr,"💾 Сохранить",self._save_all,C["accent"],padx=10,pady=4).pack(side="right",padx=8)
        btn(hdr,"📤 Экспорт JSON",self._export_json,padx=10,pady=4).pack(side="right",padx=4)
        btn(hdr,"+ Добавить НПС/Моба",self._add_entity,C["green"],"black",padx=10,pady=4).pack(side="right",padx=4)

        # Split
        main = tk.Frame(self.frame, bg=C["bg"])
        main.pack(fill="both", expand=True)

        # Левый список
        left = tk.Frame(main, bg=C["panel"], width=220)
        left.pack(side="left", fill="y"); left.pack_propagate(False)
        self._build_left(left)

        # Правая панель
        self.right_fr = tk.Frame(main, bg=C["bg"])
        self.right_fr.pack(side="left", fill="both", expand=True, padx=8, pady=8)
        self._build_empty_right()

    def _build_left(self, p):
        lbl(p,"🎭 ПЕРСОНАЖИ",C["gold"],("Segoe UI",9,"bold"),C["panel"]).pack(fill="x",padx=8,pady=(8,4))

        # Фильтр по категории
        filter_fr = tk.Frame(p, bg=C["panel"]); filter_fr.pack(fill="x", padx=4, pady=2)
        self.cat_var = tk.StringVar(value="Все")
        ttk.Combobox(filter_fr, textvariable=self.cat_var,
                    values=["Все","Враги","НПС"],
                    state="readonly", style="Dark.TCombobox",
                    font=("Segoe UI",9)).pack(fill="x")
        self.cat_var.trace_add("write", lambda *a: self._refresh_entity_list())

        # Список
        self.entity_lb = tk.Listbox(p, bg=C["panel2"], fg=C["text"],
                                   selectbackground=C["accent"], selectforeground="white",
                                   font=("Segoe UI",9), relief="flat",
                                   activestyle="none", bd=0)
        sb = ttk.Scrollbar(p, orient="vertical", command=self.entity_lb.yview,
                           style="Dark.Vertical.TScrollbar")
        self.entity_lb.configure(yscrollcommand=sb.set)
        lb_fr = tk.Frame(p, bg=C["panel"]); lb_fr.pack(fill="both", expand=True, padx=4)
        sb.pack(in_=lb_fr, side="right", fill="y")
        self.entity_lb.pack(in_=lb_fr, side="left", fill="both", expand=True)
        self.entity_lb.bind("<<ListboxSelect>>", self._on_entity_select)

        sep(p).pack(fill="x", padx=8, pady=4)
        btn(p,"🗑 Удалить",self._del_entity,C["danger"],padx=6,pady=3).pack(fill="x",padx=8,pady=4)

    def _build_empty_right(self):
        for w in self.right_fr.winfo_children(): w.destroy()
        tk.Label(self.right_fr, text="← Выбери персонажа слева\nдля редактирования его реплик",
                bg=C["bg"], fg=C["muted"], font=("Segoe UI",11),
                justify="center").pack(expand=True)

    def _build_entity_editor(self, entity_key):
        for w in self.right_fr.winfo_children(): w.destroy()

        # Заголовок сущности
        is_enemy = entity_key in ENEMIES
        is_npc = entity_key in NPCS
        info = ENEMIES.get(entity_key) or NPCS.get(entity_key) or {}
        emoji = info.get("emoji","?")
        color = info.get("color", C["text"])

        hdr2 = tk.Frame(self.right_fr, bg=C["panel2"]); hdr2.pack(fill="x", pady=(0,8))
        tk.Label(hdr2, text=f"{emoji}  {entity_key}", bg=C["panel2"],
                fg=color, font=("Segoe UI",14,"bold"), padx=16, pady=8).pack(side="left")
        tag = ("⚔ Враг" if is_enemy else "👤 НПС" if is_npc else "👤 Персонаж")
        tk.Label(hdr2, text=tag, bg=C["panel2"], fg=C["muted"],
                font=("Segoe UI",9), padx=8).pack(side="left")

        # Определяем фазы диалога для данного типа
        if is_enemy:
            phases = [
                ("aggro",  "⚡ АГРЕССИЯ",   C["red"]),
                ("combat", "⚔ В БОЮ",      C["orange"]),
                ("death",  "💀 СМЕРТЬ",     C["muted"]),
                ("idle",   "😴 ПАССИВНЫЙ",  C["text"]),
            ]
        else:
            phases = [
                ("greeting","👋 ПРИВЕТСТВИЕ", C["green"]),
                ("idle",    "😊 ПАССИВНЫЙ",   C["text"]),
                ("trade",   "🛒 ТОРГОВЛЯ",    C["gold"]),
                ("quest",   "❗ КВЕСТ",       C["blue"]),
                ("warning", "⚠ ПРЕДУПРЕЖДЕНИЕ", C["orange"]),
                ("farewell","👋 ПРОЩАНИЕ",   C["muted"]),
                ("complete","✅ ЗАВЕРШЕНИЕ",  C["green"]),
            ]

        # Скролл для всех фаз
        scroll_canvas = tk.Canvas(self.right_fr, bg=C["bg"], highlightthickness=0)
        sb2 = ttk.Scrollbar(self.right_fr, orient="vertical", command=scroll_canvas.yview,
                           style="Dark.Vertical.TScrollbar")
        scroll_canvas.configure(yscrollcommand=sb2.set)
        sb2.pack(side="right", fill="y")
        scroll_canvas.pack(side="left", fill="both", expand=True)
        inner = tk.Frame(scroll_canvas, bg=C["bg"])
        scroll_canvas.create_window((0,0), window=inner, anchor="nw")
        inner.bind("<Configure>", lambda e: scroll_canvas.configure(scrollregion=scroll_canvas.bbox("all")))

        self._phase_widgets = {}
        entity_dlg = self.dialogues.get(entity_key, {})

        for phase_id, phase_name, color2 in phases:
            fr = tk.Frame(inner, bg=C["panel"], bd=0); fr.pack(fill="x", padx=8, pady=4)

            # Заголовок фазы
            ph_hdr = tk.Frame(fr, bg=C["panel3"]); ph_hdr.pack(fill="x")
            tk.Label(ph_hdr, text=phase_name, bg=C["panel3"], fg=color2,
                    font=("Segoe UI",9,"bold"), padx=12, pady=5).pack(side="left")
            lines = entity_dlg.get(phase_id, [])
            count_lbl = tk.Label(ph_hdr, text=f"({len(lines)} реплик)",
                                bg=C["panel3"], fg=C["muted"], font=("Segoe UI",8), padx=4)
            count_lbl.pack(side="left")

            # Кнопки фазы
            btns_fr = tk.Frame(ph_hdr, bg=C["panel3"]); btns_fr.pack(side="right", padx=4)
            btn(btns_fr,"+ Добавить",lambda p=phase_id: self._add_line(entity_key, p),
               C["panel3"],C["cyan"],("Segoe UI",8),padx=4,pady=2).pack(side="left",padx=2)

            # Список реплик
            lb_fr2 = tk.Frame(fr, bg=C["panel"]); lb_fr2.pack(fill="x", padx=8, pady=4)
            lb = tk.Listbox(lb_fr2, bg=C["panel2"], fg=C["text"],
                           selectbackground=C["accent"], height=max(2,min(4,len(lines)+1)),
                           font=("Segoe UI",9), relief="flat", activestyle="none")
            lb_ctrl = tk.Frame(lb_fr2, bg=C["panel"]); lb_ctrl.pack(side="right", fill="y", padx=(4,0))
            btn(lb_ctrl,"✏",lambda p=phase_id: self._edit_line(entity_key,p),
               C["panel3"],C["gold"],("Segoe UI",9),padx=4,pady=2).pack(fill="x",pady=1)
            btn(lb_ctrl,"🗑",lambda p=phase_id: self._del_line(entity_key,p),
               C["panel3"],C["red"],("Segoe UI",9),padx=4,pady=2).pack(fill="x",pady=1)
            btn(lb_ctrl,"▲",lambda p=phase_id: self._move_line(entity_key,p,-1),
               C["panel3"],C["muted"],("Segoe UI",9),padx=4,pady=1).pack(fill="x",pady=1)
            btn(lb_ctrl,"▼",lambda p=phase_id: self._move_line(entity_key,p,1),
               C["panel3"],C["muted"],("Segoe UI",9),padx=4,pady=1).pack(fill="x",pady=1)
            lb.pack(side="left", fill="both", expand=True)

            for line in lines:
                lb.insert("end", line)

            self._phase_widgets[phase_id] = (lb, count_lbl)

        inner.bind("<MouseWheel>", lambda e: scroll_canvas.yview_scroll(-1*(e.delta//120),"units"))

    # ── Entity CRUD ───────────────────────────────────────────────
    def _refresh_entity_list(self, *a):
        self.entity_lb.delete(0,"end")
        cat = self.cat_var.get()
        all_entities = {}
        # Встроенные
        for k in ENEMIES: all_entities[k] = ("enemy", ENEMIES[k].get("emoji","👹"))
        for k in NPCS:    all_entities[k] = ("npc",   NPCS[k].get("emoji","👤"))
        # Кастомные из dialogues.json
        for k in self.dialogues:
            if k not in all_entities: all_entities[k] = ("custom","🎭")

        for key, (kind, emoji) in sorted(all_entities.items()):
            show = (cat == "Все" or
                   (cat == "Враги" and kind == "enemy") or
                   (cat == "НПС" and kind != "enemy"))
            if show:
                has_dlg = key in self.dialogues
                marker = "●" if has_dlg else "○"
                name = ENEMIES.get(key,{}).get("name") or NPCS.get(key,{}).get("name") or key
                self.entity_lb.insert("end", f"{marker} {emoji} {key} — {name}")

    def _on_entity_select(self, e):
        sel = self.entity_lb.curselection()
        if not sel: return
        text = self.entity_lb.get(sel[0])
        # Парсим ключ из "● 👺 GOBLIN — Гоблин"
        parts = text.split()
        key = parts[2] if len(parts) >= 3 else ""
        self.selected_entity_key = key
        if key not in self.dialogues:
            self.dialogues[key] = {}
        self._build_entity_editor(key)

    def _add_entity(self):
        key = simpledialog.askstring("Новый персонаж",
                                     "Введи ID персонажа (например: MY_BOSS, VILLAGE_ELDER):",
                                     parent=self.frame.winfo_toplevel())
        if not key: return
        key = key.upper().replace(" ","_")
        if key not in self.dialogues:
            self.dialogues[key] = {"greeting":["Привет!"],"idle":["..."],"farewell":["Пока!"]}
        self._refresh_entity_list()

    def _del_entity(self):
        key = self.selected_entity_key
        if not key: return
        if key in (list(ENEMIES.keys()) + list(NPCS.keys())):
            if not messagebox.askyesno("Удалить диалог",
                f"Удалить диалоги для '{key}'?\n(Персонаж из игры не удалится)"): return
        if key in self.dialogues: del self.dialogues[key]
        self.selected_entity_key = None
        self._refresh_entity_list()
        self._build_empty_right()

    # ── Line CRUD ─────────────────────────────────────────────────
    def _add_line(self, entity_key, phase):
        line = simpledialog.askstring(
            "Новая реплика",
            f"Реплика для {entity_key} [{phase}]:\n(Можно использовать {player} для имени игрока)",
            parent=self.frame.winfo_toplevel()
        )
        if not line: return
        self.dialogues.setdefault(entity_key,{}).setdefault(phase,[]).append(line)
        lb, clbl = self._phase_widgets.get(phase,(None,None))
        if lb: lb.insert("end",line)
        if clbl: clbl.config(text=f"({lb.size() if lb else 0} реплик)")

    def _edit_line(self, entity_key, phase):
        lb, clbl = self._phase_widgets.get(phase,(None,None))
        if not lb: return
        sel = lb.curselection()
        if not sel: return
        old = lb.get(sel[0])
        new = simpledialog.askstring("Изменить","Реплика:",initialvalue=old,
                                     parent=self.frame.winfo_toplevel())
        if new:
            lb.delete(sel[0]); lb.insert(sel[0],new)
            lines = self.dialogues.get(entity_key,{}).get(phase,[])
            if sel[0]<len(lines): lines[sel[0]]=new

    def _del_line(self, entity_key, phase):
        lb, clbl = self._phase_widgets.get(phase,(None,None))
        if not lb: return
        sel = lb.curselection()
        if not sel: return
        lb.delete(sel[0])
        lines = self.dialogues.get(entity_key,{}).get(phase,[])
        if sel[0]<len(lines): lines.pop(sel[0])
        if clbl: clbl.config(text=f"({lb.size()} реплик)")

    def _move_line(self, entity_key, phase, direction):
        lb, _ = self._phase_widgets.get(phase,(None,None))
        if not lb: return
        sel = lb.curselection()
        if not sel: return
        idx = sel[0]; new_idx = idx+direction
        lines = self.dialogues.get(entity_key,{}).get(phase,[])
        if new_idx<0 or new_idx>=len(lines): return
        lines[idx], lines[new_idx] = lines[new_idx], lines[idx]
        all_lines = list(lb.get(0,"end"))
        all_lines[idx], all_lines[new_idx] = all_lines[new_idx], all_lines[idx]
        lb.delete(0,"end")
        for l in all_lines: lb.insert("end",l)
        lb.selection_set(new_idx)

    # ── Save/Export ───────────────────────────────────────────────
    def _save_all(self):
        self._save_dialogues()
        messagebox.showinfo("Сохранено",f"✅ Диалоги сохранены в:\n{self.dlg_path}")

    def _export_json(self):
        p = filedialog.asksaveasfilename(defaultextension=".json",filetypes=[("JSON","*.json")])
        if not p: return
        Path(p).write_text(json.dumps(self.dialogues,ensure_ascii=False,indent=2),encoding="utf-8")
        messagebox.showinfo("Экспорт",f"Экспортировано в:\n{p}")


# ══════════════════════════════════════════════════════════════
# ГЛАВНОЕ ПРИЛОЖЕНИЕ
# ══════════════════════════════════════════════════════════════

# ══════════════════════════════════════════════════════════════
# КОНФИГ ИГРЫ — запись game_config.json для автоматического
# подхвата движком при следующем запуске
# ══════════════════════════════════════════════════════════════

DEFAULT_GAME_CONFIG = {
    "player_spawn": {"x": CITY_CX, "y": CITY_CY - 5},
    "enemy_count": 8,
    "respawn_time": 60.0,
    "boss_enabled": True,
    "particle_scale": 1.0,
    "lights": {
        "ambient": {"r": 30, "g": 20, "b": 50},
        "enabled": True,
    },
    "shaders": {
        "enabled": False,
        "brightness": 1.0,
        "saturation": 1.0,
        "vignette": 0.3,
        "bloom": 0.0,
    },
    "layers": {
        "ground":   True,
        "objects":  True,
        "entities": True,
        "effects":  True,
    },
    "location": {
        "active_zone": "Aethoria City",
        "zones": ["Aethoria City", "Dark Forest", "Goblin Caves", "Dragon Lair", "Frozen Tundra"],
    },
}


class GameConfigTab:
    """Вкладка ⚙ Настройки — spawn/particles/lights/layers/shaders/zones"""

    def __init__(self, notebook, project_root: Path):
        self.project_root = project_root
        self.config_path  = project_root / "assets" / "game_config.json"
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="⚙ Настройки")

        self.config = self._load_config()
        self._vars  = {}       # tkinter variable refs
        self._build_ui()

    # ── Загрузка / сохранение ─────────────────────────────────
    def _load_config(self) -> dict:
        if self.config_path.exists():
            try:
                data = json.loads(self.config_path.read_text(encoding="utf-8"))
                # deep-merge с дефолтами
                import copy
                result = copy.deepcopy(DEFAULT_GAME_CONFIG)
                self._deep_merge(result, data)
                return result
            except Exception:
                pass
        return __import__("copy").deepcopy(DEFAULT_GAME_CONFIG)

    def _deep_merge(self, base, override):
        for k, v in override.items():
            if k in base and isinstance(base[k], dict) and isinstance(v, dict):
                self._deep_merge(base[k], v)
            else:
                base[k] = v

    def save(self):
        """Читает все tkinter-переменные и пишет game_config.json"""
        try:
            c = self.config
            # Spawn
            c["player_spawn"]["x"] = int(self._vars["spawn_x"].get())
            c["player_spawn"]["y"] = int(self._vars["spawn_y"].get())
            # Enemies
            c["enemy_count"]   = int(self._vars["enemy_count"].get())
            c["respawn_time"]  = float(self._vars["respawn_time"].get())
            c["boss_enabled"]  = bool(self._vars["boss_enabled"].get())
            # Particles
            c["particle_scale"] = round(float(self._vars["particle_scale"].get()), 2)
            # Lights
            c["lights"]["ambient"]["r"] = int(self._vars["amb_r"].get())
            c["lights"]["ambient"]["g"] = int(self._vars["amb_g"].get())
            c["lights"]["ambient"]["b"] = int(self._vars["amb_b"].get())
            c["lights"]["enabled"]      = bool(self._vars["lights_enabled"].get())
            # Shaders
            c["shaders"]["enabled"]    = bool(self._vars["shaders_enabled"].get())
            c["shaders"]["brightness"] = round(float(self._vars["brightness"].get()), 2)
            c["shaders"]["saturation"] = round(float(self._vars["saturation"].get()), 2)
            c["shaders"]["vignette"]   = round(float(self._vars["vignette"].get()), 2)
            c["shaders"]["bloom"]      = round(float(self._vars["bloom"].get()), 2)
            # Layers
            c["layers"]["ground"]   = bool(self._vars["layer_ground"].get())
            c["layers"]["objects"]  = bool(self._vars["layer_objects"].get())
            c["layers"]["entities"] = bool(self._vars["layer_entities"].get())
            c["layers"]["effects"]  = bool(self._vars["layer_effects"].get())
            # Location
            c["location"]["active_zone"] = self._vars["active_zone"].get()
            # Write
            self.config_path.write_text(
                json.dumps(c, ensure_ascii=False, indent=2), encoding="utf-8"
            )
            return True
        except Exception as e:
            messagebox.showerror("Ошибка", f"Не удалось сохранить конфиг:\n{e}")
            return False

    # ── UI ────────────────────────────────────────────────────
    def _build_ui(self):
        # Скролл-область
        canvas = tk.Canvas(self.frame, bg=C["bg"], highlightthickness=0)
        vsb = ttk.Scrollbar(self.frame, orient="vertical", command=canvas.yview,
                            style="Dark.Vertical.TScrollbar")
        canvas.configure(yscrollcommand=vsb.set)
        vsb.pack(side="right", fill="y")
        canvas.pack(side="left", fill="both", expand=True)

        inner = tk.Frame(canvas, bg=C["bg"])
        win_id = canvas.create_window((0, 0), window=inner, anchor="nw")

        def _on_configure(event):
            canvas.configure(scrollregion=canvas.bbox("all"))
            canvas.itemconfig(win_id, width=canvas.winfo_width())
        inner.bind("<Configure>", _on_configure)
        canvas.bind("<Configure>", lambda e: canvas.itemconfig(win_id, width=e.width))
        canvas.bind_all("<MouseWheel>", lambda e: canvas.yview_scroll(-1*(e.delta//120), "units"))

        # Колонки
        left  = tk.Frame(inner, bg=C["bg"])
        right = tk.Frame(inner, bg=C["bg"])
        left.grid(row=0, column=0, sticky="nsew", padx=8, pady=8)
        right.grid(row=0, column=1, sticky="nsew", padx=8, pady=8)
        inner.columnconfigure(0, weight=1)
        inner.columnconfigure(1, weight=1)

        self._build_spawn_section(left)
        self._build_enemy_section(left)
        self._build_particle_section(left)
        self._build_layer_section(right)
        self._build_location_section(right)
        self._build_lights_section(right)
        self._build_shaders_section(right)
        self._build_save_bar(inner)

    def _section(self, parent, title):
        fr = tk.LabelFrame(parent, text=title, bg=C["panel"], fg=C["gold"],
                           font=("Segoe UI", 10, "bold"), bd=1, relief="groove",
                           labelanchor="nw")
        fr.pack(fill="x", pady=6, padx=4)
        return fr

    def _row(self, parent, label, widget_cb, row):
        tk.Label(parent, text=label, bg=C["panel"], fg=C["text"],
                 font=("Segoe UI", 9), anchor="w").grid(
            row=row, column=0, sticky="w", padx=8, pady=3)
        w = widget_cb(parent)
        w.grid(row=row, column=1, sticky="ew", padx=8, pady=3)
        parent.columnconfigure(1, weight=1)
        return w

    def _entry(self, parent, var):
        e = tk.Entry(parent, textvariable=var, width=10,
                     bg=C["input_bg"], fg=C["text"], insertbackground=C["gold"],
                     relief="flat", font=("Segoe UI", 9))
        return e

    def _check(self, parent, var, text=""):
        return tk.Checkbutton(parent, variable=var, text=text,
                              bg=C["panel"], fg=C["text"], selectcolor=C["panel2"],
                              activebackground=C["panel"], activeforeground=C["gold"],
                              font=("Segoe UI", 9))

    def _slider(self, parent, var, from_, to, resolution=0.01):
        return ttk.Scale(parent, variable=var, from_=from_, to=to,
                         orient="horizontal", style="Dark.Horizontal.TScale")

    def _color_preview(self, parent, r_var, g_var, b_var):
        """Маленький квадрат-превью цвета"""
        box = tk.Label(parent, width=4, bg=C["panel"], relief="flat")
        def update(*_):
            try:
                col = f"#{int(r_var.get()):02x}{int(g_var.get()):02x}{int(b_var.get()):02x}"
                box.config(bg=col)
            except: pass
        r_var.trace_add("write", update)
        g_var.trace_add("write", update)
        b_var.trace_add("write", update)
        update()
        return box

    # ── Блоки ──────────────────────────────────────────────────
    def _build_spawn_section(self, parent):
        fr = self._section(parent, "🎯 Спаун игрока")
        c = self.config["player_spawn"]
        sx = tk.IntVar(value=c["x"]);  self._vars["spawn_x"] = sx
        sy = tk.IntVar(value=c["y"]);  self._vars["spawn_y"] = sy

        self._row(fr, "X (тайлы):", lambda p: self._entry(p, sx), 0)
        self._row(fr, "Y (тайлы):", lambda p: self._entry(p, sy), 1)

        info = tk.Label(fr, text=f"Карта {MAP_W}×{MAP_H}  •  Город: {CITY_CX},{CITY_CY}",
                        bg=C["panel"], fg=C["muted"], font=("Segoe UI", 8))
        info.grid(row=2, column=0, columnspan=2, sticky="w", padx=8, pady=2)

        def _goto_city():
            sx.set(CITY_CX); sy.set(CITY_CY - 5)
        btn(fr, "◉ Город", _goto_city, padx=6, pady=2).grid(row=3, column=0, sticky="w", padx=8, pady=4)

    def _build_enemy_section(self, parent):
        fr = self._section(parent, "👹 Враги")
        ec_var = tk.IntVar(value=self.config["enemy_count"]);      self._vars["enemy_count"]  = ec_var
        rt_var = tk.DoubleVar(value=self.config["respawn_time"]);  self._vars["respawn_time"] = rt_var
        be_var = tk.BooleanVar(value=self.config["boss_enabled"]); self._vars["boss_enabled"] = be_var

        self._row(fr, "Кол-во врагов:", lambda p: self._entry(p, ec_var), 0)

        # Слайдер кол-ва
        sl = ttk.Scale(fr, variable=ec_var, from_=0, to=50, orient="horizontal",
                       style="Dark.Horizontal.TScale")
        sl.grid(row=1, column=0, columnspan=2, sticky="ew", padx=8, pady=2)

        self._row(fr, "Respawn (сек):", lambda p: self._entry(p, rt_var), 2)
        self._check(fr, be_var, "Боссы включены").grid(row=3, column=0, columnspan=2, sticky="w", padx=8, pady=4)

    def _build_particle_section(self, parent):
        fr = self._section(parent, "✨ Частицы")
        ps_var = tk.DoubleVar(value=self.config["particle_scale"]); self._vars["particle_scale"] = ps_var

        self._row(fr, "Масштаб:", lambda p: self._entry(p, ps_var), 0)
        sl = ttk.Scale(fr, variable=ps_var, from_=0.0, to=3.0, orient="horizontal",
                       style="Dark.Horizontal.TScale")
        sl.grid(row=1, column=0, columnspan=2, sticky="ew", padx=8, pady=2)
        tk.Label(fr, text="0 = выкл  •  1 = норм  •  3 = максимум",
                 bg=C["panel"], fg=C["muted"], font=("Segoe UI", 8)
                 ).grid(row=2, column=0, columnspan=2, sticky="w", padx=8)

    def _build_layer_section(self, parent):
        fr = self._section(parent, "🗂 Слои")
        layers = self.config["layers"]
        items = [
            ("layer_ground",   "Земля (тайлы карты)",   layers.get("ground",   True)),
            ("layer_objects",  "Объекты (деревья, etc)", layers.get("objects",  True)),
            ("layer_entities", "Сущности (игрок, мобы)", layers.get("entities", True)),
            ("layer_effects",  "Эффекты (частицы)",      layers.get("effects",  True)),
        ]
        for i, (key, label, val) in enumerate(items):
            v = tk.BooleanVar(value=val); self._vars[key] = v
            self._check(fr, v, label).grid(row=i, column=0, columnspan=2,
                                           sticky="w", padx=8, pady=3)

    def _build_location_section(self, parent):
        fr = self._section(parent, "📍 Локация / Зона")
        loc = self.config.get("location", {})
        zones = loc.get("zones", ["Aethoria City"])
        active = loc.get("active_zone", zones[0])

        az_var = tk.StringVar(value=active); self._vars["active_zone"] = az_var
        cb = ttk.Combobox(fr, textvariable=az_var, values=zones, state="readonly",
                          font=("Segoe UI", 9))
        cb.grid(row=0, column=0, columnspan=2, sticky="ew", padx=8, pady=6)
        fr.columnconfigure(0, weight=1)

        # Кнопки добавить/удалить зону
        def _add_zone():
            name = simpledialog.askstring("Зона", "Название новой зоны:", parent=fr)
            if name and name not in zones:
                zones.append(name)
                cb["values"] = zones
                az_var.set(name)

        def _del_zone():
            cur = az_var.get()
            if cur in zones and len(zones) > 1:
                zones.remove(cur)
                cb["values"] = zones
                az_var.set(zones[0])

        row_btns = tk.Frame(fr, bg=C["panel"])
        row_btns.grid(row=1, column=0, columnspan=2, sticky="w", padx=6, pady=2)
        btn(row_btns, "+ Добавить", _add_zone, padx=6, pady=2).pack(side="left", padx=2)
        btn(row_btns, "— Удалить",  _del_zone, color=C["danger"], padx=6, pady=2).pack(side="left", padx=2)

    def _build_lights_section(self, parent):
        fr = self._section(parent, "💡 Освещение")
        lights = self.config.get("lights", {})
        amb    = lights.get("ambient", {"r": 30, "g": 20, "b": 50})

        le_var = tk.BooleanVar(value=lights.get("enabled", True)); self._vars["lights_enabled"] = le_var
        r_var  = tk.IntVar(value=amb.get("r", 30));  self._vars["amb_r"] = r_var
        g_var  = tk.IntVar(value=amb.get("g", 20));  self._vars["amb_g"] = g_var
        b_var  = tk.IntVar(value=amb.get("b", 50));  self._vars["amb_b"] = b_var

        self._check(fr, le_var, "Освещение включено").grid(
            row=0, column=0, columnspan=3, sticky="w", padx=8, pady=4)

        tk.Label(fr, text="Цвет атмосферы:", bg=C["panel"], fg=C["text"],
                 font=("Segoe UI", 9)).grid(row=1, column=0, sticky="w", padx=8)

        preview = self._color_preview(fr, r_var, g_var, b_var)
        preview.grid(row=1, column=2, padx=6)

        for i, (label, var, preset_dark, preset_day, preset_dawn) in enumerate([
            ("R:", r_var, 30, 140, 255),
            ("G:", g_var, 20, 160,  80),
            ("B:", b_var, 50, 200,  30),
        ]):
            row = 2 + i
            tk.Label(fr, text=label, bg=C["panel"], fg=C["text"],
                     font=("Segoe UI", 9), width=3).grid(row=row, column=0, sticky="w", padx=8)
            sl = ttk.Scale(fr, variable=var, from_=0, to=255, orient="horizontal",
                           style="Dark.Horizontal.TScale")
            sl.grid(row=row, column=1, sticky="ew", padx=4, pady=1)
            ent = tk.Entry(fr, textvariable=var, width=4,
                           bg=C["input_bg"], fg=C["text"], relief="flat",
                           font=("Segoe UI", 9))
            ent.grid(row=row, column=2, padx=4)
        fr.columnconfigure(1, weight=1)

        # Пресеты
        presets_fr = tk.Frame(fr, bg=C["panel"])
        presets_fr.grid(row=5, column=0, columnspan=3, sticky="w", padx=6, pady=4)
        tk.Label(presets_fr, text="Пресеты:", bg=C["panel"], fg=C["muted"],
                 font=("Segoe UI", 8)).pack(side="left")

        def _apply(r, g, b):
            r_var.set(r); g_var.set(g); b_var.set(b)

        for label, r, g, b in [
            ("Ночь",  30,  20,  50),
            ("День",  140, 160, 200),
            ("Закат", 255,  80,  30),
            ("Данж",  10,   5,  15),
        ]:
            btn(presets_fr, label, lambda r=r,g=g,b=b: _apply(r,g,b),
                padx=5, pady=1).pack(side="left", padx=3)

    def _build_shaders_section(self, parent):
        fr = self._section(parent, "🎨 Шейдеры / Пост-обработка")
        sh = self.config.get("shaders", {})

        se_var  = tk.BooleanVar(value=sh.get("enabled",    False)); self._vars["shaders_enabled"] = se_var
        br_var  = tk.DoubleVar(value=sh.get("brightness",  1.0));   self._vars["brightness"]      = br_var
        sa_var  = tk.DoubleVar(value=sh.get("saturation",  1.0));   self._vars["saturation"]      = sa_var
        vi_var  = tk.DoubleVar(value=sh.get("vignette",    0.3));   self._vars["vignette"]        = vi_var
        bl_var  = tk.DoubleVar(value=sh.get("bloom",       0.0));   self._vars["bloom"]           = bl_var

        self._check(fr, se_var, "Шейдеры включены (требует rebuild)").grid(
            row=0, column=0, columnspan=2, sticky="w", padx=8, pady=4)

        sliders = [
            ("Яркость:",    br_var, 0.0, 2.0),
            ("Насыщенность:", sa_var, 0.0, 2.0),
            ("Виньетка:",   vi_var, 0.0, 1.0),
            ("Bloom:",      bl_var, 0.0, 1.0),
        ]
        for i, (label, var, lo, hi) in enumerate(sliders):
            row = 1 + i
            tk.Label(fr, text=label, bg=C["panel"], fg=C["text"],
                     font=("Segoe UI", 9)).grid(row=row, column=0, sticky="w", padx=8)
            sl = ttk.Scale(fr, variable=var, from_=lo, to=hi, orient="horizontal",
                           style="Dark.Horizontal.TScale")
            sl.grid(row=row, column=1, sticky="ew", padx=4, pady=2)
            ent = tk.Entry(fr, textvariable=var, width=5,
                           bg=C["input_bg"], fg=C["text"], relief="flat",
                           font=("Segoe UI", 9))
            ent.grid(row=row, column=2, padx=4)
        fr.columnconfigure(1, weight=1)

        def _reset_shaders():
            br_var.set(1.0); sa_var.set(1.0); vi_var.set(0.3); bl_var.set(0.0)

        btn(fr, "↺ Сбросить", _reset_shaders, padx=6, pady=2).grid(
            row=len(sliders)+1, column=0, sticky="w", padx=8, pady=6)

    def _build_save_bar(self, parent):
        bar = tk.Frame(parent, bg=C["panel2"])
        bar.grid(row=1, column=0, columnspan=2, sticky="ew", padx=8, pady=8)

        self._status_lbl = tk.Label(bar, text="", bg=C["panel2"], fg=C["green"],
                                    font=("Segoe UI", 9))
        self._status_lbl.pack(side="left", padx=12)

        def _save_and_notify():
            if self.save():
                self._status_lbl.config(text="✅ game_config.json сохранён → движок подхватит при перезапуске")
                self.frame.after(4000, lambda: self._status_lbl.config(text=""))

        btn(bar, "💾 Сохранить и применить", _save_and_notify,
            color=C["accent"], fg="white", padx=14, pady=5).pack(side="right", padx=8)
        btn(bar, "↺ Сброс к дефолтам", self._reset_to_defaults,
            padx=10, pady=5).pack(side="right", padx=4)

    def _reset_to_defaults(self):
        if messagebox.askyesno("Сброс", "Сбросить все настройки к дефолтным?"):
            self.config = __import__("copy").deepcopy(DEFAULT_GAME_CONFIG)
            # Перестраиваем UI
            for w in self.frame.winfo_children():
                w.destroy()
            self._vars = {}
            self._build_ui()


# ══════════════════════════════════════════════════════════════
# КОНФИГ ЭКРАНОВ (login / char select / UI)
# ══════════════════════════════════════════════════════════════
DEFAULT_UI_CONFIG = {
    "login_screen": {
        "title":       "AETHORIA: Eternal Realms",
        "subtitle":    "Enter the world of endless adventure",
        "bg_color":    "#0a051a",
        "title_color": "#f0c040",
        "star_count":  200,
        "show_stars":  True,
        "login_label": "Login:",
        "pass_label":  "Password:",
        "enter_btn":   "ENTER WORLD",
        "enter_color": "#6428cc",
        "logo_emoji":  "⚔",
        "music_track": "assets/music/login.ogg",
    },
    "char_select": {
        "title":           "Select Your Character",
        "slot_count":      3,
        "classes": [
            {"name":"Warrior",  "color":"#cc5040","emoji":"⚔","hp":120,"mp":50,
             "str":14,"agi":8,"int":5,"vit":12,"description":"Мощный воин ближнего боя"},
            {"name":"Mage",     "color":"#4466cc","emoji":"🔮","hp":70,"mp":120,
             "str":5,"agi":7,"int":15,"vit":7,"description":"Маг с мощными заклинаниями"},
            {"name":"Rogue",    "color":"#44aa44","emoji":"🗡","hp":90,"mp":70,
             "str":9,"agi":15,"int":8,"vit":9,"description":"Ловкий убийца из тени"},
            {"name":"Paladin",  "color":"#ccaa22","emoji":"🛡","hp":110,"mp":80,
             "str":11,"agi":7,"int":10,"vit":13,"description":"Священный воин-защитник"},
            {"name":"Ranger",   "color":"#44bb66","emoji":"🏹","hp":85,"mp":75,
             "str":8,"agi":14,"int":9,"vit":10,"description":"Меткий стрелок из леса"},
            {"name":"Shaman",   "color":"#aa44cc","emoji":"🌀","hp":80,"mp":110,
             "str":7,"agi":9,"int":13,"vit":10,"description":"Призыватель духов природы"},
        ],
        "skins": ["Classic","Shadow","Gold","Crimson","Arctic"],
        "bg_color":   "#0f0a1e",
        "enter_btn":  "ENTER WORLD",
        "back_btn":   "< Back",
    },
    "hud": {
        "hp_bar_color":    "#cc3232",
        "mp_bar_color":    "#3264cc",
        "xp_bar_color":    "#44cc44",
        "show_minimap":    True,
        "show_level":      True,
        "show_gold":       True,
        "skill_slots":     4,
        "chat_lines":      5,
    },
}

# ══════════════════════════════════════════════════════════════
# ВКЛАДКА: ЛОГИН СКРИН
# ══════════════════════════════════════════════════════════════
class LoginScreenTab:
    """Редактор экрана входа — текст, цвета, музыка, звёзды."""

    def __init__(self, notebook, project_root: Path):
        self.project_root = project_root
        self.cfg_path     = project_root / "assets" / "ui_config.json"
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="🔑 Логин")
        self.cfg = self._load()
        self._vars = {}
        self._build_ui()

    def _load(self):
        import copy
        base = copy.deepcopy(DEFAULT_UI_CONFIG["login_screen"])
        if self.cfg_path.exists():
            try:
                full = json.loads(self.cfg_path.read_text(encoding="utf-8"))
                ls = full.get("login_screen", {})
                base.update(ls)
            except Exception: pass
        return base

    def _save(self):
        full = {}
        if self.cfg_path.exists():
            try: full = json.loads(self.cfg_path.read_text(encoding="utf-8"))
            except Exception: pass
        ls = self.cfg
        for k, var in self._vars.items():
            v = var.get()
            if isinstance(v, str) and v.isdigit(): v = int(v)
            ls[k] = v
        full["login_screen"] = ls
        self.cfg_path.write_text(json.dumps(full, ensure_ascii=False, indent=2), encoding="utf-8")

    def _build_ui(self):
        # ── Header ────────────────────────────────────────────
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=6)
        hdr.pack(fill="x")
        tk.Label(hdr, text="🔑  РЕДАКТОР ЭКРАНА ВХОДА", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI",13,"bold"), padx=16).pack(side="left")
        btn(hdr, "💾 Сохранить", self._save, C["accent"], padx=14, pady=4).pack(side="right", padx=8)

        # ── Скролл-область ────────────────────────────────────
        cv = tk.Canvas(self.frame, bg=C["bg"], highlightthickness=0)
        sb = ttk.Scrollbar(self.frame, orient="vertical", command=cv.yview,
                           style="Dark.Vertical.TScrollbar")
        cv.configure(yscrollcommand=sb.set)
        sb.pack(side="right", fill="y"); cv.pack(fill="both", expand=True)
        inner = tk.Frame(cv, bg=C["bg"])
        win_id = cv.create_window((0,0), window=inner, anchor="nw")
        inner.bind("<Configure>", lambda e: cv.configure(scrollregion=cv.bbox("all")))
        cv.bind("<Configure>", lambda e: cv.itemconfig(win_id, width=e.width))

        # ── Left/Right columns ────────────────────────────────
        cols = tk.Frame(inner, bg=C["bg"])
        cols.pack(fill="both", expand=True, padx=16, pady=12)
        left  = tk.Frame(cols, bg=C["bg"]); left.pack(side="left",  fill="both", expand=True, padx=(0,8))
        right = tk.Frame(cols, bg=C["bg"]); right.pack(side="right", fill="both", expand=True, padx=(8,0))

        # ── LEFT: Тексты ──────────────────────────────────────
        self._section(left, "📝 ТЕКСТЫ")
        fields = [
            ("title",       "Заголовок игры:"),
            ("subtitle",    "Подзаголовок:"),
            ("login_label", "Лейбл «Логин»:"),
            ("pass_label",  "Лейбл «Пароль»:"),
            ("enter_btn",   "Кнопка входа:"),
            ("logo_emoji",  "Логотип (эмодзи):"),
        ]
        for key, label in fields:
            self._field(left, key, label)

        self._section(left, "🎵 ЗВУК")
        self._field(left, "music_track", "Трек меню:")
        fr = tk.Frame(left, bg=C["bg"]); fr.pack(fill="x", pady=2)
        btn(fr, "📂 Выбрать файл", lambda: self._pick_file("music_track", [("Audio","*.ogg *.mp3 *.wav")]),
            C["panel3"], padx=8, pady=3).pack(side="left")

        self._section(left, "⭐ ЗВЁЗДЫ")
        self._field(left, "star_count", "Кол-во звёзд:")
        self._bool_field(left, "show_stars", "Показывать звёзды")

        # ── RIGHT: Цвета + превью ─────────────────────────────
        self._section(right, "🎨 ЦВЕТА")
        color_fields = [
            ("bg_color",    "Фон:"),
            ("title_color", "Заголовок:"),
            ("enter_color", "Кнопка «Войти»:"),
        ]
        for key, label in color_fields:
            self._color_field(right, key, label)

        self._section(right, "👁 ПРЕВЬЮ")
        self._build_preview(right)

    def _section(self, parent, title):
        tk.Label(parent, text=title, bg=C["bg"], fg=C["gold"],
                 font=("Segoe UI",10,"bold"), pady=6).pack(fill="x")

    def _field(self, parent, key, label):
        fr = tk.Frame(parent, bg=C["bg"]); fr.pack(fill="x", pady=2)
        tk.Label(fr, text=label, bg=C["bg"], fg=C["muted"],
                 font=("Segoe UI",9), width=22, anchor="w").pack(side="left")
        var = tk.StringVar(value=str(self.cfg.get(key,"")))
        self._vars[key] = var
        e = tk.Entry(fr, textvariable=var, bg=C["input_bg"], fg=C["text"],
                     insertbackground=C["gold"], relief="flat", font=("Segoe UI",9))
        e.pack(side="left", fill="x", expand=True, padx=4)

    def _bool_field(self, parent, key, label):
        fr = tk.Frame(parent, bg=C["bg"]); fr.pack(fill="x", pady=2)
        var = tk.BooleanVar(value=bool(self.cfg.get(key, True)))
        self._vars[key] = var
        tk.Checkbutton(fr, text=label, variable=var, bg=C["bg"], fg=C["text"],
                       selectcolor=C["panel3"], activebackground=C["bg"],
                       font=("Segoe UI",9)).pack(side="left")

    def _color_field(self, parent, key, label):
        fr = tk.Frame(parent, bg=C["bg"]); fr.pack(fill="x", pady=3)
        tk.Label(fr, text=label, bg=C["bg"], fg=C["muted"],
                 font=("Segoe UI",9), width=18, anchor="w").pack(side="left")
        var = tk.StringVar(value=str(self.cfg.get(key,"#000000")))
        self._vars[key] = var
        preview = tk.Label(fr, text="  ", bg=var.get(), width=3, relief="flat")
        preview.pack(side="left", padx=4)
        e = tk.Entry(fr, textvariable=var, bg=C["input_bg"], fg=C["text"],
                     relief="flat", font=("Segoe UI",9), width=10)
        e.pack(side="left")
        def pick_color(v=var, p=preview):
            from tkinter import colorchooser
            c = colorchooser.askcolor(color=v.get())[1]
            if c: v.set(c); p.config(bg=c)
        btn(fr, "🎨", pick_color, C["panel3"], padx=4, pady=2).pack(side="left", padx=2)
        e.bind("<FocusOut>", lambda ev, v=var, p=preview: self._try_set_color(v.get(), p))

    def _try_set_color(self, val, widget):
        try: widget.config(bg=val)
        except Exception: pass

    def _pick_file(self, key, filetypes):
        p = filedialog.askopenfilename(filetypes=filetypes)
        if p and key in self._vars:
            rel = str(Path(p).relative_to(self.project_root)) if p.startswith(str(self.project_root)) else p
            self._vars[key].set(rel)

    def _build_preview(self, parent):
        """Миниатюрный превью логин-скрина"""
        pv = tk.Canvas(parent, width=320, height=200, bg="#0a051a",
                       highlightthickness=1, highlightbackground=C["border"])
        pv.pack(pady=8)
        self._pv_canvas = pv
        self._draw_preview()
        btn(parent, "🔄 Обновить превью", self._draw_preview, C["panel3"], padx=8, pady=3).pack()

    def _draw_preview(self):
        c = self._pv_canvas
        c.delete("all")
        # BG
        bg = self.cfg.get("bg_color","#0a051a")
        try: c.config(bg=bg)
        except Exception: pass
        # Stars
        if self.cfg.get("show_stars", True):
            import random as rr
            rnd = rr.Random(7)
            for _ in range(min(int(self.cfg.get("star_count",200)), 80)):
                x = rnd.randint(2, 318); y = rnd.randint(2, 198)
                c.create_oval(x,y,x+1,y+1, fill="white", outline="")
        # Title
        tc = self.cfg.get("title_color","#f0c040")
        c.create_text(160, 40, text=self.cfg.get("logo_emoji","⚔")+" "+self.cfg.get("title","AETHORIA"),
                      fill=tc, font=("Segoe UI",12,"bold"))
        c.create_text(160, 60, text=self.cfg.get("subtitle",""),
                      fill="#886688", font=("Segoe UI",7))
        # Panel
        c.create_rectangle(80,75,240,160, fill="#14102a", outline="#5533aa", width=1)
        c.create_text(100, 88, text=self.cfg.get("login_label","Login:"),
                      fill="#9977cc", font=("Segoe UI",7), anchor="w")
        c.create_rectangle(100,96,230,108, fill="#0c0a1e", outline="#332266")
        c.create_text(100, 116, text=self.cfg.get("pass_label","Password:"),
                      fill="#9977cc", font=("Segoe UI",7), anchor="w")
        c.create_rectangle(100,124,230,136, fill="#0c0a1e", outline="#332266")
        ec = self.cfg.get("enter_color","#6428cc")
        try: c.create_rectangle(110,143,220,155, fill=ec, outline="#aa66ff")
        except Exception: c.create_rectangle(110,143,220,155, fill="#6428cc", outline="#aa66ff")
        c.create_text(165, 149, text=self.cfg.get("enter_btn","ENTER WORLD"),
                      fill="white", font=("Segoe UI",7,"bold"))


# ══════════════════════════════════════════════════════════════
# ВКЛАДКА: ВЫБОР ПЕРСОНАЖА
# ══════════════════════════════════════════════════════════════
class CharacterSelectTab:
    """Редактор экрана выбора персонажа — классы, скины, статы."""

    def __init__(self, notebook, project_root: Path):
        self.project_root = project_root
        self.cfg_path     = project_root / "assets" / "ui_config.json"
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="🧙 Персонажи")
        self.cfg = self._load()
        self._sel_class = 0
        self._vars = {}
        self._build_ui()

    def _load(self):
        import copy
        base = copy.deepcopy(DEFAULT_UI_CONFIG["char_select"])
        if self.cfg_path.exists():
            try:
                full = json.loads(self.cfg_path.read_text(encoding="utf-8"))
                cs = full.get("char_select", {})
                if "classes" in cs: base["classes"] = cs["classes"]
                for k in ("title","slot_count","skins","bg_color","enter_btn","back_btn"):
                    if k in cs: base[k] = cs[k]
            except Exception: pass
        # ── Если есть classes.json — он главный источник данных о классах ──
        classes_path = self.project_root / "assets" / "classes.json"
        if classes_path.exists():
            try:
                classes_data = json.loads(classes_path.read_text(encoding="utf-8"))
                ui_classes = []
                for cid, cd in classes_data.items():
                    bs = cd.get("base_stats", {})
                    ui_classes.append({
                        "name":        cd.get("name", cid),
                        "emoji":       cd.get("emoji", "⚔"),
                        "color":       cd.get("color", "#888888"),
                        "description": cd.get("description", ""),
                        "hp":  int(bs.get("hp",  100)),
                        "mp":  int(bs.get("mp",   50)),
                        "str": int(bs.get("str",  10)),
                        "agi": int(bs.get("dex",  10)),
                        "int": int(bs.get("int",  10)),
                        "vit": int(bs.get("vit",  10)),
                    })
                if ui_classes:
                    base["classes"] = ui_classes
            except Exception: pass
        return base

    def _save(self):
        full = {}
        if self.cfg_path.exists():
            try: full = json.loads(self.cfg_path.read_text(encoding="utf-8"))
            except Exception: pass
        # Собираем из _vars
        for k, var in self._vars.items():
            if "." in k:
                # класс поле: "0.name"
                idx, field = k.split(".", 1)
                idx = int(idx)
                while len(self.cfg["classes"]) <= idx:
                    self.cfg["classes"].append({})
                v = var.get()
                for int_field in ("hp","mp","str","agi","int","vit"):
                    if field == int_field:
                        try: v = int(v)
                        except: v = 0
                self.cfg["classes"][idx][field] = v
            else:
                v = var.get()
                if k == "slot_count":
                    try: v = int(v)
                    except: v = 3
                self.cfg[k] = v
        full["char_select"] = self.cfg
        self.cfg_path.write_text(json.dumps(full, ensure_ascii=False, indent=2), encoding="utf-8")
        messagebox.showinfo("Сохранено", "Настройки выбора персонажа сохранены!")

    def _build_ui(self):
        # Header
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=6)
        hdr.pack(fill="x")
        tk.Label(hdr, text="🧙  РЕДАКТОР ВЫБОРА ПЕРСОНАЖА", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI",13,"bold"), padx=16).pack(side="left")
        btn(hdr, "💾 Сохранить", self._save, C["accent"], padx=14, pady=4).pack(side="right", padx=8)
        btn(hdr, "+ Новый класс", self._add_class, C["green"], "black", padx=10, pady=4).pack(side="right", padx=4)

        # ── Инфо-баннер ───────────────────────────────────────────
        info_bar = tk.Frame(self.frame, bg="#1a1230", pady=5)
        info_bar.pack(fill="x")
        tk.Label(info_bar,
                 text="ℹ  Базовые статы берутся из вкладки «⚔ Классы» (classes.json). "
                      "Здесь редактируются внешний вид и настройки экрана выбора. "
                      "Сохрани в «⚔ Классы» — данные сюда обновятся автоматически.",
                 bg="#1a1230", fg=C["cyan"],
                 font=("Segoe UI", 8), padx=16, anchor="w").pack(fill="x")

        body = tk.Frame(self.frame, bg=C["bg"])
        body.pack(fill="both", expand=True)

        # Левая панель — список классов
        left = tk.Frame(body, bg=C["panel"], width=220)
        left.pack(side="left", fill="y"); left.pack_propagate(False)

        tk.Label(left, text="Классы персонажей", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI",10,"bold"), pady=8).pack(fill="x", padx=8)

        self._class_listbox = tk.Listbox(left, bg=C["panel2"], fg=C["text"],
                                          selectbackground=C["accent"], relief="flat",
                                          font=("Segoe UI",10), activestyle="none",
                                          highlightthickness=0)
        self._class_listbox.pack(fill="both", expand=True, padx=4, pady=4)
        self._class_listbox.bind("<<ListboxSelect>>", self._on_class_select)

        btn(left, "🗑 Удалить класс", self._del_class, C["danger"],
            padx=6, pady=3).pack(fill="x", padx=8, pady=4)

        # Правая панель — редактор выбранного класса
        right = tk.Frame(body, bg=C["bg"])
        right.pack(side="left", fill="both", expand=True)

        # Скролл
        cv = tk.Canvas(right, bg=C["bg"], highlightthickness=0)
        sb = ttk.Scrollbar(right, orient="vertical", command=cv.yview,
                           style="Dark.Vertical.TScrollbar")
        cv.configure(yscrollcommand=sb.set)
        sb.pack(side="right", fill="y"); cv.pack(fill="both", expand=True)
        self._class_frame = tk.Frame(cv, bg=C["bg"])
        win_id = cv.create_window((0,0), window=self._class_frame, anchor="nw")
        self._class_frame.bind("<Configure>", lambda e: cv.configure(scrollregion=cv.bbox("all")))
        cv.bind("<Configure>", lambda e: cv.itemconfig(win_id, width=e.width))
        self._class_edit_cv = cv

        # Нижняя панель — глобальные настройки
        bot = tk.Frame(self.frame, bg=C["panel"], pady=6)
        bot.pack(fill="x", side="bottom")
        tk.Label(bot, text="Общие настройки:", bg=C["panel"], fg=C["muted"],
                 font=("Segoe UI",9), padx=12).pack(side="left")
        for k, lbl_text in [("title","Заголовок:"),("slot_count","Слотов:"),("enter_btn","Кнопка:"),("back_btn","Назад:")]:
            tk.Label(bot, text=lbl_text, bg=C["panel"], fg=C["muted"],
                     font=("Segoe UI",8), padx=4).pack(side="left")
            var = tk.StringVar(value=str(self.cfg.get(k,"")))
            self._vars[k] = var
            tk.Entry(bot, textvariable=var, bg=C["input_bg"], fg=C["text"],
                     relief="flat", font=("Segoe UI",9), width=14).pack(side="left", padx=2)

        self._refresh_class_list()
        if self.cfg["classes"]:
            self._class_listbox.selection_set(0)
            self._load_class_editor(0)

    def _refresh_class_list(self):
        self._class_listbox.delete(0, "end")
        for c in self.cfg["classes"]:
            em = c.get("emoji","?")
            nm = c.get("name","???")
            self._class_listbox.insert("end", f"  {em}  {nm}")

    def _on_class_select(self, event):
        sel = self._class_listbox.curselection()
        if sel:
            self._sel_class = sel[0]
            self._load_class_editor(sel[0])

    def _load_class_editor(self, idx):
        for w in self._class_frame.winfo_children(): w.destroy()
        if idx >= len(self.cfg["classes"]): return
        cls = self.cfg["classes"][idx]
        p = self._class_frame

        tk.Label(p, text=f"Класс [{idx}]: {cls.get('name','')}", bg=C["bg"], fg=C["gold2"],
                 font=("Segoe UI",12,"bold"), pady=8).pack(fill="x", padx=16)

        # Два столбца
        row = tk.Frame(p, bg=C["bg"]); row.pack(fill="x", padx=16)
        left = tk.Frame(row, bg=C["bg"]); left.pack(side="left", fill="both", expand=True)
        right = tk.Frame(row, bg=C["bg"]); right.pack(side="right", fill="both", expand=True, padx=(16,0))

        str_fields = [("name","Название:"),("emoji","Эмодзи:"),("description","Описание:"),("color","Цвет:")]
        for key, lbl_text in str_fields:
            fr = tk.Frame(left, bg=C["bg"]); fr.pack(fill="x", pady=3)
            tk.Label(fr, text=lbl_text, bg=C["bg"], fg=C["muted"],
                     font=("Segoe UI",9), width=14, anchor="w").pack(side="left")
            var = tk.StringVar(value=str(cls.get(key,"")))
            vkey = f"{idx}.{key}"
            self._vars[vkey] = var
            e = tk.Entry(fr, textvariable=var, bg=C["input_bg"], fg=C["text"],
                         relief="flat", font=("Segoe UI",9))
            e.pack(side="left", fill="x", expand=True, padx=4)
            if key == "color":
                def pick(v=var):
                    from tkinter import colorchooser
                    c2 = colorchooser.askcolor(color=v.get())[1]
                    if c2: v.set(c2)
                btn(fr, "🎨", pick, C["panel3"], padx=4, pady=2).pack(side="left")

        # Статы
        classes_path = self.project_root / "assets" / "classes.json"
        stats_readonly = classes_path.exists()
        stats_note = " (из classes.json)" if stats_readonly else ""
        tk.Label(right, text=f"📊 БАЗОВЫЕ СТАТЫ{stats_note}", bg=C["bg"], fg=C["gold"],
                 font=("Segoe UI",10,"bold"), pady=6).pack(fill="x")
        if stats_readonly:
            tk.Label(right, text="Редактируй статы во вкладке «⚔ Классы»",
                     bg=C["bg"], fg=C["muted"], font=("Segoe UI",8)).pack(anchor="w")
        stat_fields = [("hp","HP:"),("mp","MP:"),("str","Сила:"),("agi","Ловкость:"),
                       ("int","Интеллект:"),("vit","Выносливость:")]
        for key, lbl_text in stat_fields:
            fr = tk.Frame(right, bg=C["bg"]); fr.pack(fill="x", pady=2)
            tk.Label(fr, text=lbl_text, bg=C["bg"], fg=C["muted"],
                     font=("Segoe UI",9), width=14, anchor="w").pack(side="left")
            var = tk.StringVar(value=str(cls.get(key, 0)))
            vkey = f"{idx}.{key}"
            self._vars[vkey] = var
            fr2 = tk.Frame(fr, bg=C["bg"]); fr2.pack(side="left")
            spin_state = "disabled" if stats_readonly else "normal"
            spin_bg    = C["panel2"] if stats_readonly else C["input_bg"]
            tk.Spinbox(fr2, textvariable=var, from_=1, to=999, width=6,
                       bg=spin_bg, fg=C["text"] if not stats_readonly else C["muted"],
                       relief="flat", state=spin_state,
                       buttonbackground=C["panel3"], insertbackground=C["gold"],
                       font=("Segoe UI",9)).pack(side="left")

        # Превью персонажа
        tk.Label(p, text="👁 ПРЕВЬЮ", bg=C["bg"], fg=C["gold"],
                 font=("Segoe UI",10,"bold"), pady=6).pack(fill="x", padx=16)
        pv = tk.Canvas(p, width=200, height=160, bg=C["panel"],
                       highlightthickness=1, highlightbackground=C["border"])
        pv.pack(padx=16, pady=4)
        self._draw_char_preview(pv, cls)

    def _draw_char_preview(self, cv, cls):
        cv.delete("all")
        cc = cls.get("color","#8866cc")
        em = cls.get("emoji","?")
        nm = cls.get("name","???")
        # Body
        try:
            cv.create_oval(70,20,130,80, fill=cc, outline="#fff", width=2)
            cv.create_oval(55,80,145,150, fill=cc, outline="#fff", width=2)
            # Eyes
            cv.create_oval(85,38,95,48, fill="white"); cv.create_oval(85,38,95,48, fill="white")
            cv.create_oval(105,38,115,48, fill="white")
            cv.create_oval(88,41,92,45, fill="#1a1a2a"); cv.create_oval(108,41,112,45, fill="#1a1a2a")
        except Exception:
            pass
        cv.create_text(100, 100, text=em, font=("Segoe UI",32))
        cv.create_text(100, 145, text=nm, fill=cc, font=("Segoe UI",10,"bold"))

    def _add_class(self):
        name = simpledialog.askstring("Новый класс","Название класса:",
                                      parent=self.frame.winfo_toplevel())
        if not name: return
        self.cfg["classes"].append({
            "name": name,"emoji":"⚔","color":"#888888",
            "hp":100,"mp":50,"str":10,"agi":10,"int":10,"vit":10,
            "description":"Новый класс персонажа"
        })
        self._refresh_class_list()

    def _del_class(self):
        idx = self._sel_class
        if len(self.cfg["classes"]) <= 1:
            messagebox.showwarning("Удаление","Нельзя удалить последний класс.")
            return
        if messagebox.askyesno("Удалить","Удалить класс?",
                               parent=self.frame.winfo_toplevel()):
            self.cfg["classes"].pop(idx)
            self._sel_class = max(0, idx-1)
            self._refresh_class_list()
            if self.cfg["classes"]:
                self._class_listbox.selection_set(self._sel_class)
                self._load_class_editor(self._sel_class)


# ══════════════════════════════════════════════════════════════
# ВКЛАДКА: РЕДАКТОР HUD / UI
# ══════════════════════════════════════════════════════════════
class UIEditorTab:
    """Редактор HUD — полосы HP/MP/XP, скиллбар, миникарта, чат."""

    def __init__(self, notebook, project_root: Path):
        self.project_root = project_root
        self.cfg_path     = project_root / "assets" / "ui_config.json"
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="🎮 HUD/UI")
        self.cfg = self._load()
        self._vars = {}
        self._build_ui()

    def _load(self):
        import copy
        base = copy.deepcopy(DEFAULT_UI_CONFIG["hud"])
        if self.cfg_path.exists():
            try:
                full = json.loads(self.cfg_path.read_text(encoding="utf-8"))
                base.update(full.get("hud", {}))
            except Exception: pass
        return base

    def _save(self):
        full = {}
        if self.cfg_path.exists():
            try: full = json.loads(self.cfg_path.read_text(encoding="utf-8"))
            except Exception: pass
        hud = {}
        for k, var in self._vars.items():
            v = var.get()
            if k in ("skill_slots","chat_lines"):
                try: v = int(v)
                except: v = 4
            elif k in ("show_minimap","show_level","show_gold"):
                v = bool(v)
            hud[k] = v
        full["hud"] = hud
        self.cfg_path.write_text(json.dumps(full, ensure_ascii=False, indent=2), encoding="utf-8")
        messagebox.showinfo("Сохранено","HUD настройки сохранены!")

    def _build_ui(self):
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=6)
        hdr.pack(fill="x")
        tk.Label(hdr, text="🎮  РЕДАКТОР HUD / ИНТЕРФЕЙСА", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI",13,"bold"), padx=16).pack(side="left")
        btn(hdr, "💾 Сохранить", self._save, C["accent"], padx=14, pady=4).pack(side="right", padx=8)
        btn(hdr, "🔄 Обновить превью", self._draw_hud_preview, C["panel3"], padx=10, pady=4).pack(side="right", padx=4)

        body = tk.Frame(self.frame, bg=C["bg"])
        body.pack(fill="both", expand=True, padx=16, pady=12)
        left  = tk.Frame(body, bg=C["bg"]); left.pack(side="left",  fill="both", expand=True)
        right = tk.Frame(body, bg=C["bg"]); right.pack(side="right", fill="both", expand=True, padx=(16,0))

        # Цвета полос
        tk.Label(left, text="🎨 ЦВЕТА ПОЛОС", bg=C["bg"], fg=C["gold"],
                 font=("Segoe UI",10,"bold"), pady=6).pack(fill="x")
        for key, lbl_text in [("hp_bar_color","HP бар:"),("mp_bar_color","MP бар:"),("xp_bar_color","XP бар:")]:
            self._color_field(left, key, lbl_text)

        tk.Label(left, text="⚙ ПАРАМЕТРЫ", bg=C["bg"], fg=C["gold"],
                 font=("Segoe UI",10,"bold"), pady=12).pack(fill="x")
        for key, lbl_text in [("skill_slots","Слотов скиллов:"),("chat_lines","Строк чата:")]:
            fr = tk.Frame(left, bg=C["bg"]); fr.pack(fill="x", pady=2)
            tk.Label(fr, text=lbl_text, bg=C["bg"], fg=C["muted"],
                     font=("Segoe UI",9), width=18, anchor="w").pack(side="left")
            var = tk.StringVar(value=str(self.cfg.get(key,4)))
            self._vars[key] = var
            tk.Spinbox(fr, textvariable=var, from_=1, to=12, width=5,
                       bg=C["input_bg"], fg=C["text"], relief="flat",
                       buttonbackground=C["panel3"], insertbackground=C["gold"],
                       font=("Segoe UI",9)).pack(side="left")

        tk.Label(left, text="👁 ВИДИМОСТЬ", bg=C["bg"], fg=C["gold"],
                 font=("Segoe UI",10,"bold")).pack(fill="x", pady=(12,6))
        for key, lbl_text in [("show_minimap","Мини-карта"),("show_level","Уровень"),("show_gold","Золото")]:
            var = tk.BooleanVar(value=bool(self.cfg.get(key, True)))
            self._vars[key] = var
            tk.Checkbutton(left, text=lbl_text, variable=var,
                           bg=C["bg"], fg=C["text"], selectcolor=C["panel3"],
                           activebackground=C["bg"], font=("Segoe UI",9)).pack(anchor="w")

        # Превью HUD
        tk.Label(right, text="👁 ПРЕВЬЮ HUD", bg=C["bg"], fg=C["gold"],
                 font=("Segoe UI",10,"bold"), pady=6).pack(fill="x")
        self._hud_canvas = tk.Canvas(right, width=380, height=240, bg="#050410",
                                     highlightthickness=1, highlightbackground=C["border"])
        self._hud_canvas.pack(pady=4)
        self._draw_hud_preview()

    def _color_field(self, parent, key, label):
        fr = tk.Frame(parent, bg=C["bg"]); fr.pack(fill="x", pady=3)
        tk.Label(fr, text=label, bg=C["bg"], fg=C["muted"],
                 font=("Segoe UI",9), width=14, anchor="w").pack(side="left")
        var = tk.StringVar(value=str(self.cfg.get(key,"#ffffff")))
        self._vars[key] = var
        pv = tk.Label(fr, text="  ", bg=var.get(), width=3)
        pv.pack(side="left", padx=4)
        e = tk.Entry(fr, textvariable=var, bg=C["input_bg"], fg=C["text"],
                     relief="flat", font=("Segoe UI",9), width=10)
        e.pack(side="left")
        def pick(v=var, p=pv):
            from tkinter import colorchooser
            c = colorchooser.askcolor(color=v.get())[1]
            if c: v.set(c); p.config(bg=c)
        btn(fr, "🎨", pick, C["panel3"], padx=4, pady=2).pack(side="left", padx=2)

    def _draw_hud_preview(self):
        cv = self._hud_canvas; cv.delete("all")
        hp_c  = self._vars.get("hp_bar_color", tk.StringVar(value="#cc3232")).get()
        mp_c  = self._vars.get("mp_bar_color", tk.StringVar(value="#3264cc")).get()
        xp_c  = self._vars.get("xp_bar_color", tk.StringVar(value="#44cc44")).get()
        slots = int(self._vars.get("skill_slots", tk.StringVar(value="4")).get() or 4)

        # HP bar
        cv.create_rectangle(10, 10, 210, 28, fill="#1a1a2a", outline="#222")
        try: cv.create_rectangle(10, 10, 10+160, 28, fill=hp_c)
        except: cv.create_rectangle(10, 10, 170, 28, fill="#cc3232")
        cv.create_text(12, 19, text="❤ HP  160 / 200", fill="white", font=("Segoe UI",7), anchor="w")

        # MP bar
        cv.create_rectangle(10, 33, 210, 48, fill="#1a1a2a", outline="#222")
        try: cv.create_rectangle(10, 33, 10+90, 48, fill=mp_c)
        except: cv.create_rectangle(10, 33, 100, 48, fill="#3264cc")
        cv.create_text(12, 40, text="💧 MP  90 / 200", fill="white", font=("Segoe UI",7), anchor="w")

        # XP bar (bottom)
        cv.create_rectangle(10, 225, 370, 235, fill="#111120", outline="#222")
        try: cv.create_rectangle(10, 225, 10+220, 235, fill=xp_c)
        except: cv.create_rectangle(10, 225, 230, 235, fill="#44cc44")
        cv.create_text(190, 230, text="XP  2200 / 4000", fill="white", font=("Segoe UI",7))

        # Skill bar
        slot_w = 40
        sx = 380 - slots * (slot_w + 4) - 8
        for i in range(slots):
            x = sx + i*(slot_w+4)
            cv.create_rectangle(x, 185, x+slot_w, 185+slot_w, fill="#1a1540", outline="#4422aa", width=1)
            cv.create_text(x+slot_w//2, 205, text=str(i+1), fill="#6655aa", font=("Segoe UI",7))

        # Level badge
        cv.create_oval(215, 8, 243, 36, fill="#3a2a6a", outline=C["gold"], width=1)
        cv.create_text(229, 22, text="Lv 7", fill=C["gold"], font=("Segoe UI",7,"bold"))

        # Minimap
        show_mm = self._vars.get("show_minimap", tk.BooleanVar(value=True)).get()
        if show_mm:
            cv.create_rectangle(300, 8, 372, 80, fill="#0c0c20", outline="#3322aa", width=1)
            cv.create_text(336, 44, text="🗺", font=("Segoe UI",20))
            cv.create_text(336, 75, text="Мини-карта", fill="#443366", font=("Segoe UI",6))

        # Gold
        show_gold = self._vars.get("show_gold", tk.BooleanVar(value=True)).get()
        if show_gold:
            cv.create_text(260, 22, text="💰 1 250", fill=C["gold"], font=("Segoe UI",8,"bold"))

        # Chat
        chat = int(self._vars.get("chat_lines", tk.StringVar(value="5")).get() or 5)
        y0 = 90
        cv.create_rectangle(8, y0, 160, y0+chat*13+4, fill="#08060f", outline="#1a1530")
        for i in range(min(chat, 3)):
            texts = ["[Система] Вы вошли в мир","[Алар] Привет, путник!","[Грэй] Задание ждёт тебя"]
            cv.create_text(12, y0+4+i*13, text=texts[i % 3], fill="#7766aa",
                           font=("Segoe UI",6), anchor="w")
class LocationsTab:
    """
    Управление всеми сценами (локациями) проекта.
    Каждая сцена хранится в assets/scenes/<id>.json
    Редактор позволяет:
      • видеть список сцен
      • создавать новую сцену
      • удалять сцену
      • переключаться между сценами (загружает в MapTab)
      • сохранять текущую сцену
      • задавать свойства (имя, музыка, уровень, PvP, safe zone)
    """
    SCENES_DIR = "scenes"      # внутри assets/

    def __init__(self, notebook, project_root: Path, map_tab_getter):
        self.project_root  = project_root
        self.get_map_tab   = map_tab_getter   # callable → MapTab
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="🌍 Локации")
        self._scenes: list[dict] = []   # {id, name, file, ...}
        self._selected_id: str  = ""
        self._build_ui()
        self._load_scene_list()

    # ── Путь к assets/scenes/ ────────────────────────────────
    @property
    def scenes_dir(self) -> Path:
        d = self.project_root / "assets" / self.SCENES_DIR
        d.mkdir(parents=True, exist_ok=True)
        return d

    def _scene_path(self, sid: str) -> Path:
        return self.scenes_dir / f"{sid}.json"

    @staticmethod
    def _to_id(name: str) -> str:
        import re
        return re.sub(r"_+","_", re.sub(r"[^a-z0-9]","_", name.lower())).strip("_")

    # ── UI ───────────────────────────────────────────────────
    def _build_ui(self):
        pane = tk.PanedWindow(self.frame, orient="horizontal",
                              bg=C["bg"], sashwidth=4)
        pane.pack(fill="both", expand=True)

        # ── Левая панель: список сцен ─────────────────────────
        left = tk.Frame(pane, bg=C["panel"], width=260)
        pane.add(left, minsize=220)

        tk.Label(left, text="🌍 ЛОКАЦИИ", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI",10,"bold")).pack(fill="x",padx=10,pady=(10,4))

        # Список
        lf = tk.Frame(left, bg=C["panel"])
        lf.pack(fill="both", expand=True, padx=8, pady=4)

        sb = tk.Scrollbar(lf, bg=C["panel2"])
        sb.pack(side="right", fill="y")
        self._listbox = tk.Listbox(lf, bg=C["panel2"], fg=C["text"],
                                   selectbackground=C["accent"],
                                   font=("Segoe UI",10),
                                   relief="flat", bd=0, activestyle="none",
                                   yscrollcommand=sb.set)
        self._listbox.pack(fill="both", expand=True)
        sb.config(command=self._listbox.yview)
        self._listbox.bind("<<ListboxSelect>>", self._on_select)

        # Кнопки под списком
        bf = tk.Frame(left, bg=C["panel"])
        bf.pack(fill="x", padx=8, pady=6)
        btn(bf,"+ Новая",   self._new_scene,    C["green"],  padx=6,pady=3).pack(side="left",padx=2)
        btn(bf,"🗑 Удалить", self._delete_scene,  C["danger"], padx=6,pady=3).pack(side="left",padx=2)

        # ── Правая панель: свойства выбранной сцены ───────────
        right = tk.Frame(pane, bg=C["panel2"])
        pane.add(right, minsize=400)

        # Заголовок
        self._title_lbl = tk.Label(right, text="Выберите локацию",
                                   bg=C["accent"], fg="white",
                                   font=("Segoe UI",11,"bold"),
                                   anchor="w", padx=12, pady=8)
        self._title_lbl.pack(fill="x")

        # Поля свойств
        props = tk.Frame(right, bg=C["panel2"])
        props.pack(fill="x", padx=16, pady=12)
        props.columnconfigure(1, weight=1)

        def field(label, row, default=""):
            tk.Label(props, text=label, bg=C["panel2"], fg=C["muted"],
                     font=("Segoe UI",9)).grid(row=row,column=0,sticky="w",pady=4,padx=4)
            var = tk.StringVar(value=default)
            ent = tk.Entry(props, textvariable=var, bg=C["input_bg"], fg=C["text"],
                           font=("Consolas",10), insertbackground=C["text"],
                           relief="flat", bd=4)
            ent.grid(row=row, column=1, sticky="ew", pady=4, padx=4)
            return var

        self._v_id    = field("ID (файл):",    0)
        self._v_name  = field("Имя:",          1)
        self._v_music = field("Музыка:",        2)
        self._v_lvmin = field("Мин. уровень:", 3, "0")
        self._v_lvmax = field("Макс. уровень:",4, "999")

        row5 = tk.Frame(props, bg=C["panel2"])
        row5.grid(row=5, column=0, columnspan=2, sticky="w", pady=4)
        self._v_pvp  = tk.BooleanVar()
        self._v_safe = tk.BooleanVar()
        tk.Checkbutton(row5, text="PvP зона", variable=self._v_pvp,
                       bg=C["panel2"], fg=C["text"],
                       selectcolor=C["panel3"]).pack(side="left", padx=4)
        tk.Checkbutton(row5, text="Безопасная зона", variable=self._v_safe,
                       bg=C["panel2"], fg=C["text"],
                       selectcolor=C["panel3"]).pack(side="left", padx=4)

        # Статус
        self._status_var = tk.StringVar()
        tk.Label(right, textvariable=self._status_var, bg=C["panel2"],
                 fg=C["green"], font=("Segoe UI",9)).pack(fill="x",padx=16)

        # Кнопки действий
        act = tk.Frame(right, bg=C["panel2"])
        act.pack(fill="x", padx=16, pady=8)

        btn(act,"💾 Сохранить свойства", self._save_props,
            C["accent"], padx=10, pady=5).pack(side="left", padx=4)
        btn(act,"🗺 Редактировать карту", self._edit_in_map,
            C["panel3"], C["orange"], padx=10, pady=5).pack(side="left", padx=4)
        btn(act,"💾 Сохранить карту сцены", self._save_current_map,
            C["green"], padx=10, pady=5).pack(side="left", padx=4)

        # Инфо о файле
        self._info_var = tk.StringVar()
        tk.Label(right, textvariable=self._info_var, bg=C["panel2"],
                 fg=C["muted"], font=("Segoe UI",8),
                 wraplength=500, justify="left").pack(fill="x",padx=16,pady=4)

        # Цвет атмосферы
        sep(right).pack(fill="x", padx=16, pady=4)
        amb = tk.Frame(right, bg=C["panel2"])
        amb.pack(fill="x", padx=16)
        tk.Label(amb, text="Ambient RGB:", bg=C["panel2"], fg=C["muted"],
                 font=("Segoe UI",9)).pack(side="left")
        self._v_ar = tk.StringVar(value="30"); self._v_ag = tk.StringVar(value="20")
        self._v_ab = tk.StringVar(value="50")
        for v,lbl in ((self._v_ar,"R"),(self._v_ag,"G"),(self._v_ab,"B")):
            tk.Label(amb,text=lbl,bg=C["panel2"],fg=C["muted"],
                     font=("Segoe UI",8)).pack(side="left",padx=(8,1))
            tk.Entry(amb,textvariable=v,width=4,bg=C["input_bg"],fg=C["text"],
                     font=("Consolas",9),relief="flat",bd=3).pack(side="left")

    # ── Загрузка списка сцен ──────────────────────────────────
    def _load_scene_list(self):
        self._scenes.clear()
        self._listbox.delete(0,"end")

        # 1. Из game_config.json → location.zones
        cfg_path = self.project_root / "assets" / "game_config.json"
        zones = []
        if cfg_path.exists():
            try:
                cfg = json.loads(cfg_path.read_text(encoding="utf-8"))
                zones = cfg.get("location",{}).get("zones",[])
            except: pass

        seen = set()
        for zname in zones:
            sid = self._to_id(zname)
            if sid in seen: continue
            seen.add(sid)
            info = {"id":sid, "name":zname,
                    "file":str(self._scene_path(sid)),
                    "music":"", "lvmin":0, "lvmax":999,
                    "pvp":False, "safe":False,
                    "ar":30,"ag":20,"ab":50}
            # Читаем meta.json если есть
            meta_path = self.scenes_dir / f"{sid}.meta.json"
            if meta_path.exists():
                try:
                    m = json.loads(meta_path.read_text(encoding="utf-8"))
                    # Не берём "file" из meta.json — путь всегда вычисляется
                    # динамически из project_root (иначе сломается на другом компе)
                    m.pop("file", None)
                    info.update(m)
                except: pass
            self._scenes.append(info)

        # 2. Все *.json в scenes/ которых нет в списке
        for fp in sorted(self.scenes_dir.glob("*.json")):
            if fp.name.endswith(".meta.json"): continue
            sid = fp.stem
            if sid in seen: continue
            seen.add(sid)
            name = sid.replace("_"," ").title()
            self._scenes.append({"id":sid,"name":name,
                                  "file":str(fp),
                                  "music":"","lvmin":0,"lvmax":999,
                                  "pvp":False,"safe":False,
                                  "ar":30,"ag":20,"ab":50})

        # Заполняем listbox
        for sc in self._scenes:
            exists = Path(sc["file"]).exists()
            marker = "✅" if exists else "📋"
            self._listbox.insert("end", f"{marker} {sc['name']}")

        self._set_status(f"Сцен: {len(self._scenes)}")

    def _on_select(self, *_):
        sel = self._listbox.curselection()
        if not sel: return
        sc = self._scenes[sel[0]]
        self._selected_id = sc["id"]
        self._title_lbl.config(text=f"  🌍 {sc['name']}")
        self._v_id.set(sc["id"])
        self._v_name.set(sc["name"])
        self._v_music.set(sc.get("music",""))
        self._v_lvmin.set(str(sc.get("lvmin",0)))
        self._v_lvmax.set(str(sc.get("lvmax",999)))
        self._v_pvp.set(sc.get("pvp",False))
        self._v_safe.set(sc.get("safe",False))
        self._v_ar.set(str(sc.get("ar",30)))
        self._v_ag.set(str(sc.get("ag",20)))
        self._v_ab.set(str(sc.get("ab",50)))
        exists = Path(sc["file"]).exists()
        size = Path(sc["file"]).stat().st_size//1024 if exists else 0
        self._info_var.set(
            "Файл: " + sc["file"] + "\n" +
            ("✅ Существует (" + str(size) + "KB)" if exists else
             "📋 Файл не создан — создастся при сохранении карты")
        )

    # ── Новая сцена ───────────────────────────────────────────
    def _new_scene(self):
        name = simpledialog.askstring("Новая локация","Название локации:",
                                       parent=self.frame.winfo_toplevel())
        if not name: return
        sid = self._to_id(name)
        if any(s["id"]==sid for s in self._scenes):
            messagebox.showwarning("Дубликат",f"Сцена '{sid}' уже существует!")
            return
        sc = {"id":sid,"name":name,"file":str(self._scene_path(sid)),
              "music":"","lvmin":0,"lvmax":999,"pvp":False,"safe":False,
              "ar":30,"ag":20,"ab":50}
        self._scenes.append(sc)
        self._listbox.insert("end", f"📋 {name}")
        # Добавляем в game_config.json
        self._add_to_config(name)
        self._set_status(f"✅ Создана: {name}")

    def _add_to_config(self, zone_name: str):
        cfg_path = self.project_root / "assets" / "game_config.json"
        try:
            cfg = json.loads(cfg_path.read_text(encoding="utf-8"))
            zones = cfg.setdefault("location",{}).setdefault("zones",[])
            if zone_name not in zones:
                zones.append(zone_name)
            cfg_path.write_text(json.dumps(cfg,ensure_ascii=False,indent=2),encoding="utf-8")
        except Exception as e:
            print(f"[WARN] game_config update: {e}")

    # ── Удаление сцены ────────────────────────────────────────
    def _delete_scene(self):
        if not self._selected_id:
            return
        sc = next((s for s in self._scenes if s["id"] == self._selected_id), None)
        if not sc:
            return
        msg = f"Удалить сцену '{sc['name']}'?\nФайл карты НЕ удаляется с диска."
        if not messagebox.askyesno("Удалить", msg, parent=self.frame.winfo_toplevel()):
            return
        self._scenes = [s for s in self._scenes if s["id"] != self._selected_id]
        self._selected_id = ""
        self._load_scene_list()
        
    # ── Сохранить свойства ────────────────────────────────────
    def _save_props(self):
        if not self._selected_id: return
        sc = next((s for s in self._scenes if s["id"]==self._selected_id), None)
        if not sc: return
        sc["name"]  = self._v_name.get()
        sc["music"] = self._v_music.get()
        try: sc["lvmin"] = int(self._v_lvmin.get())
        except: pass
        try: sc["lvmax"] = int(self._v_lvmax.get())
        except: pass
        sc["pvp"]  = self._v_pvp.get()
        sc["safe"] = self._v_safe.get()
        try: sc["ar"]=int(self._v_ar.get()); sc["ag"]=int(self._v_ag.get()); sc["ab"]=int(self._v_ab.get())
        except: pass
        # Сохраняем meta.json — без "file", путь всегда вычисляется динамически
        meta_save = {k: v for k, v in sc.items() if k != "file"}
        meta_path = self.scenes_dir / f"{sc['id']}.meta.json"
        meta_path.write_text(json.dumps(meta_save,ensure_ascii=False,indent=2),encoding="utf-8")
        # Обновляем game_config
        self._add_to_config(sc["name"])
        self._load_scene_list()
        self._set_status(f"💾 Свойства сохранены: {sc['name']}")

    # ── Открыть сцену в редакторе карты ──────────────────────
    def _edit_in_map(self):
        if not self._selected_id: return
        sc = next((s for s in self._scenes if s["id"]==self._selected_id), None)
        if not sc: return
        mt = self.get_map_tab()
        if not mt: return
        fpath = Path(sc["file"])
        if fpath.exists():
            try:
                d = json.loads(fpath.read_text(encoding="utf-8"))
                mt.mapdata.from_dict(d)
                mt.mapdata.filepath = str(fpath)
                mt.modified = False
                mt._draw_full_map(); mt._update_minimap(); mt._update_stats()
                mt._update_status(f"📂 Сцена загружена: {sc['name']}")
                self._set_status(f"Редактируется: {sc['name']}")
            except Exception as e:
                messagebox.showerror("Ошибка", str(e))
        else:
            # Новая пустая сцена
            mt.mapdata.tiles = generate_default_map()
            mt.mapdata.registry.clear()
            mt.mapdata.filepath = str(fpath)
            mt.modified = True
            mt._draw_full_map(); mt._update_minimap(); mt._update_stats()
            mt._update_status(f"🆕 Новая сцена: {sc['name']}")
            self._set_status(f"Новая сцена: {sc['name']}")

    # ── Сохранить текущую карту как сцену ────────────────────
    def _save_current_map(self):
        if not self._selected_id: return
        sc = next((s for s in self._scenes if s["id"]==self._selected_id), None)
        if not sc: return
        mt = self.get_map_tab()
        if not mt: return
        fpath = Path(sc["file"])
        try:
            d = mt.mapdata.to_dict()
            d["name"] = sc["name"]
            # Записываем scene_id в metadata чтобы _sync_to_scenes знал имя файла
            d.setdefault("metadata", {})["scene_id"] = self._selected_id
            fpath.parent.mkdir(parents=True, exist_ok=True)
            fpath.write_text(json.dumps(d, ensure_ascii=False, indent=2), encoding="utf-8")
            mt.mapdata.filepath = str(fpath)
            mt.modified = False
            # Синхронизируем — scenes/ уже является целевой папкой,
            # но вызываем _sync_to_scenes для единообразия и вывода статуса
            try:
                mt._sync_to_scenes(d)
            except Exception:
                pass
            self._load_scene_list()
            n = len(mt.mapdata.registry.all())
            self._set_status(f"💾 Карта сохранена → {fpath.name}  ({n} сущностей)")
        except Exception as e:
            messagebox.showerror("Ошибка сохранения", str(e))

    def _set_status(self, msg: str):
        self._status_var.set(msg)
        


# ══════════════════════════════════════════════════════════════
# ТАБ: СИСТЕМА ПРЕФАБОВ (Prefab System)
# ══════════════════════════════════════════════════════════════

BUILTIN_PREFABS = {
    "enemies": [
        {"id":"pfb_goblin_basic",   "name":"Гоблин (рядовой)",   "category":"enemies","kind":"enemy", "type":"GOBLIN",          "emoji":"👺","props":{"level":1,"boss":False,"hp":45,  "dmg":7,  "speed":75, "gold":35, "patrol":False}},
        {"id":"pfb_goblin_elite",   "name":"Гоблин (элита)",      "category":"enemies","kind":"enemy", "type":"GOBLIN",          "emoji":"👺","props":{"level":5,"boss":False,"hp":120, "dmg":18, "speed":85, "gold":80, "patrol":True}},
        {"id":"pfb_goblin_boss",    "name":"Гоблин (босс)",       "category":"enemies","kind":"enemy", "type":"GOBLIN",          "emoji":"👺","props":{"level":8,"boss":True, "hp":400, "dmg":35, "speed":90, "gold":250,"patrol":True}},
        {"id":"pfb_wolf_basic",     "name":"Волк (рядовой)",      "category":"enemies","kind":"enemy", "type":"WOLF",            "emoji":"🐺","props":{"level":2,"boss":False,"hp":60,  "dmg":12, "speed":95, "gold":40, "patrol":True}},
        {"id":"pfb_wolf_alpha",     "name":"Волк (альфа)",        "category":"enemies","kind":"enemy", "type":"WOLF",            "emoji":"🐺","props":{"level":6,"boss":True, "hp":220, "dmg":28, "speed":100,"gold":150,"patrol":True}},
        {"id":"pfb_troll_basic",    "name":"Тролль",              "category":"enemies","kind":"enemy", "type":"TROLL",           "emoji":"🧌","props":{"level":4,"boss":False,"hp":180, "dmg":25, "speed":55, "gold":90, "patrol":False}},
        {"id":"pfb_skeleton_basic", "name":"Скелет",              "category":"enemies","kind":"enemy", "type":"SKELETON",        "emoji":"💀","props":{"level":3,"boss":False,"hp":70,  "dmg":14, "speed":65, "gold":55, "patrol":True}},
        {"id":"pfb_orc_warrior",    "name":"Орк-воин",            "category":"enemies","kind":"enemy", "type":"ORC",             "emoji":"👹","props":{"level":7,"boss":False,"hp":200, "dmg":30, "speed":60, "gold":110,"patrol":True}},
        {"id":"pfb_vampire_lord",   "name":"Лорд Вампиров",       "category":"enemies","kind":"enemy", "type":"VAMPIRE",         "emoji":"🧛","props":{"level":15,"boss":True,"hp":600, "dmg":55, "speed":85, "gold":400,"patrol":False}},
        {"id":"pfb_dragon_boss",    "name":"Дракон (финальный)",  "category":"enemies","kind":"enemy", "type":"DRAGON",          "emoji":"🐉","props":{"level":20,"boss":True,"hp":1500,"dmg":80, "speed":90, "gold":800,"patrol":False}},
    ],
    "npcs": [
        {"id":"pfb_npc_vendor",     "name":"Торговец",            "category":"npcs",   "kind":"npc",   "type":"VENDOR",          "emoji":"🛒","props":{"name":"Торговец","quest_id":None,"shop_level":1,"restock_time":3600}},
        {"id":"pfb_npc_quest",      "name":"Квестодатель",        "category":"npcs",   "kind":"npc",   "type":"QUEST",           "emoji":"❗","props":{"name":"Квестодатель","quest_id":"q001","repeatable":False}},
        {"id":"pfb_npc_healer",     "name":"Целитель",            "category":"npcs",   "kind":"npc",   "type":"HEALER",          "emoji":"⚕","props":{"name":"Целитель","quest_id":None,"heal_cost":10,"sell_potions":True}},
        {"id":"pfb_npc_guard",      "name":"Стражник",            "category":"npcs",   "kind":"npc",   "type":"GUARD",           "emoji":"💂","props":{"name":"Стражник","quest_id":None,"patrol":True,"aggro_on_crime":True}},
        {"id":"pfb_npc_blacksmith", "name":"Кузнец",              "category":"npcs",   "kind":"npc",   "type":"BLACKSMITH",      "emoji":"⚒","props":{"name":"Кузнец Борг","quest_id":None,"upgrade_level":3}},
        {"id":"pfb_npc_innkeeper",  "name":"Трактирщик",          "category":"npcs",   "kind":"npc",   "type":"INNKEEPER",       "emoji":"🏨","props":{"name":"Трактирщик","quest_id":None,"room_cost":10,"food_cost":5}},
        {"id":"pfb_npc_mage",       "name":"Маг-наставник",       "category":"npcs",   "kind":"npc",   "type":"MAGE_TRAINER",    "emoji":"🧙","props":{"name":"Архимаг","quest_id":None,"teaches_spells":True,"min_level":5}},
        {"id":"pfb_npc_banker",     "name":"Банкир",              "category":"npcs",   "kind":"npc",   "type":"BANKER",          "emoji":"💰","props":{"name":"Банкир","quest_id":None,"interest_rate":0.05}},
    ],
    "objects": [
        {"id":"pfb_chest_common",   "name":"Сундук (обычный)",    "category":"objects","kind":"object","type":"CHEST",           "emoji":"📦","props":{"loot_table":"common","gold_min":10, "gold_max":50,  "opened":False}},
        {"id":"pfb_chest_rare",     "name":"Сундук (редкий)",     "category":"objects","kind":"object","type":"CHEST",           "emoji":"📦","props":{"loot_table":"rare",  "gold_min":50, "gold_max":200, "opened":False}},
        {"id":"pfb_chest_epic",     "name":"Сундук (эпик)",       "category":"objects","kind":"object","type":"CHEST",           "emoji":"📦","props":{"loot_table":"epic",  "gold_min":200,"gold_max":800, "opened":False}},
        {"id":"pfb_portal_city",    "name":"Портал → Город",      "category":"objects","kind":"object","type":"PORTAL",          "emoji":"🌀","props":{"target_zone":"Aethoria City","target_x":60,"target_y":60,"color":"#aa44ff"}},
        {"id":"pfb_portal_dungeon", "name":"Портал → Данж",       "category":"objects","kind":"object","type":"PORTAL",          "emoji":"🌀","props":{"target_zone":"Goblin Caves","target_x":10,"target_y":10,"color":"#ff4422"}},
        {"id":"pfb_torch_orange",   "name":"Факел (оранжевый)",   "category":"objects","kind":"object","type":"TORCH",           "emoji":"🔥","props":{"light_radius":3,"color":"#ff8822"}},
        {"id":"pfb_torch_blue",     "name":"Факел (синий)",       "category":"objects","kind":"object","type":"TORCH",           "emoji":"🔥","props":{"light_radius":4,"color":"#4488ff"}},
        {"id":"pfb_sign_info",      "name":"Знак (инфо)",         "category":"objects","kind":"object","type":"SIGN",            "emoji":"🪧","props":{"text":"Добро пожаловать в Аэторию!"}},
        {"id":"pfb_altar_light",    "name":"Алтарь Света",        "category":"objects","kind":"object","type":"ALTAR",           "emoji":"🗽","props":{"buff":"regen","duration":60,"cooldown":300}},
        {"id":"pfb_boss_spawn",     "name":"Точка босса",         "category":"objects","kind":"object","type":"BOSS_SPAWN",      "emoji":"💀","props":{"boss_type":"DRAGON","respawn_time":300}},
        {"id":"pfb_dungeon_entry",  "name":"Вход в данж",         "category":"objects","kind":"object","type":"DUNGEON_ENTRANCE","emoji":"🚪","props":{"target_zone":"Goblin Caves","level_req":1}},
        {"id":"pfb_waypoint",       "name":"Путевая точка",       "category":"objects","kind":"object","type":"WAYPOINT",        "emoji":"📍","props":{"name":"Путевая точка","cost":0}},
        {"id":"pfb_resource_ore",   "name":"Руда",                "category":"objects","kind":"object","type":"RESOURCE",        "emoji":"💎","props":{"resource_type":"ore","respawn_time":120,"yield_amount":3}},
        {"id":"pfb_resource_herb",  "name":"Трава (лечебная)",    "category":"objects","kind":"object","type":"RESOURCE",        "emoji":"💎","props":{"resource_type":"herb","respawn_time":60,"yield_amount":2}},
    ],
    "zones": [
        {"id":"pfb_zone_spawn",     "name":"Зона спауна",         "category":"zones",  "kind":"zone",  "type":"SPAWN",           "emoji":"🔴","props":{"radius":3}},
        {"id":"pfb_zone_safe",      "name":"Безопасная зона",     "category":"zones",  "kind":"zone",  "type":"SAFE",            "emoji":"🟢","props":{"radius":18}},
        {"id":"pfb_zone_pvp",       "name":"PvP зона",            "category":"zones",  "kind":"zone",  "type":"PVP",             "emoji":"⚔","props":{"radius":15,"min_level":5}},
        {"id":"pfb_zone_boss",      "name":"Зона босса",          "category":"zones",  "kind":"zone",  "type":"BOSS",            "emoji":"👑","props":{"radius":10,"boss_id":"pfb_dragon_boss"}},
        {"id":"pfb_zone_event",     "name":"Ивент-зона",          "category":"zones",  "kind":"zone",  "type":"EVENT",           "emoji":"🎪","props":{"radius":8,"event_id":""}},
        {"id":"pfb_zone_dungeon",   "name":"Вход в данж (зона)",  "category":"zones",  "kind":"zone",  "type":"DUNGEON",         "emoji":"🏚","props":{"radius":5,"dungeon_id":"goblin_caves"}},
        {"id":"pfb_zone_town",      "name":"Город (зона)",        "category":"zones",  "kind":"zone",  "type":"TOWN",            "emoji":"🏙","props":{"radius":20,"town_name":"Аэтория"}},
    ],
    "custom": [],
}

PREFAB_CATEGORIES = {
    "enemies": {"name": "⚔ Враги",    "color": "#ff4455"},
    "npcs":    {"name": "👥 НПС",      "color": "#ffaa22"},
    "objects": {"name": "📦 Объекты",  "color": "#44aaff"},
    "zones":   {"name": "🗺 Зоны",    "color": "#44dd88"},
    "custom":  {"name": "⭐ Мои",      "color": "#cc88ff"},
}

PROP_FIELD_META = {
    "level":         ("int",   "Уровень",           1,   100),
    "boss":          ("bool",  "Босс",              None,None),
    "hp":            ("int",   "HP",                1,   99999),
    "dmg":           ("int",   "Урон",              0,   9999),
    "speed":         ("int",   "Скорость",          1,   200),
    "gold":          ("int",   "Золото (дроп)",     0,   9999),
    "patrol":        ("bool",  "Патрулирование",    None,None),
    "name":          ("str",   "Имя",               None,None),
    "quest_id":      ("str",   "ID квеста",         None,None),
    "shop_level":    ("int",   "Уровень магазина",  1,   10),
    "restock_time":  ("int",   "Пополнение (сек)",  60,  86400),
    "heal_cost":     ("int",   "Цена лечения",      0,   9999),
    "sell_potions":  ("bool",  "Продаёт зелья",     None,None),
    "aggro_on_crime":("bool",  "Агрит на преступление",None,None),
    "upgrade_level": ("int",   "Уровень апгрейда",  1,   10),
    "room_cost":     ("int",   "Стоимость ночлега", 0,   999),
    "food_cost":     ("int",   "Стоимость еды",     0,   999),
    "teaches_spells":("bool",  "Обучает заклинаниям",None,None),
    "min_level":     ("int",   "Мин. уровень",      1,   100),
    "interest_rate": ("float", "Процентная ставка", 0.0, 1.0),
    "repeatable":    ("bool",  "Повторяемый",       None,None),
    "loot_table":    ("str",   "Таблица лута",      None,None),
    "gold_min":      ("int",   "Золото (мин)",      0,   99999),
    "gold_max":      ("int",   "Золото (макс)",     0,   99999),
    "opened":        ("bool",  "Открыт",            None,None),
    "target_zone":   ("str",   "Целевая зона",      None,None),
    "target_x":      ("int",   "Цель X",            0,   9999),
    "target_y":      ("int",   "Цель Y",            0,   9999),
    "color":         ("color", "Цвет",              None,None),
    "light_radius":  ("int",   "Радиус света",      1,   20),
    "text":          ("str",   "Текст",             None,None),
    "buff":          ("str",   "Эффект",            None,None),
    "duration":      ("int",   "Длительность (с)",  1,   3600),
    "cooldown":      ("int",   "Откат (с)",         0,   86400),
    "boss_type":     ("str",   "Тип босса",         None,None),
    "respawn_time":  ("int",   "Респаун (с)",       10,  86400),
    "level_req":     ("int",   "Треб. уровень",     1,   100),
    "cost":          ("int",   "Стоимость (золото)",0,   9999),
    "resource_type": ("str",   "Тип ресурса",       None,None),
    "yield_amount":  ("int",   "Количество",        1,   99),
    "radius":        ("int",   "Радиус (тайлов)",   1,   100),
    "event_id":      ("str",   "ID ивента",         None,None),
    "dungeon_id":    ("str",   "ID данжа",          None,None),
    "boss_id":       ("str",   "ID босса-префаба",  None,None),
    "town_name":     ("str",   "Название города",   None,None),
}


def _try_set_bg(widget, color):
    try:
        widget.config(bg=color)
        for child in widget.winfo_children():
            _try_set_bg(child, color)
    except Exception:
        pass


class PrefabSystemTab:
    """Вкладка «⭐ Префабы» — каталог шаблонов объектов для размещения на карте."""

    def __init__(self, notebook, project_root, get_map_tab=None):
        self.project_root = project_root
        self.get_map_tab  = get_map_tab
        self.pfb_path     = project_root / "assets" / "prefabs.json"

        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="⭐ Префабы")

        self._prefabs = {cat: list(items) for cat, items in BUILTIN_PREFABS.items()}
        self._load_custom()

        self._sel_cat  = "enemies"
        self._sel_pfb  = None
        self._edit_vars = {}
        self._filter_var = tk.StringVar()
        # trace добавляем ПОСЛЕ _build_ui чтобы _list_frame уже существовал
        self._filter_var.trace_add("write", lambda *a: self._refresh_list())

        self._build_ui()
        self._select_cat("enemies")   # вызываем только после полного построения UI

    # ── Данные ────────────────────────────────────────────────────
    def _load_custom(self):
        if not self.pfb_path.exists():
            return
        try:
            raw = json.loads(self.pfb_path.read_text(encoding="utf-8"))
            if isinstance(raw, list):
                raw = {"custom": raw}
            for cat, items in raw.items():
                existing_ids = {p["id"] for p in self._prefabs.get(cat, [])}
                for item in items:
                    if item.get("id") not in existing_ids:
                        self._prefabs.setdefault(cat, []).append(item)
        except Exception:
            pass

    def _save_custom(self):
        builtin_ids = {p["id"] for cat in BUILTIN_PREFABS.values() for p in cat}
        out = {}
        for cat, items in self._prefabs.items():
            user = [p for p in items if p.get("id") not in builtin_ids]
            if user:
                out[cat] = user
        self.pfb_path.write_text(json.dumps(out, ensure_ascii=False, indent=2), encoding="utf-8")

    def _is_builtin(self, pfb):
        bids = {p["id"] for cat in BUILTIN_PREFABS.values() for p in cat}
        return pfb.get("id") in bids

    def _gen_id(self):
        import time
        return f"pfb_custom_{int(time.time()*1000)%10**9}"

    # ── UI ────────────────────────────────────────────────────────
    def _build_ui(self):
        # Шапка
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=6)
        hdr.pack(fill="x")
        tk.Label(hdr, text="⭐  СИСТЕМА ПРЕФАБОВ", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI",13,"bold"), padx=16).pack(side="left")
        tk.Label(hdr, text="— шаблоны объектов для карты",
                 bg=C["panel"], fg=C["muted"], font=("Segoe UI",9)).pack(side="left")
        btn(hdr, "📥 Импорт",  self._import_prefabs, C["panel3"], padx=10, pady=4).pack(side="right", padx=2)
        btn(hdr, "📤 Экспорт", self._export_prefabs, C["panel3"], padx=10, pady=4).pack(side="right", padx=2)
        btn(hdr, "➕ Новый",   self._new_prefab, C["green"], "black", padx=12, pady=4).pack(side="right", padx=4)

        # Тело
        body = tk.Frame(self.frame, bg=C["bg"])
        body.pack(fill="both", expand=True)
        self._build_left(body)
        self._build_center(body)
        self._build_right(body)

        # Статус
        self._status_var = tk.StringVar(value="Выберите префаб")
        tk.Label(self.frame, textvariable=self._status_var, bg=C["panel2"],
                 fg=C["muted"], font=("Segoe UI",8), padx=12, anchor="w",
                 pady=3).pack(fill="x", side="bottom")

    def _build_left(self, parent):
        left = tk.Frame(parent, bg=C["panel"], width=165)
        left.pack(side="left", fill="y"); left.pack_propagate(False)

        tk.Label(left, text="Категории", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI",10,"bold"), pady=10).pack(fill="x", padx=8)

        self._cat_btns = {}
        for cid, ci in PREFAB_CATEGORIES.items():
            count = len(self._prefabs.get(cid, []))
            b = tk.Button(left, text=f"{ci['name']}  ({count})",
                          bg=C["panel2"], fg=C["text"],
                          font=("Segoe UI",9,"bold"), relief="flat", bd=0,
                          cursor="hand2", anchor="w", padx=12, pady=8,
                          command=lambda c=cid: self._select_cat(c))
            b.pack(fill="x", padx=4, pady=1)
            self._cat_btns[cid] = b

        sep(left).pack(fill="x", pady=8, padx=8)
        tk.Label(left, text="Всего:", bg=C["panel"], fg=C["muted"],
                 font=("Segoe UI",8)).pack(anchor="w", padx=12)
        self._total_lbl = tk.Label(left, text="0", bg=C["panel"],
                                    fg=C["gold2"], font=("Segoe UI",14,"bold"))
        self._total_lbl.pack(anchor="w", padx=16)
        tk.Label(left, text="Пользовательских:", bg=C["panel"], fg=C["muted"],
                 font=("Segoe UI",8), pady=4).pack(anchor="w", padx=12)
        self._custom_lbl = tk.Label(left, text="0", bg=C["panel"],
                                     fg=C["cyan"], font=("Segoe UI",12,"bold"))
        self._custom_lbl.pack(anchor="w", padx=16)
        self._update_stats()

    def _build_center(self, parent):
        center = tk.Frame(parent, bg=C["panel2"], width=290)
        center.pack(side="left", fill="y"); center.pack_propagate(False)

        # Поиск
        sf = tk.Frame(center, bg=C["panel2"], pady=6)
        sf.pack(fill="x", padx=8)
        tk.Label(sf, text="🔍", bg=C["panel2"], fg=C["muted"],
                 font=("Segoe UI",10)).pack(side="left")
        tk.Entry(sf, textvariable=self._filter_var, bg=C["input_bg"], fg=C["text"],
                 relief="flat", font=("Segoe UI",9),
                 insertbackground=C["gold"]).pack(side="left", fill="x", expand=True, padx=4)
        tk.Button(sf, text="✕", bg=C["panel2"], fg=C["muted"], relief="flat",
                  font=("Segoe UI",8), command=lambda: self._filter_var.set(""),
                  cursor="hand2").pack(side="left")

        # Скролл-список
        lf = tk.Frame(center, bg=C["panel2"])
        lf.pack(fill="both", expand=True, padx=4, pady=(0,4))
        self._list_canvas = tk.Canvas(lf, bg=C["panel2"], highlightthickness=0)
        vsb = ttk.Scrollbar(lf, orient="vertical", command=self._list_canvas.yview,
                            style="Dark.Vertical.TScrollbar")
        self._list_canvas.configure(yscrollcommand=vsb.set)
        vsb.pack(side="right", fill="y")
        self._list_canvas.pack(side="left", fill="both", expand=True)
        self._list_frame = tk.Frame(self._list_canvas, bg=C["panel2"])
        win = self._list_canvas.create_window((0,0), window=self._list_frame, anchor="nw")
        self._list_frame.bind("<Configure>",
            lambda e: self._list_canvas.configure(scrollregion=self._list_canvas.bbox("all")))
        self._list_canvas.bind("<Configure>",
            lambda e: self._list_canvas.itemconfig(win, width=e.width))
        self._list_canvas.bind("<MouseWheel>",
            lambda e: self._list_canvas.yview_scroll(-1*(e.delta//120), "units"))

        # Кнопки
        bf = tk.Frame(center, bg=C["panel2"], pady=4)
        bf.pack(fill="x", padx=4)
        btn(bf, "🗑 Удалить",     self._delete_selected,    C["danger"],  padx=8, pady=3).pack(side="right", padx=2)
        btn(bf, "📋 Дублировать", self._duplicate_selected, C["panel3"],  padx=8, pady=3).pack(side="right", padx=2)

    def _build_right(self, parent):
        right = tk.Frame(parent, bg=C["bg"])
        right.pack(side="left", fill="both", expand=True)

        # Редактор (скроллируемый)
        top = tk.Frame(right, bg=C["bg"])
        top.pack(fill="both", expand=True)
        self._ed_canvas = tk.Canvas(top, bg=C["bg"], highlightthickness=0)
        vsb2 = ttk.Scrollbar(top, orient="vertical", command=self._ed_canvas.yview,
                              style="Dark.Vertical.TScrollbar")
        self._ed_canvas.configure(yscrollcommand=vsb2.set)
        vsb2.pack(side="right", fill="y"); self._ed_canvas.pack(side="left", fill="both", expand=True)
        self._ed_frame = tk.Frame(self._ed_canvas, bg=C["bg"])
        ew = self._ed_canvas.create_window((0,0), window=self._ed_frame, anchor="nw")
        self._ed_frame.bind("<Configure>",
            lambda e: self._ed_canvas.configure(scrollregion=self._ed_canvas.bbox("all")))
        self._ed_canvas.bind("<Configure>",
            lambda e: self._ed_canvas.itemconfig(ew, width=e.width))

        # Панель размещения
        bot = tk.Frame(right, bg=C["panel"], pady=6)
        bot.pack(fill="x", side="bottom")
        tk.Label(bot, text="📌 РАЗМЕЩЕНИЕ:", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI",9,"bold"), padx=12).pack(side="left")
        btn(bot, "🗺 Поставить на карту", self._place_on_map,
            C["accent"], padx=16, pady=5).pack(side="left", padx=8)
        tk.Label(bot, text="Переключает карту в режим сущностей (N) с параметрами префаба.",
                 bg=C["panel"], fg=C["muted"], font=("Segoe UI",8), padx=4).pack(side="left")

        self._show_empty()

    # ── Список ────────────────────────────────────────────────────
    def _select_cat(self, cat):
        self._sel_cat = cat
        for cid, b in self._cat_btns.items():
            if cid == cat:
                b.config(bg=PREFAB_CATEGORIES[cid]["color"], fg="black")
            else:
                b.config(bg=C["panel2"], fg=C["text"])
        self._filter_var.set("")
        self._refresh_list()

    def _refresh_list(self):
        for w in self._list_frame.winfo_children():
            w.destroy()
        items = list(self._prefabs.get(self._sel_cat, []))
        flt = self._filter_var.get().lower().strip()
        if flt:
            items = [p for p in items if flt in p.get("name","").lower()
                     or flt in p.get("type","").lower()]
        if not items:
            tk.Label(self._list_frame, text="— список пуст —",
                     bg=C["panel2"], fg=C["muted"],
                     font=("Segoe UI",9), pady=20).pack()
        else:
            for pfb in items:
                self._make_item(pfb)
        self._update_cat_counts()

    def _make_item(self, pfb):
        is_sel = self._sel_pfb and self._sel_pfb.get("id") == pfb.get("id")
        is_builtin = self._is_builtin(pfb)
        bg0 = C["tab_sel"] if is_sel else C["panel2"]
        mark = "🔒" if is_builtin else "✏"
        mark_fg = C["muted"] if is_builtin else C["cyan"]
        cat_color = PREFAB_CATEGORIES.get(self._sel_cat, {}).get("color", C["text"])

        row = tk.Frame(self._list_frame, bg=bg0, cursor="hand2")
        row.pack(fill="x", padx=2, pady=1)

        tk.Label(row, text=pfb.get("emoji","?"), bg=bg0, fg=C["text"],
                 font=("Segoe UI",14), width=2, anchor="center").pack(side="left", padx=4)
        info = tk.Frame(row, bg=bg0)
        info.pack(side="left", fill="x", expand=True, pady=4)
        tk.Label(info, text=pfb.get("name","—"), bg=bg0, fg=C["text"],
                 font=("Segoe UI",9,"bold"), anchor="w").pack(fill="x")
        tk.Label(info, text=pfb.get("type",""), bg=bg0, fg=cat_color,
                 font=("Segoe UI",7), anchor="w").pack(fill="x")
        tk.Label(row, text=mark, bg=bg0, fg=mark_fg,
                 font=("Segoe UI",9), padx=6).pack(side="right")

        def click(e, p=pfb):
            self._sel_pfb = p
            self._refresh_list()
            self._load_editor(p)
            self._status_var.set(f"Выбран: {p.get('name')}  [{p.get('id')}]")

        for w in [row, info] + list(row.winfo_children()) + list(info.winfo_children()):
            try: w.bind("<Button-1>", click)
            except Exception: pass

        def enter(e, r=row, b=bg0, p=pfb):
            if not (self._sel_pfb and self._sel_pfb.get("id") == p.get("id")):
                _try_set_bg(r, C["panel3"])
        def leave(e, r=row, b=bg0, p=pfb):
            if not (self._sel_pfb and self._sel_pfb.get("id") == p.get("id")):
                _try_set_bg(r, C["panel2"])
        row.bind("<Enter>", enter); row.bind("<Leave>", leave)

    # ── Редактор ─────────────────────────────────────────────────
    def _show_empty(self):
        for w in self._ed_frame.winfo_children(): w.destroy()
        tk.Label(self._ed_frame,
                 text="⭐\n\nВыберите префаб из списка\nили создайте новый",
                 bg=C["bg"], fg=C["muted"], font=("Segoe UI",11),
                 justify="center").pack(expand=True, pady=60)

    def _load_editor(self, pfb):
        for w in self._ed_frame.winfo_children(): w.destroy()
        self._edit_vars = {}
        p = self._ed_frame
        is_builtin = self._is_builtin(pfb)
        state = "disabled" if is_builtin else "normal"
        cat_color = PREFAB_CATEGORIES.get(pfb.get("category",""), {}).get("color", C["text"])

        # Шапка
        hdr = tk.Frame(p, bg=C["panel"], pady=8)
        hdr.pack(fill="x")
        tk.Label(hdr, text=pfb.get("emoji","?"), bg=C["panel"], fg=C["text"],
                 font=("Segoe UI",22), padx=12).pack(side="left")
        tf = tk.Frame(hdr, bg=C["panel"]); tf.pack(side="left", fill="x", expand=True)
        tk.Label(tf, text=pfb.get("name",""), bg=C["panel"], fg=C["gold2"],
                 font=("Segoe UI",12,"bold"), anchor="w").pack(fill="x")
        tk.Label(tf, text=f"ID: {pfb.get('id','')}  |  kind: {pfb.get('kind','')}  |  type: {pfb.get('type','')}",
                 bg=C["panel"], fg=C["muted"], font=("Segoe UI",7), anchor="w").pack(fill="x")
        if is_builtin:
            tk.Label(hdr, text="🔒 встроенный", bg=C["panel"], fg=C["muted"],
                     font=("Segoe UI",8), padx=12).pack(side="right")
        else:
            btn(hdr, "💾 Сохранить", lambda p=pfb: self._save_prefab(p),
                C["accent"], padx=12, pady=4).pack(side="right", padx=6)

        sep(p).pack(fill="x", pady=4)

        # Базовые поля
        bf = tk.Frame(p, bg=C["bg"]); bf.pack(fill="x", padx=16, pady=4)
        for key, label, val in [("name","Название",pfb.get("name","")),
                                  ("emoji","Эмодзи",pfb.get("emoji","?")),
                                  ("type","Тип (TYPE)",pfb.get("type","")),
                                  ("kind","Kind",pfb.get("kind",""))]:
            row = tk.Frame(bf, bg=C["bg"]); row.pack(fill="x", pady=2)
            tk.Label(row, text=f"{label}:", bg=C["bg"], fg=C["muted"],
                     font=("Segoe UI",9), width=14, anchor="w").pack(side="left")
            var = tk.StringVar(value=str(val))
            self._edit_vars[key] = var
            tk.Entry(row, textvariable=var,
                     bg=C["input_bg"] if state=="normal" else C["panel2"],
                     fg=C["text"] if state=="normal" else C["muted"],
                     relief="flat", font=("Segoe UI",9),
                     state=state, disabledforeground=C["muted"],
                     disabledbackground=C["panel2"],
                     insertbackground=C["gold"]).pack(side="left", fill="x", expand=True, padx=4)

        sep(p).pack(fill="x", pady=4, padx=12)

        # Пропсы
        ph = tk.Frame(p, bg=C["bg"]); ph.pack(fill="x", padx=16)
        tk.Label(ph, text="⚙ ПАРАМЕТРЫ (props)", bg=C["bg"], fg=C["gold"],
                 font=("Segoe UI",10,"bold")).pack(side="left")
        if not is_builtin:
            btn(ph, "+ Поле", lambda p=pfb: self._add_prop_dialog(p),
                C["green"], "black", padx=8, pady=2).pack(side="right", padx=4)

        props = pfb.get("props", {})
        if not props:
            tk.Label(p, text="— нет параметров —", bg=C["bg"], fg=C["muted"],
                     font=("Segoe UI",9), pady=8).pack(anchor="w", padx=24)
        else:
            tbl = tk.Frame(p, bg=C["bg"]); tbl.pack(fill="x", padx=16, pady=4)
            for key, val in props.items():
                self._prop_row(tbl, pfb, key, val, is_builtin)

        sep(p).pack(fill="x", pady=8, padx=12)
        self._draw_preview(p, pfb)

    def _prop_row(self, parent, pfb, key, val, is_builtin):
        meta = PROP_FIELD_META.get(key, ("str", key, None, None))
        ftype, label, mn, mx = meta
        state = "disabled" if is_builtin else "normal"

        row = tk.Frame(parent, bg=C["bg"]); row.pack(fill="x", pady=2)
        tk.Label(row, text=f"{label}:", bg=C["bg"], fg=C["muted"],
                 font=("Segoe UI",9), width=22, anchor="w").pack(side="left")

        vkey = f"prop_{key}"
        if ftype == "bool":
            var = tk.BooleanVar(value=bool(val))
            self._edit_vars[vkey] = var
            tk.Checkbutton(row, variable=var, bg=C["bg"], fg=C["text"],
                           activebackground=C["bg"], selectcolor=C["panel2"],
                           state=state, disabledforeground=C["muted"]).pack(side="left")
        elif ftype == "int":
            var = tk.IntVar(value=int(val) if val is not None else 0)
            self._edit_vars[vkey] = var
            if not is_builtin and mn is not None:
                tk.Spinbox(row, textvariable=var, from_=mn, to=mx, width=8,
                           bg=C["input_bg"], fg=C["text"], relief="flat",
                           buttonbackground=C["panel3"],
                           insertbackground=C["gold"],
                           font=("Segoe UI",9)).pack(side="left", padx=4)
            else:
                tk.Entry(row, textvariable=var, width=8,
                         bg=C["panel2"], fg=C["muted"], relief="flat",
                         font=("Segoe UI",9), state=state,
                         disabledforeground=C["muted"],
                         disabledbackground=C["panel2"]).pack(side="left", padx=4)
        elif ftype == "float":
            var = tk.DoubleVar(value=float(val) if val is not None else 0.0)
            self._edit_vars[vkey] = var
            tk.Entry(row, textvariable=var, width=8,
                     bg=C["input_bg"] if not is_builtin else C["panel2"],
                     fg=C["text"] if not is_builtin else C["muted"],
                     relief="flat", font=("Segoe UI",9), state=state,
                     disabledforeground=C["muted"],
                     disabledbackground=C["panel2"]).pack(side="left", padx=4)
        elif ftype == "color":
            var = tk.StringVar(value=str(val) if val else "#ffffff")
            self._edit_vars[vkey] = var
            fr2 = tk.Frame(row, bg=C["bg"]); fr2.pack(side="left")
            try:
                sw = tk.Label(fr2, text="  ", bg=str(val) if val else "#888888",
                               relief="flat", width=3)
                sw.pack(side="left", padx=2)
            except Exception:
                sw = tk.Label(fr2, text="  ", bg="#888888", relief="flat", width=3)
                sw.pack(side="left", padx=2)
            tk.Entry(fr2, textvariable=var, bg=C["input_bg"] if not is_builtin else C["panel2"],
                     fg=C["text"], relief="flat", font=("Segoe UI",9), width=9,
                     state=state).pack(side="left", padx=2)
            if not is_builtin:
                def pick(v=var, s=sw):
                    from tkinter import colorchooser
                    c2 = colorchooser.askcolor(color=v.get())[1]
                    if c2:
                        v.set(c2)
                        try: s.config(bg=c2)
                        except Exception: pass
                btn(fr2, "🎨", pick, C["panel3"], padx=4, pady=2).pack(side="left")
        else:
            var = tk.StringVar(value=str(val) if val is not None else "")
            self._edit_vars[vkey] = var
            tk.Entry(row, textvariable=var,
                     bg=C["input_bg"] if not is_builtin else C["panel2"],
                     fg=C["text"] if not is_builtin else C["muted"],
                     relief="flat", font=("Segoe UI",9), state=state,
                     disabledforeground=C["muted"],
                     disabledbackground=C["panel2"]).pack(side="left", fill="x", expand=True, padx=4)

        if not is_builtin:
            btn(row, "✕", lambda k=key, p=pfb: self._remove_prop(p, k),
                C["danger"], padx=4, pady=1).pack(side="right", padx=2)

    def _draw_preview(self, parent, pfb):
        tk.Label(parent, text="👁 ПРЕВЬЮ", bg=C["bg"], fg=C["gold"],
                 font=("Segoe UI",10,"bold"), padx=16).pack(anchor="w", pady=(4,2))
        pv = tk.Canvas(parent, width=240, height=190, bg=C["panel"],
                       highlightthickness=1, highlightbackground=C["border"])
        pv.pack(padx=16, pady=4)
        kind  = pfb.get("kind","")
        etype = pfb.get("type","")
        em    = pfb.get("emoji","?")
        props = pfb.get("props",{})
        bg_map = {"enemy":"#1a0808","npc":"#0a1a0a","object":"#0a0a1a","zone":"#0a1a1a"}
        pv.config(bg=bg_map.get(kind, C["panel"]))
        # Цвет типа
        mc = "#8866cc"
        if kind=="enemy"  and etype in ENEMIES:  mc = ENEMIES[etype]["color"]
        elif kind=="npc"  and etype in NPCS:     mc = NPCS[etype]["color"]
        elif kind=="object" and etype in OBJECTS: mc = OBJECTS[etype]["color"]
        elif kind=="zone"  and etype in ZONES:   mc = ZONES[etype]["color"]
        try:
            if kind == "enemy":
                pv.create_oval(85,18,145,78, fill=mc, outline="#fff", width=2)
                pv.create_oval(70,73,160,148, fill=mc, outline="#fff", width=2)
                pv.create_text(115,48, text=em, font=("Segoe UI",28))
                hp = props.get("hp",100)
                hw = min(190, max(4, int(hp/10)))
                pv.create_rectangle(20,155,220,167, fill=C["panel3"], outline="")
                pv.create_rectangle(20,155,20+hw,167, fill=C["red"], outline="")
                pv.create_text(120,161, text=f"HP: {hp}", fill="white", font=("Segoe UI",7,"bold"))
                lvl = props.get("level",1)
                pv.create_text(120,180, text=f"Lv.{lvl}  dmg:{props.get('dmg',0)}  spd:{props.get('speed',0)}",
                               fill=mc, font=("Segoe UI",7))
            elif kind == "npc":
                pv.create_oval(85,12,145,72, fill=mc, outline=C["gold"], width=2)
                pv.create_oval(75,68,155,148, fill=mc, outline=C["gold"], width=2)
                pv.create_text(115,42, text=em, font=("Segoe UI",26))
                pv.create_text(120,165, text=props.get("name", pfb.get("name","")),
                               fill=C["gold"], font=("Segoe UI",9,"bold"))
            elif kind == "object":
                cx,cy,r = 120,88,46
                import math as _m
                pts=[]
                for i in range(6):
                    a=_m.pi/2+i*_m.pi/3; pts+=[cx+r*_m.cos(a),cy+r*_m.sin(a)]
                pv.create_polygon(pts, fill=mc, outline="#fff", width=2)
                pv.create_text(cx,cy, text=em, font=("Segoe UI",26))
            elif kind == "zone":
                import math as _m
                r = props.get("radius",10)
                dr = min(75, max(18, r*3))
                pv.create_oval(120-dr,95-dr,120+dr,95+dr,
                               outline=mc, width=3, dash=(4,3))
                pv.create_text(120,95, text=em, font=("Segoe UI",20))
                pv.create_text(120,158, text=f"R: {r} тайлов", fill=mc, font=("Segoe UI",8))
            else:
                pv.create_text(120,90, text=em, font=("Segoe UI",40))
        except Exception:
            pv.create_text(120,90, text=em, font=("Segoe UI",40))
        cat_color = PREFAB_CATEGORIES.get(pfb.get("category",""), {}).get("color", C["muted"])
        pv.create_text(120,182, text=f"[{etype}]  kind:{kind}",
                       fill=cat_color, font=("Segoe UI",7))

    # ── CRUD ─────────────────────────────────────────────────────
    def _save_prefab(self, pfb):
        pfb["name"]  = self._edit_vars.get("name",  tk.StringVar()).get()
        pfb["emoji"] = self._edit_vars.get("emoji", tk.StringVar()).get()
        pfb["type"]  = self._edit_vars.get("type",  tk.StringVar()).get()
        pfb["kind"]  = self._edit_vars.get("kind",  tk.StringVar()).get()
        for vk, var in self._edit_vars.items():
            if vk.startswith("prop_"):
                key = vk[5:]
                try: pfb.setdefault("props",{})[key] = var.get()
                except Exception: pass
        self._save_custom()
        self._refresh_list()
        self._load_editor(pfb)
        self._update_stats()
        self._status_var.set(f"✅ Сохранён: {pfb.get('name')}")

    def _new_prefab(self):
        name = simpledialog.askstring("Новый префаб", "Название:",
                                      parent=self.frame.winfo_toplevel())
        if not name: return
        pfb = {"id":self._gen_id(),"name":name,"category":"custom",
               "kind":"object","type":"CHEST","emoji":"📦",
               "props":{"loot_table":"common","gold_min":10,"gold_max":50,"opened":False}}
        self._prefabs["custom"].append(pfb)
        self._save_custom()
        self._select_cat("custom")
        self._sel_pfb = pfb
        self._refresh_list()
        self._load_editor(pfb)
        self._update_stats()
        self._status_var.set(f"➕ Создан: {name}")

    def _duplicate_selected(self):
        if not self._sel_pfb:
            messagebox.showwarning("Нет выбора", "Выберите префаб."); return
        new = copy.deepcopy(self._sel_pfb)
        new["id"] = self._gen_id()
        new["name"] = self._sel_pfb.get("name","") + " (копия)"
        new["category"] = "custom"
        self._prefabs["custom"].append(new)
        self._save_custom()
        self._select_cat("custom")
        self._sel_pfb = new
        self._refresh_list()
        self._load_editor(new)
        self._update_stats()
        self._status_var.set(f"📋 Дублирован → {new['name']}")

    def _delete_selected(self):
        if not self._sel_pfb:
            messagebox.showwarning("Нет выбора", "Выберите префаб."); return
        if self._is_builtin(self._sel_pfb):
            messagebox.showwarning("Встроенный",
                "Встроенные нельзя удалить.\nДублируйте и редактируйте копию."); return
        if not messagebox.askyesno("Удалить?", f"Удалить «{self._sel_pfb.get('name')}»?"): return
        cat = self._sel_pfb.get("category","custom")
        pid = self._sel_pfb.get("id")
        self._prefabs[cat] = [p for p in self._prefabs.get(cat,[]) if p.get("id")!=pid]
        self._save_custom()
        self._sel_pfb = None
        self._refresh_list()
        self._show_empty()
        self._update_stats()
        self._status_var.set("🗑 Удалён")

    def _add_prop_dialog(self, pfb):
        win = tk.Toplevel(self.frame.winfo_toplevel())
        win.title("Добавить поле"); win.geometry("340x210")
        win.configure(bg=C["panel"]); win.resizable(False,False); win.grab_set()
        tk.Label(win, text="Ключ поля:", bg=C["panel"], fg=C["text"],
                 font=("Segoe UI",9)).pack(pady=(16,2))
        kv = tk.StringVar()
        tk.Entry(win, textvariable=kv, bg=C["input_bg"], fg=C["text"],
                 relief="flat", font=("Segoe UI",10), width=28).pack()
        tk.Label(win, text="Значение:", bg=C["panel"], fg=C["text"],
                 font=("Segoe UI",9)).pack(pady=(12,2))
        vv = tk.StringVar()
        tk.Entry(win, textvariable=vv, bg=C["input_bg"], fg=C["text"],
                 relief="flat", font=("Segoe UI",10), width=28).pack()
        def do_add():
            k = kv.get().strip()
            if not k: messagebox.showwarning("Ошибка","Ключ пуст",parent=win); return
            vs = vv.get().strip()
            try: v = int(vs)
            except ValueError:
                try: v = float(vs)
                except ValueError:
                    if vs.lower() in ("true","false"): v = vs.lower()=="true"
                    else: v = vs
            pfb.setdefault("props",{})[k] = v
            self._save_custom(); self._load_editor(pfb); win.destroy()
            self._status_var.set(f"+ Поле '{k}' добавлено")
        btn(win, "✅ Добавить", do_add, C["green"], "black", padx=16, pady=6).pack(pady=12)

    def _remove_prop(self, pfb, key):
        if messagebox.askyesno("Удалить поле?", f"Удалить '{key}'?",
                               parent=self.frame.winfo_toplevel()):
            pfb.get("props",{}).pop(key,None)
            self._save_custom(); self._load_editor(pfb)
            self._status_var.set(f"✕ Поле '{key}' удалено")

    def _place_on_map(self):
        if not self._sel_pfb:
            messagebox.showwarning("Нет выбора","Выберите префаб."); return
        mt = self.get_map_tab() if self.get_map_tab else None
        if mt is None:
            messagebox.showwarning("Карта","Вкладка карты недоступна."); return
        pfb  = self._sel_pfb
        kind = pfb.get("kind","object")
        etype= pfb.get("type","")
        try:
            mt.tool = "entity"
            mt.entity_kind = kind
            mt.entity_type = etype
            mt._prefab_props = copy.deepcopy(pfb.get("props",{}))
            mt.placing_entity = (etype, kind)          # ← без этого клик не работал
            mt.canvas.config(cursor="plus")
            # Переключаемся на вкладку карты (ищем по тексту таба)
            nb = self.frame.master
            for i in range(nb.index("end")):
                if "арт" in nb.tab(i,"text").lower() or nb.tab(i,"text").strip().startswith("🗺"):
                    nb.select(i); break
            try: mt._update_toolbar_highlight()
            except Exception: pass
            self._status_var.set(
                f"📌 Режим: {pfb.get('name')} [{etype}] — кликайте на карту (инструмент N)")
        except Exception as ex:
            messagebox.showerror("Ошибка", str(ex))

    def _export_prefabs(self):
        path = filedialog.asksaveasfilename(title="Экспорт префабов",
            defaultextension=".json", filetypes=[("JSON","*.json")],
            initialfile="my_prefabs.json",
            parent=self.frame.winfo_toplevel())
        if not path: return
        bids = {p["id"] for cat in BUILTIN_PREFABS.values() for p in cat}
        out = {}
        for cat, items in self._prefabs.items():
            u = [p for p in items if p.get("id") not in bids]
            if u: out[cat] = u
        try:
            Path(path).write_text(json.dumps(out, ensure_ascii=False, indent=2), encoding="utf-8")
            self._status_var.set(f"📤 Экспортировано → {Path(path).name}")
            messagebox.showinfo("Готово", f"Сохранено: {path}")
        except Exception as ex:
            messagebox.showerror("Ошибка", str(ex))

    def _import_prefabs(self):
        path = filedialog.askopenfilename(title="Импорт префабов",
            filetypes=[("JSON","*.json")],
            parent=self.frame.winfo_toplevel())
        if not path: return
        try:
            raw = json.loads(Path(path).read_text(encoding="utf-8"))
            if isinstance(raw, list): raw = {"custom": raw}
            added = 0
            for cat, items in raw.items():
                cat = cat if cat in self._prefabs else "custom"
                eids = {p["id"] for p in self._prefabs.get(cat,[])}
                for item in items:
                    if item.get("id") not in eids:
                        item["category"] = cat
                        self._prefabs.setdefault(cat,[]).append(item)
                        added += 1
            self._save_custom(); self._refresh_list(); self._update_stats()
            self._status_var.set(f"📥 Импортировано: {added} префабов")
            messagebox.showinfo("Готово", f"Добавлено {added} префабов.")
        except Exception as ex:
            messagebox.showerror("Ошибка", str(ex))

    def _update_stats(self):
        total = sum(len(v) for v in self._prefabs.values())
        bids = {p["id"] for cat in BUILTIN_PREFABS.values() for p in cat}
        custom = sum(1 for items in self._prefabs.values()
                     for p in items if p.get("id") not in bids)
        self._total_lbl.config(text=str(total))
        self._custom_lbl.config(text=str(custom))

    def _update_cat_counts(self):
        for cid, b in self._cat_btns.items():
            ci = PREFAB_CATEGORIES[cid]
            b.config(text=f"{ci['name']}  ({len(self._prefabs.get(cid,[]))})")

class AethoriaEditor:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title(APP_TITLE)
        self.root.geometry("1700x1000")
        self.root.configure(bg=C["bg"])
        self.root.minsize(1200, 750)

        apply_styles(self.root)

        # Данные
        self.project_root = find_project_root()
        self.mapdata = MapData()

        # ── АВТОЗАГРУЗКА map.json при старте ──────────────────
        _map_path = self.project_root / "assets" / "map.json"
        if _map_path.exists():
            try:
                d = json.loads(_map_path.read_text(encoding="utf-8"))
                self.mapdata.from_dict(d)
                self.mapdata.filepath = str(_map_path)
                print(f"[OK] map.json загружен: {_map_path}")
            except Exception as _e:
                print(f"[WARNING] Не удалось загрузить map.json: {_e}")
        else:
            # Файла нет — фиксируем путь, первый Ctrl+S запишет туда
            self.mapdata.filepath = str(_map_path)

        # Заголовок
        self._build_titlebar()

        # Основной Notebook
        self.notebook = ttk.Notebook(self.root, style="Dark.TNotebook")
        self.notebook.pack(fill="both", expand=True, padx=0, pady=0)

        # Вкладки
        self.map_tab    = MapTab(self.notebook, self.mapdata, self.project_root)
        self.locations_tab = LocationsTab(self.notebook, self.project_root, lambda: self.map_tab)
        self.asset_tab  = AssetPipelineTab(self.notebook, self.project_root)
        self.quest_tab  = QuestEditorTab(self.notebook, self.project_root)
        self.dlg_tab    = DialogueEditorTab(self.notebook, self.project_root)
        self.config_tab = GameConfigTab(self.notebook, self.project_root)
        self.prefab_tab = PrefabSystemTab(self.notebook, self.project_root, lambda: self.map_tab)
        self.login_tab  = LoginScreenTab(self.notebook, self.project_root)
        self.char_tab   = CharacterSelectTab(self.notebook, self.project_root)
        self.hud_tab    = UIEditorTab(self.notebook, self.project_root)

        # ── MMORPG вкладки ─────────────────────────────────────
        if MMO_TABS:
            self.classes_tab = ClassesTab(self.notebook, self.project_root)
            self.items_tab   = ItemsTab(self.notebook, self.project_root)
            self.server_tab  = ServerTab(self.notebook, self.project_root)

        # Горячие клавиши
        self._bind_keys()

        # Статус бар
        self._build_statusbar()

        # Восстановить активную вкладку
        self._restore_state()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build_titlebar(self):
        bar = tk.Frame(self.root, bg=C["panel"], pady=0)
        bar.pack(fill="x")

        # Логотип
        tk.Label(bar, text="⚔", bg=C["panel"], fg=C["gold"],
                font=("Segoe UI",16), padx=8).pack(side="left")
        tk.Label(bar, text="Papaz & KuponaLoa", bg=C["panel"], fg=C["gold2"],
                font=("Segoe UI",12,"bold"), padx=2).pack(side="left")
        tk.Label(bar, text=f"Game Dev Studio v{VERSION}", bg=C["panel"], fg=C["muted"],
                font=("Segoe UI",9), padx=4).pack(side="left")

        # Инфо о проекте
        proj_text = str(self.project_root.name) if self.project_root else "Нет проекта"
        tk.Label(bar, text=f"📁 {proj_text}", bg=C["panel"], fg=C["cyan"],
                font=("Segoe UI",9), padx=20).pack(side="left")

        # Кнопки
        btn(bar,"❓ Помощь",self._show_help,padx=8,pady=4).pack(side="right",padx=4)
        btn(bar,"🔍 О редакторе",self._show_about,padx=8,pady=4).pack(side="right",padx=4)

    def _build_statusbar(self):
        sb = tk.Frame(self.root, bg=C["panel2"], pady=3)
        sb.pack(fill="x", side="bottom")
        self.status_var = tk.StringVar(value=f"Papaz & KuponaLoa Editor v{VERSION} готов | Проект: {self.project_root}")
        tk.Label(sb, textvariable=self.status_var, bg=C["panel2"], fg=C["muted"],
                font=("Segoe UI",8), padx=12, anchor="w").pack(side="left")

        caps = []
        if PILLOW: caps.append("✅ Pillow")
        else: caps.append("❌ Pillow")
        if CV2: caps.append("✅ OpenCV")
        else: caps.append("❌ OpenCV")
        # Check ffmpeg
        try:
            r = subprocess.run(["ffmpeg","-version"],capture_output=True,timeout=2)
            caps.append("✅ ffmpeg")
        except: caps.append("❌ ffmpeg")

        tk.Label(sb, text="  |  ".join(caps), bg=C["panel2"], fg=C["muted"],
                font=("Segoe UI",8), padx=12, anchor="e").pack(side="right")

    def _bind_keys(self):
        r = self.root
        r.bind("<Control-z>", lambda e: self.map_tab._undo())
        r.bind("<Control-y>", lambda e: self.map_tab._redo())
        r.bind("<Control-s>", lambda e: self._save_all())
        r.bind("<F1>",        lambda e: self._show_help())
        r.bind("<Control-1>", lambda e: self.notebook.select(0))
        r.bind("<Control-2>", lambda e: self.notebook.select(1))
        r.bind("<Control-3>", lambda e: self.notebook.select(2))
        r.bind("<Control-4>", lambda e: self.notebook.select(3))
        r.bind("<Control-5>", lambda e: self.notebook.select(4))
        r.bind("<Control-6>", lambda e: self.notebook.select(5))
        r.bind("<Control-7>", lambda e: self.notebook.select(6))
        r.bind("<Control-8>", lambda e: self.notebook.select(7))
        r.bind("<Control-9>", lambda e: self.notebook.select(6))  # Префабы

    # ── Персистентность состояния ──────────────────────────────
    @property
    def _state_path(self):
        return self.project_root / "assets" / ".editor_state.json"

    def _restore_state(self):
        """Восстанавливает активную вкладку и положение карты"""
        try:
            if self._state_path.exists():
                s = json.loads(self._state_path.read_text(encoding="utf-8"))
                tab = s.get("active_tab", 0)
                self.notebook.select(min(tab, 9))
        except Exception:
            pass

    def _save_state(self):
        """Сохраняет активную вкладку и прочее UI-состояние"""
        try:
            state = {
                "active_tab": self.notebook.index(self.notebook.select()),
            }
            self._state_path.write_text(
                json.dumps(state, indent=2), encoding="utf-8"
            )
        except Exception:
            pass

    def _on_close(self):
        self._save_state()
        self.root.destroy()

    def _save_all(self):
        """Ctrl+S — сохраняет всё"""
        try: self.map_tab._save()
        except: pass
        try: self.locations_tab._save_props()
        except: pass
        try: self.quest_tab._save_all()
        except: pass
        try: self.dlg_tab._save_all()
        except: pass
        try: self.config_tab.save()
        except: pass
        try: self.prefab_tab._save_custom()
        except: pass
        try: self.login_tab._save()
        except: pass
        try: self.char_tab._save()
        except: pass
        try: self.hud_tab._save()
        except: pass
        if MMO_TABS:
            try: self.classes_tab._save()
            except: pass
            try: self.items_tab._save()
            except: pass
            try: self.server_tab._save()
            except: pass
        self._save_state()
        self.status_var.set("💾 Всё сохранено!")
        self.root.after(3000, lambda: self.status_var.set(f"Papaz & KuponaLoa Editor v{VERSION} | {self.project_root}"))

    def _show_help(self):
        win = tk.Toplevel(self.root)
        win.title("Помощь — Papaz Editor v1.3")
        win.geometry("600x600")
        win.configure(bg=C["panel"])

        tk.Label(win, text="⚔ Papaz & KuponaLoa Editor v1.3 — Помощь", bg=C["panel"],
                fg=C["gold2"], font=("Segoe UI",13,"bold")).pack(pady=12)

        fr, txt = scrolled_text(win, height=25, width=70)
        fr.pack(fill="both", expand=True, padx=12, pady=4)
        help_text = """
🗺 ВКЛАДКА «КАРТА МИРА»
──────────────────────────────────────────────────
P — рисовать тайлы
E — ластик
F — заливка (flood fill)
V — выбрать объект
N — разместить сущность
I — пипетка (захватить тип тайла)
+ / - — приближение / отдаление
0 — показать всю карту
C — перейти к городу
Ctrl+Z / Ctrl+Y — отмена / повтор
ПКМ / СКМ — перемещение камеры
Колесо мыши — зум

🎬 ВКЛАДКА «АССЕТЫ» — PIPELINE АНИМАЦИЙ
──────────────────────────────────────────────────
• Нажми «Выбрать файл» и выбери MP4/GIF/PNG
• Редактор сам определит сущность и действие
  из имени файла (player_walk.mp4 → player / walk)
• Можно задать имя и действие вручную
• Установи размер кадра (64×64 рекомендуется)
• Установи FPS (12 для плавной анимации)
• Результат: спрайт-шит в assets/textures/sprites/
  и запись в assets/animations.json

ФОРМАТ animations.json:
{
  "player": {
    "walk": {
      "sheet": "player_walk.png",
      "frames": 8, "cols": 8,
      "width": 64, "height": 64,
      "fps": 12, "loop": true
    }
  }
}

⚔ ВКЛАДКА «КВЕСТЫ»
──────────────────────────────────────────────────
• Создавай квесты с полными свойствами
• Типы: kill/collect/escort/explore/deliver/talk
• Цели: убить X врагов, собрать X предметов и т.д.
• Награды: диапазон золота, XP, предметы
• Диалог: три фазы — предложение/прогресс/завершение
• Результат: assets/quests.json

💬 ВКЛАДКА «ДИАЛОГИ»
──────────────────────────────────────────────────
• Выбери персонажа из списка слева
• Для мобов: aggro/combat/death/idle
• Для НПС: greeting/trade/quest/warning/farewell
• Добавляй, редактируй, удаляй и перемещай реплики
• Используй {player} в тексте — подставится имя игрока
• Результат: assets/dialogues.json

ГОРЯЧИЕ КЛАВИШИ (глобальные):
──────────────────────────────────────────────────
Ctrl+S       — сохранить всё
Ctrl+1-4     — переключить вкладку
F1           — эта справка
"""
        txt.insert("1.0", help_text)
        txt.config(state="disabled")

    def _show_about(self):
        messagebox.showinfo("О редакторе",
            f"Papaz & KuponaLoa Game Dev Studio v{VERSION}\n\n"
            "Полный пайплайн разработки:\n"
            "• Редактор карты мира 120×120 тайлов\n"
            "• MP4/GIF/PNG → Sprite Sheet + JSON авто\n"
            "• Редактор квестов с диалогами и наградами\n"
            "• Редактор диалогов НПС и мобов\n\n"
            f"Движок: SFML 2.5+ C++17\n"
            f"Pillow: {'✅' if PILLOW else '❌'}  OpenCV: {'✅' if CV2 else '❌'}"
        )

    def run(self):
        self.root.mainloop()


# ══════════════════════════════════════════════════════════════
if __name__ == "__main__":
    app = AethoriaEditor()
    app.run()
