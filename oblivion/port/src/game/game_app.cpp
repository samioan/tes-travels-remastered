#include "game/game_app.h"

#include <algorithm>
#include <cctype>

#include "graphics/text.h"
#include "render/sprite_renderer.h"

namespace oblivion {

namespace {

using Text::Face;

constexpr int kW = Backbuffer::kWidth;
constexpr int kH = Backbuffer::kHeight;
constexpr int kCenterX = kW / 2;
constexpr int kCenterY = kH / 2;
constexpr uint32_t kWhite = 0xFFFFFF;
constexpr uint32_t kPaper = 15327683;  // 0xE9DFC3: the level-text parchment
constexpr uint32_t kTitleBlue = 14483456;  // header text colour of the help screens

std::string Upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

int FontH(Face f) { return Text::LineHeight(f); }
int FontW(const std::string& s, Face f) { return Text::StringWidth(s, f); }

// Draws `s` horizontally centred at y.
void DrawCentered(Backbuffer& bb, const std::string& s, int y, uint32_t color, Face f) {
    Text::DrawString(bb, kCenterX - (FontW(s, f) >> 1), y, s, color, f);
}

}  // namespace

GameApp::GameApp(const AssetRoot& assets, ImageCache& images)
    : assets_(assets), images_(images), world_(assets, images) {
    playerSprites_ = ParseCml(assets.Read("/oh_pc.cml"), images);
    world_.onStateChange = [this](int o, int n) { OnStateChange(o, n); };
    world_.onLoadLevel = [this]() {
        textScrollY_ = kH - (FontH(Face::SmallBold) << 3);
        textScrollTimer_ = 0;
    };
    world_.onOpenMenu = [this](int id) {
        if (id == 4) {  // OPEN_SHOP_MENU: menuId first, so no title image
            menuId_ = 4;
            world_.SetState(3);
        } else {  // OPEN_MENU: build, switch state, then select menu 0
            BuildMenus();
            world_.SetState(3);
            menuId_ = 0;
        }
    };
    textScrollY_ = kH - (FontH(Face::SmallBold) << 3);
}

void GameApp::Start() { world_.Boot(); }

void GameApp::StartLevel(const std::string& scrPath) {
    world_.Boot();
    // Fast-forward the splash screens the way a player mashing keys would.
    for (int ms = 0; world_.state() != 3 && ms < 60000; ms += 16) {
        if (ms % 1100 < 16) world_.KeyPressed(7);
        Tick(16);
    }
    world_.LoadLevel(scrPath);
}

// ---- state changes -------------------------------------------------------

std::string GameApp::StartLine(int index) const {
    // Strings.loadStartupLine: the index-th '|'-separated entry of /start.txt.
    const std::vector<uint8_t> data = assets_.Read("/start.txt");
    int entry = 0;
    std::string out;
    for (uint8_t b : data) {
        if (b == '|') {
            if (entry == index) break;
            entry++;
        } else if (entry == index) {
            out += static_cast<char>(b);
        }
    }
    return out;
}

void GameApp::SetTextScreen(const std::string& textIn) {
    std::string text = textIn;
    const size_t v = text.find("VERSION");
    if (v != std::string::npos) text = text.substr(0, v) + "2.424" + text.substr(v + 7);
    // Lines are separated by a literal backslash-n pair in the string tables.
    std::vector<std::string> raw;
    size_t start = 0, at;
    while ((at = text.find("\\n", start)) != std::string::npos) {
        raw.push_back(text.substr(start, at - start));
        start = at + 2;
    }
    raw.push_back(text.substr(start));
    world_.SetSpeaker(nullptr);
    textLines_.clear();
    for (const std::string& line : raw) textLines_.push_back(world_.WrapText(line, kW - 10));
    textEndWaitMs_ = -1;
}

void GameApp::OnStateChange(int oldState, int newState) {
    if (oldState == 0) stateFlagF_ = true;
    if (newState == 3 && !titleImage_) {
        if (!stateFlagF_ && menuId_ != 4) titleImage_ = images_.Get("/main.png");
    } else if (oldState != 22) {
        titleImage_.reset();
    }
    const int fh = FontH(Face::SmallBold);
    switch (newState) {
        case 9:
            stateFlagF_ = false;
            SetTextScreen(S(547));
            break;
        case 4:
            SetTextScreen(S(548));
            textScrollY_ = kH - (fh << 2);
            textScrollTimer_ = 0;
            break;
        case 21: {
            const std::vector<uint8_t> d = assets_.Read("/copywrite.txt");
            SetTextScreen(std::string(d.begin(), d.end()));
            textScrollY_ = 0;
            break;
        }
        case 23:
            SetTextScreen(S(574));
            textScrollY_ = 15;
            textAtEnd_ = false;
            break;
        case 17:
            SetTextScreen(S(465));
            textScrollY_ = 15;
            textAtEnd_ = false;
            break;
        case 10:
            SetTextScreen(world_.textScreenText);
            break;
        default:
            break;
    }
}

// ---- menus -----------------------------------------------------------------

void GameApp::BuildMenus() {
    menus_.assign(7, {});
    menus_[2] = {"Level 1", "Level 2", "Level 3", "Level 4", "Level 5", "Level 6",
                 "Level 7", "Level 8", "Level 9", "Level 10", "Level 11", "Level 12"};
    menus_[3] = {"/l01_1.scr",   "/l02_2_1.scr",  "/l03_3.scr",    "/l04_4.scr",
                 "/l05_5.scr",   "/l06_6_cr.scr", "/l07_7_cr.scr", "/l08_8_cr.scr",
                 "/l09_9_cr.scr", "/l10_10_cr.scr", "/l11_11_cr.scr", "/l12_12.scr"};
    menus_[4] = {S(18), S(19), S(20)};
    menus_[6] = {S(457), S(458), S(573), S(522), S(459), S(460), S(461), S(462)};
    // No save file support yet (M8): hasSavedGame() is always false.
    menus_[0] = {S(2), S(456), S(6), S(22)};
    menus_[5] = {S(21), S(2), S(456), S(6), S(22)};
    const ScrTables& t = world_.script().tables();
    for (int i = 0; i < 9; i++)
        if (t.classBase[i][0] > 0) menus_[1].push_back(world_.script().ItemName(t.classBase[i][1]));
}

void GameApp::ResetMenu() {
    for (int& s : menuSelection_) s = 0;
    menuId_ = stateFlagF_ ? 5 : 0;
}

void GameApp::StartNewGame() {
    loadProgress_ = 0;
    world_.SetState(6);
    world_.ForgetPlayer();
    world_.SetPlayerClass(menuSelection_[1] + 1);
    world_.LoadLevel("/l01_1.scr");
    world_.gold = 100;
}

void GameApp::ActivateMenuItem() {
    if (menuId_ < 0 || menuId_ >= static_cast<int>(menus_.size()) || menus_[menuId_].empty()) return;
    const std::string item = menus_[menuId_][menuSelection_[menuId_]];
    if (menuId_ == 2) {
        loadProgress_ = 0;
        world_.SetState(6);
        world_.ForgetPlayer();
        world_.LoadLevel(menus_[3][menuSelection_[2]]);
    } else if (item == S(19)) {  // Save Game (M8)
        world_.Unimplemented("save game (M8)");
        menuSelection_[menuId_] = 2;
        world_.SetState(13);
    } else if (item == S(3)) {  // Load Game (M8)
        world_.SetState(14);
    } else if (item == S(21)) {  // Continue
        world_.SetState(0);
    } else if (item == S(2)) {  // New Game
        world_.SetState(16);
        if (world_.state() == 16) {
            // No saved game exists, so the overwrite prompt is skipped.
            world_.SetState(3);
            menuId_ = 1;
        }
    } else if (item == S(6)) {  // About
        world_.SetState(4);
    } else if (item == S(456)) {  // Help
        menuReturn_ = menuId_;
        menuId_ = 6;
    } else if (item == S(457)) {
        helpTitleId_ = 457;
        world_.SetState(17);
    } else if (item == S(458) || item == S(573) || item == S(522) || item == S(459) || item == S(460) ||
               item == S(461) || item == S(462)) {
        world_.Unimplemented("help pages / controls screen (M6b)");
    } else if (item == S(18)) {  // Go Shopping
        world_.Unimplemented("shop screen (M6c)");
    } else if (item == S(20)) {  // Continue playing (shop menu)
        world_.SetState(0);
    } else if (menuId_ == 1) {  // class chosen
        if (newGameLocked_) menuId_++;
        else StartNewGame();
    } else if (item == S(22)) {  // Exit
        world_.SetState(19);
    }
}

// ---- input ---------------------------------------------------------------

void GameApp::OnKeyDown(Key key) {
    if (key == Key::None) return;
    HandleKey(key);
    // script.keyPressed / handleDialogueKey run for any key in any state.
    int action = 0;
    switch (key) {
        case Key::Up: action = 3; break;
        case Key::Down: action = 4; break;
        case Key::Left: action = 5; break;
        case Key::Right: action = 6; break;
        case Key::Fire: action = 7; break;
        default: action = -1; break;
    }
    world_.KeyPressed(action);
}

void GameApp::HandleKey(Key key) {
    const int state = world_.state();
    switch (state) {
        case 0:
            if (key == Key::SoftRight) {
                if (world_.player() && world_.hudVisible()) world_.Unimplemented("inventory screen (M6c)");
            } else if (key == Key::SoftLeft) {
                BuildMenus();
                menuId_ = 5;
                for (int& s : menuSelection_) s = 0;
                world_.SetState(3);
            }
            break;
        case 3:
            HandleMenuKey(key);
            break;
        case 4:
        case 17:
        case 23:
            if (key == Key::SoftLeft) {
                if (state == 23 || state == 17) menuId_ = 6;
                else menuId_ = stateFlagF_ ? 5 : 0;
                world_.SetState(3);
            }
            break;
        case 11:
            if (key == Key::SoftRight) {
                world_.SetState(0);
            } else if (key == Key::SoftLeft) {
                ResetMenu();
                world_.SetState(3);
            }
            break;
        case 13:
            world_.SetState(3);
            break;
        case 16:
            if (key == Key::SoftRight) {
                menuId_ = 1;
                world_.SetState(3);
            } else if (key == Key::SoftLeft) {
                world_.SetState(3);
            }
            break;
        case 19:
            if (key == Key::SoftRight) world_.SetState(12);
            else if (key == Key::SoftLeft) world_.SetState(3);
            break;
        default:
            break;
    }
}

void GameApp::HandleMenuKey(Key key) {
    if (menuId_ < 0 || menuId_ >= static_cast<int>(menus_.size()) || menus_[menuId_].empty()) return;
    auto& sel = menuSelection_[menuId_];
    const int n = static_cast<int>(menus_[menuId_].size());
    if (key == Key::Left) {
        if (--sel == -1) sel = n - 1;
    } else if (key == Key::Right) {
        if (++sel == n) sel = 0;
    } else if (key == Key::SoftLeft) {
        if (menuId_ == 6) menuId_ = menuReturn_;
        else if (menuId_ == 1) menuId_ = stateFlagF_ ? 5 : 0;
    } else if (key == Key::Fire) {
        ActivateMenuItem();
    }
}

// ---- per frame -------------------------------------------------------------

void GameApp::Tick(int dt) {
    const int state = world_.state();
    if (state == 12) return;

    // Held keys act every frame in the original (keyState persists).
    int action = 0;
    switch (held_) {
        case Key::Up: action = 3; break;
        case Key::Down: action = 4; break;
        case Key::Left: action = 5; break;
        case Key::Right: action = 6; break;
        case Key::Fire: action = 7; break;
        default: break;
    }
    if (state == 0) world_.HeldAction(action, dt);
    if (action) world_.KeyPressed(action);

    world_.Tick(dt);

    if (blinkTimer_ >= 0) {
        blinkTimer_ -= dt;
        if (blinkTimer_ <= 0) {
            blinkTimer_ = 500;
            blinkOn_ = !blinkOn_;
        }
    }

    const int st = world_.state();
    if (st == 9 || st == 10 || st == 4 || st == 21) {
        if (textScrollTimer_ > 100) {
            textScrollY_--;
            textScrollTimer_ = 0;
        }
        textScrollTimer_ += dt;
    }
    // Text-screen scrolling with the up/down keys (Game.handleInput cases 4/9/10/17/23).
    if (st == 4 || st == 9 || st == 10 || st == 17 || st == 23) {
        const int step = dt / 10;
        if (action == 3) {
            textScrollTimer_ = 0;
            textScrollY_ = std::min(kH - (FontH(Face::SmallBold) << 2), textScrollY_ + step);
        } else if (action == 4 && !((st == 23 || st == 17) && textAtEnd_)) {
            textScrollTimer_ = 0;
            textScrollY_ -= step;
        }
    }
    if (textEndWaitMs_ >= 0) {
        textEndWaitMs_ += dt;
        if (textEndWaitMs_ >= 3000) {
            textEndWaitMs_ = -1;
            const int fh = FontH(Face::SmallBold);
            if (st == 9) {
                textScrollY_ = kH - (fh << 3);
                textScrollTimer_ = 0;
                for (int& s : menuSelection_) s = 0;
                menuId_ = stateFlagF_ ? 5 : 0;
                world_.SetState(4);
            } else if (st == 10) {
                world_.SetState(0);
            } else if (st == 4) {
                world_.SetState(3);
                menuId_ = stateFlagF_ ? 5 : 0;
            }
        }
    }
    if (st == 15) {
        spinnerTimer_ += dt;
        if (spinnerTimer_ >= 200) {
            playerSprites_.AdvanceFrame(5);
            spinnerTimer_ = 0;
        }
    }
}

// ---- painting --------------------------------------------------------------

void GameApp::Draw(Backbuffer& bb) {
    const int fhs = FontH(Face::SmallBold);
    const int fhl = FontH(Face::LargeBold);
    const SpriteSet& hud = world_.hudSprites();
    switch (world_.state()) {
        case 0:
            world_.DrawField(bb);
            DrawHud(bb);
            break;
        case 3:
            DrawMenu(bb);
            break;
        case 4:
        case 9:
        case 10:
        case 17:
        case 21:
        case 23:
            DrawTextScreen(bb);
            break;
        case 6:
        case 7: {
            bb.Fill(0);
            const int w = kW - 20;
            const int filled = loadProgress_ * w / 100;
            if (filled > 0) bb.FillRect(10, 30, filled, 10, 0xFF0000);
            bb.DrawRect(10, 30, w, 10, kWhite);
            std::string word = S(39);
            if (word.empty()) word = StartLine(0);
            Text::DrawString(bb, 10, 10, word + "...", kWhite, Face::SmallBold);
            break;
        }
        case 8: {
            bb.Fill(static_cast<uint32_t>(world_.cutsceneColor));
            const int g = world_.cutsceneSprite;
            DrawSprite(bb, images_, hud, g, kCenterX - (hud.Width(g) >> 1), kCenterY - (hud.Height(g) >> 1));
            if (world_.script().waitingForKey()) {
                if (blinkOn_)
                    DrawCentered(bb, StartLine(1), kCenterY + (hud.Height(g) >> 1) + 12, 0, Face::SmallBold);
                if (blinkTimer_ == -1) blinkTimer_ = 500;
            }
            break;
        }
        case 11:
            bb.Fill(0);
            DrawCentered(bb, S(428), kCenterY - (fhl >> 1), kWhite, Face::LargeBold);
            Text::DrawString(bb, 2, kH - fhl - 2, Upper(S(427)), kWhite, Face::LargeBold);
            Text::DrawString(bb, kW - FontW(S(426), Face::LargeBold) - 2, kH - fhl - 2, Upper(S(426)), kWhite,
                             Face::LargeBold);
            break;
        case 13:
            bb.Fill(0);
            DrawCentered(bb, S(450), kCenterY - (fhl >> 1), kWhite, Face::LargeBold);
            DrawCentered(bb, S(401), kCenterY - (fhl >> 1) + (fhl << 1), kWhite, Face::SmallBold);
            break;
        case 14:
            DrawPrompt(bb, S(451), true);
            break;
        case 15: {
            bb.Fill(0);
            DrawCentered(bb, "Please Wait...", kCenterY - (fhl >> 1), kWhite, Face::LargeBold);
            DrawSprite(bb, images_, playerSprites_, 5, kCenterX - (playerSprites_.Width(5) >> 1), kCenterY + fhl);
            break;
        }
        case 16:
            bb.Fill(0);
            DrawCentered(bb, S(455), kCenterY - (fhl >> 1), kWhite, Face::LargeBold);
            DrawCentered(bb, S(464), kCenterY - (fhl >> 1) + fhl, kWhite, Face::LargeBold);
            Text::DrawString(bb, 2, kH - fhl - 2, Upper(S(427)), kWhite, Face::LargeBold);
            Text::DrawString(bb, kW - FontW(S(426), Face::LargeBold) - 2, kH - fhl - 2, Upper(S(426)), kWhite,
                             Face::LargeBold);
            break;
        case 19:
            bb.Fill(0);
            // The original centres on the width of string 451 but draws string 475.
            Text::DrawString(bb, kCenterX - (FontW(S(451), Face::LargeBold) >> 1), kCenterY - (fhl >> 1), S(475),
                             kWhite, Face::LargeBold);
            Text::DrawString(bb, 2, kH - fhl - 2, Upper(S(427)), kWhite, Face::LargeBold);
            Text::DrawString(bb, kW - FontW(S(426), Face::LargeBold) - 2, kH - fhl - 2, Upper(S(426)), kWhite,
                             Face::LargeBold);
            break;
        case 20:
            bb.Fill(0);
            DrawCentered(bb, S(566), kCenterY - (fhs >> 1), kWhite, Face::SmallBold);
            Text::DrawString(bb, 2, kH - fhs - 2, Upper(S(567)), kWhite, Face::SmallBold);
            break;
        case 22: {
            bb.Fill(0);
            if (S(571).empty()) {
                DrawCentered(bb, StartLine(2), kCenterY - (fhl >> 1), kWhite, Face::LargeBold);
                Text::DrawString(bb, 2, kH - fhl - 2, Upper(StartLine(4)), kWhite, Face::LargeBold);
                Text::DrawString(bb, kW - FontW(StartLine(3), Face::LargeBold) - 2, kH - fhl - 2,
                                 Upper(StartLine(3)), kWhite, Face::LargeBold);
            } else {
                DrawCentered(bb, S(571), kCenterY - (fhl >> 1), kWhite, Face::LargeBold);
                Text::DrawString(bb, 2, kH - fhl - 2, Upper(S(22)), kWhite, Face::LargeBold);
                Text::DrawString(bb, kW - FontW(S(426), Face::LargeBold) - 2, kH - fhl - 2, Upper(S(426)), kWhite,
                                 Face::LargeBold);
            }
            break;
        }
        default:
            bb.Fill(0);
            break;
    }
    // Script dialogue box over any state that draws it (Game.paint case 0 only,
    // but the box is only ever open while playing).
    if (world_.state() == 0 && world_.dialogue.open) DrawDialogue(bb);
}

void GameApp::DrawPrompt(Backbuffer& bb, const std::string& text, bool twoSoftKeys, bool large) {
    const Face f = large ? Face::LargeBold : Face::SmallBold;
    bb.Fill(0);
    DrawCentered(bb, text, kCenterY - (FontH(f) >> 1), kWhite, f);
    if (twoSoftKeys) {
        Text::DrawString(bb, 2, kH - FontH(f) - 2, Upper(S(427)), kWhite, f);
        Text::DrawString(bb, kW - FontW(S(426), f) - 2, kH - FontH(f) - 2, Upper(S(426)), kWhite, f);
    }
}

void GameApp::DrawHud(Backbuffer& bb) {
    const SpriteSet& hud = world_.hudSprites();
    const int fhs = FontH(Face::SmallBold);
    Actor* p = world_.player();
    if (world_.hudVisible() && p) {
        if (p->dead == 0) {
            const int hp = std::min(70, 70 * p->hp / std::max(1, p->maxHp));
            const int mp = std::min(70, 70 * p->mp / std::max(1, p->maxMp));
            bb.FillRect(18, 10, hp, 7, 0xFF0000);
            bb.FillRect(18, 18, mp, 7, 0x0000FF);
        }
        DrawSprite(bb, images_, world_.tileSprites(), -56, 0, 0);  // the frame around the bars
        if (p->attackIcon != -1)
            DrawSprite(bb, images_, hud, p->attackIcon, kW - hud.Width(p->attackIcon) - 2, 2);
        if (p->effectIcon != -1)
            DrawSprite(bb, images_, hud, p->effectIcon, kW - (hud.Width(p->attackIcon) << 1) - 4, 2);
    }
    Text::DrawString(bb, 2, kH - fhs - 2, Upper(S(422)), kWhite, Face::SmallBold);
    if (world_.hudVisible())
        Text::DrawString(bb, kW - FontW(S(421), Face::SmallBold) - 2, kH - fhs - 2, Upper(S(421)), kWhite,
                         Face::SmallBold);
    DrawMessage(bb);
}

void GameApp::DrawMessage(Backbuffer& bb) {
    World::Message& m = world_.message;
    if (m.text.empty()) return;
    const Face f = Face::MediumPlain;
    m.y = kH - FontH(f) - 5;
    bb.FillRect(0, m.y - 5, kW, kH - (m.y - 5), 0);
    if (m.blank) return;
    if (m.x == -1) {
        if (m.style == 0 || m.style == 1) m.x = kCenterX - (FontW(m.text, f) >> 1);
        else if (m.style == 2) m.x = -FontW(m.text, f);
        else if (m.style == 3) m.x = kW;
    }
    Text::DrawString(bb, m.x, m.y, m.text, static_cast<uint32_t>(m.color), f);
}

// Game.drawDialogue.
void GameApp::DrawDialogue(Backbuffer& bb) {
    const SpriteSet& hud = world_.hudSprites();
    World::Dialogue& d = world_.dialogue;
    const Face f = Face::SmallBold;
    const int fh = FontH(f);
    const int wCap = hud.Width(51), wMid = std::max(1, hud.Width(52));
    DrawSprite(bb, images_, hud, 51, 5, 5);
    int i = 0;
    for (; i <= (d.right - 5 - wCap - hud.Width(50)) / wMid; i++) DrawSprite(bb, images_, hud, 52, 5 + wCap + i * wMid, 5);
    DrawSprite(bb, images_, hud, 50, 5 + wCap + i * wMid, 5);
    const int boxCols = i;

    int y = d.top;
    bool showedFirst = false, fits = true;
    size_t n = 0;
    for (; n < d.lines.size() && fits; n++) {
        const std::string& line = d.lines[n];
        if (y - d.scroll >= d.top && y - d.scroll <= d.top + d.height - fh) {
            showedFirst |= n == 0;
            const std::string* who = world_.speaker();
            if (who && n == 0 && line.compare(0, who->size(), *who) == 0 && line.size() >= who->size() + 2) {
                // "Name: text": the name in its own colour.
                const std::string head = *who + ": ";
                Text::DrawString(bb, d.left, y - d.scroll, head, 6684672, f);
                Text::DrawString(bb, d.left + FontW(head, f), y - d.scroll, line.substr(head.size()), kWhite, f);
            } else {
                Text::DrawString(bb, d.left, y - d.scroll, line, kWhite, f);
            }
        }
        y += fh + 1;
        fits = y - d.scroll < d.top + d.height - fh - 1;
    }
    d.atEnd = n == d.lines.size() && fits;
    if (!showedFirst)
        DrawSprite(bb, images_, hud, 54, 5 + wCap + boxCols * wMid - hud.Width(54) + 3, 8);
    if (!d.atEnd)
        DrawSprite(bb, images_, hud, 53, 5 + wCap + boxCols * wMid - hud.Width(53) + 3,
                   5 + hud.Height(51) - hud.Height(53) - 3);
}

void GameApp::DrawMenu(Backbuffer& bb) {
    bb.Fill(0);
    if (menuId_ < 0 || menuId_ >= static_cast<int>(menus_.size()) || menus_[menuId_].empty()) return;
    const Face f = Face::LargeBold;
    const int fhl = FontH(f);
    int titleH = 0;
    if (titleImage_) {
        titleH = titleImage_->height;
        bb.Blit(*titleImage_, 0, 0, titleImage_->width, titleImage_->height, kCenterX - (titleImage_->width >> 1), 0);
        if (menuId_ == 1) DrawCentered(bb, S(423), titleH + 1, 1044480, f);
    }
    const int y = titleH + 25;
    Text::DrawString(bb, 0, y, "<<", 0xFF0000, f);
    Text::DrawString(bb, kW - FontW(">>", f), y, ">>", 0xFF0000, f);
    const std::string item = menus_[menuId_][menuSelection_[menuId_]];
    const int room = kW - FontW("<<  >>", f);
    const size_t sp = item.find(' ');
    if (FontW(item, f) >= room && sp != std::string::npos) {
        DrawCentered(bb, item.substr(0, sp), y - (fhl >> 1), kWhite, f);
        DrawCentered(bb, item.substr(sp + 1), y + (fhl >> 1), kWhite, f);
    } else {
        DrawCentered(bb, item, y, kWhite, f);
    }
    if (menuId_ != 0 && menuId_ != 5 && menuId_ != 4)
        Text::DrawString(bb, 2, kH - FontH(Face::SmallBold) - 2, Upper(S(449)), kWhite, Face::SmallBold);
}

// Game.paint states 4/9/10/17/21/23: scrolling text.
void GameApp::DrawTextScreen(Backbuffer& bb) {
    const int state = world_.state();
    const bool dark = state == 4 || state == 21 || state == 23 || state == 17;
    const SpriteSet& hud = world_.hudSprites();
    const Face body = state == 21 ? Face::SmallPlain : Face::SmallBold;
    const int fh = FontH(Face::SmallBold);
    bb.Fill(dark ? 0 : kPaper);
    if ((state == 23 || state == 17) && textScrollY_ > 20) textScrollY_ = 20;
    bool firstLine = state == 21;  // state 21 does not advance before its first line
    if (state == 21) textScrollY_--;
    int y = 3 + textScrollY_;
    for (const auto& para : textLines_) {
        for (const std::string& lineIn : para) {
            std::string line = lineIn;
            uint32_t color = dark ? kWhite : 0;
            int x = 2;
            Face face = body;
            if (!firstLine) y += fh + 1;
            else firstLine = false;
            if (line.size() > 1 && line[1] == '~') {
                if (line[0] == '1') {
                    face = Face::SmallBold;
                    color = 11184640;
                } else if (line[0] == '3') {
                    color = 11141120;
                    x = kCenterX - (FontW(line.substr(line.find('~') + 1), Face::SmallBold) >> 1);
                }
                line = line.substr(line.find('~') + 1);
            }
            Text::DrawString(bb, x, y, line, color, face);
        }
        if (para.empty()) y += fh + 1;
    }
    if (state == 23 || state == 17) {
        bb.FillRect(0, 0, kW, fh + 20, 0);
        bb.FillRect(5, 5, kW - 10, fh + 10, kWhite);
        DrawCentered(bb, S(helpTitleId_), 10, kTitleBlue, Face::SmallBold);
    }
    int limit = kH - fh;
    if (state == 10 || state == 23 || state == 4 || state == 17) limit -= fh * 3;
    const int arrowUpY = kH - fh - 7, arrowDownY = kH - fh - 5 + hud.Height(54);
    if (state == 4 || state == 23 || state == 17) {
        bb.FillRect(0, kH - fh - 8, kW, fh + 8, 0);
        Text::DrawString(bb, 2, kH - fh - 2, Upper(S(449)), kWhite, Face::SmallBold);
        const int top = state == 4 ? kH - (fh << 2) : 10;
        if (textScrollY_ < top) DrawSprite(bb, images_, hud, 54, kCenterX - (hud.Width(54) >> 1), arrowUpY);
        if (y > limit) DrawSprite(bb, images_, hud, 53, kCenterX - (hud.Width(53) >> 1), arrowDownY);
    }
    if (state == 10) {
        bb.FillRect(0, kH - fh - 8, kW, fh + 8, kPaper);
        if (textScrollY_ < kH - (fh << 2)) DrawSprite(bb, images_, hud, 54, kCenterX - (hud.Width(54) >> 1), arrowUpY);
        if (y > limit) DrawSprite(bb, images_, hud, 53, kCenterX - (hud.Width(53) >> 1), arrowDownY);
    }
    if (y < limit) {
        if (state == 23 || state == 17) textAtEnd_ = true;
        else if ((state == 9 || state == 10 || state == 4) && textEndWaitMs_ < 0) textEndWaitMs_ = 0;
    } else if (state == 23 || state == 17) {
        textAtEnd_ = false;
    }
}

}  // namespace oblivion
