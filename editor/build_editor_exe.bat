@echo off
chcp 65001 > nul

echo ==============================
echo AETHORIA Editor Build
echo ==============================
echo.

:: Проверка Python
where python >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found
    pause
    exit /b
)

python --version
echo.

:: Проверка файла
if not exist "aethoria_editor3.py" (
    echo ERROR: aethoria_editor3.py not found
    pause
    exit /b
)

:: Установка зависимостей
echo Installing dependencies...
python -m pip install pyinstaller pillow opencv-python

echo.
echo Building EXE...
echo.

python -m PyInstaller ^
--onefile ^
--windowed ^
--name AethoriaEditor ^
--hidden-import=tkinter ^
--hidden-import=tkinter.ttk ^
--hidden-import=PIL ^
aethoria_editor3.py

if errorlevel 1 (
    echo.
    echo BUILD FAILED
    pause
    exit /b
)

echo.
echo DONE!
echo EXE: dist\AethoriaEditor.exe
echo.

pause