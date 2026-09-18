@echo off
rem The shipping build, and then the folder that gets zipped. Ported from
rem shadowkey-decomp's own port/build_dist.bat.
rem
rem Differs from build.bat in four ways, all of which matter to someone who
rem downloaded this rather than built it:
rem   Release             - optimised, and /MT via CMAKE_MSVC_RUNTIME_LIBRARY,
rem                         so no Visual C++ redistributable is needed
rem   DAWNSTAR_DIST=ON    - builds the two shipped executables, not every
rem                         milestone's own smoke test too
rem   DAWNSTAR_VERSION    - what the launcher prints in its corner
rem
rem Usage:  port\build_dist.bat [version]
rem Output: port\dist\  (ready to zip)  and  port\build-dist\  (objects)

setlocal
call "%~dp0vcvars.bat"
if errorlevel 1 exit /b 1
cd /d "%~dp0"

set DAWNSTAR_VER=%1
if "%DAWNSTAR_VER%"=="" set DAWNSTAR_VER=0.0.0-local

cmake -G Ninja -S . -B build-dist -DCMAKE_BUILD_TYPE=Release -DDAWNSTAR_DIST=ON ^
      -DDAWNSTAR_VERSION=%DAWNSTAR_VER%
if errorlevel 1 exit /b 1
cmake --build build-dist
if errorlevel 1 exit /b 1

rem Assemble the layout the launcher expects: itself at the top, the game
rem in bin\. `data\` and `user\` are created by the launcher on first run,
rem not shipped -- they are where the player's own files go.
if exist dist rmdir /s /q dist
mkdir dist\bin
copy /y build-dist\Dawnstar.exe dist\ >nul
if errorlevel 1 exit /b 1
copy /y build-dist\dawnstar_port.exe dist\bin\ >nul
if errorlevel 1 exit /b 1
copy /y ..\..\NOTICE.md dist\ >nul
copy /y dist_readme.txt dist\README.txt >nul

echo.
echo Built %DAWNSTAR_VER% into port\dist\ -- zip that folder.
dir /b dist
endlocal
