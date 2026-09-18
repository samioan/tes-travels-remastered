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
  src/assets/raw_image.cpp
  src/assets/shop_dialogue.cpp
  src/assets/spell_database.cpp
)
target_include_directories(stormhold_assets PUBLIC src)

# M6: procedural dungeon generation -- see docs/PORT_ROADMAP.md and
# src/world/.
add_library(stormhold_world STATIC
  src/world/dungeon_generator.cpp
  src/world/warden.cpp
)
target_include_directories(stormhold_world PUBLIC src)
target_link_libraries(stormhold_world PUBLIC stormhold_assets)

# M9: player character creation. M10: player movement -- see
# docs/PORT_ROADMAP.md and src/player/.
add_library(stormhold_player STATIC
  src/player/player_combat_stats.cpp
  src/player/player_creation.cpp
  src/player/player_inventory.cpp
  src/player/player_movement.cpp
)
target_include_directories(stormhold_player PUBLIC src)
target_link_libraries(stormhold_player PUBLIC stormhold_assets stormhold_world)

# M14: monster runtime state/AI-support (everything except tick()/
# attack()'s Monster-side reads, which need Player too -- see
# src/combat/) -- see docs/PORT_ROADMAP.md and src/monster/.
add_library(stormhold_monster STATIC
  src/monster/monster_runtime.cpp
)
target_include_directories(stormhold_monster PUBLIC src)
target_link_libraries(stormhold_monster PUBLIC stormhold_assets stormhold_world)

# M14: the two combat entry points needing both Player and Monster
# (Player.attack(Monster)/Monster.tick(Player,now)) -- see
# docs/PORT_ROADMAP.md and src/combat/.
add_library(stormhold_combat STATIC
  src/combat/combat_resolution.cpp
)
target_include_directories(stormhold_combat PUBLIC src)
target_link_libraries(stormhold_combat PUBLIC stormhold_player stormhold_monster)

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

# M6 milestone smoke test -- see src/tests/m6_dungeon_generator_smoke.cpp.
add_executable(dungeon_generator_smoke src/tests/m6_dungeon_generator_smoke.cpp)
target_link_libraries(dungeon_generator_smoke PRIVATE stormhold_world)

# M7 milestone smoke test -- see src/tests/m7_raw_image_smoke.cpp.
add_executable(raw_image_smoke src/tests/m7_raw_image_smoke.cpp)
target_link_libraries(raw_image_smoke PRIVATE stormhold_assets)

# M8 milestone smoke test -- see src/tests/m8_warden_smoke.cpp.
add_executable(warden_smoke src/tests/m8_warden_smoke.cpp)
target_link_libraries(warden_smoke PRIVATE stormhold_world)

# M9 milestone smoke test -- see src/tests/m9_player_creation_smoke.cpp.
add_executable(player_creation_smoke src/tests/m9_player_creation_smoke.cpp)
target_link_libraries(player_creation_smoke PRIVATE stormhold_player)

# M10 milestone smoke test -- see src/tests/m10_player_movement_smoke.cpp.
add_executable(player_movement_smoke src/tests/m10_player_movement_smoke.cpp)
target_link_libraries(player_movement_smoke PRIVATE stormhold_player)

# M11 milestone smoke test -- see src/tests/m11_shop_dialogue_smoke.cpp.
add_executable(shop_dialogue_smoke src/tests/m11_shop_dialogue_smoke.cpp)
target_link_libraries(shop_dialogue_smoke PRIVATE stormhold_assets)

# M12 milestone smoke test -- see src/tests/m12_player_inventory_smoke.cpp.
add_executable(player_inventory_smoke src/tests/m12_player_inventory_smoke.cpp)
target_link_libraries(player_inventory_smoke PRIVATE stormhold_player)

# M13 milestone smoke test -- see src/tests/m13_player_combat_stats_smoke.cpp.
add_executable(player_combat_stats_smoke src/tests/m13_player_combat_stats_smoke.cpp)
target_link_libraries(player_combat_stats_smoke PRIVATE stormhold_player)

# M14 milestone smoke test -- see src/tests/m14_monster_combat_smoke.cpp.
add_executable(monster_combat_smoke src/tests/m14_monster_combat_smoke.cpp)
target_link_libraries(monster_combat_smoke PRIVATE stormhold_combat)
