// M20: wires the real asset pipeline (M2-M8), the real 37-level
// procedurally-generated world (M6), a real created character (M11),
// player movement (M13), and the first-person corridor renderer
// (M9/M10) into the actual windowed app -- previously this just
// presented a solid-color placeholder backbuffer (see
// docs/PORT_ROADMAP.md's M20 entry). Arrow keys move/turn at the
// engine's fixed 250ms tick rate; there is no menu/character-creation
// UI yet, so this always starts a fixed class-0 character.
#include <windows.h>

#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/img_archive.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "dungeon/dungeon_runtime.h"
#include "engine/game_clock.h"
#include "graphics/backbuffer.h"
#include "platform/win32/window.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/player_state.h"
#include "render/frame_renderer.h"
#include "render/hud_renderer.h"
#include "util/java_random.h"
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

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    dawnstar::Window window(dawnstar::Backbuffer::kWidth * 2, dawnstar::Backbuffer::kHeight * 2,
                             L"Dawnstar Port");
    dawnstar::Backbuffer backbuffer;
    dawnstar::GameClock clock;

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
            if (clock.ConsumeTick()) {
                // GameCanvas.run()'s own steady-250ms-tick cadence (see
                // engine/game_clock.h) is also when the real key state
                // would be sampled -- one Move() per tick while a key is
                // held reproduces that pacing rather than moving once
                // per PeekMessage-idle spin.
                if (KeyPressed(VK_UP)) {
                    dawnstar::PlayerMovement::Move(player, 1, false, levels, world, items);
                } else if (KeyPressed(VK_DOWN)) {
                    dawnstar::PlayerMovement::Move(player, 2, false, levels, world, items);
                } else if (KeyPressed(VK_RIGHT)) {
                    dawnstar::PlayerMovement::Move(player, 3, false, levels, world, items);
                } else if (KeyPressed(VK_LEFT)) {
                    dawnstar::PlayerMovement::Move(player, 4, false, levels, world, items);
                }
            }

            dawnstar::DungeonView view(levels, player.currentLevel - 1);
            dawnstar::FrameRenderer::Render(backbuffer, textures, view, player.tileX, player.tileY, player.facing,
                                             levels[static_cast<size_t>(player.currentLevel - 1)].number);
            dawnstar::HudRenderer::PaintStatusBars(backbuffer, player, charData);
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
