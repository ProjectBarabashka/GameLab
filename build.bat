@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul 2>nul

echo.
echo ╔══════════════════════════════════════╗
echo ║   AETHORIA: Eternal Realms           ║
echo ║   Build Script for Windows           ║
echo ╚══════════════════════════════════════╝
echo.

REM ── Проверка CMake ───────────────────────────────────────────
where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake не найден!
    echo        Скачай: https://cmake.org/download/
    echo        Добавь в PATH при установке.
    pause & exit /b 1
)
echo [OK] CMake: 
cmake --version | findstr /C:"cmake version"

REM ── Определяем конфигурацию ──────────────────────────────────
set BUILD_TYPE=Release
if /i "%1"=="debug"   set BUILD_TYPE=Debug
if /i "%1"=="release" set BUILD_TYPE=Release

echo [*] Build type: %BUILD_TYPE%
echo.

REM ── Создаём папку build ──────────────────────────────────────
if not exist build mkdir build
cd build

REM ── Определяем генератор ─────────────────────────────────────
set GENERATOR=
where msbuild >nul 2>nul
if not errorlevel 1 (
    echo [*] Visual Studio найден — используем MSVC
    REM Пробуем VS 2022, 2019, 2017 по порядку
    cmake .. -G "Visual Studio 17 2022" >nul 2>nul
    if errorlevel 1 cmake .. -G "Visual Studio 16 2019" >nul 2>nul
    if errorlevel 1 cmake .. -G "Visual Studio 15 2017"
) else (
    where mingw32-make >nul 2>nul
    if not errorlevel 1 (
        echo [*] MinGW найден
        cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    ) else (
        echo [*] Используем дефолтный генератор
        cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    )
)

if errorlevel 1 (
    echo.
    echo [ERROR] CMake конфигурация не удалась!
    echo.
    echo Возможные причины:
    echo  1. SFML не найден. Укажи путь:
    echo     cmake .. -DSFML_DIR="C:\путь\к\SFML\lib\cmake\SFML"
    echo  2. Компилятор не найден. Установи Visual Studio или MinGW.
    cd ..
    pause & exit /b 1
)

echo.
echo [*] Сборка...
echo.
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo.
    echo [ERROR] Сборка не удалась!
    cd ..
    pause & exit /b 1
)

cd ..

echo.
echo ╔══════════════════════════════════════╗
echo ║   [OK] Сборка завершена успешно!     ║
echo ╚══════════════════════════════════════╝
echo.

REM Ищем .exe в разных местах
set EXE=
if exist "build\%BUILD_TYPE%\AETHORIA.exe" set EXE=build\%BUILD_TYPE%\AETHORIA.exe
if exist "build\AETHORIA.exe"              set EXE=build\AETHORIA.exe

if defined EXE (
    echo Исполняемый файл: %EXE%
    echo.
    echo [1] Запустить игру
    echo [2] Запустить редактор (editor\aethoria_editor3.py)
    echo [3] Открыть папку
    echo [4] Выход
    echo.
    set /p choice="Выбор: "
    if "!choice!"=="1" (
        echo.
        echo [*] Запуск игры...
        start "" "%EXE%"
    ) else if "!choice!"=="2" (
        echo.
        echo [*] Запуск редактора...
        python editor\aethoria_editor3.py
    ) else if "!choice!"=="3" (
        explorer build\%BUILD_TYPE%
    )
) else (
    echo [WARN] .exe не найден — проверь папку build\
)

pause
