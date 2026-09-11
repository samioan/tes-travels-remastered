#pragma once
#include <fstream>
#include <string>

#include "assets/binary_reader.h"

namespace dawnstar {

// Reads dawnstar's datfiles.lmp -- a named-blob archive with no index,
// linear-scanned from the start of the file on every lookup (see
// ../../docs/ASSET_FORMATS.md and ../../../src/ESGame.java's getResource(),
// the algorithm this mirrors byte-for-byte). Faithfully reproduces that
// same no-caching, rescan-every-time behavior rather than building an
// index -- these lookups only happen a handful of times at load, so
// there's no reason to diverge from the original just to be "more
// efficient".
class DatArchive {
public:
    explicit DatArchive(const std::string& path);

    // Scans for `name`, seeks to its data, and returns a reader positioned
    // there. The returned BinaryReader is only valid until the next
    // OpenResource() call (it reads through this archive's own stream).
    // Throws std::runtime_error if `name` isn't found -- the original's
    // scan loop has no "not found" path at all (it just spins forever
    // re-reading EOF as -1), since a well-formed archive always contains
    // every name the game asks for.
    BinaryReader OpenResource(const std::string& name);

private:
    std::ifstream file_;
};

}  // namespace dawnstar
