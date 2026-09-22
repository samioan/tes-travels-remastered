#pragma once
#include <string>
#include <vector>

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

// Util.java's own replace(source, tag, String[] values) overload: calls
// the single-value version once per entry, in order -- so a template with
// several `tag` occurrences gets each replaced with a different value in
// left-to-right sequence. M45's Shop.rumorFor (the "asked again" phrasing,
// which substitutes a skill name plus two rank numbers into one template)
// is its first real caller.
inline std::string ReplaceFirstTag(const std::string& source, const std::string& tag,
                                    const std::vector<std::string>& values) {
    std::string result = source;
    for (const std::string& value : values) result = ReplaceFirstTag(result, tag, value);
    return result;
}

// ESGame.java's own copyString (lines ~58-65): the copyright/legal
// notice text, kept as the original's own 6-part array. Two real,
// different consumers: secondaryParam==200/201's own "Exiting" screens
// CONCATENATE the parts into one paragraph (`CopyStringText()` below,
// main.cpp's M44/M56 Game Over/Victory chains), while
// LoadingScreen.java's mode-2 splash draws them as 6 SEPARATE lines
// (ui/boot_splash.cpp's own Copyright phase, M48) -- both real call
// sites share this one array rather than each keeping its own copy.
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
