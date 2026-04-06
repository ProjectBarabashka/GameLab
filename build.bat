@echo off
setlocal enabledelayedexpansion

echo ========================================
echo    AETHORIA: Build Script
echo ========================================

set "BUILD_TYPE=Debug"
if /I "%1"=="release" set "BUILD_TYPE=Release"

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

:build
:: Определяем путь к clang-cl
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

:: ИСПРАВЛЕНИЕ: Заменяем \ на / для CMake
set "COMPILER_PATH=%CLANG_CL:\=/%"

if not exist build mkdir build
cd build

echo [*] Running CMake...
cmake -G "Ninja" ^
 -DCMAKE_CXX_COMPILER="%COMPILER_PATH%" ^
 -DCMAKE_C_COMPILER="%COMPILER_PATH%" ^
 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
 ..

if errorlevel 1 (
    echo [ERROR] CMake failed!
    cd ..
    pause
    exit /b 1
)

echo [*] Building...
cmake --build . --config %BUILD_TYPE%

if errorlevel 1 (
    echo [ERROR] Build failed!
    cd ..
    pause
    exit /b 1
)

cd ..
echo [OK] Done!

if exist "build\AETHORIA.exe" (
    echo 1. Start Game
    echo 2. Start Editor
    set /p CHOICE="Choice: "
    if "!CHOICE!"=="1" start "" "build\AETHORIA.exe"
    if "!CHOICE!"=="2" start "" python "editor\aethoria_editor3.py"
)

pause
