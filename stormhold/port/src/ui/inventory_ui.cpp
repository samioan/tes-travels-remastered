#include "ui/inventory_ui.h"

#include <algorithm>
#include <cstdlib>

#include "graphics/bitmap_font.h"

namespace stormhold {

namespace {

// Same palette `ui/npc_dialogue.cpp`/`ui/menu_flow.cpp` already use --
// kept identical rather than inventing a separate port-only palette for
// what is, structurally, the same kind of screen (see those files' own
// class comments for why each keeps its own small local copy).
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

// Small, deliberate duplicate of `ui/npc_dialogue.cpp`'s own file-local
// WordWrap (itself a deliberate duplicate of `ui/menu_flow.cpp`'s) --
// splits on '\n' as a real paragraph break first, matching every
// `PlayerInventory::ItemTooltip` result's own '\n'-joined line layout.
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
    // M74: y nudged from 3 to 1 -- see bitmap_font.cpp's own kFontHeightPx
    // comment for why the new GDI font needs the extra headroom to stay
    // inside this 12px bar.
    BitmapFont::DrawString(bb, (Backbuffer::kWidth - titleWidth) / 2, 1, title, kTitleFg);
}

void PaintBottomBar(Backbuffer& bb, const std::string& leftLabel, const std::string& rightLabel) {
    bb.FillRect(0, kBarTop, Backbuffer::kWidth, Backbuffer::kHeight - kBarTop, kBarBg);
    if (!leftLabel.empty()) BitmapFont::DrawString(bb, 4, kBarTop + 4, leftLabel, kBarFg);
    if (!rightLabel.empty()) {
        int w = BitmapFont::StringWidth(rightLabel);
        BitmapFont::DrawString(bb, Backbuffer::kWidth - 4 - w, kBarTop + 4, rightLabel, kBarFg);
    }
}

// `header` lines are drawn as-is (already word-wrapped by the caller when
// needed); `items` is the selectable list below them, matching
// `ui/menu_flow.cpp`'s own `PaintList` layout exactly.
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

// newInventoryItemUI()'s own `labels`/`codes` construction, byte-for-byte
// (Drop always first, then Equip-or-Unequip/Learn/Use each conditionally).
void BuildActionMenu(InventoryUiState& state, const PlayerState& p, int slot, const ItemDatabase& items,
                      const SpellDatabase& spells) {
    state.actionLabels.clear();
    state.actionCodes.clear();
    state.actionLabels.emplace_back("Drop");
    state.actionCodes.push_back(0);

    if (PlayerInventory::CanEquipOrUnequip(p, slot, items)) {
        state.actionLabels.emplace_back(PlayerInventory::IsEquippedSlot(p, slot, items) ? "Unequip" : "Equip");
        state.actionCodes.push_back(1);
    }
    if (PlayerInventory::CanLearnSpellFromScroll(p, slot, items, spells)) {
        state.actionLabels.emplace_back("Learn");
        state.actionCodes.push_back(2);
    }
    if (PlayerInventory::CanUseItem(p, slot, items)) {
        state.actionLabels.emplace_back("Use");
        state.actionCodes.push_back(3);
    }
}

// newInventoryUI()'s own item-name list: "E: <name>" for an equipped
// (negative id) slot, plain name otherwise.
std::vector<std::string> BuildItemList(const PlayerState& p, const ItemDatabase& items) {
    std::vector<std::string> out;
    out.reserve(static_cast<size_t>(p.inventoryCount));
    for (int i = 0; i < p.inventoryCount; i++) {
        int8_t itemId = p.inventoryItemIds[static_cast<size_t>(i)];
        const std::string& name = items.name[static_cast<size_t>(std::abs(itemId) - 1)];
        out.push_back(itemId < 0 ? "E: " + name : name);
    }
    return out;
}

}  // namespace

