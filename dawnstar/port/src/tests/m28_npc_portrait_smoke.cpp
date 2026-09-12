// M28 smoke test: NPC-in-sight detection (player/player_movement.h's
// NpcInFront/RefreshNpcInSight) and NPC portrait rendering
// (render/visible_object_renderer.h's PaintNpcPortrait) -- GameCanvas.
// npcInFront()/refreshNpcInSight()/paintNpcPortrait(), a completely
// separate mechanism from M25's visibleObjects (see
// visible_object_renderer.h's class comment). No JVM ground truth
// available (same reason as every prior milestone) -- verified against
// the real 37-level generated world (M6/M24) plus a real character
// (M11) for the detection half, and against the real extracted textures
// (M7/M10's own standard) with hand-derived expected draws (independently
// re-transcribed from GameCanvas.OBJECT_DRAW_TABLE/OBJECT_ICON_TABLE/
// OBJECT_EXTRA_FLAGS, not read back from visible_object_renderer.cpp)
// for the portrait half.
#include <array>
#include <cstdint>
#include <cstdio>
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
#include "graphics/backbuffer.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/player_state.h"
#include "render/visible_object_renderer.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::Backbuffer;
using dawnstar::DecodedImage;
using dawnstar::DungeonRuntime;
using dawnstar::GeneratedLevel;
using dawnstar::ItemDatabase;
using dawnstar::MonsterDatabase;
using dawnstar::PackRGB565;
using dawnstar::PlayerCreation;
using dawnstar::PlayerMovement;
using dawnstar::PlayerState;
using dawnstar::VisibleObjectRenderer;
using dawnstar::VisibleObjectTextures;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Same helper as m25_visible_objects_smoke.cpp's own FindApproach --
// finds a walkable tile adjacent to (targetX,targetY) the player can
// stand on and face toward the target so a forward offset of (0,1)
// lands exactly on it (see that file's doc comment for the full
// reasoning; duplicated here rather than shared, matching this
// project's established per-test-file self-containment).
bool FindApproach(const GeneratedLevel& level, int targetX, int targetY, int* standX, int* standY, int* facing) {
    struct Candidate {
        int dx, dy, f;
    };
    const Candidate candidates[4] = {
        {0, 1, 1},
        {0, -1, 3},
        {-1, 0, 2},
        {1, 0, 4},
    };
    for (const auto& c : candidates) {
        int sx = targetX + c.dx;
        int sy = targetY + c.dy;
        if (sx < 0 || sy < 0 || sx >= level.width || sy >= level.height) continue;
        uint8_t standTile = level.tiles[static_cast<size_t>(sx)][static_cast<size_t>(sy)];
        uint8_t targetTile = level.tiles[static_cast<size_t>(targetX)][static_cast<size_t>(targetY)];
        bool standWalkable = (standTile & (1 | 2 | 32)) == 0;
        bool targetNotWall = (targetTile & 1) == 0;
        if (standWalkable && targetNotWall) {
            *standX = sx;
            *standY = sy;
            *facing = c.f;
            return true;
        }
    }
    return false;
}

// Turns the player right-then-left (net facing unchanged) purely to
// drive a real RefreshCorridorView at its current position -- same
// "exercise the private call through the public API" approach M13/M19/
// M25's own tests use (RefreshCorridorView/Refresh are private).
void RefreshAt(PlayerState& p, std::vector<GeneratedLevel>& levels, WorldRegistry& world, const ItemDatabase& items) {
    int facingBefore = p.facing;
    PlayerMovement::Move(p, 3, false, levels, world, items);
    PlayerMovement::Move(p, 4, false, levels, world, items);
    Check(p.facing == facingBefore, "the turn-right-then-left round trip should leave facing unchanged");
}

