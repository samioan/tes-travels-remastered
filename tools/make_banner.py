#!/usr/bin/env python3
"""Prepare the Dawnstar launcher's banner artwork for embedding.

Companion to shadowkey-decomp's tools/make_banner.py, adapted for a real
difference: Dawnstar's launcher decodes through stb_image (already
vendored for the game's own PNG assets, see port/third_party/stb/), which
this repo builds with STBI_ONLY_PNG -- so the embedded banner stays a PNG
rather than being re-encoded to JPEG the way shadowkey-decomp's WIC-based
decoder wants. This script only downscales (never upscales) and re-saves
as PNG, dropping the megabytes of resolution the launcher's window never
actually draws.

Usage:
    python tools/make_banner.py <image> [--width 1400]
        [-o dawnstar/port/src/launcher/assets/banner.png]

The resize goes through PowerShell's System.Drawing, the one image codec
guaranteed to be on a Windows dev box without installing anything.
"""

import argparse
import os
import subprocess
import sys
import tempfile

# Comfortably more than the launcher's own header is ever asked to draw
# (kClientWidth=760 logical px, so 1520 at 200% DPI).
DEFAULT_WIDTH = 1400

_RESIZE_PS = r"""
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$src = New-Object System.Drawing.Bitmap($args[0])
$targetWidth = [int]$args[1]
$outPath = $args[2]

# Never upscale: if the source is already smaller, keep it as it is.
if ($targetWidth -ge $src.Width) { $targetWidth = $src.Width }
$targetHeight = [int][math]::Round($src.Height * $targetWidth / $src.Width)

$dst = New-Object System.Drawing.Bitmap($targetWidth, $targetHeight,
    [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($dst)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$g.DrawImage($src, 0, 0, $targetWidth, $targetHeight)
$g.Dispose()

$dst.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)

Write-Output "$($src.Width) $($src.Height) $targetWidth $targetHeight"
$dst.Dispose()
$src.Dispose()
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("image", help="source artwork (PNG/JPEG/BMP)")
    parser.add_argument("--width", type=int, default=DEFAULT_WIDTH)
    parser.add_argument("-o", "--output", default=os.path.join(
        "dawnstar", "port", "src", "launcher", "assets", "banner.png"))
    args = parser.parse_args()

    output = os.path.abspath(args.output)
    os.makedirs(os.path.dirname(output), exist_ok=True)

    with tempfile.TemporaryDirectory() as scratch:
        script_path = os.path.join(scratch, "resize.ps1")
        with open(script_path, "w", encoding="utf-8") as handle:
            handle.write(_RESIZE_PS)
        result = subprocess.run(
            ["powershell", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass",
             "-File", script_path, os.path.abspath(args.image),
             str(args.width), output],
            capture_output=True, text=True)
        if result.returncode != 0:
            sys.exit("resize failed:\n" + (result.stderr or result.stdout))
        sourceWidth, sourceHeight, width, height = (int(n) for n in result.stdout.split())

    size = os.path.getsize(output)
    print("%s: %dx%d -> %dx%d, %.0f KB"
          % (args.output, sourceWidth, sourceHeight, width, height, size / 1024.0))
    # The launcher tolerates a bad banner by drawing a flat background, so a
    # silently truncated file would be a quiet cosmetic bug rather than a
    # crash. Check the PNG signature here instead.
    with open(output, "rb") as handle:
        signature = handle.read(8)
    if signature != b"\x89PNG\r\n\x1a\n":
        sys.exit("output is not a complete PNG (bad signature)")


if __name__ == "__main__":
    main()
