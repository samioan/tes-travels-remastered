@echo off
rem Find this machine's Visual Studio and enter an x64 build environment.
rem Called by build.bat and build_dist.bat. Ported verbatim from dawnstar's
rem own port/vcvars.bat (itself from shadowkey-decomp) -- nothing in this
rem file is specific to any one game.
rem
rem build.bat used to hardcode one developer's install path, which is fine
rem until something else has to build the project -- a CI runner has Visual
rem Studio somewhere else entirely, and a second install path in a workflow
rem file is a second thing to keep in step. This looks the path up instead,
rem so the scripts a person runs and the ones CI runs are the same scripts.
rem
rem Succeeds silently if the caller is already inside a developer prompt.

if defined VCINSTALLDIR (
    echo [vcvars] already in a developer environment: %VCINSTALLDIR%
    exit /b 0
)

rem vswhere ships with every Visual Studio 2017 and later installer, always
rem at this fixed location -- which is the whole point of it, since VS
rem itself moves around.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

set "VSPATH="
if exist "%VSWHERE%" (
    rem -products * so Build Tools installs count, not just the IDE; the
    rem -requires filter skips an install that has no C++ compiler in it.
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -prerelease -products * ^
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
        -property installationPath 2^>nul`) do set "VSPATH=%%i"
)

if not defined VSPATH (
    echo [vcvars] vswhere found no Visual Studio with the C++ tools installed.
    echo [vcvars] Install "Desktop development with C++" ^(or the Build Tools^) and retry.
    exit /b 1
)

if not exist "%VSPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
    echo [vcvars] found Visual Studio at "%VSPATH%" but no vcvarsall.bat under it.
    exit /b 1
)

echo [vcvars] using %VSPATH%
call "%VSPATH%\VC\Auxiliary\Build\vcvarsall.bat" x64
exit /b %errorlevel%
