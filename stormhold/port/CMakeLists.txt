cmake_minimum_required(VERSION 3.20)
project(stormhold_port CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4 /utf-8)
endif()

# Placeholder Win32 shell -- proves the toolchain (CMake + Ninja + MSVC)
# builds and runs on this machine. No game logic yet: that starts once
# ../docs/ROADMAP.md phase 1/2 (source read-through, asset formats) is
# done. Software-rendered raw Win32 (no SDL/D3D/GL dependency), matching
# the sibling shadowkey-decomp port's platform layer.
add_executable(stormhold_port WIN32
  src/main.cpp
)
if(MSVC)
  target_compile_options(stormhold_port PRIVATE /W4)
endif()
