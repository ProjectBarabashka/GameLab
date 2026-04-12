@echo off
setlocal enabledelayedexpansion

echo ========================================
echo    AETHORIA: Build Script
echo ========================================

set "BUILD_TYPE=Debug"
if /I "%1"=="release" set "BUILD_TYPE=Release"

:: Корень проекта — папка где лежит этот .bat (до любого cd!)
set "PROJECT_ROOT=%~dp0"
if "!PROJECT_ROOT:~-1!"=="\" set "PROJECT_ROOT=!PROJECT_ROOT:~0,-1!"

echo [*] PROJECT_ROOT = !PROJECT_ROOT!

set "VCVARS="
if defined VSCMD_ARG_TGT_ARCH (
    echo [OK] MSVC environment already ready.
    goto :build
)

echo [*] Searching for VS...

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

if defined VS_PATH (
    set "VCVARS=!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat"
)

if not exist "!VCVARS!" (
    for /d %%i in ("C:\Program Files\Microsoft Visual Studio\2022\*") do (
        if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARS=%%i\VC\Auxiliary\Build\vcvarsall.bat"
    )
)

if not exist "!VCVARS!" (
    echo [ERROR] vcvarsall.bat not found!
    pause
    exit /b 1
)

echo [OK] Found: "!VCVARS!"
call "!VCVARS!" x64

:: Переменная PROJECT_ROOT могла сброситься после call — восстанавливаем
set "PROJECT_ROOT=%~dp0"
if "!PROJECT_ROOT:~-1!"=="\" set "PROJECT_ROOT=!PROJECT_ROOT:~0,-1!"

:build
:: ════════════════════════════════════════════════
:: ИКОНКА: поиск, генерация .rc, компиляция .res
:: ════════════════════════════════════════════════
echo [*] Ищем иконку в: !PROJECT_ROOT!

set "ICO_PATH="
if exist "!PROJECT_ROOT!\logo.ico"       set "ICO_PATH=!PROJECT_ROOT!\logo.ico"
if exist "!PROJECT_ROOT!\logo_EXE.ico"   set "ICO_PATH=!PROJECT_ROOT!\logo_EXE.ico"
if exist "!PROJECT_ROOT!\logo EXE.ico"   set "ICO_PATH=!PROJECT_ROOT!\logo EXE.ico"

if not defined ICO_PATH (
    echo [SKIP] Иконка не найдена.
    echo [INFO] Проверьте что logo.ico лежит в: !PROJECT_ROOT!
    goto :icon_done
)
echo [OK] Иконка найдена: !ICO_PATH!

set "RC_FILE=!PROJECT_ROOT!\resources.rc"
set "RES_FILE=!PROJECT_ROOT!\resources.res"
set "ICO_FWD=!ICO_PATH:\=/!"

(
    echo IDI_ICON1 ICON "!ICO_FWD!"
) > "!RC_FILE!"
echo [OK] resources.rc создан

set "RC_EXE="
for /f "tokens=*" %%i in ('where rc.exe 2^>nul') do (
    if not defined RC_EXE set "RC_EXE=%%i"
)
if not defined RC_EXE (
    for %%D in (
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64"
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64"
        "C:\Program Files (x86)\Windows Kits\10\bin\x64"
    ) do (
        if exist "%%~D\rc.exe" (
            if not defined RC_EXE set "RC_EXE=%%~D\rc.exe"
        )
    )
)
if not defined RC_EXE (
    echo [WARNING] rc.exe не найден. Иконка не будет вшита.
    goto :icon_done
)

echo [*] Компилируем иконку...
"!RC_EXE!" /nologo /fo "!RES_FILE!" "!RC_FILE!"
if errorlevel 1 (
    echo [WARNING] rc.exe ошибка. Иконка не будет вшита.
    goto :icon_done
)
echo [OK] resources.res готов
set "RES_FILE_FWD=!RES_FILE:\=/!"
set "ICON_RES_FLAG=-DICON_RES_FILE=!RES_FILE_FWD!"

:icon_done

:: ════════════════════════════════════════════════
:: clang-cl
:: ════════════════════════════════════════════════
set "CLANG_CL="
where clang-cl >nul 2>&1
if %errorlevel% equ 0 (
    for /f "tokens=*" %%i in ('where clang-cl') do set "CLANG_CL=%%i"
) else (
    if exist "C:\Program Files\LLVM\bin\clang-cl.exe" set "CLANG_CL=C:\Program Files\LLVM\bin\clang-cl.exe"
)

if not defined CLANG_CL (
    echo [ERROR] clang-cl not found!
    pause
    exit /b 1
)

set "COMPILER_PATH=!CLANG_CL:\=/!"

:: ════════════════════════════════════════════════
:: CMake
:: ════════════════════════════════════════════════
if not exist "!PROJECT_ROOT!\build" mkdir "!PROJECT_ROOT!\build"
cd "!PROJECT_ROOT!\build"

if exist CMakeCache.txt del /f CMakeCache.txt

echo [*] Running CMake...
cmake -G "Ninja" ^
 -DCMAKE_CXX_COMPILER="!COMPILER_PATH!" ^
 -DCMAKE_C_COMPILER="!COMPILER_PATH!" ^
 -DCMAKE_BUILD_TYPE=!BUILD_TYPE! ^
 !ICON_RES_FLAG! ^
 "!PROJECT_ROOT!"

if errorlevel 1 (
    echo [ERROR] CMake failed!
    cd "!PROJECT_ROOT!"
    pause
    exit /b 1
)

echo [*] Building...
cmake --build . --config !BUILD_TYPE!

if errorlevel 1 (
    echo [ERROR] Build failed!
    cd "!PROJECT_ROOT!"
    pause
    exit /b 1
)

cd "!PROJECT_ROOT!"
echo [OK] Done!

if exist "build\AETHORIA.exe" (
    echo 1. Start Game
    echo 2. Start Editor
    set /p CHOICE="Choice: "
    if "!CHOICE!"=="1" start "" "build\AETHORIA.exe"
    if "!CHOICE!"=="2" start "" python "editor\aethoria_editor3.py"
)

pause
