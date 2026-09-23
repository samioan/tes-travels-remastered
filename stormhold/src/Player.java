// Renamed from decompiled/j.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (charin.dat).
//
// NOT a copy of dawnstar's Player.java -- structurally similar in the
// broad strokes (charin.dat class/race/skill templates, coreStats[10],
// attributes[16] as base+bonus pairs, skills[14][3], a 24-slot inventory,
// a 9x5 corridorView + 13-slot visibleObjects wall-occlusion cache, the
// same tileX/tileY/currentLevel/facing + pending*/prev* movement-staging
// field group, the same campLevel/campX/campY/campFacing bookmark group,
// effectDurations[25], a two-format (full/summary) save pair) but every
// name below was derived by reading this file directly and cross-checking
// call sites in Monster.java, Dungeon.java and Shop.java (all already
// renamed), not copied from dawnstar's field letters. Confirmed real
// differences from dawnstar are called out inline and summarized at the
// bottom of this comment.
//
// No field/class-name collision blocker was found here (unlike dawnstar's
// own `j`/Player.java, which needed a hand-trace specifically because
// Vineflower printed bare `j.` references that collided with instance
// fields also named `j`/`a`). This file's `a.`/`b.`/`k.` prefixed calls
// are unambiguous once you notice they're always followed immediately by
// `.methodName(`, which a primitive `byte`/`short` field could never be --
// still transcribed by hand, one member at a time, cross-checked against
// call sites, but for thoroughness rather than because of that specific
// decompiler ambiguity.
//
// KNOWN GAPS, deliberately left as TODO_*/unconfirmed* rather than guessed:
// - `TODO_isInRegion(int,int,int)` (was private `a(int,int,int)`): no
//   confirmed caller found in this file.
// - `unconfirmedIntField` (was `n`, an `int`, part of the save format):
//   no confirmed meaningful read/write site beyond (de)serialization.
// - `unconfirmedFlag2` (was `f`, a `boolean`): reset alongside the status-
//   effect group in resetState() but no other confirmed use.
// - `rumorRevealStep`/`wardenLoreStep` (was `Y`/`m`, both `short`): named
//   from Shop.java's forward references (`player.rumorRevealStep`,
//   `player.TODO_wardenLoreStep`) to the two narrative-progression
//   counters Helga/Varus's dialogue advances -- MEDIUM confidence only;
//   matched to `Y`/`m` because they reset alongside `giftPointsFound`
//   (was `W`) in the same "new character" branch of resetState(), which
//   is consistent but not independently proven for either specific field.
// - `enteredNewLevelZone`/`leftLevelZone` (was `u`/`O`): set in
//   commitMove() around a walkability-transition check; exact purpose
//   (dropped-item auto-loot bookkeeping? roaming-monster cleanup, like
//   dawnstar's own documented movement simplifications?) not pinned down.
// - `staticUnconfirmedInt`/`staticUnconfirmedString` (was `ag`/`X`, both
//   `static`): no confirmed read/write site found in this file at all.
//
// REAL FINDINGS worth flagging for whoever reads this next:
// - `Dungeon.unconfirmedH` (in ../src/Dungeon.java, from the Dungeon rename
//   pass) DOES have a write site after all -- commitMove() sets it `true`
//   on every successful move into a level. Almost certainly dawnstar's own
//   documented "visited" flag; Dungeon.java's own pass just hadn't seen
//   Player.java yet when it flagged this field as never-read/written.
//   Left as `Dungeon.unconfirmedH` here rather than renamed to avoid
//   racing that file's own pass -- rename to `visited` next time
//   Dungeon.java is touched.
// - Shop.java's `TODO_modeFlag` (`isAdjacentToVarus`'s gate,
//   `player.TODO_modeFlag != 1`) is just `player.currentLevel != 1` --
//   Varus can only be talked to from the hub town. Not fixed in Shop.java
//   here to avoid touching a file this pass wasn't asked to edit.
// - `consumeLevelExp()` (was a bare no-arg `d()`) calls
//   `Shop.clearQuestTurnInState()` (was `k.d()`) as a side effect of
//   spending 10 level-exp on a rank-up. A genuinely surprising cross-
//   system coupling -- confirmed by reading both files, not a
//   transcription slip: leveling up resets the 4 quest-shops'
//   quest-turn-in progress.
// - `rollShopOutcome(int shopId, int action)` (was a 2-int-arg `b(int,int)`)
//   reads `Shop.interactionCount[shopId]` (was `k.r[shopId]`) as a
//   threshold in its chance formula -- confirmed via Shop.dialogue()'s own
//   `player.rollShopOutcome(shopId, action)` call sites (actions 2/3, the
//   quest-turn-in steps).
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

public class Player {
   static boolean charDataLoaded = false;
   // Scratch flag used only inside rollOutcome()'s own body (set then read
   // in the same call) -- kept as a static field to match the original
   // exactly rather than converting it to a local, in case anything
   // outside this pass turns out to depend on its persisting momentarily.
   private static boolean lastDefenseRollHit;

   // The 13-slot "what's renderable at each 3D-view object slot this frame"
   // cache -- same role as dawnstar's Player.visibleObjects. Rebuilt by
   // refreshVisibleObjectSlots()/refreshVisibleObjects() every move.
   static Vector visibleObjects = newVisibleObjectsVector();
   private static final Integer SLOT_EMPTY = new Integer(0);
   private static final Integer SLOT_BLOCKED = new Integer(1);
   private static final Integer SLOT_SHADOWED = new Integer(-1);

   static short classCount;
   // Set to classCount's value a SECOND time right after classCount itself
   // (should very likely have been raceNames.length) -- same "redundant
   // count field" shape dawnstar's own CLASS_MAP.md flags as an open
   // question for its own Player.java, kept under the same name here.
   static short classCountRedundant;
   static String[] classNames;
   static String[] raceNames;
   static String[] skillNames;
   // classTemplates[classIndex][13 + 2*skillCount columns]: attributes,
   // classMagickaFactor, classUnknownPair, spell-threshold pairs.
   static short[][] classTemplates;
   static String[] statLabels;
   static String[] attributeNames;
   static short[] skillGoverningAttribute;
   // classStartingItems[classIndex][2]: the 2 starting item ids granted
   // and auto-equipped by grantStartingItems().
   static int[][] classStartingItems = new int[][]{{1, 27}, {7, 27}, {7, 22}, {17, 27}, {12, 22}, {17, 27}, {12, 22}};
   private static final String[] ailmentNames = new String[]{
      "Stone Blood", "Delusions", "Blind", "Vampirism", "Mana Burn", "Grievous Harm", "Terrified", "Haunted"
   };

   public static int staticUnconfirmedInt = -1;
   public static String staticUnconfirmedString = null;

   // Set true when a picked-up dropped item turns out to already be
   // "claimed"/locked (bit 4, mask 0x4) -- read by the UI layer (not yet
   // renamed) to show a locked-item message, same role as dawnstar's
   // MSG_CHEST_LOCKED-style flag.
   static boolean pendingLockedItemFlag = false;

   ESGame gameRef;
   String playerName;
   short classIndex;
   short raceIndex;
   short[] coreStats;
   byte levelUpAttributeFlags;
   int unconfirmedIntField;
   short[] attributes;
   short classMagickaFactor;
   short[] classUnknownPair;
   short[][] skills;
   byte inventoryCount;
   byte[] inventoryItemIds;
   int[] inventoryItemData;
   byte[] equippedItems;
   int knownSpellsMask;
   byte selectedSpellId;
   short giftPointsFound;
   short rumorRevealStep;
   short wardenLoreStep;
   byte ailmentMask;
   short vampirismTimer;
   short manaBurnTimer;
   short terrifiedTimer;
   boolean unconfirmedFlag2;
   byte currentLevel;
   byte tileX;
   byte tileY;
   byte facing;
   byte campLevel;
   byte campX;
   byte campY;
   byte campFacing;
   byte[] effectDurations;
   short lastCombatTargetId;
   short spellArmorBonus;
   boolean increaseHarmBuff = false;
   boolean increaseArmorBuff = false;
   boolean safeCampingBuff = false;
   byte pendingTileX;
   byte pendingTileY;
   byte pendingFacing;
   byte pendingLevel;
   // Read by ESGame's inventory-action handler right after useItem() (to
   // decide whether to return to the game view instead of the inventory
   // menu), but not confirmed set `true` anywhere in useItem()'s 87-99
   // switch -- same "declared, read, never written" status dawnstar's own
   // CLASS_MAP.md documents for its own endOfGameTriggered field.
   boolean endOfGameTriggered;
   boolean crossingLevelBoundary;
   boolean justMarkedCamp;
   boolean enteredNewLevelZone;
   boolean leftLevelZone;
   byte[][] corridorView;
   byte prevTileX;
   byte prevTileY;

   int estimatedSaveSize(boolean full) {
      return full ? 400 : 200;
   }

   public Player(ESGame game) {
      charDataLoaded = false;
      ensureCharDataLoaded();
      this.playerName = null;
      this.coreStats = new short[10];
      this.attributes = new short[16];
      this.classUnknownPair = new short[2];
      this.skills = new short[14][3];
      this.inventoryCount = 0;
      this.inventoryItemIds = new byte[24];
      this.inventoryItemData = new int[24];
      this.equippedItems = new byte[7];
      this.effectDurations = new byte[25];
      this.corridorView = new byte[9][5];
      this.gameRef = game;
      this.justMarkedCamp = false;
   }

   // Resets a scratch 10-element core-stats array to "maxed" values --
   // used by the lightweight summary save format, same role as dawnstar's
   // normalizeForSummary.
   void normalizeToMaxStats(short[] stats) {
      stats[2] = stats[3];
      stats[4] = stats[5];
      stats[6] = stats[7];
      stats[8] = 0;
   }

   void applyClassTemplate(int classIndex) {
      this.classIndex = (short)classIndex;
      this.raceIndex = classTemplates[this.classIndex][1];
      byte attrCount = 8;

      for (int i = 0; i < attrCount; i++) {
         int a = 2 * i;
         this.attributes[a] = classTemplates[this.classIndex][2 + i];
         this.attributes[a + 1] = 0;
      }

      this.classMagickaFactor = classTemplates[this.classIndex][10];
      this.classUnknownPair[0] = classTemplates[this.classIndex][11];
      this.classUnknownPair[1] = classTemplates[this.classIndex][12];
      this.coreStats[0] = 1;
      this.coreStats[1] = 0;
      this.computeDerivedStats();
      this.coreStats[2] = this.coreStats[3];
      this.coreStats[4] = this.coreStats[5];
      this.coreStats[6] = this.coreStats[7];
      this.coreStats[8] = 0;
      this.coreStats[9] = 0;
      this.levelUpAttributeFlags = 0;
      this.unconfirmedIntField = 0;
      int col = 13;

      for (int i = 0; i < 14; i++) {
         this.skills[i][0] = classTemplates[this.classIndex][col++];
         this.skills[i][1] = classTemplates[this.classIndex][col++];
         this.skills[i][2] = 0;
      }

      for (int i = 0; i < 24; i++) {
         this.inventoryItemIds[i] = 0;
         this.inventoryItemData[i] = 0;
      }

      for (int i = 0; i < 7; i++) {
         this.equippedItems[i] = 0;
      }

      this.knownSpellsMask = this.computeStartingSpellMask();
   }

   void computeDerivedStats() {
      this.coreStats[3] = (short)((this.attributes[0] + this.attributes[10]) / 2);
      this.coreStats[5] = (short)(this.classMagickaFactor * this.attributes[2] / 4);
      this.coreStats[7] = (short)(this.attributes[0] + this.attributes[4] + this.attributes[6] + this.attributes[10]);
   }

   // Builds the starting known-spell bitmask from the class template's
   // spell-threshold columns (14 pairs starting at column 13), and picks
   // the first known spell as the initial selectedSpellId. Bucket mapping
   // (skill index 1->bit0, 3->bit5, 4->bit10, 6->bit15, 10->bit20) matches
   // spellSkillIndexFor()'s own bucket boundaries.
   private int computeStartingSpellMask() {
      int mask = 0;
      int col = 13;
      byte firstBit = -1;
      boolean pickFirst = true;

      for (int i = 0; i < 14; i++) {
         short threshold = classTemplates[this.classIndex][col++];
         short rank = classTemplates[this.classIndex][col++];
         switch (i) {
            case 1:
               firstBit = 0;
               break;
            case 2:
            case 5:
            case 7:
            case 8:
            case 9:
            default:
               firstBit = -1;
               break;
            case 3:
               firstBit = 5;
               break;
            case 4:
               firstBit = 10;
               break;
            case 6:
               firstBit = 15;
               break;
            case 10:
               firstBit = 20;
         }

         if (firstBit != -1 && rank > 0) {
            mask |= 1 << firstBit;
            if (pickFirst) {
               this.selectedSpellId = (byte)(firstBit + 1);
               pickFirst = false;
            }
         }
      }

      return mask;
   }

   public void resetState(int classIndex) {
      this.resetState(classIndex, false);
   }

   // NOTE: `classIndex` is accepted but never actually used in this body --
   // confirmed by reading the whole method, not an omission in this pass.
   // The real game calls applyClassTemplate(int) as a separate step; this
   // method only resets movement/status/camp state (and, for a brand-new
   // character, position/starting items).
   public void resetState(int classIndex, boolean full) {
      if (!full) {
         this.giftPointsFound = 0;
         this.rumorRevealStep = 0;
         this.wardenLoreStep = 0;
      }

      this.ailmentMask = 0;
      this.vampirismTimer = 0;
      this.manaBurnTimer = 0;
      this.terrifiedTimer = 0;
      this.unconfirmedFlag2 = false;
      this.setHubSpawnPosition(full);
      this.refreshCorridorView();
      if (!full) {
         this.campLevel = 0;
         this.campX = 0;
         this.campY = 0;
         this.campFacing = 0;
      }

      for (int i = 0; i < 25; i++) {
         this.effectDurations[i] = 0;
      }

      this.lastCombatTargetId = 0;
      this.spellArmorBonus = 0;
      this.increaseHarmBuff = false;
      this.increaseArmorBuff = false;
      this.safeCampingBuff = false;
      if (!full) {
         this.grantStartingItems();
      }
   }

