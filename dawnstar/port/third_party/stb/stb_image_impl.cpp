// Compiles stb_image's implementation into one translation unit -- see
// PROVENANCE.md. STBI_ONLY_PNG: this game's assets are all plain PNG
// (see ../../docs/ASSET_FORMATS.md), so the JPEG/BMP/GIF/PSD/etc. decoders
// stb_image also bundles are compiled out entirely. STBI_NO_STDIO: every
// caller decodes from an already-loaded in-memory byte blob (ImgArchive),
// never a file path, so the FILE*-based API surface is compiled out too.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"
