# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['renz_launcher.py'],
    pathex=[],
    binaries=[],
    datas=[('personas', 'personas'), ('proxy_server.py', '.'), ('renz_app', 'renz_app')],
    hiddenimports=['customtkinter', 'rich', 'rich.console', 'rich.panel', 'rich.table', 'rich.box', 'rich.prompt'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='renz_launcher',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
