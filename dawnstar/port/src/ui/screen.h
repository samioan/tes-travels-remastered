#pragma once
#include <string>
#include <vector>

#include "graphics/backbuffer.h"

namespace dawnstar {

// Renamed-source counterpart of Screen.java (`../src/Screen.java`, itself
// renamed from decompiled/g.java): the one class that implements every
// non-3D-view UI screen in the original -- main menu, the options menu,
// NPC dialogue prompt lists, inventory/shop lists, "GenericInfoUI"-style
// message popups, and more, all through one `mode`-tagged class rather
// than a separate class per screen type.
//
// This milestone (M37) ports Screen's own self-contained data model,
// word-wrap/list-layout construction, pixel rendering, and up/down list
// navigation -- everything Screen.java implements *itself*, with no
// dependency on anything outside this class. Deliberately NOT ported
// here (this is a "logic first, wiring later" split, the same shape as
// M9/M10's corridor-selection-vs-drawing split and M25/M26's
// visibleObjects-data-model-vs-sprite-drawing split): the actual
// `ESGame`/`GameCanvas` navigation graph that CONSTRUCTS real Screen
// instances for the main menu/options menu/NPC dialogue/shops and
// dispatches their Select/Cancel/Back commands into real game actions
// (`ESGame.commandAction()`, by far the largest switch in the whole
// decompiled source) -- that's `docs/PORT_ROADMAP.md`'s own still-open
// "ESGame's full screen-wiring loop" milestone, which this class exists
// to eventually be driven by. Until that lands, nothing in `main.cpp`
// constructs or paints a real `Screen` yet, the same "the logic is real
// and tested, but nothing calls it live yet" shape M15's
// `CombatResolution::MonsterTick` sat in from M15 through M35, or M22's
// `DungeonRuntime` sat in until M23/M24.
//
// SIMPLIFIED, fields dropped entirely rather than ported: `game`/
// `canvas` (this port has no `ESGame`/`GameCanvas` object for a Screen
// to hold a pointer to -- `width()`/`height()` below just return this
// port's own fixed 176x208 constants directly, the same "port decision,
// not a queried runtime value" status every other module's 176x208 use
// already has); `listener` (no `CommandListener`/`ESGame.commandAction()`
// exists yet to route a Select/Cancel/Back press into); `backTarget`/
// `returnDisplay`/`contextIndex`/`secondaryParam`/`rawTaggedText` (all
// four are ESGame's OWN navigation bookkeeping that Screen itself never
// reads or writes back -- see Screen.java's own field doc comments --
// so they have nothing to attach to yet either). `onExit()`/`onEnter()`
// are empty no-ops in Screen.java itself (only ever overridden by the
// unported `LoadingScreen`), so there's nothing to port there at all.
//
// Text rendering reuses M30's `BitmapFont` (the same single invented
// monospace font every other text-drawing module in this port already
// uses in place of Screen.java's own 4 distinct, unrecoverable MIDP
// `Font` objects -- `TITLE_FONT`/`DEFAULT_TEXT_FONT`/`LARGE_TEXT_FONT`/
// `SOFT_KEY_FONT`, all `Font.getFont(...)` system fonts with no real
// glyph shapes or metrics to recover, same status as `SMALL_FONT`
// already was for M30) and M35's already-established `render/
// message_popup.h`'s `MessagePopup::WordWrap` for `wrapText()` (a
// straight reuse, not a second word-wrap implementation -- both need
// exactly the same "wrap this text to fit some pixel width, using
// BitmapFont::kAdvance per character" logic `MessagePopup::WordWrap`
// already implements and this milestone's own test independently
// verifies still matches Screen.java's own `wrapText()`/`wordWrap()`
// call chain).
enum class ScreenMode {
    // Screen.java's own class doc comment: "modes 3/4 are both
    // scrollable text" is imprecise -- mode 3 is a scrollable,
    // SELECTABLE list (paint()'s own renderHighlightedText, the same
    // highlighted-row renderer prompt lists use), confirmed by real call
    // sites like ESGame's mainMenuUI/OptionsUI (both `setupList`, both
    // mode 3, both later read back via a real selection).
    HighlightedList = 3,
    // Mode 4 is a scrollable, NON-selectable list/message body
    // (renderPlainList, no highlight box ever drawn) -- ESGame's own
    // GenericInfoUI-style popups (`setupMessage`) are always this mode.
    PlainList = 4,
    // Prompt text above a selectable list (renderPromptList variant 1).
    PromptList = 5,
    // Same as PromptList, plus a second word-wrapped footer text block
    // between the prompt and the list (renderPromptList variant 2).
    PromptListWithFooter = 6,
};

// Screen.java's 5 static Command singletons (`ESGame.okCommand`/
// `selectCommand`/`cancelCommand`/`backCommand`/`exitCommand`), each a
// real distinct MIDP `Command` object real code compares against by
// REFERENCE IDENTITY (`c == ESGame.okCommand`) -- every real command a
// Screen ever attaches in `../src/ESGame.java` is one of these 5 fixed
// singletons, never a fresh instance, so an enum reproduces that
// identity comparison exactly. `Exit` is never actually attached to any
// real Screen (`ESGame.java`'s own only two uses of it are direct
// equality checks inside its own unported command-listener switch), kept
// here anyway for a complete, literal mirror of the 5 singletons rather
// than silently dropping one.
enum class CommandId { Ok, Select, Cancel, Back, Exit };

// Screen.java's own `mode`-dispatched data model, layout construction,
// pixel rendering (`paint()`), and up/down list navigation (the
// `handleKey()` game-action-1/6 branches) -- see this header's own class
// comment above for what's deliberately not included yet.
class Screen {
public:
    explicit Screen(ScreenMode mode);

