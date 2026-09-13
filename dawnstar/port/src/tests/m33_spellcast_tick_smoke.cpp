// M33 smoke test: CombatTick::ProcessSpellCast/CycleSpell (combat/
// combat_tick.h) -- GameCanvas.processSpellCast()/cycleSelectedSpell(),
// the player-initiated spellcasting actions. No JVM ground truth is
// available (same reason as every prior milestone) -- verified against
// the real 37-level generated world (M6/M24), a real created character
// (M11), and the real spellsin.dat SpellDatabase (M16), with
// CastOnMonster/CastOnSelf's own probabilistic roll NOT forced to a
// specific outcome (that underlying math was already verified against
// real data in M16) -- this test is only about the NEW tick-orchestration
// wiring around it: the invalid-id/magicka/cooldown gate order, the
// offensive-vs-self dispatch, the message text/priority at each gate,
// and the two flash flags.
#include <cstdio>
#include <string>
#include <vector>

#include "assets/character_data.h"
#include "assets/dat_archive.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/monster_database.h"
#include "assets/spell_database.h"
#include "combat/combat_tick.h"
#include "dungeon/dungeon_runtime.h"
#include "monster/monster_runtime.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/player_state.h"
#include "render/message_popup.h"
#include "util/java_random.h"
#include "world/dungeon_generator.h"

namespace {

using dawnstar::CombatTick;
using dawnstar::GeneratedLevel;
using dawnstar::MessagePopupState;
using dawnstar::MonsterRuntime;
using dawnstar::MonsterState;
using dawnstar::PlayerCreation;
using dawnstar::PlayerMovement;
using dawnstar::PlayerState;
using dawnstar::SpellDatabase;
using dawnstar::WorldRegistry;

bool g_ok = true;

void Check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
}

