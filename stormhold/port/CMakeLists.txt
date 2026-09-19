cmake_minimum_required(VERSION 3.20)
project(stormhold_port CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4 /utf-8)
endif()

# M24: vendored stb_image (PNG decoding) -- see
# third_party/stb/PROVENANCE.md.
add_library(stb_image STATIC third_party/stb/stb_image_impl.cpp)
target_include_directories(stb_image PUBLIC third_party/stb)
if(MSVC)
  # Vendored third-party source, not held to this project's own /W4.
  target_compile_options(stb_image PRIVATE /W3)
endif()

# M2/M3/M4/M24: asset foundations -- see docs/PORT_ROADMAP.md and
# src/assets/.
add_library(stormhold_assets STATIC
  src/assets/character_data.cpp
  src/assets/decoded_image.cpp
  src/assets/dungeon_geometry.cpp
  src/assets/item_database.cpp
  src/assets/monster_database.cpp
  src/assets/monster_image_set.cpp
  src/assets/raw_image.cpp
  src/assets/shop_dialogue.cpp
  src/assets/spell_database.cpp
)
target_include_directories(stormhold_assets PUBLIC src)
target_link_libraries(stormhold_assets PRIVATE stb_image)

# M6: procedural dungeon generation -- see docs/PORT_ROADMAP.md and
# src/world/.
add_library(stormhold_world STATIC
  src/world/dungeon_generator.cpp
  src/world/warden.cpp
)
target_include_directories(stormhold_world PUBLIC src)
target_link_libraries(stormhold_world PUBLIC stormhold_assets)

# M14: monster runtime state/AI-support (everything except tick()/
# attack()'s Monster-side reads, which need Player too -- see
# src/combat/) -- see docs/PORT_ROADMAP.md and src/monster/.
add_library(stormhold_monster STATIC
  src/monster/monster_runtime.cpp
)
target_include_directories(stormhold_monster PUBLIC src)
target_link_libraries(stormhold_monster PUBLIC stormhold_assets stormhold_world)

# M16: the live per-level Monster/chest/dropped-item registry
# (WorldRegistry) plus Dungeon.java's own registry-management methods --
# see docs/PORT_ROADMAP.md and src/dungeon/. Depends on stormhold_monster/
# stormhold_world only -- stormhold_player/stormhold_combat depend on THIS
# module (M17), never the other way, so there's no cycle.
add_library(stormhold_dungeon STATIC
  src/dungeon/dungeon_runtime.cpp
)
target_include_directories(stormhold_dungeon PUBLIC src)
target_link_libraries(stormhold_dungeon PUBLIC stormhold_monster stormhold_world)

# M9: player character creation. M10: player movement. M17: player_movement
# .h/player_inventory.h's own dropped-item auto-loot/drop registry wiring
# -- see docs/PORT_ROADMAP.md and src/player/.
add_library(stormhold_player STATIC
  src/player/player_combat_stats.cpp
  src/player/player_creation.cpp
  src/player/player_inventory.cpp
  src/player/player_leveling.cpp
  src/player/player_movement.cpp
  src/player/player_save.cpp
  src/player/visible_objects.cpp
)
target_include_directories(stormhold_player PUBLIC src)
target_link_libraries(stormhold_player PUBLIC stormhold_assets stormhold_world stormhold_dungeon)

# M14: the two combat entry points needing both Player and Monster
# (Player.attack(Monster)/Monster.tick(Player,now)). M17: target.store()/
# Dungeon.spawnAmbushMonsters()'s own WorldRegistry wiring -- see
# docs/PORT_ROADMAP.md and src/combat/.
add_library(stormhold_combat STATIC
  src/combat/combat_resolution.cpp
)
target_include_directories(stormhold_combat PUBLIC src)
target_link_libraries(stormhold_combat PUBLIC stormhold_player stormhold_monster stormhold_dungeon)

# M21: GameCanvas.paintWalls()'s corridor wall-segment SELECTION logic
# (data only, no pixels yet -- see docs/PORT_ROADMAP.md). M25: GameRenderer,
# actually drawing those decisions into a real Backbuffer. M26:
# StatusBarPlan + GameRenderer::RenderStatusBars (paintStatusBars()) --
# needs stormhold_player for PlayerCombatStats::EffectiveStat(), the only
# reason this library now links it (no cycle: stormhold_player doesn't
# depend on stormhold_render) -- see src/render/.
add_library(stormhold_render STATIC
  src/graphics/bitmap_font.cpp
  src/render/corridor_render_plan.cpp
  src/render/game_renderer.cpp
  src/render/message_popup.cpp
  src/render/status_bar_plan.cpp
  src/render/visible_object_renderer.cpp
)
target_include_directories(stormhold_render PUBLIC src)
target_link_libraries(stormhold_render PUBLIC stormhold_dungeon stormhold_assets stormhold_player)

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

