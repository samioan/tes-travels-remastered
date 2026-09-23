#pragma once
#include <string>

#include "graphics/backbuffer.h"

namespace stormhold {

// A port-only blocking screen standing in for `../../../src/ESGame.java`'s
// `npcHelloUI` (`new UIScreen(this, 4, 8)`, `talkToNpc()`'s own real
// target) -- title (Shop.NAMES[npcId]) + word-wrapped dialogue body text,
// dismissed back into live gameplay on Ok. **Deliberately NOT** the real
// `npcChoicesUI[npcId]` follow-up screen (the interactive Train/Give/
// Befriend/Threaten/Kill -- or Beneca/Helga's own item-donation/point-
// spending -- menu, `ESGame.java`'s own screenGroups 9-14/20/22/27/350):
// that's a substantially bigger lift (its own item-selection sub-screens,
// result popups, and per-shop action dispatch) than this milestone's own
// scope, matching the "one coherent slice at a time" discipline
// `player/shop_interaction.h`'s own M56-M59 quartet already used for the
// dialogue TEXT side of this same gap. See docs/PORT_ROADMAP.md's own
// "what's next" note for the deferred follow-up screen.
struct NpcDialogueState {
    bool active = false;
    std::string title;
    std::string body;
};

class NpcDialogue {
public:
    // Opens the screen with `title`/`body` -- `body` may contain an
    // embedded '\n' (every multi-line `ShopInteraction::*Dialogue` result
    // already uses '\n' as its own real line break, e.g. Helga's
    // greeting/Varus's lore reveal), which `Render` below treats as a
    // real paragraph break the same way `ui/menu_flow.cpp`'s own
    // `WordWrap` does for its message screens.
    static void Show(NpcDialogueState& state, const std::string& title, const std::string& body);

    // Closes the screen, returning control to live gameplay.
    static void Dismiss(NpcDialogueState& state);

    // Paints the title bar + word-wrapped body -- REPLACES the normal
    // corridor/HUD/minimap frame entirely while `state.active`, matching
    // `npcHelloUI` fully taking over `Display.setCurrent()` in the
    // original (not an overlay on top of the game view).
    static void Render(Backbuffer& bb, const NpcDialogueState& state);
};

}  // namespace stormhold