void InventoryUi::Open(InventoryUiState& state) {
    state.active = true;
    state.screen = InventoryScreen::List;
    state.selectedIndex = 0;
    state.selectedSlot = -1;
    state.actionLabels.clear();
    state.actionCodes.clear();
}

void InventoryUi::MoveSelection(InventoryUiState& state, int delta, const PlayerState& p) {
    if (!state.active) return;
    int count = (state.screen == InventoryScreen::List) ? p.inventoryCount
                                                          : static_cast<int>(state.actionLabels.size());
    if (count <= 0) {
        state.selectedIndex = 0;
        return;
    }
    state.selectedIndex = std::max(0, std::min(state.selectedIndex + delta, count - 1));
}

void InventoryUi::Confirm(InventoryUiState& state, PlayerState& p, const ItemDatabase& items,
                           const SpellDatabase& spells, const MonsterDatabase& monsters, GeneratedLevel& level,
                           WorldRegistry& world, JavaRandom& rng, const GameAdvancement::LevelLookup& levels) {
    if (!state.active) return;

    if (state.screen == InventoryScreen::List) {
        int slot = state.selectedIndex;
        if (slot < 0 || slot >= p.inventoryCount) return;
        state.selectedSlot = slot;
        BuildActionMenu(state, p, slot, items, spells);
        state.screen = InventoryScreen::ItemAction;
        state.selectedIndex = 0;
        return;
    }

    // ItemAction screen: dispatch the chosen action, matching
    // ESGame.java's own screenGroup 34 switch exactly (action==0 Drop,
    // 1 Equip-or-Unequip, 2 Learn, 3 Use).
    if (state.selectedIndex < 0 || state.selectedIndex >= static_cast<int>(state.actionCodes.size())) return;
    int action = state.actionCodes[static_cast<size_t>(state.selectedIndex)];
    int slot = state.selectedSlot;

    if (action == 0) {
        PlayerInventory::DropInventoryItem(p, slot, items, level, world);
    } else if (action == 1) {
        if (!PlayerInventory::IsEquippedSlot(p, slot, items)) {
            PlayerInventory::EquipItem(p, slot, true, items);
        } else {
            PlayerInventory::UnequipSlot(p, slot, items);
        }
    } else if (action == 2) {
        PlayerInventory::LearnSpellFromScroll(p, slot, items);
    } else if (action == 3) {
        // target is always nullptr -- see this file's own header comment
        // and PlayerInventory::UseItem's doc comment for the confirmed
        // "real call site always passes null" finding.
        PlayerInventory::UseItem(p, slot, nullptr, items, monsters, world, rng, levels);
    }

    state.selectedSlot = -1;
    state.screen = InventoryScreen::List;
    state.selectedIndex = 0;
}

void InventoryUi::Cancel(InventoryUiState& state) {
    if (!state.active) return;
    if (state.screen == InventoryScreen::ItemAction) {
        state.screen = InventoryScreen::List;
        state.selectedSlot = -1;
        state.selectedIndex = 0;
    } else {
        state.active = false;
    }
}

void InventoryUi::Render(Backbuffer& bb, const InventoryUiState& state, const PlayerState& p, const ItemDatabase& items,
                          const SpellDatabase& spells, const CharacterData& charData) {
    if (state.screen == InventoryScreen::List) {
        std::vector<std::string> itemList = BuildItemList(p, items);
        PaintList(bb, "Inventory", {"Items:"}, itemList, state.selectedIndex);
        PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
        return;
    }

    std::string tooltip;
    if (state.selectedSlot >= 0 && state.selectedSlot < p.inventoryCount) {
        tooltip = PlayerInventory::ItemTooltip(p, state.selectedSlot, items, spells, charData);
    }
    std::vector<std::string> header = WordWrap(tooltip, Backbuffer::kWidth - 2 * kMargin);
    PaintList(bb, "Item", header, state.actionLabels, state.selectedIndex);
    PaintBottomBar(bb, "Enter: Ok", "Esc: Back");
}

}  // namespace stormhold