   // isRespawn=false: new-character hub spawn (9,10). isRespawn=true: the
   // death/respawn hub spawn point is DIFFERENT (12,14) -- a real
   // Stormhold-specific detail dawnstar's own M11 port milestone (a single
   // hub-town spawn position) doesn't have.
   private void setHubSpawnPosition(boolean isRespawn) {
      if (!isRespawn) {
         this.currentLevel = this.pendingLevel = 1;
         this.tileX = this.pendingTileX = 9;
         this.tileY = this.pendingTileY = 10;
         this.facing = this.pendingFacing = 1;
      } else {
         this.currentLevel = this.pendingLevel = 1;
         this.tileX = this.pendingTileX = 12;
         this.tileY = this.pendingTileY = 14;
         this.facing = this.pendingFacing = 1;
      }
   }

   // Compact ~300-char character summary (name/level/HP/Magicka/Fatigue/
   // attributes/skills-with-rank).
   String characterSummaryShort() {
      StringBuffer out = new StringBuffer(300);
      String sp = " ";
      String colon = ": ";
      out.append(raceNames[this.raceIndex]);
      out.append(sp);
      out.append(classNames[this.classIndex]);
      out.append('\n');
      out.append(statLabels[0]);
      out.append(colon);
      out.append(this.coreStats[0]);
      out.append('\n');
      out.append(statLabels[2]);
      out.append(colon);
      out.append(this.effectiveStat(2));
      out.append('\n');
      out.append(statLabels[4]);
      out.append(colon);
      out.append(this.effectiveStat(4));
      out.append('\n');
      out.append(statLabels[6]);
      out.append(colon);
      out.append(this.effectiveStat(6));
      out.append('\n');

      for (int i = 0; i < 8; i++) {
         int a = 2 * i;
         out.append(attributeNames[a]);
         out.append(colon);
         out.append(this.attributes[a]);
         out.append('\n');
      }

      for (int i = 0; i < 14; i++) {
         if (this.skills[i][0] > 0) {
            out.append(skillNames[i]);
            out.append(colon);
            out.append(this.skills[i][0]);
            out.append('\n');
         }
      }

      return out.toString();
   }

   static void ensureCharDataLoaded() {
      if (!charDataLoaded) {
         try {
            loadCharacterData("/charin.dat");
            charDataLoaded = true;
         } catch (Exception e) {
            System.out.println("Error: could not load character data");
            System.out.println("Exception: " + e);
         }
      }
   }

   private static void loadCharacterData(String path) throws Exception {
      DataInputStream in = ESGame.getResource(path);
      int available = in.available();
      statLabels = readStringArray(in);
      attributeNames = readStringArray(in);
      classNames = readStringArray(in);
      classCount = (short)classNames.length;
      raceNames = readStringArray(in);
      classCountRedundant = (short)classNames.length;
      skillNames = readStringArray(in);
      short skillCount = (short)skillNames.length;
      if (skillCount != 14) {
         throw new Exception("Error: mismatch between input number of skill types and that specified in code");
      }

      skillGoverningAttribute = new short[skillCount];

      for (int i = 0; i < skillCount; i++) {
         skillGoverningAttribute[i] = in.readShort();
      }

      int width = 13 + 2 * skillCount;
      classTemplates = new short[classCount][width];

      for (int c = 0; c < classCount; c++) {
         for (int col = 0; col < width; col++) {
            classTemplates[c][col] = in.readShort();
         }
      }
   }

   private static String[] readStringArray(DataInputStream in) throws Exception {
      short count = in.readShort();
      String[] out = new String[count];

      for (int i = 0; i < count; i++) {
         out[i] = in.readUTF();
      }

      return out;
   }

   // Save-format reader. full=true: the complete in-progress save
   // (everything). full=false: the lightweight "character summary" format
   // (re-derives class template + starting items/spells fresh, layers the
   // serialized summary fields on top) -- same two-format split as
   // dawnstar's Player.fromBytes.
   static Player fromBytes(byte[] data, boolean full) throws Exception {
      DataInputStream in = new DataInputStream(new ByteArrayInputStream(data, 0, data.length));
      Player p = new Player(null);
      p.playerName = in.readUTF();
      p.classIndex = in.readShort();
      if (!full) {
         p.applyClassTemplate(p.classIndex);
         p.resetState(p.classIndex);
      }

      p.raceIndex = in.readShort();

      for (int i = 0; i < 10; i++) {
         p.coreStats[i] = in.readShort();
      }

      if (full) {
         p.levelUpAttributeFlags = in.readByte();
      }

      p.unconfirmedIntField = in.readInt();

      for (int i = 0; i < 16; i++) {
         p.attributes[i] = in.readShort();
      }

      p.classMagickaFactor = in.readShort();
      p.classUnknownPair[0] = in.readShort();
      p.classUnknownPair[1] = in.readShort();

      for (int i = 0; i < 14; i++) {
         for (int c = 0; c < 3; c++) {
            p.skills[i][c] = in.readShort();
         }
      }

      if (full) {
         p.inventoryCount = in.readByte();

         for (int i = 0; i < 24; i++) {
            p.inventoryItemIds[i] = in.readByte();
         }

         for (int i = 0; i < 24; i++) {
            p.inventoryItemData[i] = in.readInt();
         }

         for (int i = 0; i < 7; i++) {
            p.equippedItems[i] = in.readByte();
         }

         p.knownSpellsMask = in.readInt();
         p.selectedSpellId = in.readByte();
      } else {
         p.knownSpellsMask = in.readInt();
      }

      if (full) {
         p.giftPointsFound = in.readShort();
         p.rumorRevealStep = in.readShort();
         p.wardenLoreStep = in.readShort();
         p.ailmentMask = in.readByte();
         p.vampirismTimer = in.readShort();
         p.manaBurnTimer = in.readShort();
         p.terrifiedTimer = in.readShort();
         p.unconfirmedFlag2 = in.readBoolean();
         p.currentLevel = in.readByte();
         p.tileX = in.readByte();
         p.tileY = in.readByte();
         p.facing = in.readByte();
         p.campLevel = in.readByte();
         p.campX = in.readByte();
         p.campY = in.readByte();
         p.campFacing = in.readByte();

         for (int i = 0; i < 25; i++) {
            p.effectDurations[i] = in.readByte();
         }

         p.lastCombatTargetId = in.readShort();
         p.spellArmorBonus = in.readShort();
         p.increaseHarmBuff = in.readBoolean();
         p.increaseArmorBuff = in.readBoolean();
         p.safeCampingBuff = in.readBoolean();
      }

      return p;
   }

   byte[] toBytes(boolean full) throws Exception {
      int hint = this.estimatedSaveSize(full);
      ByteArrayOutputStream buf = new ByteArrayOutputStream(hint);
      DataOutputStream out = new DataOutputStream(buf);
      out.writeUTF(this.playerName);
      out.writeShort(this.classIndex);
      out.writeShort(this.raceIndex);
      if (full) {
         for (int i = 0; i < 10; i++) {
            out.writeShort(this.coreStats[i]);
         }

         out.writeByte(this.levelUpAttributeFlags);
      } else {
         short[] scratch = new short[10];

         for (int i = 0; i < 10; i++) {
            scratch[i] = this.coreStats[i];
         }

         this.normalizeToMaxStats(scratch);

         for (int i = 0; i < 10; i++) {
            out.writeShort(scratch[i]);
         }
      }

      out.writeInt(this.unconfirmedIntField);

      for (int i = 0; i < 16; i++) {
         out.writeShort(this.attributes[i]);
      }

      out.writeShort(this.classMagickaFactor);
      out.writeShort(this.classUnknownPair[0]);
      out.writeShort(this.classUnknownPair[1]);

      for (int i = 0; i < 14; i++) {
         for (int c = 0; c < 3; c++) {
            out.writeShort(this.skills[i][c]);
         }
      }

      if (full) {
         out.writeByte(this.inventoryCount);

         for (int i = 0; i < 24; i++) {
            out.writeByte(this.inventoryItemIds[i]);
         }

         for (int i = 0; i < 24; i++) {
            out.writeInt(this.inventoryItemData[i]);
         }

         for (int i = 0; i < 7; i++) {
            out.writeByte(this.equippedItems[i]);
         }

         out.writeInt(this.knownSpellsMask);
         out.writeByte(this.selectedSpellId);
      } else {
         int mask = this.computeStartingSpellMask();
         out.writeInt(mask);
      }

      if (full) {
         out.writeShort(this.giftPointsFound);
         out.writeShort(this.rumorRevealStep);
         out.writeShort(this.wardenLoreStep);
         out.writeByte(this.ailmentMask);
         out.writeShort(this.vampirismTimer);
         out.writeShort(this.manaBurnTimer);
         out.writeShort(this.terrifiedTimer);
         out.writeBoolean(this.unconfirmedFlag2);
         out.writeByte(this.currentLevel);
         out.writeByte(this.tileX);
         out.writeByte(this.tileY);
         out.writeByte(this.facing);
         out.writeByte(this.campLevel);
         out.writeByte(this.campX);
         out.writeByte(this.campY);
         out.writeByte(this.campFacing);

         for (int i = 0; i < 25; i++) {
            out.writeByte(this.effectDurations[i]);
         }

         out.writeShort(this.lastCombatTargetId);
         out.writeShort(this.spellArmorBonus);
         out.writeBoolean(this.increaseHarmBuff);
         out.writeBoolean(this.increaseArmorBuff);
         out.writeBoolean(this.safeCampingBuff);
      }

      return buf.toByteArray();
   }

   // Computes pendingTileX/Y/Level/Facing from currentLevel/tileX/tileY/
   // facing for a step in compass direction `dir` (1=N,2=E,3=S,4=W,
   // fallthrough switch preserved exactly), including cross-level boundary
   // stitching via Dungeon.neighbors[] (was `.x[]`) and the hub-town
   // (19x19) <-> standard-level (35x35) recentering math.
   void computeMoveTarget(int dir) {
      byte step = -1;
      switch (dir) {
         case 1:
            step = 1;
         case 2:
            this.pendingFacing = this.facing;
            if (this.facing == 1) {
               this.pendingTileX = this.tileX;
               this.pendingTileY = (byte)(this.tileY - step);
            } else if (this.facing == 3) {
               this.pendingTileX = this.tileX;
               this.pendingTileY = (byte)(this.tileY + step);
            } else if (this.facing == 2) {
               this.pendingTileX = (byte)(this.tileX + step);
               this.pendingTileY = this.tileY;
            } else if (this.facing == 4) {
               this.pendingTileX = (byte)(this.tileX - step);
               this.pendingTileY = this.tileY;
            }

            Dungeon level = ESGame.dungeons[this.currentLevel - 1];
            if (this.pendingTileX < 0) {
               this.crossingLevelBoundary = true;
               this.pendingLevel = level.neighbors[3];
               Dungeon neighbor = ESGame.dungeons[this.pendingLevel - 1];
               if (this.pendingLevel != 1 && this.currentLevel != 1) {
                  this.pendingTileX = (byte)(neighbor.width - 1);
               } else {
                  this.pendingTileX = (byte)(neighbor.width - 1);
                  this.pendingTileY = (byte)(this.pendingTileY + (neighbor.height - level.height) / 2);
               }
            } else if (this.pendingTileX >= level.width) {
               this.crossingLevelBoundary = true;
               this.pendingLevel = level.neighbors[1];
               Dungeon neighbor = ESGame.dungeons[this.pendingLevel - 1];
               if (this.pendingLevel != 1 && this.currentLevel != 1) {
                  this.pendingTileX = 0;
               } else {
                  this.pendingTileX = 0;
                  this.pendingTileY = (byte)(this.pendingTileY + (neighbor.height - level.height) / 2);
               }
            } else if (this.pendingTileY < 0) {
               this.crossingLevelBoundary = true;
               this.pendingLevel = level.neighbors[0];
               Dungeon neighbor = ESGame.dungeons[this.pendingLevel - 1];
               if (this.pendingLevel != 1 && this.currentLevel != 1) {
                  this.pendingTileY = (byte)(neighbor.height - 1);
               } else {
                  this.pendingTileX = (byte)(this.pendingTileX + (neighbor.width - level.width) / 2);
                  this.pendingTileY = (byte)(neighbor.height - 1);
               }
            } else if (this.pendingTileY >= level.height) {
               this.crossingLevelBoundary = true;
               this.pendingLevel = ESGame.dungeons[this.currentLevel - 1].neighbors[2];
               Dungeon neighbor = ESGame.dungeons[this.pendingLevel - 1];
               if (this.pendingLevel != 1 && this.currentLevel != 1) {
                  this.pendingTileY = 0;
               } else {
                  this.pendingTileX = (byte)(this.pendingTileX + (neighbor.width - level.width) / 2);
                  this.pendingTileY = 0;
               }
            } else {
               this.crossingLevelBoundary = false;
               this.pendingLevel = this.currentLevel;
            }
            break;
         case 3:
            this.pendingLevel = this.currentLevel;
            this.crossingLevelBoundary = false;
            this.pendingFacing = (byte)(this.facing + 1);
            if (this.pendingFacing > 4) {
               this.pendingFacing = 1;
            }

            this.pendingTileX = this.tileX;
            this.pendingTileY = this.tileY;
            break;
         case 4:
            this.pendingLevel = this.currentLevel;
            this.crossingLevelBoundary = false;
            this.pendingFacing = (byte)(this.facing - 1);
            if (this.pendingFacing < 1) {
               this.pendingFacing = 4;
            }

            this.pendingTileX = this.tileX;
            this.pendingTileY = this.tileY;
      }
   }