    static constexpr int kWidth = Backbuffer::kWidth;
    static constexpr int kHeight = Backbuffer::kHeight;

    // Screen.java's own accessors of the same name (`width()`/
    // `height()`) -- return this port's fixed constants directly rather
    // than a queried `canvas.screenWidth`/`screenHeight` (see this
    // header's own class comment on why `canvas` itself isn't ported).
    static constexpr int width() { return kWidth; }
    static constexpr int height() { return kHeight; }

    ScreenMode mode() const { return mode_; }

    // Mode-3/4-style plain scrollable list: up to 10 items visible at
    // once. `cancelable` adds a Cancel command alongside the always-
    // present Select command (real call sites only ever pass `false` --
    // `ESGame.OptionsUI` instead adds its own Back command manually,
    // after construction, via AddCommand below -- so both paths are
    // exercised, just through two different real call shapes).
    void SetupList(const std::string& title, std::vector<std::string> items, bool cancelable);

    // Word-wrapped scrollable text (up to 11 lines visible). Real call
    // sites always use this with mode PlainList (`ESGame.GenericInfoUI`),
    // per Screen.java's own class comment -- the constructor already
    // added an Ok command for that mode, see below.
    void SetupMessage(const std::string& title, const std::string& body);

    // Prompt text (word-wrapped) above a selectable list, with per-item
    // word-wrap that groups a long item's own wrapped continuation lines
    // under one logical item for up/down navigation (`itemGroupStart`).
    // Always adds Select + Cancel commands, matching Screen.java's own
    // `setupPromptList` exactly.
    void SetupPromptList(const std::string& title, const std::string& prompt, std::vector<std::string> items);

    // Same as the 3-arg overload, plus a second word-wrapped footer text
    // block (mode PromptListWithFooter only).
    void SetupPromptList(const std::string& title, const std::string& prompt, const std::string& footer,
                         std::vector<std::string> items);

    // Screen.java's own `paint(Graphics)`, dispatched by `mode_` exactly
    // like the original's own switch, plus the soft-key bar every mode
    // shares.
    void Paint(Backbuffer& bb) const;

    // The `handleKey()` game-action-1 (UP) / game-action-6 (DOWN)
    // branches -- the real up/down list-navigation logic (selection
    // move, or plain scroll for a non-selectable mode-4 list, with
    // itemGroupStart-aware jumps for a word-wrapped prompt-list item).
    // SIMPLIFIED: the original's own `requestRepaint()` tail call (a
    // render-loop scheduling side effect, not part of Screen's own
    // navigation STATE) is not reproduced -- nothing in this port drives
    // a live Screen through a render loop yet for it to matter to, and
    // the state mutation these methods exist to test is unaffected by
    // whether a repaint gets scheduled.
    void MoveSelectionUp();
    void MoveSelectionDown();

    // `addCommand`/`removeCommand` (Vector::addElement/removeElement,
    // ported as ordinary vector insert/first-match-erase -- every real
    // call site attaches at most 2 commands, confirmed by grepping every
    // Screen construction site in `../src/ESGame.java`).
    void AddCommand(CommandId c);
    void RemoveCommand(CommandId c);

