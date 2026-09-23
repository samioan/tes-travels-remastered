// M34: wire the real pipeline into the actual windowed stormhold_port.exe.
// Until now main.cpp was still M1's placeholder: a solid-color
// Backbuffer::Fill and an empty tick, with every real milestone (M2-M33)
// only ever exercised through console smoke tests. This milestone makes
// the windowed exe itself do something: load the real extracted assets,
// build the real 37-level world (M6's DungeonGenerator + M18's
// RegisterGeneratedSpawns), create a real class-0 character (M9's
// PlayerCreation -- there's no character-creation UI yet, so the class is
// a fixed stand-in), and on every GameClock tick (M1's real 250ms cadence)
// read arrow-key state (GetAsyncKeyState, polled once per tick so a held
// key advances once per tick rather than as fast as the message pump
// spins) into PlayerMovement::Move (M17/M19/M25), then render the
// player's live position/facing through every already-verified pixel
// pipeline this port has (M25's corridor view, M28's object/monster
// sprites off M27's VisibleObjects, M26's status bars, M31's hotbar, and
// M33's minimap, toggled between its two zoom levels with 'M', off M32's
// SampleSquareView) and present it through the existing GDI
// Window::Present. Same scope as dawnstar's own M20 -- no new gameplay
// logic ported here, pure wiring of already-verified pieces, following
// dawnstar's own precedent for this exact milestone number/shape.
//
// M35 layers in the first tick-loop helper, `PlayerCombatStats::
// TickStatusCountdowns` (GameCanvas.tickStatusCountdowns(), was a
// throw-stub until this session): 3 ailment-timer countdowns, called
// once per tick ahead of this frame's own render pass, using whatever
// the PREVIOUS frame's `RenderMonsters` returned (see
// `monsterRenderedLastFrame`'s own comment below) for its own
// real, confirmed paint/tick coupling. `RenderCorridorView`'s own
// ailment3/4 flags are also now real (`PlayerCombatStats::HasAilment`)
// instead of hardcoded false -- currently a no-op either way, since
// nothing yet sets any ailment bit (Monster.tick() itself still has no
// wired caller), but no longer a placeholder.
//
// M36 layers in the second tick-loop helper, `PlayerCombatStats::
// TickPerSecond` (GameCanvas.tickPerSecond(), also a throw-stub until
// this session): 3 more per-real-second mechanics (see its own header
// comment). Accumulated with a plain `secondAccumulatorMs`, matching
// GameCanvas.run()'s own "secondAccumulator += deltaMs; if (>1000)
// {-=1000; tickPerSecond();}" pattern exactly, just against
// `GameClock::kTickInterval` (a fixed 250ms) instead of a real
// wall-clock delta -- this port's own ticks are already fixed-interval,
// so the two are equivalent here.
//
// M37 layers in `CombatResolution::TickMonstersOnLevel`
// (GameCanvas.tickMonsterAI(), was a throw-stub -- see its own header
// comment for the full writeup): real monster AI, finally closing the
// long-flagged "Monster.tick()/chase() have no wired caller" gap
// (docs/ROADMAP.md, open since phase-3 M14/M15). Also the first real
// call site for `MessagePopup` (M30) -- the "Creature attacks!" popup
// this method's own return value triggers is now actually shown and
// painted. Called ahead of `VisibleObjects::Refresh`, matching
// `run()`'s own real relative order for this piece specifically; the
// exact relative order against `TickStatusCountdowns`/`TickPerSecond`
// below (which run AFTER `refreshVisibleObjectsAndMinimap()` and this
// tick's own repaint in the real `run()`) is NOT reproduced exactly --
// a deliberate simplification, since nothing yet reads state in a way
// that would make the difference observable.
//
// M38 layers in `PlayerMovement::MonsterInFront` (Player.
// monsterInFront(), new this session) plus the inline "refresh
// targetMonster" / "resolve targetMonster death" orchestration
// GameCanvas.refreshTargetMonster()/resolveTargetMonsterDeath() (were
// decompiled/e.java's a()/m(), also new this session -- see
// ../src/GameCanvas.java's own header comments) describe. NEITHER of
// those two Java methods has a reachable caller in the real game yet
// either (their own real caller, tickMovementAndAI/e(long), remains a
// stub) -- wired directly into this file's own tick loop anyway, same
// as M35-M37's own precedent of wiring a confirmed-real mechanic ahead
// of its still-stubbed original dispatcher when nothing else depends on
// the exact original call ordering. This is also the second real call
// site for `MessagePopup` (the "Found <Name>"/"Creature is dead!"
// popups) and gives `render/hud_state.h`'s `HudState`/`TargetMonsterInfo`
// (M29) their own first live values instead of a fixed all-false/
// nullopt stand-in.
//
// M39 layers in `CombatResolution::ResolveAttackInput`
// (GameCanvas.resolveAttackInput(), was decompiled/e.java's d(long),
// also new this session) -- the player's own attack input, finally
// giving the player a way to actually swing at `targetMonster`. Bound
// to a new SPACE key (polled the same way as the arrow keys -- the
// original's own real gate, `unconfirmed_av`/key '1' with
// `hotbarActionSet == 1`, isn't reproduced, since `hotbarActionSet`
// itself is out of scope -- input handling in general, same gap M29/
// M31 already flagged). `ResolveAttackInput`'s own internal 500ms
// cooldown means holding SPACE auto-repeats at that rate, same as
// holding an arrow key auto-repeats movement once per tick.
//
// Deliberately NOT wired here: paintFlashOverlays()/paintUnknown_b()
// (both still gated on live tick-loop state, see docs/PORT_ROADMAP.md's
// own "what's next"), and the still-untranscribed tick-loop helpers
// (rollCampInterrupted is transcribed but has no reachable caller worth
// wiring without the camp system around it; tickMovementAndAI/the real
// per-tick action dispatcher) -- so there is still no camp system and
// no rank-up/level-up flow. Turning (Move dir 3/4, no strafe), stepping
// forward/backward, and now attacking are the only player actions this
// milestone wires.
//
// M40 layers in `MenuFlow` (ui/menu_flow.h, new this session) --
// src/UIScreen.java + src/ESGame.java's own commandAction() screenGroups
// 2 (main menu)/3-6 (class select/confirm/info/character-created)/7+101
// (welcome/intro/into-gameplay)/305 (no-saved-game), fused into one
// small state machine (see menu_flow.h's own class comment for exactly
// what's modeled vs. simplified). Until now this file hardcoded a
// class-0 "Traveler" character and jumped straight into the live tick
// loop on launch (M34's own placeholder, called out explicitly in that
// milestone's header comment as "there's no character-creation UI
// yet") -- this milestone replaces that with the real flow: Main Menu,
// then New Game -> class select -> class confirm (with a real "See
// Class Info" summary) -> character created -> enter a name (typed via
// GetAsyncKeyState on 'A'-'Z'/'0'-'9'/Backspace, the same polling-based
// input model every other key in this file already uses -- no WM_CHAR
// plumbing added to platform/win32/window.cpp for this) -> welcome ->
// intro, before finally handing off into the same tick/render pipeline
// M34-M39 already built. The live gameplay loop itself is completely
// unchanged; only what runs BEFORE it changed.
//
// M42 layers in `Camping`/`CampState` (player/camp_state.h, new this
// session) -- GameCanvas's own campState/campRollAt fields and run()'s
// already-Java-transcribed camp state machine (2.5s roll-for-
// interruption, then 5s safe wait), finally with a real trigger
// (GameCanvas.startCampOrRest(), M41) and a real driver
// (Camping::Tick, this session). Bound to a new 'C' key, the same
// pragmatic stand-in M39 used for attack (the real trigger, hotbar key
// '0', stays out of scope). While camping, movement/attack/target-
// monster refresh/monster-AI are all skipped for the tick (matching
// run()'s own shouldRunTick==false gate) -- everything else
// (TickStatusCountdowns/TickPerSecond/VisibleObjects::Refresh/
// rendering) keeps running unconditionally, same as the original.
// `PlayerCombatStats::ApplyRestRecovery` and `DungeonRuntime::
// SpawnAmbushMonsterNearPlayer` (both new this session) are the two
// Player/Dungeon methods it drives. Deliberately NOT rendered as its
// own "CAMPING" screen (`GameCanvas.paintCampScreen()`, already real
// Java since long before this milestone) -- same "primitive/logic
// first, pixels later" discipline M21/M27/M29 already used; the corridor
// view just keeps rendering underneath for now.
//
// M43 layers in chest detection/interaction: new `PlayerMovement::
// ChestAheadOfPlayer`/`PlayerInventory::CollectChestItem` (both new this
// session, mirroring `MonsterInFront`'s own established shape) port
// `GameCanvas.checkChestAhead()`'s "Chest" popup and
// `resolveInteractInput()`'s chest-opening half (M41) -- NOT its NPC-talk
// half, which still needs `Shop.questShopAt()`/`talkToNpc()`, both
// unported. Bound to a new 'F' key. Also wires the OTHER half of
// `refreshNpcNameplateAndWardenLeave()` (M41): the Warden-leaving
// trigger, via the already-ported `WardenState::Leave` (M8) and
// `DungeonRuntime::ViewGridAt` (M21) against the player's own
// `corridorView` -- the NPC-nameplate half (gated on the SAME
// `Shop.questShopAt()` gap) stays unported too, flagged at its own
// omission point below rather than silently dropped.
//
// M45 layers in `FlashOverlay`/`FlashOverlayState` (render/flash_
// overlay.h, new this session) -- GameCanvas.paintFlashOverlays()'s 3
// one-shot flash sprites (M22's own confirmed Java transcription; this
// milestone only ports the C++ body). Only the monster-hit flash
// (`FlashOverlayState::hit`) has a live trigger anywhere in this port:
// `CombatResolution::ResolveAttackInput`'s own return value (M39),
// previously discarded here, now sets it. The spell-hit-monster/
// self-spell-hit flashes paint for real but stay unreachable until
// spell casting itself is wired (see docs/PORT_ROADMAP.md's own "what's
// next").
//
// M47 layers in `DeathSequence`/`DeathState` (player/death_sequence.h,
// new this session) -- GameCanvas.tickDeathAndRegen() plus run()'s own
// already-Java-transcribed `facing != 1` death/respawn state machine,
// finally with a real driver (DeathSequence::Tick, this session), the
// same "primitive ported long ago, wired now" shape M42's Camping gave
// the camp state machine. HP reaching 0 (via CombatResolution::
// TickMonstersOnLevel, M37) now actually does something instead of
// silently going negative forever. While DeathSequence::Tick returns
// Waiting, movement/attack/spellcast/target-monster-refresh/monster-AI
// are all skipped for the tick (same shouldRunTick gate Camping::Tick
// already shares this block with) -- everything else keeps running
// unconditionally, same as the original. New `assets/dungeon_names.h`
// (Dungeon.loadNames()/displayNames(), never ported before now) backs
// the real respawn-location message.
//
// M48 layers in `MovementMessages` (player/movement_messages.h, new this
// session) -- GameCanvas.resolveMovementSideEffects(), the message-popup
// layer real movement should show (confirmed real and flagged as
// unwired by M47's own "what's next" note): a level-crossing message
// (reusing DungeonNames, M47) and a "Found <item>!"/"Several items!"
// message off the same before/after inventoryCount comparison M17
// already wires through DungeonRuntime.
#include <windows.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/dungeon_geometry.h"
#include "assets/dungeon_names.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/shop_dialogue.h"
#include "assets/spell_database.h"
#include "combat/combat_resolution.h"
#include "combat/spell_casting.h"
#include "dungeon/dungeon_runtime.h"
#include "engine/game_clock.h"
#include "graphics/backbuffer.h"
#include "monster/monster_runtime.h"
#include "platform/win32/window.h"
#include "player/camp_state.h"
#include "player/death_sequence.h"
#include "player/game_save.h"
#include "player/movement_messages.h"
#include "player/player_combat_stats.h"
#include "player/player_inventory.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/visible_objects.h"
#include "render/corridor_assets.h"
#include "render/flash_overlay.h"
#include "render/game_renderer.h"
#include "render/hotbar_assets.h"
#include "render/hud_state.h"
#include "render/message_popup.h"
#include "render/status_bar_plan.h"
#include "render/visible_object_assets.h"
#include "render/visible_object_renderer.h"
#include "ui/menu_flow.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"
#include "world/warden.h"

