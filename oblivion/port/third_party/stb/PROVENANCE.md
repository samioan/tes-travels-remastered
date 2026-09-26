# stb_image

Single-header PNG decoder (nothings/stb, v2.30, MIT / public domain), vendored
for the Oblivion port. Copied verbatim from the sibling `stormhold` port's own
vendoring (see `../../../../stormhold/port/third_party/stb/PROVENANCE.md` for
the pinned commit). `stb_image_impl.cpp` compiles it PNG-only, memory-only.
The game's images are all palettised PNGs (`ts_lvl*.png`, `oh_*`-sprite sheets),
decoded to RGBA with the tRNS/alpha channel kept.
