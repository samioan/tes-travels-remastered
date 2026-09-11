cmake_minimum_required(VERSION 3.20)
project(dawnstar_port CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4 /utf-8)
endif()

# M2/M3/M4: asset foundations -- see docs/PORT_ROADMAP.md and src/assets/.
add_library(dawnstar_assets STATIC
  src/assets/character_data.cpp
  src/assets/dat_archive.cpp
  src/assets/dungeon_geometry.cpp
  src/assets/item_database.cpp
  src/assets/monster_database.cpp
  src/assets/spell_database.cpp
)
target_include_directories(dawnstar_assets PUBLIC src)

# M1: real tick loop + backbuffer + present -- see docs/PORT_ROADMAP.md and
# src/engine/, src/graphics/, src/platform/win32/.
add_executable(dawnstar_port WIN32
  src/main.cpp
  src/platform/win32/window.cpp
)
target_include_directories(dawnstar_port PRIVATE src)
if(WIN32)
  target_compile_definitions(dawnstar_port PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
  target_link_libraries(dawnstar_port PRIVATE user32 gdi32)
endif()

# M2 milestone smoke test -- see src/tests/m2_asset_smoke.cpp. Separate
# console executable so it doesn't disturb the M0/M1 windowed app.
add_executable(asset_smoke src/tests/m2_asset_smoke.cpp)
target_link_libraries(asset_smoke PRIVATE dawnstar_assets)

# M3 milestone smoke test -- see src/tests/m3_monster_dungeon_smoke.cpp.
add_executable(monster_dungeon_smoke src/tests/m3_monster_dungeon_smoke.cpp)
target_link_libraries(monster_dungeon_smoke PRIVATE dawnstar_assets)

# M4 milestone smoke test -- see src/tests/m4_character_data_smoke.cpp.
add_executable(character_data_smoke src/tests/m4_character_data_smoke.cpp)
target_link_libraries(character_data_smoke PRIVATE dawnstar_assets)
