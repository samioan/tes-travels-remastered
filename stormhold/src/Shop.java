// Renamed from decompiled/k.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (npcstrings.dat).
//
// NOT a copy of dawnstar's Shop.java -- structurally similar (same
// npcstrings.dat loader shape, same isValidShopAction/shopActionCode
// matched pair, same questFlagsFor 2-bit extraction) but Stormhold's own
// NPC roster and quest system are genuinely different, confirmed by
// reading this file directly:
//
// - 7 NPCs, not 9. NAMES/SHOP_CATEGORY split them into 3 real groups:
//   shops 0-3 (category 1) are quest-turn-in shopkeepers -- structurally
//   the counterpart of dawnstar's *named* shops 5-8, not its generic
//   buy/sell peddlers 0-3. **No buy/sell (action 14/15-style) branch
//   exists anywhere in this file** -- Stormhold's item economy, if any,
//   must live elsewhere (ESGame or Player), not in this class.
// - Shop 4 ("Beneca", category 2) and shop 5 ("Helga", category 2) are
//   bespoke single-NPC branches, not part of the shops-0-3 pattern.
//   Helga's action branches (10=cure ailment, 11=warp to camp, 12=full
//   HP+Magicka heal) are the closest thing to dawnstar's Jakar's
//   (shop 4) role, gated by her own `giftPoints` economy (this class's
//   `g` field) instead of dawnstar's Player.giftPointsFound.
// - Shop 6 ("Varus", category 3) is tied to a Stormhold-only "Warden"
//   world event (wardenArrives/wardenLeaves, prints "WARDEN VISITS!!"/
//   "WARDEN LEAVES!!", flips dungeon tile bit 32 at Varus's own world
//   position) with no dawnstar equivalent at all.
// - The rumor/quest-turn-in tracking piggybacks directly on
//   Player.skills[step][0] (was `R[][0]`) and Player.skills[13][2] (was
//   `R[13][2]`, an exp-like accumulator) -- there is no separate
//   eventFlags-based rumor-reveal table or traitorIndex-keyed offset
//   table like dawnstar's RUMOR_STRING_OFFSET/UNCONFIRMED_A/B. Either
//   Stormhold has no "hidden traitor" subplot, or it's implemented
//   somewhere this class doesn't touch.
//
// `Player` is forward-referenced by its expected real name/fields (same
// convention Monster.java already uses) -- it is NOT renamed yet as of
// this pass. Field names below with a `TODO_`/`unconfirmed` prefix are
// this pass's best guess, not confirmed; the four without that prefix
// (tileX/tileY as `l`/`k`, coreStats as `U`, ailmentMask as `A`, skills
// as `R`, inventoryItemIds as `H`, hasCampMark/warpToCampMark as `x()`/
// `e()`, removeInventorySlot as `y(int)`, rollShopOutcome as `b(int,int)`)
// are corroborated either by matching dawnstar's confirmed Player layout
// 1:1 in role, or by direct cross-reads of j.java itself (initializeItemCharge/
// isItemCharged, i.e. `h(int)`/`j(int)`, were independently confirmed
// while renaming Item.java -- see Item.isEquipmentCategory's doc comment).
import java.io.DataInputStream;

public class Shop {
   static String[] NAMES = new String[]{"Arantamo", "Celegil", "Favela Dralor", "Vander", "Beneca", "Helga", "Varus"};
   // 1 = quest-turn-in shopkeeper (shops 0-3), 2 = bespoke single-NPC
   // (Beneca/Helga, shops 4-5), 3 = Varus/Warden-tied (shop 6).
   static byte[] SHOP_CATEGORY = new byte[]{1, 1, 1, 1, 2, 2, 3};
   static byte[] SHOP_X = new byte[]{12, 3, 15, 6, 7, 12, 9};
   static byte[] SHOP_Y = new byte[]{3, 7, 7, 13, 2, 13, 9};

   // Two DISTINCT one-shot flag arrays, both reset all-true -- not a
   // single firstVisit[] like dawnstar. `firstVisit` gates shops 4/5's
   // own first-greeting branch; `questRewardClaimable` (was `b`) gates a
   // one-time reward-collection branch for shops 0-3 (action 6, clears
   // dungeon tile bit 32 at that shop's own world position -- same bit
   // the Warden mechanic uses, at a different position).
   static boolean[] questRewardClaimable;
   static boolean[] firstVisit;

