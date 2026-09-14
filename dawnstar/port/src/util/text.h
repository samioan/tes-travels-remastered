#pragma once
#include <string>

namespace dawnstar {

// Util.java's own replace(source, tag, value), promoted here from
// options_menu.cpp's former file-local copy (M43, its first real ported
// call site) once M44's main.cpp Game Over chain needed the same
// first-occurrence substitution -- the same reuse-over-duplication call
// M16/M17 already made for the inventory/starting-spell helpers.
// Replaces only the FIRST occurrence of `tag`: Util.java's own doc
// comment notes that callers substituting several distinct placeholders
// must call it once per placeholder in order, relying on that.
inline std::string ReplaceFirstTag(const std::string& source, const std::string& tag, const std::string& value) {
    size_t at = source.find(tag);
    if (at == std::string::npos) return source;
    return source.substr(0, at) + value + source.substr(at + tag.size());
}

// ESGame.java's own copyString (lines ~58-65): the copyright/legal
// notice text, kept as the original's own 6-part array (several real
// consumers concatenate the parts -- secondaryParam==200/201's own
// "Exiting" screens and LoadingScreen.java's mode-2 splash -- rather
// than display them line-by-line) plus that exact concatenation here.
// main.cpp's M44 Game Over chain is its first consumer in this port; the
// still-unported boot splash (LoadingScreen modes 1/2) will be another.
inline const char* const kCopyStringParts[6] = {
    "(c) 2003 Vir2L Studios, ",
    "a ZeniMax Media company. ",
    "The Elder Scrolls and Vir2L ",
    "are registered trademarks ",
    "of ZeniMax Media Inc. ",
    "All rights reserved.",
};

// ESGame.java's own `String result = ""; for (i...) result +=
// copyString[i];` idiom (secondaryParam==200/201's own "Exiting"
// screens), transcribed once.
inline std::string CopyStringText() {
    std::string out;
    for (const char* part : kCopyStringParts) out += part;
    return out;
}

}  // namespace dawnstar
