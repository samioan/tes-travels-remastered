#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "assets/cml.h"
#include "assets/image.h"
#include "assets/lang.h"
#include "graphics/backbuffer.h"

namespace oblivion {

// DialogueNode (src/DialogueNode.java): a line of a tab. A node with children
// is a submenu (drawn "<text>"), a leaf is actionable.
struct DialogueNode {
    DialogueNode* parent = nullptr;
    bool marked = false;     // the equipped / chosen sibling (single-select group)
    bool available = true;   // false: drawn red and not selectable (except in Buy/Sell)
    std::string text;
    std::string tooltip;     // shown in a box for the highlighted line
    bool hasTooltip = false;
    std::vector<std::unique_ptr<DialogueNode>> children;
    std::vector<std::string> answerLines;  // label/value pairs of the character sheet
    bool hasAnswer = false;

    DialogueNode() = default;
    explicit DialogueNode(std::string t) : text(std::move(t)) {}
    DialogueNode* Add(std::unique_ptr<DialogueNode> child) {
        child->parent = this;
        children.push_back(std::move(child));
        return children.back().get();
    }
};

// The shared list-menu screen (src/DialogueScreen.java): the inventory tabs
// (sprite groups {4,1,2,3,18}) and the shop's Buy/Sell tabs ({17,15,16}).
// Keys: 3 up, 4 down, 5 previous tab, 6 next tab, 7 select.
class DialogueScreen {
public:
    DialogueScreen(SpriteSet ui, ImageCache& images, const Strings& strings);

    bool open = false;
    std::string caption;
    bool hasCaption = false;
    // Game.menuSelected: a leaf was chosen (the node is still alive during the call).
    std::function<void(DialogueNode&)> onSelected;

    // `roots`: one tree per tab. `tabSprites[0]` is the tab strip, [1..] the
    // highlighted image of each tab.
    void OpenTabs(std::vector<uint8_t> tabSprites, std::vector<std::unique_ptr<DialogueNode>> roots,
                  const std::string* caption);
    void HandleKey(int action);
    bool GoBack();
    void Tick(int dtMs);
    void Paint(Backbuffer& bb);

private:
    void DrawFrame();
    void SetScrollState(int s);
    bool SameBuySellGroup(const DialogueNode& a, const DialogueNode& b) const;
    int LineHeight() const;
    std::string S(int id) const { return strings_.Get(id); }

    SpriteSet ui_;
    ImageCache& images_;
    const Strings& strings_;
    std::vector<uint8_t> tabSprites_;
    std::vector<std::unique_ptr<DialogueNode>> owners_;   // the trees
    std::vector<std::unique_ptr<DialogueNode>> retired_;  // replaced while a handler runs
    std::vector<DialogueNode*> roots_;                    // current node of each tab
    Backbuffer backdrop_;
    int cursor_ = 0, lastVisible_ = 0, firstVisible_ = 0, tab_ = 0;
    int textScroll_ = 0, scrollDir_ = 1, scrollState_ = 0, scrollPause_ = 0;
    int scrollY_ = 0, scrollTimer_ = 0;
};

}  // namespace oblivion
