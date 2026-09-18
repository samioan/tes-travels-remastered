@echo off
call "%~dp0vcvars.bat"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b 1
cmake --build build
