cmake_minimum_required(VERSION 3.20)
project(stormhold_port CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4 /utf-8)
endif()

# M2/M3/M4: asset foundations -- see docs/PORT_ROADMAP.md and src/assets/.
add_library(stormhold_assets STATIC
  src/assets/character_data.cpp
  src/assets/dungeon_geometry.cpp
  src/assets/item_database.cpp
  src/assets/monster_database.cpp
  src/assets/spell_database.cpp
)
target_include_directories(stormhold_assets PUBLIC src)

# M1: real tick loop + backbuffer + present. Software-rendered raw Win32
# (no SDL/D3D/GL dependency), matching the sibling shadowkey-decomp/
# dawnstar ports' platform layer.
add_executable(stormhold_port WIN32
  src/main.cpp
  src/platform/win32/window.cpp
)
target_include_directories(stormhold_port PRIVATE src)
target_compile_definitions(stormhold_port PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
if(MSVC)
  target_compile_options(stormhold_port PRIVATE /W4)
endif()

# M2 milestone smoke test -- see src/tests/m2_asset_smoke.cpp.
add_executable(asset_smoke src/tests/m2_asset_smoke.cpp)
target_link_libraries(asset_smoke PRIVATE stormhold_assets)

# M3 milestone smoke test -- see src/tests/m3_monster_dungeon_smoke.cpp.
add_executable(monster_dungeon_smoke src/tests/m3_monster_dungeon_smoke.cpp)
target_link_libraries(monster_dungeon_smoke PRIVATE stormhold_assets)

# M4 milestone smoke test -- see src/tests/m4_character_data_smoke.cpp.
add_executable(character_data_smoke src/tests/m4_character_data_smoke.cpp)
target_link_libraries(character_data_smoke PRIVATE stormhold_assets)

# M5 milestone smoke test -- see src/tests/m5_java_random_smoke.cpp.
# util/java_random.h is header-only, so no library to link against.
add_executable(java_random_smoke src/tests/m5_java_random_smoke.cpp)
target_include_directories(java_random_smoke PRIVATE src)