#include <cstdlib>
#include <filesystem>

namespace {

// argv[1], if given, overrides the asset root; falls back to the
// repo-relative default every smoke test and a plain dev build from
// build/ already uses (build/../../extracted). __argc/__wargv are
// populated by the CRT for a wWinMain entry point same as argc/argv
// would be for main().
std::string ResolveAssetRoot() {
    if (__argc > 1 && __wargv && __wargv[1] && __wargv[1][0] != L'\0') {
        int size = WideCharToMultiByte(CP_UTF8, 0, __wargv[1], -1, nullptr, 0, nullptr, nullptr);
        std::string narrow(static_cast<size_t>(size - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, __wargv[1], -1, narrow.data(), size, nullptr, nullptr);
        return narrow;
    }
    return "../../extracted";
}

// STORMHOLD_USER_DIR (set by the launcher to <install>/user, mirroring
// dawnstar's own DAWNSTAR_USER_DIR/shadowkey-decomp's SK_USER_DIR) is
// where the log file lives. Unset -- a plain dev build -- stays exactly
// where it's always been: no log file at all.
std::string ResolveUserDir() {
    char* value = nullptr;
    size_t size = 0;
    // _dupenv_s rather than std::getenv: MSVC's own /W4 flags getenv as
    // C4996 (not thread-safe against a concurrent SetEnvironmentVariable),
    // and this project holds a real zero-/W4-warnings bar (see
    // docs/PORT_ROADMAP.md's own "Decisions carried through every
    // milestone").
    if (_dupenv_s(&value, &size, "STORMHOLD_USER_DIR") != 0 || !value) return std::string();
    std::string result(value);
    free(value);
    return result;
}

// STORMHOLD_SCALE (set by the launcher from its own window-size picker,
// mirroring DAWNSTAR_SCALE/SK_SCALE) overrides the window's integer scale
// over the native 176x208 backbuffer. Unset, or garbage, keeps the *2 this
// file hardcoded before the launcher existed.
int ResolveScale() {
    char* value = nullptr;
    size_t size = 0;
    if (_dupenv_s(&value, &size, "STORMHOLD_SCALE") != 0 || !value) return 2;
    int scale = std::atoi(value);
    free(value);
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    return scale;
}

// Redirects stdout/stderr into <userDir>/stormhold_port.log so a bug
// report has something to attach -- this is a WINAPI-subsystem app with no
// console to print to otherwise. A no-op (same as running with no launcher
// at all) when userDir is empty.
void OpenLogFile(const std::string& userDir) {
    if (userDir.empty()) return;
    std::error_code error;
    std::filesystem::create_directories(userDir, error);
    const std::string path = (std::filesystem::path(userDir) / "stormhold_port.log").string();
    FILE* unused = nullptr;
    freopen_s(&unused, path.c_str(), "a", stdout);
    freopen_s(&unused, path.c_str(), "a", stderr);
    std::printf("--- stormhold_port starting ---\n");
    std::fflush(stdout);
}

// Builds all 37 levels (M6's DungeonGenerator: BuildHubLevel for the hub,
// PopulateLevel for the 36 standard levels, same pattern m18_registered_
// spawns_smoke.cpp already exercises against the real geometry table) and
// registers every one's generated monster/chest spawns into `world` (M18's
// RegisterGeneratedSpawns). Indexed levelNumber-1, matching WorldRegistry's
// own vector convention.
std::vector<stormhold::GeneratedLevel> BuildWorld(const stormhold::DungeonGeometry& geometry,
                                                   const stormhold::ItemDatabase& items,
                                                   const stormhold::MonsterDatabase& monsters,
                                                   stormhold::WorldRegistry& world) {
    std::vector<stormhold::GeneratedLevel> levels;
    levels.reserve(37);
    levels.push_back(stormhold::DungeonGenerator::BuildHubLevel(geometry.rows[0]));
    for (int levelNumber = 2; levelNumber <= 37; levelNumber++) {
        levels.push_back(
            stormhold::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[levelNumber - 1], items, monsters));
    }
    for (const stormhold::GeneratedLevel& level : levels) {
        stormhold::DungeonRuntime::RegisterGeneratedSpawns(level, world, monsters);
    }
    return levels;
}

// M50: port-only, no real Dungeon.java counterpart. This port eagerly
// builds/registers EVERY level up front (BuildWorld, above) rather than
// the original's own lazy per-level `populate()` -- so unlike a REAL
// `refreshTileFlagsFromRegistries()` call (always a level's very FIRST
// population, onto all-zero transient bits), calling
// `DungeonRuntime::RefreshTileFlags` after a `GameSave::Load` would OR
// the loaded registries' bits on top of whatever the throwaway freshly-
// generated world already left set -- a chest/monster/dropped-item that
// isn't in the save (e.g. a chest already looted before that save was
// written) would wrongly stay flagged forever. `RefreshTileFlags` itself
// is left untouched -- still a faithful, purely-additive port of the
// real method (see its own doc comment and M16's own smoke test, which
// only ever calls it against an all-zero level, same as the original
// always does) -- this just clears exactly the 3 bits it ever sets
// (2/4/16) first, so the load path gets the same "OR onto zero" result
// the original always has.
void ClearTransientTileFlags(stormhold::GeneratedLevel& level) {
    for (auto& column : level.tiles) {
        for (uint8_t& tile : column) {
            tile = static_cast<uint8_t>(tile & ~(2 | 4 | 16));
        }
    }
}

// GameCanvas.itemFoundMessageLines() (was decompiled/e.java's `k()`, M41)
// -- splits an item's display name into the message popup's 2 lines: the
// first 2 words merged onto line 1 when there are 3+ words (line 2 gets
// just the 3rd, matching the original's own "only ever looks at the
// first 3 words" shape exactly, never a 4th+), else one word per line.
std::array<std::string, 2> ItemFoundMessageLines(const std::string& name) {
    std::vector<std::string> words;
    size_t start = 0;
    while (start <= name.size()) {
        size_t space = name.find(' ', start);
        if (space == std::string::npos) {
            words.push_back(name.substr(start));
            break;
        }
        words.push_back(name.substr(start, space - start));
        start = space + 1;
    }

    std::array<std::string, 2> lines{"", ""};
    if (words.size() >= 3) {
        lines[0] = words[0] + " " + words[1];
        lines[1] = words[2];
    } else {
        for (size_t i = 0; i < words.size() && i < 2; i++) lines[i] = words[i];
    }
    return lines;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    const std::string userDir = ResolveUserDir();
    OpenLogFile(userDir);
    const int scale = ResolveScale();
    // M50: GameSave's own file, next to the log file -- empty (same
    // no-launcher-no-persistence convention OpenLogFile above already
    // uses) when userDir itself is empty, so GameSave::Exists/Save/Load
    // all degrade to harmless no-ops for a plain dev build.
    const std::string savePath =
        userDir.empty() ? std::string() : (std::filesystem::path(userDir) / "savegame.dat").string();

    const std::string assetRootPath = ResolveAssetRoot();
    stormhold::AssetRoot assetRoot(assetRootPath);

    stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assetRoot);
    stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assetRoot);
    stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assetRoot);
    stormhold::CharacterData charData = stormhold::CharacterData::Load(assetRoot);
    stormhold::ShopDialogue dialogue = stormhold::ShopDialogue::Load(assetRoot);
    // M46: combat/spell_casting.h's own SpellCasting::CastOnSelf/
    // CastOnMonster/CycleSelectedSpell.
    stormhold::SpellDatabase spells = stormhold::SpellDatabase::Load(assetRoot);
    // M47: player/death_sequence.h's own respawn-message lookup.
    stormhold::DungeonNames dungeonNames = stormhold::DungeonNames::Load(assetRoot);

    stormhold::WorldRegistry world(37);
    std::vector<stormhold::GeneratedLevel> levels = BuildWorld(geometry, items, monsters, world);
    auto levelLookup = [&](int levelNumber) -> stormhold::GeneratedLevel& { return levels[static_cast<size_t>(levelNumber - 1)]; };

    stormhold::WardenState warden;
    // M40: no longer created up front -- MenuFlowState::draft holds the
    // in-progress character through the whole Main Menu/new-game flow;
    // `player` itself is only given real content once that flow reaches
    // MenuScreen::Finished (see the transition block inside the idle
    // callback below). Default-constructed here is safe: nothing reads
    // `player` before that transition runs.
    stormhold::PlayerState player;
    stormhold::MenuFlowState menuState;
    bool gameStarted = false;

    stormhold::CorridorAssets corridorAssets = stormhold::CorridorAssets::Load(assetRoot);
    stormhold::HotbarAssets hotbarAssets = stormhold::HotbarAssets::Load(assetRoot);
    stormhold::VisibleObjectAssets objectAssets = stormhold::VisibleObjectAssets::Load(assetRoot);
    stormhold::FlashOverlayAssets flashOverlayAssets = stormhold::FlashOverlayAssets::Load(assetRoot);
    stormhold::FlashOverlayState flashOverlay;

    stormhold::Window window(stormhold::Backbuffer::kWidth * scale, stormhold::Backbuffer::kHeight * scale,
                              L"Stormhold Port");
    stormhold::GameClock clock;
    stormhold::Backbuffer backbuffer;

    bool minimapZoomedOut = true;
    bool mKeyWasDown = false;
    // M35: GameCanvas's own unconfirmed_A -- whether paintMonsters() drew
    // a real monster sprite, carried over from the PREVIOUS tick's own
    // RenderMonsters call (see PlayerCombatStats::TickStatusCountdowns's
    // own doc comment: this tick's tickStatusCountdowns reads whatever
    // the last repaint set, not this tick's own not-yet-run one).
    bool monsterRenderedLastFrame = false;
    int64_t secondAccumulatorMs = 0;
    int64_t gameTimeMs = 0;
    // M37: JavaRandom seeds are arbitrary here -- this port has no
    // persisted ESGame-wide RNG session state yet (same class of gap
    // M6/M9 already flagged for dungeon generation/character creation).
    stormhold::JavaRandom combatRng(1);
    stormhold::JavaRandom ambushRng(2);
    // Deliberately far above anything M6/M18's own generation-time
    // spawnId counters would ever reach across 37 levels, avoiding a
    // collision with a REAL registered monster's own spawnId -- same
    // reasoning player/player_creation.h's own spawnId=1 stand-in
    // documents for its own local counter. Shared by ambush spawns
    // (M37) AND M38's own death-drop spawnId, same "one shared counter"
    // reasoning ESGame.nextSpawnId() itself documents (see player/
    // player_creation.h's own class comment).
    int16_t nextSpawnIdCounter = 10000;
    // M42: GameCanvas's own campState/campRollAt, finally with a real
    // trigger ('C' key, below) and a real Tick() driving run()'s own
    // already-transcribed camp state machine.
    stormhold::CampState camp;
    // M47: GameCanvas's own unconfirmed_s -- finally with a real driver
    // (DeathSequence::Tick, below).
    stormhold::DeathState death;
    stormhold::MessagePopupState messagePopup;
    // M38: GameCanvas.targetMonster -- refreshed every tick by
    // PlayerMovement::MonsterInFront below.
    std::optional<stormhold::MonsterState> targetMonster;
    // M38: GameCanvas's own UI-flag statics resolveHudIconSet() reads
    // (M29) -- unconfirmedAa now gets its first real live value
    // (whether targetMonster is currently set); the other 3 stay false,
    // still no reachable setter for any of them.
    stormhold::HudState hudState;
    // M39: GameCanvas.unconfirmed_av's own port-side stand-in --
    // ResolveAttackInput's own job to clear it again either way.
    bool attackRequested = false;
    int64_t lastAttackTimeMs = 0;
    // M46: GameCanvas.unconfirmed_ap/unconfirmed_U's own port-side stand-
    // ins -- SpellCasting::ResolveSpellCastInput/ResolveSpellCycleInput's
    // own job to clear each again either way. Unlike attackRequested
    // above, keys '3'/'5' (below) are the REAL, unconditional key codes,
    // not pragmatic stand-ins -- see combat/spell_casting.h's own
    // ResolveSpellCastInput doc comment.
    bool spellCastRequested = false;
    bool spellCycleRequested = false;
    int64_t lastSpellCastTimeMs = 0;

    // M40: edge-triggered key state for the Main Menu/new-game flow --
    // one MenuFlow action per physical keypress rather than once per
    // idle-callback poll, since a held key has no "auto-repeat" meaning
    // in a menu the way it does for movement/attack below. Shared
    // across every virtual-key code the menu ever reads (navigation,
    // Ok/Cancel, and every letter/digit EnterName accepts).
    std::array<bool, 256> keyDownLast{};
    auto KeyEdge = [&](int vk) {
        bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        bool edge = down && !keyDownLast[static_cast<size_t>(vk)];
        keyDownLast[static_cast<size_t>(vk)] = down;
        return edge;
    };

    window.RunMessageLoop([&]() {
        if (menuState.screen != stormhold::MenuScreen::Finished) {
            if (KeyEdge(VK_UP)) stormhold::MenuFlow::MoveSelection(menuState, -1, charData);
            if (KeyEdge(VK_DOWN)) stormhold::MenuFlow::MoveSelection(menuState, 1, charData);
            if (KeyEdge(VK_RETURN)) stormhold::MenuFlow::Confirm(menuState, charData, items, savePath);
            if (KeyEdge(VK_ESCAPE)) stormhold::MenuFlow::Cancel(menuState);

            if (menuState.screen == stormhold::MenuScreen::EnterName) {
                // No WM_CHAR plumbing added to platform/win32/window.h
                // for this -- GetAsyncKeyState('A'..'Z'/'0'..'9') reads
                // those exact keys directly, same polling-based input
                // model as every other key in this file, and sufficient
                // since graphics/bitmap_font.h only ever renders
                // uppercase letters and digits anyway.
                for (int vk = 'A'; vk <= 'Z'; vk++) {
                    if (KeyEdge(vk)) stormhold::MenuFlow::TypeChar(menuState, static_cast<char>(vk));
                }
                for (int vk = '0'; vk <= '9'; vk++) {
                    if (KeyEdge(vk)) stormhold::MenuFlow::TypeChar(menuState, static_cast<char>(vk));
                }
                if (KeyEdge(VK_BACK)) stormhold::MenuFlow::Backspace(menuState);
            }

            backbuffer.Fill(stormhold::PackRGB565(0, 0, 0));
            stormhold::MenuFlow::Render(backbuffer, menuState, charData, dialogue);
            window.Present(backbuffer);
            if (menuState.exitRequested) window.RequestClose();
            return;
        }

        if (!gameStarted) {
            // commandAction()'s own screenGroup==101 branch: gameCanvas.
            // player=player -- the draft built across the whole New Game
            // flow finally becomes the live `player` right here, once.
            if (menuState.draft.has_value()) {
                player = std::move(*menuState.draft);
            } else if (menuState.loadRequested) {
                // M50: ESGame's own helperThreadState==6 branch
                // (loadGameState() then player.refreshCorridorView()) --
                // GameSave::Exists already confirmed the file is there
                // (MenuFlow::Confirm, ui/menu_flow.cpp) before `screen`
                // was even allowed to reach Finished, so a Load failure
                // here would mean the file vanished/corrupted between
                // those two moments; `player`/`world` are simply left at
                // their already-valid default-constructed state either
                // way, same fallback the real `noSavedGameUI` branch
                // effectively gives (this port has nowhere left to show
                // that screen from once `Finished` is reached).
                if (stormhold::GameSave::Load(savePath, levels.size(), player, world)) {
                    for (stormhold::GeneratedLevel& level : levels) {
                        ClearTransientTileFlags(level);
                        stormhold::DungeonRuntime::RefreshTileFlags(level, world);
                    }
                }
            }
            stormhold::PlayerMovement::RefreshCorridorView(player, levelLookup(player.currentLevel), levelLookup);
            gameStarted = true;
        }

        if (clock.ConsumeTick()) {
            bool up = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
            bool down = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
            bool left = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
            bool right = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
            bool mDown = (GetAsyncKeyState('M') & 0x8000) != 0;
            bool campKeyEdge = KeyEdge('C');
            bool interactKeyEdge = KeyEdge('F');
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) attackRequested = true;
            // M46: real key codes ('3'/'5'), not stand-ins -- see
            // spellCastRequested/spellCycleRequested's own comment above.
            if (KeyEdge('3')) spellCastRequested = true;
            if (KeyEdge('5')) spellCycleRequested = true;

            gameTimeMs += stormhold::GameClock::kTickInterval.count();
            stormhold::GeneratedLevel& currentLevelMutable = levelLookup(player.currentLevel);

            // M42: GameCanvas.tickPlayerAction()'s own unconfirmed_I
            // branch, gated there on unconfirmed_A -- "Cannot Camp!"
            // instead of starting a rest when a monster is currently
            // visible (monsterRenderedLastFrame, same flag
            // TickStatusCountdowns's own ailment-7 coupling already
            // reads) -- else GameCanvas.startCampOrRest() (M41, was
            // decompiled/e.java's a(long)). Bound to a new 'C' key, the
            // same pragmatic "no hotbar system exists" stand-in M39
            // already used for attack (real key: '0' when
            // hotbarActionSet==0, out of scope -- see M29/M31's own
            // input-handling gap).
            if (campKeyEdge && camp.state == 0) {
                if (monsterRenderedLastFrame) {
                    stormhold::MessagePopup::Show(messagePopup, {"Cannot", "Camp!"}, 1, gameTimeMs);
                } else {
                    stormhold::Camping::Start(camp, player, gameTimeMs);
                }
            }

            // M42: run()'s own campState==1/2 handling (Java-
            // transcribed long before this port's tick loop existed) --
            // CampState::Tick returns StillWaiting while camping/
            // waiting, matching run()'s own shouldRunTick==false gate
            // below (movement/attack/monster-AI are the only things
            // that gate on it -- TickStatusCountdowns/TickPerSecond/
            // VisibleObjects::Refresh/rendering all run unconditionally
            // every tick in the original, same as here).
            stormhold::CampTickResult campResult =
                stormhold::Camping::Tick(camp, player, currentLevelMutable, world, items, monsters, combatRng,
                                          nextSpawnIdCounter, gameTimeMs);
            if (campResult == stormhold::CampTickResult::Disturbed) {
                stormhold::MessagePopup::Show(messagePopup, {"Rest", "disturbed!"}, 1, gameTimeMs);
            } else if (campResult == stormhold::CampTickResult::Complete) {
                stormhold::MessagePopup::Show(messagePopup, {"Rest", "complete!"}, 1, gameTimeMs);
            }

            // M47: run()'s own `facing != 1` dispatch (Java-transcribed
            // long before this port's tick loop existed) -- DeathSequence
            // ::Tick returns Waiting while dead/waiting to respawn,
            // matching run()'s own shouldRunTick==false gate below the
            // exact same way CampTickResult::StillWaiting already does
            // (both AND together into one shouldRunTick, harmless since
            // real gameplay can't reach both at once -- see death_
            // sequence.h's own DeathState comment).
            stormhold::DeathTickResult deathResult = stormhold::DeathSequence::Tick(player, death, items, gameTimeMs);
            if (deathResult == stormhold::DeathTickResult::Respawned) {
                stormhold::MessagePopup::Show(
                    messagePopup, stormhold::DeathSequence::RespawnMessageLines(player, dungeonNames), 1, gameTimeMs);
            }

            bool shouldRunTick = campResult != stormhold::CampTickResult::StillWaiting &&
                                  deathResult != stormhold::DeathTickResult::Waiting;
            if (shouldRunTick) {
                bool moveKeyPressed = up || down || left || right;
                int8_t inventoryCountBeforeMove = player.inventoryCount;
                if (up) {
                    stormhold::PlayerMovement::Move(player, 1, false, levelLookup, world, items, monsters, warden);
                } else if (down) {
                    stormhold::PlayerMovement::Move(player, 2, false, levelLookup, world, items, monsters, warden);
                } else if (left) {
                    stormhold::PlayerMovement::Move(player, 3, false, levelLookup, world, items, monsters, warden);
                } else if (right) {
                    stormhold::PlayerMovement::Move(player, 4, false, levelLookup, world, items, monsters, warden);
                }

                // M48: GameCanvas.resolveMovementSideEffects() (was
                // decompiled/e.java's `n()`) -- see player/
                // movement_messages.h's own header comment for why both
                // messages below can fire from the same move, and for
                // `lockedItemEndOfGame`'s own not-modeled end-of-game gap.
                if (moveKeyPressed) {
                    stormhold::MovementMessageResult moveMsg = stormhold::MovementMessages::Resolve(
                        player, inventoryCountBeforeMove, dungeonNames, items);
                    if (moveMsg.crossingMessage.has_value()) {
                        stormhold::MessagePopup::Show(messagePopup, *moveMsg.crossingMessage, 1, gameTimeMs);
                    }
                    if (moveMsg.itemsFound == stormhold::ItemsFoundKind::One) {
                        stormhold::MessagePopup::Show(messagePopup, ItemFoundMessageLines(moveMsg.foundItemName), -1,
                                                        gameTimeMs);
                    } else if (moveMsg.itemsFound == stormhold::ItemsFoundKind::Several) {
                        stormhold::MessagePopup::Show(messagePopup, {"Several", "items!"}, -1, gameTimeMs);
                    }
                }

                // M43: GameCanvas.checkChestAhead() (was decompiled/
                // e.java's `h()` no-arg, M41): polls the look-ahead tile
                // for a chest every tick, showing "Chest" the moment one
                // comes into view.
                std::optional<std::array<int8_t, 8>> chestAhead =
                    stormhold::PlayerMovement::ChestAheadOfPlayer(player, levelLookup, world);
                if (chestAhead.has_value()) {
                    stormhold::MessagePopup::Show(messagePopup, {"Chest", ""}, 1, gameTimeMs);
                }

                // M43: GameCanvas.resolveInteractInput()'s chest-opening
                // half (M41, was decompiled/e.java's `f(long)`) -- its
                // NPC-talk half (Player.shopAheadOfPlayer()>=0) is NOT
                // wired here, see this file's own header comment. Bound
                // to a new 'F' key, the same "no hotbar system exists"
                // stand-in pattern as 'C' for camp/rest above.
                // collectChestItem()'s own confirmed-unreachable `-1`
                // ("locked") branch (M41's own finding) isn't reproduced
                // here either -- CollectChestItem can only return 0 or 1.
                if (interactKeyEdge && chestAhead.has_value()) {
                    int result = stormhold::PlayerInventory::CollectChestItem(player, *chestAhead, items,
                                                                                currentLevelMutable, world);
                    if (result == 0) {
                        stormhold::MessagePopup::Show(messagePopup, {"Inventory", "full!"}, -1, gameTimeMs);
                    } else {
                        int8_t newItemId = player.inventoryItemIds[static_cast<size_t>(player.inventoryCount - 1)];
                        std::string name = items.name[static_cast<size_t>(std::abs(newItemId) - 1)];
                        stormhold::MessagePopup::Show(messagePopup, ItemFoundMessageLines(name), -1, gameTimeMs);
                    }
                }

                // M43: GameCanvas.refreshNpcNameplateAndWardenLeave()'s
                // Warden-leaving half (M41, was decompiled/e.java's `c()`
                // no-arg) -- the OTHER half (an NPC-nameplate popup when
                // the tile ahead IS a shop tile) needs
                // Player.shopAheadOfPlayer()/Shop.questShopAt(), not
                // wired here either, see this file's own header comment.
                // WardenState::Leave (M8) finally gets a real caller.
                {
                    uint8_t tileAhead = stormhold::DungeonRuntime::ViewGridAt(player.corridorView, 0, 1);
                    if (!(tileAhead & 0x20)) {  // bit 32 clear -- not looking at a shop tile
                        std::optional<stormhold::TargetMonsterInfo> npcCheckTarget;
                        if (targetMonster.has_value()) {
                            npcCheckTarget = stormhold::TargetMonsterInfo{targetMonster->tileX, targetMonster->tileY,
                                                                            targetMonster->typeIndex};
                        }
                        bool dialogueDue = stormhold::IsNpcDialogueDue(player, npcCheckTarget);
                        if (!dialogueDue && warden.present && player.wardenLoreStep == warden.visitCount) {
                            warden.Leave(levelLookup(1), levelLookup(2));
                        }
                    }
                }

                // M38: GameCanvas.refreshTargetMonster() (was e.java's a()).
                targetMonster = stormhold::PlayerMovement::MonsterInFront(player, levelLookup, world);
                hudState.unconfirmedAa = targetMonster.has_value();
                if (targetMonster.has_value()) {
                    std::string name = monsters.TypeName(targetMonster->typeIndex);
                    size_t spaceAt = name.find(' ');
                    std::array<std::string, 2> lines =
                        (spaceAt == std::string::npos)
                            ? std::array<std::string, 2>{name, ""}
                            : std::array<std::string, 2>{name.substr(0, spaceAt), name.substr(spaceAt + 1)};
                    stormhold::MessagePopup::Show(messagePopup, lines, 1, gameTimeMs);
                }

                // M38: GameCanvas.resolveTargetMonsterDeath() (was e.java's
                // m()).
                if (targetMonster.has_value() && targetMonster->currentHp <= 0) {
                    bool guaranteedDrop = (targetMonster->typeIndex == 41);
                    stormhold::MonsterRuntime::DeathDrop drop = stormhold::MonsterRuntime::OnDeath(
                        *targetMonster, monsters, items, currentLevelMutable.tier, guaranteedDrop, nextSpawnIdCounter,
                        combatRng);
                    if (drop.dropped) {
                        std::array<int8_t, 7> record;
                        for (size_t i = 0; i < record.size(); i++) record[i] = static_cast<int8_t>(drop.record[i]);
                        stormhold::DungeonRuntime::AddDroppedItem(currentLevelMutable, world, record);
                    }
                    stormhold::DungeonRuntime::RemoveMonster(currentLevelMutable, world, targetMonster->spawnId);
                    if (stormhold::PlayerCombatStats::HasAilment(player, 4)) {
                        int heal = 3 * player.coreStats[3] / 10;
                        player.coreStats[2] = static_cast<int16_t>(
                            std::min<int>(player.coreStats[2] + heal, player.coreStats[3]));
                    }
                    stormhold::MessagePopup::Show(messagePopup, {"Creature", "is dead!"}, 1, gameTimeMs);
                    targetMonster = std::nullopt;
                    hudState.unconfirmedAa = false;
                }

                // M39: GameCanvas.resolveAttackInput() (was e.java's
                // d(long)) -- see this file's own header comment for the
                // input-binding simplification.
                if (attackRequested) {
                    // M45: the previously-discarded return value now
                    // drives FlashOverlayState::hit (paintFlashOverlays()'s
                    // own unconfirmed_S, see this file's own header
                    // comment).
                    if (stormhold::CombatResolution::ResolveAttackInput(player, targetMonster, attackRequested,
                                                                          gameTimeMs, lastAttackTimeMs, charData,
                                                                          items, monsters, combatRng, world)) {
                        flashOverlay.hit = true;
                    }
                }

                // M46: GameCanvas.resolveSpellCastInput() (was e.java's
                // h(long)) -- see combat/spell_casting.h's own header
                // comment for the two flash-overlay flags this finally
                // gives a real trigger (M45's own "what's next" note).
                if (spellCastRequested) {
                    stormhold::SpellCasting::Result castResult = stormhold::SpellCasting::ResolveSpellCastInput(
                        player, targetMonster, spellCastRequested, gameTimeMs, lastSpellCastTimeMs, spells, charData,
                        items, monsters, combatRng, world);
                    if (castResult == stormhold::SpellCasting::Result::NotEnoughMagicka) {
                        stormhold::MessagePopup::Show(messagePopup, {"Not enough", "magic!"}, 3, gameTimeMs);
                    } else if (castResult == stormhold::SpellCasting::Result::NoMonster) {
                        stormhold::MessagePopup::Show(messagePopup, {"No monster", "here!"}, 1, gameTimeMs);
                    } else if (castResult == stormhold::SpellCasting::Result::CastOnMonster) {
                        flashOverlay.spellHitMonster = true;
                    } else if (castResult == stormhold::SpellCasting::Result::CastOnSelf) {
                        flashOverlay.spellHitSelf = true;
                    }
                    // InvalidSpell/OnCooldown/NotRequested: no message in
                    // the original either (InvalidSpell only prints a
                    // debug line, not reproduced -- see combat/
                    // spell_casting.h's own Result::InvalidSpell comment).
                }

                // M46: GameCanvas.resolveSpellCycleInput() (was e.java's
                // g(long)).
                if (spellCycleRequested) {
                    int newSpellId =
                        stormhold::SpellCasting::ResolveSpellCycleInput(player, spellCycleRequested, spells);
                    if (newSpellId == 0) {
                        stormhold::MessagePopup::Show(messagePopup, {"No spells!", ""}, -1, gameTimeMs);
                    } else {
                        // Reuses ItemFoundMessageLines' own word-split
                        // algorithm -- confirmed IDENTICAL to
                        // resolveSpellCycleInput()'s own inlined copy of
                        // itemFoundMessageLines()'s split (both: >=3 words
                        // -> first two words joined on line 1, 3rd word
                        // alone on line 2; else one word per line), so this
                        // port reuses the one already-confirmed helper
                        // rather than duplicating it a 3rd time, unlike the
                        // original's own two separate near-identical
                        // copies (see combat/spell_casting.h's own
                        // ResolveSpellCycleInput doc comment).
                        stormhold::MessagePopup::Show(messagePopup, ItemFoundMessageLines(spells.ById(newSpellId).name),
                                                        -1, gameTimeMs);
                    }
                }

                // M37: GameCanvas.run()'s own this.tickMonsterAI(frameStart)
                // call (see this file's own header comment on the relative-
                // order simplification). Only the player's OWN current level
                // ever ticks -- matching the real ESGame.G[]-indexed read
                // exactly, not an invented simplification.
                bool showAttackMessage = stormhold::CombatResolution::TickMonstersOnLevel(
                    world, currentLevelMutable, player, charData, items, monsters, levels, gameTimeMs, combatRng,
                    ambushRng, nextSpawnIdCounter);
                if (showAttackMessage) {
                    stormhold::MessagePopup::Show(messagePopup, {"Creature", "attacks!"}, 2, gameTimeMs);
                }

                // M47: GameCanvas.tickDeathAndRegen() (was a throw-stub) --
                // see player/death_sequence.h's own header comment for why
                // this runs last, matching run()'s own real call order
                // relative to tickPlayerAction/tickMonsterAI. A true
                // return means the player JUST died this tick -- clears
                // targetMonster the same way GameCanvas's own
                // unconfirmed_aa reset does (this port's own render/HUD-
                // state boundary, see DeathSequence::TickDeathAndRegen's
                // own comment).
                if (stormhold::DeathSequence::TickDeathAndRegen(
                        player, death, charData, gameTimeMs,
                        static_cast<int64_t>(stormhold::GameClock::kTickInterval.count()))) {
                    targetMonster = std::nullopt;
                    hudState.unconfirmedAa = false;
                }
            }

            if (mDown && !mKeyWasDown) minimapZoomedOut = !minimapZoomedOut;
            mKeyWasDown = mDown;

            stormhold::MessagePopup::Tick(messagePopup, gameTimeMs);

            // M35: GameCanvas.run()'s own this.c(deltaMs) call, ahead of
            // this tick's own repaint (see monsterRenderedLastFrame's own
            // comment above for why that arrives one frame stale here,
            // faithfully).
            stormhold::PlayerCombatStats::TickStatusCountdowns(
                player, static_cast<int64_t>(stormhold::GameClock::kTickInterval.count()), monsterRenderedLastFrame);

            // M36: GameCanvas.run()'s own secondAccumulator gate (see
            // this file's own header comment).
            secondAccumulatorMs += stormhold::GameClock::kTickInterval.count();
            if (secondAccumulatorMs > 1000) {
                secondAccumulatorMs -= 1000;
                stormhold::PlayerCombatStats::TickPerSecond(player, items);
            }

            // M27: no live tick loop yet refreshes this automatically (see
            // this file's own header comment) -- refreshed unconditionally
            // every tick here instead, harmless since nothing else in this
            // milestone mutates monster/chest/dropped-item state between
            // ticks.
            stormhold::VisibleObjects::Refresh(player, world, /*includeWarden=*/false, warden);

            backbuffer.Fill(stormhold::PackRGB565(0, 0, 0));
            stormhold::GameRenderer::RenderCorridorView(
                backbuffer, corridorAssets, player.corridorView,
                /*ailment3Active=*/stormhold::PlayerCombatStats::HasAilment(player, 3),
                /*ailment4Active=*/stormhold::PlayerCombatStats::HasAilment(player, 4));
            stormhold::VisibleObjectRenderer::RenderObjects(backbuffer, objectAssets, player);
            monsterRenderedLastFrame = stormhold::VisibleObjectRenderer::RenderMonsters(backbuffer, objectAssets, player);
            stormhold::GameRenderer::RenderStatusBars(backbuffer, stormhold::StatusBarPlan::Plan(player, charData));
            std::optional<stormhold::TargetMonsterInfo> targetMonsterInfo;
            if (targetMonster.has_value()) {
                targetMonsterInfo = stormhold::TargetMonsterInfo{targetMonster->tileX, targetMonster->tileY,
                                                                  targetMonster->typeIndex};
            }
            int iconSet = stormhold::ResolveHudIconSet(hudState, player, targetMonsterInfo);
            stormhold::GameRenderer::RenderHud(backbuffer, hotbarAssets, iconSet);

            if (minimapZoomedOut) {
                stormhold::SquareViewGrid grid = stormhold::DungeonRuntime::SampleSquareView(
                    currentLevelMutable, world, player.tileX, player.tileY, player.facing, 7, levelLookup);
                stormhold::GameRenderer::RenderMinimapZoomedOut(backbuffer, grid, player.facing);
            } else {
                stormhold::SquareViewGrid grid = stormhold::DungeonRuntime::SampleSquareView(
                    currentLevelMutable, world, player.tileX, player.tileY, player.facing, 17, levelLookup);
                stormhold::GameRenderer::RenderMinimapNormal(backbuffer, grid, player.facing);
            }
            stormhold::MessagePopup::Paint(backbuffer, messagePopup);
            // M45: paintGameView()'s own call order has this directly
            // after paintMessagePopup() (see this file's own header
            // comment) -- combatRng reused for the jitter, see
            // FlashOverlay's own header comment on why that's not a new
            // RNG-stream divergence.
            stormhold::FlashOverlay::Paint(backbuffer, flashOverlay, flashOverlayAssets, combatRng);
        }
        window.Present(backbuffer);
    });

    return 0;
}
