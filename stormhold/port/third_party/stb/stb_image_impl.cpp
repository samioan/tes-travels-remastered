// Compiles stb_image's implementation into one translation unit -- see
// PROVENANCE.md. STBI_ONLY_PNG: this game's assets are all plain PNG (see
// ../../docs/ASSET_FORMATS.md's own `.png` section), so the JPEG/BMP/GIF/
// PSD/etc. decoders stb_image also bundles are compiled out entirely.
// STBI_NO_STDIO: the only caller decodes from an already-loaded in-memory
// byte blob (AssetRoot has no archive to seek within, but DecodedImage::Load
// still reads the whole file into memory first, same shape as dawnstar's own
// ImgArchive-backed loader), never a file path, so the FILE*-based API
// surface is compiled out too.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"
