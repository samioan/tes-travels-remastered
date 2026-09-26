#include "game/dialogue_screen.h"

#include <algorithm>
#include <cctype>

#include "graphics/text.h"
#include "render/sprite_renderer.h"

namespace oblivion {

namespace {

using Text::Face;

constexpr int kW = Backbuffer::kWidth;
constexpr int kH = Backbuffer::kHeight;

}  // namespace

DialogueScreen::DialogueScreen(SpriteSet ui, ImageCache& images, const Strings& strings)
    : ui_(std::move(ui)), images_(images), strings_(strings) {}

int DialogueScreen::LineHeight() const { return Text::LineHeight(Face::SmallPlain); }

void DialogueScreen::OpenTabs(std::vector<uint8_t> tabSprites, std::vector<std::unique_ptr<DialogueNode>> roots,
                              const std::string* captionText) {
    tabSprites_ = std::move(tabSprites);
    // Handlers may call this while one of the old nodes is still on their stack.
    for (auto& o : owners_) retired_.push_back(std::move(o));
    owners_ = std::move(roots);
    roots_.clear();
    for (auto& o : owners_) roots_.push_back(o.get());
    open = true;
    tab_ = 0;
    hasCaption = captionText != nullptr;
    caption = captionText ? *captionText : std::string();
    cursor_ = lastVisible_ = firstVisible_ = 0;
    scrollY_ = 0;
    textScroll_ = 0;
    scrollState_ = 0;
    DrawFrame();
}

void DialogueScreen::DrawFrame() {
    Backbuffer& g = backdrop_;
    g.Fill(0);
    const int fh = Text::LineHeight(Face::SmallPlain);
    const int w13 = ui_.Width(13), h13 = ui_.Height(13), h11 = ui_.Height(11), h12 = ui_.Height(12);
    const int w12 = ui_.Width(12), w8 = ui_.Width(8), h5 = ui_.Height(5), w5 = ui_.Width(5);
    const int bottom = kH - fh - 4;
    if (w13 > 0 && h13 > 0)
        for (int x = 0; x < kW; x += w13)
            for (int y = 0; y < bottom - h13; y += h13) DrawSprite(g, images_, ui_, 13, x, y);
    if (h11 > 0)
        for (int y = 0; y < bottom - h11; y += h11) DrawSprite(g, images_, ui_, 11, 0, y);
    if (h12 > 0)
        for (int y = 0; y < bottom - h12; y += h12) DrawSprite(g, images_, ui_, 12, kW - w12, y);
    if (w8 > 0)
        for (int x = 0; x < kW; x += w8) DrawSprite(g, images_, ui_, 8, x, 0);
    if (w5 > 0)
        for (int x = 0; x < kW; x += w5) DrawSprite(g, images_, ui_, 5, x, bottom - h5);
    DrawSprite(g, images_, ui_, 9, 0, 0);
    DrawSprite(g, images_, ui_, 10, kW - ui_.Width(10), 0);
    DrawSprite(g, images_, ui_, 6, 0, bottom - ui_.Height(6));
    DrawSprite(g, images_, ui_, 7, kW - ui_.Width(7), bottom - ui_.Height(7));
}

bool DialogueScreen::GoBack() {
    if (roots_.empty() || !roots_[tab_]) return false;
    if (roots_[tab_]->parent) {
        roots_[tab_] = roots_[tab_]->parent;
        cursor_ = lastVisible_ = firstVisible_ = 0;
        scrollY_ = 0;
        return true;
    }
    return false;
}

void DialogueScreen::SetScrollState(int s) {
    if (s != scrollState_) {
        scrollTimer_ = 0;
        textScroll_ = 0;
        scrollDir_ = 1;
        scrollPause_ = 0;
        scrollState_ = s;
    }
}

void DialogueScreen::Tick(int dt) {
    if (scrollState_ != 1) return;
    scrollTimer_ += dt;
    if (scrollPause_ == 1) {
        if (scrollTimer_ >= 1000) {
            scrollTimer_ = 0;
            scrollPause_ = 0;
        }
    } else if (scrollTimer_ >= 500) {
        textScroll_ += scrollDir_;
        scrollTimer_ = 0;
    }
}

bool DialogueScreen::SameBuySellGroup(const DialogueNode& a, const DialogueNode& b) const {
    auto is = [&](const DialogueNode& n, int id) { return n.text == S(id); };
    if ((!is(a, 149) && !is(a, 151)) || (!is(b, 149) && !is(b, 151)))
        return (is(a, 150) || is(a, 152)) && (is(b, 150) || is(b, 152));
    return true;
}

void DialogueScreen::HandleKey(int action) {
    if (roots_.empty()) return;
    const int lh = LineHeight();
    const int fh = Text::LineHeight(Face::SmallPlain);
    auto answerLimit = [&]() { return kH - fh - 4 - ui_.Height(5); };
    DialogueNode* root = roots_[tab_];
    const int count = static_cast<int>(root->children.size());
    if (action == 4) {
        if (++cursor_ >= count) cursor_ = 0;
        if (root->hasAnswer) {
            scrollY_ -= lh;
            if ((static_cast<int>(root->answerLines.size()) >> 1) * lh + scrollY_ + (lh << 2) < answerLimit()) scrollY_ += lh;
        }
    } else if (action == 3) {
        if (--cursor_ < 0) cursor_ = count - 1;
        if (root->hasAnswer) {
            scrollY_ += lh;
            if (scrollY_ > 0) scrollY_ = 0;
        }
    } else if (action == 5 || action == 6) {
        GoBack();
        const int tabs = static_cast<int>(tabSprites_.size()) - 1;
        if (action == 5) {
            if (--tab_ < 0) tab_ = tabs - 1;
        } else if (++tab_ == tabs) {
            tab_ = 0;
        }
        cursor_ = lastVisible_ = firstVisible_ = 0;
        scrollY_ = 0;
    } else if (action == 7 && cursor_ >= 0 && cursor_ < count) {
        DialogueNode* node = root->children[static_cast<size_t>(cursor_)].get();
        if (root->text != S(36) && root->text != S(37) && !node->available) return;
        if (!node->children.empty()) {
            roots_[tab_] = node;
            cursor_ = 0;
        } else {
            for (auto& sib : root->children) {
                if (node->parent->text == S(27)) {
                    if (SameBuySellGroup(*sib, *node)) sib->marked = false;
                } else {
                    sib->marked = false;
                }
            }
            node->marked = true;
        }
        if (onSelected) onSelected(*node);
    }
    retired_.clear();  // any tree replaced by the handler above may go now

    if (cursor_ > lastVisible_) scrollY_ = -lh * (cursor_ - (lastVisible_ - firstVisible_));
    else if (cursor_ < firstVisible_) scrollY_ = -lh * cursor_;
    SetScrollState(0);
}

void DialogueScreen::Paint(Backbuffer& bb) {
    if (roots_.empty()) return;
    bb.CopyIn(backdrop_);
    const int lh = LineHeight();
    const int fh = Text::LineHeight(Face::SmallPlain);
    const int h5 = ui_.Height(5);
    const int footerTop = kH - fh - 4 - h5;  // top of the bottom border strip
    DialogueNode* root = roots_[tab_];
    bool more = false, moreBelow = false;

    if (tabSprites_.size() > 1) {
        const int g0 = static_cast<int8_t>(tabSprites_[0]);
        const int gt = static_cast<int8_t>(tabSprites_[static_cast<size_t>(tab_) + 1]);
        DrawSprite(bb, images_, ui_, g0, (kW >> 1) - (ui_.Width(g0) >> 1), kH - fh - 4 - ui_.Height(g0));
        DrawSprite(bb, images_, ui_, gt, (kW >> 1) - (ui_.Width(gt) >> 1), kH - fh - 4 - ui_.Height(gt));
    }
    Text::DrawString(bb, (kW >> 1) - (Text::StringWidth(root->text, Face::SmallPlain) >> 1), 12, root->text, 0,
                     Face::SmallPlain);
    if (hasCaption)
        Text::DrawString(bb, (kW >> 1) - (Text::StringWidth(caption, Face::SmallPlain) >> 1), kH - h5 - (lh << 1),
                         caption, 0, Face::SmallPlain);

    int y = scrollY_ + 12 + (lh << 1);
    firstVisible_ = -1;
    int i = 0;
    const int n = static_cast<int>(root->children.size());
    for (; i < n; i++) {
        DialogueNode& node = *root->children[static_cast<size_t>(i)];
        if (y >= 12 + (lh << 1)) {
            if (firstVisible_ == -1) firstVisible_ = i;
            uint32_t color;
            if (i == cursor_) {
                bb.FillRect(15, y, kW - 30, lh, 16448974);
                if (node.hasTooltip) {
                    const int boxY = kH - fh - 4 - h5 - (lh << 1) - 6;
                    bb.DrawRect(20, boxY, kW - 40, lh + 4, 0);
                    Text::DrawString(bb, 23, boxY + 3, node.tooltip, 0, Face::SmallPlain);
                }
                color = node.available ? 10318649 : 0xFF0000;
            } else {
                color = node.available ? 0 : 0xFF0000;
            }
            Face face;
            int indent;
            if (node.marked) {
                DrawSprite(bb, images_, ui_, 14, 15, y);
                face = Face::SmallBold;
                indent = 15;
            } else {
                face = Face::SmallPlain;
                indent = 0;
            }
            if (node.children.empty()) {
                std::string shown = node.text, drawn = node.text;
                bool clipped = false;
                if (i == cursor_) shown = drawn = drawn.substr(std::min<size_t>(static_cast<size_t>(textScroll_), drawn.size()));
                while (!shown.empty() && ui_.Width(12) + indent + 15 > kW - Text::StringWidth(drawn, face)) {
                    clipped = true;
                    shown.pop_back();
                    drawn = shown + "...";
                }
                if (i == cursor_) {
                    if (clipped) {
                        if (scrollDir_ == -1 && textScroll_ == 0) {
                            scrollDir_ = 1;
                            scrollPause_ = 1;
                        }
                        SetScrollState(1);
                    } else if (scrollState_ == 1 && scrollDir_ == 1) {
                        scrollDir_ = -1;
                        scrollPause_ = 1;
                    }
                }
                Text::DrawString(bb, 15 + indent, y, drawn, color, face);
            } else {
                Text::DrawString(bb, 15 + indent, y, "<" + node.text + ">", color, face);
            }
        } else {
            more = true;
            moreBelow = true;
        }
        y += lh;
        if (y + (lh << 1) >= kH - fh - 4 - h5 - (lh << 1)) {
            more = true;
            moreBelow = true;
            break;
        }
    }

    if (root->hasAnswer) {
        const auto& lines = root->answerLines;
        y = scrollY_ + lh * 3;
        for (size_t k = 0; k < lines.size(); k += 2) {
            if (y >= 12 + (lh << 1)) {
                if (!lines[k].empty()) Text::DrawString(bb, 10, y, lines[k], 0, Face::SmallBold);
                if (k + 1 < lines.size() && !lines[k + 1].empty())
                    Text::DrawString(bb, 15 + Text::StringWidth(lines[k], Face::SmallBold), y, lines[k + 1], 0xFF0000,
                                     Face::SmallPlain);
            } else {
                more = true;
            }
            y += lh;
            if (y + lh >= kH - fh - 4 - h5) {
                if (k + 2 < lines.size()) moreBelow = true;
                break;
            }
        }
    }
    (void)footerTop;

    lastVisible_ = i;
    std::string back = S(449);
    for (char& c : back) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    Text::DrawString(bb, 2, kH - fh - 2, back, 0xFF0000, Face::SmallPlain);
    if (more) DrawSprite(bb, images_, ui_, 54, kW - ui_.Width(54) - 10, 35);
    if (moreBelow)
        DrawSprite(bb, images_, ui_, 53, kW - ui_.Width(53) - 10, kH - fh - ui_.Height(53) - h5 - 6);
}

}  // namespace oblivion
