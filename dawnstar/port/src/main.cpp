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
// (see ui/options_menu.h's own class comment). M42 made "Save Game"/
// "Load Game" real too: this file runs GameSave::SaveGameState/
// LoadGameState/ResumeGame itself while presenting ui/loading_screen.h's
// own progress bar at every reported percent -- the port's stand-in for
// the original's own background Thread + repaint()/serviceRepaints()
// pair, and the same ESGame-level-not-Screen-level split (see
// OptionsMenuAction::SaveGame/LoadGame's own doc comment). M43 handed
// the Reveal Traitor quiz its own live item spawn-id counter
// (nextDropSpawnId, for the StarFrost grant). M44 added run()'s own
// per-tick timed tail: TickStatusCountdowns + the secondAccum-gated
// PassiveTick::TickPerSecond (the passive regen/drain, effect countdown,
// and the ambush spawner M43's Reveal Traitor result arms), plus the real
// "Game Over" -> "Exiting" -> exit chain its EndOfGame result plays.
#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Populated by the CRT for a wWinMain entry point the same way argc/argv
// are for main() -- not declared in any header this file already includes.
extern "C" {
extern int __argc;
extern wchar_t** __wargv;
}

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
#include "death/death_tick.h"
#include "dungeon/dungeon_runtime.h"
#include "engine/game_clock.h"
#include "graphics/backbuffer.h"
#include "graphics/bitmap_font.h"
#include "interact/interact_tick.h"
#include "npc/shop_interaction.h"
#include "passive/passive_tick.h"
#include "platform/win32/window.h"
#include "player/player_combat_stats.h"
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
#include "save/game_save.h"
#include "ui/screen.h"
#include "ui/character_creation_flow.h"
#include "ui/loading_screen.h"
#include "ui/menu_flow.h"
#include "ui/boot_splash.h"
#include "ui/level_up_menu.h"
#include "ui/npc_menu.h"
#include "ui/options_menu.h"
#include "util/java_random.h"
#include "util/text.h"
#include "world/dungeon_generator.h"
#include "world/dungeon_view.h"