   // Warden world-event state: wardenVisitCount (was `f`, 0-3, escalating
   // threshold gate via shouldWardenVisit), wardenPresent (was `d`).
   static byte wardenVisitCount = 0;
   static boolean wardenPresent = false;

   // Per-quest-shop (index = shopId, 0-3) state, same role as dawnstar's
   // questState1/questState2/interactionCount/rewardsGiven but sized 4
   // (one slot per shop 0-3, not shop-5..8-offset like dawnstar).
   static byte[] questState1;
   static byte[] questState2;
   static short[] interactionCount;
   static short[] rewardsGiven;
   // A THIRD short[4] (was `h`, distinct from interactionCount/was `r`,
   // easy to conflate since both are per-shop short counters read/reset
   // the same way) -- gates both the greeting branch (action 1) and the
   // reward-claim branch (action 5) with a ">50" cooldown-shaped check.
   // No increment site found anywhere in this file, so its producer is
   // external (a per-tick decay driven by ESGame/GameCanvas, most likely)
   // -- name and exact unit unconfirmed.
   static short[] unconfirmedCooldownH;

   // Beneca's own spendable-points counter (was `a`) -- earned via her
   // action==4 branch, spent 3-at-a-time via action==7 (a "training"-
   // shaped call into Player, see dialogue()'s case 4).
   static short benecaPoints;
   // Helga's own spendable "gift points" counter (was `g`) -- earned via
   // her action==4 branch, spent by her action 8-11 branches (cure/rest/
   // warp/heal). Analogous role to dawnstar's Jakar's but a completely
   // separate economy from Beneca's benecaPoints.
   static short helgaPoints;

   // Set true externally (no assignment site found in this file, same
   // "producer lives elsewhere" situation as dawnstar's showDeathGreeting)
   // -- when true, Helga's greeting is prefixed with dialogue[5][21] once
   // and the flag clears. Exact trigger unconfirmed.
   static boolean showSpecialGreeting;

   static String[][] dialogue;
   static int[] GROUP_SIZES = new int[]{20, 20, 20, 20, 5, 22, 5, 41};
   static boolean dialogueLoaded = false;

   static void loadDialogue() {
      load("/npcstrings.dat");
   }

   static void load(String path) {
      try {
         DataInputStream in = Util.openResource(path);
         byte groups = 8;
         dialogue = new String[groups][];

         for (int i = 0; i < groups; i++) {
            loadGroup(i, GROUP_SIZES[i], in);
         }

         dialogueLoaded = true;
      } catch (Exception e) {
         System.out.println("ERROR loading NPC and generic strings!");
         dialogueLoaded = false;
      }
   }

   // Whether `elapsedCounter` (exact unit/source unconfirmed -- some kind
   // of playtime/turn counter, passed in by the caller) has crossed the
   // next escalating Warden-visit threshold (13/26/39) for the current
   // wardenVisitCount, while the Warden isn't already present.
   static boolean shouldWardenVisit(int elapsedCounter) {
      if (wardenVisitCount == 0 && !wardenPresent && elapsedCounter >= 13) {
         return true;
      } else {
         return wardenVisitCount == 1 && !wardenPresent && elapsedCounter >= 26
            ? true
            : wardenVisitCount == 2 && !wardenPresent && elapsedCounter >= 39;
      }
   }

   // Stormhold-only: the Warden manifests at Varus's (shop 6) own world
   // position, flipping dungeon tile bit 32 there. No dawnstar equivalent.
   static void wardenArrives() {
      System.out.println("WARDEN VISITS!!");
      wardenPresent = true;
      wardenVisitCount++;
      byte x = SHOP_X[6];
      byte y = SHOP_Y[6];
      ESGame.dungeons[0].tiles[x][y] = (byte)(ESGame.dungeons[0].tiles[x][y] | 32);
   }

