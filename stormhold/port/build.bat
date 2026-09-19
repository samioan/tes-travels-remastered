@echo off
rem Was a hardcoded developer VS install path -- replaced with vcvars.bat's
rem vswhere lookup (see that file's own header comment) so this script
rem also works on a CI runner, whose Visual Studio lives somewhere else
rem entirely. Same fix dawnstar's own port/build.bat already needed.
call "%~dp0vcvars.bat"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b 1
cmake --build build