   // move(dir, strafe): dir 1=forward,2=backward for a plain step; when
   // strafe is true, dir 3/4 are strafe-left/right (turn, step, turn back)
   // -- same turn/step/turn-back pattern as dawnstar's Player.move.
   boolean move(int dir, boolean strafe) {
      Dungeon level = this.currentDungeon();
      level.refreshNearbyMonsterFlags(this.tileX, this.tileY);
      if (this.coreStats[6] <= 0) {
         return false;
      } else if (strafe && dir == 4) {
         this.commitMove(4);
         boolean result = this.commitMove(1);
         boolean savedCrossing = this.crossingLevelBoundary;
         result = this.commitMove(3);
         this.crossingLevelBoundary = savedCrossing;
         return result;
      } else if (strafe && dir == 3) {
         this.commitMove(3);
         boolean result = this.commitMove(1);
         boolean savedCrossing = this.crossingLevelBoundary;
         result = this.commitMove(4);
         this.crossingLevelBoundary = savedCrossing;
         return result;
      } else {
         return this.commitMove(dir);
      }
   }

   boolean commitMove(int dir) {
      if (this.coreStats[6] <= 0) {
         return false;
      }

      if (dir == 0) {
         return false;
      }

      this.computeMoveTarget(dir);
      boolean isStep = dir == 1 || dir == 2;
      if (this.pendingLevel <= 0) {
         return false;
      }

      Dungeon target = ESGame.dungeons[this.pendingLevel - 1];
      if (!target.populated) {
         return false;
      }

      byte tileBits = target.tiles[this.pendingTileX][this.pendingTileY];
      if (!this.isWalkableTileBits(tileBits)) {
         return false;
      }

      if (this.pendingLevel == 37 && this.currentLevel != 37) {
         Enumeration e = ESGame.monsters[this.pendingLevel - 1].elements();

         while (e.hasMoreElements()) {
            byte[] data = (byte[])e.nextElement();
            Monster m = Monster.fromBytesShared(data);
            if (m.typeIndex == 41) {
               m.currentHp = (byte)m.stat(14);
               m.store();
            }
         }
      }

      Dungeon oldLevel = ESGame.dungeons[this.currentLevel - 1];
      boolean oldWalkable = oldLevel.isWalkable(this.tileX, this.tileY);
      boolean newWalkable = target.isWalkable(this.pendingTileX, this.pendingTileY);
      this.enteredNewLevelZone = !oldWalkable && newWalkable;
      this.leftLevelZone = oldWalkable && !newWalkable;
      this.currentLevel = this.pendingLevel;
      this.prevTileX = this.tileX;
      this.prevTileY = this.tileY;
      this.tileX = this.pendingTileX;
      this.tileY = this.pendingTileY;
      this.facing = this.pendingFacing;
      target.unconfirmedH = true;
      if (dir == 1 || dir == 2) {
         if (Shop.wardenPresent) {
            Shop.wardenPresent = false;
         }

         this.coreStats[6] = (short)(this.coreStats[6] - 1 * this.fatigueCostMultiplier());
         this.coreStats[6] = (short)Math.max(this.coreStats[6], 0);
      }

      boolean hasDroppedItem = (tileBits & 4) != 0;
      if (hasDroppedItem) {
         int count = target.countDroppedItemsAt(this.tileX, this.tileY);
         if (count == 1) {
            byte[] record = target.firstDroppedItemAt(this.tileX, this.tileY);
            if ((record[6] & 4) != 0) {
               pendingLockedItemFlag = true;
               this.refreshCorridorView();
               return true;
            }

            System.out.println(">>>>>>Found a dropped item item: " + describeDroppedItemRecord(record));
            boolean picked = this.tryPickUpItem(record);
            if (picked) {
               target.removeDroppedItem(record);
               if ((record[6] & 2) == 0) {
                  System.out.println("Dropped item not possessed before");
                  int itemIndex = record[2] - 1;
                  System.out.println("item index=" + itemIndex);
                  if (Item.column(1, itemIndex + 1) == 11) {
                     // Adds the item's SUBTYPE column (Item.column(2,...))
                     // to giftPointsFound -- not the magnitude/questFlags
                     // column (3) armorValue()/itemTooltip() use elsewhere
                     // in this file. Confirmed against the original's
                     // direct `a.c[idx]` field read (subtype), not a call
                     // through `a.a(col,id)`.
                     this.giftPointsFound = (short)(this.giftPointsFound + Item.column(2, itemIndex + 1));
                     int advancement = ESGame.getGameAdvancementLevel(this.giftPointsFound);
                     this.gameRef.checkOpenAndPopulateDungeons(advancement);
                  }
               }
            }
         } else if (count > 1) {
            System.out.println("Found several items in square");
            Vector items = target.droppedItemsAt(this.tileX, this.tileY);
            Enumeration e = items.elements();

            while (e.hasMoreElements()) {
               byte[] record = (byte[])e.nextElement();
               if ((record[6] & 4) != 0) {
                  pendingLockedItemFlag = true;
                  this.refreshCorridorView();
                  return true;
               }

               boolean picked = this.tryPickUpItem(record);
               if (picked) {
                  target.removeDroppedItem(record);
                  if ((record[6] & 2) != 0) {
                     int itemIndex = record[2] - 1;
                     System.out.println("item index=" + itemIndex);
                     if (Item.column(1, itemIndex + 1) == 11) {
                        this.giftPointsFound = (short)(this.giftPointsFound + Item.column(2, itemIndex + 1));
                        this.gameRef.checkOpenAndPopulateDungeons(ESGame.getGameAdvancementLevel(this.giftPointsFound));
                     }
                  }
               }
            }
         }
      }

      this.refreshCorridorView();
      if (isStep && (tileBits & 8) != 0) {
         this.autoMarkCampOnTile();
      }

      return true;
   }

   // Not-wall, not-monster-occupied, not the special bit-3 tile, not
   // blocked(bit5) -- matches Dungeon.isWalkable()'s own bit reading.
   boolean isWalkableTileBits(byte tileBits) {
      if ((tileBits & 1) != 0) {
         return false;
      } else {
         return (tileBits & 32) != 0 ? false : (tileBits & 2) == 0;
      }
   }

   // Player attacks `target`: rolls attackPower/attackAccuracy vs. the
   // monster's stats, applies rollOutcome's 4-tier result, deals damage,
   // and grants activeWeaponSkillIndex() exp on a strong hit.
   void attack(Monster target) {
      this.lastCombatTargetId = target.spawnId;
      byte type = target.typeIndex;
      int offense = this.attackPower(true);
      int defense = target.stat(7);
      int diff = offense - defense;
      diff = Math.min(diff, target.stat(2));
      if (this.isEffectActive(10)) {
         if (target.scratch[8] == 0) {
            this.clearEffect(10);
         } else {
            diff += target.scratch[8];
         }
      }

      int monsterDefendChance = target.stat(6) - diff * 5;
      int playerAttackChance = this.attackAccuracy() + diff * 5;
      monsterDefendChance = Math.min(Math.max(monsterDefendChance, 10), 95);
      playerAttackChance = Math.min(Math.max(playerAttackChance, 10), 95);
      int tier = rollOutcome(playerAttackChance, monsterDefendChance);
      if (tier != 0) {
         int power = this.weaponDamage();
         int armor = target.stat(8);
         if (this.isEffectActive(13)) {
            if (target.scratch[5] == 0) {
               this.effectDurations[12] = 0;
            } else {
               armor -= target.scratch[5];
            }
         }

         if (tier == 1) {
            armor = 2 * armor;
         } else if (tier == 3) {
            power = 2 * power;
         }

         int dmg = power - armor;
         dmg = Math.max(dmg, 4);
         int scaled = dmg * target.stat(14) / 100;
         target.takeDamage(scaled);
         target.store();
         if (this.isEffectActive(7)) {
            if (target.scratch[1] == 0) {
               this.clearEffect(7);
            } else {
               int raw = target.scratch[1];
               raw = Math.max(raw, 4);
               scaled = raw * target.stat(14) / 100;
               target.takeDamage(scaled);
            }
         }

         if (tier >= 2) {
            this.gainSkillExp(this.activeWeaponSkillIndex(), 1);
         }

         if (!this.isEffectActive(7)) {
            this.coreStats[6] = (short)(this.coreStats[6] - 7 * this.fatigueCostMultiplier());
            this.coreStats[6] = (short)Math.max(this.coreStats[6], 0);
         }

         if (this.hasAilment(6)) {
            int selfDmg = 2 * this.coreStats[3] / 100;
            if (selfDmg < 1) {
               selfDmg = 1;
            }

            this.coreStats[2] = (short)(this.coreStats[2] - (short)selfDmg);
         }
      }
   }

   // Distance from this player to `target` (a 28-byte packed Monster
   // record or a 2-byte [x,y] pair) along the axis this player is facing,
   // used by the visible-object slot resolver. Negative-along-facing-axis
   // distances are treated as "not visible" (Integer.MAX_VALUE).
   int facingAxisDistance(Object target) {
      byte tx;
      byte ty;
      byte[] data = (byte[])target;
      if (data.length == 28) {
         Monster m = Monster.fromBytesShared(data);
         tx = m.tileX;
         ty = m.tileY;
      } else {
         tx = data[0];
         ty = data[1];
      }

      int dist;
      if (this.facing == 1) {
         dist = this.tileY - ty;
      } else if (this.facing == 3) {
         dist = ty - this.tileY;
      } else if (this.facing == 2) {
         dist = tx - this.tileX;
      } else {
         dist = this.tileX - tx;
      }

      if (dist < 0) {
         dist = Integer.MAX_VALUE;
      }

      return dist;
   }

   // kind: 1=monster, 2=dropped item. Marks the object's own record as
   // "seen"/visible (monster.unconfirmedFlag=true / dropped-item bit0) and
   // persists it.
   void placeVisibleObject(int kind, Object obj) {
      byte[] data;
      switch (kind) {
         case 1:
            data = (byte[])obj;
            data[6] = 1;
            Monster m = Monster.fromBytesShared(data);
            m.unconfirmedFlag = true;
            m.store();
            break;
         case 2:
            data = (byte[])obj;
            data[6] = (byte)(data[6] | 1);
      }
   }

   // Rebuilds visibleObjects for the current tile: refreshes the 13 slots'
   // wall-occlusion state (refreshVisibleObjectSlots), then places every
   // dropped item, chest, and monster on the current level into a slot if
   // one resolves for it, plus (Stormhold-only) the Warden NPC when
   // present in the hub town.
   void refreshVisibleObjects(boolean includeWarden) {
      this.refreshVisibleObjectSlots();
      Vector dropped = ESGame.droppedItems[this.currentLevel - 1];
      if (dropped != null) {
         Enumeration e = dropped.elements();

         while (e.hasMoreElements()) {
            byte[] data = (byte[])e.nextElement();
            this.placeVisibleObjectIfSlotFree(2, data, includeWarden);
         }
      }

      Hashtable chests = ESGame.chests[this.currentLevel - 1];
      if (chests != null) {
         Enumeration e = chests.elements();

         while (e.hasMoreElements()) {
            byte[] data = (byte[])e.nextElement();
            this.placeVisibleObjectIfSlotFree(4, data, includeWarden);
         }
      }

      Hashtable monsters = ESGame.monsters[this.currentLevel - 1];
      if (monsters != null) {
         Enumeration e = monsters.elements();

         while (e.hasMoreElements()) {
            byte[] data = (byte[])e.nextElement();
            this.placeVisibleObjectIfSlotFree(1, data, includeWarden);
         }
      }

      if (this.currentLevel == 1 && Shop.wardenPresent) {
         this.resolveVisibleObjectSlot(5, "W");
      }
   }

   boolean placeVisibleObjectIfSlotFree(int kind, Object obj, boolean unused) {
      int dist = this.facingAxisDistance(obj);
      boolean resolved = this.resolveVisibleObjectSlot(kind, obj);
      if (!resolved) {
         return false;
      } else if (kind != 4 && dist != 1) {
         this.placeVisibleObject(kind, obj);
         return true;
      } else {
         this.placeVisibleObject(kind, obj);
         return true;
      }
   }

   // Resets the 13-slot visibleObjects cache to SLOT_EMPTY, then samples
   // corridorView through Dungeon.viewGridAt() (was `i.a(dx,depth,grid)`)
   // at 13 fixed relative positions to mark which slots are directly
   // wall-blocked (SLOT_BLOCKED); a fixed cascade of "if slot N is
   // blocked, these other slots are also effectively blocked (SLOT_SHADOWED)"
   // rules follows, e.g. the center-front slot being blocked implies
   // several side slots are too.
   void refreshVisibleObjectSlots() {
      Dungeon level = this.currentDungeon();

      for (int i = 0; i < 13; i++) {
         visibleObjects.setElementAt(SLOT_EMPTY, i);
      }

      byte v;
      v = level.viewGridAt(-1, 1, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 0);
      }

