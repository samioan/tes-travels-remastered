cmake_minimum_required(VERSION 3.20)
project(dawnstar_port CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4 /utf-8)
endif()

# M10: vendored stb_image (PNG decoding) -- see
# third_party/stb/PROVENANCE.md.
add_library(stb_image STATIC third_party/stb/stb_image_impl.cpp)
target_include_directories(stb_image PUBLIC third_party/stb)
if(MSVC)
  # Vendored third-party source, not held to this project's own /W4.
  target_compile_options(stb_image PRIVATE /W3)
endif()

# M2/M3/M4/M7/M8/M10: asset foundations -- see docs/PORT_ROADMAP.md and
# src/assets/.
add_library(dawnstar_assets STATIC
  src/assets/character_data.cpp
  src/assets/dat_archive.cpp
  src/assets/decoded_image.cpp
  src/assets/dungeon_geometry.cpp
  src/assets/help_text.cpp
  src/assets/img_archive.cpp
  src/assets/item_database.cpp
  src/assets/monster_database.cpp
  src/assets/shop_dialogue.cpp
  src/assets/spell_database.cpp
)
target_include_directories(dawnstar_assets PUBLIC src)
target_link_libraries(dawnstar_assets PRIVATE stb_image)

# M6: procedural dungeon generation -- see docs/PORT_ROADMAP.md and
# src/world/.
add_library(dawnstar_world STATIC
  src/world/dungeon_generator.cpp
)
target_include_directories(dawnstar_world PUBLIC src)
target_link_libraries(dawnstar_world PUBLIC dawnstar_assets)

# M9/M10/M21: corridor rendering + the HUD status-bar overlay -- see
# docs/PORT_ROADMAP.md, src/render/, src/world/dungeon_view.h.
add_library(dawnstar_render STATIC
  src/render/corridor_render_plan.cpp
  src/render/frame_renderer.cpp
)
target_include_directories(dawnstar_render PUBLIC src)
target_link_libraries(dawnstar_render PUBLIC dawnstar_world dawnstar_player)

# M11/M12/M13/M14/M16/M17/M23: player character creation + full save
# format + movement (now also touching the live world registry for
# dropped-item auto-loot/roaming-monster cleanup) + combat/skill stat
# math + inventory/equip management + self-targeted spellcasting + the
# lightweight summary save format -- see docs/PORT_ROADMAP.md and
# src/player/.
add_library(dawnstar_player STATIC
  src/player/player_creation.cpp
  src/player/player_save.cpp
  src/player/player_movement.cpp
  src/player/player_combat_stats.cpp
  src/player/player_inventory.cpp
  src/player/player_spellcasting.cpp
)
target_include_directories(dawnstar_player PUBLIC src)
target_link_libraries(dawnstar_player PUBLIC dawnstar_assets dawnstar_world dawnstar_dungeon)

# M15: monster runtime state/AI/combat-support (everything except
# tick()/attack()/castOnMonster(), which need both Player and Monster --
# see src/combat/) -- see docs/PORT_ROADMAP.md and src/monster/.
add_library(dawnstar_monster STATIC
  src/monster/monster_runtime.cpp
)
target_include_directories(dawnstar_monster PUBLIC src)
target_link_libraries(dawnstar_monster PUBLIC dawnstar_assets dawnstar_world)

# M15/M16: the combat entry points needing both Player and Monster --
# see docs/PORT_ROADMAP.md and src/combat/.
add_library(dawnstar_combat STATIC
  src/combat/combat_resolution.cpp
)
target_include_directories(dawnstar_combat PUBLIC src)
target_link_libraries(dawnstar_combat PUBLIC dawnstar_player dawnstar_monster dawnstar_dungeon)

# M22: the live per-level monster/chest/dropped-item registry (needs
# both World and Monster) -- see docs/PORT_ROADMAP.md and src/dungeon/.
add_library(dawnstar_dungeon STATIC
  src/dungeon/dungeon_runtime.cpp
)
target_include_directories(dawnstar_dungeon PUBLIC src)
target_link_libraries(dawnstar_dungeon PUBLIC dawnstar_world dawnstar_monster)

# M1/M20: real tick loop + backbuffer + present, now wired to the real
# asset/world/player/render pipeline -- see docs/PORT_ROADMAP.md and
# src/engine/, src/graphics/, src/platform/win32/, src/main.cpp.
add_executable(dawnstar_port WIN32
  src/main.cpp
  src/platform/win32/window.cpp
)
target_include_directories(dawnstar_port PRIVATE src)
target_link_libraries(dawnstar_port PRIVATE dawnstar_player dawnstar_render)
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

