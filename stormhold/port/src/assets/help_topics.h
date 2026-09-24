#pragma once
#include <array>
#include <string>
#include <vector>

#include "assets/shop_dialogue.h"

namespace stormhold {

// ESGame.loadHelpTopicTitles()/loadHelpTopicBodies(): 12 help topics, each a
// single title row plus a 1-5-row body span, all into `ShopDialogue` group 7
// (the 41-entry generic/rumor pool -- see shop_dialogue.h's own class
// comment). Transcribed verbatim from `decompiled/ESGame.java`'s `private
// void a()` (M69) -- `src/ESGame.java`'s own copy of the same method
// previously stopped partway through topic 5, flagged as a transcription
// gap; the decompiled bytecode itself was never actually incomplete, just
// not fully hand-copied yet.
//
// Shared by both real entry points into this same content (M69 built the
// pause-menu one; M72 adds the main-menu one, `newHelpMenuUI`/
// `newHelpTopicUI`'s OTHER real call site, `ESGame.java`'s own
// screenGroup==2 case 2, `mainMenuItems[2]`) -- both read the identical 12
// topics out of the identical `ShopDialogue`, so this lives here once
// rather than as two copies (ui/pause_menu.cpp's own earlier local copy was
// folded into this file, not duplicated).
class HelpTopics {
public:
    static constexpr int kCount = 12;

    static std::string Title(const ShopDialogue& dialogue, int topicIndex) {
        return dialogue.groups[7][static_cast<size_t>(kTitleRow[static_cast<size_t>(topicIndex)])];
    }

    static std::string Body(const ShopDialogue& dialogue, int topicIndex) {
        std::string body;
        for (int row : kBodyRows[static_cast<size_t>(topicIndex)]) {
            body += dialogue.groups[7][static_cast<size_t>(row)];
        }
        return body;
    }

    static std::vector<std::string> Titles(const ShopDialogue& dialogue) {
        std::vector<std::string> titles;
        titles.reserve(static_cast<size_t>(kCount));
        for (int i = 0; i < kCount; i++) titles.push_back(Title(dialogue, i));
        return titles;
    }

private:
    static constexpr std::array<int, 12> kTitleRow = {6, 8, 11, 13, 19, 21, 24, 29, 31, 34, 37, 39};
    static inline const std::array<std::vector<int>, 12> kBodyRows = {{
        {7},
        {9, 10},
        {12},
        {14, 15, 16, 17, 18},
        {20},
        {22, 23},
        {25, 26, 27, 28},
        {30},
        {32, 33},
        {35, 36},
        {38},
        {40},
    }};
};

}  // namespace stormhold
