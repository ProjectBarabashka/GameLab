# 🚀 Quick Start Guide

Get Aethoria running in 5 minutes.

---

## Windows

### Step 1 — Get SFML

Download **SFML 2.6.1 (Visual C++ 17 64-bit)** from:
https://www.sfml-dev.org/download/sfml/2.6.1/

Extract to `C:\SFML-2.6.1`

Your folder should look like:
```
C:\SFML-2.6.1\
    bin\
    include\
    lib\
        cmake\
            SFML\
                SFMLConfig.cmake   ← CMake needs this
```

### Step 2 — Get CMake

Download from https://cmake.org/download/ and install.
**Check "Add CMake to PATH"** during install.

### Step 3 — Build

Double-click `build.bat` or run from terminal:
```bat
build.bat
```

The script will:
1. Find Visual Studio or MinGW automatically
2. Configure CMake with your SFML path
3. Compile the project
4. Ask if you want to run the game

### Step 4 — Run the Editor

```bat
python editor\aethoria_editor3.py
```

---

## Linux (Ubuntu / Debian)

```bash
# 1. Install everything
sudo apt-get update
sudo apt-get install cmake build-essential libsfml-dev python3

# 2. Build
chmod +x build.sh
./build.sh

# 3. Run editor
python3 editor/aethoria_editor3.py
```

---

## Common Problems

### "SFML not found"
Make sure you extracted SFML to `C:\SFML-2.6.1` exactly.
Or run CMake manually with the correct path:
```bat
cmake .. -DSFML_DIR="D:\YourPath\SFML-2.6.1\lib\cmake\SFML"
```

### "CMake not found"
Install CMake from https://cmake.org and add it to PATH.

### Game window opens but map is empty / entities missing
Run the editor first (`python editor/aethoria_editor3.py`),
open any scene in the **🌍 Locations** tab, and press **Save**.
This syncs `map.json` → `assets/scenes/aethoria_city.json`.

### Editor crashes immediately
Make sure you're using Python 3.10 or newer:
```bash
python --version
```

### Animations not loading
Check `assets/animations.json` is valid JSON.
The asset pipeline tab in the editor can regenerate it.

---

## Adding your own sprites

1. Prepare a spritesheet PNG (rows × cols of equal-size frames)
2. Open the editor → **🎬 Assets** tab
3. Drop your video/image file
4. Set frame size and FPS
5. Click Convert — spritesheet and JSON entry are created automatically
6. The engine will load it next run

---

## Folder layout after first build

```
build/
└── Release/
    ├── AETHORIA.exe
    ├── sfml-graphics-2.dll
    ├── sfml-audio-2.dll
    ├── sfml-window-2.dll
    ├── sfml-system-2.dll
    └── assets/              ← auto-copied from project root
        ├── map.json
        ├── animations.json
        ├── scenes/
        └── ...
```

Everything the game needs is in `build/Release/`. You can zip that folder
and distribute it (with SFML DLLs included).
