// Renamed from decompiled/a.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (itemsin.dat / droppeditemsin.dat).
//
// Field-for-field identical layout to dawnstar's Item.java (../../dawnstar/
// src/Item.java) -- itemsin.dat/droppeditemsin.dat are read by the exact
// same column-oriented loader, same 13-entry "gift" flavor-text convention
// for item ids 87-99 (Item.name()[86+i]), same rollLoot/randomGiftItemOfSubtype
// algorithms. The two real differences found diffing against dawnstar's
// decompiled/a.java: specialEffectText here is String[13][2] (each entry
// pre-split into two display lines, e.g. {"Grants level","experience"})
// rather than dawnstar's String[13] single-line array, and the three
// instant-kill scroll entries all read the literal text "Kill monster"
// here instead of dawnstar's "Kills weak/normal/strong monster" -- Stormhold
// doesn't distinguish them by flavor text alone.
import java.io.DataInputStream;
import java.util.Random;

public class Item {
   static int categoryCount;
   static String[] categoryNames;
   static int itemCount;
   static String[] name;
   static byte[] category;
   static byte[] subtype;
   static byte[] questFlags;
   static short[] buyPrice;
   static short[] sellPrice;
   static byte[] equipSlot;

   // Flavor text for the 13 "gift"/special consumable item ids 87-99, in
   // order, each pre-wrapped into up to 2 display lines. Confirmed 1:1
   // against Player's useItem-equivalent switch on those exact ids.
   static String[][] specialEffectText = new String[][]{
      {"Warp to camp", ""},
      {"Cures ailment", ""},
      {"Restores Health", ""},
      {"Restores Magicka", ""},
      {"", ""},
      {"Grants level", "experience"},
      {"Health & Magicka", ""},
      {"Increase harm", ""},
      {"Increase armor", ""},
      {"Safe camping", ""},
      {"Kill monster", ""},
      {"Kill monster", ""},
      {"Kill monster", ""}
   };

   // droppeditemsin.dat: [depthRow][col] loot table, see load().
   static byte[][] lootTable;
   static byte depthRows;
   static short nextSpawnId;

   Item() {
   }

   static short nextSpawnId() {
      nextSpawnId++;
      return nextSpawnId;
   }

   private static int index0(int itemId) {
      return itemId - 1;
   }

   static boolean isEquippable(int itemId) {
      int i = index0(itemId);
      return equipSlot[i] != -1;
   }

   // Category-based gate distinct from isEquippable() (which checks the
   // equipSlot column): true iff itemId's *category* is 1-10. Confirmed
   // call sites: Player's inventory-pickup path (j.h(int), where it gates
   // initializing a per-slot "charge" byte to 3) and Shop's buy-time
   // stacking check (k.java, alongside a `!alreadyOwned` guard). Categories
   // 11 ("gift"), 12, 13 are excluded -- exact semantic name (durability
   // init? "is wearable/carryable equipment category"?) not confidently
   // pinned down; kept close to its literal check rather than guessed.
   static boolean isEquipmentCategory(int itemId) {
      int i = index0(itemId);
      byte cat = category[i];
      switch (cat) {
         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
            return true;
         default:
            return false;
      }
   }

   static int equipSlotOf(int itemId) {
      int i = index0(itemId);
      return equipSlot[i];
   }

   static String nameOf(int itemId) {
      int i = index0(itemId);
      return name[i];
   }

   // column: 1=category,2=subtype,3=questFlags,4=buyPrice,5=sellPrice,6=equipSlot
   static int column(int column, int itemId) {
      int i = index0(itemId);
      short result;
      switch (column) {
         case 1:
            result = category[i];
            break;
         case 2:
            result = subtype[i];
            break;
         case 3:
            result = questFlags[i];
            break;
         case 4:
            result = buyPrice[i];
            break;
         case 5:
            result = sellPrice[i];
            break;
         case 6:
            result = equipSlot[i];
            break;
         default:
            result = -1;
      }

      return result;
   }

   static void load() throws Exception {
      nextSpawnId = 0;
      loadItems();
      loadLootTable();
   }

