#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
aethoria_mmo_tabs.py — MMORPG вкладки для Papaz & KuponaLoa Editor
══════════════════════════════════════════════════════════════════
  ⚔  ClassesTab     — классы персонажей + деревья навыков
  📦  ItemsTab       — база предметов, редкости, таблицы лута
  🌐  ServerTab      — конфиг сервера, зоны, рейты, гильдии
══════════════════════════════════════════════════════════════════
Импортируй в aethoria_editor3.py:
    from aethoria_mmo_tabs import ClassesTab, ItemsTab, ServerTab
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import json, copy
from pathlib import Path

# ─── Цветовая схема (зеркало C из основного файла) ────────────
C = {
    "bg": "#080714", "panel": "#0e0c20", "panel2": "#13112a",
    "panel3": "#1a1740", "border": "#2d2260", "gold": "#f0c040",
    "gold2": "#ffe880", "text": "#e0d8ff", "muted": "#6655aa",
    "accent": "#8c3df5", "accent2": "#b270ff", "green": "#3dde7a",
    "red": "#ff4455", "blue": "#44aaff", "orange": "#ff9933",
    "cyan": "#33eeff", "pink": "#ff55cc", "canvas": "#05040f",
    "tab_bg": "#100e22", "tab_sel": "#1f1a45", "input_bg": "#12102a",
    "success": "#22cc66", "warning": "#ffaa22", "danger": "#ff3344",
}

def btn(parent, text, cmd=None, bg=None, fg=None, **kw):
    bg = bg or C["panel3"]; fg = fg or C["text"]
    b = tk.Button(parent, text=text, command=cmd, bg=bg, fg=fg,
                  relief="flat", cursor="hand2",
                  activebackground=C["accent"], activeforeground="white",
                  font=("Segoe UI", 9), **kw)
    return b

def lbl(parent, text, fg=None, font=None, **kw):
    return tk.Label(parent, text=text, bg=C["panel"], fg=fg or C["text"],
                    font=font or ("Segoe UI", 9), **kw)

def entry(parent, textvariable=None, width=20, **kw):
    return tk.Entry(parent, textvariable=textvariable, bg=C["input_bg"],
                    fg=C["text"], insertbackground=C["text"],
                    relief="flat", font=("Segoe UI", 9), width=width, **kw)

def scrolled_listbox(parent, height=12, width=24, **kw):
    fr = tk.Frame(parent, bg=C["panel"])
    sb = tk.Scrollbar(fr, orient="vertical")
    lb = tk.Listbox(fr, bg=C["panel2"], fg=C["text"], selectbackground=C["accent"],
                    font=("Segoe UI", 9), relief="flat", height=height, width=width,
                    yscrollcommand=sb.set, **kw)
    sb.config(command=lb.yview)
    lb.pack(side="left", fill="both", expand=True)
    sb.pack(side="right", fill="y")
    return fr, lb

def sep(parent):
    tk.Frame(parent, bg=C["border"], height=1).pack(fill="x", pady=4)


# ══════════════════════════════════════════════════════════════════
# ДЕФОЛТНЫЕ ДАННЫЕ КЛАССОВ
# ══════════════════════════════════════════════════════════════════
DEFAULT_CLASSES = {
    "WARRIOR": {
        "name": "Воин", "emoji": "⚔", "color": "#cc4422",
        "description": "Непробиваемый боец ближнего боя. Высокое HP и броня.",
        "base_stats": {"hp": 200, "mp": 60, "str": 18, "dex": 10, "int": 6, "vit": 16},
        "stat_per_level": {"hp": 22, "mp": 4, "str": 2.0, "dex": 0.8, "int": 0.3, "vit": 1.8},
        "skills": [
            {"id": "s_war_slash",   "name": "Разящий удар",   "icon": "⚔",  "type": "active",
             "mana": 10, "cd": 2.0, "damage_formula": "STR*2.5+50",    "description": "Мощный удар оружием по врагу."},
            {"id": "s_war_shield",  "name": "Щитовой блок",   "icon": "🛡",  "type": "active",
             "mana": 15, "cd": 8.0, "damage_formula": "0",              "description": "Блокирует 60% урона на 3 сек."},
            {"id": "s_war_charge",  "name": "Боевой клич",    "icon": "📢",  "type": "active",
             "mana": 20, "cd": 12.0,"damage_formula": "STR*1.5",        "description": "Оглушает врагов в радиусе 150."},
            {"id": "s_war_berserk", "name": "Берсерк",        "icon": "😡",  "type": "active",
             "mana": 30, "cd": 20.0,"damage_formula": "STR*4.0",        "description": "+50% урон, -20% броня на 6 сек."},
            {"id": "s_war_taunt",   "name": "Насмешка",       "icon": "👊",  "type": "active",
             "mana": 8,  "cd": 6.0, "damage_formula": "0",              "description": "Переключает aggro на себя."},
            {"id": "s_war_passive", "name": "Закалённость",   "icon": "💪",  "type": "passive",
             "mana": 0,  "cd": 0,   "damage_formula": "VIT*3",          "description": "+VIT*3 к максимальному HP."},
        ]
    },
    "MAGE": {
        "name": "Маг", "emoji": "🔮", "color": "#4466ff",
        "description": "Повелитель магических сил. Огромный урон, низкое HP.",
        "base_stats": {"hp": 100, "mp": 200, "str": 5, "dex": 10, "int": 22, "vit": 7},
        "stat_per_level": {"hp": 10, "mp": 20, "str": 0.3, "dex": 0.8, "int": 2.5, "vit": 0.7},
        "skills": [
            {"id": "s_mag_fireball", "name": "Огненный шар",  "icon": "🔥",  "type": "active",
             "mana": 20, "cd": 1.5, "damage_formula": "INT*3.5+80",    "description": "Взрывается в AoE радиусе 80."},
            {"id": "s_mag_ice",      "name": "Ледяные стрелы","icon": "❄",   "type": "active",
             "mana": 15, "cd": 1.0, "damage_formula": "INT*2.0+40",    "description": "3 стрелы, замедление цели."},
            {"id": "s_mag_teleport","name": "Телепортация",   "icon": "⚡",  "type": "active",
             "mana": 30, "cd": 10.0,"damage_formula": "0",              "description": "Мгновенно перемещается к курсору."},
            {"id": "s_mag_meteor",   "name": "Метеор",         "icon": "☄",   "type": "active",
             "mana": 60, "cd": 30.0,"damage_formula": "INT*8.0+300",   "description": "Огромный AoE, кратратер 200px."},
            {"id": "s_mag_shield",   "name": "Магический щит", "icon": "🔵",  "type": "active",
             "mana": 25, "cd": 15.0,"damage_formula": "INT*5",          "description": "Поглощает INT*5 урона."},
            {"id": "s_mag_passive",  "name": "Аркан. мощь",   "icon": "✨",  "type": "passive",
             "mana": 0,  "cd": 0,   "damage_formula": "INT*0.1",        "description": "Все заклинания +INT*10% урона."},
        ]
    },
    "ROGUE": {
        "name": "Плут", "emoji": "🗡", "color": "#aa6622",
        "description": "Скрытный убийца. Высокий криткал, скорость атаки.",
        "base_stats": {"hp": 130, "mp": 80, "str": 14, "dex": 20, "int": 8, "vit": 10},
        "stat_per_level": {"hp": 14, "mp": 7, "str": 1.3, "dex": 2.2, "int": 0.5, "vit": 1.0},
        "skills": [
            {"id": "s_rog_backstab", "name": "Удар в спину",  "icon": "🗡",  "type": "active",
             "mana": 15, "cd": 3.0, "damage_formula": "DEX*4.0+100",   "description": "x3 урон если позади цели."},
            {"id": "s_rog_stealth",  "name": "Незаметность",  "icon": "👻",  "type": "active",
             "mana": 20, "cd": 12.0,"damage_formula": "0",              "description": "Становится невидимым на 5 сек."},
            {"id": "s_rog_poison",   "name": "Яд",             "icon": "☠",   "type": "active",
             "mana": 12, "cd": 5.0, "damage_formula": "DEX*1.5",        "description": "Наносит яд: DEX*1.5 каждые 2 сек."},
            {"id": "s_rog_fan",      "name": "Веер клинков",   "icon": "💨",  "type": "active",
             "mana": 18, "cd": 6.0, "damage_formula": "DEX*2.5",        "description": "5 клинков конусом впереди."},
            {"id": "s_rog_smoke",    "name": "Дымовая шашка",  "icon": "💨",  "type": "active",
             "mana": 10, "cd": 8.0, "damage_formula": "0",              "description": "Ослепляет врагов в радиусе 100."},
            {"id": "s_rog_passive",  "name": "Смертельный удар","icon": "🎯", "type": "passive",
             "mana": 0,  "cd": 0,   "damage_formula": "DEX*0.05",       "description": "+DEX*5% шанс критического удара."},
        ]
    },
    "RANGER": {
        "name": "Рейнджер", "emoji": "🏹", "color": "#44aa44",
        "description": "Меткий лучник. Дальний бой, ловушки, следопыт.",
        "base_stats": {"hp": 140, "mp": 100, "str": 12, "dex": 18, "int": 10, "vit": 11},
        "stat_per_level": {"hp": 15, "mp": 9, "str": 1.0, "dex": 2.0, "int": 0.8, "vit": 1.1},
        "skills": [
            {"id": "s_ran_shot",     "name": "Меткий выстрел", "icon": "🏹",  "type": "active",
             "mana": 8,  "cd": 0.8, "damage_formula": "DEX*2.0+60",    "description": "Быстрый выстрел по одной цели."},
            {"id": "s_ran_multishot","name": "Залп",           "icon": "🎯",  "type": "active",
             "mana": 20, "cd": 4.0, "damage_formula": "DEX*1.5",        "description": "5 стрел веером."},
            {"id": "s_ran_trap",     "name": "Ловушка",        "icon": "🪤",  "type": "active",
             "mana": 15, "cd": 6.0, "damage_formula": "DEX*3.0",        "description": "Обездвиживает и наносит урон."},
            {"id": "s_ran_eagle",    "name": "Орлиный глаз",   "icon": "👁",   "type": "active",
             "mana": 10, "cd": 15.0,"damage_formula": "DEX*5.0",        "description": "Следующий выстрел x5 урона, мгновенный."},
            {"id": "s_ran_pet",      "name": "Питомец",        "icon": "🐺",  "type": "active",
             "mana": 25, "cd": 30.0,"damage_formula": "DEX*2.0",        "description": "Призывает волка-компаньона."},
            {"id": "s_ran_passive",  "name": "Лесная охота",   "icon": "🌲",  "type": "passive",
             "mana": 0,  "cd": 0,   "damage_formula": "DEX*0.03",       "description": "+30% дальность выстрела."},
        ]
    },
    "PALADIN": {
        "name": "Паладин", "emoji": "✝", "color": "#eecc44",
        "description": "Священный воитель. Исцеление + броня. Гибрид.",
        "base_stats": {"hp": 170, "mp": 120, "str": 15, "dex": 9, "int": 14, "vit": 14},
        "stat_per_level": {"hp": 18, "mp": 12, "str": 1.5, "dex": 0.7, "int": 1.4, "vit": 1.5},
        "skills": [
            {"id": "s_pal_holy",     "name": "Священный удар", "icon": "✝",   "type": "active",
             "mana": 12, "cd": 2.0, "damage_formula": "STR*2.0+INT*1.5","description": "Урон нежити +50%."},
            {"id": "s_pal_heal",     "name": "Исцеление",      "icon": "💚",  "type": "active",
             "mana": 25, "cd": 4.0, "damage_formula": "INT*4.0+80",    "description": "Восстанавливает HP себе или союзнику."},
            {"id": "s_pal_aura",     "name": "Аура защиты",    "icon": "🌟",  "type": "active",
             "mana": 20, "cd": 10.0,"damage_formula": "VIT*2.0",        "description": "+VIT*2 брони союзникам в радиусе 200."},
            {"id": "s_pal_smite",    "name": "Кара Небес",     "icon": "⚡",  "type": "active",
             "mana": 40, "cd": 20.0,"damage_formula": "INT*6.0+STR*2.0","description": "Молния с небес, AoE круговой."},
            {"id": "s_pal_res",      "name": "Воскрешение",    "icon": "🌅",  "type": "active",
             "mana": 80, "cd": 60.0,"damage_formula": "0",              "description": "Воскрешает павшего союзника."},
            {"id": "s_pal_passive",  "name": "Праведность",    "icon": "☀",   "type": "passive",
             "mana": 0,  "cd": 0,   "damage_formula": "INT*0.05",       "description": "+INT*5% к эффектам исцеления."},
        ]
    },
}

