cmake_minimum_required(VERSION 3.20)
project(stormhold_port CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4 /utf-8)
endif()

# --- building something other people can download ---------------------
#
# `STORMHOLD_VERSION` is what the launcher prints in its corner. A local
# build leaves it at the "-dev" default; the release workflow passes the
# git tag with its "stormhold-v" prefix stripped.
set(STORMHOLD_VERSION "0.1.0-dev" CACHE STRING "Version string shown by the launcher")

# Where the launcher checks for newer builds, and the tag prefix that picks
# Stormhold's own releases out of a repo that also publishes Dawnstar's and
# Oblivion's (see src/launcher/update_check.h). Cache variables so a fork
# updates from its own repository without editing source. A version
# carrying "-dev" does not check at all -- see src/launcher/updater.h.
set(STORMHOLD_UPDATE_OWNER "samioan" CACHE STRING "GitHub owner to check for updates")
set(STORMHOLD_UPDATE_REPO "tes-travels-remastered" CACHE STRING "GitHub repo to check for updates")
set(STORMHOLD_UPDATE_TAG_PREFIX "stormhold-v" CACHE STRING "Release tag prefix identifying a Stormhold build")

# The shipping build: no smoke tests, so a release compiles the two
# executables that actually get zipped instead of every milestone's own
# test binary too. Does not change any game behaviour.
option(STORMHOLD_DIST "Build only the shipped executables (launcher + game)" OFF)

# Static CRT for release builds. This is the single thing that most decides
# whether "unzip and play" works: with the dynamic runtime, every machine
# without a matching Visual C++ redistributable gets a missing-DLL dialog
# instead of a game. Debug builds stay dynamic so the usual debugging tools
# behave normally.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
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
  src/assets/dungeon_names.cpp
  src/assets/gdr_font.cpp
  src/assets/item_database.cpp
  src/assets/monster_database.cpp
  src/assets/monster_image_set.cpp
  src/assets/raw_image.cpp
  src/assets/shop_dialogue.cpp
  src/assets/spell_database.cpp
)
target_include_directories(stormhold_assets PUBLIC src)
target_link_libraries(stormhold_assets PRIVATE stb_image)