    // `leftSoftKeyCommand()`/`rightSoftKeyCommand()`, exposed publicly
    // (Screen.java keeps them private, only `handleKey()`'s own
    // key==-6/-7 branch and `renderSoftKeyBar()` call them) since this
    // port has no `handleKey()` soft-key dispatch yet (that needs a real
    // `CommandListener`) -- a future wiring milestone reads these
    // directly instead. Returns false (no command) exactly where the
    // original returns `null`: 0 commands: neither has one. 1 command:
    // it's ALWAYS the right key, regardless of which of the 5 commands
    // it is. 2 commands: right is whichever one IS Ok/Select (if
    // neither is, right has none); left is whichever one IS Back/Cancel
    // (if neither is, left has none) -- so e.g. a 2-command Screen whose
    // commands are Back+Exit has a left key (Back) but no right key at
    // all, exactly matching the original's own real (if seemingly
    // incomplete) logic. More than 2 commands: neither key resolves
    // (unreachable in practice, matching the original's own if/else-if
    // that only ever checks count==1/count==2).
    bool LeftSoftKeyCommand(CommandId* out) const;
    bool RightSoftKeyCommand(CommandId* out) const;

    // `selectedIndexOrMinusOne()`/`setSelectedIndex()`/
    // `selectedItemText()`/`firstLine()`/`setTitle()`/`setItems()`/
    // `setTextColumn()` -- Screen's own small runtime-reconfiguration
    // accessors, ported directly even though nothing in this port calls
    // them live yet (same "port the whole self-contained class" standard
    // M13/M14/M25 already established, rather than only porting the
    // subset something happens to already call).
    int SelectedIndex() const { return selectedIndex_; }
    int SelectedIndexOrMinusOne() const;
    void SetSelectedIndex(int index);
    const std::string& SelectedItemText() const;
    // Returns "" for an as-yet-unpopulated Screen (promptLines_/items_
    // empty), matching a real call the original would only ever make
    // after some setup* call already populated them -- see this method's
    // own .cpp doc comment for why that guard is defensive, not a
    // behavior change.
    const std::string& FirstLine() const;
    void SetTitle(const std::string& title);
    // `setItems(text)`: mode PromptList/PromptListWithFooter word-wraps
    // into promptLines_; mode PlainList word-wraps into items_ (and
    // itemCount_/scrollTop_ reset) -- HighlightedList is UNREACHABLE
    // here (Screen.java's own `setItems` only ever branches on
    // mode==5/6 or mode==4, silently doing nothing for mode 3, ported
    // exactly).
    void SetItems(const std::string& text);
    // `setTextColumn(column, text)`: column 0 rewraps promptLines_ (modes
    // 5/6/4 only -- HighlightedList excluded, matching the source's own
    // condition exactly); column 1 rewraps footerLines_ (mode 6 only).
    void SetTextColumn(int column, const std::string& text);

    // Read-only accessors this milestone's own test (and a future
    // wiring milestone) need to inspect constructed state directly.
    const std::string& Title() const { return title_; }
    const std::vector<std::string>& Items() const { return items_; }
    int ScrollTop() const { return scrollTop_; }
    int ScrollBottom() const { return scrollBottom_; }
    int ItemCount() const { return itemCount_; }
    // Empty for a Screen built via SetupList/SetupMessage (Screen.java's
    // own `itemGroupStart == null`); one entry per LOGICAL item (not
    // visual row) for a Screen built via SetupPromptList -- see
    // SetupPromptList's own doc comment for how a long item's own group
    // entry can outgrow its original identity value once earlier items
    // split into multiple visual rows.
    const std::vector<int>& ItemGroupStart() const { return itemGroupStart_; }

private:
    void RenderTitleBar(Backbuffer& bb) const;
    void RenderHighlightedText(Backbuffer& bb) const;
    void RenderPromptList(Backbuffer& bb, int variant) const;
    void RenderPlainList(Backbuffer& bb) const;
    // Returns the cursorY just past the last drawn row, matching the
    // original's own `this.cursorY` being left mutated for
    // renderPromptList's own tail call into this same method.
    int RenderItemRows(Backbuffer& bb, int cursorY, int lineHeight) const;
    void RenderSoftKeyBar(Backbuffer& bb) const;

    std::vector<std::string> WrapText(const std::string& text) const;

    ScreenMode mode_;
    std::string title_;
    int marginX_ = 0;
    int marginRight_ = 0;
    int scrollTop_ = 0;
    int scrollBottom_ = 0;
    int itemCount_ = 0;
    std::vector<std::string> promptLines_;
    std::vector<std::string> footerLines_;
    std::vector<std::string> items_;
    // Empty means Screen.java's own `itemGroupStart == null` (set only
    // by SetupPromptList).
    std::vector<int> itemGroupStart_;
    int selectedIndex_ = 0;
    std::vector<CommandId> commands_;
};

}  // namespace dawnstar