# ══════════════════════════════════════════════════════════════════
# ДЕФОЛТНЫЕ ДАННЫЕ ПРЕДМЕТОВ
# ══════════════════════════════════════════════════════════════════
RARITIES = {
    "Common":    {"color": "#aaaaaa", "drop_weight": 60},
    "Uncommon":  {"color": "#44cc44", "drop_weight": 25},
    "Rare":      {"color": "#4488ff", "drop_weight": 10},
    "Epic":      {"color": "#9944ff", "drop_weight": 4},
    "Legendary": {"color": "#ff8800", "drop_weight": 1},
}

ITEM_CATEGORIES = ["weapon", "armor", "accessory", "consumable", "material", "quest"]

DEFAULT_ITEMS = [
    # ── Оружие ──────────────────────────────────────────────────
    {"id":"i_wood_sword", "name":"Деревянный меч",   "category":"weapon",    "rarity":"Common",
     "icon":"🗡", "stats":{"dmg_min":5,"dmg_max":9,"spd":1.0},   "level_req":1,  "sell_price":5,   "buy_price":20,
     "description":"Грубое деревянное оружие.", "classes":["WARRIOR","PALADIN"]},
    {"id":"i_iron_sword",  "name":"Железный меч",    "category":"weapon",    "rarity":"Common",
     "icon":"⚔", "stats":{"dmg_min":14,"dmg_max":20,"spd":1.0},  "level_req":5,  "sell_price":30,  "buy_price":100,
     "description":"Надёжное железное оружие.", "classes":["WARRIOR","PALADIN"]},
    {"id":"i_staff_oak",   "name":"Дубовый посох",   "category":"weapon",    "rarity":"Common",
     "icon":"🪄", "stats":{"dmg_min":8,"dmg_max":12,"int_bonus":5},"level_req":1, "sell_price":8,   "buy_price":25,
     "description":"Посох начинающего мага.", "classes":["MAGE"]},
    {"id":"i_dagger",      "name":"Кинжал",          "category":"weapon",    "rarity":"Common",
     "icon":"🗡", "stats":{"dmg_min":8,"dmg_max":14,"spd":1.5,"crit":5},"level_req":1,"sell_price":20,"buy_price":70,
     "description":"Быстрый клинок для плута.", "classes":["ROGUE"]},
    {"id":"i_bow_wood",    "name":"Деревянный лук",  "category":"weapon",    "rarity":"Common",
     "icon":"🏹", "stats":{"dmg_min":10,"dmg_max":16,"range":400},"level_req":1, "sell_price":15,  "buy_price":55,
     "description":"Базовый лук рейнджера.", "classes":["RANGER"]},
    {"id":"i_legend_blade","name":"Клинок Вечности", "category":"weapon",    "rarity":"Legendary",
     "icon":"🌟", "stats":{"dmg_min":80,"dmg_max":120,"str_bonus":20,"all_stats":5},"level_req":40,"sell_price":5000,"buy_price":0,
     "description":"Оружие, выкованное самим Аэтором.", "classes":["WARRIOR","PALADIN"]},
    # ── Броня ──────────────────────────────────────────────────
    {"id":"i_leather_chest","name":"Кожаный нагрудник","category":"armor",  "rarity":"Common",
     "icon":"🧥", "stats":{"armor":8,"vit_bonus":2},              "level_req":1,  "sell_price":12,  "buy_price":45,
     "description":"Лёгкая кожаная броня.", "slot":"chest", "classes":["ROGUE","RANGER"]},
    {"id":"i_plate_chest",  "name":"Рыцарские доспехи","category":"armor",  "rarity":"Rare",
     "icon":"🛡", "stats":{"armor":35,"vit_bonus":10,"str_bonus":5},"level_req":20,"sell_price":200,"buy_price":800,
     "description":"Тяжёлая броня паладина.", "slot":"chest", "classes":["WARRIOR","PALADIN"]},
    {"id":"i_robe_mage",   "name":"Мантия мага",     "category":"armor",    "rarity":"Uncommon",
     "icon":"👘", "stats":{"armor":5,"int_bonus":12,"mp_bonus":50},"level_req":10,"sell_price":80,  "buy_price":300,
     "description":"Пропитана магическими рунами.", "slot":"chest", "classes":["MAGE"]},
    # ── Расходники ────────────────────────────────────────────
    {"id":"i_hp_small",    "name":"Малое зелье HP",  "category":"consumable","rarity":"Common",
     "icon":"🧪", "stats":{"heal_hp":50},                         "level_req":1,  "sell_price":5,   "buy_price":15,
     "description":"Восстанавливает 50 HP."},
    {"id":"i_hp_medium",   "name":"Среднее зелье HP","category":"consumable","rarity":"Common",
     "icon":"💊", "stats":{"heal_hp":150},                        "level_req":10, "sell_price":12,  "buy_price":40,
     "description":"Восстанавливает 150 HP."},
    {"id":"i_mp_potion",   "name":"Зелье маны",      "category":"consumable","rarity":"Common",
     "icon":"🔵", "stats":{"heal_mp":80},                         "level_req":1,  "sell_price":8,   "buy_price":25,
     "description":"Восстанавливает 80 MP."},
    # ── Материалы ────────────────────────────────────────────
    {"id":"i_goblin_ear",  "name":"Ухо гоблина",     "category":"material",  "rarity":"Common",
     "icon":"👂", "stats":{},                                     "level_req":0,  "sell_price":3,   "buy_price":0,
     "description":"Трофей с гоблина. Нужен для квестов."},
    {"id":"i_wolf_fang",   "name":"Волчий клык",     "category":"material",  "rarity":"Common",
     "icon":"🦷", "stats":{},                                     "level_req":0,  "sell_price":5,   "buy_price":0,
     "description":"Острый клык волка. Материал для крафта."},
    {"id":"i_dragon_scale","name":"Драконья чешуя",  "category":"material",  "rarity":"Epic",
     "icon":"🐉", "stats":{},                                     "level_req":0,  "sell_price":500, "buy_price":0,
     "description":"Невероятно прочная. Основа для эпических доспехов."},
]

