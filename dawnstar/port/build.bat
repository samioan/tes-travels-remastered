@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b 1
cmake --build build
