// Renamed from decompiled/k.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (npcstrings.dat).
//
// `Player` here is the real, already-renamed Player class -- this file
// was updated alongside Player's own rename pass to integrate directly.
import java.io.DataInputStream;

public class Shop {
   static String[] NAMES = new String[]{
      "Weapon Peddler", "Heavy Armor Peddler", "Light Armor Peddler", "Jakar's", "Eustacia", "Alhavara", "Beatrice", "Chung", "Delacroix"
   };
   // 4 = generic hub peddler (shops 0-3), 2 = Jakar's (the main
   // quest-giver, shop index 3 despite being counted with the "generic"
   // group by every other table here), 1 = named quest shopkeeper
   // (shops 5-8). Confirmed via isGenericPeddler/isNamedShop below;
   // exact numeric choice (4/2/1) otherwise arbitrary.
   static byte[] SHOP_CATEGORY = new byte[]{4, 4, 4, 4, 2, 1, 1, 1, 1};
   // World position, indices 0-4 fixed (hub town), 5-8 overwritten by
   // DungeonGenerator (those 4 shopkeepers live inside levels 3/12/21/30).
   static byte[] SHOP_X = new byte[]{12, 6, 7, 12, 12, 1, 1, 1, 1};
   static byte[] SHOP_Y = new byte[]{12, 11, 7, 8, 6, 1, 1, 1, 1};
   static boolean[] firstVisit;
   // Per named-shop (index = shopId-5) quest-turn-in tracking.
   static byte[] questState1;
   static byte[] questState2;
   static short[] interactionCount;
   static short[] rewardsGiven;
   // Declared, reset in reset(), no confirmed read site.
   static short unused;
   // Set true on respawn-after-death (see GameCanvas's tick loop);
   // consumed by dialogue() to show an extra line the next time Jakar's
   // greeting fires.
   static boolean showDeathGreeting;
   static String[][] dialogue;
   static int[] GROUP_SIZES = new int[]{3, 3, 3, 3, 14, 16, 16, 16, 16, 77};
   static boolean dialogueLoaded = false;

   // Each generic shop's (0-3) stock: item ids sellable there. Shop 3
   // ("Jakar's") stocks the 87-96 "gift"/special-consumable range.
   static byte[][] SHOP_STOCK = new byte[][]{
      {1, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13, 14, 15, 17, 18, 19, 20},
      {22, 23, 24, 25, 32, 33, 34, 35, 47, 48, 49, 50},
      {27, 28, 29, 30, 37, 38, 39, 40, 42, 43, 44, 45},
      {87, 88, 89, 90, 91, 93, 94, 95, 96}
   };
   // [traitorId 0-3][revealStep 0-5] -> offset into dialogue[9] (+5) for
   // that step's rumor-fragment text. Ties into Player's hidden
   // "traitor" field (confusingly also named `ai` in the original,
   // rolled 0-3 at character creation) -- the rumor system gradually
   // reveals which of 4 candidates is the traitor.
   static final byte[][] RUMOR_STRING_OFFSET = new byte[][]{{1, 3, 5, 8, 10, 12}, {1, 2, 4, 7, 9, 12}, {2, 3, 6, 7, 10, 11}, {2, 4, 5, 8, 9, 11}};
   // Declared, no confirmed read site found -- values (13-60) don't fit
   // any range used elsewhere in this class, so not documented as
   // anything more specific.
   static final byte[] UNCONFIRMED_A = new byte[]{13, 19, 25, 31, 14, 20, 26, 32, 15, 21, 27, 33, 17, 23, 29, 35, 16, 22, 28, 34, 18, 24, 30, 36};
   static final byte[] UNCONFIRMED_B = new byte[]{37, 43, 49, 55, 38, 44, 50, 56, 39, 45, 51, 57, 41, 47, 53, 59, 40, 46, 52, 58, 42, 48, 54, 60};

   static void loadDialogue() {
      load("/npcstrings.dat");
   }