# Таблицы лута для врагов
DEFAULT_LOOT_TABLES = {
    "GOBLIN":   [{"item_id":"i_goblin_ear","chance":70,"min":1,"max":2},
                 {"item_id":"i_hp_small","chance":30,"min":1,"max":1}],
    "WOLF":     [{"item_id":"i_wolf_fang","chance":60,"min":1,"max":2},
                 {"item_id":"i_hp_small","chance":25,"min":1,"max":1}],
    "TROLL":    [{"item_id":"i_iron_sword","chance":5,"min":1,"max":1},
                 {"item_id":"i_hp_medium","chance":40,"min":1,"max":2}],
    "DRAGON":   [{"item_id":"i_dragon_scale","chance":80,"min":1,"max":3},
                 {"item_id":"i_legend_blade","chance":2,"min":1,"max":1}],
}

# ══════════════════════════════════════════════════════════════════
# ДЕФОЛТНЫЙ КОНФИГ СЕРВЕРА
# ══════════════════════════════════════════════════════════════════
DEFAULT_SERVER_CONFIG = {
    "server": {
        "name": "AETHORIA Server",
        "host": "0.0.0.0",
        "port": 7777,
        "max_players": 500,
        "tick_rate": 20,
        "timeout_seconds": 60,
        "log_level": "INFO",
        "version": "1.0.0",
    },
    "rates": {
        "xp_rate": 1.0,
        "gold_rate": 1.0,
        "drop_rate": 1.0,
        "respawn_rate": 1.0,
        "craft_success_rate": 1.0,
    },
    "gameplay": {
        "max_level": 60,
        "starting_gold": 50,
        "starting_zone": "aethoria_city",
        "death_penalty_xp_pct": 5,
        "death_penalty_gold_pct": 0,
        "pvp_level_diff": 5,
        "party_max_size": 6,
        "guild_max_size": 100,
        "auction_fee_pct": 5,
        "trade_enabled": True,
        "pvp_enabled": True,
    },
    "zones": [
        {"id":"aethoria_city", "name":"Город Аэтория", "pvp":False, "safe":True,  "level_min":0,  "level_max":999, "max_players_per_zone":200},
        {"id":"dark_forest",   "name":"Тёмный лес",    "pvp":False, "safe":False, "level_min":1,  "level_max":15,  "max_players_per_zone":100},
        {"id":"goblin_caves",  "name":"Пещеры гоблинов","pvp":False,"safe":False, "level_min":5,  "level_max":20,  "max_players_per_zone":50},
        {"id":"pvp_arena",     "name":"Арена ПвП",      "pvp":True, "safe":False, "level_min":10, "level_max":999, "max_players_per_zone":100},
        {"id":"dragon_lair",   "name":"Логово дракона", "pvp":False,"safe":False, "level_min":40, "level_max":60,  "max_players_per_zone":30},
    ],
    "instances": [
        {"id":"dungeon_goblin","name":"Данж: Гоблинские пещеры","level_req":5,"level_max":15,
         "max_players":5,"cooldown_minutes":60,"boss":"GOBLIN_KING"},
        {"id":"dungeon_undead","name":"Данж: Склеп нежити",     "level_req":15,"level_max":30,
         "max_players":5,"cooldown_minutes":120,"boss":"LICH"},
        {"id":"dungeon_dragon","name":"Данж: Логово Дракона",   "level_req":40,"level_max":60,
         "max_players":10,"cooldown_minutes":360,"boss":"ANCIENT_DRAGON"},
    ],
    "economy": {
        "shop_restock_hours": 24,
        "bank_interest_rate": 0,
        "max_gold_per_slot": 999999,
        "mail_cost": 1,
        "repair_cost_pct": 10,
    },
}


