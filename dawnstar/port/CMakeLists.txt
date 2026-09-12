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

# M9/M10: corridor rendering -- see docs/PORT_ROADMAP.md, src/render/,
# src/world/dungeon_view.h.
add_library(dawnstar_render STATIC
  src/render/corridor_render_plan.cpp
  src/render/frame_renderer.cpp
)
target_include_directories(dawnstar_render PUBLIC src)
target_link_libraries(dawnstar_render PUBLIC dawnstar_world)

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
