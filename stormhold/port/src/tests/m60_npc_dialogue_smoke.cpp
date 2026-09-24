// M60 smoke test: PlayerMovement::ShopAheadOfPlayer (Player.shopAheadOfPlayer()),
// Shop::kNames, and ui/npc_dialogue.h's NpcDialogue -- the pieces main.cpp
// wires together to make talking to any of the 7 NPCs a real, live-gameplay
// action (GameCanvas.talkToNpc()'s own action=1 "greeting" call, NOT the
// deeper interactive Train/Give/Threaten/Kill choices menu -- see this
// milestone's own doc comments for that deliberate scope line).
#include <cstdio>
#include <map>
#include <string>

#include "assets/asset_root.h"
#include "assets/character_data.h"
#include "assets/dungeon_geometry.h"
#include "assets/item_database.h"
#include "assets/shop_dialogue.h"
#include "player/player_creation.h"
#include "player/player_movement.h"
#include "player/shop_interaction.h"
#include "ui/npc_dialogue.h"
#include "world/dungeon_generator.h"
#include "world/warden.h"

namespace {

using namespace stormhold;

bool g_ok = true;

bool Expect(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        g_ok = false;
    }
    return cond;
}

// Positions `p` one tile away from (x, y), facing toward it, so
// ComputeMoveTarget(1) lands exactly on (x, y) -- matches every real shop
// tile's own hub-town position, none of which sit on the level's edge (so
// no boundary-crossing case to worry about here).
void FaceTile(PlayerState& p, int x, int y) {
    p.currentLevel = 1;
    p.tileX = static_cast<int8_t>(x);
    p.tileY = static_cast<int8_t>(y + 1);
    p.facing = 1;  // facing==1 steps toward -Y, landing on (x, y).
}

void TestShopAheadOfPlayer(const DungeonGeometry& geometry) {
    std::printf("-- PlayerMovement::ShopAheadOfPlayer against the real hub level --\n");
    GeneratedLevel hub = DungeonGenerator::BuildHubLevel(geometry.rows[0]);
    std::map<int, GeneratedLevel> cache;
    cache[1] = hub;
    // A synthetic non-hub level, just so ComputeMoveTarget's own
    // `levels(p.currentLevel)` lookup has somewhere real to land for the
    // "non-hub level" sub-case below -- ShopAheadOfPlayer itself never
    // reads any of its tile data (it bails on `pendingLevel != 1` first).
    GeneratedLevel level2;
    level2.number = 2;
    level2.width = 35;
    level2.height = 35;
    level2.tiles.assign(35, std::vector<uint8_t>(35, 0));
    level2.populated = true;
    cache[2] = level2;
    PlayerMovement::LevelLookup lookup = [&](int n) -> GeneratedLevel& { return cache.at(n); };

    // Every one of the 7 real shop positions should resolve to its own
    // real shopId, with a fresh (all-questRewardClaimable-true) ShopState.
    for (int shopId = 0; shopId < 7; shopId++) {
        ShopState shop;
        PlayerState p;
        FaceTile(p, Shop::kShopX[shopId], Shop::kShopY[shopId]);
        int found = PlayerMovement::ShopAheadOfPlayer(p, lookup, shop);
        Expect(found == shopId, "facing a real shop tile should return that shop's own real id");
    }

    // Once a quest shop's own reward is claimed (questRewardClaimable
    // cleared -- Shop::QuestShopAt's own flag gate, M53), the same
    // position should no longer resolve.
    {
        ShopState shop;
        shop.questRewardClaimable[0] = false;
        PlayerState p;
        FaceTile(p, Shop::kShopX[0], Shop::kShopY[0]);
        int found = PlayerMovement::ShopAheadOfPlayer(p, lookup, shop);
        Expect(found == -1, "a cleared questRewardClaimable flag should make the same tile resolve to -1");
    }

    // A non-hub level should always return -1, regardless of position --
    // every NPC stands on the hub town only.
    {
        ShopState shop;
        PlayerState p;
        FaceTile(p, Shop::kShopX[0], Shop::kShopY[0]);
        p.currentLevel = 2;
        int found = PlayerMovement::ShopAheadOfPlayer(p, lookup, shop);
        Expect(found == -1, "a non-hub level should always return -1");
    }

    // An ordinary empty tile (no shop) should return -1.
    {
        ShopState shop;
        PlayerState p;
        FaceTile(p, 1, 1);
        int found = PlayerMovement::ShopAheadOfPlayer(p, lookup, shop);
        Expect(found == -1, "an empty tile should return -1");
    }
}

void TestShopNames() {
    std::printf("-- Shop::kNames --\n");
    const char* expected[7] = {"Arantamo", "Celegil", "Favela Dralor", "Vander", "Beneca", "Helga", "Varus"};
    for (int i = 0; i < 7; i++) {
        Expect(std::string(Shop::kNames[i]) == expected[i], "Shop::kNames should match Shop.NAMES exactly");
    }
}