# ══════════════════════════════════════════════════════════════════
# ВКЛ. 1: КЛАССЫ И НАВЫКИ
# ══════════════════════════════════════════════════════════════════
class ClassesTab:
    def __init__(self, notebook, project_root):
        self.project_root = Path(project_root)
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="⚔ Классы")

        self._classes = copy.deepcopy(DEFAULT_CLASSES)
        self._load()
        self._sel_class = list(self._classes.keys())[0]
        self._sel_skill_idx = None
        self._build_ui()

    # ── Сохранение / загрузка ──────────────────────────────────
    def _path(self): return self.project_root / "assets" / "classes.json"

    def _load(self):
        p = self._path()
        if p.exists():
            try:
                data = json.loads(p.read_text(encoding="utf-8"))
                self._classes = data
            except Exception: pass

    def _save(self):
        p = self._path(); p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(json.dumps(self._classes, ensure_ascii=False, indent=2), encoding="utf-8")
        # ── Синхронизируем базовые статы → ui_config.json (char_select) ──
        ui_path = self.project_root / "assets" / "ui_config.json"
        try:
            full = json.loads(ui_path.read_text(encoding="utf-8")) if ui_path.exists() else {}
            cs = full.setdefault("char_select", {})
            ui_classes = []
            for cid, cd in self._classes.items():
                bs = cd.get("base_stats", {})
                ui_classes.append({
                    "name":        cd.get("name", cid),
                    "emoji":       cd.get("emoji", "⚔"),
                    "color":       cd.get("color", "#888888"),
                    "description": cd.get("description", ""),
                    "hp":  int(bs.get("hp",  100)),
                    "mp":  int(bs.get("mp",   50)),
                    "str": int(bs.get("str",  10)),
                    "agi": int(bs.get("dex",  10)),   # dex → agi для ui_config
                    "int": int(bs.get("int",  10)),
                    "vit": int(bs.get("vit",  10)),
                })
            cs["classes"] = ui_classes
            ui_path.write_text(json.dumps(full, ensure_ascii=False, indent=2), encoding="utf-8")
        except Exception:
            pass
        self._status.set(f"💾 Сохранено → {p.name}  (ui_config.json обновлён)")

    # ── Построение UI ─────────────────────────────────────────
    def _build_ui(self):
        # Заголовок
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=6)
        hdr.pack(fill="x")
        tk.Label(hdr, text="⚔  Классы персонажей и деревья навыков",
                 bg=C["panel"], fg=C["gold2"], font=("Segoe UI", 12, "bold"), padx=16).pack(side="left")
        self._status = tk.StringVar(value="Готово")
        tk.Label(hdr, textvariable=self._status, bg=C["panel"], fg=C["green"],
                 font=("Segoe UI", 9), padx=16).pack(side="right")
        btn(hdr, "💾 Сохранить", self._save, C["accent"], "white", padx=12, pady=4).pack(side="right", padx=4)

        # Основной layout
        body = tk.Frame(self.frame, bg=C["bg"])
        body.pack(fill="both", expand=True, padx=8, pady=4)

        # ─ Левая панель: список классов ───────────────────────
        lf = tk.Frame(body, bg=C["panel"], width=200)
        lf.pack(side="left", fill="y", padx=(0,4))
        lf.pack_propagate(False)
        tk.Label(lf, text="Классы", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI", 10, "bold"), pady=8).pack()
        self._class_listframe, self._class_lb = scrolled_listbox(lf, height=20, width=22)
        self._class_listframe.pack(fill="both", expand=True, padx=4, pady=4)
        self._class_lb.bind("<<ListboxSelect>>", self._on_class_select)
        self._refresh_class_list()

        # ─ Центр: базовые характеристики ──────────────────────
        cf = tk.Frame(body, bg=C["panel"])
        cf.pack(side="left", fill="both", expand=True, padx=4)
        tk.Label(cf, text="Базовые характеристики", bg=C["panel"], fg=C["cyan"],
                 font=("Segoe UI", 10, "bold"), pady=6).pack()

        self._stat_frame = tk.Frame(cf, bg=C["panel"])
        self._stat_frame.pack(fill="x", padx=8)
        self._stat_vars = {}
        stats_order = ["hp","mp","str","dex","int","vit"]
        stat_names   = {"hp":"❤ HP","mp":"💙 MP","str":"💪 Сила","dex":"🏃 Ловкость","int":"🔮 Интел.","vit":"🛡 Выносл."}
        for i, s in enumerate(stats_order):
            row = tk.Frame(self._stat_frame, bg=C["panel"])
            row.pack(fill="x", pady=2)
            tk.Label(row, text=stat_names[s], bg=C["panel"], fg=C["text"],
                     font=("Segoe UI", 9), width=14, anchor="w").pack(side="left")
            v = tk.StringVar()
            self._stat_vars[s] = v
            entry(row, textvariable=v, width=8).pack(side="left", padx=4)
            # per-level
            tk.Label(row, text="+/ур:", bg=C["panel"], fg=C["muted"],
                     font=("Segoe UI", 8)).pack(side="left", padx=(8,2))
            lv = tk.StringVar()
            self._stat_vars[s+"_lvl"] = lv
            entry(row, textvariable=lv, width=6).pack(side="left")

        sep(cf)

        # Описание класса
        tk.Label(cf, text="Описание", bg=C["panel"], fg=C["cyan"],
                 font=("Segoe UI", 9, "bold")).pack(anchor="w", padx=8)
        self._desc_var = tk.StringVar()
        entry(cf, textvariable=self._desc_var, width=50).pack(fill="x", padx=8, pady=4)

        btn(cf, "✅ Применить характеристики", self._apply_stats,
            C["green"], "black", padx=10, pady=5).pack(pady=6)

        # ─ Правая панель: навыки ───────────────────────────────
        sf = tk.Frame(body, bg=C["panel"], width=380)
        sf.pack(side="right", fill="y", padx=(4,0))
        sf.pack_propagate(False)
        tk.Label(sf, text="Навыки", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI", 10, "bold"), pady=6).pack()

        self._skill_listframe, self._skill_lb = scrolled_listbox(sf, height=8, width=40)
        self._skill_listframe.pack(fill="x", padx=4)
        self._skill_lb.bind("<<ListboxSelect>>", self._on_skill_select)

        sep(sf)
        tk.Label(sf, text="Редактор навыка", bg=C["panel"], fg=C["cyan"],
                 font=("Segoe UI", 9, "bold")).pack(anchor="w", padx=8)

        self._skill_fields = {}
        skill_defs = [
            ("name","Название навыка","Огненный шар"),
            ("icon","Иконка (emoji)","🔥"),
            ("type","Тип (active/passive)","active"),
            ("mana","Стоимость маны","20"),
            ("cd","Кулдаун (сек)","3.0"),
            ("damage_formula","Формула урона","INT*3.5+80"),
            ("description","Описание","..."),
        ]
        for key, label, placeholder in skill_defs:
            row = tk.Frame(sf, bg=C["panel"])
            row.pack(fill="x", padx=8, pady=2)
            tk.Label(row, text=label+":", bg=C["panel"], fg=C["muted"],
                     font=("Segoe UI", 8), width=20, anchor="w").pack(side="left")
            v = tk.StringVar()
            self._skill_fields[key] = v
            entry(row, textvariable=v, width=24).pack(side="left")

        bf = tk.Frame(sf, bg=C["panel"])
        bf.pack(fill="x", padx=8, pady=6)
        btn(bf, "✅ Обновить", self._apply_skill, C["green"], "black", padx=8, pady=4).pack(side="left", padx=2)
        btn(bf, "➕ Новый",    self._new_skill,   C["blue"],  "white", padx=8, pady=4).pack(side="left", padx=2)
        btn(bf, "🗑 Удалить",  self._del_skill,   C["red"],   "white", padx=8, pady=4).pack(side="left", padx=2)

        self._refresh_all()

    def _refresh_class_list(self):
        self._class_lb.delete(0, "end")
        for cid, cd in self._classes.items():
            self._class_lb.insert("end", f"  {cd.get('emoji','')} {cd.get('name',cid)}")

    def _on_class_select(self, event=None):
        sel = self._class_lb.curselection()
        if not sel: return
        cids = list(self._classes.keys())
        if sel[0] < len(cids):
            self._sel_class = cids[sel[0]]
            self._sel_skill_idx = None
            self._refresh_all()

    def _refresh_all(self):
        cd = self._classes.get(self._sel_class, {})
        bs = cd.get("base_stats", {})
        bp = cd.get("stat_per_level", {})
        for s in ["hp","mp","str","dex","int","vit"]:
            self._stat_vars[s].set(str(bs.get(s, 0)))
            self._stat_vars[s+"_lvl"].set(str(bp.get(s, 0)))
        self._desc_var.set(cd.get("description",""))
        self._refresh_skill_list()

    def _refresh_skill_list(self):
        self._skill_lb.delete(0, "end")
        cd = self._classes.get(self._sel_class, {})
        for sk in cd.get("skills", []):
            t = "⚡" if sk.get("type") == "active" else "🔘"
            self._skill_lb.insert("end", f"  {t} {sk.get('icon','')} {sk.get('name','')}")

    def _on_skill_select(self, event=None):
        sel = self._skill_lb.curselection()
        if not sel: return
        skills = self._classes.get(self._sel_class,{}).get("skills",[])
        if sel[0] < len(skills):
            self._sel_skill_idx = sel[0]
            sk = skills[sel[0]]
            for key, var in self._skill_fields.items():
                var.set(str(sk.get(key,"")))

    def _apply_stats(self):
        cd = self._classes.setdefault(self._sel_class, {})
        bs = {}; bp = {}
        for s in ["hp","mp","str","dex","int","vit"]:
            try: bs[s] = float(self._stat_vars[s].get())
            except: bs[s] = 0
            try: bp[s] = float(self._stat_vars[s+"_lvl"].get())
            except: bp[s] = 0
        cd["base_stats"] = bs; cd["stat_per_level"] = bp
        cd["description"] = self._desc_var.get()
        self._status.set(f"✅ Характеристики {self._sel_class} обновлены")

    def _apply_skill(self):
        if self._sel_skill_idx is None: return
        skills = self._classes.get(self._sel_class,{}).get("skills",[])
        if self._sel_skill_idx >= len(skills): return
        sk = skills[self._sel_skill_idx]
        for key, var in self._skill_fields.items():
            v = var.get()
            if key in ("mana","cd"):
                try: v = float(v)
                except: pass
            sk[key] = v
        self._refresh_skill_list()
        self._status.set(f"✅ Навык обновлён")

    def _new_skill(self):
        sk = {"id": f"s_new_{len(self._classes.get(self._sel_class,{}).get('skills',[]))}",
              "name":"Новый навык","icon":"⭐","type":"active","mana":10,"cd":5.0,
              "damage_formula":"STR*2.0","description":"Описание навыка"}
        self._classes.setdefault(self._sel_class,{}).setdefault("skills",[]).append(sk)
        self._sel_skill_idx = len(self._classes[self._sel_class]["skills"]) - 1
        self._refresh_skill_list()
        self._skill_lb.selection_set(self._sel_skill_idx)
        for key, var in self._skill_fields.items():
            var.set(str(sk.get(key,"")))

    def _del_skill(self):
        if self._sel_skill_idx is None: return
        skills = self._classes.get(self._sel_class,{}).get("skills",[])
        if not skills: return
        if messagebox.askyesno("Удалить навык?", f"Удалить «{skills[self._sel_skill_idx].get('name')}»?"):
            skills.pop(self._sel_skill_idx)
            self._sel_skill_idx = None
            self._refresh_skill_list()


# ══════════════════════════════════════════════════════════════════
# ВКЛ. 2: ПРЕДМЕТЫ И ЛУТ
# ══════════════════════════════════════════════════════════════════
class ItemsTab:
    def __init__(self, notebook, project_root):
        self.project_root = Path(project_root)
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="📦 Предметы")

        self._items = copy.deepcopy(DEFAULT_ITEMS)
        self._loot  = copy.deepcopy(DEFAULT_LOOT_TABLES)
        self._load()
        self._sel_item = None
        self._sel_enemy = list(DEFAULT_LOOT_TABLES.keys())[0]
        self._build_ui()

    def _items_path(self): return self.project_root / "assets" / "items.json"
    def _loot_path(self):  return self.project_root / "assets" / "loot_tables.json"

    def _load(self):
        p = self._items_path()
        if p.exists():
            try: self._items = json.loads(p.read_text(encoding="utf-8"))
            except: pass
        p2 = self._loot_path()
        if p2.exists():
            try: self._loot = json.loads(p2.read_text(encoding="utf-8"))
            except: pass

    def _save(self):
        for p, data in [(self._items_path(), self._items),
                        (self._loot_path(),  self._loot)]:
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
        self._status.set("💾 items.json + loot_tables.json сохранены")

    def _build_ui(self):
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=6)
        hdr.pack(fill="x")
        tk.Label(hdr, text="📦  База предметов и таблицы лута",
                 bg=C["panel"], fg=C["gold2"], font=("Segoe UI", 12, "bold"), padx=16).pack(side="left")
        self._status = tk.StringVar(value="Готово")
        tk.Label(hdr, textvariable=self._status, bg=C["panel"], fg=C["green"],
                 font=("Segoe UI", 9), padx=16).pack(side="right")
        btn(hdr, "💾 Сохранить", self._save, C["accent"], "white", padx=12, pady=4).pack(side="right", padx=4)

        body = tk.Frame(self.frame, bg=C["bg"])
        body.pack(fill="both", expand=True, padx=8, pady=4)

        # ─ Список предметов ────────────────────────────────────
        lf = tk.Frame(body, bg=C["panel"], width=220)
        lf.pack(side="left", fill="y", padx=(0,4))
        lf.pack_propagate(False)

        # Фильтр по категории
        frow = tk.Frame(lf, bg=C["panel"])
        frow.pack(fill="x", padx=4, pady=4)
        tk.Label(frow, text="Фильтр:", bg=C["panel"], fg=C["muted"],
                 font=("Segoe UI", 8)).pack(side="left")
        self._filter_var = tk.StringVar(value="Все")
        cats_display = ["Все"] + ITEM_CATEGORIES
        cm = ttk.Combobox(frow, textvariable=self._filter_var, values=cats_display,
                          state="readonly", width=12)
        cm.pack(side="left", padx=4)
        cm.bind("<<ComboboxSelected>>", lambda e: self._refresh_item_list())

        self._item_listframe, self._item_lb = scrolled_listbox(lf, height=22, width=24)
        self._item_listframe.pack(fill="both", expand=True, padx=4, pady=2)
        self._item_lb.bind("<<ListboxSelect>>", self._on_item_select)

        bf = tk.Frame(lf, bg=C["panel"])
        bf.pack(fill="x", padx=4, pady=4)
        btn(bf, "➕ Новый",   self._new_item,  C["blue"],  "white", padx=6, pady=3).pack(side="left", padx=2)
        btn(bf, "🗑 Удалить", self._del_item,  C["red"],   "white", padx=6, pady=3).pack(side="left", padx=2)
        btn(bf, "📋 Копия",   self._dup_item,  C["panel3"],"white", padx=6, pady=3).pack(side="left", padx=2)

        # ─ Редактор предмета ───────────────────────────────────
        ef = tk.Frame(body, bg=C["panel"])
        ef.pack(side="left", fill="both", expand=True, padx=4)
        tk.Label(ef, text="Редактор предмета", bg=C["panel"], fg=C["cyan"],
                 font=("Segoe UI", 10, "bold"), pady=6).pack()

        self._item_vars = {}
        fields_top = [
            ("id",         "ID предмета",    "i_new_item"),
            ("name",       "Название",       "Новый предмет"),
            ("icon",       "Иконка (emoji)", "⭐"),
            ("category",   "Категория",      "weapon"),
            ("rarity",     "Редкость",       "Common"),
            ("level_req",  "Требуемый уровень","1"),
            ("sell_price", "Цена продажи",   "10"),
            ("buy_price",  "Цена покупки",   "50"),
            ("description","Описание",       "..."),
        ]
        for key, label, placeholder in fields_top:
            row = tk.Frame(ef, bg=C["panel"])
            row.pack(fill="x", padx=8, pady=2)
            tk.Label(row, text=label+":", bg=C["panel"], fg=C["muted"],
                     font=("Segoe UI", 8), width=22, anchor="w").pack(side="left")
            v = tk.StringVar()
            self._item_vars[key] = v
            if key in ("category","rarity"):
                vals = ITEM_CATEGORIES if key=="category" else list(RARITIES.keys())
                cb = ttk.Combobox(row, textvariable=v, values=vals, state="readonly", width=18)
                cb.pack(side="left")
            else:
                entry(row, textvariable=v, width=28).pack(side="left")

        sep(ef)
        tk.Label(ef, text="Характеристики (key:value через Enter)", bg=C["panel"],
                 fg=C["cyan"], font=("Segoe UI", 9, "bold")).pack(anchor="w", padx=8)
        self._stats_text = tk.Text(ef, bg=C["input_bg"], fg=C["text"], height=6,
                                   font=("Segoe UI", 9), relief="flat", insertbackground=C["text"])
        self._stats_text.pack(fill="x", padx=8, pady=4)

        btn(ef, "✅ Сохранить предмет", self._apply_item,
            C["green"], "black", padx=12, pady=5).pack(pady=4)

        # ─ Таблицы лута ────────────────────────────────────────
        sep(ef)
        tk.Label(ef, text="Таблица лута врага", bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI", 9, "bold")).pack(anchor="w", padx=8)
        lrow = tk.Frame(ef, bg=C["panel"])
        lrow.pack(fill="x", padx=8, pady=4)
        tk.Label(lrow, text="Враг:", bg=C["panel"], fg=C["muted"], font=("Segoe UI",8)).pack(side="left")
        self._loot_enemy_var = tk.StringVar(value=self._sel_enemy)
        enemy_names = list(DEFAULT_LOOT_TABLES.keys())
        ec = ttk.Combobox(lrow, textvariable=self._loot_enemy_var,
                          values=enemy_names, width=16)
        ec.pack(side="left", padx=4)
        ec.bind("<<ComboboxSelected>>", self._refresh_loot_view)
        btn(lrow, "➕ Добавить запись", self._add_loot_entry, C["blue"],"white", padx=6).pack(side="left",padx=4)

        self._loot_listframe, self._loot_lb = scrolled_listbox(ef, height=6, width=55)
        self._loot_listframe.pack(fill="x", padx=8, pady=2)

        btn(ef, "🗑 Удалить выбранную запись лута", self._del_loot_entry, C["red"],"white",
            padx=8, pady=3).pack(anchor="w", padx=8, pady=2)

        self._refresh_item_list()
        self._refresh_loot_view()

    def _refresh_item_list(self):
        self._item_lb.delete(0, "end")
        filt = self._filter_var.get()
        for item in self._items:
            if filt != "Все" and item.get("category") != filt: continue
            r = item.get("rarity","Common")
            color = RARITIES.get(r,{}).get("color","#aaaaaa")
            self._item_lb.insert("end", f"  {item.get('icon','')} {item.get('name','')}")

    def _on_item_select(self, event=None):
        sel = self._item_lb.curselection()
        if not sel: return
        filt = self._filter_var.get()
        visible = [i for i in self._items if filt == "Все" or i.get("category") == filt]
        if sel[0] < len(visible):
            self._sel_item = visible[sel[0]]
            self._load_item_to_form(self._sel_item)

    def _load_item_to_form(self, item):
        for key, var in self._item_vars.items():
            var.set(str(item.get(key,"")))
        self._stats_text.delete("1.0","end")
        for k, v in item.get("stats",{}).items():
            self._stats_text.insert("end", f"{k}:{v}\n")

    def _apply_item(self):
        if self._sel_item is None: return
        for key, var in self._item_vars.items():
            v = var.get()
            if key in ("level_req","sell_price","buy_price"):
                try: v = int(v)
                except: pass
            self._sel_item[key] = v
        # Parse stats
        stats = {}
        for line in self._stats_text.get("1.0","end").strip().split("\n"):
            if ":" in line:
                k, _, v = line.partition(":")
                k = k.strip(); v = v.strip()
                try: v = float(v) if "." in v else int(v)
                except: pass
                if k: stats[k] = v
        self._sel_item["stats"] = stats
        self._refresh_item_list()
        self._status.set(f"✅ {self._sel_item.get('name')} обновлён")

    def _new_item(self):
        item = {"id":f"i_new_{len(self._items)}","name":"Новый предмет","icon":"⭐",
                "category":"weapon","rarity":"Common","stats":{},"level_req":1,
                "sell_price":10,"buy_price":50,"description":""}
        self._items.append(item)
        self._sel_item = item
        self._refresh_item_list()
        self._load_item_to_form(item)

    def _del_item(self):
        if self._sel_item is None: return
        if messagebox.askyesno("Удалить?", f"Удалить «{self._sel_item.get('name')}»?"):
            self._items.remove(self._sel_item)
            self._sel_item = None
            self._refresh_item_list()

    def _dup_item(self):
        if self._sel_item is None: return
        new = copy.deepcopy(self._sel_item)
        new["id"] += "_copy"; new["name"] += " (копия)"
        self._items.append(new)
        self._sel_item = new
        self._refresh_item_list()
        self._load_item_to_form(new)

    def _refresh_loot_view(self, event=None):
        enemy = self._loot_enemy_var.get()
        self._sel_enemy = enemy
        self._loot_lb.delete(0,"end")
        for entry_data in self._loot.get(enemy, []):
            iid = entry_data.get("item_id","?")
            iname = next((i.get("name","?") for i in self._items if i.get("id")==iid), iid)
            self._loot_lb.insert("end",
                f"  {iname}  |  шанс:{entry_data.get('chance',0)}%  "
                f"|  кол-во:{entry_data.get('min',1)}-{entry_data.get('max',1)}")

    def _add_loot_entry(self):
        if not self._sel_item:
            messagebox.showwarning("Выбери предмет","Сначала выбери предмет из списка слева."); return
        enemy = self._loot_enemy_var.get()
        chance = simpledialog_int(self.frame, "Шанс выпадения (%)", "Процент (1-100):", 30)
        if chance is None: return
        count_min = simpledialog_int(self.frame, "Минимум", "Мин. количество:", 1)
        if count_min is None: return
        count_max = simpledialog_int(self.frame, "Максимум", "Макс. количество:", 1)
        if count_max is None: return
        self._loot.setdefault(enemy, []).append({
            "item_id": self._sel_item["id"],
            "chance": max(1, min(100, chance)),
            "min": count_min, "max": count_max
        })
        self._refresh_loot_view()

    def _del_loot_entry(self):
        sel = self._loot_lb.curselection()
        if not sel: return
        enemy = self._loot_enemy_var.get()
        entries = self._loot.get(enemy, [])
        if sel[0] < len(entries):
            entries.pop(sel[0])
            self._refresh_loot_view()


def simpledialog_int(parent, title, prompt, default=1):
    """Простой диалог ввода числа без зависимости от tkinter.simpledialog"""
    result = [None]
    win = tk.Toplevel(parent)
    win.title(title); win.geometry("280x120"); win.configure(bg=C["panel"]); win.grab_set()
    tk.Label(win, text=prompt, bg=C["panel"], fg=C["text"], font=("Segoe UI",9)).pack(pady=(16,4))
    v = tk.StringVar(value=str(default))
    e = entry(win, textvariable=v, width=16)
    e.pack(); e.focus()
    def ok():
        try: result[0] = int(v.get())
        except: result[0] = default
        win.destroy()
    btn(win, "ОК", ok, C["green"], "black", padx=12, pady=4).pack(pady=8)
    win.bind("<Return>", lambda _: ok())
    win.wait_window()
    return result[0]


# ══════════════════════════════════════════════════════════════════
# ВКЛ. 3: СЕРВЕР И MMO КОНФИГ
# ══════════════════════════════════════════════════════════════════
class ServerTab:
    def __init__(self, notebook, project_root):
        self.project_root = Path(project_root)
        self.frame = tk.Frame(notebook, bg=C["bg"])
        notebook.add(self.frame, text="🌐 Сервер")

        self._cfg = copy.deepcopy(DEFAULT_SERVER_CONFIG)
        self._load()
        self._build_ui()

    def _path(self): return self.project_root / "assets" / "server_config.json"

    def _load(self):
        p = self._path()
        if p.exists():
            try: self._cfg = json.loads(p.read_text(encoding="utf-8"))
            except: pass

    def _save(self):
        self._collect_all()
        p = self._path(); p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(json.dumps(self._cfg, ensure_ascii=False, indent=2), encoding="utf-8")
        self._status.set(f"💾 server_config.json сохранён")

    def _build_ui(self):
        hdr = tk.Frame(self.frame, bg=C["panel"], pady=6)
        hdr.pack(fill="x")
        tk.Label(hdr, text="🌐  Конфигурация MMORPG сервера",
                 bg=C["panel"], fg=C["gold2"], font=("Segoe UI", 12, "bold"), padx=16).pack(side="left")
        self._status = tk.StringVar(value="Готово")
        tk.Label(hdr, textvariable=self._status, bg=C["panel"], fg=C["green"],
                 font=("Segoe UI", 9), padx=16).pack(side="right")
        btn(hdr, "💾 Сохранить", self._save, C["accent"], "white", padx=12, pady=4).pack(side="right", padx=4)
        btn(hdr, "🔄 Сброс к дефолту", self._reset_defaults, C["danger"], "white", padx=10, pady=4).pack(side="right", padx=4)

        # Sub-notebook
        body = tk.Frame(self.frame, bg=C["bg"])
        body.pack(fill="both", expand=True)
        self._sub_nb = ttk.Notebook(body)
        self._sub_nb.pack(fill="both", expand=True, padx=8, pady=4)

        self._build_server_tab()
        self._build_rates_tab()
        self._build_gameplay_tab()
        self._build_zones_tab()
        self._build_instances_tab()
        self._build_economy_tab()

    # ─ Сервер ─────────────────────────────────────────────────
    def _build_server_tab(self):
        f = tk.Frame(self._sub_nb, bg=C["panel"])
        self._sub_nb.add(f, text="🖥 Сервер")
        self._server_vars = {}
        fields = [
            ("name",            "Имя сервера",        "str"),
            ("host",            "Хост (IP/0.0.0.0)",  "str"),
            ("port",            "Порт",               "int"),
            ("max_players",     "Макс. игроков",      "int"),
            ("tick_rate",       "Тик-рейт (Hz)",      "int"),
            ("timeout_seconds", "Таймаут (сек)",      "int"),
            ("log_level",       "Уровень лога",       "str"),
            ("version",         "Версия сервера",     "str"),
        ]
        self._build_fields_section(f, "Параметры сервера", fields, self._cfg["server"], self._server_vars)

    # ─ Рейты ──────────────────────────────────────────────────
    def _build_rates_tab(self):
        f = tk.Frame(self._sub_nb, bg=C["panel"])
        self._sub_nb.add(f, text="📈 Рейты")
        self._rates_vars = {}
        fields = [
            ("xp_rate",            "Рейт опыта (1.0=x1)",      "float"),
            ("gold_rate",          "Рейт золота",               "float"),
            ("drop_rate",          "Рейт лута",                 "float"),
            ("respawn_rate",       "Рейт респауна мобов",       "float"),
            ("craft_success_rate", "Успех крафта (0.0-1.0)",    "float"),
        ]
        self._build_fields_section(f, "Множители сервера (1.0 = стандарт)", fields,
                                   self._cfg["rates"], self._rates_vars)
        # Справка
        sep(f)
        help_text = ("Примеры:\n"
                     "  xp_rate=2.0  → все получают x2 опыта\n"
                     "  drop_rate=0.5 → лут выпадает в 2 раза реже\n"
                     "  Для «хардкор» режима: xp_rate=0.5, drop_rate=0.5")
        tk.Label(f, text=help_text, bg=C["panel"], fg=C["muted"],
                 font=("Segoe UI", 8), justify="left", padx=16).pack(anchor="w", pady=4)

    # ─ Геймплей ────────────────────────────────────────────────
    def _build_gameplay_tab(self):
        f = tk.Frame(self._sub_nb, bg=C["panel"])
        self._sub_nb.add(f, text="🎮 Геймплей")
        self._gameplay_vars = {}
        fields = [
            ("max_level",             "Максимальный уровень",     "int"),
            ("starting_gold",         "Начальное золото",         "int"),
            ("starting_zone",         "Стартовая зона (id)",      "str"),
            ("death_penalty_xp_pct",  "Штраф смерти XP (%)",      "int"),
            ("death_penalty_gold_pct","Штраф смерти золото (%)",   "int"),
            ("pvp_level_diff",        "Разброс уровней PvP",      "int"),
            ("party_max_size",        "Макс. размер группы",      "int"),
            ("guild_max_size",        "Макс. размер гильдии",     "int"),
            ("auction_fee_pct",       "Комиссия аукциона (%)",    "int"),
            ("trade_enabled",         "Торговля между игроками",  "bool"),
            ("pvp_enabled",           "PvP включено глобально",   "bool"),
        ]
        self._build_fields_section(f, "Правила геймплея", fields,
                                   self._cfg["gameplay"], self._gameplay_vars)

    # ─ Зоны ────────────────────────────────────────────────────
    def _build_zones_tab(self):
        f = tk.Frame(self._sub_nb, bg=C["panel"])
        self._sub_nb.add(f, text="🗺 Зоны")

        toprow = tk.Frame(f, bg=C["panel"])
        toprow.pack(fill="x", padx=8, pady=6)
        tk.Label(toprow, text="Зоны мира (уровни, PvP, Safe, лимиты)", bg=C["panel"],
                 fg=C["cyan"], font=("Segoe UI",10,"bold")).pack(side="left")
        btn(toprow, "➕ Добавить зону", self._add_zone, C["blue"],"white", padx=8).pack(side="right")
        btn(toprow, "🗑 Удалить", self._del_zone, C["red"],"white", padx=8).pack(side="right", padx=4)

        cols = ("id","name","pvp","safe","level_min","level_max","max_players")
        self._zones_tree = ttk.Treeview(f, columns=cols, show="headings", height=10)
        hdrs = {"id":"ID зоны","name":"Название","pvp":"PvP","safe":"Safe",
                "level_min":"Мин.ур","level_max":"Макс.ур","max_players":"Макс.игр"}
        widths = {"id":130,"name":180,"pvp":50,"safe":50,"level_min":70,"level_max":70,"max_players":80}
        for c in cols:
            self._zones_tree.heading(c, text=hdrs[c])
            self._zones_tree.column(c, width=widths[c], anchor="center")
        self._zones_tree.pack(fill="both", expand=True, padx=8, pady=4)
        self._refresh_zones()
        self._zones_tree.bind("<Double-1>", self._edit_zone)

    def _build_instances_tab(self):
        f = tk.Frame(self._sub_nb, bg=C["panel"])
        self._sub_nb.add(f, text="⚔ Инстанции")

        toprow = tk.Frame(f, bg=C["panel"])
        toprow.pack(fill="x", padx=8, pady=6)
        tk.Label(toprow, text="Инстанции / Данжи", bg=C["panel"],
                 fg=C["cyan"], font=("Segoe UI",10,"bold")).pack(side="left")
        btn(toprow, "➕ Добавить данж", self._add_instance, C["blue"],"white", padx=8).pack(side="right")
        btn(toprow, "🗑 Удалить", self._del_instance, C["red"],"white", padx=8).pack(side="right", padx=4)

        cols = ("id","name","level_req","level_max","max_players","cooldown_minutes","boss")
        self._inst_tree = ttk.Treeview(f, columns=cols, show="headings", height=10)
        hdrs2 = {"id":"ID","name":"Название","level_req":"Мин.ур","level_max":"Макс.ур",
                 "max_players":"Игроки","cooldown_minutes":"КД (мин)","boss":"Босс"}
        for c in cols:
            self._inst_tree.heading(c, text=hdrs2[c])
            self._inst_tree.column(c, width=130, anchor="center")
        self._inst_tree.pack(fill="both", expand=True, padx=8, pady=4)
        self._refresh_instances()
        self._inst_tree.bind("<Double-1>", self._edit_instance)

    def _build_economy_tab(self):
        f = tk.Frame(self._sub_nb, bg=C["panel"])
        self._sub_nb.add(f, text="💰 Экономика")
        self._economy_vars = {}
        fields = [
            ("shop_restock_hours",  "Рестоки магазинов (часы)",   "int"),
            ("bank_interest_rate",  "Процент банка (%/день)",     "float"),
            ("max_gold_per_slot",   "Макс. золото в стаке",       "int"),
            ("mail_cost",           "Стоимость письма (золото)",  "int"),
            ("repair_cost_pct",     "Стоимость починки (%)",      "int"),
        ]
        self._build_fields_section(f, "Параметры экономики", fields,
                                   self._cfg.get("economy",{}), self._economy_vars)

    # ─ Утилиты ─────────────────────────────────────────────────
    def _build_fields_section(self, parent, title, fields, data, vars_dict):
        tk.Label(parent, text=title, bg=C["panel"], fg=C["gold"],
                 font=("Segoe UI", 10, "bold"), pady=8, padx=16).pack(anchor="w")
        grid = tk.Frame(parent, bg=C["panel"])
        grid.pack(fill="x", padx=16)
        for i, (key, label, vtype) in enumerate(fields):
            row = tk.Frame(grid, bg=C["panel"])
            row.pack(fill="x", pady=3)
            tk.Label(row, text=label+":", bg=C["panel"], fg=C["text"],
                     font=("Segoe UI", 9), width=30, anchor="w").pack(side="left")
            v = tk.StringVar(value=str(data.get(key,"")))
            vars_dict[key] = (v, vtype)
            if vtype == "bool":
                chk = tk.Checkbutton(row, variable=v, onvalue="True", offvalue="False",
                                     bg=C["panel"], fg=C["text"], selectcolor=C["accent"],
                                     activebackground=C["panel"])
                v.set("True" if data.get(key, False) else "False")
                chk.pack(side="left")
            else:
                entry(row, textvariable=v, width=20).pack(side="left")

    def _collect_section(self, vars_dict, target_dict):
        for key, (var, vtype) in vars_dict.items():
            raw = var.get()
            try:
                if   vtype == "int":   target_dict[key] = int(raw)
                elif vtype == "float": target_dict[key] = float(raw)
                elif vtype == "bool":  target_dict[key] = raw == "True"
                else:                  target_dict[key] = raw
            except: target_dict[key] = raw

    def _collect_all(self):
        self._collect_section(self._server_vars,   self._cfg["server"])
        self._collect_section(self._rates_vars,    self._cfg["rates"])
        self._collect_section(self._gameplay_vars, self._cfg["gameplay"])
        self._collect_section(getattr(self,"_economy_vars",{}), self._cfg.setdefault("economy",{}))

    # ─ Зоны CRUD ─────────────────────────────────────────────
    def _refresh_zones(self):
        for row in self._zones_tree.get_children():
            self._zones_tree.delete(row)
        for z in self._cfg.get("zones", []):
            self._zones_tree.insert("", "end", values=(
                z.get("id",""), z.get("name",""),
                "✅" if z.get("pvp") else "❌",
                "✅" if z.get("safe") else "❌",
                z.get("level_min",0), z.get("level_max",999),
                z.get("max_players_per_zone",100)
            ))

    def _add_zone(self):
        z = {"id":f"zone_{len(self._cfg['zones'])}","name":"Новая зона",
             "pvp":False,"safe":False,"level_min":1,"level_max":60,"max_players_per_zone":100}
        self._cfg["zones"].append(z)
        self._refresh_zones()

    def _del_zone(self):
        sel = self._zones_tree.selection()
        if not sel: return
        idx = self._zones_tree.index(sel[0])
        if 0 <= idx < len(self._cfg["zones"]):
            if messagebox.askyesno("Удалить зону?", "Удалить выбранную зону?"):
                self._cfg["zones"].pop(idx); self._refresh_zones()

    def _edit_zone(self, event=None):
        sel = self._zones_tree.selection()
        if not sel: return
        idx = self._zones_tree.index(sel[0])
        zones = self._cfg.get("zones",[])
        if idx >= len(zones): return
        z = zones[idx]
        self._zone_editor_dialog(z, lambda: self._refresh_zones())

    def _zone_editor_dialog(self, z, on_save):
        win = tk.Toplevel(self.frame); win.title("Редактор зоны")
        win.geometry("380x420"); win.configure(bg=C["panel"]); win.grab_set()
        vars_z = {}
        fields_z = [
            ("id","ID зоны","str"), ("name","Название","str"),
            ("pvp","PvP","bool"), ("safe","Безопасная","bool"),
            ("level_min","Мин. уровень","int"), ("level_max","Макс. уровень","int"),
            ("max_players_per_zone","Макс. игроков в зоне","int"),
        ]
        tk.Label(win, text="Редактор зоны", bg=C["panel"], fg=C["gold2"],
                 font=("Segoe UI",11,"bold"), pady=8).pack()
        for key, label, vtype in fields_z:
            row = tk.Frame(win, bg=C["panel"]); row.pack(fill="x", padx=16, pady=3)
            tk.Label(row, text=label+":", bg=C["panel"], fg=C["text"],
                     font=("Segoe UI",9), width=24, anchor="w").pack(side="left")
            v = tk.StringVar(value=str(z.get(key,"")))
            vars_z[key] = (v, vtype)
            if vtype == "bool":
                v.set("True" if z.get(key,False) else "False")
                tk.Checkbutton(row, variable=v, onvalue="True", offvalue="False",
                               bg=C["panel"], fg=C["text"], selectcolor=C["accent"],
                               activebackground=C["panel"]).pack(side="left")
            else:
                entry(row, textvariable=v, width=20).pack(side="left")
        def save():
            self._collect_section(vars_z, z)
            on_save(); win.destroy()
        btn(win, "✅ Сохранить", save, C["green"],"black",padx=14,pady=6).pack(pady=12)

    # ─ Инстанции CRUD ─────────────────────────────────────────
    def _refresh_instances(self):
        for row in self._inst_tree.get_children():
            self._inst_tree.delete(row)
        for inst in self._cfg.get("instances", []):
            self._inst_tree.insert("", "end", values=(
                inst.get("id",""), inst.get("name",""),
                inst.get("level_req",1), inst.get("level_max",60),
                inst.get("max_players",5),
                inst.get("cooldown_minutes",60),
                inst.get("boss","")
            ))

    def _add_instance(self):
        inst = {"id":f"dungeon_{len(self._cfg['instances'])}","name":"Новый данж",
                "level_req":1,"level_max":60,"max_players":5,"cooldown_minutes":60,"boss":"BOSS_NAME"}
        self._cfg["instances"].append(inst)
        self._refresh_instances()

    def _del_instance(self):
        sel = self._inst_tree.selection()
        if not sel: return
        idx = self._inst_tree.index(sel[0])
        if 0 <= idx < len(self._cfg["instances"]):
            if messagebox.askyesno("Удалить данж?", "Удалить выбранный данж?"):
                self._cfg["instances"].pop(idx); self._refresh_instances()

    def _edit_instance(self, event=None):
        sel = self._inst_tree.selection()
        if not sel: return
        idx = self._inst_tree.index(sel[0])
        instances = self._cfg.get("instances",[])
        if idx >= len(instances): return
        inst = instances[idx]
        win = tk.Toplevel(self.frame); win.title("Редактор данжа")
        win.geometry("380x380"); win.configure(bg=C["panel"]); win.grab_set()
        vars_i = {}
        fields_i = [
            ("id","ID данжа","str"),("name","Название","str"),
            ("level_req","Мин. уровень","int"),("level_max","Макс. уровень","int"),
            ("max_players","Макс. игроков","int"),("cooldown_minutes","Кулдаун (мин)","int"),
            ("boss","Тип босса","str"),
        ]
        tk.Label(win, text="Редактор данжа", bg=C["panel"], fg=C["gold2"],
                 font=("Segoe UI",11,"bold"), pady=8).pack()
        for key, label, vtype in fields_i:
            row = tk.Frame(win, bg=C["panel"]); row.pack(fill="x", padx=16, pady=3)
            tk.Label(row, text=label+":", bg=C["panel"], fg=C["text"],
                     font=("Segoe UI",9), width=22, anchor="w").pack(side="left")
            v = tk.StringVar(value=str(inst.get(key,"")))
            vars_i[key] = (v, vtype)
            entry(row, textvariable=v, width=22).pack(side="left")
        def save():
            self._collect_section(vars_i, inst)
            self._refresh_instances(); win.destroy()
        btn(win, "✅ Сохранить", save, C["green"],"black",padx=14,pady=6).pack(pady=10)

    def _reset_defaults(self):
        if messagebox.askyesno("Сброс", "Сбросить все настройки сервера к дефолтным?"):
            self._cfg = copy.deepcopy(DEFAULT_SERVER_CONFIG)
            # Пересоздаём все sub-tabs (проще пересоздать весь ServerTab не выходя)
            for w in self._sub_nb.winfo_children():
                w.destroy()
            self._build_server_tab(); self._build_rates_tab(); self._build_gameplay_tab()
            self._build_zones_tab(); self._build_instances_tab(); self._build_economy_tab()
            self._status.set("🔄 Сброшено к дефолтным настройкам")
