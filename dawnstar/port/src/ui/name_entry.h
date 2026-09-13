#pragma once
#include <string>

#include "graphics/backbuffer.h"

namespace dawnstar {

// Renamed-source counterpart of `../src/ESGame.java`'s own
// `charNameTextForm`: a raw MIDP `Form` holding one `StringItem`
// ("Enter a name for your character") and one `TextField(null, null,
// 10, 0)` (max 10 characters, `TextField.ANY` -- any character) --
// used for exactly ONE thing in the whole game (naming a new
// character; confirmed the only real `new TextField(` call site in
// `../src/*.java`), so this is a small, purpose-built widget for that
// one real use, not a generic MIDP `Form`/`TextField` port (matching
// M38/M39's own "purpose-built flow over Screen, not a generic engine"
// precedent).
//
// UNRECOVERABLE, invented rendering: unlike `Screen` (M37), a real
// MIDP `Form` has NO custom `paint(Graphics)` of its own at all --
// its entire visual appearance (text field box, cursor, label layout)
// was rendered by the phone's own MIDP implementation, outside the
// game's control, and is not present anywhere in the decompiled
// source to transcribe. This class's own `Render()` is therefore an
// invented presentation (reusing M37 `Screen`'s own color palette for
// visual consistency with the rest of this port), the same
// "unrecoverable system UI, build something reasonable" status
// `graphics/bitmap_font.h`'s own invented glyphs already have for
// MIDP's system fonts.
//
// SIMPLIFIED character set: the real `TextField`'s own `ANY` constraint
// allows any character, but this only accepts what `BitmapFont` can
// actually render (space/'/-/! and A-Z/0-9 -- see its own class
// comment) -- a real phone's own keypad-driven text entry was never
// going to produce anything BitmapFont can't already draw anyway (no
// real extracted string in this whole game uses a character outside
// that set), so this is a practical, harmless restriction, not a
// content-altering "fix".
class NameEntry {
public:
    // TextField(null, null, 10, 0)'s own max length.
    static constexpr size_t kMaxLength = 10;

    void Clear() { text_.clear(); }

    // Appends `c` if under kMaxLength AND `c` is one of the characters
    // BitmapFont can render (see this class's own doc comment) --
    // silently ignored otherwise, matching a real TextField simply not
    // accepting more input once its own max length is reached.
    void OnChar(char c);
    void OnBackspace();

    const std::string& Text() const { return text_; }

    void Render(Backbuffer& bb) const;

private:
    std::string text_;
};

}  // namespace dawnstar
