cmake_minimum_required(VERSION 3.20)
project(dawnstar_port CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4 /utf-8)
endif()

# --- building something other people can download ---------------------
#
# `DAWNSTAR_VERSION` is what the launcher prints in its corner. A local
# build leaves it at the "-dev" default; the release workflow passes the
# git tag with its "dawnstar-v" prefix stripped.
set(DAWNSTAR_VERSION "0.1.0-dev" CACHE STRING "Version string shown by the launcher")

# Where the launcher checks for newer builds, and the tag prefix that picks
# Dawnstar's own releases out of a repo that also publishes Oblivion's and
# Stormhold's (see src/launcher/update_check.h). Cache variables so a fork
# updates from its own repository without editing source. A version
# carrying "-dev" does not check at all -- see src/launcher/updater.h.
set(DAWNSTAR_UPDATE_OWNER "samioan" CACHE STRING "GitHub owner to check for updates")
set(DAWNSTAR_UPDATE_REPO "tes-travels-remastered" CACHE STRING "GitHub repo to check for updates")
set(DAWNSTAR_UPDATE_TAG_PREFIX "dawnstar-v" CACHE STRING "Release tag prefix identifying a Dawnstar build")

# The shipping build: no smoke tests, so a release compiles the two
# executables that actually get zipped instead of every milestone's own
# test binary too. Does not change any game behaviour.
option(DAWNSTAR_DIST "Build only the shipped executables (launcher + game)" OFF)

# Static CRT for release builds. This is the single thing that most decides
# whether "unzip and play" works: with the dynamic runtime, every machine
# without a matching Visual C++ redistributable gets a missing-DLL dialog
# instead of a game. Debug builds stay dynamic so the usual debugging tools
# behave normally.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
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
  src/assets/monster_image_names.cpp
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

# M9/M10/M21/M26/M27/M28/M29/M30/M31: corridor rendering + the HUD
# status-bar overlay + the far/mid/close monster-chest-dropped-item-NPC
# sprite renderer (now also the closest slot's monster sprite/stairs
# icon, NPC portraits, the minimap, the message popup, and the bottom
# hotbar panel) -- see docs/PORT_ROADMAP.md, src/render/, src/world/
# dungeon_view.h. M29's minimap_renderer.cpp is the second module (after
# player/player_movement.cpp) needing both a live WorldRegistry
# (dawnstar_dungeon) and rendering (dawnstar_render) at once -- no cycle
# results, since dawnstar_dungeon doesn't depend back on dawnstar_render.
# M30's graphics/bitmap_font.cpp (a hand-authored pixel font -- see its
# own doc comment) is compiled directly into this library too, the same
# "no separate library needed for a header-only-adjacent utility only
# one consumer needs" precedent src/platform/win32/window.cpp already
# set for dawnstar_port itself. M31's hotbar_renderer.cpp reuses that
# same font (digits, extended for this milestone). M37's src/ui/screen.cpp
# (Screen.java's generic list/message/prompt-list UI primitive) needs
# nothing beyond what this library already bundles (Backbuffer/BitmapFont/
# MessagePopup::WordWrap), so it's folded in here too rather than
# standing up a whole new library for one self-contained class. M38's
# src/ui/menu_flow.cpp (the real main-menu/help/credits/quit-confirm
# navigation flow built on top of Screen) needs nothing further either.
# M39's src/ui/options_menu.cpp (the real in-game options menu) needs
# player/player_combat_stats.h (EffectiveStat/HasAilment) on top of
# that -- already available transitively via dawnstar_player below.
# M40's src/ui/name_entry.cpp (the raw name-entry widget) needs nothing
# further; src/ui/character_creation_flow.cpp needs
# player/player_creation.h (CreateCharacter/BuildCreationSummary), same
# transitive-availability reasoning as options_menu.cpp above. M41's real
# Inventory/Skills/Spells actions need player/player_inventory.h/
# player/player_spellcasting.h on top of that -- also already available
# transitively via dawnstar_player. M42's src/ui/loading_screen.cpp
# (LoadingScreen.java's modes 8-11 progress bars) needs nothing further
# either -- Backbuffer + BitmapFont, both already in this library.
add_library(dawnstar_render STATIC
  src/graphics/bitmap_font.cpp
  src/render/corridor_render_plan.cpp
  src/render/frame_renderer.cpp
  src/render/hotbar_renderer.cpp
  src/render/message_popup.cpp
  src/render/minimap_renderer.cpp
  src/render/visible_object_renderer.cpp
  src/ui/screen.cpp
  src/ui/menu_flow.cpp
  src/ui/options_menu.cpp
  src/ui/name_entry.cpp
  src/ui/character_creation_flow.cpp
  src/ui/loading_screen.cpp
  src/ui/boot_splash.cpp
  src/ui/level_up_menu.cpp
  src/ui/npc_menu.cpp
)
target_include_directories(dawnstar_render PUBLIC src)
# M46: ui/npc_menu.cpp drives ShopInteraction::Dialogue (dawnstar_npc, which
# depends only on dawnstar_player/dawnstar_assets -- no cycle back to render).
target_link_libraries(dawnstar_render PUBLIC dawnstar_world dawnstar_player dawnstar_dungeon dawnstar_npc)