// Same frame-aware pixel-sampling helpers as
// m27_object_at_position_smoke.cpp (PaintObjectAtPosition's sprites are
// often multi-frame sheets where only part of the image is visible).
bool FindFirstOpaquePixelInFrame(const DecodedImage& img, int frame, int frameCount, int* outX, int* outY) {
    int frameWidth = img.width / frameCount;
    int x0 = frame * frameWidth;
    int x1 = x0 + frameWidth;
    for (int y = 0; y < img.height; y++) {
        for (int x = x0; x < x1; x++) {
            if (img.A(x, y) != 0) {
                *outX = x;
                *outY = y;
                return true;
            }
        }
    }
    return false;
}

void CheckFrameAt(const Backbuffer& bb, int drawX, int drawY, const DecodedImage& img, int frame, int frameCount,
                   const char* label) {
    int sx = 0, sy = 0;
    bool found = FindFirstOpaquePixelInFrame(img, frame, frameCount, &sx, &sy);
    Check(found, "frame should have at least one opaque pixel to sample");
    if (!found) return;

    int frameWidth = img.width / frameCount;
    uint16_t expected = PackRGB565(img.R(sx, sy), img.G(sx, sy), img.B(sx, sy));
    int px = drawX + (sx - frame * frameWidth);
    int py = drawY + sy;
    uint16_t actual = bb.Data()[static_cast<size_t>(py) * Backbuffer::kWidth + px];
    if (actual != expected) {
        std::printf("  FAIL: %s: pixel at (%d,%d) expected 0x%04x, got 0x%04x\n", label, px, py, expected, actual);
        g_ok = false;
    }
}

