#!/bin/bash
set -e

echo ""
echo "╔══════════════════════════════════════╗"
echo "║   AETHORIA: Eternal Realms           ║"
echo "║   Build Script for Linux / macOS     ║"
echo "╚══════════════════════════════════════╝"
echo ""

BUILD_TYPE="Release"
[[ "$1" == "debug" ]]   && BUILD_TYPE="Debug"
[[ "$1" == "release" ]] && BUILD_TYPE="Release"
echo "[*] Build type: $BUILD_TYPE"
echo ""

# ── Проверки зависимостей ────────────────────────────────────
check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo "[ERROR] $1 не найден!"
        echo "  $2"
        exit 1
    fi
    echo "[OK] $1: $(command -v $1)"
}

check_dep cmake  "Ubuntu: sudo apt install cmake | macOS: brew install cmake"
check_dep g++    "Ubuntu: sudo apt install build-essential | macOS: xcode-select --install" 2>/dev/null \
    || check_dep clang++ "Установи Xcode Command Line Tools"

# SFML
if ! pkg-config --exists sfml-all 2>/dev/null; then
    echo "[WARN] SFML не найден в pkg-config"
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "       Ubuntu/Debian: sudo apt install libsfml-dev"
        echo "       Fedora:        sudo dnf install SFML-devel"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "       macOS: brew install sfml"
    fi
    echo ""
    read -rp "Продолжить? (y/N): " yn
    [[ "$yn" =~ ^[Yy]$ ]] || exit 1
else
    echo "[OK] SFML: $(pkg-config --modversion sfml-all)"
fi
echo ""

# ── Сборка ───────────────────────────────────────────────────
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "[*] Используется $NPROC ядер"

mkdir -p build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build . --config "$BUILD_TYPE" -j"$NPROC"

cd ..

echo ""
echo "╔══════════════════════════════════════╗"
echo "║   [OK] Сборка завершена!             ║"
echo "╚══════════════════════════════════════╝"
echo ""

EXE="./build/AETHORIA"
if [ -f "$EXE" ]; then
    SIZE=$(du -h "$EXE" | cut -f1)
    echo "Исполняемый файл: $EXE ($SIZE)"
    echo ""
    echo "[1] Запустить игру"
    echo "[2] Запустить редактор"
    echo "[3] Выход"
    echo ""
    read -rp "Выбор: " choice
    case "$choice" in
        1) echo "" && echo "[*] Запуск..." && "$EXE" ;;
        2) echo "" && echo "[*] Редактор..." && python3 editor/aethoria_editor3.py ;;
        *) exit 0 ;;
    esac
else
    echo "[WARN] Бинарник не найден: $EXE"
fi
