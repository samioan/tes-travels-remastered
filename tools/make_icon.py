#!/usr/bin/env python3
"""Build the Dawnstar launcher's .ico from a square crop of the banner art.

Companion to tools/make_banner.py and shadowkey-decomp's own
tools/make_icon.py (same approach, same reasoning: the decode goes through
PowerShell's System.Drawing because that is the one image decoder present
on a Windows box without installing anything).

An .ico is a tiny container format -- a 6-byte header, one 16-byte directory
entry per size, then the images -- and since Vista every size may be a PNG
rather than a BMP, which removes the AND-mask and bottom-up-BMP fiddliness
entirely. So this writes PNGs straight from System.Drawing and wraps them.

Usage:
    python tools/make_icon.py <image> [--box X,Y,W,H]
        [-o dawnstar/port/src/launcher/assets/dawnstar.ico]

--box selects the square region to crop before scaling, in source pixels;
the default is the monster's head in the committed banner art.
"""

import argparse
import os
import struct
import subprocess
import sys
import tempfile

# The sizes Explorer, the taskbar, the title bar and the Alt-Tab switcher
# actually ask for. 256 is what a "large icons" folder view uses.
SIZES = [16, 24, 32, 48, 64, 128, 256]

_CROP_PS = r"""
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$src = New-Object System.Drawing.Bitmap($args[0])
$x = [int]$args[1]; $y = [int]$args[2]; $w = [int]$args[3]; $h = [int]$args[4]
$outDir = $args[5]
$sizes = $args[6].Split(',')
$crop = New-Object System.Drawing.Rectangle($x, $y, $w, $h)
foreach ($s in $sizes) {
    $size = [int]$s
    $target = New-Object System.Drawing.Bitmap($size, $size,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($target)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $dest = New-Object System.Drawing.Rectangle(0, 0, $size, $size)
    $g.DrawImage($src, $dest, $crop, [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose()
    $target.Save((Join-Path $outDir "$size.png"),
                 [System.Drawing.Imaging.ImageFormat]::Png)
    $target.Dispose()
}
$src.Dispose()
Write-Output "ok"
"""


def render_pngs(image_path, box, out_dir):
    script_path = os.path.join(out_dir, "crop.ps1")
    with open(script_path, "w", encoding="utf-8") as handle:
        handle.write(_CROP_PS)
    result = subprocess.run(
        ["powershell", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass",
         "-File", script_path, os.path.abspath(image_path),
         str(box[0]), str(box[1]), str(box[2]), str(box[3]),
         out_dir, ",".join(str(s) for s in SIZES)],
        capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit("crop failed:\n" + (result.stderr or result.stdout))
    return [(size, os.path.join(out_dir, "%d.png" % size)) for size in SIZES]


def write_ico(entries, output_path):
    images = []
    for size, path in entries:
        with open(path, "rb") as handle:
            images.append((size, handle.read()))

    # ICONDIR: reserved, type 1 (icon), count.
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = len(header) + 16 * len(images)
    directory = b""
    for size, data in images:
        # 256 is stored as 0 -- the field is one byte, so it cannot hold 256.
        dimension = 0 if size >= 256 else size
        directory += struct.pack("<BBBBHHII", dimension, dimension, 0, 0, 1, 32,
                                 len(data), offset)
        offset += len(data)

    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    with open(output_path, "wb") as handle:
        handle.write(header)
        handle.write(directory)
        for _, data in images:
            handle.write(data)
    return offset


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("image")
    # The monster's head in the committed banner art (2752x1536): the one
    # element in this key art that still reads as a distinct shape at
    # 16x16, the same reasoning shadowkey-decomp's own default box (a crop
    # of its key art's dragon-key head) used.
    parser.add_argument("--box", default="1082,758,450,450",
                        help="square crop as X,Y,W,H in source pixels")
    parser.add_argument("-o", "--output", default=os.path.join(
        "dawnstar", "port", "src", "launcher", "assets", "dawnstar.ico"))
    args = parser.parse_args()

    box = [int(n) for n in args.box.split(",")]
    if len(box) != 4:
        sys.exit("--box needs exactly X,Y,W,H")

    with tempfile.TemporaryDirectory() as scratch:
        entries = render_pngs(args.image, box, scratch)
        total = write_ico(entries, args.output)
    print("%s: %d sizes (%s), %d bytes"
          % (args.output, len(SIZES), " ".join(str(s) for s in SIZES), total))


if __name__ == "__main__":
    main()