# M5 milestone smoke test -- see src/tests/m5_java_random_smoke.cpp and
# src/util/java_random.h (header-only, so no library needed).
add_executable(java_random_smoke src/tests/m5_java_random_smoke.cpp)
target_include_directories(java_random_smoke PRIVATE src)

# M6 milestone smoke test -- see src/tests/m6_dungeon_generator_smoke.cpp.
add_executable(dungeon_generator_smoke src/tests/m6_dungeon_generator_smoke.cpp)
target_link_libraries(dungeon_generator_smoke PRIVATE dawnstar_world)

# M7 milestone smoke test -- see src/tests/m7_img_archive_smoke.cpp.
add_executable(img_archive_smoke src/tests/m7_img_archive_smoke.cpp)
target_link_libraries(img_archive_smoke PRIVATE dawnstar_assets)

# M8 milestone smoke test -- see src/tests/m8_dialogue_smoke.cpp.
add_executable(dialogue_smoke src/tests/m8_dialogue_smoke.cpp)
target_link_libraries(dialogue_smoke PRIVATE dawnstar_assets)

# M9 milestone smoke test -- see src/tests/m9_corridor_plan_smoke.cpp.
add_executable(corridor_plan_smoke src/tests/m9_corridor_plan_smoke.cpp)
target_link_libraries(corridor_plan_smoke PRIVATE dawnstar_render)

# M10 milestone smoke test -- see src/tests/m10_frame_render_smoke.cpp.
add_executable(frame_render_smoke src/tests/m10_frame_render_smoke.cpp)
target_link_libraries(frame_render_smoke PRIVATE dawnstar_render)

# M11 milestone smoke test -- see src/tests/m11_player_creation_smoke.cpp.
add_executable(player_creation_smoke src/tests/m11_player_creation_smoke.cpp)
target_link_libraries(player_creation_smoke PRIVATE dawnstar_player)

# M12 milestone smoke test -- see src/tests/m12_player_save_smoke.cpp.
add_executable(player_save_smoke src/tests/m12_player_save_smoke.cpp)
target_link_libraries(player_save_smoke PRIVATE dawnstar_player)

# M13 milestone smoke test -- see src/tests/m13_player_movement_smoke.cpp.
add_executable(player_movement_smoke src/tests/m13_player_movement_smoke.cpp)
target_link_libraries(player_movement_smoke PRIVATE dawnstar_player)

# M14 milestone smoke test -- see src/tests/m14_player_combat_stats_smoke.cpp.
add_executable(player_combat_stats_smoke src/tests/m14_player_combat_stats_smoke.cpp)
target_link_libraries(player_combat_stats_smoke PRIVATE dawnstar_player)

# M15 milestone smoke test -- see src/tests/m15_monster_combat_smoke.cpp.
add_executable(monster_combat_smoke src/tests/m15_monster_combat_smoke.cpp)
target_link_libraries(monster_combat_smoke PRIVATE dawnstar_combat)

# M16 milestone smoke test -- see src/tests/m16_spellcasting_smoke.cpp.
add_executable(spellcasting_smoke src/tests/m16_spellcasting_smoke.cpp)
target_link_libraries(spellcasting_smoke PRIVATE dawnstar_combat)

# M17 milestone smoke test -- see src/tests/m17_summary_save_smoke.cpp.
add_executable(summary_save_smoke src/tests/m17_summary_save_smoke.cpp)
target_link_libraries(summary_save_smoke PRIVATE dawnstar_player)

# M18 milestone smoke test -- see src/tests/m18_use_item_smoke.cpp.
add_executable(use_item_smoke src/tests/m18_use_item_smoke.cpp)
target_link_libraries(use_item_smoke PRIVATE dawnstar_combat)

# M19 milestone smoke test -- see src/tests/m19_dungeon_view_smoke.cpp.
add_executable(dungeon_view_smoke src/tests/m19_dungeon_view_smoke.cpp)
target_link_libraries(dungeon_view_smoke PRIVATE dawnstar_world)

# M21 milestone smoke test -- see src/tests/m21_hud_renderer_smoke.cpp.
add_executable(hud_renderer_smoke src/tests/m21_hud_renderer_smoke.cpp)
target_link_libraries(hud_renderer_smoke PRIVATE dawnstar_render)

# M22 milestone smoke test -- see src/tests/m22_dungeon_runtime_smoke.cpp.
add_executable(dungeon_runtime_smoke src/tests/m22_dungeon_runtime_smoke.cpp)
target_link_libraries(dungeon_runtime_smoke PRIVATE dawnstar_dungeon)

# M23 milestone smoke test -- see
# src/tests/m23_movement_registry_wiring_smoke.cpp.
add_executable(movement_registry_wiring_smoke src/tests/m23_movement_registry_wiring_smoke.cpp)
target_link_libraries(movement_registry_wiring_smoke PRIVATE dawnstar_player)