Backbuffer RenderPortrait(const VisibleObjectTextures& textures, int shopId) {
    Backbuffer bb;
    bb.Fill(0);
    VisibleObjectRenderer::PaintNpcPortrait(bb, textures, shopId);
    return bb;
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);
        dawnstar::ImgArchive imageArchive(root + "/imgfiles.lmp");
        dawnstar::MonsterImageNames names = dawnstar::MonsterImageNames::Load(archive);
        VisibleObjectTextures textures = VisibleObjectTextures::Load(imageArchive, names);

        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                               monsterDb));
            DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }
        std::printf("generated + registered %zu levels\n", levels.size());

        // --- RefreshNpcInSight: standing right next to the hub's shop 0
        // (Shop.SHOP_X/Y[0] = (12,12)) should set npcInSight to 0 ---
        {
            const GeneratedLevel& hub = levels[0];
            int standX = 0, standY = 0, facing = 0;
            bool found = FindApproach(hub, 12, 12, &standX, &standY, &facing);
            Check(found, "should find a walkable approach tile next to the hub's shop 0");
            if (found) {
                dawnstar::JavaRandom rng(10);
                PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, rng);
                p.currentLevel = 1;
                p.tileX = standX;
                p.tileY = standY;
                p.facing = facing;
                RefreshAt(p, levels, world, items);

                PlayerMovement::RefreshNpcInSight(p, levels, world);
                Check(p.npcInSight == 0, "standing next to the hub's shop 0 should set npcInSight to 0");
            }
        }

        // --- RefreshNpcInSight: level 3's named shopkeeper
        // (GeneratedLevel::specialShopX/Y) should set npcInSight to 5 ---
        {
            const size_t kLevelIndex = 2;  // level 3
            const GeneratedLevel& level = levels[kLevelIndex];
            Check(level.specialShopX >= 0, "level 3 should have a real specialShopX/Y from generation");
            int standX = 0, standY = 0, facing = 0;
            bool found = level.specialShopX >= 0 &&
                         FindApproach(level, level.specialShopX, level.specialShopY, &standX, &standY, &facing);
            Check(found, "should find a walkable approach tile next to level 3's special shop room");
            if (found) {
                dawnstar::JavaRandom rng(11);
                PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, rng);
                p.currentLevel = 3;
                p.tileX = standX;
                p.tileY = standY;
                p.facing = facing;
                RefreshAt(p, levels, world, items);

                PlayerMovement::RefreshNpcInSight(p, levels, world);
                Check(p.npcInSight == 5, "standing next to level 3's named shopkeeper should set npcInSight to 5");
            }
        }

        // --- RefreshNpcInSight: standing somewhere in the open hub town
        // with no shop tile directly ahead should clear npcInSight ---
        {
            const GeneratedLevel& hub = levels[0];
            // The hub's own spawn tile, (9,9) facing north (see
            // PlayerCreation/PlayerMovement::ResetToHubPosition) -- not
            // adjacent to any of the 5 fixed peddler positions.
            dawnstar::JavaRandom rng(12);
            PlayerState p = PlayerCreation::CreateCharacter(0, "Tester", charData, items, rng);
            p.currentLevel = 1;
            p.tileX = 9;
            p.tileY = 9;
            p.facing = 1;
            RefreshAt(p, levels, world, items);

            uint8_t aheadTile = hub.tiles[static_cast<size_t>(p.tileX)][static_cast<size_t>(p.tileY - 1)];
            Check((aheadTile & 32) == 0, "the hub spawn's own forward tile should not be a shop tile (test premise)");

            PlayerMovement::RefreshNpcInSight(p, levels, world);
            Check(p.npcInSight == -1, "no shop tile ahead should leave npcInSight at -1");
        }

        // --- PaintNpcPortrait: shopId 0 -> (posCode 1, frameOverride 2)
        // -- bucket 0's row (same iconA/iconB/positions as
        // m27_object_at_position_smoke.cpp's own posCode-1 case), but
        // with iconB's frame OVERRIDDEN to 2 instead of
        // OBJECT_ICON_TABLE[0]'s own default of 0 -- exercising the one
        // behavior PaintObjectAtPosition's OWN smoke test never does
        // (it always passes frameOverride=-1). OBJECT_EXTRA_FLAGS[0] is
        // all-false, so no extras ---
        {
            Backbuffer bb = RenderPortrait(textures, 0);
            CheckFrameAt(bb, 43, 48, textures.objectSprites[0], 0, 2, "shopId 0 portrait iconA");
            CheckFrameAt(bb, 66, 55, textures.objectSprites[1], 2, 3, "shopId 0 portrait iconB (overridden frame 2)");
        }

        // --- PaintNpcPortrait: shopId 2 -> (posCode 7, frameOverride 0)
        // -- bucket 1's row (a DIFFERENT bucket than shopId 0 above),
        // iconB's frame overridden to 0 instead of OBJECT_ICON_TABLE[6]'s
        // own default of 1, plus a real extra0 decoration
        // (OBJECT_EXTRA_FLAGS[6] = {true,false,false,false}) ---
        {
            Backbuffer bb = RenderPortrait(textures, 2);
            CheckFrameAt(bb, 43, 49, textures.objectSprites[7], 1, 2, "shopId 2 portrait iconA");
            CheckFrameAt(bb, 61, 57, textures.objectSprites[8], 0, 3, "shopId 2 portrait iconB (overridden frame 0)");
            CheckFrameAt(bb, 46, 133, textures.objectSprites[11], 0, 1, "shopId 2 portrait extra0");
        }

        // --- PaintNpcPortrait: out-of-range shopId is a no-op (the
        // original switch has no default case either) ---
        {
            Backbuffer bb = RenderPortrait(textures, 9);
            bool anyPixel = false;
            for (int y = 0; y < Backbuffer::kHeight && !anyPixel; y++) {
                for (int x = 0; x < Backbuffer::kWidth; x++) {
                    if (bb.Data()[static_cast<size_t>(y) * Backbuffer::kWidth + x] != 0) {
                        anyPixel = true;
                        break;
                    }
                }
            }
            Check(!anyPixel, "an out-of-range shopId should draw nothing");
        }

        if (g_ok) {
            std::printf("all npc-in-sight/portrait checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 2;
    }
}
