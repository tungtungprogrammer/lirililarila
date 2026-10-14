cmake_minimum_required(VERSION 3.16)
project(OldBloxStudio CXX)

# G3D is old C++ and needs C++14 (plus MSVC compatibility switches); Qt 6 needs C++17.
# So the program is split in four targets that never share a translation unit across that line:
#   model  - data model + editor logic (plain C++)
#   render - the G3D viewport
#   ui     - the Qt widgets
#   Studio - main(), glue

set(G3D_INCLUDE_DIRS "" CACHE STRING "G3D include directories")
set(G3D_LIBRARIES    "" CACHE STRING "G3D libraries to link")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB MODEL_SOURCES ${SRC}/DataModel/*.cpp)
add_library(model STATIC ${MODEL_SOURCES})
target_include_directories(model PUBLIC ${SRC})
target_compile_features(model PUBLIC cxx_std_14)

add_library(render STATIC
    ${SRC}/Render/Viewport.cpp ${SRC}/Render/MeshBuilder.cpp ${SRC}/Render/Interaction.cpp ${SRC}/Render/Hud.cpp)
target_include_directories(render PUBLIC ${SRC} ${G3D_INCLUDE_DIRS})
set_target_properties(render PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
target_link_libraries(render PUBLIC model ${G3D_LIBRARIES})

file(GLOB UI_SOURCES ${SRC}/UI/*.cpp)
add_library(ui STATIC ${UI_SOURCES})
target_include_directories(ui PUBLIC ${SRC})
set_target_properties(ui PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(ui PUBLIC model Qt6::Widgets)

add_executable(Studio ${SRC}/main.cpp)
set_target_properties(Studio PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(Studio PRIVATE ui render model Qt6::Widgets)

if(MSVC)
  target_compile_definitions(render PRIVATE _HAS_AUTO_PTR_ETC=1 _HAS_UNARY_BINARY_FUNCTION=1
      _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS _CRT_SECURE_NO_WARNINGS NOMINMAX)
  foreach(t model ui Studio)
    target_compile_definitions(${t} PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
  endforeach()
  target_compile_options(render PRIVATE /bigobj)
endif()