   static void wardenLeaves() {
      System.out.println("WARDEN LEAVES!!");
      byte x = SHOP_X[6];
      byte y = SHOP_Y[6];
      wardenPresent = false;
      byte current = ESGame.dungeons[1].tiles[x][y];
      ESGame.dungeons[0].tiles[x][y] = Util.clearBit((byte)32, current);
   }

   private static void loadGroup(int group, int expectedCount, DataInputStream in) throws Exception {
      int count = in.readInt();
      if (count != expectedCount) {
         System.out.println("Unexpected number of messages for whichNPC = " + group);
         throw new Exception("Error in readNPCMessages: npc is " + group);
      }

      dialogue[group] = new String[count];

      for (int i = 0; i < count; i++) {
         dialogue[group][i] = in.readUTF();
      }
   }

   static void reset() {
      questRewardClaimable = new boolean[7];
      firstVisit = new boolean[7];

      for (int i = 0; i < 7; i++) {
         questRewardClaimable[i] = true;
         firstVisit[i] = true;
      }

      interactionCount = new short[4];
      rewardsGiven = new short[4];
      unconfirmedCooldownH = new short[4];
      questState1 = new byte[4];
      questState2 = new byte[4];

      for (int i = 0; i < 4; i++) {
         interactionCount[i] = 0;
         rewardsGiven[i] = 0;
         unconfirmedCooldownH[i] = 0;
         questState1[i] = 0;
         questState2[i] = 0;
      }

      benecaPoints = 0;
      helgaPoints = 0;
      showSpecialGreeting = false;
   }

   // Clears per-level-up quest-turn-in state for shops 0-3.
   static void clearQuestTurnInState() {
      for (int i = 0; i < 4; i++) {
         questState1[i] = 0;
         questState2[i] = 0;
      }
   }

   static boolean isQuestShop(int shopId) {
      return SHOP_CATEGORY[shopId] == 1;
   }

   // Finds which of shops 0-6 is at (x, y) AND still has its one-shot
   // reward flag set (questRewardClaimable) -- unlike dawnstar's plain
   // hubShopAt, this is gated by that flag, not a pure position lookup.
   static int questShopAt(int x, int y) {
      for (int i = 0; i < 7; i++) {
         if (x == SHOP_X[i] && y == SHOP_Y[i] && questRewardClaimable[i]) {
            return i;
         }
      }

      return -1;
   }

   // Extracts the 2-bit quest-turn-in flag for shop 0-3 from an item's
   // packed questFlags byte (Item.column(3, itemId)).
   static int questFlagsFor(int shopId, int itemId) {
      int flags = Item.column(3, itemId);
      if (shopId == 0) {
         return flags >>> 6 & 3;
      } else if (shopId == 1) {
         return flags >>> 4 & 3;
      } else if (shopId == 2) {
         return flags >>> 2 & 3;
      } else {
         return shopId == 3 ? flags & 3 : 0;
      }
   }

   // Reveals the next rumor-pool fragment (group 7) for `player`, tagged
   // by Player.skillNames[step] (was `j.E`) -- or, on a repeat ask, a
   // phrasing citing how many times it's been asked before. Piggybacks on
   // Player.skills[step][0] as the per-step ask counter.
   static String rumorFor(Player player, int step) {
      short asked = player.skills[step][0];
      if (asked == 0) {
         player.skills[step][0] = 1;
         String template = dialogue[7][1];
         return Util.replace(template, "<TAG>", Player.skillNames[step]);
      } else {
         player.skills[step][0] = (short)(asked + 1);
         String template = dialogue[7][2];
         String[] values = new String[]{Player.skillNames[step], String.valueOf(asked), String.valueOf(asked + 1)};
         return Util.replace(template, "<TAG>", values);
      }
   }

   // Whether `player` is standing adjacent to Varus (shop 6), and on
   // level 1. RESOLVED (was flagged "player.j == 1, name TODO"): found
   // the actual original method, decompiled/k.java:450 `static boolean
   // a(j var0) { if (var0.j != 1) return false; ... }` -- `var0.j` is
   // Player's own field `j` (not Monster's unrelated field letters, just
   // a same-letter coincidence), confirmed elsewhere to be currentLevel
   // (decompiled/j.java's `g(int)`, the pendingLevel-computing method,
   // defaults `this.ab = this.j` when no level boundary is crossed).
   // What's below is a byte-for-byte-complete match of that method, not
   // an approximation missing a condition.
   static boolean isAdjacentToVarus(Player player) {
      if (player.currentLevel != 1) {
         return false;
      }

      int dx = Math.abs(player.tileX - SHOP_X[6]);
      int dy = Math.abs(player.tileY - SHOP_Y[6]);
      return dx + dy == 1;
   }

