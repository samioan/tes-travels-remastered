#pragma once

namespace dawnstar {

// ESGame.getGameAdvancementLevel(giftPointsFound): the 0-5 "how far along"
// bucket. Promoted to its own tiny header once a second consumer needed it
// (save/game_save.h's own resumeGame() stand-in had the only copy through
// M42; npc/shop_interaction.h's Jakar's-greeting rumor-reveal-step
// advancement, M45, is the second) -- same reuse-over-duplication
// precedent util/text.h's ReplaceFirstTag already set. GameSave::
// GetGameAdvancementLevel keeps its own public static wrapper (still
// called by main.cpp's ResumeGame path and m42_game_save_smoke.cpp
// directly) forwarding here rather than being removed, so neither needs
// to change.
inline int GetGameAdvancementLevel(int giftPointsFound) {
    if (giftPointsFound < 17) return 0;
    if (giftPointsFound < 29) return 1;
    if (giftPointsFound < 38) return 2;
    if (giftPointsFound < 49) return 3;
    return giftPointsFound < 62 ? 4 : 5;
}

}  // namespace dawnstar
