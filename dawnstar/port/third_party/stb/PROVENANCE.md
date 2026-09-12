# stb_image

Single-header PNG (and other format) decoder, vendored for M10 (see
`../../docs/PORT_ROADMAP.md`) so `ImgArchive`'s raw PNG byte blobs (M7)
can actually be turned into pixels to draw. Same rationale
`shadowkey-decomp` vendored `puff`/`stb_vorbis` for: a small, well-known,
permissively-licensed library for a solved problem, rather than writing a
PNG/DEFLATE decoder from scratch.

- **Source**: https://github.com/nothings/stb
- **File**: `stb_image.h`
- **Pinned commit**: `2c980bb59875b0d32144a71867fbdebb2f77cd20` (2026-08-02)
- **Version**: v2.30 (per the file's own header comment)
- **License**: dual MIT / public domain (Unlicense), see the file's own
  trailing license block -- either is compatible with this project.

Not modified from upstream. `STBI_ONLY_PNG` is defined when compiling
(see `CMakeLists.txt`) since that's the only format this game's assets
use (`../../docs/ASSET_FORMATS.md`) -- shrinks the compiled decoder to
just the PNG/zlib path rather than also building JPEG/BMP/GIF/etc.
support nothing here calls.