   // The single dispatcher for every NPC interaction. shopId 0-3 share one
   // quest-turn-in pattern (actions 1-6); 4 (Beneca), 5 (Helga), 6 (Varus)
   // are each bespoke. Action-code meanings beyond what's used here are
   // inferred from this switch alone, same caveat dawnstar's own
   // dialogue() left documented.
   static String dialogue(Player player, int shopId, int action, int extra) {
      switch (shopId) {
         case 0:
         case 1:
         case 2:
         case 3:
            if (action == 1) {
               if (firstVisit[shopId]) {
                  firstVisit[shopId] = false;
                  return dialogue[shopId][0];
               } else if (unconfirmedCooldownH[shopId] > 50) {
                  return dialogue[shopId][1];
               } else {
                  if (player.coreStats[8] > 50) {
                     return dialogue[shopId][2];
                  }

                  int line = ESGame.lingoRandomInt(3);
                  return dialogue[shopId][3 + line];
               }
            } else if (action == 2) {
               if (questState1[shopId] != 0) {
                  return dialogue[shopId][6];
               }

               int outcome = player.rollShopOutcome(shopId, action);
               if (outcome == 0) {
                  questState1[shopId] = 1;
               } else if (outcome == 1) {
                  player.skills[13][2] = (short)(player.skills[13][2] + 2);
               } else if (outcome == 2) {
                  player.skills[13][2] = (short)(player.skills[13][2] + 5);
                  rewardsGiven[shopId]++;
                  questState1[shopId] = 1;
               } else if (outcome == 3) {
                  player.skills[13][2] = (short)(player.skills[13][2] + 8);
                  rewardsGiven[shopId]++;
                  questState1[shopId] = 1;
               }

               interactionCount[shopId]++;
               return dialogue[shopId][7 + outcome];
            } else if (action == 3) {
               if (questState2[shopId] != 0) {
                  return dialogue[shopId][6];
               }

               int outcome = player.rollShopOutcome(shopId, action);
               int variant = extra <= 1 ? 0 : 1;
               if (outcome == 0) {
                  questState2[shopId] = 2;
               } else if (outcome == 1) {
                  player.skills[13][2] = (short)(player.skills[13][2] + 2);
                  questState2[shopId] = 2;
               } else if (outcome == 2) {
                  player.skills[13][2] = (short)(player.skills[13][2] + 5);
                  rewardsGiven[shopId]++;
                  questState2[shopId] = 1;
               } else if (outcome == 3) {
                  player.skills[13][2] = (short)(player.skills[13][2] + 8);
                  rewardsGiven[shopId]++;
                  questState2[shopId] = 1;
               }

               interactionCount[shopId]++;
               return dialogue[shopId][11 + variant];
            } else if (action == 4) {
               if (questState1[shopId] != 2 && questState2[shopId] != 2) {
                  int slot = extra;
                  int itemId = Math.abs(player.inventoryItemIds[slot]);
                  // Category 15: a "reduce the greeting cooldown" item --
                  // consumed, its RAW (non-bit-extracted) questFlags column
                  // value is subtracted from unconfirmedCooldownH, clamped
                  // at 0. Distinct from category 11 below; reuses the same
                  // itemsin.dat column for an unrelated purpose.
                  if (Item.column(1, itemId) == 15) {
                     player.removeInventorySlot(slot);
                     int reduceBy = Item.column(3, itemId);
                     unconfirmedCooldownH[shopId] = (short)(unconfirmedCooldownH[shopId] - reduceBy);
                     unconfirmedCooldownH[shopId] = (short)Math.max(unconfirmedCooldownH[shopId], 0);
                     return dialogue[shopId][17];
                  }

                  // Category 11 ("gift"/quest item): the real quest-item
                  // delivery, gated by the bit-extracted questFlagsFor.
                  if (Item.column(1, itemId) == 11) {
                     int flags = questFlagsFor(shopId, itemId);
                     if (flags > 0) {
                        player.removeInventorySlot(slot);
                        rewardsGiven[shopId] = (short)(rewardsGiven[shopId] + flags);
                        questState1[shopId] = 0;
                        questState2[shopId] = 0;
                     }

                     return dialogue[shopId][13 + flags];
                  }

                  return dialogue[shopId][13];
               }

               return dialogue[shopId][13];
            } else if (action == 5) {
               if (rewardsGiven[shopId] == 0) {
                  return dialogue[shopId][18];
               } else if (unconfirmedCooldownH[shopId] > 50) {
                  return dialogue[shopId][1];
               } else {
                  if (player.coreStats[8] > 50) {
                     return dialogue[shopId][2];
                  }

                  rewardsGiven[shopId]--;
                  int step = extra;
                  return rumorFor(player, step);
               }
            } else {
               if (action == 6) {
                  questRewardClaimable[shopId] = false;
                  Dungeon hub = ESGame.dungeons[0];
                  hub.tiles[SHOP_X[shopId]][SHOP_Y[shopId]] = Util.clearBit((byte)32, hub.tiles[SHOP_X[shopId]][SHOP_Y[shopId]]);
                  return dialogue[shopId][19];
               }

               return null;
            }
         case 4:
            if (action == 1) {
               if (firstVisit[4]) {
                  firstVisit[4] = false;
                  return dialogue[4][0];
               }

               return null;
            } else if (action == 4) {
               int slot = extra;
               int itemId = Math.abs(player.inventoryItemIds[slot]);
               int col1 = Item.column(1, itemId);
               if (col1 != 13 && col1 != 15 && col1 != 17) {
                  benecaPoints++;
                  player.removeInventorySlot(slot);
                  return dialogue[4][2];
               }

               return dialogue[4][1];
            } else {
               if (action == 7) {
                  if (benecaPoints / 3 > 0) {
                     int slot = extra;
                     short spawnId = Item.nextSpawnId();
                     boolean trained = player.addInventoryItemRaw(slot, spawnId, 0);
                     if (!trained) {
                        return dialogue[7][0];
                     }

                     benecaPoints = (short)(benecaPoints - 3);
                     return dialogue[4][3];
                  }

                  return dialogue[4][4];
               }

               return null;
            }
         case 5:
            if (action == 1) {
               System.out.println("Greeting Helga");
               if (firstVisit[5]) {
                  System.out.println("first meeting");
                  firstVisit[5] = false;
                  player.rumorRevealStep = 0;
                  System.out.println("message[iNPC] length is " + dialogue[5].length);
                  System.out.println(dialogue[5][0]);
                  System.out.println(dialogue[5][2]);
                  if (showSpecialGreeting) {
                     showSpecialGreeting = false;
                     return dialogue[5][21] + "\n" + dialogue[5][0] + "\n" + dialogue[5][2];
                  }

                  return dialogue[5][0] + "\n" + dialogue[5][2];
               } else {
                  int advancement = ESGame.getGameAdvancementLevel(player.giftPointsFound);
                  if (advancement > player.rumorRevealStep) {
                     player.rumorRevealStep++;
                     if (showSpecialGreeting) {
                        showSpecialGreeting = false;
                        return dialogue[5][21] + "\n" + dialogue[5][2 + player.rumorRevealStep];
                     }

                     return dialogue[5][2 + player.rumorRevealStep];
                  } else {
                     if (showSpecialGreeting) {
                        showSpecialGreeting = false;
                        return dialogue[5][21];
                     }

                     return null;
                  }
               }
            } else if (action == 13) {
               return dialogue[5][2 + player.rumorRevealStep];
            } else if (action == 4) {
               int slot = extra;
               int itemId = Math.abs(player.inventoryItemIds[slot]);
               if (Item.column(1, itemId) == 13) {
                  int quality = player.itemSubtypeAtSlot(slot);
                  if (quality > 3) {
                     helgaPoints = (short)(helgaPoints + 5);
                  } else {
                     helgaPoints = (short)(helgaPoints + 3);
                  }

                  player.removeInventorySlot(slot);
                  return dialogue[5][11];
               }

               return dialogue[5][12];
            } else if (action == 8) {
               if (helgaPoints < 7) {
                  return dialogue[5][1];
               } else {
                  int slot = extra;
                  int itemId = Math.abs(player.inventoryItemIds[slot]);
                  if (Item.isEquipmentCategory(itemId) && !player.isItemCharged(slot)) {
                     helgaPoints = (short)(helgaPoints - 7);
                     player.initializeItemCharge(slot);
                     return dialogue[5][13];
                  }

                  return dialogue[5][14];
               }
            } else if (action == 9) {
               if (helgaPoints < 2) {
                  return dialogue[5][1];
               } else {
                  if (player.safeCampingBuff) {
                     return dialogue[5][15];
                  }

                  player.safeCampingBuff = true;
                  helgaPoints = (short)(helgaPoints - 2);
                  return dialogue[5][16];
               }
            } else if (action == 10) {
               if (helgaPoints < 1) {
                  return dialogue[5][1];
               }

               player.ailmentMask = 0;
               helgaPoints--;
               return dialogue[5][17];
            } else if (action == 11) {
               if (helgaPoints < 1) {
                  return dialogue[5][1];
               } else {
                  if (!player.hasCampMark()) {
                     return dialogue[5][18];
                  }

                  helgaPoints--;
                  player.warpToCampMark();
                  return dialogue[5][19];
               }
            } else {
               if (action == 12) {
                  player.coreStats[2] = player.coreStats[3];
                  player.coreStats[4] = player.coreStats[5];
                  return dialogue[5][20];
               }

               return null;
            }
         case 6:
            if (wardenVisitCount == 0) {
               return null;
            } else if (wardenVisitCount == 1 && player.wardenLoreStep == 0) {
               player.wardenLoreStep = 1;
               return dialogue[6][0];
            } else if (wardenVisitCount == 2 && player.wardenLoreStep <= 1) {
               player.wardenLoreStep = 2;
               return dialogue[6][1];
            } else if (wardenVisitCount == 3 && player.wardenLoreStep <= 2) {
               player.wardenLoreStep = 3;
               return dialogue[6][2];
            } else {
               if (wardenVisitCount == 4 && player.wardenLoreStep <= 3) {
                  player.wardenLoreStep = 4;
                  return dialogue[6][3] + "\n" + dialogue[6][4];
               }

               return null;
            }
         default:
            return null;
      }
   }