// Same helper as m25/m28/m32's own FindApproach -- finds a walkable tile
// adjacent to (targetX,targetY) the player can stand on and face toward
// the target so a forward step lands exactly on it (duplicated per this
// project's established per-test-file self-containment convention).
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

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        dawnstar::DatArchive archive(root + "/datfiles.lmp");
        dawnstar::ItemDatabase items = dawnstar::ItemDatabase::Load(archive);
        dawnstar::MonsterDatabase monsterDb = dawnstar::MonsterDatabase::Load(archive);
        dawnstar::CharacterData charData = dawnstar::CharacterData::Load(archive);
        SpellDatabase spells = SpellDatabase::Load(archive);
        dawnstar::DungeonGeometry geometry = dawnstar::DungeonGeometry::Load(archive);

        std::vector<GeneratedLevel> levels;
        WorldRegistry world(geometry.rows.size());
        for (size_t i = 0; i < geometry.rows.size(); i++) {
            int levelNumber = static_cast<int>(i) + 1;
            levels.push_back(levelNumber == 1
                                  ? dawnstar::DungeonGenerator::BuildHubLevel(geometry.rows[0])
                                  : dawnstar::DungeonGenerator::PopulateLevel(levelNumber, geometry.rows[i], items,
                                                                               monsterDb));
            dawnstar::DungeonRuntime::RegisterGeneratedSpawns(levels.back(), world);
        }
        std::printf("generated + registered %zu levels\n", levels.size());

        // A real offensive spell (school == 2) and a real non-offensive
        // one, whichever the archive's own spellsin.dat happens to list
        // first -- not hand-picked ids, so this keeps working even if
        // the asset's own ordering ever changes.
        int offensiveId = -1, selfId = -1;
        for (int i = 1; i <= spells.Count(); i++) {
            if (spells.IsOffensive(i) && offensiveId < 0 && spells.ById(i).magickaCost > 0) offensiveId = i;
            if (!spells.IsOffensive(i) && selfId < 0 && spells.ById(i).magickaCost > 0) selfId = i;
        }
        Check(offensiveId > 0, "the real spell database should have at least one offensive spell with a nonzero cost");
        Check(selfId > 0, "the real spell database should have at least one self-targeted spell with a nonzero cost");
        if (offensiveId <= 0 || selfId <= 0) return 1;
        std::printf("using offensive spell %d (%s), self spell %d (%s)\n", offensiveId,
                    spells.ById(offensiveId).name.c_str(), selfId, spells.ById(selfId).name.c_str());

        // Find a non-hub level with a real pre-placed monster spawn, and
        // stand the player facing it (same setup as M32's own test).
        int levelIdx = -1;
        for (size_t i = 1; i < levels.size(); i++) {
            if (!levels[i].monsters.empty()) {
                levelIdx = static_cast<int>(i);
                break;
            }
        }
        Check(levelIdx >= 0, "at least one non-hub level should have a real pre-placed monster spawn");
        if (levelIdx < 0) return 1;

        GeneratedLevel& level = levels[static_cast<size_t>(levelIdx)];
        int levelNumber = level.number;
        const auto& spawn = level.monsters[0];
        int standX = 0, standY = 0, facing = 0;
        Check(FindApproach(level, spawn.x, spawn.y, &standX, &standY, &facing),
              "should find a walkable approach tile facing the real monster spawn");

        dawnstar::JavaRandom globalRng(555);
        PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", charData, items, globalRng);
        player.currentLevel = levelNumber;
        player.tileX = standX;
        player.tileY = standY;
        player.facing = facing;
        player.coreStats[4] = 999;  // Magicka: plenty, for every test except B below
        player.coreStats[5] = 999;

        // --- A: invalid spellId -> a hard no-op ---
        {
            player.selectedSpellId = 0;
            int64_t last = -10000;
            MessagePopupState popup;
            bool spellHit = false, selfHit = false;
            CombatTick::ProcessSpellCast(player, levels, world, monsterDb, items, charData, spells, popup, globalRng,
                                          1000, last, spellHit, selfHit);
            Check(last == -10000, "an invalid spell id should not touch the cooldown timer");
            Check(!popup.visible, "an invalid spell id should show no message");
            Check(!spellHit && !selfHit, "an invalid spell id should not set either flash flag");
        }

        // --- B: not enough Magicka -> the message fires, but (checked
        // BEFORE the cooldown gate, matching the original's own if/else
        // if chain) the cooldown timer is untouched ---
        {
            player.selectedSpellId = static_cast<int8_t>(offensiveId);
            int16_t savedMagicka = player.coreStats[4];
            player.coreStats[4] = 0;
            int64_t last = -10000;
            MessagePopupState popup;
            bool spellHit = false, selfHit = false;
            CombatTick::ProcessSpellCast(player, levels, world, monsterDb, items, charData, spells, popup, globalRng,
                                          2000, last, spellHit, selfHit);
            Check(popup.visible && popup.lines[0] == "Not enough" && popup.lines[1] == "magicka!" &&
                      popup.priority == 3,
                  "not enough Magicka should show \"Not enough/magicka!\" at priority 3");
            Check(last == -10000, "not enough Magicka should NOT advance the cooldown timer");
            Check(!spellHit && !selfHit, "not enough Magicka should not set either flash flag");
            player.coreStats[4] = savedMagicka;
        }

        // --- C: an offensive spell with no monster targeted -> "No
        // monster here!", but the cooldown DOES advance here (a real
        // preserved quirk -- see combat_tick.h's own doc comment) ---
        {
            player.selectedSpellId = static_cast<int8_t>(offensiveId);
            player.monsterTargeted = false;
            int64_t last = -10000;
            MessagePopupState popup;
            bool spellHit = false, selfHit = false;
            CombatTick::ProcessSpellCast(player, levels, world, monsterDb, items, charData, spells, popup, globalRng,
                                          3000, last, spellHit, selfHit);
            Check(popup.visible && popup.lines[0] == "No monster" && popup.lines[1] == "here!" &&
                      popup.priority == 1,
                  "an offensive cast with no target should show \"No monster/here!\" at priority 1");
            Check(last == 3000, "the cooldown SHOULD advance even when no monster is targeted (preserved quirk)");
            Check(!spellHit && !selfHit, "an offensive cast with no target should not set either flash flag");
        }

        // --- D: an offensive spell WITH a real monster in front ---
        {
            auto* record = PlayerMovement::MonsterInFront(player, levels, world);
            Check(record != nullptr, "setup: the real spawn should still be directly ahead");
            MonsterState before = record != nullptr ? MonsterRuntime::FromBytes(*record) : MonsterState{};

            player.selectedSpellId = static_cast<int8_t>(offensiveId);
            player.monsterTargeted = true;
            int64_t last = -10000;
            MessagePopupState popup;
            bool spellHit = false, selfHit = false;
            CombatTick::ProcessSpellCast(player, levels, world, monsterDb, items, charData, spells, popup, globalRng,
                                          4000, last, spellHit, selfHit);

            Check(!popup.visible, "a resolved offensive cast shows no message of its own (CastOnMonster's own damage isn't a popup)");
            Check(spellHit && !selfHit, "an offensive cast against a real target should set spellHitFlash only");
            Check(last == 4000, "a resolved offensive cast should advance the cooldown");
            Check(player.coreStats[4] < 999, "casting an offensive spell should spend some Magicka");

            auto* recordAfter = PlayerMovement::MonsterInFront(player, levels, world);
            Check(recordAfter != nullptr, "the target should still be in the registry (a spell cast alone doesn't remove it)");
            if (recordAfter != nullptr) {
                MonsterState after = MonsterRuntime::FromBytes(*recordAfter);
                Check(after.hp <= before.hp, "CastOnMonster should never leave the target with MORE hp than before");
            }
        }

        // --- E: a self-targeted spell ---
        {
            player.coreStats[4] = 999;
            player.selectedSpellId = static_cast<int8_t>(selfId);
            int64_t last = -10000;
            MessagePopupState popup;
            bool spellHit = false, selfHit = false;
            CombatTick::ProcessSpellCast(player, levels, world, monsterDb, items, charData, spells, popup, globalRng,
                                          5000, last, spellHit, selfHit);

            Check(!spellHit && selfHit, "a self-targeted cast should set selfSpellFlash only");
            Check(last == 5000, "a resolved self-targeted cast should advance the cooldown");
            Check(player.coreStats[4] < 999, "casting a self-targeted spell should spend some Magicka");
        }

        // --- F: cooldown gating -- a second attempt well within 500ms
        // should be a hard no-op, even with plenty of Magicka and a
        // valid target ---
        {
            player.coreStats[4] = 999;
            player.selectedSpellId = static_cast<int8_t>(selfId);
            int64_t last = 6000;
            int16_t magickaBefore = player.coreStats[4];
            MessagePopupState popup;
            bool spellHit = false, selfHit = false;
            CombatTick::ProcessSpellCast(player, levels, world, monsterDb, items, charData, spells, popup, globalRng,
                                          6100, last, spellHit, selfHit);
            Check(!popup.visible, "a cooldown-blocked attempt should show no message");
            Check(!spellHit && !selfHit, "a cooldown-blocked attempt should not set either flash flag");
            Check(last == 6000, "a cooldown-blocked attempt should NOT advance the cooldown timer");
            Check(player.coreStats[4] == magickaBefore, "a cooldown-blocked attempt should not spend Magicka");
        }

        // --- G: CycleSpell -- no known spells ---
        {
            player.knownSpellsMask = 0;
            player.selectedSpellId = 0;
            MessagePopupState popup;
            CombatTick::CycleSpell(player, spells, popup, 7000);
            // MessagePopup::Show stores a negative `priority` argument
            // internally as 10 (its own "always wins" sentinel) -- see
            // render/message_popup.cpp.
            Check(popup.visible && popup.lines[0] == "No spells!" && popup.lines[1].empty() && popup.priority == 10,
                  "cycling with no known spells should show \"No spells!\" at (internal) priority 10");
            Check(player.selectedSpellId == 0, "cycling with no known spells should leave selectedSpellId untouched");
        }

        // --- G2: CycleSpell -- one real known spell ---
        {
            player.knownSpellsMask = 1u << (offensiveId - 1);
            player.selectedSpellId = 0;
            MessagePopupState popup;
            CombatTick::CycleSpell(player, spells, popup, 8000);
            Check(player.selectedSpellId == offensiveId,
                  "cycling with exactly one known spell should select it");
            Check(popup.visible && popup.lines[0] + popup.lines[1] != "" && popup.priority == 10,
                  "cycling to a real spell should show its name at (internal) priority 10");
            auto wrapped = dawnstar::MessagePopup::WrapToTwoLines(spells.ById(offensiveId).name);
            Check(popup.lines[0] == wrapped[0] && popup.lines[1] == wrapped[1],
                  "the cycle message should show the newly-selected spell's own real name");
        }

        if (g_ok) {
            std::printf("all spellcast-tick checks passed\n");
            return 0;
        } else {
            std::printf("SOME CHECKS FAILED\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
}
