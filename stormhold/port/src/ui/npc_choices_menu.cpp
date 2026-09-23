#include "ui/npc_choices_menu.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <vector>

#include "graphics/bitmap_font.h"
#include "player/player_combat_stats.h"
#include "player/player_inventory.h"

namespace stormhold {

namespace {

// Same small local palette/layout copy every other `ui/*.cpp` file in
// this port already keeps -- see e.g. `ui/pause_menu.cpp`'s own class
// comment for why this port duplicates rather than shares them.
constexpr uint16_t kTitleBg = PackRGB565(0, 0, 0);
constexpr uint16_t kTitleFg = PackRGB565(255, 255, 255);
constexpr uint16_t kBodyBg = PackRGB565(0xAE, 0x68, 0x2E);
constexpr uint16_t kItemFg = PackRGB565(255, 255, 0);
constexpr uint16_t kSelectedBg = PackRGB565(0x66, 0x66, 0x66);
constexpr uint16_t kBarBg = PackRGB565(255, 255, 255);
constexpr uint16_t kBarFg = PackRGB565(0, 0, 0);

constexpr int kLineHeight = BitmapFont::kGlyphHeight + 2;
constexpr int kContentTop = 16;
constexpr int kBarTop = Backbuffer::kHeight - 14;
constexpr int kMargin = 8;

std::vector<std::string> WordWrap(const std::string& text, int maxWidthPx) {
    std::vector<std::string> lines;
    size_t paraStart = 0;

    while (true) {
        size_t nl = text.find('\n', paraStart);
        std::string paragraph =
            (nl == std::string::npos) ? text.substr(paraStart) : text.substr(paraStart, nl - paraStart);

        std::vector<std::string> words;
        size_t p = 0;
        while (p < paragraph.size()) {
            while (p < paragraph.size() && paragraph[p] == ' ') p++;
            size_t wordStart = p;
            while (p < paragraph.size() && paragraph[p] != ' ') p++;
            if (p > wordStart) words.push_back(paragraph.substr(wordStart, p - wordStart));
        }

        std::string current;
        for (const std::string& word : words) {
            std::string candidate = current.empty() ? word : current + " " + word;
            if (BitmapFont::StringWidth(candidate) > maxWidthPx && !current.empty()) {
                lines.push_back(current);
                current = word;
            } else {
                current = candidate;
            }
        }
        lines.push_back(current);

        if (nl == std::string::npos) break;
        paraStart = nl + 1;
    }

    return lines;
}

void PaintPanel(Backbuffer& bb, const std::string& title) {
    bb.FillRect(0, 12, Backbuffer::kWidth, kBarTop - 12, kBodyBg);
    bb.FillRect(0, 0, Backbuffer::kWidth, 12, kTitleBg);
    int titleWidth = BitmapFont::StringWidth(title);
    BitmapFont::DrawString(bb, (Backbuffer::kWidth - titleWidth) / 2, 3, title, kTitleFg);
}

void PaintBottomBar(Backbuffer& bb, const std::string& leftLabel, const std::string& rightLabel) {
    bb.FillRect(0, kBarTop, Backbuffer::kWidth, Backbuffer::kHeight - kBarTop, kBarBg);
    if (!leftLabel.empty()) BitmapFont::DrawString(bb, 4, kBarTop + 4, leftLabel, kBarFg);
    if (!rightLabel.empty()) {
        int w = BitmapFont::StringWidth(rightLabel);
        BitmapFont::DrawString(bb, Backbuffer::kWidth - 4 - w, kBarTop + 4, rightLabel, kBarFg);
    }
}

void PaintList(Backbuffer& bb, const std::string& title, const std::vector<std::string>& header,
               const std::vector<std::string>& items, int selected) {
    PaintPanel(bb, title);
    int y = kContentTop;
    for (const std::string& line : header) {
        if (y + kLineHeight > kBarTop) break;
        BitmapFont::DrawString(bb, kMargin, y, line, kItemFg);
        y += kLineHeight;
    }
    if (!header.empty()) y += 2;

    for (size_t i = 0; i < items.size(); i++) {
        if (y + kLineHeight > kBarTop) break;
        if (static_cast<int>(i) == selected) {
            bb.FillRect(kMargin - 4, y - 1, Backbuffer::kWidth - 2 * (kMargin - 4), kLineHeight, kSelectedBg);
        }
        BitmapFont::DrawString(bb, kMargin, y, items[i], kItemFg);
        y += kLineHeight;
    }
}

// Shop.isValidShopAction(shopId, skillIndex) is scanned in ascending
// skillIndex order in both the original's trainWhatMenu() and here --
// ShopInteraction::ShopActionCode(shopId, choiceIndex) (M56) already
// relies on that same ascending-order match, confirmed by reading
// IsValidShopAction/ShopActionCode's own kCodes table side by side.
int ValidSkillCount(int shopId) {
    int count = 0;
    for (int skillIndex = 0; skillIndex < 14; skillIndex++) {
        if (ShopInteraction::IsValidShopAction(shopId, skillIndex)) count++;
    }
    return count;
}

// trainWhatMenu(shopId)'s own label: "<skillName> (<currentSkillValue>)".
std::vector<std::string> BuildTrainWhatList(int shopId, const PlayerState& p, const CharacterData& charData) {
    std::vector<std::string> out;
    for (int skillIndex = 0; skillIndex < 14; skillIndex++) {
        if (!ShopInteraction::IsValidShopAction(shopId, skillIndex)) continue;
        int value = PlayerCombatStats::SkillValue(p, charData, skillIndex, false);
        out.push_back(charData.skillNames[static_cast<size_t>(skillIndex)] + " (" + std::to_string(value) + ")");
    }
    return out;
}

// giveWhatMenu(shopId)'s own item list -- "E:<name>" for an equipped slot
// (NO space after the colon, confirmed by reading giveWhatMenu() directly
// -- a real, minor divergence from newInventoryUI()'s own "E: <name>"
// WITH a space; reproduced exactly, not normalized to match).
std::vector<std::string> BuildGiveWhatList(const PlayerState& p, const ItemDatabase& items) {
    std::vector<std::string> out;
    out.reserve(static_cast<size_t>(p.inventoryCount));
    for (int i = 0; i < p.inventoryCount; i++) {
        int8_t itemId = p.inventoryItemIds[static_cast<size_t>(i)];
        const std::string& name = items.name[static_cast<size_t>(std::abs(itemId) - 1)];
        out.push_back(PlayerInventory::IsEquippedSlot(p, i, items) ? "E:" + name : name);
    }
    return out;
}

// takeWhatMenu(shopId)'s own item list: Item.specialItemNames() inlined
// directly (a one-line `name[86+i]` read in the original, same "don't
// give a one-line real read its own wrapper" precedent
// player/shop_interaction.h's own class comment already uses for
// itemSubtypeAtSlot) -- the 13 real "gift" item names (ids 87-99).
std::vector<std::string> BuildTakeWhatList(const ItemDatabase& items) {
    std::vector<std::string> out;
    out.reserve(13);
    for (int i = 0; i < 13; i++) {
        out.push_back(items.name[static_cast<size_t>(86 + i)]);
    }
    return out;
}

// enchantWhatMenu(shopId)'s own item list -- Helga's own catalog, the
// player's full inventory UNPREFIXED (confirmed by reading
// enchantWhatMenu() directly: unlike giveWhatMenu, it never checks
// isEquippedSlot at all), so the selected row equals the slot index
// directly, same as GiveWhat.
std::vector<std::string> BuildEnchantWhatList(const PlayerState& p, const ItemDatabase& items) {
    std::vector<std::string> out;
    out.reserve(static_cast<size_t>(p.inventoryCount));
    for (int i = 0; i < p.inventoryCount; i++) {
        int8_t itemId = p.inventoryItemIds[static_cast<size_t>(i)];
        out.push_back(items.name[static_cast<size_t>(std::abs(itemId) - 1)]);
    }
    return out;
}

}  // namespace

void NpcChoicesMenu::Open(NpcChoicesMenuState& state, int shopId) {
    state.active = true;
    state.shopId = shopId;
    state.screen = NpcChoicesScreen::Choices;
    state.selectedIndex = 0;
    state.resultCloses = false;
}

void NpcChoicesMenu::MoveSelection(NpcChoicesMenuState& state, int delta, const PlayerState& p) {
    if (!state.active) return;

    int count = 0;
    switch (state.screen) {
        case NpcChoicesScreen::Choices:
            // Shops 0-3: Train/Give/Befriend/Threaten/Kill (5). Beneca
            // (shopId 4): Give Item/Take Crystal (2). Helga (shopId 5):
            // Rumors/Give Crystal/Enchant/Bless/Cure/Warp/Recovery (7).
            // Each count matches that shop's own separately-built items
            // array exactly.
            count = state.shopId <= 3 ? 5 : (state.shopId == 4 ? 2 : 7);
            break;
        case NpcChoicesScreen::TrainWhat:
            count = ValidSkillCount(state.shopId);
            break;
        case NpcChoicesScreen::GiveWhat:
            count = p.inventoryCount;
            break;
        case NpcChoicesScreen::TakeWhat:
            count = 13;
            break;
        case NpcChoicesScreen::EnchantWhat:
            count = p.inventoryCount;
            break;
        case NpcChoicesScreen::Result:
            count = 0;
            break;
    }

    if (count <= 0) {
        state.selectedIndex = 0;
        return;
    }
    state.selectedIndex = std::max(0, std::min(state.selectedIndex + delta, count - 1));
}

namespace {

// giveWhatMenu is shared by every shop group, but the real dialogue()
// call it feeds differs (QuestShopDialogue for shops 0-3, BenecaDialogue
// for shop 4, HelgaDialogue for shop 5) -- all three take the same
// (action=4, extra=slot) shape, just through different functions, so this
// is the one place that dispatch needs to branch by shopId.
std::optional<std::string> DialogueFor(int shopId, int action, int extra, PlayerState& p, ShopState& shop,
                                        const ShopDialogue& text, const CharacterData& charData,
                                        const ItemDatabase& items, GeneratedLevel& hub, JavaRandom& rng,
                                        int16_t& spawnIdCounter) {
    if (shopId <= 3) {
        return ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, shopId, action, extra);
    }
    if (shopId == 4) {
        return ShopInteraction::BenecaDialogue(p, shop, text, items, spawnIdCounter, action, extra);
    }
    return ShopInteraction::HelgaDialogue(p, shop, text, items, action, extra);
}

}  // namespace

