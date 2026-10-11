# OldBlox Studio

A tribute to the 2007-era Roblox Studio: a Qt "Luna"-style editor around a G3D viewport.

* Viewport: studded bricks, sun lighting, sky with clouds, blue selection box, Move/Resize handles.
* UI: menus, toolbars, Toolbox, Explorer, Properties, Output (Qt 6, classic style).

## Controls (viewport)
Right-drag look - W/A/S/D move, Q/E down/up, Shift faster, wheel zoom, F focus selection.
Click select - 1/2/3 Select/Move/Resize - Delete, Ctrl+Z/Y/C/X/V/D - Ctrl+R rotate 90 (Y), Ctrl+T tilt 90 (Z).
Tools > Grid Snap changes the snap step. Files save as `.obxl`.

## Build
Push to GitHub: `.github/workflows/build.yml` builds G3D and Studio and uploads `OldBloxStudio-windows`.
Run `Studio.exe` from that folder (it must sit next to `data/`). `--floating-viewport` keeps the 3D view as its own window.

## Layout
`src/DataModel` (plain C++ model + editor), `src/Render` (G3D, C++14), `src/UI` (Qt, C++17), `src/main.cpp`.
