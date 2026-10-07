# OldBlox Studio (starter)

Mini Roblox-2007-style Studio on G3D Innovation Engine 10.

## Layout
- src/main.cpp            entry point
- src/Studio.*            GApp: camera, input, rendering
- src/DataModel/Instance  base tree node (Name, Parent, Children)
- src/DataModel/Part      brick (CFrame, Size, Color, Anchored)
- src/DataModel/Workspace container + ray picking

## Controls
Left click = select, P = insert part, Delete = remove, camera = G3D default (right mouse + WASD)

## Build (MinGW-w64)
    mkdir build && cd build
    cmake -G "MinGW Makefiles" .. ^
      -DG3D_INCLUDE_DIRS="C:/G3D/G3D-base.lib/include;C:/G3D/G3D-gfx.lib/include;C:/G3D/G3D-app.lib/include;C:/G3D/external/glfw.lib/include;..." ^
      -DG3D_LIBRARIES="C:/G3D/build/lib/libG3D-app.a;...;opengl32;gdi32;winmm;ws2_32"
    mingw32-make

Note: G3D 10 officially targets MSVC on Windows. Getting it to build with MinGW may need patches.