void NpcChoicesMenu::Confirm(NpcChoicesMenuState& state, PlayerState& p, ShopState& shop, const ShopDialogue& text,
                              const CharacterData& charData, const ItemDatabase& items, GeneratedLevel& hub,
                              JavaRandom& rng, int16_t& spawnIdCounter) {
    if (!state.active) return;

    // Result's own single "Ok" -- closes the whole menu for the 2
    // confirmed-working real results (Kill, Warp -- `resultCloses`), or
    // returns to Choices for every other, confirmed-dead result (the
    // real result popups have no reachable nextScreen/backTarget at
    // all). See this file's own header comment for the full breakdown.
    if (state.screen == NpcChoicesScreen::Result) {
        if (state.resultCloses) {
            state.active = false;
        } else {
            state.screen = NpcChoicesScreen::Choices;
            state.selectedIndex = 0;
        }
        return;
    }

    if (state.screen == NpcChoicesScreen::TrainWhat) {
        int extra = ShopInteraction::ShopActionCode(state.shopId, state.selectedIndex);
        if (extra < 0) return;
        std::optional<std::string> result =
            ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, state.shopId, 5, extra);
        state.resultTitle = "NPC name here";  // npcResponsePopup's own dead-write placeholder, see class comment.
        state.resultBody = result.value_or("");
        state.resultCloses = false;  // screenGroup 21, a confirmed dead end.
        state.screen = NpcChoicesScreen::Result;
        state.selectedIndex = 0;
        return;
    }

    if (state.screen == NpcChoicesScreen::GiveWhat) {
        int slot = state.selectedIndex;
        if (slot < 0 || slot >= p.inventoryCount) return;
        std::optional<std::string> result =
            DialogueFor(state.shopId, 4, slot, p, shop, text, charData, items, hub, rng, spawnIdCounter);
        state.resultTitle = "NPC name here";
        state.resultBody = result.value_or("");
        state.resultCloses = false;  // screenGroup 23, a confirmed dead end for every shop group.
        state.screen = NpcChoicesScreen::Result;
        state.selectedIndex = 0;
        return;
    }

    if (state.screen == NpcChoicesScreen::TakeWhat) {
        // BenecaDialogue's action 7: `extra` is a real item id (87-99),
        // NOT a slot -- see BenecaDialogue's own doc comment for the
        // confirmed naming trap this matches exactly.
        int itemId = state.selectedIndex + 87;
        std::optional<std::string> result =
            ShopInteraction::BenecaDialogue(p, shop, text, items, spawnIdCounter, 7, itemId);
        state.resultTitle = "NPC name here";
        state.resultBody = result.value_or("");
        state.resultCloses = false;  // screenGroup 28, a confirmed dead end.
        state.screen = NpcChoicesScreen::Result;
        state.selectedIndex = 0;
        return;
    }

    if (state.screen == NpcChoicesScreen::EnchantWhat) {
        // HelgaDialogue's action 8: `extra` IS a slot here (confirmed by
        // reading enchantWhatMenu()'s own selectedIndex-as-extra call
        // site directly -- unlike BenecaDialogue's action 7, no naming
        // trap this time).
        int slot = state.selectedIndex;
        if (slot < 0 || slot >= p.inventoryCount) return;
        std::optional<std::string> result = ShopInteraction::HelgaDialogue(p, shop, text, items, 8, slot);
        state.resultTitle = "NPC name here";
        state.resultBody = result.value_or("");
        state.resultCloses = false;  // screenGroup 351, a confirmed dead end.
        state.screen = NpcChoicesScreen::Result;
        state.selectedIndex = 0;
        return;
    }

    // Choices screen: dispatchNpcChoice()'s own case 0/1/2/3 group (shops
    // 0-3), case 4 (Beneca), or case 5 (Helga) -- see class comment for
    // the confirmed per-shop item-count difference (5/2/7).
    if (state.shopId <= 3) {
        switch (state.selectedIndex) {
            case 0:  // Train
                state.screen = NpcChoicesScreen::TrainWhat;
                state.selectedIndex = 0;
                return;
            case 1:  // Give
                if (p.inventoryCount <= 0) {
                    state.resultTitle = "Oracle";  // npcResponseUI's own leftover placeholder, see class comment.
                    state.resultBody = "You have nothing to give me!";
                    state.resultCloses = false;
                    state.screen = NpcChoicesScreen::Result;
                } else {
                    state.screen = NpcChoicesScreen::GiveWhat;
                }
                state.selectedIndex = 0;
                return;
            case 2: {  // Befriend
                std::optional<std::string> result =
                    ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, state.shopId, 2, 0);
                state.resultTitle = "NPC name here";
                state.resultBody = result.value_or("");
                state.resultCloses = false;  // screenGroup 24, a confirmed dead end.
                state.screen = NpcChoicesScreen::Result;
                state.selectedIndex = 0;
                return;
            }
            case 3: {  // Threaten
                std::optional<std::string> result =
                    ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, state.shopId, 3, 0);
                state.resultTitle = "NPC name here";
                state.resultBody = result.value_or("");
                state.resultCloses = false;  // screenGroup 25, a confirmed dead end.
                state.screen = NpcChoicesScreen::Result;
                state.selectedIndex = 0;
                return;
            }
            case 4: {  // Kill
                std::optional<std::string> result =
                    ShopInteraction::QuestShopDialogue(p, shop, text, charData, items, hub, rng, state.shopId, 6, 0);
                state.resultTitle = "NPC name here";
                state.resultBody = result.value_or("");
                // screenGroup 26 -- a real, WORKING, unconditional
                // showScreen(gameCanvas) handler (see class comment's own
                // corrected-M64-bug writeup). Closes the whole menu.
                state.resultCloses = true;
                state.screen = NpcChoicesScreen::Result;
                state.selectedIndex = 0;
                return;
            }
            default:
                return;
        }
    }

    if (state.shopId == 4) {
        // Beneca: Give Item (0) / Take Crystal (1).
        switch (state.selectedIndex) {
            case 0:  // Give Item
                if (p.inventoryCount <= 0) {
                    state.resultTitle = "Oracle";
                    state.resultBody = "You have nothing to give me!";
                    state.resultCloses = false;
                    state.screen = NpcChoicesScreen::Result;
                } else {
                    state.screen = NpcChoicesScreen::GiveWhat;
                }
                state.selectedIndex = 0;
                return;
            case 1:  // Take Crystal
                state.screen = NpcChoicesScreen::TakeWhat;
                state.selectedIndex = 0;
                return;
            default:
                return;
        }
    }

    // Helga (shopId 5): Rumors (0) / Give Crystal (1) / Enchant (2) /
    // Bless (3) / Cure (4) / Warp (5) / Recovery (6).
    switch (state.selectedIndex) {
        case 0: {  // Rumors -- showRumors()'s own dialogue(5, 13, 0) call.
            std::optional<std::string> result = ShopInteraction::HelgaDialogue(p, shop, text, items, 13, 0);
            state.resultTitle = "Rumors";  // rumorsUI's own dead-write placeholder, see class comment.
            state.resultBody = result.value_or("No rumors!");  // showRumors()'s own real fallback text.
            state.resultCloses = false;  // rumorsUI's own confirmed softlock, not reproduced -- see class comment.
            state.screen = NpcChoicesScreen::Result;
            state.selectedIndex = 0;
            return;
        }
        case 1:  // Give Crystal
            if (p.inventoryCount <= 0) {
                state.resultTitle = "Oracle";
                state.resultBody = "You have nothing to give me!";
                state.resultCloses = false;
                state.screen = NpcChoicesScreen::Result;
            } else {
                state.screen = NpcChoicesScreen::GiveWhat;
            }
            state.selectedIndex = 0;
            return;
        case 2:  // Enchant
            state.screen = NpcChoicesScreen::EnchantWhat;
            state.selectedIndex = 0;
            return;
        case 3: {  // Bless
            std::optional<std::string> result = ShopInteraction::HelgaDialogue(p, shop, text, items, 9, 0);
            state.resultTitle = "NPC name here";
            state.resultBody = result.value_or("");
            state.resultCloses = false;  // screenGroup 352, a confirmed dead end.
            state.screen = NpcChoicesScreen::Result;
            state.selectedIndex = 0;
            return;
        }
        case 4: {  // Cure
            std::optional<std::string> result = ShopInteraction::HelgaDialogue(p, shop, text, items, 10, 0);
            state.resultTitle = "NPC name here";
            state.resultBody = result.value_or("");
            state.resultCloses = false;  // screenGroup 353, a confirmed dead end.
            state.screen = NpcChoicesScreen::Result;
            state.selectedIndex = 0;
            return;
        }
        case 5: {  // Warp
            std::optional<std::string> result = ShopInteraction::HelgaDialogue(p, shop, text, items, 11, 0);
            // screenGroup 41's own real handler clears this on dismissal;
            // applied here at result-creation time instead -- observably
            // identical, since nothing else reads this flag in between
            // (see class comment).
            p.justMarkedCamp = false;
            state.resultTitle = "NPC name here";
            state.resultBody = result.value_or("");
            // screenGroup 41 -- a real, WORKING `cmd == cmdOk` handler
            // (reused from the camp-mark-confirmation screen, see class
            // comment). Closes the whole menu.
            state.resultCloses = true;
            state.screen = NpcChoicesScreen::Result;
            state.selectedIndex = 0;
            return;
        }
        case 6: {  // Recovery
            std::optional<std::string> result = ShopInteraction::HelgaDialogue(p, shop, text, items, 12, 0);
            state.resultTitle = "NPC name here";
            state.resultBody = result.value_or("");
            state.resultCloses = false;  // screenGroup 355, a confirmed dead end.
            state.screen = NpcChoicesScreen::Result;
            state.selectedIndex = 0;
            return;
        }
        default:
            return;
    }
}

