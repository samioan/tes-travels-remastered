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
#include "assets/spell_database.h"
#include "camp/camp_tick.h"
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
#include "ui/menu_flow.h"
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
        // like M20's own original simplification. Still no real
        // character-creation UI exists (that's `ESGame`'s own
        // class-selection/name-entry flow, still unported -- see
        // ui/menu_flow.h's own class comment): "New Game" starts the
        // same fixed class-0 stand-in character M20 always did, just
        // now reached through a real menu selection instead of
        // automatically.
        dawnstar::MenuFlow menuFlow(helpText);
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
                            playerSlot.emplace(
                                dawnstar::PlayerCreation::CreateCharacter(0, "Traveler", charData, items, globalRng));
                            inMenu = false;
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
                    // tick, never more than one. Camp/interact/cast/
                    // cycle/attack/move are wired so far (options alone
                    // still needs UI this port doesn't have yet -- an
                    // options menu); camp outranks interact outranks
                    // cast outranks cycle outranks attack outranks move,
                    // matching the original's own ordering exactly.
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
