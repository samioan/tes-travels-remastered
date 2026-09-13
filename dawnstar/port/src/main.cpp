// M20: wires the real asset pipeline (M2-M8), the real 37-level
// procedurally-generated world (M6), a real created character (M11),
// player movement (M13), and the first-person corridor renderer
// (M9/M10) into the actual windowed app -- previously this just
// presented a solid-color placeholder backbuffer (see
// docs/PORT_ROADMAP.md's M20 entry). Arrow keys move/turn at the
// engine's fixed 250ms tick rate. M38 added the real main-menu/help/
// credits/quit-confirm flow shown before a game starts (`inMenu`); M39
// added the real in-game options menu (`inOptionsMenu`, the 'O' key --
// see ui/options_menu.h); M40 added the real class-selection/name-entry
// character-creation flow (`inCharacterCreation` -- see
// ui/character_creation_flow.h), so "New Game" now creates a real,
// player-chosen/named character instead of M20's own fixed "class 0,
// Traveler" stand-in. M41 made the options menu's own "Inventory"/
// "Skills"/"Spells" actions real (previously silent no-ops) -- "Use"
// alone needs a real CombatResolution::UseItem call this file performs
// itself, via the new OptionsMenuAction::UseInventoryItem round-trip
// (see ui/options_menu.h's own class comment).
#include <windows.h>

#include <array>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/help_text.h"
#include "assets/img_archive.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/monster_image_names.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "camp/camp_tick.h"
#include "combat/combat_resolution.h"
#include "combat/combat_tick.h"
#include "dungeon/dungeon_runtime.h"
#include "engine/game_clock.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "interact/interact_tick.h"
#include "platform/win32/window.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/player_state.h"
#include "player/visible_objects.h"
#include "render/frame_renderer.h"
#include "render/hotbar_renderer.h"
#include "render/hud_renderer.h"
#include "render/message_popup.h"
#include "render/minimap_renderer.h"
#include "render/visible_object_renderer.h"
#include "ui/character_creation_flow.h"
#include "ui/menu_flow.h"
#include "ui/options_menu.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"
#include "world/dungeon_view.h"

namespace {

// Shop.NAMES -- needed here (not just the SHOP_X/Y position tables
// player/visible_objects.cpp/world/dungeon_generator.cpp already
// inline) for M30's NPC-shop-greeting popup text. Still no real Shop
// class in this port (see player/player_state.h's own npcShopIndex doc
// comment) -- just the one array's worth of display strings this one
// new call site needs.
const char* kShopNames[9] = {
    "Weapon Peddler", "Heavy Armor Peddler", "Light Armor Peddler", "Jakar's",
    "Eustacia",       "Alhavara",            "Beatrice",            "Chung",
    "Delacroix",
};

// Mirrors every M13 test's own world-building loop: one GeneratedLevel
// per real geomin.dat row, hub town (level 1) hand-carved, every other
// level procedurally generated -- see world/dungeon_generator.h. M24
// additionally registers each level's pre-placed monster/chest spawns
// into the live `world` registry right after generating it -- the
// port's substitute for DungeonGenerator.java's populateLevel/
// placeChests directly calling Monster.store()/ESGame.chests[...].put()
// as part of generation itself (see dungeon/dungeon_runtime.h's
// RegisterGeneratedSpawns doc comment for why that can't happen inside
// DungeonGenerator itself here).
std::vector<dawnstar::GeneratedLevel> BuildWorld(const dawnstar::DungeonGeometry& geometry,
                                                  const dawnstar::ItemDatabase& items,
                                                  const dawnstar::MonsterDatabase& monsters,
                                                  dawnstar::WorldRegistry& world) {
    std::vector<dawnstar::GeneratedLevel> levels;
    levels.reserve(geometry.rows.size());
    for (size_t i = 0; i < geometry.rows.size(); i++) {
        int levelNumber = static_cast<int>(i) + 1;
        levels.push_back(levelNumber == 1
                              ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                              : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                           monsters));
        dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
    }
    return levels;
}