namespace {

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

std::string NarrowArg(const wchar_t* text) {
    if (!text) return std::string();
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return std::string();
    std::string narrow(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, narrow.data(), size, nullptr, nullptr);
    return narrow;
}

// argv[1], if given (the launcher's dawnstar_port.exe <dataPath> call --
// see port/src/launcher/launcher_main.cpp's Play()), overrides the asset
// root; falls back to the repo-relative default every smoke test and a
// plain dev build from build/ already uses. __argc/__wargv are populated
// by the CRT for a wWinMain entry point same as argc/argv would be for
// main().
std::string ResolveAssetRoot() {
    if (__argc > 1 && __wargv && __wargv[1] && __wargv[1][0] != L'\0') {
        return NarrowArg(__wargv[1]);
    }
    return "../../extracted";
}

// DAWNSTAR_USER_DIR (set by the launcher to <install>/user, mirroring
// shadowkey-decomp's SK_USER_DIR) is where saves and the log file live.
// Unset -- a plain dev build -- both stay exactly where they've always
// been: "saves" relative to the working directory, no log file at all.
std::string ResolveUserDir() {
    const char* value = std::getenv("DAWNSTAR_USER_DIR");
    return value ? std::string(value) : std::string();
}

// DAWNSTAR_SCALE (set by the launcher from its own window-size picker,
// mirroring SK_SCALE) overrides the window's integer scale over the
// native 176x208 backbuffer. Unset, or garbage, keeps the *2 this file
// hardcoded before the launcher existed.
int ResolveScale() {
    const char* value = std::getenv("DAWNSTAR_SCALE");
    if (!value) return 2;
    int scale = std::atoi(value);
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    return scale;
}

// Redirects stdout/stderr into <userDir>/dawnstar_port.log so a bug report
// has something to attach -- this is a WINAPI-subsystem app with no
// console to print to otherwise. A no-op (same as running with no launcher
// at all) when userDir is empty.
void OpenLogFile(const std::string& userDir) {
    if (userDir.empty()) return;
    std::error_code error;
    std::filesystem::create_directories(userDir, error);
    const std::string path = (std::filesystem::path(userDir) / "dawnstar_port.log").string();
    FILE* unused = nullptr;
    freopen_s(&unused, path.c_str(), "a", stdout);
    freopen_s(&unused, path.c_str(), "a", stderr);
    std::printf("--- dawnstar_port starting ---\n");
    std::fflush(stdout);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    const std::string userDir = ResolveUserDir();
    OpenLogFile(userDir);
    const int scale = ResolveScale();
    dawnstar::Window window(dawnstar::Backbuffer::kWidth * scale, dawnstar::Backbuffer::kHeight * scale,
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
    // M44: GameCanvas.run()'s own tail bookkeeping -- run()'s own `long
    // now` / `long secondAccum` locals (lines ~1251-1252), consumed at
    // the top of each tick (M51 moved this up from the tick's own tail --
    // see `elapsedMs`'s own doc comment there). lastTickNowMs mirrors the
    // original's `prevNow` (the previous iteration's `now`, which
    // `elapsed` is computed against); initialized from the same
    // GetTickCount64() source the tick itself samples from, at the point
    // run() itself samples its pre-loop `now`.
    int64_t lastTickNowMs = static_cast<int64_t>(GetTickCount64());
    int64_t secondAccumMs = 0;
    // M44 (Game Over) + M56 (Victory): ESGame's own `endOfGameUI =
    // newGameOverUI()`/`newEndOfGameUI()` plus the shared
    // secondaryParam==200/201 -> "Exiting" (399) -> exit() dispatch,
    // played by main.cpp here (this port's ESGame stand-in), exactly
    // like the Options menu's own main.cpp-performed actions. Both
    // triggers (the ambush-spawner checkpoint pushing a level past 5
    // monsters, and killing the literal type-42 end-game monster) reuse
    // this same state -- the original's own dispatch never distinguishes
    // 200 from 201 past the initial screen. The inGameOver early-return
    // below also genuinely PAUSES the whole tick loop, the same way
    // inMenu/inOptionsMenu do -- matching the original's own
    // `activeScreen != null` branch, which stops tickPerSecond itself
    // (the ambush clock freezes while the Game Over/Victory screen
    // shows).
    bool inGameOver = false;
    bool gameOverExiting = false;
    bool gameOverSelectKeyWasDown = false;
    bool gameOverCancelKeyWasDown = false;
    dawnstar::Screen gameOverScreen(dawnstar::ScreenMode::PlainList);
    dawnstar::Screen gameOverExitingScreen(dawnstar::ScreenMode::PlainList);
    // M45/M46: GameCanvas.openNpcDialogue()'s greeting popup and, via its Ok,
    // the whole NPCChoicesUI menu graph (buy/sell/train/give/question/warp
    // -- see ui/npc_menu.h). Shown whenever InteractTick::ProcessInteract's
    // npcInSight branch fires. Same early-return-pauses-the-tick-loop shape
    // as inGameOver/inOptionsMenu above (`activeScreen != null`); leaves
    // only when the flow itself returns to the game view (Cancel on a
    // choices screen, a warp, ...).
    bool inNpcMenu = false;
    bool npcMenuUpKeyWasDown = false;
    bool npcMenuDownKeyWasDown = false;
    bool npcMenuSelectKeyWasDown = false;
    bool npcMenuCancelKeyWasDown = false;
    dawnstar::NpcMenu npcMenu;
    // M49: GameCanvas.run()'s `levelUpPending` hand-off into ESGame's
    // LevelUpUI (three attribute picks -- see ui/level_up_menu.h). Same
    // early-return-pauses-the-tick-loop shape as inNpcMenu; there is no
    // Cancel, so it only leaves after the third pick.
    bool inLevelUp = false;
    bool levelUpUpKeyWasDown = false;
    bool levelUpDownKeyWasDown = false;
    bool levelUpSelectKeyWasDown = false;
    dawnstar::LevelUpMenu levelUpMenu;
    // GameCanvas.campStartTime -- a GameCanvas field, not one of
    // Player's own, same reasoning as lastAttackTimeMs below.
    int64_t campStartTimeMs = 0;
    // GameCanvas.deathTime -- same "GameCanvas field, not Player's own,
    // read only by the tick-gated state machine itself" reasoning as
    // campStartTimeMs just above (see player/player_state.h's own
    // deathState doc comment for why deathState itself, unlike this, DID
    // need to move into PlayerState) -- M50.
    int64_t deathTimeMs = 0;
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
    // M48: LoadingScreen mode 2 -- the boot splash shown before the main
    // menu (ESGame.initSplash()). Runs first, timed from its own first
    // frame; early-returns like inMenu below. The original has no skip key;
    // Return/Esc/Space skip it here since a PC start-up has nothing left to
    // load behind the wait.
    bool inSplash = true;
    int64_t splashStartMs = -1;
    bool splashSkipKeyWasDown = false;
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

    // argv[1], when the launcher (or anyone else) passes one; otherwise
    // the same default-relative-path convention every console smoke test
    // uses (see e.g. tests/m10_frame_render_smoke.cpp) -- this exe also
    // lands in build/, two levels above dawnstar/extracted/.
    const std::string root = ResolveAssetRoot();

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

        // M42: ESGame's own `Shop.*` static state (Item.nextSpawnId,
        // Monster.nextSpawnIdCounter, the 9 firstVisit / 4 interactionCount /
        // 4 rewardsGiven / 4 questState1 / 4 questState2 arrays and
        // showDeathGreeting), carried as this own flat save container
        // (rather than read/written straight from `ShopState`, npc/
        // shop_interaction.h's own M45 port of `Shop.java`'s live state)
        // -- see save/game_save.h's own OtherStateInfo comment for why.
        // Starts at Shop.reset()'s own post-condition, is reset to it
        // again whenever a new character is created (Player.resetState()'s own
        // `Shop.reset()` call, ../../../src/Player.java line 2576), and is
        // replaced wholesale by a real load.
        dawnstar::OtherStateInfo otherState = dawnstar::OtherStateInfo::Reset();
        // M45: the real, live counterpart of the same Shop.* fields
        // `otherState` above carries as a flat save container. M46 syncs the
        // two around every save/load (see syncOtherStateFromLive/
        // syncLiveFromOtherState below). Reset alongside `otherState`, same
        // two call sites (startup and a fresh character's own Shop.reset()
        // tail below).
        dawnstar::ShopState shopState = dawnstar::ShopState::Reset();
        // M46: ESGame's own Shop.*/Item.nextSpawnId/Monster.nextSpawnIdCounter
        // statics are what saveGameState()'s writeOtherStateInfoToBytes()
        // serializes and readOtherStateInfo() restores; here the live copies
        // are shopState/nextDropSpawnId/nextMonsterSpawnId, so mirror them
        // into/out of the flat OtherStateInfo container around save/load.
        auto syncOtherStateFromLive = [&] {
            otherState.itemNextSpawnId = nextDropSpawnId;
            otherState.monsterNextSpawnIdCounter = nextMonsterSpawnId;
            otherState.shopFirstVisit = shopState.firstVisit;
            otherState.shopInteractionCount = shopState.interactionCount;
            otherState.shopRewardsGiven = shopState.rewardsGiven;
            otherState.shopQuestState1 = shopState.questState1;
            otherState.shopQuestState2 = shopState.questState2;
            otherState.shopShowDeathGreeting = shopState.showDeathGreeting;
        };
        auto syncLiveFromOtherState = [&] {
            nextDropSpawnId = otherState.itemNextSpawnId;
            nextMonsterSpawnId = otherState.monsterNextSpawnIdCounter;
            shopState.firstVisit = otherState.shopFirstVisit;
            shopState.interactionCount = otherState.shopInteractionCount;
            shopState.rewardsGiven = otherState.shopRewardsGiven;
            shopState.questState1 = otherState.shopQuestState1;
            shopState.questState2 = otherState.shopQuestState2;
            shopState.showDeathGreeting = otherState.shopShowDeathGreeting;
        };
        // The port's own RecordStore substitute: a directory of
        // "es_gamestate<N>" files, of which exactly one (the newest) is ever
        // kept -- see save/game_save.h's own class comment. Under
        // DAWNSTAR_USER_DIR when the launcher set one (see ResolveUserDir),
        // otherwise "saves" relative to the working directory (build/),
        // same convention as `root` above.
        const std::string saveDir =
            userDir.empty() ? "saves" : (std::filesystem::path(userDir) / "saves").string();

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

        dawnstar::BootSplash bootSplash = dawnstar::BootSplash::Load(root, imageArchive);

        window.RunMessageLoop([&] {
            if (inSplash) {
                const int64_t now = static_cast<int64_t>(GetTickCount64());
                if (splashStartMs < 0) splashStartMs = now;
                bool skipDown = KeyPressed(VK_RETURN) || KeyPressed(VK_ESCAPE) || KeyPressed(VK_SPACE);
                bool skip = skipDown && !splashSkipKeyWasDown;
                splashSkipKeyWasDown = skipDown;
                if (skip || dawnstar::BootSplash::IsDone(now - splashStartMs)) {
                    inSplash = false;
                    // A key still held from the skip mustn't also fire
                    // the main menu's first selection/cancel.
                    menuSelectKeyWasDown = KeyPressed(VK_RETURN);
                    menuCancelKeyWasDown = KeyPressed(VK_ESCAPE);
                } else {
                    // M57: ESGame.runAppload()/allocateESGame() are what
                    // actually mutate `splashUI.percent` on the original
                    // (0 -> 5 -> 10 -> 15 -> ... -> 100, driven by real
                    // loading work on a real device) -- LoadingScreen
                    // itself just repaints whatever it currently reads.
                    // This port does every bit of that loading upfront,
                    // before the splash is ever shown (see this file's
                    // own asset-loading block above), so there is no
                    // real progress left to report by the time this
                    // callback ever runs -- `bootSplash.SetPercent`
                    // previously was simply never called at all, which
                    // left `percent_` frozen at its own default-
                    // constructed 100 for the splash's entire run: the
                    // bar rendered essentially full from its very first
                    // visible frame instead of visibly filling. Since
                    // the original's own real timing (dominated by
                    // `allocAllDungeons()`'s cost on 2003-era hardware)
                    // has no faithful modern equivalent to replay, this
                    // drives a plain linear fill across the bar's own
                    // visible window instead -- 0% at kBarStartMs, 100%
                    // at kCopyrightStartMs -- so the bar actually
                    // animates rather than sitting static-full.
                    const int64_t splashElapsed = now - splashStartMs;
                    if (splashElapsed >= dawnstar::BootSplash::kBarStartMs) {
                        const int64_t span =
                            dawnstar::BootSplash::kCopyrightStartMs - dawnstar::BootSplash::kBarStartMs;
                        const int64_t into = splashElapsed - dawnstar::BootSplash::kBarStartMs;
                        const int percent = static_cast<int>(std::min<int64_t>(100, into * 100 / span));
                        bootSplash.SetPercent(percent);
                    }
                    bootSplash.Render(backbuffer, splashElapsed);
                    window.Present(backbuffer);
                    return;
                }
            }

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
                            // Bugfix: this same physical Enter press is
                            // still held on the very next tick (the
                            // inMenu block's own `return` below only
                            // skips the REST of *this* tick, not the
                            // next one) -- without this, the character
                            // creation block's own `ccSelectKeyWasDown`
                            // (still false, never touched before this
                            // point) would read it as a brand new Select
                            // press and immediately fire OnSelect() while
                            // still on ClassSelect's own default index 0
                            // (Barbarian), silently skipping the class-
                            // selection screen the player never actually
                            // saw yet. Same "a key still held from the
                            // transition mustn't also fire the next
                            // screen's own first action" precaution the
                            // splash->menu transition above already
                            // takes for exactly this reason.
                            ccSelectKeyWasDown = KeyPressed(VK_RETURN);
                            ccCancelKeyWasDown = KeyPressed(VK_ESCAPE);
                            break;
                        case dawnstar::MenuFlowAction::ContinueGame: {
                            // M52: ESGame's own mainMenuUI case 1
                            // (`gameCanvas.stopGameThread()` -- moot,
                            // no game thread is running yet from the
                            // main menu -- then `helperThreadState = 6`)
                            // plus run()'s own helperThreadState==6
                            // branch. Same GameSave::LoadGameState/
                            // ResumeGame machinery M42/M46 already wired
                            // to the in-game Options menu's own "Load
                            // Game" (OptionsMenuAction::LoadGame below),
                            // just reached from here instead, BEFORE any
                            // game session exists -- so this populates a
                            // fresh `playerSlot` rather than mutating an
                            // already-live `*playerSlot`.
                            dawnstar::LoadingScreen loadGameUI(dawnstar::LoadingScreenMode::LoadingGame);
                            loadGameUI.Render(backbuffer);
                            window.Present(backbuffer);
                            dawnstar::PlayerState loaded;
                            bool loadedOk = dawnstar::GameSave::LoadGameState(
                                saveDir, loaded, world, otherState, [&](int percent) {
                                    loadGameUI.SetPercent(percent);
                                    loadGameUI.Render(backbuffer);
                                    window.Present(backbuffer);
                                });
                            if (loadedOk) {
                                playerSlot.emplace(std::move(loaded));
                                syncLiveFromOtherState();
                                // run()'s own tail: resumeGame(), then
                                // `loadGameUI.percent = 100` + one final
                                // repaint, then setCurrentDisplay(gameCanvas)
                                // + startGameThread() -- this port's own
                                // `inMenu = false` below plays that role.
                                dawnstar::GameSave::ResumeGame(*playerSlot, levels, world, [&](int percent) {
                                    loadGameUI.SetPercent(percent);
                                    loadGameUI.Render(backbuffer);
                                    window.Present(backbuffer);
                                });
                                loadGameUI.SetPercent(100);
                                loadGameUI.Render(backbuffer);
                                window.Present(backbuffer);
                                inMenu = false;
                            } else {
                                // run()'s own else-branch: noSavedGameUI,
                                // whose backTarget here is mainMenuUI
                                // (see MenuFlow::ShowNoSavedGame's own
                                // doc comment).
                                menuFlow.ShowNoSavedGame();
                            }
                            break;
                        }
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
                            // M42: CreateCharacter's own tail is
                            // Player.resetState(), whose own last act is
                            // `Shop.reset()` (../../../src/Player.java line
                            // 2576) -- so a brand-new character starts with
                            // every shop's own firstVisit flag set again.
                            otherState = dawnstar::OtherStateInfo::Reset();
                            shopState = dawnstar::ShopState::Reset();
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

            // M44 (Game Over) + M56 (Victory): reachable either from the
            // tick tail's EndOfGame result below, or from a type-42 kill
            // in the same tail (i.e. only once `playerSlot` holds a
            // live, playing character). The whole early-return shape
            // mirrors inMenu/inOptionsMenu above -- and, like those, it
            // also genuinely PAUSES the whole tick loop, matching the
            // original's own `activeScreen != null` branch
            // (tickPerSecond stops, so the ambush clock freezes while
            // the screen shows).
            if (inGameOver) {
                // secondaryParam==200/201's own dispatch has NO command
                // check (`else if (uic.secondaryParam == 200 ||
                // uic.secondaryParam == 201) { GenericInfoUI.
                // setSecondaryParam(399); ... }`) -- Ok is the only
                // command attached to the mode-4 Game Over/Victory
                // screen, but ANY command that ever arrives advances the
                // chain; 399's own `this.exit()` has no check either.
                // The full chain: Game Over/Victory -> "Exiting" (GenericInfoUI 399, the
                // concatenated ESGame.copyString notice, its own only
                // command swapped from Ok to Exit) -> exit.
                bool selectDown = KeyPressed(VK_RETURN);
                bool cancelDown = KeyPressed(VK_ESCAPE);
                bool anyCommand = (selectDown && !gameOverSelectKeyWasDown) ||
                                  (cancelDown && !gameOverCancelKeyWasDown);
                gameOverSelectKeyWasDown = selectDown;
                gameOverCancelKeyWasDown = cancelDown;

                if (anyCommand) {
                    if (!gameOverExiting) {
                        gameOverExitingScreen.SetupMessage("Exiting", dawnstar::CopyStringText());
                        gameOverExiting = true;
                    } else {
                        // secondaryParam==399's own dispatch: `this.exit();`
                        window.Close();
                    }
                }

                if (gameOverExiting) {
                    gameOverExitingScreen.Paint(backbuffer);
                } else {
                    gameOverScreen.Paint(backbuffer);
                }
                window.Present(backbuffer);
                return;
            }

            // M45/M46: only reachable once `playerSlot` holds a character
            // (inNpcMenu is only ever set true from inside the tick-gated
            // interact dispatch further below) -- see its own declaration
            // comment above.
            if (inNpcMenu) {
                bool upDown = KeyPressed(VK_UP);
                if (upDown && !npcMenuUpKeyWasDown) npcMenu.OnUp();
                npcMenuUpKeyWasDown = upDown;

                bool downDown = KeyPressed(VK_DOWN);
                if (downDown && !npcMenuDownKeyWasDown) npcMenu.OnDown();
                npcMenuDownKeyWasDown = downDown;

                dawnstar::NpcMenuContext npcCtx{*playerSlot, shopState, charData,          items,
                                                shopDialogue, levels,   world,             globalRng,
                                                nextDropSpawnId};

                bool cancelDown = KeyPressed(VK_ESCAPE);
                if (cancelDown && !npcMenuCancelKeyWasDown &&
                    npcMenu.OnCancel(npcCtx) == dawnstar::NpcMenuAction::ReturnToGame) {
                    inNpcMenu = false;
                }
                npcMenuCancelKeyWasDown = cancelDown;

                bool selectDown = KeyPressed(VK_RETURN);
                if (selectDown && !npcMenuSelectKeyWasDown &&
                    npcMenu.OnSelect(npcCtx) == dawnstar::NpcMenuAction::ReturnToGame) {
                    inNpcMenu = false;
                }
                npcMenuSelectKeyWasDown = selectDown;

                npcMenu.Render(backbuffer);
                window.Present(backbuffer);
                return;
            }

            // M49: only reachable once a character exists (opened from the
            // tick-gated block below).
            if (inLevelUp) {
                bool upDown = KeyPressed(VK_UP);
                if (upDown && !levelUpUpKeyWasDown) levelUpMenu.OnUp();
                levelUpUpKeyWasDown = upDown;

                bool downDown = KeyPressed(VK_DOWN);
                if (downDown && !levelUpDownKeyWasDown) levelUpMenu.OnDown();
                levelUpDownKeyWasDown = downDown;

                bool selectDown = KeyPressed(VK_RETURN);
                if (selectDown && !levelUpSelectKeyWasDown &&
                    levelUpMenu.OnSelect(*playerSlot, shopState, charData) == dawnstar::LevelUpAction::ReturnToGame) {
                    inLevelUp = false;
                }
                levelUpSelectKeyWasDown = selectDown;

                levelUpMenu.Render(backbuffer);
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
                    // M43: nextDropSpawnId is handed through so the Reveal
                    // Traitor quiz's correct-guess StarFrost award
                    // (PlayerInventory::GrantStarFrostItem) draws its spawn
                    // id from the same live Item.nextSpawnId()-stand-in
                    // counter every other item-granting call site here
                    // (combat's death drops) already uses.
                    switch (optionsMenu.OnSelect(optionsPlayer, charData, items, spells, levels, world,
                                                  nextDropSpawnId)) {
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
                        case dawnstar::OptionsMenuAction::SaveGame: {
                            // M42: ESGame.commandAction's own case 5 plus
                            // run()'s helperThreadState==5 branch. The
                            // original shows a fresh `new LoadingScreen(this,
                            // 10, 303)` and then runs saveGameState() on a
                            // background thread; this port runs it inline and
                            // presents the same bar at every reported percent
                            // (see GameSave::ProgressCallback's own doc
                            // comment on why that's the same observable frame
                            // sequence). The 0-percent frame is presented here,
                            // before the work starts, exactly as
                            // setCurrentDisplay(saveGameUI) does -- and
                            // saveGameState()'s own `percent = 0` is the one
                            // assignment the original never repaints for.
                            dawnstar::LoadingScreen saveGameUI(dawnstar::LoadingScreenMode::SavingGame);
                            saveGameUI.Render(backbuffer);
                            window.Present(backbuffer);
                            syncOtherStateFromLive();
                            bool saved = dawnstar::GameSave::SaveGameState(
                                saveDir, optionsPlayer, world, otherState, globalRng, [&](int percent) {
                                    saveGameUI.SetPercent(percent);
                                    saveGameUI.Render(backbuffer);
                                    window.Present(backbuffer);  // repaint() + serviceRepaints()
                                });
                            if (saved) {
                                // `this.setCurrentDisplay(this.gameCanvas)`
                                inOptionsMenu = false;
                            } else {
                                // run()'s own else-branch: GenericInfoUI
                                // secondaryParam 499, whose Ok exits.
                                optionsMenu.ShowSaveError();
                            }
                            break;
                        }
                        case dawnstar::OptionsMenuAction::LoadGame: {
                            // M42: case 6 (`System.gc()` +
                            // `gameCanvas.stopGameThread()` first -- this
                            // port's inOptionsMenu early-return already stops
                            // the whole tick) plus run()'s
                            // helperThreadState==6 branch.
                            dawnstar::LoadingScreen loadGameUI(dawnstar::LoadingScreenMode::LoadingGame);
                            loadGameUI.Render(backbuffer);
                            window.Present(backbuffer);
                            bool loaded = dawnstar::GameSave::LoadGameState(saveDir, optionsPlayer, world, otherState,
                                                                            [&](int percent) {
                                                                                loadGameUI.SetPercent(percent);
                                                                                loadGameUI.Render(backbuffer);
                                                                                window.Present(backbuffer);
                                                                            });
                            if (loaded) {
                                syncLiveFromOtherState();
                                // run()'s own tail: resumeGame(), then
                                // `loadGameUI.percent = 100` + one final
                                // repaint, then setCurrentDisplay(gameCanvas)
                                // + startGameThread(). The loadingDungeonID/
                                // imgloadRunning/reloadGame dance in between
                                // has no counterpart here -- see
                                // save/game_save.h's own class comment.
                                dawnstar::GameSave::ResumeGame(optionsPlayer, levels, world, [&](int percent) {
                                    loadGameUI.SetPercent(percent);
                                    loadGameUI.Render(backbuffer);
                                    window.Present(backbuffer);
                                });
                                loadGameUI.SetPercent(100);
                                loadGameUI.Render(backbuffer);
                                window.Present(backbuffer);
                                inOptionsMenu = false;
                            } else {
                                // run()'s own else-branch: noSavedGameUI,
                                // whose backTarget case 6 set to OptionsUI.
                                optionsMenu.ShowNoSavedGame();
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
            // M60: remapped from 'I' to 'E' -- "E to interact" is the
            // near-universal modern PC convention (Half-Life 2, Skyrim,
            // and most first-person games since). M62: moved again, off
            // 'E' (now turn-right, next to WASD) to 'R'.
            bool interactKeyDown = KeyPressed('R');
            if (interactKeyDown && !interactKeyWasDown && hotbarContext == 2) {
                interactPending = true;
            }
            interactKeyWasDown = interactKeyDown;

            // GameCanvas.keyPressed()'s own `key == 48` handler
            // (campRequested, gated on hotbarContext == 0) -- same shape
            // as interact above, just with the opposite hotbarContext
            // value (explore, not interact).
            // M60: remapped from 'Z' to 'R' -- "R to rest" is a common
            // modern RPG convention and reads more directly as "Rest"
            // than 'Z' ever did. M62: 'R' freed up for Interact instead
            // (see above), so Camp/Rest moves back to 'Z'.
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
            // numeric-keypad '7' to Tab for a PC keyboard (M60 first
            // used 'O', M61 moved it to Tab -- the common "open a menu
            // overlay" key in modern PC games) -- see `inOptionsMenu`'s
            // own doc comment above for what actually opening this menu
            // does.
            bool optionsKeyDown = KeyPressed(VK_TAB);
            if (optionsKeyDown && !optionsKeyWasDown) {
                optionsPending = true;
            }
            optionsKeyWasDown = optionsKeyDown;

            if (clock.ConsumeTick()) {
                // GameCanvas.run()'s own `now = System.
                // currentTimeMillis()`, sampled once per tick and reused
                // for every showMessage/timeout check below -- M30.
                int64_t nowMs = static_cast<int64_t>(GetTickCount64());

                // GameCanvas.run()'s own `elapsed = now - prevNow`, moved
                // up to be computed once per tick here (a further
                // simplification on the same "sampled once, reused
                // everywhere" precedent nowMs itself set above) and reused
                // both by the fatigue-regen call below (M51, inside
                // runTick) and by the tail's own TickStatusCountdowns
                // further down. NOT reproduced: the original actually
                // recomputes this at the very END of each iteration, so
                // processIdleTick's own tickFatigueRegen call reads the
                // PREVIOUS iteration's stale value there, one iteration
                // behind tickStatusCountdowns' fresh one that same
                // iteration -- an obscure quirk this port doesn't bother
                // reproducing, same reasoning as nowMs not reproducing
                // the original's own stale-`now` quirk for the
                // campState/deathState checks below.
                int64_t elapsedMs = nowMs - lastTickNowMs;
                lastTickNowMs = nowMs;

                // GameCanvas.run()'s own `actionTakenThisTick = false;`,
                // reset unconditionally every tick (M51) -- set true by
                // attack/spell-cast/move below, gating processIdleTick's
                // own tickFatigueRegen call further down.
                bool actionTakenThisTick = false;

                // refreshChestInSight() + refreshNpcInSight(), as one
                // step: the query halves live in PlayerMovement (see
                // ChestInFront's doc comment for why); the showMessage
                // halves are here. Run after a committed move (below) and
                // after a hub/camp warp (M47, sightRefreshPending).
                auto refreshSightings = [&] {
                    player.sightRefreshPending = false;
                    const std::array<uint8_t, 8>* chest =
                        dawnstar::PlayerMovement::ChestInFront(player, levels, world);
                    // M31: persisted for HotbarRenderer::
                    // ComputeHotbarContext (see player/player_state.h's
                    // own doc comment on why).
                    player.chestInSight = chest != nullptr;
                    if (chest != nullptr) {
                        dawnstar::MessagePopup::Show(messagePopup, {"Chest", ""}, 1, nowMs);
                    }

                    // RefreshNpcInSight itself only sets player.npcInSight
                    // (M28); the shop-greeting showMessage call is here.
                    dawnstar::PlayerMovement::RefreshNpcInSight(player, levels, world);
                    if (player.npcInSight >= 0) {
                        dawnstar::MessagePopup::Show(
                            messagePopup,
                            dawnstar::MessagePopup::WrapToTwoLines(
                                dawnstar::ShopInteraction::kNames[static_cast<size_t>(player.npcInSight)]),
                            1, nowMs);
                    }
                };
                // A warp that happened outside the move path (NPC-menu
                // Warp/Recovery, "Warp to Camp" item, Eustacia's warp).
                if (player.sightRefreshPending) refreshSightings();

                // Player.commitMove()'s own `Shop.showDeathGreeting = false`
                // reset on a forward/backward step -- M50. Same
                // one-tick-later, main.cpp-consumes-the-flag shape as
                // sightRefreshPending just above, and the same reason
                // (PlayerState::clearDeathGreetingPending's own doc
                // comment).
                if (player.clearDeathGreetingPending) {
                    player.clearDeathGreetingPending = false;
                    shopState.showDeathGreeting = false;
                }

                // GameCanvas.run()'s own per-tick campState 1/2/3 state
                // machine -- the block immediately preceding
                // dispatchTickActions() itself in the original. Returns
                // whether the rest of this tick's normal work should run
                // at all (false while actually camping, before the
                // relevant timer elapses); also sets suppressMoveThisTick
                // true on the one tick a camp cycle actually resolves --
                // see camp/camp_tick.h's own doc comment on both.
                bool suppressMoveThisTick = false;
                // Captured BEFORE TickCampState can change it: GameCanvas.
                // run()'s own if/else-if chain gives campState 1/2/3 total
                // priority over deathState for the tick -- if campState
                // already had something to do, deathState is never even
                // examined this tick, regardless of what campState ends up
                // AFTER TickCampState resolves it (e.g. camp 2 -> 0 on its
                // own resolving tick).
                int campStateBeforeTick = player.campState;
                bool runTick = dawnstar::CampTick::TickCampState(player, levels, world, items, monsters, globalRng,
                                                                  nowMs, campStartTimeMs, nextMonsterSpawnId,
                                                                  messagePopup, suppressMoveThisTick);
                // GameCanvas.run()'s own `else if (deathState != 1)` --
                // M50. Only reached when campState found nothing to do
                // (see campStateBeforeTick's own comment above); overrides
                // TickCampState's own fallthrough `return true` with
                // whatever the death state machine actually decides,
                // exactly matching the original's exclusive elseif shape.
                if (campStateBeforeTick == 0) {
                    runTick = dawnstar::DeathTick::TickDeathState(player, levels, world, items, shopState,
                                                                   messagePopup, deathTimeMs, nowMs,
                                                                   suppressMoveThisTick);
                }

                if (runTick) {
                    // GameCanvas.run()'s own `tickNearbyMonsters()` call
                    // -- the FIRST thing inside `if (runTick)` in the
                    // original, run every tick the player isn't actually
                    // camping: every monster on the player's own level
                    // within Manhattan distance 1-3 either attacks
                    // (distance 1) or takes a step toward the player
                    // (distance 2-3), at most once every 5 calls each --
                    // M36. `nextMonsterSpawnId` (M53) is the same
                    // Monster.nextSpawnIdCounter substitute camp/
                    // camp_tick.h's own TrySpawnMonsterNear call already
                    // advances -- see CombatTick::TickNearbyMonsters's
                    // own doc comment.
                    dawnstar::CombatTick::TickNearbyMonsters(player, levels, world, charData, items, monsters, nowMs,
                                                              globalRng, messagePopup, nextMonsterSpawnId);

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
                    //
                    // M60: Cast moved off 'S' (now "move backward") to
                    // 'F' (a common "use ability" key in modern action/
                    // RPG layouts); Attack moved off 'A' (now "strafe
                    // left") to Space (a common "primary action" key --
                    // conflict-free here, since the only other gameplay
                    // use of Space, skipping the boot splash, only ever
                    // runs before a character even exists).
                    bool castActive = KeyPressed('F');
                    bool attackActive = KeyPressed(VK_SPACE) && hotbarContext == 1;

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
                        // GameCanvas.openNpcDialogue(npcInSight): capture the
                        // shop id first -- a chest/no-target interact leaves
                        // npcInSight < 0 and opens nothing.
                        const int talkShopId = player.npcInSight;
                        std::optional<std::string> npcLine = dawnstar::InteractTick::ProcessInteract(
                            player, levels, world, items, charData, shopDialogue, shopState, messagePopup, globalRng,
                            nextDropSpawnId, nowMs);
                        if (talkShopId >= 0 && npcMenu.Open(talkShopId, npcLine, player, shopState)) {
                            inNpcMenu = true;
                        }
                        interactPending = false;
                    } else if (castActive) {
                        dawnstar::CombatTick::ProcessSpellCast(player, levels, world, monsters, items, charData,
                                                                spells, messagePopup, globalRng, nowMs,
                                                                lastSpellCastTimeMs, spellHitFlash, selfSpellFlash,
                                                                actionTakenThisTick);
                    } else if (spellCyclePending) {
                        dawnstar::CombatTick::CycleSpell(player, spells, messagePopup, nowMs);
                        spellCyclePending = false;
                    } else if (attackActive) {
                        if (dawnstar::CombatTick::ProcessAttack(player, levels, world, monsters, items, charData,
                                                                  globalRng, nowMs, lastAttackTimeMs,
                                                                  actionTakenThisTick)) {
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
                        //
                        // M60: modernized default keybinds. The arrow
                        // keys keep their own original behavior exactly
                        // (Up/Down step forward/backward, Left/Right
                        // turn in place) -- W/S mirror Up/Down, and A/D
                        // are a genuinely NEW capability this port never
                        // actually bound to any key before now:
                        // sidestep-strafing (`strafe=true`, direction
                        // 3/4 -- see PlayerMovement::Move's own doc
                        // comment), the original's real numeric-keypad
                        // '6'/'4' action that simply had no PC-keyboard
                        // equivalent wired up until this milestone. This
                        // is the WASD-strafe-plus-turn-keys layout most
                        // grid-based first-person dungeon crawlers played
                        // today use (Legend of Grimrock and its own
                        // genre-mates), rather than the original phone's
                        // turn-only D-pad.
                        //
                        // M62: user-reported discomfort turning via the
                        // Left/Right arrows while the rest of the hand
                        // sits on WASD -- Q/E (right next to WASD, no
                        // hand movement needed) now turn left/right too,
                        // alongside the arrow keys rather than replacing
                        // them (same "add, don't remove" precedent M60
                        // already set for W/S/A/D next to Up/Down/Left/
                        // Right).
                        moveAttempted = true;
                        int slotsBefore = player.inventoryCount;
                        if (KeyPressed(VK_UP) || KeyPressed('W')) {
                            dawnstar::PlayerMovement::Move(player, 1, false, levels, world, items);
                        } else if (KeyPressed(VK_DOWN) || KeyPressed('S')) {
                            dawnstar::PlayerMovement::Move(player, 2, false, levels, world, items);
                        } else if (KeyPressed(VK_RIGHT) || KeyPressed('E')) {
                            dawnstar::PlayerMovement::Move(player, 3, false, levels, world, items);
                        } else if (KeyPressed(VK_LEFT) || KeyPressed('Q')) {
                            dawnstar::PlayerMovement::Move(player, 4, false, levels, world, items);
                        } else if (KeyPressed('D')) {
                            dawnstar::PlayerMovement::Move(player, 3, true, levels, world, items);
                        } else if (KeyPressed('A')) {
                            dawnstar::PlayerMovement::Move(player, 4, true, levels, world, items);
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
                            // GameCanvas.commitMove()'s own
                            // `actionTakenThisTick = true;`, set
                            // unconditionally right here (whenever a move
                            // was actually requested this tick), BEFORE
                            // player.move() itself even runs -- so this
                            // is set regardless of whether the move goes
                            // on to actually commit a position change --
                            // M51.
                            actionTakenThisTick = true;
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
                            refreshSightings();

                            // commitMove()'s own unconditional
                            // `this.minimapDirty = true;` -- M29.
                            player.minimapDirty = true;
                        }
                    }

                    // dispatchTickActions()'s own tail:
                    // refreshTargetMonster() + resolveMonsterDeath(),
                    // unconditional every tick regardless of which
                    // action (if any) fired above -- M32. M56: a true
                    // return means this call just resolved the literal
                    // type-42 end-game monster's death --
                    // resolveMonsterDeath()'s own `this.game.endOfGameUI
                    // = this.game.newEndOfGameUI(); this.game.
                    // setCurrentDisplay(...)`. `newEndOfGameUI()`:
                    // Screen(4, 200) + setupMessage("Victory!",
                    // dialogue[9][74]+"\n"+[75]+"\n"+[76]) -- no <TAG>
                    // substitution needed here, unlike Game Over's
                    // traitor name. secondaryParam 200 and 201 share the
                    // EXACT SAME next dispatch branch in the original
                    // (`uic.secondaryParam == 200 || uic.secondaryParam
                    // == 201`), so this reuses the very same
                    // gameOverScreen/gameOverExiting state machine M44
                    // already built for the Game Over chain -- only the
                    // initial screen's title/text differ.
                    if (dawnstar::CombatTick::RefreshAndResolveTargetMonster(player, levels, world, monsters, items,
                                                                              messagePopup, globalRng, nowMs,
                                                                              nextDropSpawnId)) {
                        gameOverScreen.SetupMessage("Victory!", shopDialogue.groups[9][74] + "\n" +
                                                                     shopDialogue.groups[9][75] + "\n" +
                                                                     shopDialogue.groups[9][76]);
                        inGameOver = true;
                        gameOverExiting = false;
                    }

                    // GameCanvas.processIdleTick(): the `hp <= 0` half
                    // (M50) -- only ever reached while deathState == 1
                    // (this whole `if (runTick)` block is itself skipped
                    // the instant DeathTick::TickDeathState's own
                    // `deathState != 1` freeze kicks in, above), matching
                    // the original, which relies on the exact same
                    // runTick gating rather than an explicit deathState
                    // guard here.
                    if (dawnstar::PlayerCombatStats::EffectiveStat(player, charData, 2) <= 0) {
                        player.monsterTargeted = false;
                        player.deathState = 2;
                        deathTimeMs = nowMs;
                    }

                    // processIdleTick()'s other half -- `if
                    // (!actionTakenThisTick) tickFatigueRegen(elapsed);`
                    // -- M51. `actionTakenThisTick` (reset at the top of
                    // this tick) was set true above by a landed-or-missed
                    // attack, a resolved-or-not spell cast, or an
                    // attempted move; `elapsedMs` is this tick's own
                    // elapsed-time local (see its own doc comment on why
                    // this port doesn't reproduce the original's stale-
                    // by-one-iteration version of it here).
                    if (!actionTakenThisTick) {
                        dawnstar::PlayerCombatStats::TickFatigueRegen(player, elapsedMs);
                    }

                    // run()'s own `if (player.levelUpPending)`, right after
                    // processIdleTick and before tickVisibleObjects -- M49.
                    // The flag is consumed even if there is nothing to pick
                    // (see LevelUpMenu's doc comment on that null case).
                    if (player.levelUpPending) {
                        player.levelUpPending = false;
                        if (levelUpMenu.Open(player, charData)) {
                            inLevelUp = true;
                            // Enter is likely still down from whatever
                            // triggered this: don't let it pick at once.
                            levelUpSelectKeyWasDown = true;
                        }
                    }

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

                // GameCanvas.run()'s own per-iteration timed tail (lines
                // ~1407-1420): tickStatusCountdowns(elapsed), then the
                // secondAccum accumulation, then -- once past 1000ms --
                // tickPerSecond(). Runs on EVERY tick, outside runTick's
                // gate exactly like the original's own position outside
                // its `if (runTick)`, so the ambush clock (and the three
                // ailment countdowns) keep running while camping too.
                // `monsterAttacking` reads whatever the LAST render
                // step's paintVisibleObjects()-equivalent computed, the
                // same one-tick lag the campRequested dispatch above has
                // -- and the same lag the original's own repaint-then-
                // countdown ordering produces. `elapsedMs` itself is now
                // computed once at the top of the tick (M51) -- see that
                // declaration's own doc comment.
                dawnstar::PassiveTick::TickStatusCountdowns(player, elapsedMs, monsterAttacking);
                secondAccumMs += elapsedMs;
                if (secondAccumMs > 1000) {
                    secondAccumMs -= 1000;
                    // tickPerSecond()'s own ambush tail: a checkpoint
                    // monster spawned into a level already holding > 5
                    // monsters triggers ESGame's own `endOfGameUI =
                    // newGameOverUI()` + `setCurrentDisplay` -- the real
                    // "Game Over" chain (secondaryParam 201), played here
                    // by this port's ESGame stand-in exactly like the
                    // Options menu's other main.cpp-performed actions.
                    if (dawnstar::PassiveTick::TickPerSecond(player, levels, world, items, monsters, globalRng,
                                                              nextMonsterSpawnId, nowMs,
                                                              messagePopup) ==
                        dawnstar::PassiveTick::PerSecondResult::EndOfGame) {
                        // newGameOverUI(): Screen(4, 201) + setupMessage(
                        // "Game Over", Util.replace(dialogue[9][73],
                        // "<TAG>", Shop.NAMES[5 + traitorIndex])) -- the
                        // quiz's real traitor, same as the Clue Log's own
                        // suspect names.
                        gameOverScreen.SetupMessage(
                            "Game Over",
                            dawnstar::ReplaceFirstTag(
                                shopDialogue.groups[9][73], "<TAG>",
                                dawnstar::ShopInteraction::kNames[static_cast<size_t>(5 + player.traitorIndex)]));
                        inGameOver = true;
                        gameOverExiting = false;
                    }
                }
            }

            // GameCanvas.paint()'s own top-level branch: `deathState == 3`
            // (paintDeathScreen) outranks the campState check below it,
            // exactly like the original's own else-if chain -- M50. A
            // black screen plus "You're Dead!" centered in BIG_MESSAGE_FONT,
            // same invented-font reasoning as paintCampingScreen's own
            // "CAMPING" just below (see that block's doc comment).
            if (player.deathState == 3) {
                backbuffer.Fill(dawnstar::PackRGB565(0, 0, 0));
                const std::string deathText = "You're Dead!";
                int textX = (dawnstar::Backbuffer::kWidth - dawnstar::BitmapFont::StringWidth(deathText)) / 2;
                int textY = (dawnstar::Backbuffer::kHeight - dawnstar::BitmapFont::kGlyphHeight) / 2;
                dawnstar::BitmapFont::DrawString(backbuffer, textX, textY, deathText,
                                                  dawnstar::PackRGB565(255, 255, 255));
                window.Present(backbuffer);
                return;
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
                                             levels[static_cast<size_t>(player.currentLevel - 1)].number, player);
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
    } catch (const std::exception& e) {
        // No fallback rendering is possible without the extracted
        // assets -- surface a visibly distinct color (rather than the
        // placeholder's own dark blue) so a missing-assets failure
        // isn't mistaken for "the game is just idle". Logged too, when
        // there's a log file to put it in -- this is the one failure
        // shape a player pointing the launcher at the wrong .jar/folder
        // actually hits.
        std::printf("fatal: %s\n", e.what());
        std::fflush(stdout);
        backbuffer.Fill(dawnstar::PackRGB565(80, 0, 0));
        window.RunMessageLoop([&] { window.Present(backbuffer); });
    }

    return 0;
}
