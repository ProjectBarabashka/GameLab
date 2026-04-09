#!/bin/bash
# AETHORIA Editor — сборка в один исполняемый файл (Linux / macOS)
set -e

echo ""
echo "╔══════════════════════════════════════╗"
echo "║  AETHORIA Editor — Build Executable  ║"
echo "╚══════════════════════════════════════╝"
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EDITOR_PY="$SCRIPT_DIR/aethoria_editor3.py"

if [ ! -f "$EDITOR_PY" ]; then
    echo "[ERROR] aethoria_editor3.py не найден рядом со скриптом!"
    exit 1
fi

PY=""
for cmd in python3 python; do
    if command -v "$cmd" &>/dev/null; then PY="$cmd"; break; fi
done
[ -z "$PY" ] && echo "[ERROR] Python не найден!" && exit 1
echo "[OK] Python: $($PY --version)"

echo "[*] Устанавливаем зависимости..."
$PY -m pip install -q --upgrade pip
$PY -m pip install -q pyinstaller pillow opencv-python
echo "[OK] Зависимости установлены"

EXTRA_ARGS=()

if [ -f "$SCRIPT_DIR/aethoria_mmo_tabs.py" ]; then
    EXTRA_ARGS+=("--add-data" "$SCRIPT_DIR/aethoria_mmo_tabs.py:.")
    echo "[OK] aethoria_mmo_tabs.py — включён"
fi

if [ -d "$SCRIPT_DIR/assets" ]; then
    EXTRA_ARGS+=("--add-data" "$SCRIPT_DIR/assets:assets")
    echo "[OK] Папка assets — включена"
fi

if [ -f "$SCRIPT_DIR/assets/icon.png" ]; then
    EXTRA_ARGS+=("--icon" "$SCRIPT_DIR/assets/icon.png")
elif [ -f "$SCRIPT_DIR/icon.png" ]; then
    EXTRA_ARGS+=("--icon" "$SCRIPT_DIR/icon.png")
fi

echo ""
echo "[*] Собираем исполняемый файл (1-3 минуты)..."
echo ""

$PY -m PyInstaller \
    --noconfirm \
    --onefile \
    --windowed \
    --name "AethoriaEditor" \
    --distpath "$SCRIPT_DIR/editor_dist" \
    --workpath "$SCRIPT_DIR/editor_build" \
    --specpath "$SCRIPT_DIR/editor_build" \
    --hidden-import "PIL._tkinter_finder" \
    --hidden-import "tkinter" \
    --hidden-import "tkinter.ttk" \
    --collect-all "PIL" \
    "${EXTRA_ARGS[@]}" \
    "$EDITOR_PY"

RESULT="$SCRIPT_DIR/editor_dist/AethoriaEditor"
if [ -f "$RESULT" ]; then
    SIZE=$(du -h "$RESULT" | cut -f1)
    echo ""
    echo "╔══════════════════════════════════════╗"
    echo "║  [OK] Готово!                        ║"
    echo "╚══════════════════════════════════════╝"
    echo "  Файл: $RESULT ($SIZE)"
    echo ""
    echo "  Как отдать другу:"
    echo "    - Если assets включены — шли только этот файл"
    echo "    - Иначе скопируй папку assets рядом с исполняемым файлом"
    echo ""
    read -rp "Запустить редактор сейчас? (y/n): " RUN
    [[ "$RUN" =~ ^[Yy]$ ]] && "$RESULT"
else
    echo "[ERROR] Исполняемый файл не найден."
    exit 1
fi

read -rp "Удалить временные файлы сборки? (y/n): " CLEAN
[[ "$CLEAN" =~ ^[Yy]$ ]] && rm -rf "$SCRIPT_DIR/editor_build"