bool KeyPressed(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    dawnstar::Window window(dawnstar::Backbuffer::kWidth * 2, dawnstar::Backbuffer::kHeight * 2,
                             L"Dawnstar Port");
    dawnstar::Backbuffer backbuffer;
    dawnstar::MinimapSurface minimap;
    dawnstar::MessagePopupState messagePopup;
    dawnstar::GameClock clock;
    // GameCanvas.keyPressed()'s `key == 42` handler (minimap zoom
    // toggle) fires once per physical key-down transition, not once per
    // 250ms tick like movement -- edge-detected here (rather than
    // reusing KeyPressed's held-key polling as-is) so holding the key
    // doesn't rapid-toggle every tick. Remapped from the original's
    // numeric-keypad '*' to 'M' for a PC keyboard; not a behavior
    // simplification, just a different physical key for the same
    // one-shot toggle (same spirit as VK_UP/DOWN/LEFT/RIGHT already
    // standing in for whatever the original's own arrow/game-action keys
    // were).
    bool zoomKeyWasDown = false;
    // GameCanvas.spellCycleRequested -- a genuine discrete-keydown-event
    // flag (unlike attack/cast, it has no internal cooldown of its own
    // to throttle repeated firing), so it's captured the same
    // full-frame-rate edge-detected way as the zoom key above (matching
    // the original's own keyPressed(), which sets it immediately on a
    // physical keydown independent of the 250ms tick) but -- unlike
    // zoom's own immediate effect -- only actually CONSUMED once inside
    // the tick-gated dispatch below, matching dispatchTickActions()'s
    // real once-per-tick consumption.
    bool spellCycleKeyWasDown = false;
    bool spellCyclePending = false;
    // GameCanvas.interactRequested -- same discrete-keydown-event shape
    // as spellCycleRequested above (no internal cooldown of its own),
    // captured the same full-frame-rate edge-detected way, but ALSO
    // gated on `hotbarContext == 2` at the moment of the keydown itself
    // (GameCanvas.keyPressed()'s own `key == 57` handler: `if
    // (hotbarContext == 2) interactRequested = true;`) -- hence
    // `hotbarContext` itself is hoisted to a persistent local below
    // (rather than a fresh one recomputed inside the tick-gated block
    // every tick, as it was through M32/M33) so this full-frame-rate
    // check can read "hotbarContext as of the end of the last tick",
    // exactly like a real device's keyPressed() would.
    bool interactKeyWasDown = false;
    bool interactPending = false;
    // GameCanvas.campRequested -- same discrete-keydown-event shape as
    // interact above, gated on `hotbarContext == 0` (GameCanvas.
    // keyPressed()'s own `key == 48` handler) at the moment of the
    // keydown.
    bool campKeyWasDown = false;
    bool campPending = false;
    // GameCanvas.keyPressed()'s own `key == 55` handler (optionsRequested)
    // -- same discrete-keydown-event shape as interact/camp above (no
    // hotbarContext gate this time, matching the original's own handler
    // exactly), consumed at the same point in the tick's own priority
    // chain `dispatchTickActions()` checks it (right after attack,
    // before movement) -- see M39's own `inOptionsMenu` doc comment
    // below for why opening it needs to be a tick-gated event at all
    // (not just full-frame-rate like the 'M' zoom key) to preserve that
    // real priority ordering against camp/interact/cast/cycle/attack.
    bool optionsKeyWasDown = false;
    bool optionsPending = false;
    // GameCanvas.hotbarContext -- see interactKeyWasDown's own doc
    // comment above for why this is now a persistent local instead of a
    // tick-local one. Only ever reassigned inside a tick whose camp
    // state machine says `runTick` (see below) -- while actually
    // camping, paintHotbar() (the original's only writer of this field)
    // never runs either, since paint()'s own top-level branch shows
    // paintCampingScreen() instead of paintGameView() -- so this stays
    // frozen at its pre-camp value for the same reason there, not
    // because this port specifically special-cased it.
    int hotbarContext = 0;
    // GameCanvas.monsterAttacking -- same "written only during
    // paintGameView()'s own paintVisibleObjects(), read one tick later
    // by the campRequested dispatch" lag as hotbarContext above, so it's
    // a persistent local for the same reason. M36 finally makes this
    // real (VisibleObjects::AnyMonsterAttacking) -- every prior
    // milestone's CampTick::TryEnterCamp call passed a hardcoded false.
    bool monsterAttacking = false;
    // GameCanvas.campStartTime -- a GameCanvas field, not one of
    // Player's own, same reasoning as lastAttackTimeMs below.
    int64_t campStartTimeMs = 0;
    // Monster.nextSpawnIdCounter -- see camp/camp_tick.h's own doc
    // comment on TickCampState for why this is a SEPARATE counter from
    // nextDropSpawnId below, not the same one.
    int16_t nextMonsterSpawnId = 1;
    // GameCanvas.lastAttackTime -- a GameCanvas field, not one of
    // Player's own, so kept here rather than folded into PlayerState
    // (same reasoning as M30's MessagePopupState being kept separate --
    // see its own doc comment).
    int64_t lastAttackTimeMs = 0;
    // GameCanvas.lastSpellCastTime -- same reasoning as lastAttackTimeMs
    // above.
    int64_t lastSpellCastTimeMs = 0;
    // Item.nextSpawnId()'s counter, substituted the same way
    // player/player_creation.cpp's GrantStartingItems already does --
    // see MonsterRuntime::OnDeath's own doc comment.
    int16_t nextDropSpawnId = 1;
    // GameCanvas.monsterHitFlash -- another GameCanvas-only field (see
    // lastAttackTimeMs's own comment above for why it's not folded into
    // PlayerState). Set true for one tick by a landed attack, drawn (and
    // cleared) at the next render below -- at this port's own much
    // higher render rate than the original's tick-paced repaint(), this
    // means the flash is only actually visible for one of this port's
    // (much shorter) frames rather than one of the original's own
    // ~250ms ticks, same as every other tick-vs-render-rate distinction
    // already noted elsewhere in this file (e.g. the 'M' key's own
    // comment above).
    bool monsterHitFlash = false;
    // GameCanvas.spellHitFlash/selfSpellFlash -- same reasoning as
    // monsterHitFlash above.
    bool spellHitFlash = false;
    bool selfSpellFlash = false;

    // M38: whether the real main-menu/help/credits/quit-confirm flow
    // (see ui/menu_flow.h) is currently showing instead of the game
    // itself -- ESGame's own real startup sequence shows its main menu
    // first, not a fixed character directly (M20's own original
    // simplification). Up/Down/Select/Cancel are each genuinely discrete
    // keypress events in the original (Screen.handleKey()'s own
    // per-keydown dispatch, not a per-tick poll), so all 4 are
    // edge-detected at full frame rate the same way the 'M' zoom key
    // above already is, rather than gated behind clock.ConsumeTick().
    bool inMenu = true;
    bool menuUpKeyWasDown = false;
    bool menuDownKeyWasDown = false;
    bool menuSelectKeyWasDown = false;
    bool menuCancelKeyWasDown = false;

    // M39: whether the real in-game options menu (`../src/ESGame.java`'s
    // own `OptionsUI`, see ui/options_menu.h) is currently showing.
    // `GameCanvas.run()`'s own per-tick loop takes a completely
    // different branch whenever `activeScreen != null` -- it skips
    // `dispatchTickActions()` (movement/combat/camp/interact/casting,
    // ALL of it) entirely, only repainting; `keyPressed()` similarly
    // routes every key straight to `activeScreen.handleKey()` instead of
    // any game key at all. So this early-returns exactly like `inMenu`
    // above, rather than merely overlaying the game view -- see
    // OptionsMenu's own class comment for the full writeup.
    bool inOptionsMenu = false;
    bool optionsMenuUpKeyWasDown = false;
    bool optionsMenuDownKeyWasDown = false;
    bool optionsMenuSelectKeyWasDown = false;
    bool optionsMenuCancelKeyWasDown = false;

    // M40: whether the real class-selection/name-entry character-
    // creation flow (see ui/character_creation_flow.h) is showing --
    // entered from MenuFlow's own "New Game" instead of that
    // immediately constructing M20's own fixed stand-in character. Same
    // early-return shape as inMenu/inOptionsMenu above (this isn't a
    // GameCanvas.activeScreen state at all in the original -- character
    // creation happens entirely before `gameCanvas` is ever shown -- but
    // the "only one of these UIs owns the frame at a time" shape is the
    // same).
    bool inCharacterCreation = false;
    bool ccUpKeyWasDown = false;
    bool ccDownKeyWasDown = false;
    bool ccSelectKeyWasDown = false;
    bool ccCancelKeyWasDown = false;
    // Name-entry character input -- polled only while
    // CharacterCreationFlow's own NameEntry state is active (see
    // ui/name_entry.h's own class comment on why there's no real MIDP
    // TextField to defer to). 'A'-'Z'/'0'-'9' are valid Win32 virtual-
    // key codes equal to their own ASCII values, so these two ranges
    // need no separate lookup table.
    std::array<bool, 26> ccLetterKeyWasDown{};
    std::array<bool, 10> ccDigitKeyWasDown{};
    bool ccSpaceKeyWasDown = false;
    bool ccApostropheKeyWasDown = false;
    bool ccHyphenKeyWasDown = false;
    bool ccBackspaceKeyWasDown = false;

    // Same default-relative-path convention every console smoke test
    // uses (see e.g. tests/m10_frame_render_smoke.cpp) -- this exe also
    // lands in build/, two levels above dawnstar/extracted/.
    const std::string root = "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsters = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::SpellDatabase spells = dawnstar::SpellDatabase::Load(archive);
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
        dawnstar::HelpText helpText = dawnstar::HelpText::Load(archive);
        // M39: npcstrings.dat is NOT bundled inside datfiles.lmp (see
        // assets/shop_dialogue.h's own doc comment) -- loaded from its
        // own top-level file, same convention as datfiles.lmp/
        // imgfiles.lmp themselves.
        dawnstar::ShopDialogue shopDialogue = dawnstar::ShopDialogue::Load(root + "/npcstrings.dat");
        dawnstar::ImgArchive imageArchive(root + "/imgfiles.lmp");
        dawnstar::FrameTextures textures = dawnstar::FrameTextures::Load(imageArchive);
        dawnstar::MonsterImageNames monsterImageNames = dawnstar::MonsterImageNames::Load(archive);
        dawnstar::VisibleObjectTextures visibleObjectTextures =
            dawnstar::VisibleObjectTextures::Load(imageArchive, monsterImageNames);
        dawnstar::HotbarTextures hotbarTextures = dawnstar::HotbarTextures::Load(imageArchive);

        // M22/M23's live per-level monster/chest/dropped-item registry.
        // M24 populates it with every level's pre-placed monster/chest
        // spawns as part of BuildWorld below; dropped items only ever
        // enter it dynamically (loot drops, monster death -- the latter
        // still unwired, see docs/PORT_ROADMAP.md's M22 entry).
        dawnstar::WorldRegistry world(geometry.rows.size());
        std::vector<dawnstar::GeneratedLevel> levels = BuildWorld(geometry, items, monsters, world);

        // ESGame.r's real seed (System.currentTimeMillis()) was never
        // meant to be reproducible either -- see player/player_creation.h's
        // doc comment on CreateCharacter's `globalRng` parameter.
        dawnstar::JavaRandom globalRng(static_cast<int64_t>(GetTickCount64()));

        // M38: the real main menu is now shown first (see `inMenu`
        // above) -- the character itself is only constructed once "New
        // Game" is actually selected, not unconditionally at startup
        // like M20's own original simplification.
        dawnstar::MenuFlow menuFlow(helpText);
        // M39: the real in-game options menu -- constructed here (not
        // lazily once a game starts) since HelpText/ShopDialogue are
        // both already loaded and it needs nothing else at construction
        // time (see OptionsMenu::OnSelect's own doc comment on why the
        // player/character data are passed in fresh per call instead).
        // Passed BY VALUE (copied, not moved) below, same convention
        // `menuFlow`'s own HelpText parameter already uses.
        dawnstar::OptionsMenu optionsMenu(helpText, shopDialogue);
        // M40: the real class-selection/name-entry character-creation
        // flow -- "New Game" now leads here instead of immediately
        // constructing M20's own fixed class-0 stand-in character (see
        // CharacterCreationFlow's own class comment for the full
        // writeup). Also constructed eagerly, for the same reason
        // `optionsMenu` is.
        dawnstar::CharacterCreationFlow characterCreationFlow(charData, items, shopDialogue);
        std::optional<dawnstar::PlayerState> playerSlot;

        window.RunMessageLoop([&] {
            if (inMenu) {
                bool upDown = KeyPressed(VK_UP);
                if (upDown && !menuUpKeyWasDown) menuFlow.OnUp();
                menuUpKeyWasDown = upDown;

                bool downDown = KeyPressed(VK_DOWN);
                if (downDown && !menuDownKeyWasDown) menuFlow.OnDown();
                menuDownKeyWasDown = downDown;

                bool cancelDown = KeyPressed(VK_ESCAPE);
                if (cancelDown && !menuCancelKeyWasDown) menuFlow.OnCancel();
                menuCancelKeyWasDown = cancelDown;

                bool selectDown = KeyPressed(VK_RETURN);
                if (selectDown && !menuSelectKeyWasDown) {
                    switch (menuFlow.OnSelect()) {
                        case dawnstar::MenuFlowAction::StartNewGame:
                            // M40: "New Game" now leads to the real
                            // class-selection/name-entry flow instead of
                            // immediately constructing a character (M20/
                            // M38's own fixed class-0 stand-in).
                            inMenu = false;
                            inCharacterCreation = true;
                            break;
                        case dawnstar::MenuFlowAction::Exit:
                            window.Close();
                            break;
                        case dawnstar::MenuFlowAction::None:
                            break;
                    }
                }
                menuSelectKeyWasDown = selectDown;

                menuFlow.Render(backbuffer);
                window.Present(backbuffer);
                return;
            }

            // M40: only reachable once `inMenu` is false (from "New
            // Game" above). Character-by-character text input is only
            // routed to the flow while its own NameEntry state is
            // active -- `OnChar`/`OnBackspace` are themselves real
            // no-ops otherwise (see CharacterCreationFlow's own doc
            // comment), so this doesn't need to know which of the
            // flow's own states is currently showing.
            if (inCharacterCreation) {
                bool upDown = KeyPressed(VK_UP);
                if (upDown && !ccUpKeyWasDown) characterCreationFlow.OnUp();
                ccUpKeyWasDown = upDown;

                bool downDown = KeyPressed(VK_DOWN);
                if (downDown && !ccDownKeyWasDown) characterCreationFlow.OnDown();
                ccDownKeyWasDown = downDown;

                bool cancelDown = KeyPressed(VK_ESCAPE);
                if (cancelDown && !ccCancelKeyWasDown) {
                    if (characterCreationFlow.OnCancel() == dawnstar::CharacterCreationAction::CancelToMainMenu) {
                        inCharacterCreation = false;
                        inMenu = true;
                    }
                }
                ccCancelKeyWasDown = cancelDown;

                bool selectDown = KeyPressed(VK_RETURN);
                if (selectDown && !ccSelectKeyWasDown) {
                    switch (characterCreationFlow.OnSelect()) {
                        case dawnstar::CharacterCreationAction::StartGame:
                            // The real end of character creation: the
                            // REAL class/name are used here for the
                            // first time, replacing M20/M38's own fixed
                            // "class 0, Traveler" stand-in.
                            playerSlot.emplace(dawnstar::PlayerCreation::CreateCharacter(
                                characterCreationFlow.SelectedClassIndex(), characterCreationFlow.EnteredName(),
                                charData, items, globalRng));
                            inCharacterCreation = false;
                            break;
                        case dawnstar::CharacterCreationAction::CancelToMainMenu:
                        case dawnstar::CharacterCreationAction::None:
                            break;
                    }
                }
                ccSelectKeyWasDown = selectDown;

                for (int c = 'A'; c <= 'Z'; c++) {
                    bool down = KeyPressed(c);
                    size_t idx = static_cast<size_t>(c - 'A');
                    if (down && !ccLetterKeyWasDown[idx]) characterCreationFlow.OnChar(static_cast<char>(c));
                    ccLetterKeyWasDown[idx] = down;
                }
                for (int c = '0'; c <= '9'; c++) {
                    bool down = KeyPressed(c);
                    size_t idx = static_cast<size_t>(c - '0');
                    if (down && !ccDigitKeyWasDown[idx]) characterCreationFlow.OnChar(static_cast<char>(c));
                    ccDigitKeyWasDown[idx] = down;
                }
                bool spaceDown = KeyPressed(VK_SPACE);
                if (spaceDown && !ccSpaceKeyWasDown) characterCreationFlow.OnChar(' ');
                ccSpaceKeyWasDown = spaceDown;
                bool apostropheDown = KeyPressed(VK_OEM_7);
                if (apostropheDown && !ccApostropheKeyWasDown) characterCreationFlow.OnChar('\'');
                ccApostropheKeyWasDown = apostropheDown;
                bool hyphenDown = KeyPressed(VK_OEM_MINUS);
                if (hyphenDown && !ccHyphenKeyWasDown) characterCreationFlow.OnChar('-');
                ccHyphenKeyWasDown = hyphenDown;
                bool backspaceDown = KeyPressed(VK_BACK);
                if (backspaceDown && !ccBackspaceKeyWasDown) characterCreationFlow.OnBackspace();
                ccBackspaceKeyWasDown = backspaceDown;

                characterCreationFlow.Render(backbuffer);
                window.Present(backbuffer);
                return;
            }

            // M39: only reachable once `playerSlot` holds a character
            // (inOptionsMenu is only ever set true from inside the
            // tick-gated dispatch further below, itself only reachable
            // once `inMenu` is false) -- see `inOptionsMenu`'s own doc
            // comment above for why this mirrors the `inMenu` branch's
            // shape (an early return, not an overlay).
            if (inOptionsMenu) {
                dawnstar::PlayerState& optionsPlayer = *playerSlot;

                bool upDown = KeyPressed(VK_UP);
                if (upDown && !optionsMenuUpKeyWasDown) optionsMenu.OnUp();
                optionsMenuUpKeyWasDown = upDown;

                bool downDown = KeyPressed(VK_DOWN);
                if (downDown && !optionsMenuDownKeyWasDown) optionsMenu.OnDown();
                optionsMenuDownKeyWasDown = downDown;

                bool cancelDown = KeyPressed(VK_ESCAPE);
                if (cancelDown && !optionsMenuCancelKeyWasDown) {
                    if (optionsMenu.OnCancel() == dawnstar::OptionsMenuAction::ReturnToGame) {
                        inOptionsMenu = false;
                    }
                }
                optionsMenuCancelKeyWasDown = cancelDown;

                bool selectDown = KeyPressed(VK_RETURN);
                if (selectDown && !optionsMenuSelectKeyWasDown) {
                    switch (optionsMenu.OnSelect(optionsPlayer, charData, items, spells, levels, world)) {
                        case dawnstar::OptionsMenuAction::ReturnToGame:
                            inOptionsMenu = false;
                            break;
                        case dawnstar::OptionsMenuAction::Exit:
                            window.Close();
                            break;
                        case dawnstar::OptionsMenuAction::UseInventoryItem: {
                            // M41: the one inventory-item action
                            // OptionsMenu can't finish by itself (see its
                            // own OptionsMenuAction::UseInventoryItem doc
                            // comment) -- perform the real
                            // CombatResolution::UseItem call here, with a
                            // freshly re-derived front monster (same
                            // "SIMPLIFIED but not lossy" reasoning as
                            // combat/combat_tick.cpp's own ProcessAttack/
                            // ProcessSpellCast), then hand back to
                            // FinishUseItem() to run the shared
                            // Drop/Equip/Unequip/Learn/Use tail.
                            int slot = optionsMenu.PendingUseItemSlot();
                            auto* record = dawnstar::PlayerMovement::MonsterInFront(optionsPlayer, levels, world);
                            if (record != nullptr) {
                                dawnstar::MonsterState target = dawnstar::MonsterRuntime::FromBytes(*record);
                                dawnstar::CombatResolution::UseItem(optionsPlayer, slot, &target, items, monsters,
                                                                    levels, world, globalRng);
                                *record = dawnstar::MonsterRuntime::ToBytes(target);
                            } else {
                                dawnstar::CombatResolution::UseItem(optionsPlayer, slot, nullptr, items, monsters,
                                                                    levels, world, globalRng);
                            }
                            if (optionsMenu.FinishUseItem(optionsPlayer, items) ==
                                dawnstar::OptionsMenuAction::ReturnToGame) {
                                inOptionsMenu = false;
                            }
                            break;
                        }
                        case dawnstar::OptionsMenuAction::None:
                            break;
                    }
                }
                optionsMenuSelectKeyWasDown = selectDown;

                optionsMenu.Render(backbuffer);
                window.Present(backbuffer);
                return;
            }

            dawnstar::PlayerState& player = *playerSlot;
            // Sampled every loop iteration rather than gated behind
            // clock.ConsumeTick(): the original's keyPressed() event
            // fires immediately on a physical key-down, independent of
            // the 250ms game tick, so polling it at full framerate is
            // the more faithful reproduction here (its actual effect --
            // minimapDirty -- is still only ever consumed inside the
            // tick-gated block below, same as the original).
            bool zoomKeyDown = KeyPressed('M');
            if (zoomKeyDown && !zoomKeyWasDown) {
                player.minimapZoomedOut = !player.minimapZoomedOut;
                player.minimapDirty = true;
            }
            zoomKeyWasDown = zoomKeyDown;

            // GameCanvas.keyPressed()'s own `key == 53` handler
            // (spellCycleRequested) -- captured at full frame rate for
            // the same immediate-keydown-event reason as the zoom key
            // above, but left PENDING rather than acted on here (see
            // spellCyclePending's own doc comment above).
            bool spellCycleKeyDown = KeyPressed('C');
            if (spellCycleKeyDown && !spellCycleKeyWasDown) {
                spellCyclePending = true;
            }
            spellCycleKeyWasDown = spellCycleKeyDown;

            // GameCanvas.keyPressed()'s own `key == 57` handler
            // (interactRequested, gated on hotbarContext == 2) -- same
            // full-frame-rate edge detection as cycle above, but with
            // that extra hotbarContext gate reproduced at the exact same
            // point the original checks it (inside the keydown handler
            // itself, reading whatever hotbarContext currently holds).
            bool interactKeyDown = KeyPressed('I');
            if (interactKeyDown && !interactKeyWasDown && hotbarContext == 2) {
                interactPending = true;
            }
            interactKeyWasDown = interactKeyDown;

            // GameCanvas.keyPressed()'s own `key == 48` handler
            // (campRequested, gated on hotbarContext == 0) -- same shape
            // as interact above, just with the opposite hotbarContext
            // value (explore, not interact).
            bool campKeyDown = KeyPressed('Z');
            if (campKeyDown && !campKeyWasDown && hotbarContext == 0) {
                campPending = true;
            }
            campKeyWasDown = campKeyDown;

            // GameCanvas.keyPressed()'s own `key == 55` handler
            // (optionsRequested) -- same full-frame-rate edge detection
            // as interact/camp above, but with NO hotbarContext gate
            // (matching the original's own handler, which sets this
            // unconditionally). Remapped from the original's own
            // numeric-keypad '7' to 'O' for a PC keyboard -- see
            // `inOptionsMenu`'s own doc comment above for what actually
            // opening this menu does.
            bool optionsKeyDown = KeyPressed('O');
            if (optionsKeyDown && !optionsKeyWasDown) {
                optionsPending = true;
            }
            optionsKeyWasDown = optionsKeyDown;

            if (clock.ConsumeTick()) {
                // GameCanvas.run()'s own `now = System.
                // currentTimeMillis()`, sampled once per tick and reused
                // for every showMessage/timeout check below -- M30.
                int64_t nowMs = static_cast<int64_t>(GetTickCount64());

                // GameCanvas.run()'s own per-tick campState 1/2/3 state
                // machine -- the block immediately preceding
                // dispatchTickActions() itself in the original. Returns
                // whether the rest of this tick's normal work should run
                // at all (false while actually camping, before the
                // relevant timer elapses); also sets suppressMoveThisTick
                // true on the one tick a camp cycle actually resolves --
                // see camp/camp_tick.h's own doc comment on both.
                bool suppressMoveThisTick = false;
                bool runTick = dawnstar::CampTick::TickCampState(player, levels, world, items, monsters, globalRng,
                                                                  nowMs, campStartTimeMs, nextMonsterSpawnId,
                                                                  messagePopup, suppressMoveThisTick);

                if (runTick) {
                    // GameCanvas.run()'s own `tickNearbyMonsters()` call
                    // -- the FIRST thing inside `if (runTick)` in the
                    // original, run every tick the player isn't actually
                    // camping: every monster on the player's own level
                    // within Manhattan distance 1-3 either attacks
                    // (distance 1) or takes a step toward the player
                    // (distance 2-3), at most once every 5 calls each --
                    // M36.
                    dawnstar::CombatTick::TickNearbyMonsters(player, levels, world, charData, items, monsters, nowMs,
                                                              globalRng, messagePopup);

                    // GameCanvas.paintHotbar()'s own `hotbarContext`
                    // field, as it stood after the LAST tick's
                    // dispatch/refresh -- exactly what keyPressed()
                    // itself reads on a real device (the hotbar shown is
                    // always one tick behind whatever action that
                    // tick's dispatch takes, matching the original's own
                    // real timing: paintHotbar's write to this field
                    // only ever happens once per repaint, right after a
                    // tick's dispatch has already run). Reassigns the
                    // persistent local declared above (M34) rather than
                    // a fresh tick-local one, since the interact/camp
                    // keys' own full-frame-rate edge detection needs to
                    // read this same value between ticks too.
                    hotbarContext = dawnstar::HotbarRenderer::ComputeHotbarContext(
                        player.monsterTargeted, player.chestInSight, player.npcInSight);

                    // GameCanvas.dispatchTickActions()'s own if/else-if
                    // priority chain: only camp/interact/cast/cycle/
                    // attack/options/unusedKey9/move ever fires per
                    // tick, never more than one. camp outranks interact
                    // outranks cast outranks cycle outranks attack
                    // outranks options outranks move, matching the
                    // original's own ordering exactly. (unusedKey9 is
                    // real dead code in the original -- see GameCanvas.
                    // java's own `unusedKey9Request` field -- so it has
                    // nothing to port.)
                    //
                    // Cast, like attack, is polled every tick rather
                    // than edge-detected: GameCanvas.keyPressed()'s own
                    // `key == 51` handler sets castSpellRequested
                    // unconditionally (no hotbarContext gate, unlike
                    // attack's own `key == 49`), but processSpellCast's
                    // own 500ms cooldown (lastSpellCastTimeMs) throttles
                    // it to the same pacing a held key would produce
                    // anyway -- same reasoning as attackActive below.
                    bool castActive = KeyPressed('S');
                    bool attackActive = KeyPressed('A') && hotbarContext == 1;

                    bool moveAttempted = false;
                    if (campPending) {
                        // GameCanvas.dispatchTickActions()'s own
                        // campRequested branch -- `monsterAttacking`
                        // (the persistent local above) reads whatever
                        // the LAST render step's paintVisibleObjects()
                        // equivalent computed, same one-tick lag as
                        // hotbarContext -- M36 makes this a real value
                        // instead of a hardcoded false.
                        dawnstar::CampTick::TryEnterCamp(player, globalRng, nowMs, campStartTimeMs, messagePopup,
                                                          monsterAttacking);
                        campPending = false;
                    } else if (interactPending) {
                        dawnstar::InteractTick::ProcessInteract(player, levels, world, items, messagePopup, nowMs);
                        interactPending = false;
                    } else if (castActive) {
                        dawnstar::CombatTick::ProcessSpellCast(player, levels, world, monsters, items, charData,
                                                                spells, messagePopup, globalRng, nowMs,
                                                                lastSpellCastTimeMs, spellHitFlash, selfSpellFlash);
                    } else if (spellCyclePending) {
                        dawnstar::CombatTick::CycleSpell(player, spells, messagePopup, nowMs);
                        spellCyclePending = false;
                    } else if (attackActive) {
                        if (dawnstar::CombatTick::ProcessAttack(player, levels, world, monsters, items, charData,
                                                                  globalRng, nowMs, lastAttackTimeMs)) {
                            monsterHitFlash = true;
                        }
                    } else if (optionsPending) {
                        // GameCanvas.dispatchTickActions()'s own
                        // `optionsRequested` branch: `this.
                        // openOptionsMenu();` -- just a display swap in
                        // the original (`setCurrentDisplay(OptionsUI)`),
                        // reproduced here as the same `inOptionsMenu`
                        // early-return gate `inMenu` above already uses
                        // (see its own doc comment for why that's the
                        // faithful shape, not merely an overlay).
                        inOptionsMenu = true;
                        optionsPending = false;
                    } else if (!suppressMoveThisTick) {
                        // GameCanvas.run()'s own steady-250ms-tick
                        // cadence (see engine/game_clock.h) is also when
                        // the real key state would be sampled -- one
                        // Move() per tick while a key is held reproduces
                        // that pacing rather than moving once per
                        // PeekMessage-idle spin. `suppressMoveThisTick`
                        // (see camp/camp_tick.h's own doc comment) skips
                        // this whole branch on the one tick a camp cycle
                        // just resolved, matching GameCanvas.
                        // suppressMoveInput's own real effect.
                        moveAttempted = true;
                        int slotsBefore = player.inventoryCount;
                        if (KeyPressed(VK_UP)) {
                            dawnstar::PlayerMovement::Move(player, 1, false, levels, world, items);
                        } else if (KeyPressed(VK_DOWN)) {
                            dawnstar::PlayerMovement::Move(player, 2, false, levels, world, items);
                        } else if (KeyPressed(VK_RIGHT)) {
                            dawnstar::PlayerMovement::Move(player, 3, false, levels, world, items);
                        } else if (KeyPressed(VK_LEFT)) {
                            dawnstar::PlayerMovement::Move(player, 4, false, levels, world, items);
                        } else {
                            moveAttempted = false;
                        }

                        // GameCanvas.commitMove()'s own "only when
                        // pendingMoveDir != 0" gate -- called
                        // unconditionally whenever a move was requested
                        // this tick, regardless of whether it actually
                        // committed (matching the original, which does
                        // all of this right after player.move() with no
                        // success check).
                        if (moveAttempted) {
                            // commitMove()'s own `int pickedUp = player.
                            // inventoryCount - slotsBefore;` -- diffed
                            // the same way here as there, rather than
                            // threading a count out of Move() itself --
                            // M30.
                            int pickedUp = player.inventoryCount - slotsBefore;
                            if (pickedUp == 1) {
                                int slot = player.inventoryCount - 1;
                                int itemId = std::abs(static_cast<int>(player.inventoryItemIds[slot]));
                                dawnstar::MessagePopup::Show(
                                    messagePopup,
                                    dawnstar::MessagePopup::WrapToTwoLines(
                                        items.name[static_cast<size_t>(itemId - 1)]),
                                    -1, nowMs);
                            } else if (pickedUp > 1) {
                                dawnstar::MessagePopup::Show(messagePopup, {"Several", "items!"}, -1, nowMs);
                            }

                            // refreshChestInSight(): ChestInFront's own
                            // query half lives in PlayerMovement (see its
                            // own doc comment for why); the showMessage
                            // half is here.
                            const std::array<uint8_t, 8>* chest =
                                dawnstar::PlayerMovement::ChestInFront(player, levels, world);
                            // M31: persisted for HotbarRenderer::
                            // ComputeHotbarContext (see player/
                            // player_state.h's own doc comment on why).
                            player.chestInSight = chest != nullptr;
                            if (chest != nullptr) {
                                dawnstar::MessagePopup::Show(messagePopup, {"Chest", ""}, 1, nowMs);
                            }

                            // refreshNpcInSight(): same split as
                            // refreshChestInSight above --
                            // RefreshNpcInSight itself only sets
                            // player.npcInSight (M28); the shop-greeting
                            // showMessage call is here.
                            dawnstar::PlayerMovement::RefreshNpcInSight(player, levels, world);
                            if (player.npcInSight >= 0) {
                                dawnstar::MessagePopup::Show(
                                    messagePopup,
                                    dawnstar::MessagePopup::WrapToTwoLines(kShopNames[player.npcInSight]), 1, nowMs);
                            }

                            // commitMove()'s own unconditional
                            // `this.minimapDirty = true;` -- M29.
                            player.minimapDirty = true;
                        }
                    }

                    // dispatchTickActions()'s own tail:
                    // refreshTargetMonster() + resolveMonsterDeath(),
                    // unconditional every tick regardless of which
                    // action (if any) fired above -- M32.
                    dawnstar::CombatTick::RefreshAndResolveTargetMonster(player, levels, world, monsters, items,
                                                                           messagePopup, globalRng, nowMs,
                                                                           nextDropSpawnId);

                    // GameCanvas.run()'s own per-tick order: movement
                    // first, then Player.tickVisibleObjects() (M25) --
                    // unconditional every tick, not just on a movement
                    // tick.
                    dawnstar::VisibleObjects::Tick(player, levels, world);

                    // run()'s own "if (minimapDirty) refreshMinimap()"
                    // gate, right after tickVisibleObjects -- M29.
                    if (player.minimapDirty) {
                        dawnstar::MinimapRenderer::Refresh(minimap, player, levels, world);
                    }

                    // run()'s own unconditional per-tick auto-hide
                    // timeout check -- M30.
                    dawnstar::MessagePopup::Tick(messagePopup, nowMs);
                }
            }

            // GameCanvas.paint()'s own top-level branch: paintCampingScreen()
            // entirely REPLACES paintGameView() while camping (not layered
            // on top of it) -- no corridor/HUD/hotbar/minimap at all.
            if (player.campState != 0) {
                // paintCampingScreen(): a black screen plus "CAMPING"
                // centered in BIG_MESSAGE_FONT -- another MIDP built-in
                // system font (like SMALL_FONT, see graphics/
                // bitmap_font.h's own doc comment) with no recoverable
                // real glyph shapes/metrics, so this reuses the same
                // invented BitmapFont rather than hand-authoring a
                // second, bigger invented font purely for this one
                // screen.
                backbuffer.Fill(dawnstar::PackRGB565(0, 0, 0));
                const std::string campingText = "CAMPING";
                int textX = (dawnstar::Backbuffer::kWidth - dawnstar::BitmapFont::StringWidth(campingText)) / 2;
                int textY = (dawnstar::Backbuffer::kHeight - dawnstar::BitmapFont::kGlyphHeight) / 2;
                dawnstar::BitmapFont::DrawString(backbuffer, textX, textY, campingText,
                                                  dawnstar::PackRGB565(255, 255, 255));
                window.Present(backbuffer);
                return;
            }

            dawnstar::DungeonView view(levels, player.currentLevel - 1);
            dawnstar::FrameRenderer::Render(backbuffer, textures, view, player.tileX, player.tileY, player.facing,
                                             levels[static_cast<size_t>(player.currentLevel - 1)].number);
            dawnstar::VisibleObjectRenderer::Render(backbuffer, visibleObjectTextures, player.visibleObjects);
            // GameCanvas.paintVisibleObjects()'s own `monsterAttacking`
            // recomputation -- M36. Only ever updated here (skipped
            // during the camping-screen `return` above), matching the
            // original's own paintHotbar-style one-tick lag (see this
            // variable's own doc comment above).
            monsterAttacking = dawnstar::VisibleObjects::AnyMonsterAttacking(player);
            // paintGameView()'s own "if (npcInSight >= 0)" gate, drawn
            // right after paintVisibleObjects and before
            // paintStatusBars -- M28.
            if (player.npcInSight >= 0) {
                dawnstar::VisibleObjectRenderer::PaintNpcPortrait(backbuffer, visibleObjectTextures,
                                                                    player.npcInSight);
            }
            dawnstar::HudRenderer::PaintStatusBars(backbuffer, player, charData);
            // paintGameView()'s own paintHotbar() call -- M31; real
            // `monsterTargeted` since M32.
            int renderHotbarContext = dawnstar::HotbarRenderer::ComputeHotbarContext(
                player.monsterTargeted, player.chestInSight, player.npcInSight);
            dawnstar::HotbarRenderer::Paint(backbuffer, hotbarTextures, renderHotbarContext);
            // paintGameView()'s own paintMessagePopup() call -- M30.
            dawnstar::MessagePopup::Paint(backbuffer, messagePopup);
            // paintGameView()'s own paintActionFlashes() call -- all 3
            // cases now wired (monsterHitFlash since M32; spellHitFlash/
            // selfSpellFlash since M33), same icon/offset table as the
            // original.
            if (monsterHitFlash) {
                int x = 40 + dawnstar::LingoRandomInt(globalRng, 30);
                int y = 50 + dawnstar::LingoRandomInt(globalRng, 20);
                dawnstar::HotbarRenderer::PaintActionFlashIcon(backbuffer, hotbarTextures, 6, x, y);
                monsterHitFlash = false;
            }
            if (spellHitFlash) {
                int x = 40 + dawnstar::LingoRandomInt(globalRng, 30);
                int y = 50 + dawnstar::LingoRandomInt(globalRng, 22);
                dawnstar::HotbarRenderer::PaintActionFlashIcon(backbuffer, hotbarTextures, 8, x, y);
                spellHitFlash = false;
            }
            if (selfSpellFlash) {
                int x = 50 + dawnstar::LingoRandomInt(globalRng, 2);
                int y = 80 + dawnstar::LingoRandomInt(globalRng, 2);
                dawnstar::HotbarRenderer::PaintActionFlashIcon(backbuffer, hotbarTextures, 7, x, y);
                selfSpellFlash = false;
            }
            // paintGameView()'s own actual LAST drawing step (outside
            // its own try block, after paintMessagePopup/
            // paintActionFlashes/paintErrorOverlay -- paintErrorOverlay
            // alone still isn't ported) -- M29.
            dawnstar::MinimapRenderer::Composite(backbuffer, minimap, player);
            window.Present(backbuffer);
        });
    } catch (const std::exception&) {
        // No fallback rendering is possible without the extracted
        // assets -- surface a visibly distinct color (rather than the
        // placeholder's own dark blue) so a missing-assets failure
        // isn't mistaken for "the game is just idle".
        backbuffer.Fill(dawnstar::PackRGB565(80, 0, 0));
        window.RunMessageLoop([&] { window.Present(backbuffer); });
    }

    return 0;
}