      v = level.viewGridAt(0, 1, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 1);
      }

      v = level.viewGridAt(1, 1, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 2);
      }

      v = level.viewGridAt(-2, 2, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 3);
      }

      v = level.viewGridAt(-1, 2, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 4);
      }

      v = level.viewGridAt(0, 2, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 5);
      }

      v = level.viewGridAt(1, 2, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 6);
      }

      v = level.viewGridAt(2, 2, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 7);
      }

      v = level.viewGridAt(-2, 3, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 8);
      }

      v = level.viewGridAt(-1, 3, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 9);
      }

      v = level.viewGridAt(0, 3, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 10);
      }

      v = level.viewGridAt(1, 3, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 11);
      }

      v = level.viewGridAt(2, 3, this.corridorView);
      if (Util.testBit((byte)1, v)) {
         visibleObjects.setElementAt(SLOT_BLOCKED, 12);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(0))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 4);
         visibleObjects.setElementAt(SLOT_SHADOWED, 8);
         visibleObjects.setElementAt(SLOT_SHADOWED, 9);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(1))) {
         for (int i = 0; i < 13; i++) {
            if (i != 1) {
               visibleObjects.setElementAt(SLOT_SHADOWED, i);
            }
         }
      }

      if (isBlockedSentinel(visibleObjects.elementAt(2))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 6);
         visibleObjects.setElementAt(SLOT_SHADOWED, 11);
         visibleObjects.setElementAt(SLOT_SHADOWED, 12);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(3))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 8);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(4))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 8);
         visibleObjects.setElementAt(SLOT_SHADOWED, 9);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(5))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 9);
         visibleObjects.setElementAt(SLOT_SHADOWED, 10);
         visibleObjects.setElementAt(SLOT_SHADOWED, 11);
         visibleObjects.setElementAt(SLOT_SHADOWED, 4);
         visibleObjects.setElementAt(SLOT_SHADOWED, 6);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(6))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 11);
         visibleObjects.setElementAt(SLOT_SHADOWED, 12);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(7))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 12);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(9))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 8);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(10))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 9);
         visibleObjects.setElementAt(SLOT_SHADOWED, 11);
      }

      if (isBlockedSentinel(visibleObjects.elementAt(11))) {
         visibleObjects.setElementAt(SLOT_SHADOWED, 12);
      }
   }

   // kind: 1=monster(4/5-index xy),4=chest(0/1-index xy),5=Warden (fixed
   // Shop.shopX/Y[6]), else=dropped item(0/1-index xy). **Phase-3 M27
   // correction:** an earlier pass's comment here had the kind==4 and
   // else branches' own labels SWAPPED (said "4=dropped-item"/
   // "else=chest") -- the real callers are refreshVisibleObjects()'s
   // placeVisibleObjectIfSlotFree(2, ...) for dropped items (landing in
   // the else branch) and placeVisibleObjectIfSlotFree(4, ...) for
   // chests (landing in the kind==4 branch), confirmed by reading those
   // call sites directly. Purely a doc-comment bug in this port's own
   // earlier transcription, not an original-game one, and functionally
   // inert either way -- both branches read data[0]/data[1], and
   // dropped-item/chest records both confirmed ([0]/[1]=x/y, M12/M16) to
   // use those same offsets, so the swap never changed behavior. Fixed
   // here rather than just flagged, same discipline M22's own real
   // mapping-bug fixes used. Computes the object's relative view offset
   // via Dungeon.relativeViewOffset() and resolves it to one of the 13
   // visibleObjects slots via a fixed position cascade, each guarded by
   // "and none of these other slots are already occupied" checks
   // (occlusion priority).
   boolean resolveVisibleObjectSlot(int kind, Object obj) {
      Dungeon level = this.currentDungeon();
      byte ox;
      byte oy;
      if (kind == 1) {
         byte[] data = (byte[])obj;
         ox = data[4];
         oy = data[5];
      } else if (kind == 4) {
         byte[] data = (byte[])obj;
         ox = data[0];
         oy = data[1];
      } else if (kind == 5) {
         ox = Shop.SHOP_X[6];
         oy = Shop.SHOP_Y[6];
      } else {
         byte[] data = (byte[])obj;
         ox = data[0];
         oy = data[1];
      }

      int[] offset = level.relativeViewOffset(this.tileX, this.tileY, this.facing, ox, oy);
      boolean resolved = false;
      if (coordsEqual(offset[0], offset[1], 3, 2)) {
         resolved = true;
         visibleObjects.setElementAt(obj, 1);
      } else if (coordsEqual(offset[0], offset[1], 2, 1)) {
         if (!isBlockedSentinel(visibleObjects.elementAt(0))
            && !isBlockedSentinel(visibleObjects.elementAt(1))
            && !isBlockedSentinel(visibleObjects.elementAt(5))) {
            resolved = true;
            visibleObjects.setElementAt(obj, 4);
         }
      } else if (coordsEqual(offset[0], offset[1], 3, 1)) {
         if (!isBlockedSentinel(visibleObjects.elementAt(1))) {
            resolved = true;
            visibleObjects.setElementAt(obj, 5);
         }
      } else if (coordsEqual(offset[0], offset[1], 4, 1)) {
         if (!isBlockedSentinel(visibleObjects.elementAt(1))
            && !isBlockedSentinel(visibleObjects.elementAt(2))
            && !isBlockedSentinel(visibleObjects.elementAt(5))) {
            resolved = true;
            visibleObjects.setElementAt(obj, 6);
         }
      } else if (coordsEqual(offset[0], offset[1], 1, 0)) {
         if (!isBlockedSentinel(visibleObjects.elementAt(0))
            && !isBlockedSentinel(visibleObjects.elementAt(1))
            && !isBlockedSentinel(visibleObjects.elementAt(3))
            && !isBlockedSentinel(visibleObjects.elementAt(4))
            && !isBlockedSentinel(visibleObjects.elementAt(9))) {
            resolved = true;
            visibleObjects.setElementAt(obj, 8);
         }
      } else if (coordsEqual(offset[0], offset[1], 2, 0)) {
         if (!isBlockedSentinel(visibleObjects.elementAt(0))
            && !isBlockedSentinel(visibleObjects.elementAt(1))
            && !isBlockedSentinel(visibleObjects.elementAt(4))
            && !isBlockedSentinel(visibleObjects.elementAt(5))
            && !isBlockedSentinel(visibleObjects.elementAt(10))) {
            resolved = true;
            visibleObjects.setElementAt(obj, 9);
         }
      } else if (coordsEqual(offset[0], offset[1], 3, 0)) {
         if (!isBlockedSentinel(visibleObjects.elementAt(1)) && !isBlockedSentinel(visibleObjects.elementAt(5))) {
            resolved = true;
            visibleObjects.setElementAt(obj, 10);
         }
      } else if (coordsEqual(offset[0], offset[1], 4, 0)) {
         if (!isBlockedSentinel(visibleObjects.elementAt(1))
            && !isBlockedSentinel(visibleObjects.elementAt(2))
            && !isBlockedSentinel(visibleObjects.elementAt(5))
            && !isBlockedSentinel(visibleObjects.elementAt(6))
            && !isBlockedSentinel(visibleObjects.elementAt(10))) {
            resolved = true;
            visibleObjects.setElementAt(obj, 11);
         }
      } else if (coordsEqual(offset[0], offset[1], 5, 0)
         && !isBlockedSentinel(visibleObjects.elementAt(1))
         && !isBlockedSentinel(visibleObjects.elementAt(2))
         && !isBlockedSentinel(visibleObjects.elementAt(6))
         && !isBlockedSentinel(visibleObjects.elementAt(7))
         && !isBlockedSentinel(visibleObjects.elementAt(11))) {
         resolved = true;
         visibleObjects.setElementAt(obj, 12);
      }

      return resolved;
   }

   private static boolean coordsEqual(int x1, int y1, int x2, int y2) {
      return x1 == x2 && y1 == y2;
   }

   private static boolean isBlockedSentinel(Object slotValue) {
      if (slotValue instanceof Integer) {
         Integer v = (Integer)slotValue;
         if (v == 1) {
            return true;
         }

         if (v == -1) {
            return true;
         }
      }

      return false;
   }

   void addDroppedItem(byte[] record) {
      Dungeon level = ESGame.dungeons[this.currentLevel - 1];
      level.addDroppedItem(record);
   }

   // rollOutcome(atkChance, defChance): two independent percentile rolls,
   // returns 0=miss,1/2=graze(loser/winner by margin),3=clean hit -- shared
   // by attack()/castOnSelf()/castOnMonster()/rollShopOutcome().
   static int rollOutcome(int atkChance, int defChance) {
      int atkRoll = ESGame.nextInt(100);
      int defRoll = ESGame.nextInt(100);
      boolean atkHit = atkRoll <= defChance;
      lastDefenseRollHit = defRoll <= atkChance;
      byte tier;
      if (lastDefenseRollHit && !atkHit) {
         tier = 3;
      } else if (lastDefenseRollHit && atkHit) {
         tier = (byte)(defRoll >= atkRoll ? 2 : 1);
      } else if (lastDefenseRollHit || atkHit) {
         tier = 0;
      } else if (defRoll >= atkRoll) {
         tier = 2;
      } else {
         tier = 1;
      }

      return tier;
   }

   // Skill rank (skills[i][0]) plus, if includeBonus, a governing-attribute
   // bonus term (attributes[1 + skillGoverningAttribute[i]]/3); skill 11
   // (index) gets a bonus from skill 1's rank when effect3 is active; a
   // fatigue<7 penalty of -1 applies universally.
   int skillValue(int skillIndex, boolean includeBonus) {
      int v = this.skills[skillIndex][0];
      if (includeBonus) {
         int attrIdx = 1 + skillGoverningAttribute[skillIndex];
         v += this.attributes[attrIdx] / 3;
      }

      if (skillIndex == 11 && this.isEffectActive(3)) {
         v += this.skills[1][0];
      }

      if (this.coreStats[6] < 7) {
         v--;
      }

      return v;
   }

   int skillBonus(int skillIndex) {
      return this.skills[skillIndex][1];
   }

   // Defense-context skill value keyed off the offhand/shield equip slot
   // (equippedItems[1]): category 5 -> skill 5, else skill 7. Used when
   // THIS player is the defender (see Monster.tick()).
   int defenseSkillValue(boolean includeBonus) {
      if (this.equippedItems[1] != 0) {
         int cat = Item.column(1, this.equippedItems[1]);
         cat = Math.abs(cat);
         return cat == 5 ? this.skillValue(5, includeBonus) : this.skillValue(7, includeBonus);
      } else {
         return 0;
      }
   }

   int baseEvasion() {
      if (this.equippedItems[1] != 0) {
         int cat = Item.column(1, this.equippedItems[1]);
         cat = Math.abs(cat);
         return cat == 5 ? this.skillBonus(5) : this.skillBonus(7);
      } else {
         return 20;
      }
   }

   // Best of skills 0/2/8/12's rank (unbonused) -- "best armor skill" in
   // the dawnstar sense; returns the WINNING skill's index (0/2/8/12), not
   // its value.
   int bestArmorSkillIndex() {
      byte best = 0;
      int bestVal = this.skillValue(0, false);
      int v = this.skillValue(2, false);
      if (v > bestVal) {
         bestVal = v;
         best = 2;
      }

      v = this.skillValue(8, false);
      if (v > bestVal) {
         bestVal = v;
         best = 8;
      }

      v = this.skillValue(12, false);
      if (v > bestVal) {
         best = 12;
      }

      return best;
   }

   // effect6 overrides to bestArmorSkillIndex(); otherwise reads the
   // equipped weapon's (equippedItems[0]) category to pick a skill index
   // (1->0, 2->2, 3->8, else->12); -1 if unarmed.
   int activeWeaponSkillIndex() {
      if (this.isEffectActive(6)) {
         return this.bestArmorSkillIndex();
      } else if (this.equippedItems[0] != 0) {
         int cat = Item.column(1, this.equippedItems[0]);
         cat = Math.abs(cat);
         if (cat == 1) {
            return 0;
         } else if (cat == 2) {
            return 2;
         } else {
            return cat == 3 ? 8 : 12;
         }
      } else {
         return -1;
      }
   }

   // attackPower(includeBonus): effect14 -> spell-power path
   // (5+skillValue(4,false)); effect6 -> bestArmorSkillIndex()'s
   // skillValue; else the equipped weapon's category-mapped skillValue;
   // +skillValue(1,false) when effect5 is active.
   int attackPower(boolean includeBonus) {
      if (this.isEffectActive(14)) {
         return 5 + this.skillValue(4, false);
      }

      if (this.isEffectActive(6)) {
         int skill = this.bestArmorSkillIndex();
         return this.skillValue(skill, includeBonus);
      }

      int v = 0;
      if (this.equippedItems[0] != 0) {
         int cat = Item.column(1, this.equippedItems[0]);
         cat = Math.abs(cat);
         if (cat == 1) {
            v = this.skillValue(0, includeBonus);
         } else if (cat == 2) {
            v = this.skillValue(2, includeBonus);
         } else if (cat == 3) {
            v = this.skillValue(8, includeBonus);
         } else {
            v = this.skillValue(12, includeBonus);
         }
      } else {
         v = 0;
      }

      if (this.isEffectActive(5)) {
         v += this.skillValue(1, false);
      }

      return v;
   }

   // attackAccuracy(): the bonus-only counterpart of attackPower, used as
   // the base offensive chance in attack().
   int attackAccuracy() {
      if (this.isEffectActive(6) || this.isEffectActive(14)) {
         int skill = this.bestArmorSkillIndex();
         return this.skillBonus(skill);
      }

      if (this.equippedItems[0] != 0) {
         int cat = Item.column(1, this.equippedItems[0]);
         cat = Math.abs(cat);
         if (cat == 1) {
            return this.skillBonus(0);
         } else if (cat == 2) {
            return this.skillBonus(2);
         } else {
            return cat == 3 ? this.skillBonus(8) : this.skillBonus(12);
         }
      } else {
         return 20;
      }
   }

   // weaponDamage(): effect14 -> spell-power-shaped path (5+skillValue(4));
   // effect6 -> 20+skillValue(3); else the equipped weapon's raw magnitude
   // column (Item.column(3,itemId)); +10+skillValue(1) when effect1
   // active; +25 flat when increaseHarmBuff is set.
   int weaponDamage() {
      int v = 0;
      if (this.isEffectActive(14)) {
         v = 5 + this.skillValue(4, false);
      } else if (this.isEffectActive(6)) {
         v = 20 + this.skillValue(3, false);
      } else if (this.equippedItems[0] != 0) {
         v = Item.column(3, this.equippedItems[0]);
      } else {
         v = 0;
      }

      if (this.isEffectActive(1)) {
         v += 10 + this.skillValue(1, false);
      }

      if (this.increaseHarmBuff) {
         v += 25;
      }

      return v;
   }

   // Offhand/shield-slot skill INDEX (5 or 7, or -1 if unequipped) -- the
   // index-returning counterpart of defenseSkillValue()'s value.
   int defenseSkillIndex() {
      if (this.equippedItems[1] != 0) {
         int cat = Item.column(1, this.equippedItems[1]);
         cat = Math.abs(cat);
         return cat == 5 ? 5 : 7;
      } else {
         return -1;
      }
   }

   // Sums equip slots 1-5's magnitude column (Item.column(3,itemId),
   // weighted 4/2/2/1/1, /10) -- same "questFlags column doubles as
   // equip-magnitude" reuse dawnstar's own Item.java documents for its
   // armorValue(). +10+skillValue(1) when effect2 active; +spellArmorBonus
   // when effect17 active; +15 flat when increaseArmorBuff is set.
   int armorValue() {
      int v = 0;
      int col;
      if (this.equippedItems[1] != 0) {
         col = Item.column(3, this.equippedItems[1]);
         v += 4 * col;
      }

      if (this.equippedItems[2] != 0) {
         col = Item.column(3, this.equippedItems[2]);
         v += 2 * col;
      }

      if (this.equippedItems[3] != 0) {
         col = Item.column(3, this.equippedItems[3]);
         v += 2 * col;
      }

      if (this.equippedItems[4] != 0) {
         col = Item.column(3, this.equippedItems[4]);
         v += col;
      }

      if (this.equippedItems[5] != 0) {
         col = Item.column(3, this.equippedItems[5]);
         v += col;
      }

      v /= 10;
      if (this.isEffectActive(2)) {
         v += 10 + this.skillValue(1, false);
      }

      if (this.isEffectActive(17)) {
         v += this.spellArmorBonus;
      }

      if (this.increaseArmorBuff) {
         v += 15;
      }

      return v;
   }

   // Opening a chest (record: [x,y,type,idHi,idLo,value,flags,tier/gift]):
   // if a gift item was already possessed (isEquipmentCategory + charge
   // check via hasInventorySpace()/initializeItemCharge-shaped path),
   // reuses the existing slot's data; otherwise adds a fresh inventory
   // slot. Either way removes the chest from the level.
   int collectChestItem(byte[] record) {
      record[2] = 2;
      if (this.hasInventorySpace()) {
         byte itemId = record[4];
         int packedValue = (record[5] << 8) + record[6];
         byte extra = record[7];
         this.addInventoryItemRaw(itemId, packedValue, extra);
         ESGame.dungeons[this.currentLevel - 1].removeChest(record);
         System.out.println("Chest item not possessed before");
         int itemIndex = itemId - 1;
         System.out.println("item index=" + itemIndex);
         if (Item.column(1, itemIndex + 1) == 11) {
            this.giftPointsFound = (short)(this.giftPointsFound + Item.column(2, itemIndex + 1));
            int advancement = ESGame.getGameAdvancementLevel(this.giftPointsFound);
            this.gameRef.checkOpenAndPopulateDungeons(advancement);
         }

         return 1;
      } else {
         byte[] dropped = new byte[]{record[0], record[1], record[4], record[5], record[6], record[7], 1};
         this.addDroppedItem(dropped);
         ESGame.dungeons[this.currentLevel - 1].removeChest(record);
         return 0;
      }
   }

   // isEffectActive: -1=always active,-2=conditional on giftPointsFound!=0
   // (was `this.t != 0` -- note `t` there is the SHORT FIELD lastCombatTargetId,
   // not a typo for giftPointsFound; preserved exactly as found, flagged
   // here since it looks like it could be either a real design choice or a
   // latent bug -- t!=0 means "has attacked something at least once"),
   // >0=countdown still running.
   private boolean isEffectActive(int effectId) {
      if (this.effectDurations[effectId - 1] == -1) {
         return true;
      } else {
         return this.effectDurations[effectId - 1] == -2 ? this.lastCombatTargetId != 0 : this.effectDurations[effectId - 1] > 0;
      }
   }

   void clearEffect(int effectId) {
      this.effectDurations[effectId - 1] = 0;
   }

   // Steps pendingTile* one tile forward (computeMoveTarget(1)) and
   // returns the Monster there, if any (also fires its unusedHook()).
   Monster monsterAheadOfPlayer() {
      this.computeMoveTarget(1);
      if (this.pendingLevel <= 0) {
         return null;
      }

      Dungeon level = ESGame.dungeons[this.pendingLevel - 1];
      Monster m = level.monsterAt(this.pendingTileX, this.pendingTileY);
      if (m != null) {
         m.unusedHook();
      }

      return m;
   }

   byte[] chestAheadOfPlayer() {
      this.computeMoveTarget(1);
      if (this.pendingLevel <= 0) {
         return null;
      }

      byte level = this.pendingLevel;
      Hashtable chests = ESGame.chests[level - 1];
      if (chests == null) {
         return null;
      }

      Object c = chests.get(Util.posKey(this.pendingTileX, this.pendingTileY));
      return c == null ? null : (byte[])c;
   }

   int shopAheadOfPlayer() {
      this.computeMoveTarget(1);
      if (this.pendingLevel <= 0) {
         return -1;
      }

      byte level = this.pendingLevel;
      return level != 1 ? -1 : Shop.questShopAt(this.pendingTileX, this.pendingTileY);
   }

   // Bucket lookup from spellId to a skill-tier constant (1/3/4/6/10),
   // same shape as dawnstar's spellSkillIndexFor bucket boundaries.
   int spellSkillIndexFor(int spellId) {
      if (spellId <= 5) {
         return 1;
      } else if (spellId <= 10) {
         return 3;
      } else if (spellId <= 15) {
         return 4;
      } else {
         return spellId <= 20 ? 6 : 10;
      }
   }

   // castOnSelf(spellId): rolls a chance via skillValue/skillBonus of
   // spellSkillIndexFor(spellId) vs. the spell's own icon/durationMultiplier
   // fields (see below), spends Magicka scaled by tier, and on a hit
   // dispatches a big switch on spellId for the actual self-targeted
   // effect (buffs, heals, ailment cure, learn-spell-from-scroll for
   // spell 6).
   //
   // REAL FINDING, confirmed by re-reading decompiled/j.java's `p(int)`
   // directly (not assumed from dawnstar): the chance formula's second
   // input reads Spell.durationMultiplier (`.j`), NOT Spell.school
   // (`.d`) -- despite `school` being the field whose name would fit a
   // spell-resistance-chance formula. And separately, the byte actually
   // STORED into effectDurations[] on a hit (`power * mult` below) is
   // Spell.power, not Spell.durationMultiplier, even though the latter's
   // name is what you'd expect to feed a duration. Same species of
   // "field read doesn't match its own name" oddity dawnstar's own
   // CLASS_MAP.md documents for its castOnSelf/castOnMonster (there:
   // magickaCost/power read into locals named the opposite of their real
   // role) -- ported here as an independently-confirmed Stormhold
   // instance of the same kind of bug, not copied from dawnstar's finding.
   void castOnSelf(int spellId) {
      System.out.println("start of castSpell");
      int skillIdx = this.spellSkillIndexFor(spellId);
      int skillVal = this.skillValue(skillIdx, true);
      int skillBns = this.skillBonus(skillIdx);
      byte icon = Spell.byId(spellId).icon;
      byte durationMultiplier = Spell.byId(spellId).durationMultiplier;
      byte magickaCost = Spell.byId(spellId).magickaCost;
      byte power = Spell.byId(spellId).power;
      int diff = skillVal - icon;
      int defendChance = skillBns + diff * 5;
      int attackChance = durationMultiplier - diff * 5;
      defendChance = Math.min(Math.max(defendChance, 10), 95);
      attackChance = Math.min(Math.max(attackChance, 10), 95);
      int tier = rollOutcome(defendChance, attackChance);
      byte mult = 1;
      if (tier == 0) {
         this.coreStats[4] = (short)(this.coreStats[4] - 3 * magickaCost);
      } else if (tier == 1) {
         this.coreStats[4] = (short)(this.coreStats[4] - 3 * magickaCost / 2);
      } else if (tier == 2) {
         this.coreStats[4] = (short)(this.coreStats[4] - magickaCost);
      } else if (tier == 3) {
         this.coreStats[4] = (short)(this.coreStats[4] - magickaCost);
         mult = 2;
      }

      this.coreStats[4] = (short)Math.max(this.coreStats[4], 0);
      if (tier >= 2) {
         this.gainSkillExp(skillIdx, 1);
      }

      switch (spellId) {
         case 1:
         case 2:
         case 3:
         case 5:
            this.effectDurations[spellId - 1] = (byte)(power * mult);
         case 4:
         case 7:
         case 8:
         case 9:
         case 10:
         case 11:
         case 12:
         case 13:
         case 14:
         case 15:
         case 16:
         case 17:
         case 18:
         case 19:
         case 20:
         case 22:
         default:
            break;
         case 6:
            // Grants item 109 (a "cure poison"-shaped scroll, per its
            // fixed id) and auto-equips it -- same addInventoryItem+
            // equipItem pairing dawnstar's own M16 port milestone
            // documents for its castOnSelf's case-6 branch. FIXED here
            // after an earlier draft wrongly called canLearnSpellFromScroll
            // (a DIFFERENT 2-int overload, `e(int)`, not `c(int,int)`) --
            // the original decompiled call is `this.c(109, var14)` with
            // var14 a literal 0, i.e. addInventoryItem(109, 0).
            if (this.addInventoryItem(109, 0) && this.equipLastPickedUpItem(true)) {
               this.effectDurations[spellId - 1] = (byte)(power * mult);
            }
            break;
         case 21:
            int heal = 6 + this.skillValue(10, false);
            this.coreStats[2] = (short)(this.coreStats[2] + mult * heal);
            this.coreStats[2] = (short)Math.min(this.coreStats[2], this.coreStats[3]);
            break;
         case 23:
            this.effectDurations[spellId - 1] = -2;
            break;
         case 24:
            this.effectDurations[spellId - 1] = -4;
            break;
         case 25:
            for (int i = 1; i <= mult; i++) {
               this.cureRandomAilment();
            }
      }

      this.coreStats[6] = (short)(this.coreStats[6] - 5 * this.fatigueCostMultiplier());
      this.coreStats[6] = (short)Math.max(this.coreStats[6], 0);
      if (this.hasAilment(6)) {
         int selfDmg = 2 * this.coreStats[3] / 100;
         if (selfDmg < 1) {
            selfDmg = 1;
         }

         this.coreStats[2] = (short)(this.coreStats[2] - (short)selfDmg);
      }
   }

   // castOnMonster(spellId, target): the offensive counterpart of
   // castOnSelf, dispatching a big switch of monster-targeted effects
   // (damage, ailment infliction, buffs). Only reads Spell.magickaCost/
   // Spell.power (unlike castOnSelf, this method does NOT also touch
   // Spell.icon/durationMultiplier) -- confirmed directly against
   // decompiled/j.java's `b(int, d)`, no school/duration mixup here.
   void castOnMonster(int spellId, Monster target) {
      int skillIdx = this.spellSkillIndexFor(spellId);
      int skillVal = this.skillValue(skillIdx, true);
      int skillBns = this.skillBonus(skillIdx);
      int targetOffense = target.stat(10);
      int targetDefense = target.stat(9);
      byte magickaCost = Spell.byId(spellId).magickaCost;
      byte power = Spell.byId(spellId).power;
      int diff = skillVal - targetOffense;
      int maxHp = target.stat(2);
      diff = Math.min(diff, maxHp);
      int attackChance = skillBns + diff * 5;
      int defendChance = targetDefense - diff * 5;
      attackChance = Math.min(Math.max(attackChance, 10), 95);
      defendChance = Math.min(Math.max(defendChance, 10), 95);
      int tier = rollOutcome(attackChance, defendChance);
      byte mult = 1;
      if (tier == 0) {
         this.coreStats[4] = (short)(this.coreStats[4] - 3 * magickaCost);
      } else if (tier == 1) {
         this.coreStats[4] = (short)(this.coreStats[4] - 3 * magickaCost / 2);
      } else if (tier == 2) {
         this.coreStats[4] = (short)(this.coreStats[4] - magickaCost);
      } else if (tier == 3) {
         this.coreStats[4] = (short)(this.coreStats[4] - magickaCost);
         mult = 2;
      }

      this.coreStats[4] = (short)Math.max(this.coreStats[4], 0);
      if (tier >= 2) {
         this.gainSkillExp(skillIdx, 1);
      }

      switch (spellId) {
         case 4:
            target.scratch[9] = -2;
            target.store();
         case 5:
         case 6:
         default:
            break;
         case 7:
            int dmg7 = 10 + this.skillValue(3, false);
            target.scratch[1] = (byte)dmg7;
            break;
         case 8:
            int amt8 = this.skillValue(3, false);
            int hp8 = 12 + 2 * amt8;
            target.takeDamage(hp8);
            this.coreStats[6] = (short)(this.coreStats[6] + amt8);
            this.coreStats[6] = (short)Math.min(this.coreStats[6], this.coreStats[7]);
            this.coreStats[2] = (short)(this.coreStats[2] + amt8);
            this.coreStats[2] = (short)Math.min(this.coreStats[2], this.coreStats[3]);
            this.coreStats[4] = (short)(this.coreStats[4] + 12);
            this.coreStats[4] = (short)Math.min(this.coreStats[4], this.coreStats[5]);
            break;
         case 9:
            if (target.isUndead()) {
               int base9 = 60 * mult;
               int dmg9 = base9 - target.stat(8);
               dmg9 = Math.max(dmg9, 4);
               int scaled9 = dmg9 * target.stat(14) / 100;
               target.takeDamage(scaled9);
            }
            break;
         case 10:
            this.effectDurations[spellId - 1] = -2;
            target.scratch[8] = (byte)(2 * mult);
            break;
         case 11:
            int base11 = 25 + this.skillValue(4, false);
            int scaled11a = base11 * mult;
            int dmg11 = scaled11a - target.stat(8);
            dmg11 = Math.max(dmg11, 4);
            int scaled11 = dmg11 * target.stat(14) / 100;
            target.takeDamage(scaled11);
            target.store();
            break;
         case 12:
            this.effectDurations[spellId - 1] = -2;
            int v12 = mult * (10 + this.skillValue(4, false));
            v12 = Math.min(v12, 255);
            target.scratch[4] = (byte)v12;
            target.store();
            break;
         case 13:
            this.effectDurations[spellId - 1] = -2;
            int v13 = mult * (10 + this.skillValue(4, false));
            v13 = Math.min(v13, 255);
            target.scratch[5] = (byte)v13;
            target.store();
            break;
         case 14:
            this.effectDurations[spellId - 1] = -1;
            this.attack(target);
            this.effectDurations[spellId - 1] = 0;
            break;
         case 15:
            this.effectDurations[spellId - 1] = -2;
            target.scratch[2] = 1;
            target.store();
            break;
         case 16:
            int room16 = 10 - targetOffense;
            if (room16 > 0) {
               room16 = mult * room16;
               this.effectDurations[spellId - 1] = (byte)room16;
               target.scratch[6] = 1;
            }
            break;
         case 17:
            this.effectDurations[spellId - 1] = -2;
            this.spellArmorBonus = (short)(10 + this.skillValue(6, false));
            break;
         case 18:
            this.effectDurations[spellId - 1] = -2;
            target.scratch[0] = (byte)(3 * mult);
            break;
         case 19:
            this.effectDurations[spellId - 1] = -2;
            int chance19 = mult * (60 - 5 * targetOffense);
            chance19 = Math.min(Math.max(chance19, 0), 100);
            target.scratch[3] = (byte)chance19;
            break;
         case 20:
            int base20 = 80 - 5 * targetOffense;
            int scaled20a = base20 * mult;
            int dmg20 = scaled20a - target.stat(8);
            dmg20 = Math.max(dmg20, 4);
            int scaled20 = dmg20 * target.stat(14) / 100;
            target.takeDamage(scaled20);
      }

      this.coreStats[6] = (short)(this.coreStats[6] - 5 * this.fatigueCostMultiplier());
      this.coreStats[6] = (short)Math.max(this.coreStats[6], 0);
      if (this.hasAilment(6)) {
         int selfDmg = 2 * this.coreStats[3] / 100;
         if (selfDmg < 1) {
            selfDmg = 1;
         }

         this.coreStats[2] = (short)(this.coreStats[2] - (short)selfDmg);
      }
   }

   void gainSkillExp(int skillIndex, int amount) {
      if (skillIndex >= 0) {
         this.skills[skillIndex][2] = (short)(this.skills[skillIndex][2] + amount);
      }
   }

   boolean tryPickUpItem(byte[] record) {
      byte itemId = record[2];
      int packedValue = (record[3] << 8) + record[4];
      byte extra = record[5];
      return this.addInventoryItemRaw(itemId, packedValue, extra);
   }

   boolean addInventoryItem(int itemId, int value) {
      return this.addInventoryItemRaw(itemId, value, 0);
   }

   boolean addInventoryItemRaw(int itemId, int packedValue, int charge) {
      if (this.inventoryCount < 24) {
         this.inventoryItemIds[this.inventoryCount] = (byte)itemId;
         int packed = (packedValue << 16) + (byte)charge;
         this.inventoryItemData[this.inventoryCount] = packed;
         this.inventoryCount++;
         return true;
      } else {
         return false;
      }
   }

   // RESOLVED (was LOW CONFIDENCE, see git history for the earlier
   // `stateByteAb` version): was `r()` -- `this.g(1); if (this.ab<=0)
   // return -1; return this.ab!=1 ? -1 : k.a(this.z,this.w);`. `g(1)` is
   // this same file's own computePendingPosition() (the two sibling
   // methods immediately above `r()` in decompiled/j.java, `n()`/`h()`,
   // open with the identical `this.g(1); if (this.ab<=0) ...` guard and
   // are already ported above as `currentDungeonMonsterAtPendingTile()`-
   // /chest-lookup code using `pendingLevel`/`pendingTileX`/`pendingTileY`
   // -- so `ab`/`z`/`w` are the same three fields here too, not a fresh
   // unconfirmed alias). Independently corroborated by dawnstar's own
   // `Player.java`, which names the identical shared-engine mechanic
   // `pendingLevel`/`pendingTileX`/`pendingTileY` with full confidence
   // (see its own `computePendingPosition()`-equivalent doc comment).
   // `k.a(int,int)` (decompiled/k.java:124) is `Shop.questShopAt`,
   // confirmed by direct read: it scans the 7 shop tiles for a position
   // match with an active per-shop flag, exactly what's called below.
   int questShopAtPendingTile() {
      if (this.pendingLevel <= 0) {
         return -1;
      }

      return this.pendingLevel != 1 ? -1 : Shop.questShopAt(this.pendingTileX, this.pendingTileY);
   }

   // Applies camp-rest recovery (was `e(boolean)`): restores a fraction of
   // missing HP/Magicka/Fatigue -- full amount if `fullyRested`, else 2/3
   // -- further scaled to 3/4 while ailment 8 (Winter Worn) is active;
   // clears the 3 temporary combat buffs (increaseHarmBuff/
   // increaseArmorBuff/safeCampingBuff -- confirmed via this method's
   // field-declaration-order match to the original's `s`/`L`/`I`, the
   // same trick that confirmed pendingTileX/Y/Facing/Level below); a 10%
   // chance to consume one "Safe Camping" item (id 96) from inventory if
   // present; and a 25% chance per non-ailment-4/5 bit to cure that
   // ailment (ailments 4/5 -- Troll Thirst/Glacier Curse in dawnstar's
   // naming -- have their own dedicated timers elsewhere and are skipped
   // here). Confirmed caller: GameCanvas's camp-interrupted/camp-complete
   // branches in run().
   void applyRestRecovery(boolean fullyRested) {
      short missingHp = (short)(this.coreStats[3] - this.coreStats[2]);
      short missingMagicka = (short)(this.coreStats[5] - this.coreStats[4]);
      short missingFatigue = (short)(this.coreStats[7] - this.coreStats[6]);
      if (!fullyRested) {
         missingHp = (short)(2 * missingHp / 3);
         missingMagicka = (short)(2 * missingMagicka / 3);
         missingFatigue = (short)(2 * missingFatigue / 3);
      }

      if (this.hasAilment(8)) {
         missingHp = (short)(3 * missingHp / 4);
         missingMagicka = (short)(3 * missingMagicka / 4);
         missingFatigue = (short)(3 * missingFatigue / 4);
      }

      this.coreStats[2] = (short)(this.coreStats[2] + missingHp);
      this.coreStats[4] = (short)(this.coreStats[4] + missingMagicka);
      this.coreStats[6] = (short)(this.coreStats[6] + missingFatigue);
      this.increaseHarmBuff = false;
      this.increaseArmorBuff = false;
      this.safeCampingBuff = false;
      int roll = Util.randomInt(100);
      if (roll <= 10) {
         for (int slot = 0; slot < this.inventoryCount; slot++) {
            int itemId = Math.abs(this.inventoryItemIds[slot]);
            if (itemId == 96) {
               this.removeInventorySlot(slot);
               break;
            }
         }
      }

      for (int bit = 0; bit < 8; bit++) {
         int ailmentId = bit + 1;
         if (ailmentId != 4 && ailmentId != 5) {
            roll = Util.randomInt(100);
            if (roll <= 25) {
               this.ailmentMask = (byte)Util.clearBit(bit, this.ailmentMask);
            }
         }
      }
   }

   // True iff inventory `slot` holds an equippable item AND it's currently
   // equipped (negative id, this codebase's equipped-slot convention) --
   // was `C(int)`. Confirmed sole caller: GameCanvas's death/respawn
   // handling, which strips every non-equipped item on respawn.
   boolean isSlotEquipped(int slot) {
      byte itemId = this.inventoryItemIds[slot];
      return !Item.isEquippable(Math.abs(itemId)) ? false : itemId < 0;
   }

   // Item.subtype of the item occupying inventory `slot` (was `D(int)`) --
   // Shop.java's Helga dialogue (action==4) reads this as a "quality tier"
   // (subtype > 3 = better payout) for a category-13 item turn-in.
   int itemSubtypeAtSlot(int slot) {
      int itemId = Math.abs(this.inventoryItemIds[slot]);
      return Item.column(2, itemId);
   }

   // Removes inventory slot `slot`, unequipping it first if it was
   // equipped, and compacts the remaining slots down.
   boolean removeInventorySlot(int slot) {
      if (slot >= this.inventoryCount) {
         return false;
      }

      this.unequipSlot(slot);
      this.inventoryItemIds[slot] = 0;

      for (int i = slot; i < this.inventoryCount - 1; i++) {
         this.inventoryItemIds[i] = this.inventoryItemIds[i + 1];
         this.inventoryItemData[i] = this.inventoryItemData[i + 1];
      }

      this.inventoryCount--;
      return true;
   }

   // Drops inventory slot `slot` onto the current tile as a dropped-item
   // record (flags=3, i.e. bits 0+1 set) unless it's item 109 (a
   // non-droppable special id), then removes the slot.
   void dropInventoryItem(int slot) {
      int itemId = Math.abs(this.inventoryItemIds[slot]);
      if (itemId != 109) {
         byte[] record = new byte[]{
            this.tileX, this.tileY, (byte)itemId, 0, 0, (byte)(this.inventoryItemData[slot] & 0xFF), 0
         };
         int extended = this.inventoryItemData[slot] >>> 16 & 65535;
         record[3] = (byte)(extended >> 8 & 0xFF);
         record[4] = (byte)(extended & 0xFF);
         record[6] = 3;
         this.currentDungeon().addDroppedItem(record);
         this.removeInventorySlot(slot);
      } else {
         this.removeInventorySlot(slot);
      }
   }

   // True iff slot's item is currently equipped (negative id, and its
   // category is 1-10 per Item.isEquipmentCategory).
   boolean isEquippedSlot(int slot) {
      byte id = this.inventoryItemIds[slot];
      return !Item.isEquipmentCategory(Math.abs(id)) ? false : id < 0;
   }

   // Equips/unequips slot `slot`. If the item's equip slot is already
   // occupied by a different item and `allowSwap` is true, the old one is
   // unequipped first; if allowSwap is false and a conflict exists, fails.
   //
   // **Phase-1 renaming correction (found during phase-3 M9, NOT a bug in
   // the original game):** this method and unequipMatchingCategory() below
   // both used to read `Item.column(1, id)` here -- itemsin.dat's
   // *category* column (1-10, e.g. Boots=7/Gloves=8/Helmet=9/Shield=10),
   // used directly as an index into `equippedItems` (byte[7], valid
   // indices 0-6). Real itemsin.dat data has items in every category 1-10
   // (confirmed against extracted/itemsin.dat directly), so that would
   // throw ArrayIndexOutOfBoundsException the moment a real player
   // equipped any Boots/Gloves/Helmet/Shield item -- clearly wrong for a
   // shipped game. Re-checked against decompiled/j.java (the raw,
   // unrenamed decompiler output) directly: the real call at this site is
   // the SINGLE-argument `a.a(var3)` (decompiled/j.java line 1865, and
   // `f(int)`'s line 1883) -- i.e. `Item.equipSlotOf(id)`, itemsin.dat's
   // dedicated `equipSlot` column, whose real values are confirmed 0-6
   // (fits `equippedItems[7]` exactly, and matches every OTHER read of
   // `equippedItems[0]`/`[1]`/etc. throughout this file). The two-argument
   // `Item.column(1, ...)` calls at other call sites in this file (e.g.
   // deriveWeaponSkillIndex-style methods around line 1430-1630) ARE
   // correct as originally transcribed -- those read the *category* of an
   // item ALREADY STORED in an equippedItems slot for an unrelated lookup,
   // matching decompiled/j.java's own two-argument `a.a(1, this.T[n])`
   // calls at those sites exactly. Only the two call sites below, where
   // the result is used to INDEX `equippedItems` itself, were wrong.
   boolean equipItem(int slot, boolean allowSwap) {
      byte id = this.inventoryItemIds[slot];
      if (id < 0) {
         return false;
      }

      if (!Item.isEquipmentCategory(id)) {
         return false;
      }

      int equipSlot = Item.equipSlotOf(id);
      if (this.equippedItems[equipSlot] != 0) {
         if (!allowSwap) {
            return false;
         }

         this.unequipMatchingCategory(equipSlot);
      }

      this.equippedItems[equipSlot] = id;
      this.inventoryItemIds[slot] = (byte)(-Math.abs(this.inventoryItemIds[slot]));
      return true;
   }

   private void unequipMatchingCategory(int equipSlotWanted) {
      for (int i = 0; i < this.inventoryCount; i++) {
         byte id = this.inventoryItemIds[i];
         id = (byte)Math.abs(id);
         int equipSlot = Item.equipSlotOf(id);
         if (equipSlot == equipSlotWanted) {
            this.unequipSlot(i);
         }
      }
   }

   boolean equipLastPickedUpItem(boolean allowSwap) {
      int slot = this.inventoryCount - 1;
      return this.equipItem(slot, allowSwap);
   }

   void unequipSlot(int slot) {
      if (this.isEquippedSlot(slot)) {
         if (slot >= 0 && slot <= 23) {
            byte id = this.inventoryItemIds[slot];
            id = (byte)Math.abs(id);
            this.inventoryItemIds[slot] = id;

            for (int i = 0; i < 7; i++) {
               if (this.equippedItems[i] == id) {
                  this.equippedItems[i] = 0;
                  break;
               }
            }
         }
      }
   }

   // MATCHES Shop.java's own forward reference (`player.initializeItemCharge`)
   // -- gates on Item.isEquipmentCategory, sets a fixed charge value of 3
   // in the low byte of inventoryItemData[slot].
   boolean initializeItemCharge(int slot) {
      byte id = this.inventoryItemIds[slot];
      id = (byte)Math.abs(id);
      if (Item.isEquipmentCategory(id)) {
         byte charge = (byte)(this.inventoryItemData[slot] & 0xFF);
         charge = 3;
         this.inventoryItemData[slot] = this.inventoryItemData[slot] & -256;
         this.inventoryItemData[slot] = this.inventoryItemData[slot] | charge;
         return true;
      } else {
         return false;
      }
   }

   boolean isItemCharged(int slot) {
      byte id = this.inventoryItemIds[slot];
      id = (byte)Math.abs(id);
      byte charge = (byte)(this.inventoryItemData[slot] & 0xFF);
      return charge == 3;
   }

   int itemSubtypeInSlot(int slot) {
      byte id = this.inventoryItemIds[slot];
      id = (byte)Math.abs(id);
      return Item.column(2, id);
   }

   int findEquippedSlotForItem(int itemId) {
      int found = -1;
      int target = -Math.abs(itemId);

      for (int i = 0; i < this.inventoryCount; i++) {
         if (target == this.inventoryItemIds[i]) {
            found = i;
            break;
         }
      }

      return found;
   }

   boolean hasInventorySpace() {
      return this.inventoryCount < 24;
   }

   // Item tooltip text: category-dependent (weapon/armor slots show value,
   // spells show "Spell: <name>", "gift" items (category 13) show
   // Item.specialEffectText, etc).
   String itemTooltip(int slot) {
      int id = Math.abs(this.inventoryItemIds[slot]);
      byte cat = (byte)Item.column(1, id);
      String out = null;
      switch (cat) {
         case 1:
         case 2:
         case 3:
         case 4:
            out = Item.nameOf(id) + '\n' + Item.categoryNames[cat - 1];
            int wv = Item.column(3, id) + (this.inventoryItemData[slot] & 0xFF);
            out = out + "\nWeapon value: " + wv;
            break;
         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
            out = Item.nameOf(id) + '\n' + Item.categoryNames[cat - 1];
            int av = Item.column(3, id) + (this.inventoryItemData[slot] & 0xFF);
            out = out + "\nArmor value: " + av;
            break;
         case 11:
            out = Item.nameOf(id) + '\n' + Item.categoryNames[cat - 1];
            break;
         case 12:
            out = Item.nameOf(id) + '\n' + "Spell: ";
            int spellId = this.inventoryItemData[slot] & 0xFF;
            out = out + Spell.byId(spellId).name;
            break;
         case 13:
            int giftIdx = id - 87;
            String[] lines = Item.specialEffectText[giftIdx];
            out = Item.nameOf(id) + '\n' + Item.categoryNames[cat - 1] + '\n' + lines[0];
            if (lines[1].length() > 0) {
               out = out + '\n' + lines[1];
            }
            break;
         case 17:
            out = Item.nameOf(id) + '\n' + Item.categoryNames[cat - 1];
            int bonus = this.skillValue(3, false);
            int val17 = 20 + bonus;
            out = out + "\nWeapon value: " + val17;
            break;
         default:
            out = Item.nameOf(id) + '\n' + Item.categoryNames[cat - 1];
      }

      return out;
   }

   // Learns a spell from a category-12 scroll in `slot`: sets the
   // matching bit in knownSpellsMask (an `int`, so the `int`-overload of
   // Util.setBit -- NOT selectedSpellId, an earlier draft of this method
   // mixed the two up) and consumes the slot. Always returns true and
   // never itself calls canLearnSpellFromScroll() (a separate, unrelated
   // skill-prerequisite gate) -- any "don't call this twice" gating is
   // the caller's job.
   boolean learnSpellFromScroll(int slot) {
      int id = Math.abs(this.inventoryItemIds[slot]);
      byte cat = (byte)Item.column(1, id);
      int packedValue = this.inventoryItemData[slot] & 0xFF;
      int bit = packedValue - 1;
      this.knownSpellsMask = Util.setBit(bit, this.knownSpellsMask);
      this.removeInventorySlot(slot);
      return true;
   }

   // canEquipOrUnequip(slot): equipment categories 1-10, or 17.
   boolean canEquipOrUnequip(int slot) {
      int id = Math.abs(this.inventoryItemIds[slot]);
      byte cat = (byte)Item.column(1, id);
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
         case 17:
            return true;
         default:
            return false;
      }
   }

   boolean isScrollCategory(int slot) {
      int id = Math.abs(this.inventoryItemIds[slot]);
      byte cat = (byte)Item.column(1, id);
      switch (cat) {
         case 13:
         case 15:
            return true;
         default:
            return false;
      }
   }

   // canLearnSpellFromScroll(slot): category-12 gate + a skill-PREREQUISITE
   // gate (skills[spellSkill][0] > 0, i.e. must already have SOME rank in
   // the spell's governing skill) -- NOT a "not already known" gate as an
   // earlier draft of this comment said; fixed after re-reading the
   // original single-arg `e(int)` directly (this method originally took
   // only the slot argument, not two -- also fixed here, an earlier draft
   // wrongly added a second unused `extra` parameter). No confirmed
   // caller found anywhere in j.java itself; likely used by ESGame's own
   // inventory-menu code, same situation as dawnstar's canLearnSpell.
   boolean canLearnSpellFromScroll(int slot) {
      int id = Math.abs(this.inventoryItemIds[slot]);
      byte cat = (byte)Item.column(1, id);
      switch (cat) {
         case 12:
            int spellId = this.inventoryItemData[slot] & 0xFF;
            byte skillIdx = Spell.byId(spellId).skillRequired;
            if (this.skills[skillIdx][0] > 0) {
               return true;
            }

            return false;
         default:
            return false;
      }
   }

   // effectiveStat(statIndex): coreStats[statIndex] plus, when effect23
   // ("Regeneration") is active, a skillValue(10,false)-scaled bonus
   // clamped to the matching max stat (index 2->max3, 6->max7, 4->max5).
   int effectiveStat(int statIndex) {
      int v = this.coreStats[statIndex];
      if (this.isEffectActive(23)) {
         if (statIndex == 2) {
            v += this.skillValue(10, false);
            if (v > this.coreStats[3]) {
               v = this.coreStats[3];
            }
         } else if (statIndex == 6) {
            v += this.skillValue(10, false);
            if (v > this.coreStats[7]) {
               v = this.coreStats[7];
            }
         } else if (statIndex == 4) {
            v += this.skillValue(10, false);
            if (v > this.coreStats[5]) {
               v = this.coreStats[5];
            }
         }
      }

      return v;
   }

   boolean hasCampMark() {
      return this.campLevel > 0;
   }

   void markCampAndReturnToTown() {
      this.campLevel = this.currentLevel;
      this.campX = this.tileX;
      this.campY = this.tileY;
      this.campFacing = this.facing;
      this.setHubSpawnPosition(true);
      this.refreshCorridorView();
      this.justMarkedCamp = true;
   }

   void warpToCampMark() {
      this.currentLevel = this.pendingLevel = this.campLevel;
      this.tileX = this.pendingTileX = this.campX;
      this.tileY = this.pendingTileY = this.campY;
      this.refreshCorridorView();
      this.justMarkedCamp = true;
   }

   int activeAilmentCount() {
      int count = 0;

      for (int i = 0; i < 8; i++) {
         int bit = this.ailmentMask >> i & 1;
         if (bit != 0) {
            count++;
         }
      }

      return count;
   }

   void cureRandomAilment() {
      int count = this.activeAilmentCount();
      if (count > 0) {
         int pick;
         if (count == 1) {
            pick = 1;
         } else {
            pick = Util.randomInt(count);
         }

         int seen = 0;

         for (int i = 0; i < 8; i++) {
            int bit = this.ailmentMask >> i & 1;
            if (bit == 1) {
               if (++seen == pick) {
                  this.ailmentMask = (byte)Util.clearBit(i, this.ailmentMask);
                  break;
               }
            }
         }
      }
   }

   int fatigueCostMultiplier() {
      return (this.ailmentMask & 1) == 1 ? 3 : 1;
   }

   Dungeon currentDungeon() {
      return ESGame.dungeons[this.currentLevel - 1];
   }

   void refreshCorridorView() {
      this.currentDungeon().sampleCorridorView(this.tileX, this.tileY, this.facing, this.corridorView);
   }

   // Confirmed (phase-3 port M38): byte-for-byte from decompiled/j.java's
   // n(). The monster at the forward-facing look-ahead tile, via
   // computeMoveTarget(1) -- but UNLIKE commitMove()'s own unguarded
   // computeMoveTarget() call (a real latent crash risk if a step would
   // cross to a nonexistent neighbor, see Dungeon.tileAt()'s own port-
   // side guard discipline), this gracefully returns null instead when
   // that happens (`pendingLevel <= 0`), matching the original's own
   // explicit check here. The original also calls a confirmed real
   // no-op method on the found monster (decompiled/d.java's own empty
   // c()) -- not reproduced, it does literally nothing.
   Monster monsterInFront() {
      this.computeMoveTarget(1);
      if (this.pendingLevel <= 0) {
         return null;
      }

      return ESGame.dungeons[this.pendingLevel - 1].monsterAt(this.pendingTileX, this.pendingTileY);
   }

   // The longer (~450 char) character-sheet dump: name/class/level/HP/
   // Magicka/Fatigue/active ailments (by name)/giftPointsFound/attributes.
   // NOT transcribed -- was `K()`, a ~84-line debug-only string builder
   // (current dungeon name, then 4 sub-reports via further un-traced
   // helper calls, then inventory count) feeding ESGame's "Debug" Form
   // only (a dev-menu screen, not player-facing). Left as a minimal
   // placeholder rather than guessed at; revisit if the debug form is
   // ever actually needed.
   String debugSummary() {
      return "Player debug summary: level=" + this.coreStats[0] + " gold=" + this.inventoryCount;
   }

   String characterSheetText() {
      StringBuffer out = new StringBuffer(450);
      String sp = " ";
      String colon = ": ";
      out.append(this.playerName);
      out.append('\n');
      out.append(classNames[this.classIndex]);
      out.append('\n');
      out.append("Level ");
      out.append(this.coreStats[0]);
      out.append(" (");
      out.append(this.coreStats[1]);
      out.append("/10)");
      out.append('\n');
      out.append("Health: ");
      out.append(this.effectiveStat(2));
      out.append('/');
      out.append(this.coreStats[3]);
      out.append('\n');
      out.append("Magic: ");
      out.append(this.effectiveStat(4));
      out.append('/');
      out.append(this.coreStats[5]);
      out.append('\n');
      out.append("Fatigue: ");
      out.append(this.effectiveStat(6));
      out.append('/');
      out.append(this.coreStats[7]);
      out.append('\n');
      out.append('\n');
      out.append("Status ailments: ");
      int shown = 0;

      for (int i = 1; i <= 8; i++) {
         if (this.hasAilment(i)) {
            out.append('\n');
            out.append(ailmentNames[i - 1]);
            shown++;
         }
      }

      if (shown == 0) {
         out.append('\n');
         out.append("None");
      }

      out.append('\n');
      out.append('\n');
      out.append("Gift points found: ");
      out.append(this.giftPointsFound);
      out.append('\n');
      out.append('\n');
      out.append("Attributes:");
      out.append('\n');

      for (int i = 0; i < 8; i++) {
         int a = 2 * i;
         out.append(attributeNames[a]);
         out.append(colon);
         out.append(this.attributes[a]);
         out.append('\n');
      }

      String result = out.toString();
      System.out.println("Length of stats string is " + result.length());
      return result;
   }

   static Vector newVisibleObjectsVector() {
      Vector v = new Vector();

      for (int i = 0; i < 13; i++) {
         v.addElement(new Object());
      }

      return v;
   }

   static String describeDroppedItemRecord(byte[] record) {
      String out = "X = " + record[0] + '\n';
      out = out + "Y = " + record[1] + '\n';
      out = out + "Type = " + record[2] + '\n';
      out = out + "ID(MSB) = " + record[3] + '\n';
      out = out + "ID(LSB) = " + record[4] + '\n';
      out = out + "value = " + record[5] + '\n';
      return out + "flags = " + record[6] + '\n';
   }

   Vector skillSummaryList() {
      Vector out = new Vector();

      for (int i = 0; i < 14; i++) {
         if (this.skills[i][0] > 0) {
            String line = skillNames[i] + ": " + this.skills[i][0];
            out.addElement(line);
         }
      }

      return out;
   }

   int nthLearnedSkillIndex(int n) {
      int seen = 0;

      for (int i = 0; i < 14; i++) {
         if (this.skills[i][0] > 0) {
            if (seen == n) {
               return i;
            }

            seen++;
         }
      }

      return -1;
   }

   String skillTooltip(int skillIndex) {
      return skillNames[skillIndex] + '\n' + "Rank: " + this.skills[skillIndex][0] + '\n' + "Exp: " + this.skills[skillIndex][2] + "/10";
   }

   // knownSpellsSummary(): every known spell's name, "R: " prefixed for
   // the currently selectedSpellId.
   Vector knownSpellsSummary() {
      Vector out = new Vector();

      for (int i = 0; i < Spell.count; i++) {
         if ((this.knownSpellsMask & 1 << i) != 0) {
            int spellId = i + 1;
            String name = Spell.all[i].name;
            if (spellId == this.selectedSpellId) {
               name = "R: " + name;
            }

            out.addElement(name);
         }
      }

      return out;
   }

   int nthKnownSpellId(int n) {
      int seen = 0;

      for (int i = 0; i < Spell.count; i++) {
         if ((this.knownSpellsMask & 1 << i) != 0) {
            int spellId = i + 1;
            if (seen == n) {
               return i;
            }

            seen++;
         }
      }

      return -1;
   }

   // Cycles selectedSpellId to the next known spell (wrapping), or the
   // first known spell if selectedSpellId isn't itself known.
   int cycleSelectedSpell() {
      if (!Spell.isValidId(this.selectedSpellId)) {
         int first = this.nthKnownSpellId(0);
         return first < 0 ? 0 : first + 1;
      }

      int cur = this.selectedSpellId - 1;
      int next = cur + 1;
      if (next == Spell.count) {
         next = 0;
      }

      while (next != cur) {
         if ((this.knownSpellsMask & 1 << next) != 0) {
            return next + 1;
         }

         if (++next == Spell.count) {
            next = 0;
         }
      }

      return this.selectedSpellId;
   }

   String spellTooltip(int spellId) {
      int i = spellId;
      String out = Spell.all[i].name + '\n';
      out = out + skillNames[Spell.all[i].skillRequired] + '\n';
      out = out + "Cost: " + Spell.all[i].magickaCost + '\n';
      return out + Spell.all[i].description;
   }

   // restAndRegen(full): restores a fraction of the missing HP/Magicka/
   // Fatigue (2/3 if !full, further reduced to 3/4 of that if effect8 is
   // active), clears the 3 combat buffs, and rolls a 10% chance to remove
   // a "Haunted"(96)-tagged item plus, separately, a 25% chance per
   // ailment (skipping ailments 4/5) to cure it.
   void restAndRegen(boolean full) {
      short missingHp = (short)(this.coreStats[3] - this.coreStats[2]);
      short missingMagicka = (short)(this.coreStats[5] - this.coreStats[4]);
      short missingFatigue = (short)(this.coreStats[7] - this.coreStats[6]);
      if (!full) {
         missingHp = (short)(2 * missingHp / 3);
         missingMagicka = (short)(2 * missingMagicka / 3);
         missingFatigue = (short)(2 * missingFatigue / 3);
      }

      if (this.hasAilment(8)) {
         missingHp = (short)(3 * missingHp / 4);
         missingMagicka = (short)(3 * missingMagicka / 4);
         missingFatigue = (short)(3 * missingFatigue / 4);
      }

      this.coreStats[2] = (short)(this.coreStats[2] + missingHp);
      this.coreStats[4] = (short)(this.coreStats[4] + missingMagicka);
      this.coreStats[6] = (short)(this.coreStats[6] + missingFatigue);
      this.increaseHarmBuff = false;
      this.increaseArmorBuff = false;
      this.safeCampingBuff = false;
      int roll = ESGame.nextInt(100);
      if (roll <= 10) {
         for (int i = 0; i < this.inventoryCount; i++) {
            int id = Math.abs(this.inventoryItemIds[i]);
            if (id == 96) {
               this.removeInventorySlot(i);
               break;
            }
         }
      }

      for (int i = 0; i < 8; i++) {
         int ailmentId = i + 1;
         if (ailmentId != 4 && ailmentId != 5) {
            roll = ESGame.nextInt(100);
            if (roll <= 25) {
               this.ailmentMask = (byte)Util.clearBit(i, this.ailmentMask);
            }
         }
      }
   }

   boolean hasAilment(int ailmentId) {
      int bit = ailmentId - 1;
      return (this.ailmentMask & 1 << bit) != 0;
   }

   // useItem(slot, target): the 87-99 "gift"/special-consumable switch --
   // warp/mark camp, cure ailment, HP/Magicka/Fatigue/level-exp
   // restoratives, harm/armor/safe-camping buffs, and 3 instant-kill
   // scrolls gated on target's difficulty stats (columns 4/10 vs.
   // 13/22/29). Same id-to-effect mapping as Item.specialEffectText.
   // Menu-gate mirror of useItem's own category check (13 or 15) --
   // matches dawnstar's own canUseItem, a method ESGame's UI needs that
   // useItem() itself never exposed as a standalone predicate.
   boolean canUseItem(int slot) {
      int id = Math.abs(this.inventoryItemIds[slot]);
      byte cat = (byte)Item.column(1, id);
      return cat == 13 || cat == 15;
   }

   void useItem(int slot, Monster target) {
      int id = Math.abs(this.inventoryItemIds[slot]);
      byte cat = (byte)Item.column(1, id);
      if (cat == 13 || cat == 15) {
         boolean consume = true;
         switch (id) {
            case 87:
               if (this.currentLevel == 1 && this.hasCampMark()) {
                  this.warpToCampMark();
                  break;
               }

               this.markCampAndReturnToTown();
               break;
            case 88:
               this.cureRandomAilment();
               break;
            case 89:
               this.coreStats[2] = this.coreStats[3];
               break;
            case 90:
               this.coreStats[4] = this.coreStats[5];
               break;
            case 91:
               this.coreStats[6] = (short)(this.coreStats[6] + 3 * this.coreStats[5]);
               break;
            case 92:
               this.coreStats[1]++;
               break;
            case 93:
               this.coreStats[2] = this.coreStats[3];
               this.coreStats[4] = this.coreStats[5];
               break;
            case 94:
               this.increaseHarmBuff = true;
               break;
            case 95:
               this.increaseArmorBuff = true;
               break;
            case 96:
               this.safeCampingBuff = true;
               consume = false;
               break;
            case 97:
               if (target != null) {
                  int a = target.stat(4);
                  int b = target.stat(10);
                  if (a <= 13 && b <= 13) {
                     target.currentHp = 0;
                     target.store();
                  }
               }
               break;
            case 98:
               if (target != null) {
                  int a = target.stat(4);
                  int b = target.stat(10);
                  if (a <= 22 && b <= 22) {
                     target.currentHp = 0;
                     target.store();
                  }
               }
               break;
            case 99:
               if (target != null) {
                  int a = target.stat(4);
                  int b = target.stat(10);
                  if (a <= 29 && b <= 29) {
                     target.currentHp = 0;
                     target.store();
                  }
               }
         }

         if (consume) {
            this.removeInventorySlot(slot);
         }
      }
   }

   // Checks every skill for exp>=10, rank++/exp-=10 on a rank-up (marking
   // levelUpAttributeFlags's governing-attribute bit and granting 1
   // level-exp per rank-up); returns true (and increments coreStats[0],
   // the character level) once coreStats[1] (level-exp) reaches 10.
   boolean tryRankUpSkills() {
      for (int i = 0; i < 14; i++) {
         if (this.skills[i][2] >= 10) {
            this.skills[i][2] = (short)(this.skills[i][2] - 10);
            this.skills[i][0]++;
            short attr = skillGoverningAttribute[i];
            int bit = attr / 2;
            this.levelUpAttributeFlags = (byte)(this.levelUpAttributeFlags | 1 << bit);
            this.coreStats[1]++;
         }
      }

      if (this.coreStats[1] >= 10) {
         this.coreStats[0]++;
         return true;
      } else {
         return false;
      }
   }

   // Spends 10 level-exp on a rank-up -- see this file's header comment
   // for the surprising Shop.clearQuestTurnInState() side effect.
   void consumeLevelExp() {
      Shop.clearQuestTurnInState();
      this.coreStats[1] = (short)(this.coreStats[1] - 10);
   }

   String[] pendingLevelUpAttributeNames() {
      Vector out = new Vector();

      for (int i = 0; i < 8; i++) {
         if ((this.levelUpAttributeFlags & 1 << i) != 0) {
            int a = i * 2;
            out.addElement(attributeNames[a]);
         }
      }

      int count = out.size();
      if (count == 0) {
         return null;
      }

      String[] result = new String[count];

      for (int i = 0; i < count; i++) {
         result[i] = (String)out.elementAt(i);
      }

      return result;
   }

   private void grantStartingItems() {
      short spawnId = Item.nextSpawnId();
      int[] starting = classStartingItems[this.classIndex];

      for (int i = 0; i < starting.length; i++) {
         this.addInventoryItem(starting[i], spawnId);
         int slot = this.inventoryCount - 1;
         this.equipItem(slot, true);
      }
   }

   void tickFatigueRegen(long elapsedMs) {
      int gain = (int)(elapsedMs * (this.attributes[10] + this.attributes[11]) / 2000L);
      this.coreStats[6] = (short)(this.coreStats[6] + gain);
      if (this.coreStats[6] > this.coreStats[7]) {
         this.coreStats[6] = this.coreStats[7];
      }
   }

   // No confirmed caller found in this file -- left unresolved rather than
   // guessed at.
   private boolean TODO_isInRegion(int level, int x, int y) {
      if (level != 1) {
         return false;
      } else {
         return x < 6 || x > 12 ? false : y >= 6 && y <= 12;
      }
   }

   private void autoMarkCampOnTile() {
      this.markCampAndReturnToTown();
      if (this.justMarkedCamp) {
         this.justMarkedCamp = false;
      }
   }

   // rollShopOutcome(shopId, action): skillValue(13,true) (+3 if
   // action==3) vs. Shop.interactionCount[shopId] (was `k.r[shopId]`) as a
   // threshold, feeding rollOutcome() -- CONFIRMED via Shop.dialogue()'s
   // own call sites (actions 2/3, see this file's header comment).
   int rollShopOutcome(int shopId, int action) {
      int skill = this.skillValue(13, true);
      if (action == 3) {
         skill += 3;
      }

      short threshold = Shop.interactionCount[shopId];
      int diff = skill - threshold;
      int a = 20 - diff * 5;
      int b = 20 + this.attributes[12] / 2 + diff * 5;
      a = Math.min(Math.max(a, 10), 95);
      b = Math.min(Math.max(b, 10), 95);
      return rollOutcome(b, a);
   }

   private void decrementFacing() {
      this.facing--;
      if (this.facing <= 0) {
         this.facing = 4;
      }
   }

   private void incrementFacing() {
      this.facing++;
      if (this.facing >= 5) {
         this.facing = 1;
      }
   }

   // Big debug dump: samples all 4 relative squares (forward/backward/
   // right/left) around the player, listing any monster/chest/dropped-item
   // found in each, plus Warden presence and inventory count. Never
   // called from anywhere traced in this pass -- almost certainly a
   // developer debug command.
   String debugDumpNearbyState() {
      byte savedFacing = this.facing;
      StringBuffer out = new StringBuffer(1000);
      System.out.println("here 1");
      String[] names = this.currentDungeon().displayNames();
      out.append("Current dungeon is " + names[0] + " " + names[1] + "\n");

      for (int i = 1; i <= 4; i++) {
         if (i <= 2) {
            this.computeMoveTarget(i);
         } else if (i == 3) {
            this.incrementFacing();
            this.computeMoveTarget(1);
         } else if (i == 4) {
            this.decrementFacing();
            this.computeMoveTarget(1);
         }

         System.out.println("here 2, i =" + i);
         if (i == 1) {
            out.append("FORWARD SQUARE: \n");
         } else if (i == 2) {
            out.append("BACKWARD SQUARE: \n");
         } else if (i == 3) {
            out.append("RIGHT SIDE SQUARE: \n");
         } else if (i == 4) {
            out.append("LEFT SIDE SQUARE: \n");
         }

         out.append("x,y = " + this.pendingTileX + ", " + this.pendingTileY + "\n");
         System.out.println("New a and y are " + this.pendingTileX + ", " + this.pendingTileY);
         out.append("map value = " + this.currentDungeon().tiles[this.pendingTileX][this.pendingTileY] + "\n");
         System.out.println("New dungeon id is " + this.pendingLevel);
         Hashtable monsters = ESGame.monsters[this.pendingLevel - 1];
         if (monsters != null) {
            Enumeration e = monsters.elements();

            while (e.hasMoreElements()) {
               byte[] data = (byte[])e.nextElement();
               System.out.println("Found a monster");
               if (data[4] == this.pendingTileX && data[5] == this.pendingTileY) {
                  out.append("Found monster in square \n");
                  out.append("type=" + data[2] + ", health=" + data[3] + ", dungeon id = " + data[7] + "\n");
               }
            }
         }

         System.out.println("here 3, i =" + i);
         Hashtable chests = ESGame.chests[this.pendingLevel - 1];
         if (chests != null) {
            Enumeration e = chests.elements();

            while (e.hasMoreElements()) {
               byte[] data = (byte[])e.nextElement();
               if (data[0] == this.pendingTileX && data[1] == this.pendingTileY) {
                  out.append("Found chest in square \n");
                  out.append("item type=" + data[4] + ", value=" + data[7] + "\n");
               }
            }
         }

         System.out.println("here 4, i =" + i);
         Enumeration de = ESGame.droppedItems[this.pendingLevel - 1].elements();

         while (de.hasMoreElements()) {
            byte[] data = (byte[])de.nextElement();
            if (data[0] == this.pendingTileX && data[1] == this.pendingTileY) {
               out.append("Found dropped item in square \n");
               out.append("item type=" + data[2] + ", value=" + data[5] + " flags = " + data[6] + "\n");
            }
         }

         this.facing = savedFacing;
      }

      if (Shop.wardenPresent) {
         out.append("Warden IS visiting now\n");
      } else {
         out.append("Warden IS NOT visiting now\n");
      }

      out.append("Player inventory: nitems=" + this.inventoryCount);
      return out.toString();
   }
}