void TestNpcDialogueState() {
    std::printf("-- NpcDialogue::Show/Dismiss --\n");
    NpcDialogueState state;
    Expect(!state.active, "a fresh NpcDialogueState should start inactive");

    NpcDialogue::Show(state, "Arantamo", "Greetings, traveler.", 0);
    Expect(state.active, "Show should set active");
    Expect(state.title == "Arantamo", "Show should store the title");
    Expect(state.body == "Greetings, traveler.", "Show should store the body");
    Expect(state.shopId == 0, "Show should store the shopId");

    NpcDialogue::Dismiss(state);
    Expect(!state.active, "Dismiss should clear active");
}

// Mirrors the exact by-shopId dispatch main.cpp's own tick loop performs
// once ShopAheadOfPlayer finds a real NPC -- action=1/extra=0 always,
// matching GameCanvas.talkToNpc()'s own `Shop.dialogue(player, npcId, 1,
// 0)` call regardless of which NPC group answers it.
std::optional<std::string> TalkTo(int shopId, PlayerState& player, ShopState& shop, WardenState& warden,
                                   const ShopDialogue& text, const CharacterData& charData, const ItemDatabase& items,
                                   GeneratedLevel& hub, JavaRandom& rng, int16_t& spawnIdCounter,
                                   const GameAdvancement::LevelLookup& levels) {
    if (shopId <= 3) {
        return ShopInteraction::QuestShopDialogue(player, shop, text, charData, items, hub, rng, shopId, 1, 0);
    }
    if (shopId == 4) {
        return ShopInteraction::BenecaDialogue(player, shop, text, items, spawnIdCounter, 1, 0);
    }
    if (shopId == 5) {
        return ShopInteraction::HelgaDialogue(player, shop, text, items, 1, 0, levels);
    }
    return ShopInteraction::VarusDialogue(player, warden, text);
}

// Integration check: for each of the 7 real shops, ShopAheadOfPlayer finds
// the right id AND dispatching through it (the same way main.cpp will)
// produces a real dialogue line that actually opens NpcDialogue -- proving
// the pieces this milestone wires together actually fit, not just each in
// isolation.
void TestEndToEndGreeting(const DungeonGeometry& geometry, const CharacterData& charData, const ItemDatabase& items,
                           const ShopDialogue& text) {
    std::printf("-- end-to-end: ShopAheadOfPlayer -> per-NPC dialogue -> NpcDialogue::Show --\n");
    GeneratedLevel hub = DungeonGenerator::BuildHubLevel(geometry.rows[0]);
    std::map<int, GeneratedLevel> cache;
    cache[1] = hub;
    PlayerMovement::LevelLookup lookup = [&](int n) -> GeneratedLevel& { return cache.at(n); };

    for (int shopId = 0; shopId < 7; shopId++) {
        ShopState shop;
        WardenState warden;
        warden.visitCount = 1;  // Varus needs at least one visit to say anything.
        PlayerState player = PlayerCreation::CreateCharacter(0, "Tester", 1, charData, items);
        FaceTile(player, Shop::kShopX[shopId], Shop::kShopY[shopId]);
        JavaRandom rng(1);
        int16_t spawnIdCounter = 10000;

        int found = PlayerMovement::ShopAheadOfPlayer(player, lookup, shop);
        Expect(found == shopId, "ShopAheadOfPlayer should find this shop before dispatching");

        std::optional<std::string> line =
            TalkTo(found, player, shop, warden, text, charData, items, cache.at(1), rng, spawnIdCounter, lookup);
        Expect(line.has_value(), "a fresh, never-greeted NPC should always have a first-visit line to say");

        NpcDialogueState dialogueState;
        if (line.has_value()) {
            NpcDialogue::Show(dialogueState, Shop::kNames[shopId], *line, shopId);
            Expect(dialogueState.active, "a real greeting line should open the dialogue screen");
            Expect(dialogueState.title == Shop::kNames[shopId], "the screen's title should be this NPC's own name");
            Expect(!dialogueState.body.empty(), "the screen's body should be the real dialogue line");
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : "../../extracted";

    try {
        AssetRoot assets(root);
        DungeonGeometry geometry = DungeonGeometry::Load(assets);
        CharacterData charData = CharacterData::Load(assets);
        ItemDatabase items = ItemDatabase::Load(assets);
        ShopDialogue text = ShopDialogue::Load(assets);

        TestShopAheadOfPlayer(geometry);
        TestShopNames();
        TestNpcDialogueState();
        TestEndToEndGreeting(geometry, charData, items, text);

        if (!g_ok) {
            std::fprintf(stderr, "m60_npc_dialogue_smoke: FAILED self-consistency checks\n");
            return 1;
        }

        std::printf("all self-consistency checks passed\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "m60_npc_dialogue_smoke: FAILED: %s\n", e.what());
        return 1;
    }

    return 0;
}
