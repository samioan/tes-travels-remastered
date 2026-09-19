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
#include <windows.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/shop_dialogue.h"
#include "combat/combat_resolution.h"
#include "dungeon/dungeon_runtime.h"
#include "engine/game_clock.h"
#include "graphics/backbuffer.h"
#include "monster/monster_runtime.h"
#include "platform/win32/window.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/visible_objects.h"
#include "render/corridor_assets.h"
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

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    const std::string userDir = ResolveUserDir();
    OpenLogFile(userDir);
    const int scale = ResolveScale();

    const std::string assetRootPath = ResolveAssetRoot();
    stormhold::AssetRoot assetRoot(assetRootPath);

    stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assetRoot);
    stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assetRoot);
    stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assetRoot);
    stormhold::CharacterData charData = stormhold::CharacterData::Load(assetRoot);
    stormhold::ShopDialogue dialogue = stormhold::ShopDialogue::Load(assetRoot);

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
            if (KeyEdge(VK_RETURN)) stormhold::MenuFlow::Confirm(menuState, charData, items);
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
            if (menuState.draft.has_value()) player = std::move(*menuState.draft);
            stormhold::PlayerMovement::RefreshCorridorView(player, levelLookup(player.currentLevel), levelLookup);
            gameStarted = true;
        }

        if (clock.ConsumeTick()) {
            bool up = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
            bool down = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
            bool left = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
            bool right = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
            bool mDown = (GetAsyncKeyState('M') & 0x8000) != 0;
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) attackRequested = true;

            if (up) {
                stormhold::PlayerMovement::Move(player, 1, false, levelLookup, world, items, monsters, warden);
            } else if (down) {
                stormhold::PlayerMovement::Move(player, 2, false, levelLookup, world, items, monsters, warden);
            } else if (left) {
                stormhold::PlayerMovement::Move(player, 3, false, levelLookup, world, items, monsters, warden);
            } else if (right) {
                stormhold::PlayerMovement::Move(player, 4, false, levelLookup, world, items, monsters, warden);
            }

            if (mDown && !mKeyWasDown) minimapZoomedOut = !minimapZoomedOut;
            mKeyWasDown = mDown;

            gameTimeMs += stormhold::GameClock::kTickInterval.count();
            stormhold::GeneratedLevel& currentLevelMutable = levelLookup(player.currentLevel);

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
            // m()) -- currently unreachable in practice, since nothing
            // yet lets the player actually damage targetMonster (no
            // combat input is wired -- see this file's own header
            // comment), but wired anyway, matching this port's own
            // "wire the confirmed mechanic even ahead of its own
            // trigger" precedent (M35-M37).
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
                stormhold::CombatResolution::ResolveAttackInput(player, targetMonster, attackRequested, gameTimeMs,
                                                                  lastAttackTimeMs, charData, items, monsters,
                                                                  combatRng, world);
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
        }
        window.Present(backbuffer);
    });

    return 0;
}