# M6: procedural dungeon generation. M52: game_advancement.cpp (ESGame.
# getGameAdvancementLevel()/checkOpenAndPopulateDungeons(), the dungeon
# zone-gating system). M53: shop_state.h (Shop.java's live per-NPC
# quest-economy state, header-only -- see its own class comment) -- see
# docs/PORT_ROADMAP.md and src/world/.
add_library(stormhold_world STATIC
  src/world/dungeon_generator.cpp
  src/world/game_advancement.cpp
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
# (WorldRegistry) plus Dungeon.java's own registry-management methods.
# M49: world_save.cpp (ESGame.writeAllLevelRegistries()/
# readPerLevelRecords(), the WorldRegistry half of the real save format)
# -- see docs/PORT_ROADMAP.md and src/dungeon/. Depends on
# stormhold_monster/stormhold_world only -- stormhold_player/
# stormhold_combat depend on THIS module (M17), never the other way, so
# there's no cycle.
add_library(stormhold_dungeon STATIC
  src/dungeon/dungeon_runtime.cpp
  src/dungeon/world_save.cpp
)
target_include_directories(stormhold_dungeon PUBLIC src)
target_link_libraries(stormhold_dungeon PUBLIC stormhold_monster stormhold_world)

# M9: player character creation. M10: player movement. M17: player_movement
# .h/player_inventory.h's own dropped-item auto-loot/drop registry wiring.
# M47: death_sequence.cpp (GameCanvas.tickDeathAndRegen()/run()'s own
# facing!=1 death-respawn dispatch). M48: movement_messages.cpp
# (GameCanvas.resolveMovementSideEffects()). M50: game_save.cpp
# (ESGame.saveGameState()/loadGameState(), the file-I/O framing around
# PlayerSave (this same library) and WorldSave (stormhold_dungeon)). M56:
# shop_interaction.cpp (Shop.dialogue()'s dispatcher, scoped to shops 0-3,
# the quest-turn-in shopkeepers -- lives here, not stormhold_world, since
# it needs PlayerState directly). M61: player_inventory.cpp's own
# ItemTooltip/CanEquipOrUnequip/IsScrollCategory/CanLearnSpellFromScroll/
# LearnSpellFromScroll/CanUseItem/UseItem additions -- the last un-ported
# pieces of Player's inventory-ACTION logic, still deliberately not wired
# into any UI screen -- see docs/PORT_ROADMAP.md and src/player/.
add_library(stormhold_player STATIC
  src/player/camp_state.cpp
  src/player/death_sequence.cpp
  src/player/game_save.cpp
  src/player/movement_messages.cpp
  src/player/player_combat_stats.cpp
  src/player/player_creation.cpp
  src/player/player_inventory.cpp
  src/player/player_leveling.cpp
  src/player/player_movement.cpp
  src/player/player_save.cpp
  src/player/shop_interaction.cpp
  src/player/visible_objects.cpp
)
target_include_directories(stormhold_player PUBLIC src)
target_link_libraries(stormhold_player PUBLIC stormhold_assets stormhold_world stormhold_dungeon)

# M14: the two combat entry points needing both Player and Monster
# (Player.attack(Monster)/Monster.tick(Player,now)). M17: target.store()/
# Dungeon.spawnAmbushMonsters()'s own WorldRegistry wiring. M46:
# spell_casting.cpp (Player.castOnSelf/castOnMonster/cycleSelectedSpell +
# GameCanvas.resolveSpellCastInput/resolveSpellCycleInput) -- see
# docs/PORT_ROADMAP.md and src/combat/.
add_library(stormhold_combat STATIC
  src/combat/combat_resolution.cpp
  src/combat/spell_casting.cpp
)
target_include_directories(stormhold_combat PUBLIC src)
target_link_libraries(stormhold_combat PUBLIC stormhold_player stormhold_monster stormhold_dungeon)

# M21: GameCanvas.paintWalls()'s corridor wall-segment SELECTION logic
# (data only, no pixels yet -- see docs/PORT_ROADMAP.md). M25: GameRenderer,
# actually drawing those decisions into a real Backbuffer. M26:
# StatusBarPlan + GameRenderer::RenderStatusBars (paintStatusBars()) --
# needs stormhold_player for PlayerCombatStats::EffectiveStat(), the only
# reason this library now links it (no cycle: stormhold_player doesn't
# depend on stormhold_render). M60: ui/npc_dialogue.cpp, the blocking NPC-
# greeting screen (npcHelloUI's own simplified port-only counterpart) --
# see src/render/. M62: ui/inventory_ui.cpp, the item-list + per-slot
# action-menu screens (openInventory's own dispatch, the last branch of
# M41's dispatch web). M63: ui/pause_menu.cpp, the in-game Options/pause
# menu (Stats/Inventory/Skills/Spells/Save/Load) -- the first thing in this
# library to need stormhold_combat (SpellCasting::KnownSpellsSummary/
# NthKnownSpellId/SpellTooltip, the Spells screen's own view-model
# builders), no cycle since stormhold_combat never depends back on
# stormhold_render. M64: ui/npc_choices_menu.cpp, the interactive
# Train/Give/Befriend/Threaten/Kill follow-up menu for shops 0-3
# (player/shop_interaction.h's own ShopInteraction::QuestShopDialogue,
# already in stormhold_player -- no new library dependency).
add_library(stormhold_render STATIC
  src/graphics/bitmap_font.cpp
  src/render/corridor_render_plan.cpp
  src/render/flash_overlay.cpp
  src/render/game_renderer.cpp
  src/render/message_popup.cpp
  src/render/status_bar_plan.cpp
  src/render/visible_object_renderer.cpp
  src/ui/boot_splash.cpp
  src/ui/inventory_ui.cpp
  src/ui/level_up_menu.cpp
  src/ui/menu_flow.cpp
  src/ui/npc_choices_menu.cpp
  src/ui/npc_dialogue.cpp
  src/ui/pause_menu.cpp
)
target_include_directories(stormhold_render PUBLIC src)
# M74: gdi32 PUBLIC (not PRIVATE), same reasoning as dawnstar's own M58:
# src/graphics/bitmap_font.cpp's own real-font text rendering needs it
# directly, and PUBLIC here makes every executable that links
# stormhold_render -- transitively including every smoke test below, not
# just stormhold_port itself -- automatically link it too, without each one
# needing its own explicit gdi32 entry.
target_link_libraries(stormhold_render PUBLIC stormhold_dungeon stormhold_assets stormhold_player stormhold_combat gdi32)

# M1: real tick loop + backbuffer + present. Software-rendered raw Win32
# (no SDL/D3D/GL dependency), matching the sibling shadowkey-decomp/
# dawnstar ports' platform layer.
add_executable(stormhold_port WIN32
  src/main.cpp
  src/platform/win32/exe_dir.cpp
  src/platform/win32/window.cpp
)
target_include_directories(stormhold_port PRIVATE src)
target_compile_definitions(stormhold_port PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
# M34: the live pipeline itself -- stormhold_render pulls in
# stormhold_dungeon/stormhold_assets/stormhold_player transitively
# (PUBLIC), listed explicitly anyway for clarity at this, the actual
# executable's own link line.
target_link_libraries(stormhold_port PRIVATE stormhold_render stormhold_player stormhold_dungeon stormhold_combat)
if(MSVC)
  target_compile_options(stormhold_port PRIVATE /W4)
endif()

# --- the launcher -------------------------------------------------------
#
# The "unzip and play" front end for the game; see
# src/launcher/launcher_main.cpp for why it draws its own panel rather than
# using a dialog template, and src/launcher/banner.h for why its artwork
# decodes through stb_image (already vendored for the game's own PNG
# assets) rather than WIC. Ported from dawnstar's own launcher (itself from
# shadowkey-decomp) -- see each launcher/ file's own header comment for
# what's shared verbatim and what's genuinely different (no font, a .jar
# instead of a folder to search, a tag-prefix-filtered update check for a
# repo that publishes more than one game).
#
# The artwork the .rc embeds (src/launcher/assets/banner.png and
# stormhold.ico) is generated from src/launcher/assets/banner_source.jpg by:
#
#   python tools/make_banner.py stormhold/port/src/launcher/assets/banner_source.jpg
#   python tools/make_icon.py   stormhold/port/src/launcher/assets/banner_source.jpg
configure_file(src/launcher/version.h.in
               "${CMAKE_BINARY_DIR}/generated/launcher/version.h" @ONLY)

# third_party/puff (zlib's reference inflate), vendored for the launcher's
# zip reader -- see third_party/puff/PROVENANCE.md.
add_library(puff STATIC third_party/puff/puff.c)
target_include_directories(puff PUBLIC third_party/puff)
if(MSVC)
  target_compile_options(puff PRIVATE /W3)  # vendored third-party source
endif()

add_executable(stormhold_launcher WIN32
  src/launcher/launcher_main.cpp
  src/launcher/banner.cpp
  src/launcher/install.cpp
  src/launcher/launcher_config.cpp
  src/launcher/update_check.cpp
  src/launcher/updater.cpp
  src/launcher/zip_reader.cpp
  src/launcher/launcher.rc
  src/assets/gdr_font.cpp
  src/platform/win32/exe_dir.cpp
)
set_target_properties(stormhold_launcher PROPERTIES OUTPUT_NAME "Stormhold")
target_include_directories(stormhold_launcher PRIVATE src "${CMAKE_BINARY_DIR}/generated")
# ole32 is COM (IFileOpenDialog, the file pickers); winhttp is the update
# check/download. No windowscodecs -- the banner decodes through stb_image.
target_link_libraries(stormhold_launcher PRIVATE stb_image puff comctl32 ole32 winhttp)
target_compile_definitions(stormhold_launcher PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
# The manifest is a resource in launcher.rc (see the comment there for why);
# CMake's own generated manifest has to be turned off so only one
# RT_MANIFEST ends up in the image.
target_link_options(stormhold_launcher PRIVATE /MANIFEST:NO)

# Launcher smoke test -- see src/tests/m_launcher_smoke.cpp. Headless: it
# exercises the launcher's decode/config/discovery layer directly, so it
# never opens a window and needs no game data or network. Compiles the
# launcher's own sources rather than linking a library, same as
# stormhold_launcher itself.
add_executable(launcher_smoke
  src/tests/m_launcher_smoke.cpp
  src/assets/gdr_font.cpp
  src/launcher/banner.cpp
  src/launcher/install.cpp
  src/launcher/launcher_config.cpp
  src/launcher/update_check.cpp
  src/launcher/updater.cpp
  src/launcher/zip_reader.cpp
)
target_include_directories(launcher_smoke PRIVATE src "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(launcher_smoke PRIVATE stb_image puff ole32 winhttp)
target_compile_definitions(launcher_smoke PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

if(NOT STORMHOLD_DIST)

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

# M31 milestone smoke test -- see src/tests/m31_hud_renderer_smoke.cpp.
add_executable(hud_renderer_smoke src/tests/m31_hud_renderer_smoke.cpp)
target_link_libraries(hud_renderer_smoke PRIVATE stormhold_render)

# M32 milestone smoke test -- see src/tests/m32_minimap_sample_smoke.cpp.
add_executable(minimap_sample_smoke src/tests/m32_minimap_sample_smoke.cpp)
target_link_libraries(minimap_sample_smoke PRIVATE stormhold_dungeon)

# M33 milestone smoke test -- see src/tests/m33_minimap_renderer_smoke.cpp.
add_executable(minimap_renderer_smoke src/tests/m33_minimap_renderer_smoke.cpp)
target_link_libraries(minimap_renderer_smoke PRIVATE stormhold_render)

# M35 milestone smoke test -- see src/tests/m35_status_countdowns_smoke.cpp.
add_executable(status_countdowns_smoke src/tests/m35_status_countdowns_smoke.cpp)
target_link_libraries(status_countdowns_smoke PRIVATE stormhold_player)

# M36 milestone smoke test -- see src/tests/m36_tick_per_second_smoke.cpp.
add_executable(tick_per_second_smoke src/tests/m36_tick_per_second_smoke.cpp)
target_link_libraries(tick_per_second_smoke PRIVATE stormhold_player)

# M37 milestone smoke test -- see src/tests/m37_monster_ai_tick_smoke.cpp.
add_executable(monster_ai_tick_smoke src/tests/m37_monster_ai_tick_smoke.cpp)
target_link_libraries(monster_ai_tick_smoke PRIVATE stormhold_combat stormhold_player)

# M38 milestone smoke test -- see src/tests/m38_target_monster_smoke.cpp.
add_executable(target_monster_smoke src/tests/m38_target_monster_smoke.cpp)
target_link_libraries(target_monster_smoke PRIVATE stormhold_player)

# M39 milestone smoke test -- see src/tests/m39_attack_input_smoke.cpp.
add_executable(attack_input_smoke src/tests/m39_attack_input_smoke.cpp)
target_link_libraries(attack_input_smoke PRIVATE stormhold_combat stormhold_player)

# M40 milestone smoke test -- see src/tests/m40_menu_flow_smoke.cpp.
add_executable(menu_flow_smoke src/tests/m40_menu_flow_smoke.cpp)
target_link_libraries(menu_flow_smoke PRIVATE stormhold_render stormhold_player)

# M42 milestone smoke test -- see src/tests/m42_camp_state_smoke.cpp.
add_executable(camp_state_smoke src/tests/m42_camp_state_smoke.cpp)
target_link_libraries(camp_state_smoke PRIVATE stormhold_player)

# M43 milestone smoke test -- see src/tests/m43_chest_interaction_smoke.cpp.
add_executable(chest_interaction_smoke src/tests/m43_chest_interaction_smoke.cpp)
target_link_libraries(chest_interaction_smoke PRIVATE stormhold_player)

# M45 milestone smoke test -- see src/tests/m45_flash_overlay_smoke.cpp.
add_executable(flash_overlay_smoke src/tests/m45_flash_overlay_smoke.cpp)
target_link_libraries(flash_overlay_smoke PRIVATE stormhold_render)

# M46 milestone smoke test -- see src/tests/m46_spell_casting_smoke.cpp.
add_executable(spell_casting_smoke src/tests/m46_spell_casting_smoke.cpp)
target_link_libraries(spell_casting_smoke PRIVATE stormhold_combat stormhold_player)

# M47 milestone smoke test -- see src/tests/m47_death_sequence_smoke.cpp.
add_executable(death_sequence_smoke src/tests/m47_death_sequence_smoke.cpp)
target_link_libraries(death_sequence_smoke PRIVATE stormhold_player)

# M48 milestone smoke test -- see src/tests/m48_movement_messages_smoke.cpp.
add_executable(movement_messages_smoke src/tests/m48_movement_messages_smoke.cpp)
target_link_libraries(movement_messages_smoke PRIVATE stormhold_player)

# M49 milestone smoke test -- see src/tests/m49_world_save_smoke.cpp.
add_executable(world_save_smoke src/tests/m49_world_save_smoke.cpp)
target_link_libraries(world_save_smoke PRIVATE stormhold_dungeon)

# M50 milestone smoke test -- see src/tests/m50_game_save_smoke.cpp.
add_executable(game_save_smoke src/tests/m50_game_save_smoke.cpp)
target_link_libraries(game_save_smoke PRIVATE stormhold_render stormhold_player)

# M52 milestone smoke test -- see src/tests/m52_game_advancement_smoke.cpp.
add_executable(game_advancement_smoke src/tests/m52_game_advancement_smoke.cpp)
target_link_libraries(game_advancement_smoke PRIVATE stormhold_player)

# M53 milestone smoke test -- see src/tests/m53_shop_state_smoke.cpp.
add_executable(shop_state_smoke src/tests/m53_shop_state_smoke.cpp)
target_link_libraries(shop_state_smoke PRIVATE stormhold_world)

# M56 milestone smoke test -- see src/tests/m56_shop_interaction_smoke.cpp.
add_executable(shop_interaction_smoke src/tests/m56_shop_interaction_smoke.cpp)
target_link_libraries(shop_interaction_smoke PRIVATE stormhold_player)

# M60 milestone smoke test -- see src/tests/m60_npc_dialogue_smoke.cpp.
add_executable(npc_dialogue_smoke src/tests/m60_npc_dialogue_smoke.cpp)
target_link_libraries(npc_dialogue_smoke PRIVATE stormhold_render stormhold_player)

# M61 milestone smoke test -- see src/tests/m61_inventory_actions_smoke.cpp.
add_executable(inventory_actions_smoke src/tests/m61_inventory_actions_smoke.cpp)
target_link_libraries(inventory_actions_smoke PRIVATE stormhold_player)

# M62 milestone smoke test -- see src/tests/m62_inventory_ui_smoke.cpp.
add_executable(inventory_ui_smoke src/tests/m62_inventory_ui_smoke.cpp)
target_link_libraries(inventory_ui_smoke PRIVATE stormhold_render stormhold_player)

# M63 milestone smoke test -- see src/tests/m63_pause_menu_smoke.cpp.
add_executable(pause_menu_smoke src/tests/m63_pause_menu_smoke.cpp)
target_link_libraries(pause_menu_smoke PRIVATE stormhold_render stormhold_player stormhold_combat)

# M64 milestone smoke test -- see src/tests/m64_npc_choices_menu_smoke.cpp.
add_executable(npc_choices_menu_smoke src/tests/m64_npc_choices_menu_smoke.cpp)
target_link_libraries(npc_choices_menu_smoke PRIVATE stormhold_render stormhold_player)

# M65 milestone smoke test -- see src/tests/m65_beneca_choices_menu_smoke.cpp.
add_executable(beneca_choices_menu_smoke src/tests/m65_beneca_choices_menu_smoke.cpp)
target_link_libraries(beneca_choices_menu_smoke PRIVATE stormhold_render stormhold_player)

# M66 milestone smoke test -- see src/tests/m66_helga_choices_menu_smoke.cpp.
add_executable(helga_choices_menu_smoke src/tests/m66_helga_choices_menu_smoke.cpp)
target_link_libraries(helga_choices_menu_smoke PRIVATE stormhold_render stormhold_player)

# M78 smoke test -- see src/tests/m78_device_font_smoke.cpp. Skips when the
# (never-committed) Nokia fonts aren't in port/assets/fonts/.
add_executable(device_font_smoke src/tests/m78_device_font_smoke.cpp)
target_link_libraries(device_font_smoke PRIVATE stormhold_render)

# M77 smoke test -- see src/tests/m77_level_up_menu_smoke.cpp.
add_executable(level_up_menu_smoke src/tests/m77_level_up_menu_smoke.cpp)
target_link_libraries(level_up_menu_smoke PRIVATE stormhold_render stormhold_player)

# M67 milestone smoke test -- see src/tests/m67_paint_unknown_b_smoke.cpp.
add_executable(paint_unknown_b_smoke src/tests/m67_paint_unknown_b_smoke.cpp)
target_link_libraries(paint_unknown_b_smoke PRIVATE stormhold_render)

# M73 milestone smoke test -- see src/tests/m73_boot_splash_smoke.cpp.
add_executable(boot_splash_smoke src/tests/m73_boot_splash_smoke.cpp)
target_link_libraries(boot_splash_smoke PRIVATE stormhold_render)

endif()  # NOT STORMHOLD_DIST
