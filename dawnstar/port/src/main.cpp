// M20: wires the real asset pipeline (M2-M8), the real 37-level
// procedurally-generated world (M6), a real created character (M11),
// player movement (M13), and the first-person corridor renderer
// (M9/M10) into the actual windowed app -- previously this just
// presented a solid-color placeholder backbuffer (see
// docs/PORT_ROADMAP.md's M20 entry). Arrow keys move/turn at the
// engine's fixed 250ms tick rate; there is no menu/character-creation
// UI yet, so this always starts a fixed class-0 character.
#include <windows.h>

#include <array>
#include <cstdlib>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/img_archive.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/monster_image_names.h"
#include "dungeon/dungeon_runtime.h"
#include "engine/game_clock.h"
#include "graphics/backbuffer.h"
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

    // Same default-relative-path convention every console smoke test
    // uses (see e.g. tests/m10_frame_render_smoke.cpp) -- this exe also
    // lands in build/, two levels above dawnstar/extracted/.
    const std::string root = "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsters = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
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
        // No character-creation UI exists yet (that's ESGame's own
        // screen-wiring loop, not yet ported) -- always start a fixed
        // class-0 character, same stand-in class index the M11 test
        // exercises first.
        dawnstar::PlayerState player =
            dawnstar::PlayerCreation::CreateCharacter(0, "Traveler", charData, items, globalRng);

        window.RunMessageLoop([&] {
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

            if (clock.ConsumeTick()) {
                // GameCanvas.run()'s own `now = System.
                // currentTimeMillis()`, sampled once per tick and reused
                // for every showMessage/timeout check below -- M30.
                int64_t nowMs = static_cast<int64_t>(GetTickCount64());

                // GameCanvas.run()'s own steady-250ms-tick cadence (see
                // engine/game_clock.h) is also when the real key state
                // would be sampled -- one Move() per tick while a key is
                // held reproduces that pacing rather than moving once
                // per PeekMessage-idle spin.
                bool moveAttempted = true;
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
                // pendingMoveDir != 0" gate -- called unconditionally
                // whenever a move was requested this tick, regardless of
                // whether it actually committed (matching the original,
                // which does all of this right after player.move() with
                // no success check).
                if (moveAttempted) {
                    // commitMove()'s own `int pickedUp = player.
                    // inventoryCount - slotsBefore;` -- diffed the same
                    // way here as there, rather than threading a count
                    // out of Move() itself -- M30.
                    int pickedUp = player.inventoryCount - slotsBefore;
                    if (pickedUp == 1) {
                        int slot = player.inventoryCount - 1;
                        int itemId = std::abs(static_cast<int>(player.inventoryItemIds[slot]));
                        dawnstar::MessagePopup::Show(
                            messagePopup, dawnstar::MessagePopup::WrapToTwoLines(items.name[static_cast<size_t>(itemId - 1)]),
                            -1, nowMs);
                    } else if (pickedUp > 1) {
                        dawnstar::MessagePopup::Show(messagePopup, {"Several", "items!"}, -1, nowMs);
                    }

                    // refreshChestInSight(): ChestInFront's own query
                    // half lives in PlayerMovement (see its own doc
                    // comment for why); the showMessage half is here.
                    const std::array<uint8_t, 8>* chest =
                        dawnstar::PlayerMovement::ChestInFront(player, levels, world);
                    // M31: persisted for HotbarRenderer::ComputeHotbarContext
                    // (see player/player_state.h's own doc comment on why).
                    player.chestInSight = chest != nullptr;
                    if (chest != nullptr) {
                        dawnstar::MessagePopup::Show(messagePopup, {"Chest", ""}, 1, nowMs);
                    }

                    // refreshNpcInSight(): same split as
                    // refreshChestInSight above -- RefreshNpcInSight
                    // itself only sets player.npcInSight (M28); the
                    // shop-greeting showMessage call is here.
                    dawnstar::PlayerMovement::RefreshNpcInSight(player, levels, world);
                    if (player.npcInSight >= 0) {
                        dawnstar::MessagePopup::Show(
                            messagePopup, dawnstar::MessagePopup::WrapToTwoLines(kShopNames[player.npcInSight]), 1,
                            nowMs);
                    }

                    // commitMove()'s own unconditional `this.minimapDirty
                    // = true;` -- M29.
                    player.minimapDirty = true;
                }

                // GameCanvas.run()'s own per-tick order: movement first,
                // then Player.tickVisibleObjects() (M25) -- unconditional
                // every tick, not just on a movement tick.
                dawnstar::VisibleObjects::Tick(player, levels, world);

                // run()'s own "if (minimapDirty) refreshMinimap()" gate,
                // right after tickVisibleObjects -- M29.
                if (player.minimapDirty) {
                    dawnstar::MinimapRenderer::Refresh(minimap, player, levels, world);
                }

                // run()'s own unconditional per-tick auto-hide timeout
                // check -- M30.
                dawnstar::MessagePopup::Tick(messagePopup, nowMs);
            }

            dawnstar::DungeonView view(levels, player.currentLevel - 1);
            dawnstar::FrameRenderer::Render(backbuffer, textures, view, player.tileX, player.tileY, player.facing,
                                             levels[static_cast<size_t>(player.currentLevel - 1)].number);
            dawnstar::VisibleObjectRenderer::Render(backbuffer, visibleObjectTextures, player.visibleObjects);
            // paintGameView()'s own "if (npcInSight >= 0)" gate, drawn
            // right after paintVisibleObjects and before
            // paintStatusBars -- M28.
            if (player.npcInSight >= 0) {
                dawnstar::VisibleObjectRenderer::PaintNpcPortrait(backbuffer, visibleObjectTextures,
                                                                    player.npcInSight);
            }
            dawnstar::HudRenderer::PaintStatusBars(backbuffer, player, charData);
            // paintGameView()'s own paintHotbar() call -- M31.
            // `monsterTargeted` is always false here: nothing in this
            // port sets it yet (see render/hotbar_renderer.h's own doc
            // comment).
            int hotbarContext =
                dawnstar::HotbarRenderer::ComputeHotbarContext(false, player.chestInSight, player.npcInSight);
            dawnstar::HotbarRenderer::Paint(backbuffer, hotbarTextures, hotbarContext);
            // paintGameView()'s own paintMessagePopup() call -- M30.
            dawnstar::MessagePopup::Paint(backbuffer, messagePopup);
            // paintGameView()'s own actual LAST drawing step (outside
            // its own try block, after paintMessagePopup/
            // paintActionFlashes/paintErrorOverlay -- the latter two
            // aren't ported) -- M29.
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