   private static void loadItems() throws Exception {
      DataInputStream in = Util.openResource("/itemsin.dat");
      categoryCount = in.readShort();
      categoryNames = new String[categoryCount];

      for (int i = 0; i < categoryCount; i++) {
         categoryNames[i] = in.readUTF();
      }

      itemCount = in.readShort();
      name = new String[itemCount];
      category = new byte[itemCount];
      subtype = new byte[itemCount];
      questFlags = new byte[itemCount];
      buyPrice = new short[itemCount];
      sellPrice = new short[itemCount];
      equipSlot = new byte[itemCount];

      for (int i = 0; i < itemCount; i++) {
         name[i] = in.readUTF();
      }

      for (int i = 0; i < itemCount; i++) {
         category[i] = in.readByte();
      }

      for (int i = 0; i < itemCount; i++) {
         subtype[i] = in.readByte();
      }

      for (int i = 0; i < itemCount; i++) {
         questFlags[i] = in.readByte();
      }

      for (int i = 0; i < itemCount; i++) {
         buyPrice[i] = in.readShort();
      }

      for (int i = 0; i < itemCount; i++) {
         sellPrice[i] = in.readShort();
      }

      for (int i = 0; i < itemCount; i++) {
         equipSlot[i] = in.readByte();
      }

      in.close();
   }

   private static void loadLootTable() throws Exception {
      DataInputStream in = Util.openResource("/droppeditemsin.dat");
      short rows = in.readShort();
      depthRows = (byte)rows;
      System.out.println("numTableRows=" + depthRows);
      short cols = in.readShort();
      lootTable = new byte[rows][cols];

      for (int r = 0; r < rows; r++) {
         for (int c = 0; c < cols; c++) {
            lootTable[r][c] = in.readByte();
         }
      }

      in.close();
   }

   // Picks a random item id from category 11 ("gift") whose subtype
   // matches `subtypeWanted` -- used for the camp-warp/special items
   // that share a subtype grouping.
   static int randomGiftItemOfSubtype(Random rng, int subtypeWanted) {
      int first = -1;
      int last = -1;

      for (int i = 0; i < itemCount; i++) {
         if (category[i] == 11 && subtype[i] == (byte)subtypeWanted) {
            if (first == -1) {
               first = i;
            }

            last = i;
         }
      }

      int span = last - first + 1;
      int pick = first + Math.abs(rng.nextInt() % span);
      return 1 + pick;
   }

   // Rolls a loot-table item id for a monster/chest drop at dungeon
   // `depth`, weighted toward rarer rows for higher `bonusRolls` (best of
   // `bonusRolls` percentile samples). Returns a plain item id, or a
   // 2-byte extended id packed as (highByte<<8)|lowByte when the rolled
   // rarity column is 1.
   static int rollLoot(Random rng, int depth, int bonusRolls) {
      int roll = Util.randomInt(rng, 100);
      int best = roll;

      for (int i = 1; i < bonusRolls; i++) {
         roll = Util.randomInt(rng, 100);
         if (roll > best) {
            best = roll;
         }
      }

      roll = best;
      byte rarityCol;
      if (roll <= 64) {
         rarityCol = 0;
      } else if (roll <= 75) {
         rarityCol = 1;
      } else if (roll <= 90) {
         rarityCol = 3;
      } else {
         rarityCol = 4;
      }

      int depthRoll = Util.randomInt(rng, 10);
      depthRoll += depth - 2;
      if (depthRoll > depthRows - 1) {
         depthRoll = depthRows - 1;
      }

      if (depthRoll < 0) {
         depthRoll = 0;
      }

      byte low = lootTable[depthRoll][rarityCol];
      int result = low;
      if (rarityCol == 1) {
         byte high = lootTable[depthRoll][2];
         result |= high << 8;
      }

      return result;
   }

   // The real item *names* of the 13 "gift" items (ids 87-99), in order --
   // distinct from specialEffectText above (which is fixed flavor text, not
   // read from itemsin.dat). Confirmed caller: ESGame.java:547.
   static String[] specialItemNames() {
      String[] result = new String[13];

      for (int i = 0; i < 13; i++) {
         result[i] = name[86 + i];
      }

      return result;
   }
}
