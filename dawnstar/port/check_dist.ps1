<#
.SYNOPSIS
    Checks that port/dist is a shippable, self-contained install.

.DESCRIPTION
    Ported from shadowkey-decomp's own port/check_dist.ps1. Run after
    port/build_dist.bat, and by the release workflow before anything is
    uploaded. Everything here is something that would otherwise only be
    discovered by a person downloading the zip:

      * both executables present, in the layout the launcher looks for
      * neither one linked against the Visual C++ runtime DLLs -- the single
        failure that turns "unzip and play" into a missing-DLL dialog on
        every machine without a matching redistributable installed
      * no game data (a .jar, or anything extract_jar.py would produce)
        accidentally swept into the package, which would be a licensing
        problem, not just a large download
      * the launcher really carries its artwork and its icon

    Exits non-zero, naming what failed, so CI stops on it.
#>
[CmdletBinding()]
param(
    [string] $DistDir = "port/dist"
)

$ErrorActionPreference = 'Stop'
$problems = @()

function Fail([string] $message) { $script:problems += $message }

if (-not (Test-Path $DistDir)) {
    Write-Error "$DistDir does not exist -- run port/build_dist.bat first."
}

# --- layout -----------------------------------------------------------
$launcher = Join-Path $DistDir 'Dawnstar.exe'
$game = Join-Path $DistDir 'bin/dawnstar_port.exe'
foreach ($required in @($launcher, $game, (Join-Path $DistDir 'README.txt'),
                        (Join-Path $DistDir 'NOTICE.md'))) {
    if (-not (Test-Path $required)) { Fail "missing: $required" }
}

# --- nothing copyrighted came along -----------------------------------
# The game's own jar/extracted assets are the user's to supply. `data/`
# and `user/` are created at runtime and must not be in the package.
foreach ($forbidden in @('data', 'user', 'launcher.cfg')) {
    $path = Join-Path $DistDir $forbidden
    if (Test-Path $path) { Fail "must not be shipped: $path" }
}
$strays = Get-ChildItem -Recurse -File $DistDir |
          Where-Object { $_.Extension -in @('.jar', '.lmp', '.dat', '.cus', '.sav') }
foreach ($stray in $strays) { Fail "game data in the package: $($stray.FullName)" }

# --- no Visual C++ runtime dependency ---------------------------------
$dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
if (-not $dumpbin) {
    Write-Host "note: dumpbin not on PATH (run this from a developer prompt, or after" `
               "port/vcvars.bat) -- skipping the runtime-dependency check"
} else {
    foreach ($exe in @($launcher, $game)) {
        if (-not (Test-Path $exe)) { continue }
        $deps = & dumpbin /nologo /dependents $exe | Out-String
        foreach ($bad in @('VCRUNTIME', 'MSVCP', 'api-ms-win-crt')) {
            if ($deps -match $bad) {
                Fail ("$(Split-Path -Leaf $exe) links $bad -- it needs the Visual C++ " +
                      "redistributable, so it is not portable. Check that " +
                      "CMAKE_MSVC_RUNTIME_LIBRARY is taking effect.")
            }
        }
    }
}

# --- the launcher carries its own artwork -----------------------------
# A resource that failed to embed still links and still runs; it just draws
# a flat background, which is easy to miss and looks like a bug.
if (Test-Path $launcher) {
    $size = (Get-Item $launcher).Length
    if ($size -lt 400KB) {
        Fail ("Dawnstar.exe is only $([math]::Round($size/1KB)) KB -- too small to " +
              "contain the embedded artwork, so the .rc probably did not build in")
    }
}

if ($problems.Count -gt 0) {
    Write-Host ""
    Write-Host "dist check FAILED:" -ForegroundColor Red
    foreach ($problem in $problems) { Write-Host "  - $problem" -ForegroundColor Red }
    exit 1
}

$total = (Get-ChildItem -Recurse -File $DistDir | Measure-Object -Property Length -Sum).Sum
Write-Host ("dist check OK -- {0} files, {1:N1} MB" -f
            (Get-ChildItem -Recurse -File $DistDir).Count, ($total / 1MB))
exit 0