void NpcChoicesMenu::Cancel(NpcChoicesMenuState& state) {
    if (!state.active) return;
    if (state.screen == NpcChoicesScreen::Result) {
        if (state.resultCloses) {
            state.active = false;
        } else {
            state.screen = NpcChoicesScreen::Choices;
            state.selectedIndex = 0;
        }
        return;
    }
    if (state.screen == NpcChoicesScreen::TakeWhat) {
        // A real, confirmed EXCEPTION to every other sub-screen's own
        // Cancel behavior -- see class comment for why takeWhatMenu's own
        // nextScreen=null makes this the genuine real behavior, not an
        // inconsistency on this port's side.
        state.screen = NpcChoicesScreen::Choices;
        state.selectedIndex = 0;
        return;
    }
    // Choices/TrainWhat/GiveWhat/EnchantWhat all real-target gameCanvas
    // directly -- see class comment's own Cancel doc.
    state.active = false;
}

void NpcChoicesMenu::Render(Backbuffer& bb, const NpcChoicesMenuState& state, const PlayerState& p,
                             const CharacterData& charData, const ItemDatabase& items, const ShopState& shop) {
    switch (state.screen) {
        case NpcChoicesScreen::Choices: {
            // Real per-shop-group point value (RefreshChoicesMenuGiftLabel's
            // own value computation) -- rewardsGiven for quest shops 0-3,
            // benecaPoints for shop 4, helgaPoints for shop 5. This port
            // always rebuilds this fresh rather than caching/patching a
            // `tagTemplate`, same "port-only always fresh" choice
            // M62/M63 already made.
            int value = state.shopId <= 3 ? shop.rewardsGiven[static_cast<size_t>(state.shopId)]
                        : state.shopId == 4 ? shop.benecaPoints
                                            : shop.helgaPoints;
            std::string prompt = "Aid: " + std::to_string(value);
            // Title: literal "Name" for shops 0-3 (a real, confirmed
            // never-patched placeholder), the shop's own real name for
            // Beneca/Helga (their own separate constructions pass it
            // correctly) -- see class comment's own "fourth finding".
            std::string title = state.shopId <= 3 ? "Name" : Shop::kNames[static_cast<size_t>(state.shopId)];
            std::vector<std::string> items_;
            if (state.shopId <= 3) {
                items_ = {"Train", "Give", "Befriend", "Threaten", "Kill"};
            } else if (state.shopId == 4) {
                items_ = {"Give Item", "Take Crystal"};
            } else {
                items_ = {"Rumors", "Give Crystal", "Enchant", "Bless", "Cure", "Warp", "Recovery"};
            }
            PaintList(bb, title, {prompt}, items_, state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        }
        case NpcChoicesScreen::TrainWhat:
            PaintList(bb, Shop::kNames[static_cast<size_t>(state.shopId)], {"Train What?"},
                      BuildTrainWhatList(state.shopId, p, charData), state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        case NpcChoicesScreen::GiveWhat:
            PaintList(bb, Shop::kNames[static_cast<size_t>(state.shopId)], {"Give What?"},
                      BuildGiveWhatList(p, items), state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        case NpcChoicesScreen::TakeWhat:
            PaintList(bb, Shop::kNames[static_cast<size_t>(state.shopId)], {"Take What?"}, BuildTakeWhatList(items),
                      state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        case NpcChoicesScreen::EnchantWhat:
            PaintList(bb, Shop::kNames[static_cast<size_t>(state.shopId)], {"Enchant What?"},
                      BuildEnchantWhatList(p, items), state.selectedIndex);
            PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
            return;
        case NpcChoicesScreen::Result: {
            std::vector<std::string> header = WordWrap(state.resultBody, Backbuffer::kWidth - 2 * kMargin);
            PaintList(bb, state.resultTitle, header, {}, -1);
            PaintBottomBar(bb, "Enter: Ok", "");
            return;
        }
    }
}

}  // namespace stormhold
