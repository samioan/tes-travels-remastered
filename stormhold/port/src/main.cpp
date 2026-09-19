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
// Deliberately NOT wired here: paintFlashOverlays()/paintUnknown_b() (both
// still gated on live tick-loop state, see docs/PORT_ROADMAP.md's own
// "what's next"), the message popup (MessagePopup::Show has no reachable
// real call site yet either -- every one lives inside a still-
// untranscribed tick-loop helper), and the still-untranscribed tick-loop
// helpers themselves (rollCampInterrupted/tickMovementAndAI/setSomeFlag)
// -- so there is still no monster AI and no combat input yet. Turning
// (Move dir 3/4, no strafe) and stepping forward/backward are the only
// player actions this milestone wires.
#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "engine/game_clock.h"
#include "graphics/backbuffer.h"
#include "platform/win32/window.h"
#include "player/player_combat_stats.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/visible_objects.h"
#include "render/corridor_assets.h"
#include "render/game_renderer.h"
#include "render/hotbar_assets.h"
#include "render/hud_state.h"
#include "render/status_bar_plan.h"
#include "render/visible_object_assets.h"
#include "render/visible_object_renderer.h"
#include "world/dungeon_generator.h"
#include "world/warden.h"

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
    const std::string assetRootPath = ResolveAssetRoot();
    stormhold::AssetRoot assetRoot(assetRootPath);

    stormhold::ItemDatabase items = stormhold::ItemDatabase::Load(assetRoot);
    stormhold::MonsterDatabase monsters = stormhold::MonsterDatabase::Load(assetRoot);
    stormhold::DungeonGeometry geometry = stormhold::DungeonGeometry::Load(assetRoot);
    stormhold::CharacterData charData = stormhold::CharacterData::Load(assetRoot);

    stormhold::WorldRegistry world(37);
    std::vector<stormhold::GeneratedLevel> levels = BuildWorld(geometry, items, monsters, world);
    auto levelLookup = [&](int levelNumber) -> stormhold::GeneratedLevel& { return levels[static_cast<size_t>(levelNumber - 1)]; };

    stormhold::WardenState warden;
    stormhold::PlayerState player =
        stormhold::PlayerCreation::CreateCharacter(0, "Traveler", 1, charData, items);

    stormhold::CorridorAssets corridorAssets = stormhold::CorridorAssets::Load(assetRoot);
    stormhold::HotbarAssets hotbarAssets = stormhold::HotbarAssets::Load(assetRoot);
    stormhold::VisibleObjectAssets objectAssets = stormhold::VisibleObjectAssets::Load(assetRoot);

    stormhold::Window window(stormhold::Backbuffer::kWidth * 2, stormhold::Backbuffer::kHeight * 2,
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

    window.RunMessageLoop([&]() {
        if (clock.ConsumeTick()) {
            bool up = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
            bool down = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
            bool left = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
            bool right = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
            bool mDown = (GetAsyncKeyState('M') & 0x8000) != 0;

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
            int iconSet = stormhold::ResolveHudIconSet(stormhold::HudState{}, player, std::nullopt);
            stormhold::GameRenderer::RenderHud(backbuffer, hotbarAssets, iconSet);

            const stormhold::GeneratedLevel& currentLevel = levels[static_cast<size_t>(player.currentLevel - 1)];
            if (minimapZoomedOut) {
                stormhold::SquareViewGrid grid = stormhold::DungeonRuntime::SampleSquareView(
                    currentLevel, world, player.tileX, player.tileY, player.facing, 7, levelLookup);
                stormhold::GameRenderer::RenderMinimapZoomedOut(backbuffer, grid, player.facing);
            } else {
                stormhold::SquareViewGrid grid = stormhold::DungeonRuntime::SampleSquareView(
                    currentLevel, world, player.tileX, player.tileY, player.facing, 17, levelLookup);
                stormhold::GameRenderer::RenderMinimapNormal(backbuffer, grid, player.facing);
            }
        }
        window.Present(backbuffer);
    });

    return 0;
}
