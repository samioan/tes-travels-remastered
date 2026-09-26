@echo off
rem Builds a Release exe with a static CRT and stages it in dist\ with the README.
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cmake -G Ninja -S . -B build-release -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build-release
if errorlevel 1 exit /b 1
cmake --install build-release --prefix dist
