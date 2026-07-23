#!/usr/bin/env python3
"""
Renz GUI Launcher — auto-passes --gui flag.
Built as renz_gui.exe with --noconsole so no cmd window.
"""
import sys, os

# Inject --gui before running the main launcher
if "--gui" not in sys.argv:
    sys.argv.insert(1, "--gui")

# Run the main launcher
launcher = os.path.join(os.path.dirname(__file__), "renz_launcher.py")
if os.path.exists(launcher):
    exec(open(launcher).read())
else:
    # Running as frozen exe — import directly
    import renz_launcher