   // Matched pair with shopActionCode below, for shops 0-3's quest
   // actions. No confirmed call site within this file -- likely used by
   // ESGame's own menu-building code, same as dawnstar's version.
   static boolean isValidShopAction(int shopId, int action) {
      switch (shopId) {
         case 0:
            switch (action) {
               case 3:
               case 4:
               case 13:
                  return true;
               default:
                  return false;
            }
         case 1:
            switch (action) {
               case 7:
               case 8:
               case 10:
                  return true;
               case 9:
               default:
                  return false;
            }
         case 2:
            switch (action) {
               case 1:
               case 6:
               case 12:
                  return true;
               default:
                  return false;
            }
         case 3:
            switch (action) {
               case 0:
               case 2:
               case 5:
                  return true;
               default:
                  return false;
            }
         default:
            return false;
      }
   }

   static int shopActionCode(int shopId, int choiceIndex) {
      switch (shopId) {
         case 0:
            switch (choiceIndex) {
               case 0:
                  return 3;
               case 1:
                  return 4;
               case 2:
                  return 13;
               default:
                  return -1;
            }
         case 1:
            switch (choiceIndex) {
               case 0:
                  return 7;
               case 1:
                  return 8;
               case 2:
                  return 10;
               default:
                  return -1;
            }
         case 2:
            switch (choiceIndex) {
               case 0:
                  return 1;
               case 1:
                  return 6;
               case 2:
                  return 12;
               default:
                  return -1;
            }
         case 3:
            switch (choiceIndex) {
               case 0:
                  return 0;
               case 1:
                  return 2;
               case 2:
                  return 5;
               default:
                  return -1;
            }
         default:
            return -1;
      }
   }

   static {
      reset();
   }
}