# M11/M12/M13/M14/M16/M17/M23/M25/M28: player character creation + full
# save format + movement (now also touching the live world registry for
# dropped-item auto-loot/roaming-monster cleanup, and detecting an NPC
# directly in front) + combat/skill stat math + inventory/equip
# management + self-targeted spellcasting + the lightweight summary save
# format + the 13-slot visibleObjects cache (data model only -- no
# sprite drawing yet) -- see docs/PORT_ROADMAP.md and src/player/.
add_library(dawnstar_player STATIC
  src/player/player_creation.cpp
  src/player/player_save.cpp
  src/player/player_movement.cpp
  src/player/player_camp.cpp
  src/player/player_combat_stats.cpp
  src/player/player_inventory.cpp
  src/player/player_spellcasting.cpp
  src/player/visible_objects.cpp
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

# M15/M16/M32/M33: the combat entry points needing both Player and
# Monster, plus (M32/M33) the attack/spellcast-action/death-resolution
# tick orchestration, which additionally needs dawnstar_render
# (render/message_popup.h) -- see docs/PORT_ROADMAP.md and src/combat/.
# No cycle: dawnstar_render doesn't depend back on dawnstar_combat.
add_library(dawnstar_combat STATIC
  src/combat/combat_resolution.cpp
  src/combat/combat_tick.cpp
)
target_include_directories(dawnstar_combat PUBLIC src)
target_link_libraries(dawnstar_combat PUBLIC dawnstar_player dawnstar_monster dawnstar_dungeon dawnstar_render)

# M45: Shop.java's own static state + the real dialogue()
# line-selection logic (buy/sell/quest-turn-in/rumor dispatch) -- the gap
# M34's own InteractTick doc comment (and M8's ShopDialogue, raw
# npcstrings.dat text only) flagged as still open. Needs dawnstar_player
# (PlayerCombatStats::SkillValue/RollOutcome/GainSkillExp,
# PlayerInventory::AddItem/RemoveSlot/AddGold, PlayerMovement::
# HasCampMark/WarpToCampMark) and dawnstar_assets (ItemDatabase/
# CharacterData/ShopDialogue) -- see docs/PORT_ROADMAP.md's M45 entry and
# src/npc/.
add_library(dawnstar_npc STATIC
  src/npc/shop_interaction.cpp
)
target_include_directories(dawnstar_npc PUBLIC src)
target_link_libraries(dawnstar_npc PUBLIC dawnstar_player dawnstar_assets)

# M34/M45: the interact action (chest looting, now also the real NPC
# "talk" greeting), needing dawnstar_player (PlayerInventory::AddItem) +
# dawnstar_dungeon (chest/dropped-item registry) + dawnstar_render (the
# found-item/inventory-full popups) + dawnstar_npc (ShopInteraction::
# Dialogue) at once -- same "own small module, no cycle" shape as
# dawnstar_combat above. See docs/PORT_ROADMAP.md and src/interact/.
add_library(dawnstar_interact STATIC
  src/interact/interact_tick.cpp
)
target_include_directories(dawnstar_interact PUBLIC src)
target_link_libraries(dawnstar_interact PUBLIC dawnstar_player dawnstar_dungeon dawnstar_render dawnstar_npc)

# M35: the camp action (enterCampState + the campState 1/2/3 tick state
# machine), needing dawnstar_player (PlayerCamp::Rest) + dawnstar_dungeon
# (DungeonRuntime::TrySpawnMonsterNear) + dawnstar_render (the "Cannot
# Camp!"/"Rest disturbed!"/"Rest complete!" popups) at once -- same "own
# small module, no cycle" shape as dawnstar_combat/dawnstar_interact
# above. See docs/PORT_ROADMAP.md and src/camp/.
add_library(dawnstar_camp STATIC
  src/camp/camp_tick.cpp
)
target_include_directories(dawnstar_camp PUBLIC src)
target_link_libraries(dawnstar_camp PUBLIC dawnstar_player dawnstar_dungeon dawnstar_render)

# M44: GameCanvas.run()'s own timed tail (tickStatusCountdowns +
# tickPerSecond/the ambush spawner), needing dawnstar_player
# (PlayerCombatStats::HasAilment, PlayerInventory::FindSlotOf/RemoveSlot)
# + dawnstar_dungeon (DungeonRuntime::TrySpawnMonsterNear) +
# dawnstar_render (the "Enemy/arrived!" popup) at once -- same "own small
# module, no cycle" shape as dawnstar_combat/dawnstar_interact/
# dawnstar_camp above. See docs/PORT_ROADMAP.md's M44 entry.
add_library(dawnstar_passive STATIC
  src/passive/passive_tick.cpp
)
target_include_directories(dawnstar_passive PUBLIC src)
target_link_libraries(dawnstar_passive PUBLIC dawnstar_player dawnstar_dungeon dawnstar_render)

# M22/M24: the live per-level monster/chest/dropped-item registry (needs
# both World and Monster), now also populated from world generation
# itself -- see docs/PORT_ROADMAP.md and src/dungeon/.
add_library(dawnstar_dungeon STATIC
  src/dungeon/dungeon_runtime.cpp
)
target_include_directories(dawnstar_dungeon PUBLIC src)
target_link_libraries(dawnstar_dungeon PUBLIC dawnstar_world dawnstar_monster)

# M42: the real character+world save/load persistence -- ESGame.java's own
# saveGameState()/loadGameState() plus every helper those two call
# (writeMasterListsToRecordStore/readMasterListRecords/
# writeOtherStateInfoToBytes/readOtherStateInfo/maxWriteSize/
# getRSNameNotInUse/getLastGoodRSName/cleanupRecordStores/resumeGame/
# openAndRepopulateDungeons/getGameAdvancementLevel), with MIDP's
# RecordStore substituted by one plain file per store -- see src/save/'s
# own header comment and docs/PORT_ROADMAP.md's M42 entry. Needs
# dawnstar_player (PlayerSave::ToBytes/FromBytes + PlayerState) and,
# transitively through it, dawnstar_dungeon (WorldRegistry/
# DungeonRuntime::RefreshTileFlags) and dawnstar_monster
# (MonsterRuntime::ReadFrom/WriteTo/ToBytes/FromBytes). Nothing depends
# back on dawnstar_save, so no cycle results.
add_library(dawnstar_save STATIC
  src/save/game_save.cpp
)
target_include_directories(dawnstar_save PUBLIC src)
target_link_libraries(dawnstar_save PUBLIC dawnstar_player)

# M1/M20: real tick loop + backbuffer + present, now wired to the real
# asset/world/player/render pipeline -- see docs/PORT_ROADMAP.md and
# src/engine/, src/graphics/, src/platform/win32/, src/main.cpp. M32
# adds dawnstar_combat: the attack action needs CombatResolution::
# PlayerAttack/MonsterRuntime::OnDeath directly from main.cpp's own
# tick-dispatch orchestration. M33 additionally wires spellcasting the
# same way (CombatTick::ProcessSpellCast/CycleSpell). M35 adds
# dawnstar_camp (CampTick::TryEnterCamp/TickCampState). M42 adds
# dawnstar_save: the Options menu's real "Save Game"/"Load Game" actions
# run GameSave::SaveGameState/LoadGameState/ResumeGame from main.cpp's own
# display orchestration (see ui/options_menu.h's own
# OptionsMenuAction::SaveGame/LoadGame doc comment on why that's ESGame's
# job, not the Screen's). M45 adds dawnstar_npc: InteractTick's own
# npcInSight branch now calls the real ShopInteraction::Dialogue, and
# main.cpp constructs the ShopState it's threaded through directly (the
# same "constructed and reset alongside OtherStateInfo" pattern its own
# doc comment describes).
add_executable(dawnstar_port WIN32
  src/main.cpp
  src/platform/win32/window.cpp
)
target_include_directories(dawnstar_port PRIVATE src)
target_link_libraries(dawnstar_port PRIVATE dawnstar_player dawnstar_render dawnstar_combat dawnstar_interact dawnstar_camp dawnstar_passive dawnstar_save dawnstar_npc)
if(WIN32)
  target_compile_definitions(dawnstar_port PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
  target_link_libraries(dawnstar_port PRIVATE user32 gdi32)
endif()

# --- the launcher -------------------------------------------------------
#
# The "unzip and play" front end for the game; see
# src/launcher/launcher_main.cpp for why it draws its own panel rather than
# using a dialog template, and src/launcher/banner.h for why its artwork
# decodes through stb_image (already vendored for the game's own PNG
# assets) rather than WIC. Ported from shadowkey-decomp's own launcher --
# see each launcher/ file's own header comment for what's shared verbatim
# and what's genuinely different (no font, a .jar instead of a folder to
# search, a tag-prefix-filtered update check for a repo that publishes more
# than one game).
#
# The artwork the .rc embeds (src/launcher/assets/banner.png and
# dawnstar.ico) is generated from src/launcher/assets/banner_source.jpg by:
#
#   python tools/make_banner.py dawnstar/port/src/launcher/assets/banner_source.jpg
#   python tools/make_icon.py   dawnstar/port/src/launcher/assets/banner_source.jpg
configure_file(src/launcher/version.h.in
               "${CMAKE_BINARY_DIR}/generated/launcher/version.h" @ONLY)

# third_party/puff (zlib's reference inflate), vendored for the launcher's
# zip reader -- see third_party/puff/PROVENANCE.md.
add_library(puff STATIC third_party/puff/puff.c)
target_include_directories(puff PUBLIC third_party/puff)
if(MSVC)
  target_compile_options(puff PRIVATE /W3)  # vendored third-party source
endif()

add_executable(dawnstar_launcher WIN32
  src/launcher/launcher_main.cpp
  src/launcher/banner.cpp
  src/launcher/install.cpp
  src/launcher/launcher_config.cpp
  src/launcher/update_check.cpp
  src/launcher/updater.cpp
  src/launcher/zip_reader.cpp
  src/launcher/launcher.rc
  src/platform/win32/exe_dir.cpp
)
set_target_properties(dawnstar_launcher PROPERTIES OUTPUT_NAME "Dawnstar")
target_include_directories(dawnstar_launcher PRIVATE src "${CMAKE_BINARY_DIR}/generated")
# ole32 is COM (IFileOpenDialog, the file pickers); winhttp is the update
# check/download. No windowscodecs -- the banner decodes through stb_image.
target_link_libraries(dawnstar_launcher PRIVATE stb_image puff comctl32 ole32 winhttp)
target_compile_definitions(dawnstar_launcher PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
# The manifest is a resource in launcher.rc (see the comment there for why);
# CMake's own generated manifest has to be turned off so only one
# RT_MANIFEST ends up in the image.
target_link_options(dawnstar_launcher PRIVATE /MANIFEST:NO)

# Launcher smoke test -- see src/tests/m_launcher_smoke.cpp. Headless: it
# exercises the launcher's decode/config/discovery layer directly, so it
# never opens a window and needs no game data or network. Compiles the
# launcher's own sources rather than linking a library, same as
# dawnstar_launcher itself.
add_executable(launcher_smoke
  src/tests/m_launcher_smoke.cpp
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

if(NOT DAWNSTAR_DIST)

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

# M24 milestone smoke test -- see
# src/tests/m24_registered_spawns_smoke.cpp.
add_executable(registered_spawns_smoke src/tests/m24_registered_spawns_smoke.cpp)
target_link_libraries(registered_spawns_smoke PRIVATE dawnstar_dungeon)

# M25 milestone smoke test -- see src/tests/m25_visible_objects_smoke.cpp.
add_executable(visible_objects_smoke src/tests/m25_visible_objects_smoke.cpp)
target_link_libraries(visible_objects_smoke PRIVATE dawnstar_player)

# M26 milestone smoke test -- see
# src/tests/m26_visible_object_renderer_smoke.cpp.
add_executable(visible_object_renderer_smoke src/tests/m26_visible_object_renderer_smoke.cpp)
target_link_libraries(visible_object_renderer_smoke PRIVATE dawnstar_render)

# M27 milestone smoke test -- see
# src/tests/m27_object_at_position_smoke.cpp.
add_executable(object_at_position_smoke src/tests/m27_object_at_position_smoke.cpp)
target_link_libraries(object_at_position_smoke PRIVATE dawnstar_render)

# M28 milestone smoke test -- see src/tests/m28_npc_portrait_smoke.cpp.
add_executable(npc_portrait_smoke src/tests/m28_npc_portrait_smoke.cpp)
target_link_libraries(npc_portrait_smoke PRIVATE dawnstar_render)

# M29 milestone smoke test -- see src/tests/m29_minimap_smoke.cpp.
add_executable(minimap_smoke src/tests/m29_minimap_smoke.cpp)
target_link_libraries(minimap_smoke PRIVATE dawnstar_render)

# M30 milestone smoke test -- see src/tests/m30_message_popup_smoke.cpp.
add_executable(message_popup_smoke src/tests/m30_message_popup_smoke.cpp)
target_link_libraries(message_popup_smoke PRIVATE dawnstar_render)

# M31 milestone smoke test -- see src/tests/m31_hotbar_renderer_smoke.cpp.
add_executable(hotbar_renderer_smoke src/tests/m31_hotbar_renderer_smoke.cpp)
target_link_libraries(hotbar_renderer_smoke PRIVATE dawnstar_render)

# M32 milestone smoke test -- see src/tests/m32_combat_tick_smoke.cpp.
add_executable(combat_tick_smoke src/tests/m32_combat_tick_smoke.cpp)
target_link_libraries(combat_tick_smoke PRIVATE dawnstar_combat)

# M33 milestone smoke test -- see src/tests/m33_spellcast_tick_smoke.cpp.
add_executable(spellcast_tick_smoke src/tests/m33_spellcast_tick_smoke.cpp)
target_link_libraries(spellcast_tick_smoke PRIVATE dawnstar_combat)

# M34 milestone smoke test -- see src/tests/m34_interact_tick_smoke.cpp.
add_executable(interact_tick_smoke src/tests/m34_interact_tick_smoke.cpp)
target_link_libraries(interact_tick_smoke PRIVATE dawnstar_interact)

# M35 milestone smoke test -- see src/tests/m35_camp_tick_smoke.cpp.
add_executable(camp_tick_smoke src/tests/m35_camp_tick_smoke.cpp)
target_link_libraries(camp_tick_smoke PRIVATE dawnstar_camp)

# M36 milestone smoke test -- see src/tests/m36_monster_ai_tick_smoke.cpp.
add_executable(monster_ai_tick_smoke src/tests/m36_monster_ai_tick_smoke.cpp)
target_link_libraries(monster_ai_tick_smoke PRIVATE dawnstar_combat dawnstar_player)

# M37 milestone smoke test -- see src/tests/m37_screen_smoke.cpp.
add_executable(screen_smoke src/tests/m37_screen_smoke.cpp)
target_link_libraries(screen_smoke PRIVATE dawnstar_render)

# M38 milestone smoke test -- see src/tests/m38_menu_flow_smoke.cpp.
add_executable(menu_flow_smoke src/tests/m38_menu_flow_smoke.cpp)
target_link_libraries(menu_flow_smoke PRIVATE dawnstar_render)

# M39 milestone smoke test -- see src/tests/m39_options_menu_smoke.cpp.
add_executable(options_menu_smoke src/tests/m39_options_menu_smoke.cpp)
target_link_libraries(options_menu_smoke PRIVATE dawnstar_render)

# M40 milestone smoke test -- see
# src/tests/m40_character_creation_flow_smoke.cpp.
add_executable(character_creation_flow_smoke src/tests/m40_character_creation_flow_smoke.cpp)
target_link_libraries(character_creation_flow_smoke PRIVATE dawnstar_render)

# M41 milestone smoke test -- see
# src/tests/m41_inventory_skills_spells_smoke.cpp. Needs dawnstar_combat
# too (CombatResolution::UseItem), on top of dawnstar_render (OptionsMenu
# itself) -- no cycle, same reasoning as dawnstar_combat's own CMake
# comment above.
add_executable(inventory_skills_spells_smoke src/tests/m41_inventory_skills_spells_smoke.cpp)
target_link_libraries(inventory_skills_spells_smoke PRIVATE dawnstar_render dawnstar_combat)

# M42 milestone smoke test -- see src/tests/m42_game_save_smoke.cpp. Needs
# dawnstar_save (GameSave/OtherStateInfo themselves) on top of
# dawnstar_render (LoadingScreen's own pixel layout) -- no cycle, same
# reasoning as dawnstar_combat's own CMake comment above.
add_executable(game_save_smoke src/tests/m42_game_save_smoke.cpp)
target_link_libraries(game_save_smoke PRIVATE dawnstar_save dawnstar_render)

# M48 milestone smoke test -- see src/tests/m48_boot_splash_smoke.cpp.
add_executable(boot_splash_smoke src/tests/m48_boot_splash_smoke.cpp)
target_link_libraries(boot_splash_smoke PRIVATE dawnstar_render)

# M49 milestone smoke test -- see src/tests/m49_level_up_smoke.cpp.
add_executable(level_up_smoke src/tests/m49_level_up_smoke.cpp)
target_link_libraries(level_up_smoke PRIVATE dawnstar_render dawnstar_player)

# M43 milestone smoke test -- see src/tests/m43_reveal_traitor_smoke.cpp.
# Links dawnstar_player too (this test's own direct PlayerSave/
# PlayerInventory calls) on top of dawnstar_render (OptionsMenu itself,
# whose transitive set already includes dawnstar_player anyway) -- made
# explicit for the direct calls, same reasoning as m36's own extra
# dawnstar_player.
add_executable(reveal_traitor_smoke src/tests/m43_reveal_traitor_smoke.cpp)
target_link_libraries(reveal_traitor_smoke PRIVATE dawnstar_render dawnstar_player)

# M44 milestone smoke test -- see src/tests/m44_passive_tick_smoke.cpp.
# Links dawnstar_passive (PassiveTick itself) + dawnstar_dungeon
# (DungeonRuntime::TrySpawnMonsterNear, for the test's own twin-replay
# oracle) + dawnstar_render (Backbuffer/MessagePopup) -- dawnstar_player/
# dawnstar_monster come transitively.
add_executable(passive_tick_smoke src/tests/m44_passive_tick_smoke.cpp)
target_link_libraries(passive_tick_smoke PRIVATE dawnstar_passive dawnstar_dungeon dawnstar_render)

# M45 milestone smoke test -- see src/tests/m45_shop_interaction_smoke.cpp.
# Links dawnstar_interact too (InteractTick::ProcessInteract's own new
# npcInSight wiring) on top of dawnstar_npc (ShopInteraction itself) --
# dawnstar_player/dawnstar_assets come transitively.
add_executable(shop_interaction_smoke src/tests/m45_shop_interaction_smoke.cpp)
target_link_libraries(shop_interaction_smoke PRIVATE dawnstar_npc dawnstar_interact)

# M46 milestone smoke test -- see src/tests/m46_npc_menu_smoke.cpp.
add_executable(npc_menu_smoke src/tests/m46_npc_menu_smoke.cpp)
target_link_libraries(npc_menu_smoke PRIVATE dawnstar_render dawnstar_npc)

endif()  # NOT DAWNSTAR_DIST
