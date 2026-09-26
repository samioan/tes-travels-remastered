cmake_minimum_required(VERSION 3.20)
project(oblivion_port CXX C)
# Static CRT: the packaged exe runs without the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# `OBLIVION_VERSION` is what the launcher prints in its corner. A local
# build says 0.0.0-local; the release workflow passes the git tag with its
# "oblivion-v" prefix stripped.
set(OBLIVION_VERSION "0.1.0-dev" CACHE STRING "Version string shown by the launcher")
# Where the launcher looks for updates. The tag prefix keeps Oblivion's own
# releases apart from Dawnstar's and Stormhold's in the same repository.
set(OBLIVION_UPDATE_OWNER "samioan" CACHE STRING "GitHub owner to check for updates")
set(OBLIVION_UPDATE_REPO "tes-travels-remastered" CACHE STRING "GitHub repo to check for updates")
set(OBLIVION_UPDATE_TAG_PREFIX "oblivion-v" CACHE STRING "Release tag prefix identifying an Oblivion build")
option(OBLIVION_DIST "Build only the shipped executables (launcher + game)" OFF)

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
  src/world/dungeon.cpp
  src/world/player_record.cpp
  src/game/input_mapper.cpp
  src/world/level_view.cpp
)
target_link_libraries(oblivion_world PUBLIC oblivion_assets)

# Software-rendered raw Win32 (no SDL/D3D/GL), like the sibling ports.
add_executable(oblivion_port WIN32
  src/main.cpp
  src/platform/win32/window.cpp
  src/platform/win32/display.cpp
  src/platform/win32/input_device.cpp
)
target_link_libraries(oblivion_port PRIVATE oblivion_world)


# The "unzip and play" front end: picks your own TEST-Oblivion.jar (and the
# optional Nokia font), plays, and updates itself from GitHub releases. Ported
# from stormhold's launcher (itself dawnstar's, itself shadowkey-decomp's).
# The artwork the .rc embeds (src/launcher/assets/banner.png and oblivion.ico)
# is generated from src/launcher/assets/banner_source.png by:
#
#   python tools/make_banner.py oblivion/port/src/launcher/assets/banner_source.png -o oblivion/port/src/launcher/assets/banner.png
#   python tools/make_icon.py   oblivion/port/src/launcher/assets/banner_source.png --box 136,420,205,205 -o oblivion/port/src/launcher/assets/oblivion.ico
configure_file(src/launcher/version.h.in
               "${CMAKE_BINARY_DIR}/generated/launcher/version.h" @ONLY)

# third_party/puff (zlib's reference inflate), for the launcher's zip reader.
add_library(puff STATIC third_party/puff/puff.c)
target_include_directories(puff PUBLIC third_party/puff)
if(MSVC)
  target_compile_options(puff PRIVATE /W3)
endif()

add_executable(oblivion_launcher WIN32
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
set_target_properties(oblivion_launcher PROPERTIES OUTPUT_NAME "Oblivion")
target_include_directories(oblivion_launcher PRIVATE src "${CMAKE_BINARY_DIR}/generated")
# ole32 is COM (IFileOpenDialog); winhttp is the update check/download.
target_link_libraries(oblivion_launcher PRIVATE stb_image puff comctl32 ole32 winhttp)
target_compile_definitions(oblivion_launcher PRIVATE UNICODE _UNICODE)
# The manifest is a resource in launcher.rc; CMake's generated one must be off.
target_link_options(oblivion_launcher PRIVATE /MANIFEST:NO)

# Headless launcher check: banner decode, config, jar install against a
# synthetic jar, the zip reader, version compare, the release parser.
add_executable(launcher_smoke
  src/tests/launcher_smoke.cpp
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
enable_testing()
add_test(NAME launcher_smoke COMMAND launcher_smoke)

# Milestone smoke tests (plain executables that exit non-zero on failure).
# Skipped in the shipping build (OBLIVION_DIST=ON), which only needs the two exes.
enable_testing()
if(NOT OBLIVION_DIST)
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

add_executable(dungeon_smoke src/tests/dungeon_smoke.cpp)
target_link_libraries(dungeon_smoke PRIVATE oblivion_world)
add_test(NAME dungeon_smoke COMMAND dungeon_smoke ${CMAKE_SOURCE_DIR}/../extracted)

add_executable(save_smoke src/tests/save_smoke.cpp)
target_link_libraries(save_smoke PRIVATE oblivion_world)
add_test(NAME save_smoke COMMAND save_smoke ${CMAKE_SOURCE_DIR}/../extracted)

add_executable(input_smoke src/tests/input_smoke.cpp)
target_link_libraries(input_smoke PRIVATE oblivion_world)
add_test(NAME input_smoke COMMAND input_smoke ${CMAKE_SOURCE_DIR}/../extracted)
add_executable(display_smoke src/tests/display_smoke.cpp src/platform/win32/display.cpp src/platform/win32/window.cpp)
target_link_libraries(display_smoke PRIVATE oblivion_world)
add_test(NAME display_smoke COMMAND display_smoke ${CMAKE_SOURCE_DIR}/../extracted)
endif()  # NOT OBLIVION_DIST
