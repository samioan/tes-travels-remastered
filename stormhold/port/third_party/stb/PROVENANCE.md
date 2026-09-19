# stb_image

Single-header PNG (and other format) decoder, vendored for phase-3 M24 (see
`../../docs/PORT_ROADMAP.md`) so `GameCanvas`'s plain-`Image` assets
(`floorTexture`/`wallTexture`/`effectImages`/`hotbarIcons`, all real `.png`
files loaded via `Image.createImage()` -- confirmed distinct from M7's
from-scratch `RawImage`/`.cus` format) can actually be turned into pixels to
draw. Copied verbatim from the sibling `dawnstar` project's own identical
vendoring (its M10) -- same rationale `shadowkey-decomp` vendored `puff`/
`stb_vorbis` for and dawnstar vendored this same file for: a small,
well-known, permissively-licensed library for a solved format, rather than
writing a PNG/DEFLATE decoder from scratch the way M7's own from-scratch
`RawImage` format genuinely required reverse-engineering.

- **Source**: https://github.com/nothings/stb
- **File**: `stb_image.h`
- **Pinned commit**: `2c980bb59875b0d32144a71867fbdebb2f77cd20` (2026-08-02)
- **Version**: v2.30 (per the file's own header comment)
- **License**: dual MIT / public domain (Unlicense), see the file's own
  trailing license block -- either is compatible with this project.

Not modified from upstream. `STBI_ONLY_PNG` is defined when compiling (see
`CMakeLists.txt`) since that's the only format this game's assets use
(`../../docs/ASSET_FORMATS.md`'s own `.png` section) -- shrinks the compiled
decoder to just the PNG/zlib path rather than also building JPEG/BMP/GIF/etc.
support nothing here calls.
