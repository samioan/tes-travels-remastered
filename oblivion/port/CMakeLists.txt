cmake_minimum_required(VERSION 3.20)
project(oblivion_port CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
if(MSVC)
  add_compile_options(/W4 /utf-8)
endif()

# Vendored stb_image (PNG decoding), see third_party/stb/PROVENANCE.md.
add_library(stb_image STATIC third_party/stb/stb_image_impl.cpp)
target_include_directories(stb_image PUBLIC third_party/stb)
if(MSVC)
  target_compile_options(stb_image PRIVATE /W3)
endif()

# Asset loaders: .jtm tile maps, .cml sprite sets, .scr scripts, PNG images.
# Each mirrors a parser in ../tools/parse_*.py (and the Java loader it was
# derived from); see ../docs/ASSET_FORMATS.md.
add_library(oblivion_assets STATIC
  src/assets/cml.cpp
  src/assets/gdr_font.cpp
  src/assets/image.cpp
  src/assets/jtm.cpp
  src/assets/lang.cpp
  src/assets/scr.cpp
)
target_include_directories(oblivion_assets PUBLIC src)
target_link_libraries(oblivion_assets PUBLIC stb_image)

# Rendering + the static level view.
add_library(oblivion_world STATIC
  src/game/dialogue_screen.cpp
  src/game/game_app.cpp
  src/game/help_pages.cpp
  src/graphics/text.cpp
  src/render/sprite_renderer.cpp
  src/script/interpreter.cpp
  src/world/actor.cpp
  src/world/game_world.cpp
  src/world/items.cpp
  src/world/combat.cpp
  src/world/projectiles.cpp
  src/world/level_view.cpp
)
target_link_libraries(oblivion_world PUBLIC oblivion_assets)

# Software-rendered raw Win32 (no SDL/D3D/GL), like the sibling ports.
add_executable(oblivion_port WIN32
  src/main.cpp
  src/platform/win32/window.cpp
)
target_link_libraries(oblivion_port PRIVATE oblivion_world)

# Milestone smoke tests (plain executables that exit non-zero on failure).
enable_testing()
add_executable(asset_smoke src/tests/asset_smoke.cpp)
target_link_libraries(asset_smoke PRIVATE oblivion_assets)
add_test(NAME asset_smoke COMMAND asset_smoke ${CMAKE_SOURCE_DIR}/../extracted)

add_executable(actor_smoke src/tests/actor_smoke.cpp)
target_link_libraries(actor_smoke PRIVATE oblivion_world)
add_test(NAME actor_smoke COMMAND actor_smoke ${CMAKE_SOURCE_DIR}/../extracted)

add_executable(script_smoke src/tests/script_smoke.cpp)
target_link_libraries(script_smoke PRIVATE oblivion_world)
add_test(NAME script_smoke COMMAND script_smoke ${CMAKE_SOURCE_DIR}/../extracted)

add_executable(combat_smoke src/tests/combat_smoke.cpp)
target_link_libraries(combat_smoke PRIVATE oblivion_world)
add_test(NAME combat_smoke COMMAND combat_smoke ${CMAKE_SOURCE_DIR}/../extracted)
