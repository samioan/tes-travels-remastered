#pragma once
#include <string>

#include "graphics/backbuffer.h"

namespace stormhold {

// A port-only blocking screen standing in for `../../../src/ESGame.java`'s
// `npcHelloUI` (`new UIScreen(this, 4, 8)`, `talkToNpc()`'s own real
// target) -- title (Shop.NAMES[npcId]) + word-wrapped dialogue body text.
//
// **M64 update:** dismissing this (Enter) now transitions into
// `ui/npc_choices_menu.h`'s own interactive follow-up menu for shops 0-3
// (`main.cpp`'s own dismiss handling, keyed on `shopId` below) rather than
// closing straight back to gameplay -- see that file's own class comment
// for the real, confirmed softlock this deliberately does NOT reproduce
// (the original's own `npcHelloUI` -> `npcChoicesUI[npcId]` transition is
// broken for every NPC, not just these 4). Shops 4/5/6 still just close
// back to gameplay on dismiss, same as before M64 -- their own choices
// menus (Beneca/Helga's bespoke ones, and Varus's own confirmed-crashing
// array-bounds issue) remain deliberately unbuilt, per that same class
// comment's own "what's next" scoping.
struct NpcDialogueState {
    bool active = false;
    std::string title;
    std::string body;
    // M64: which NPC this greeting was for -- `main.cpp`'s own Enter-key
    // dismiss handling reads this to decide whether to transition into
    // `ui/npc_choices_menu.h`'s own follow-up menu (shops 0-3) or just
    // close back to gameplay (shops 4/5/6, not built by any milestone
    // yet -- see that file's own class comment).
    int shopId = -1;
};

class NpcDialogue {
public:
    // Opens the screen with `title`/`body` -- `body` may contain an
    // embedded '\n' (every multi-line `ShopInteraction::*Dialogue` result
    // already uses '\n' as its own real line break, e.g. Helga's
    // greeting/Varus's lore reveal), which `Render` below treats as a
    // real paragraph break the same way `ui/menu_flow.cpp`'s own
    // `WordWrap` does for its message screens.
    static void Show(NpcDialogueState& state, const std::string& title, const std::string& body, int shopId);

    // Closes the screen, returning control to live gameplay.
    static void Dismiss(NpcDialogueState& state);

    // Paints the title bar + word-wrapped body -- REPLACES the normal
    // corridor/HUD/minimap frame entirely while `state.active`, matching
    // `npcHelloUI` fully taking over `Display.setCurrent()` in the
    // original (not an overlay on top of the game view).
    static void Render(Backbuffer& bb, const NpcDialogueState& state);
};

}  // namespace stormhold