   static void load(String path) {
      try {
         DataInputStream in = Util.openResource(path);
         byte groups = 10;
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
      firstVisit = new boolean[9];

      for (int i = 0; i < 9; i++) {
         firstVisit[i] = true;
      }

      interactionCount = new short[4];
      rewardsGiven = new short[4];
      questState1 = new byte[4];
      questState2 = new byte[4];

      for (int i = 0; i < 4; i++) {
         interactionCount[i] = 0;
         rewardsGiven[i] = 0;
         questState1[i] = 0;
         questState2[i] = 0;
      }

      unused = 0;
      showDeathGreeting = false;
   }

   // Called on level-up (Player.c()) to clear per-level-up quest-turn-in
   // state.
   static void clearQuestTurnInState() {
      for (int i = 0; i < 4; i++) {
         questState1[i] = 0;
         questState2[i] = 0;
      }
   }

   static boolean isNamedShop(int shopId) {
      return SHOP_CATEGORY[shopId] == 1;
   }

   static boolean isGenericPeddler(int shopId) {
      return SHOP_CATEGORY[shopId] == 4;
   }

   // Finds which of the 5 fixed hub shop positions (x, y) is at, or -1.
   static int hubShopAt(int x, int y) {
      for (int i = 0; i < 5; i++) {
         if (x == SHOP_X[i] && y == SHOP_Y[i]) {
            return i;
         }
      }

      return -1;
   }

   // Extracts the 2-bit quest-turn-in flag for shop 5-8 from an item's
   // packed questFlags byte (Item.column(3, itemId)).
   static int questFlagsFor(int shopId, int itemId) {
      int flags = Item.column(3, itemId);
      if (shopId == 5) {
         return flags >>> 6 & 3;
      } else if (shopId == 6) {
         return flags >>> 4 & 3;
      } else if (shopId == 7) {
         return flags >>> 2 & 3;
      } else {
         return shopId == 8 ? flags & 3 : 0;
      }
   }

   // Reveals the next traitor-rumor fragment for `player`, or repeats
   // the current one if already revealed once (2nd+ ask -> a slightly
   // different phrasing citing the previous reveal, see original for
   // the exact `<TAG>` substitution).
   static String rumorFor(Player player, int step) {
      short revealed = player.skills[step][0];
      if (revealed == 0) {
         player.skills[step][0] = 1;
         String template = dialogue[9][1];
         return Util.replace(template, "<TAG>", Player.skillNames[step]);
      } else {
         player.skills[step][0] = (short)(revealed + 1);
         String template = dialogue[9][2];
         String[] values = new String[]{Player.skillNames[step], String.valueOf(revealed), String.valueOf(revealed + 1)};
         return Util.replace(template, "<TAG>", values);
      }
   }

   // The single dispatcher for every shop/NPC interaction: greet, buy
   // (action 14, catalogSlot picks from SHOP_STOCK), sell (action 15,
   // blocks selling category-11 "gift" items), and richer scripted
   // branches for the named shops (5-8, quest-item delivery for a
   // reward) and Jakar's (shop 4: rumors, "traitor" advancement,
   // level-up-gated dialogue). Exact meaning of each action code beyond
   // what's used here (1/2/3/4/5/8/10/11/12/13/14/15) is inferred from
   // this switch alone, not from the UI call sites choosing them.
   static String dialogue(Player player, int shopId, int action, int extra) {
      switch (shopId) {
         case 0:
         case 1:
         case 2:
         case 3:
            if (action == 1) {
               return dialogue[shopId][0];
            } else if (action == 14) {
               int catalogSlot = extra;
               byte itemId = SHOP_STOCK[shopId][catalogSlot];
               int price = Item.column(4, itemId);
               if (price > player.gold) {
                  return dialogue[shopId][1];
               } else {
                  short spawnId = Item.nextSpawnId();
                  boolean added = player.addInventoryItem(itemId, spawnId, 0);
                  if (added) {
                     player.addGold(-price);
                     return dialogue[shopId][2];
                  }

                  return "Sorry, but your pack is too full.";
               }
            } else {
               if (action == 15) {
                  int slot = extra;
                  int itemId = Math.abs(player.inventoryItemIds[slot]);
                  if (Item.column(1, itemId) == 11) {
                     return "Sorry, you may not sell a gift item.  It should be given to one of the champions.";
                  }

                  int saleValue = Item.column(5, itemId);
                  player.addGold(saleValue);
                  player.removeInventorySlot(slot);
                  return "For that you can have " + saleValue + " gold.";
               }

               return "quack";
            }
         case 5:
         case 6:
         case 7:
         case 8:
            if (action == 1) {
               if (firstVisit[shopId]) {
                  firstVisit[shopId] = false;
                  return dialogue[shopId][0];
               }

               int line = ESGame.nextInt(3);
               return dialogue[shopId][1 + line];
            } else if (action == 2) {
               if (questState1[shopId - 5] != 0) {
                  return dialogue[shopId][4];
               }

               // Player's own quest-skill-check roll, not Shop's
               // shopActionCode.
               int outcome = player.rollShopOutcome(shopId, action);
               if (outcome == 0) {
                  questState1[shopId - 5] = 1;
               } else if (outcome == 1) {
                  player.gainSkillExp(13, 2);
               } else if (outcome == 2) {
                  player.gainSkillExp(13, 5);
                  rewardsGiven[shopId - 5]++;
                  questState1[shopId - 5] = 1;
               } else if (outcome == 3) {
                  player.gainSkillExp(13, 8);
                  rewardsGiven[shopId - 5]++;
                  questState1[shopId - 5] = 1;
               }

               interactionCount[shopId - 5]++;
               return dialogue[shopId][5 + outcome];
            } else if (action == 3) {
               if (questState2[shopId - 5] != 0) {
                  return dialogue[shopId][4];
               }

               int outcome = player.rollShopOutcome(shopId, action);
               int variant = extra <= 1 ? 0 : 1;
               if (outcome == 0) {
                  questState2[shopId - 5] = 2;
               } else if (outcome == 1) {
                  player.gainSkillExp(13, 2);
                  questState2[shopId - 5] = 2;
               } else if (outcome == 2) {
                  player.gainSkillExp(13, 5);
                  rewardsGiven[shopId - 5]++;
                  questState2[shopId - 5] = 1;
               } else if (outcome == 3) {
                  player.gainSkillExp(13, 8);
                  rewardsGiven[shopId - 5]++;
                  questState2[shopId - 5] = 1;
               }

               interactionCount[shopId - 5]++;
               return dialogue[shopId][9 + variant];
            } else if (action == 4) {
               if (questState1[shopId - 5] != 2 && questState2[shopId - 5] != 2) {
                  int slot = extra;
                  int itemId = Math.abs(player.inventoryItemIds[slot]);
                  if (Item.column(1, itemId) == 11) {
                     int flags = questFlagsFor(shopId, itemId);
                     if (flags > 0) {
                        player.removeInventorySlot(slot);
                        rewardsGiven[shopId - 5] = (short)(rewardsGiven[shopId - 5] + flags);
                        questState1[shopId - 5] = 0;
                        questState2[shopId - 5] = 0;
                     }

                     return dialogue[shopId][11 + flags];
                  }

                  return dialogue[shopId][11];
               }

               return dialogue[shopId][11];
            } else if (action == 5) {
               if (rewardsGiven[shopId - 5] == 0) {
                  return dialogue[shopId][15];
               }

               rewardsGiven[shopId - 5]--;
               int step = extra;
               return rumorFor(player, step);
            } else if (action != 8) {
               return "quack";
            }
         case 4:
            if (action == 1) {
               if (firstVisit[shopId]) {
                  firstVisit[shopId] = false;
                  player.rumorRevealStep = 0;
                  if (showDeathGreeting) {
                     showDeathGreeting = false;
                     return dialogue[4][13] + "\n \n" + dialogue[4][0] + "\n \n" + dialogue[4][1] + "\n \n" + dialogue[4][2];
                  }

                  return dialogue[4][0] + "\n \n" + dialogue[4][1] + "\n \n" + dialogue[4][2];
               } else {
                  int advancement = ESGame.getGameAdvancementLevel(player.giftPointsFound);
                  if (advancement > player.rumorRevealStep) {
                     player.rumorRevealStep++;
                  }

                  if (showDeathGreeting) {
                     showDeathGreeting = false;
                     return dialogue[4][13];
                  }

                  return null;
               }
            } else {
               if (action != 13) {
                  if (action == 10) {
                     player.ailmentMask = 0;
                     return dialogue[shopId][9];
                  }

                  if (action == 11) {
                     if (!player.hasCampMark()) {
                        return dialogue[shopId][10];
                     }

                     player.warpToCampMark();
                     return dialogue[shopId][11];
                  }

                  if (action == 12) {
                     player.coreStats[2] = player.coreStats[3];
                     player.coreStats[4] = player.coreStats[5];
                     return dialogue[shopId][12];
                  }

                  return null;
               }

               int revealedCount = 0;
               int step = 0;

               for (; step < 6; step++) {
                  if (player.eventFlags[90 + step]) {
                     revealedCount++;
                  }
               }

               String result = "";
               if (revealedCount <= player.rumorRevealStep) {
                  result = dialogue[shopId][3 + player.rumorRevealStep];
                  int pick = 0;
                  if (revealedCount < 6) {
                     pick = Util.randomInt(6 - revealedCount) - 1;
                  }

                  for (int i = 0; i < 6; i++) {
                     if (!player.eventFlags[90 + i]) {
                        if (pick == 0) {
                           pick = i;
                           break;
                        }

                        pick--;
                     }
                  }

                  player.eventFlags[90 + pick] = true;
                  result = Util.replace(result, "<TAG>", dialogue[9][5 + RUMOR_STRING_OFFSET[player.traitorIndex][pick]]);
               } else {
                  result = "I have no new rumors.";
               }

               return result;
            }
         default:
            return "quack2";
      }
   }

   // Whether `action` is one of shop `shopId`'s 3 special quest actions.
   // Paired with shopActionCode below. NOT called from dialogue() above
   // (that uses Player's own `c(shopId,action)` skill-check roll
   // instead) -- no confirmed call site found anywhere in what's been
   // traced. Kept since both are clearly a matched pair and may be
   // called from ESGame's menu-building code (not yet read closely).
   static boolean isValidShopAction(int shopId, int action) {
      switch (shopId) {
         case 5:
            switch (action) {
               case 7:
               case 8:
               case 10:
                  return true;
               case 9:
               default:
                  return false;
            }
         case 6:
            switch (action) {
               case 1:
               case 3:
               case 4:
                  return true;
               case 2:
               default:
                  return false;
            }
         case 7:
            switch (action) {
               case 0:
               case 2:
               case 5:
                  return true;
               default:
                  return false;
            }
         case 8:
            switch (action) {
               case 6:
               case 12:
               case 13:
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
         case 5:
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
         case 6:
            switch (choiceIndex) {
               case 0:
                  return 1;
               case 1:
                  return 3;
               case 2:
                  return 4;
               default:
                  return -1;
            }
         case 7:
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
         case 8:
            switch (choiceIndex) {
               case 0:
                  return 6;
               case 1:
                  return 12;
               case 2:
                  return 13;
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