# M15 milestone smoke test -- see src/tests/m15_player_leveling_smoke.cpp.
add_executable(player_leveling_smoke src/tests/m15_player_leveling_smoke.cpp)
target_link_libraries(player_leveling_smoke PRIVATE stormhold_combat)

# M16 milestone smoke test -- see src/tests/m16_dungeon_runtime_smoke.cpp.
add_executable(dungeon_runtime_smoke src/tests/m16_dungeon_runtime_smoke.cpp)
target_link_libraries(dungeon_runtime_smoke PRIVATE stormhold_dungeon)

# M17 milestone smoke test -- see src/tests/m17_world_wiring_smoke.cpp.
add_executable(world_wiring_smoke src/tests/m17_world_wiring_smoke.cpp)
target_link_libraries(world_wiring_smoke PRIVATE stormhold_player)

# M18 milestone smoke test -- see src/tests/m18_registered_spawns_smoke.cpp.
add_executable(registered_spawns_smoke src/tests/m18_registered_spawns_smoke.cpp)
target_link_libraries(registered_spawns_smoke PRIVATE stormhold_dungeon)

# M19 milestone smoke test -- see src/tests/m19_warden_and_boss_respawn_smoke.cpp.
add_executable(warden_and_boss_respawn_smoke src/tests/m19_warden_and_boss_respawn_smoke.cpp)
target_link_libraries(warden_and_boss_respawn_smoke PRIVATE stormhold_player)

# M20 milestone smoke test -- see src/tests/m20_save_format_smoke.cpp.
add_executable(save_format_smoke src/tests/m20_save_format_smoke.cpp)
target_link_libraries(save_format_smoke PRIVATE stormhold_player stormhold_monster)

# M21 milestone smoke test -- see src/tests/m21_corridor_render_plan_smoke.cpp.
add_executable(corridor_render_plan_smoke src/tests/m21_corridor_render_plan_smoke.cpp)
target_link_libraries(corridor_render_plan_smoke PRIVATE stormhold_render)

# M23 milestone smoke test -- see src/tests/m23_backbuffer_blit_smoke.cpp.
# graphics/backbuffer.h is header-only but now includes assets/raw_image.h,
# so this needs stormhold_assets to link RawImage::Load's real body.
add_executable(backbuffer_blit_smoke src/tests/m23_backbuffer_blit_smoke.cpp)
target_include_directories(backbuffer_blit_smoke PRIVATE src)
target_link_libraries(backbuffer_blit_smoke PRIVATE stormhold_assets)

# M24 milestone smoke test -- see src/tests/m24_decoded_image_smoke.cpp.
add_executable(decoded_image_smoke src/tests/m24_decoded_image_smoke.cpp)
target_include_directories(decoded_image_smoke PRIVATE src)
target_link_libraries(decoded_image_smoke PRIVATE stormhold_assets)

# M25 milestone smoke test -- see src/tests/m25_game_renderer_smoke.cpp.
add_executable(game_renderer_smoke src/tests/m25_game_renderer_smoke.cpp)
target_link_libraries(game_renderer_smoke PRIVATE stormhold_render stormhold_player)

# M26 milestone smoke test -- see src/tests/m26_status_bar_smoke.cpp.
add_executable(status_bar_smoke src/tests/m26_status_bar_smoke.cpp)
target_link_libraries(status_bar_smoke PRIVATE stormhold_render stormhold_player)

# M27 milestone smoke test -- see src/tests/m27_visible_objects_smoke.cpp.
add_executable(visible_objects_smoke src/tests/m27_visible_objects_smoke.cpp)
target_link_libraries(visible_objects_smoke PRIVATE stormhold_player)

# M28 milestone smoke test -- see src/tests/m28_visible_object_renderer_smoke.cpp.
add_executable(visible_object_renderer_smoke src/tests/m28_visible_object_renderer_smoke.cpp)
target_include_directories(visible_object_renderer_smoke PRIVATE src)
target_link_libraries(visible_object_renderer_smoke PRIVATE stormhold_render stormhold_player)

# M29 milestone smoke test -- see src/tests/m29_hud_state_smoke.cpp.
# render/hud_state.h is header-only (a small facing/isNpcDialogueDue
# helper, no pixels yet) but pulls in player/player_state.h and
# world/warden.h, so this links stormhold_player/stormhold_world for
# their real symbols even though this test itself only touches the
# header's own inline functions.
add_executable(hud_state_smoke src/tests/m29_hud_state_smoke.cpp)
target_include_directories(hud_state_smoke PRIVATE src)
target_link_libraries(hud_state_smoke PRIVATE stormhold_player stormhold_world)

# M30 milestone smoke test -- see src/tests/m30_message_popup_smoke.cpp.
add_executable(message_popup_smoke src/tests/m30_message_popup_smoke.cpp)
target_link_libraries(message_popup_smoke PRIVATE stormhold_render)
