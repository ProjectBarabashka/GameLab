import sys
import os

editor = sys.argv[1]
ico    = sys.argv[2] if len(sys.argv) > 2 and sys.argv[2] != "NONE" else None
out    = sys.argv[3]

ico_line = "icon=%r," % ico if ico else ""

spec = """\
# -*- mode: python -*-
block_cipher = None
a = Analysis(
    [%(editor)r],
    pathex=[],
    binaries=[],
    datas=[],
    hiddenimports=['tkinter', 'tkinter.ttk', 'PIL', 'PIL._tkinter_finder'],
    hookspath=[],
    runtime_hooks=[],
    excludes=[],
    cipher=block_cipher,
)
pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)
exe = EXE(
    pyz, a.scripts, a.binaries, a.zipfiles, a.datas,
    name='AethoriaEditor',
    debug=False,
    strip=False,
    upx=False,
    console=False,
    %(ico_line)s
)
""" % {"editor": editor, "ico_line": ico_line}

with open(out, "w", encoding="utf-8") as f:
    f.write(spec)

print("[OK] spec written to:", out)
if ico:
    print("[OK] icon:", ico)
else:
    print("[SKIP] no icon")
