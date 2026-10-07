cmake_minimum_required(VERSION 3.16)
project(OldBloxStudio CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- G3D location (set these on the cmake command line) ----
# G3D_INCLUDE_DIRS : semicolon-separated list of G3D include folders
#                    (G3D-base.lib/include, G3D-gfx.lib/include, G3D-app.lib/include, external/*/include)
# G3D_LIBRARIES    : semicolon-separated list of built G3D .a/.lib files + system libs
set(G3D_INCLUDE_DIRS "" CACHE STRING "G3D include directories")
set(G3D_LIBRARIES    "" CACHE STRING "G3D libraries to link")

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(Studio ${SOURCES})
target_include_directories(Studio PRIVATE src ${G3D_INCLUDE_DIRS})
target_link_libraries(Studio PRIVATE ${G3D_LIBRARIES})
