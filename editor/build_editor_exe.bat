@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion

echo ==============================
echo AETHORIA Editor - Build EXE
echo ==============================
echo.

set "BAT_DIR=%~dp0"
if "%BAT_DIR:~-1%"=="\" set "BAT_DIR=%BAT_DIR:~0,-1%"

where python >nul 2>&1
if errorlevel 1 ( echo ERROR: Python not found & pause & exit /b 1 )

set "EDITOR_PY=%BAT_DIR%\aethoria_editor3.py"
if not exist "%EDITOR_PY%" set "EDITOR_PY=%BAT_DIR%\editor\aethoria_editor3.py"
if not exist "%EDITOR_PY%" ( echo ERROR: aethoria_editor3.py not found & pause & exit /b 1 )
echo [OK] %EDITOR_PY%

set "ICO_PATH=%BAT_DIR%\logo_EXE.ico"
if not exist "%ICO_PATH%" set "ICO_PATH=%BAT_DIR%\logo EXE.ico"
if not exist "%ICO_PATH%" (
    set "ICO_ARG=NONE"
    echo [SKIP] logo_EXE.ico not found - EXE will use default icon
) else (
    set "ICO_ARG=%ICO_PATH%"
    echo [OK] icon: %ICO_PATH%
)

echo [*] Installing dependencies...
python -m pip install -q pyinstaller pillow opencv-python
echo [OK] Dependencies ready

set "SPEC=%BAT_DIR%\editor_build.spec"
set "MAKE_SPEC=%BAT_DIR%\make_spec.py"

if not exist "%MAKE_SPEC%" (
    echo ERROR: make_spec.py not found next to bat file!
    pause & exit /b 1
)

echo [*] Generating spec file...
python "%MAKE_SPEC%" "%EDITOR_PY%" "%ICO_ARG%" "%SPEC%"
if errorlevel 1 ( echo ERROR: spec generation failed & pause & exit /b 1 )

echo.
echo [*] Building EXE (1-3 minutes)...
echo.
python -m PyInstaller --noconfirm --distpath "%BAT_DIR%\dist" --workpath "%BAT_DIR%\build_tmp" "%SPEC%"

if errorlevel 1 (
    echo.
    echo ==============================
    echo    BUILD FAILED
    echo ==============================
    del "%SPEC%" >nul 2>&1
    pause & exit /b 1
)

del "%SPEC%" >nul 2>&1

echo.
echo ==============================
echo [OK] dist\AethoriaEditor.exe
echo ==============================
pause
