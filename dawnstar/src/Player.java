// Renamed from decompiled/j.java. See ../docs/CLASS_MAP.md.
//
// By far the largest and most central class: player stats, inventory,
// equipment, spellbook, position/movement (including the pending-move
// staging fields used to validate a move before committing it, and the
// cross-level-boundary math), combat (attack/castOnMonster/castOnSelf
// sharing rollOutcome's hit-tier roll), the 13-slot visibleObjects cache
// GameCanvas paints from, camp/warp bookmarking, rest, leveling, and the
// two save-game serializations (toBytes/fromBytes, switched by a
// `full` boolean: true = complete in-progress save, false = lightweight
// "character summary" with no position/inventory).
//
// `ESGame`, `Monster`, `Dungeon`, `Shop`, `Item`, `Spell`, and `Util`
// are all the real, already-renamed classes -- this file was updated
// alongside ESGame's own rename pass so `currentDungeon()` now returns
// the real `Dungeon` (it used to return the old `i`, forwarding
// `ESGame.dungeons[]`'s then-unrenamed element type; both sides are
// renamed now).
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

public class Player {
   // Set true by GameCanvas.commitMove whenever Player.move crosses a
   // level boundary, per the untouched j.R semantics -- but nothing in
   // the entire codebase ever sets it true (checked every decompiled
   // file), so GameCanvas's `if (endOfGameTriggered)` branch is
   // unreachable dead code as found. Preserved as-is rather than
   // "fixed", since faithfulness to the original build is the point.
   static boolean endOfGameTriggered = false;
   // Scratch flag inside rollOutcome; written then read within the same
   // call, never carries state across calls despite being static.
   private static boolean lastRollCrit;
   // The 13-slot "what's renderable at each 3D-view object slot this
   // frame" cache GameCanvas paints from directly (GameCanvas's
   // paintVisibleObjects reads it as `j.al.elementAt(n)`, since that
   // access predates this rename and still uses the old class letter).
   static Vector visibleObjects = newVisibleObjectsVector();
   // visibleObjects slot markers: an empty slot holds EMPTY_SLOT: a
   // wall/edge directly blocks that slot -> WALL_BLOCKED_SLOT; a slot
   // occluded *because* an earlier slot in the same line of sight was
   // wall-blocked -> OCCLUDED_SLOT (see refreshVisibleObjects).
   private static final Integer EMPTY_SLOT = new Integer(0);
   private static final Integer WALL_BLOCKED_SLOT = new Integer(1);
   private static final Integer OCCLUDED_SLOT = new Integer(-1);
   static boolean charDataLoaded = false;
   static short raceCount;
   // Also set from `raceNames.length`, same value as raceCount -- no
   // distinguishing read site found; likely just redundant.
   static short raceCountRedundant;
   static String[] raceNames;
   static String[] genderNames;
   static String[] skillNames;
   // [raceIndex][41] per-race template: base attributes, base skills,
   // starting spell-knowledge thresholds -- see applyRaceTemplate.
   static short[][] raceTemplates;
   static String[] statLabels;
   static String[] attributeNames;
   // Per-skill index -> governing attribute's `attributes[]` slot,
   // read straight from charin.dat.
   static short[] skillAttributeIndex;
   // Per-race starting item id pairs, indexed by raceIndex -- see
   // grantStartingItems.
   static int[][] STARTING_ITEMS = new int[][]{{1, 27}, {7, 27}, {7, 22}, {17, 27}, {12, 22}, {17, 27}, {12, 22}};
   private static final String[] AILMENT_NAMES = new String[]{
      "Frost Limbs", "Snow Mirage", "Blind", "Troll Thirst", "Glacier Curse", "Grievous Harm", "Terrified", "Winter Worn"
   };
   // Declared, never read or written anywhere -- dead.
   public static int unusedV = -1;
   ESGame game;
   // Vestigial: set from ESGame's dead Pluto-Server-URL/Mserver-User-Id
   // JAD-property mechanism (see ESGame's own writeup in CLASS_MAP.md),
   // never read back anywhere in this class.
   public static String serverUserId = null;
   String name;
   short raceIndex;
   short genderIndex;
   // level, levelExp, curHP, maxHP, curMagicka, maxMagicka, curFatigue,
   // maxFatigue, and two more slots (coreStats[8]/[9]) whose use is
   // unconfirmed -- zeroed on rest, never otherwise touched in what's
   // been traced.
   short[] coreStats;
   // Bitmask (1 bit per attribute) of which attributes currently have a
   // pending "+1 available" from a skill ranking up -- see
   // availableAttributeIncreases/gainSkillExp/levelUp.
   byte attributeIncreaseFlags;
   int gold;
   // 8 attributes as base+bonus pairs (attributes[2*i]=base,
   // attributes[2*i+1]=bonus).
   short[] attributes;
   // Per-race magicka formula factor: maxMagicka = raceMagickaFactor *
   // attributes[2]/4 (see recalcMaxStats).
   short raceMagickaFactor;
   // Two more per-race template values (raceTemplates[raceIndex][11]/
   // [12]) -- read/written (including in the save format) but never
   // observed being used meaningfully in what's been traced.
   short[] raceUnknownPair;
   // Skills as rank/bonus/exp-toward-next-rank triples.
   short[][] skills;
   byte inventoryCount;
   // Item-type ids, negative = currently equipped.
   byte[] inventoryItemIds;
   // Packed value/charge per inventory slot.
   int[] inventoryItemData;
   // Equipped item-type per equip slot, indexed by Item's equip-slot
   // column (0=weapon,1=shield/offhand,2..6=armor slots).
   byte[] equippedItems;
   // Bitmask of known spell ids (bit (id-1) set = known).
   int knownSpellsMask;
   byte selectedSpellId;
   // "Gift points found" (character-sheet label) -- accumulated from
   // auto-collected category-11 ("gift") dropped items; see
   // pickUpDroppedItem/commitMove.
   short giftPointsFound;
   // Rumor-reveal-step counter for Shop's mystery-subplot hint feed
   // (Shop.RUMOR_STRING_OFFSET[traitorIndex][rumorRevealStep]).
   short rumorRevealStep;
   // 8-bit active-ailment mask, matches AILMENT_NAMES.
   byte ailmentMask;
   // Countdown timers for 3 of the 8 ailments (bit index 3/4/6); the
   // other 5 appear to be binary/durationless in what's been traced, or
   // use effectDurations[] instead.
   short trollThirstTimer;
   short glacierCurseTimer;
   short terrifiedTimer;
   // Read/written (including in the save format) but never observed
   // being used meaningfully in what's been traced.
   boolean unconfirmedZ;
   byte currentLevel;
   byte tileX;
   byte tileY;
   byte facing;
   // Camp/warp bookmark (level, x, y, facing) set by
   // markCampAndReturnToTown and consumed by warpToCampMark/hasCampMark
   // -- the "Warp to Camp" item and the "walk onto a camp-marker tile"
   // mechanic both bookmark here.
   byte campLevel;
   byte campX;
   byte campY;
   byte campFacing;
   // Generic spell/effect duration timers, -1=until cured, -2=until a
   // condition check (isEffectActive) rather than a countdown.
   byte[] effectDurations;
   // Current combat target's spawnId (Monster.spawnId), used by
   // isEffectActive's -2 case (bit-13 "until in combat" condition).
   short combatTargetSpawnId;
   // Scratch magnitude for the effectDurations[17] buff (see
   // castOnMonster case 17 / armorValue).
   short tempArmorBonus;
   // "Increase Harm"(+25 weapon damage)/"Increase Armor"(+15
   // armor)/"Safe Camping" buffs, granted by useItem ids 94/95/96.
   boolean increaseHarmBuff = false;
   boolean increaseArmorBuff = false;
   boolean safeCampingBuff = false;
   // Pending move target tile/level/facing -- computeMoveTarget fills
   // these in, commitMove applies them (letting a move be validated,
   // including cross-level-boundary math, before committing it).
   byte pendingTileX;
   byte pendingTileY;
   byte pendingLevel;
   byte pendingFacing;
   // Set true by computeMoveTarget when the pending move crosses a
   // level boundary; consumed by GameCanvas.commitMove for the
   // level-name popup.
   boolean levelChanged;
   // One-shot flag set after a camp-mark/warp/death-reset, consumed by
   // the next move() call to skip its strafe-fallback adjustment.
   boolean suppressStrafeAdjust;
   // Corridor tile-occlusion view grid (9 wide x 5 deep), populated by
   // refreshCorridorView and read via tileAt(dx,dy) -- GameCanvas's
   // corridor renderer and minimap both sample through it. Previously
   // flagged in CLASS_MAP.md as an unconfirmed "byte[9][5] built from a
   // strange formula"; confirmed while renaming this class (see
   // tileAt's doc comment) once GameCanvas's own call sites -- which
   // pass (dx, dy) offsets, not (shopId, slot) as first guessed -- were
   // cross-checked against this file.
   byte[][] corridorView;
   // Previous tile (for the "just arrived here" chest/item auto-trigger
   // check in commitMove).
   byte prevTileX;
   byte prevTileY;
   // General-purpose one-time event/dialogue flags. Indices 90-95 are
   // confirmed: Shop's rumor system uses them as "has rumor-reveal-step
   // N already been shown" markers (see Shop.dialogue). The rest (0-89)
   // have no confirmed read site but are presumably similar one-time
   // story/dialogue flags given the shared array and packed-bit save
   // format.
   boolean[] eventFlags = new boolean[96];
   // Packed into the save format alongside traitorIndex. Confirmed via
   // ESGame's NPCQuestionWhomUI handler: counts (capped at 3) how many
   // times the player has asked the actual traitor's shop (shopId-5 ==
   // traitorIndex) about a topic; once it reaches 2 (or 3 with a 20%
   // roll) the answer switches to the "I suspect you..." traitor-reveal
   // flavor text and marks that (topic, target) pair as confirmed.
   byte traitorSuspicionCount = 0;
   // Hidden "traitor" index (0-3), rolled at character creation --
   // Shop's rumor system gradually reveals which of 4 candidates it is.
   byte traitorIndex = 0;
   // Set when the type-41 "roaming" special monster (nicknamed "gehen"
   // in the original debug prints) has been dealt with -- gates the
   // rare scripted campState==3 "disturbed camp" event in GameCanvas
   // and is set by GameCanvas.resolveMonsterDeath on killing it.
   boolean specialEncounterResolved = false;
   // True while the type-41 "roaming" special monster is believed to be
   // alive on the current level -- see the duplicated "remove roaming
   // gehen" cleanup block in resetToHubPosition/computeMoveTarget/rest.
   boolean roamingSpecialMonsterPresent = false;
   // "New Game+" flag -- set externally by ESGame on a NG+ continuation;
   // selects the alternate ambush-checkpoint schedule in GameCanvas.
   boolean newGamePlus = false;
   // Per-second "overstayed in one place" ambush counter, -1 = inactive
   // (see GameCanvas.tickPerSecond).
   int ambushTimer = -1;
   boolean levelUpPending = false;
   // Set once grantStarFrostItem has run; grants a flat +4 bonus to
   // every skillValue() check thereafter.
   boolean starFrostBonusActive = false;

   // Estimated save-buffer size in bytes for toBytes's ByteArrayOutputStream.
   int estimatedSaveSize(boolean full) {
      return full ? 400 : 200;
   }

   public Player(ESGame game) {
      endOfGameTriggered = false;
      ensureCharDataLoaded();
      this.name = null;
      this.coreStats = new short[10];
      this.attributes = new short[16];
      this.raceUnknownPair = new short[2];
      this.skills = new short[14][3];
      this.inventoryCount = 0;
      this.inventoryItemIds = new byte[24];
      this.inventoryItemData = new int[24];
      this.equippedItems = new byte[7];
      this.effectDurations = new byte[25];
      this.corridorView = new byte[9][5];
      this.game = game;
      this.suppressStrafeAdjust = false;
   }

   // Normalizes a copy of coreStats for the lightweight "character
   // summary" save format: current = max for HP/Magicka/Fatigue, and
   // coreStats[8] zeroed.
   void normalizeForSummary(short[] stats) {
      stats[2] = stats[3];
      stats[4] = stats[5];
      stats[6] = stats[7];
      stats[8] = 0;
   }

   // Character-creation initializer: applies raceIndex's template
   // (attributes, magicka/fatigue factors, starting skills), resets
   // level/exp/gold, rolls the hidden traitor index, and computes the
   // starting known-spell mask.
   void applyRaceTemplate(int race) {
      this.raceIndex = (short)race;
      this.genderIndex = raceTemplates[this.raceIndex][1];
      byte attrCount = 8;

      for (int i = 0; i < attrCount; i++) {
         int slot = 2 * i;
         this.attributes[slot] = raceTemplates[this.raceIndex][2 + i];
         this.attributes[slot + 1] = 0;
      }

      this.raceMagickaFactor = raceTemplates[this.raceIndex][10];
      this.raceUnknownPair[0] = raceTemplates[this.raceIndex][11];
      this.raceUnknownPair[1] = raceTemplates[this.raceIndex][12];
      this.coreStats[0] = 1;
      this.coreStats[1] = 0;
      this.recalcMaxStats();
      this.coreStats[2] = this.coreStats[3];
      this.coreStats[4] = this.coreStats[5];
      this.coreStats[6] = this.coreStats[7];
      this.coreStats[8] = 0;
      this.coreStats[9] = 0;
      this.attributeIncreaseFlags = 0;
      this.gold = 50;
      this.traitorIndex = (byte)(Util.randomInt(4) - 1);
      System.out.println("traitor is " + this.traitorIndex);
      int col = 13;

      for (int i = 0; i < 14; i++) {
         this.skills[i][0] = raceTemplates[this.raceIndex][col++];
         this.skills[i][1] = raceTemplates[this.raceIndex][col++];
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

   // maxHP/maxMagicka/maxFatigue from the current attributes.
   void recalcMaxStats() {
      this.coreStats[3] = (short)((this.attributes[0] + this.attributes[10]) / 2);
      this.coreStats[5] = (short)(this.raceMagickaFactor * this.attributes[2] / 4);
      this.coreStats[7] = (short)(this.attributes[0] + this.attributes[4] + this.attributes[6] + this.attributes[10]);
   }

   // Starting known-spell bitmask from the race template's per-skill
   // thresholds: for 5 specific skill slots (mapped to bit positions
   // 0/5/10/15/20, i.e. spell ids 1/6/11/16/21), a nonzero template
   // value grants that tier's first spell. Also sets selectedSpellId to
   // the first one granted.
   private int computeStartingSpellMask() {
      int mask = 0;
      int col = 13;
      byte bit = -1;
      boolean first = true;

      for (int i = 0; i < 14; i++) {
         short threshold = raceTemplates[this.raceIndex][col++];
         short unused = raceTemplates[this.raceIndex][col++];
         switch (i) {
            case 1:
               bit = 0;
               break;
            case 2:
            case 5:
            case 7:
            case 8:
            case 9:
            default:
               bit = -1;
               break;
            case 3:
               bit = 5;
               break;
            case 4:
               bit = 10;
               break;
            case 6:
               bit = 15;
               break;
            case 10:
               bit = 20;
         }

         if (bit != -1 && threshold > 0) {
            mask |= 1 << bit;
            if (first) {
               this.selectedSpellId = (byte)(bit + 1);
               first = false;
            }
         }
      }

      return mask;
   }

   // Resets combat/status/position state. `respawning` = true keeps the
   // camp-point bookmark and skips re-granting starting items (used
   // after a normal death); false wipes the camp point and grants a
   // fresh set of starting items (used starting a brand-new character).
   public void resetState(boolean respawning) {
      if (!respawning) {
         this.giftPointsFound = 0;
         this.rumorRevealStep = 0;
      }

      this.ailmentMask = 0;
      this.trollThirstTimer = 0;
      this.glacierCurseTimer = 0;
      this.terrifiedTimer = 0;
      this.unconfirmedZ = false;
      this.resetToHubPosition(respawning);
      if (!respawning) {
         this.campLevel = 0;
         this.campX = 0;
         this.campY = 0;
         this.campFacing = 0;
      }

      for (int i = 0; i < 25; i++) {
         this.effectDurations[i] = 0;
      }

      this.combatTargetSpawnId = 0;
      this.tempArmorBonus = 0;
      this.increaseHarmBuff = false;
      this.increaseArmorBuff = false;
      this.safeCampingBuff = false;
      if (!respawning) {
         this.grantStartingItems();
      }
   }

   // Resets position to one of two fixed level-1 (hub) entry points,
   // and cleans up the type-41 "roaming" special monster if it's still
   // tracked as alive.
   public void resetToHubPosition(boolean altSpawn) {
      if (this.roamingSpecialMonsterPresent) {
         Hashtable levelMonsters = ESGame.monsters[this.currentLevel - 1];
         if (levelMonsters != null) {
            Monster scratch = new Monster();
            Enumeration monsters = levelMonsters.elements();

            while (monsters.hasMoreElements()) {
               byte[] rec = (byte[])monsters.nextElement();
               Monster.fromBytes(scratch, rec);
               if (scratch.monsterType == 41) {
                  this.roamingSpecialMonsterPresent = false;
                  ESGame.removeMonster(this.currentLevel, scratch.x, scratch.y);
                  break;
               }
            }
         }

         if (this.roamingSpecialMonsterPresent) {
            System.out.println("Remove roaming gehen failed");
         }
      }

      if (!altSpawn) {
         this.currentLevel = this.pendingLevel = 1;
         this.tileX = this.pendingTileX = 9;
         this.tileY = this.pendingTileY = 9;
         this.facing = this.pendingFacing = 1;
      } else {
         this.currentLevel = this.pendingLevel = 1;
         this.tileX = this.pendingTileX = 13;
         this.tileY = this.pendingTileY = 6;
         this.facing = this.pendingFacing = 4;
      }

      if (this.game.gameCanvas != null) {
         this.refreshCorridorView();
         this.game.gameCanvas.refreshChestInSight();
         this.game.gameCanvas.refreshNpcInSight();
      }
   }

   // Shorter creation-time summary string (race/gender/level/HP/Magicka/
   // Fatigue/attributes), distinct from the fuller buildCharacterSheet.
   String buildCreationSummary() {
      StringBuffer out = new StringBuffer(300);
      String sp = " ";
      String colon = ": ";
      out.append(genderNames[this.genderIndex]);
      out.append(sp);
      out.append(raceNames[this.raceIndex]);
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
         int slot = 2 * i;
         out.append(attributeNames[slot]);
         out.append(colon);
         out.append(this.attributes[slot]);
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
            loadCharacterData();
            charDataLoaded = true;
         } catch (Exception e) {
            System.out.println("Error: could not load character data");
            System.out.println("Exception: " + e);
         }
      }
   }

   private static void loadCharacterData() throws Exception {
      DataInputStream in = ESGame.getResource("charin.dat");
      int available = in.available();
      statLabels = readStringArray(in);
      attributeNames = readStringArray(in);
      raceNames = readStringArray(in);
      raceCount = (short)raceNames.length;
      genderNames = readStringArray(in);
      raceCountRedundant = (short)raceNames.length;
      skillNames = readStringArray(in);
      short skillCount = (short)skillNames.length;
      if (skillCount != 14) {
         throw new Exception("Error: mismatch between input number of skill types and that specified in code");
      }

      skillAttributeIndex = new short[skillCount];

      for (int i = 0; i < skillCount; i++) {
         skillAttributeIndex[i] = in.readShort();
      }

      int cols = 13 + 2 * skillCount;
      raceTemplates = new short[raceCount][cols];

      for (int r = 0; r < raceCount; r++) {
         for (int c = 0; c < cols; c++) {
            raceTemplates[r][c] = in.readShort();
         }
      }

      in.close();
   }

   private static String[] readStringArray(DataInputStream in) throws Exception {
      short count = in.readShort();
      String[] out = new String[count];

      for (int i = 0; i < count; i++) {
         out[i] = in.readUTF();
      }

      return out;
   }

   // Deserializes either save format (see toBytes for the layout).
   static Player fromBytes(byte[] data, boolean full) throws Exception {
      Player p = null;
      ByteArrayInputStream bytesIn = new ByteArrayInputStream(data, 0, data.length);
      DataInputStream in = new DataInputStream(bytesIn);
      p = new Player(null);
      p.name = in.readUTF();
      p.raceIndex = in.readShort();
      if (!full) {
         p.applyRaceTemplate(p.raceIndex);
         p.resetState(false);
      }

      p.genderIndex = in.readShort();

      for (int i = 0; i < 10; i++) {
         p.coreStats[i] = in.readShort();
      }

      if (full) {
         p.attributeIncreaseFlags = in.readByte();
      }

      p.gold = in.readInt();

      for (int i = 0; i < 16; i++) {
         p.attributes[i] = in.readShort();
      }

      p.raceMagickaFactor = in.readShort();
      p.raceUnknownPair[0] = in.readShort();
      p.raceUnknownPair[1] = in.readShort();

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
         p.ailmentMask = in.readByte();
         p.trollThirstTimer = in.readShort();
         p.glacierCurseTimer = in.readShort();
         p.terrifiedTimer = in.readShort();
         p.unconfirmedZ = in.readBoolean();
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

         p.combatTargetSpawnId = in.readShort();
         p.tempArmorBonus = in.readShort();
         p.increaseHarmBuff = in.readBoolean();
         p.increaseArmorBuff = in.readBoolean();
         p.safeCampingBuff = in.readBoolean();
         byte packed = 0;
         packed = in.readByte();
         p.roamingSpecialMonsterPresent = (packed & 32) == 32;
         p.specialEncounterResolved = (packed & 16) == 16;
         p.traitorSuspicionCount = (byte)(packed % 4);
         p.traitorIndex = (byte)((packed >> 2) % 4);
         System.out.println("traitor is " + p.traitorIndex);
         int flagIdx = 0;

         while (flagIdx < 96) {
            packed = in.readByte();
            p.eventFlags[flagIdx++] = (packed & 128) != 0;
            p.eventFlags[flagIdx++] = (packed & 64) != 0;
            p.eventFlags[flagIdx++] = (packed & 32) != 0;
            p.eventFlags[flagIdx++] = (packed & 16) != 0;
            p.eventFlags[flagIdx++] = (packed & 8) != 0;
            p.eventFlags[flagIdx++] = (packed & 4) != 0;
            p.eventFlags[flagIdx++] = (packed & 2) != 0;
            p.eventFlags[flagIdx++] = (packed & 1) != 0;
         }
      }

      try {
         in.close();
      } catch (Exception e) {
      }

      return p;
   }

   // Serializes either save format: `full` = complete in-progress save
   // (everything including inventory/position/status); not full = the
   // lightweight "character summary" (no position/inventory) -- likely
   // a high-score/leaderboard record, needs confirming against where
   // the not-full path is actually called from ESGame.
   byte[] toBytes(boolean full) throws Exception {
      int size = this.estimatedSaveSize(full);
      ByteArrayOutputStream bytesOut = new ByteArrayOutputStream(size);
      DataOutputStream out = new DataOutputStream(bytesOut);
      out.writeUTF(this.name);
      out.writeShort(this.raceIndex);
      out.writeShort(this.genderIndex);
      if (full) {
         for (int i = 0; i < 10; i++) {
            out.writeShort(this.coreStats[i]);
         }

         out.writeByte(this.attributeIncreaseFlags);
      } else {
         short[] summary = new short[10];

         for (int i = 0; i < 10; i++) {
            summary[i] = this.coreStats[i];
         }

         this.normalizeForSummary(summary);

         for (int i = 0; i < 10; i++) {
            out.writeShort(summary[i]);
         }
      }

      out.writeInt(this.gold);

      for (int i = 0; i < 16; i++) {
         out.writeShort(this.attributes[i]);
      }

      out.writeShort(this.raceMagickaFactor);
      out.writeShort(this.raceUnknownPair[0]);
      out.writeShort(this.raceUnknownPair[1]);

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
         out.writeByte(this.ailmentMask);
         out.writeShort(this.trollThirstTimer);
         out.writeShort(this.glacierCurseTimer);
         out.writeShort(this.terrifiedTimer);
         out.writeBoolean(this.unconfirmedZ);
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

         out.writeShort(this.combatTargetSpawnId);
         out.writeShort(this.tempArmorBonus);
         out.writeBoolean(this.increaseHarmBuff);
         out.writeBoolean(this.increaseArmorBuff);
         out.writeBoolean(this.safeCampingBuff);
         int packed = 0;
         packed = (byte)(this.traitorIndex << 2 + this.traitorSuspicionCount);
         if (this.specialEncounterResolved) {
            packed = (byte)(packed + 16);
         }

         if (this.roamingSpecialMonsterPresent) {
            packed = (byte)(packed + 32);
         }

         out.writeByte(packed);
         int flagIdx = 0;

         while (flagIdx < 96) {
            packed = this.eventFlags[flagIdx++] ? -128 : 0;
            byte b = (byte)(packed + (this.eventFlags[flagIdx++] ? 64 : 0));
            b = (byte)(b + (this.eventFlags[flagIdx++] ? 32 : 0));
            b = (byte)(b + (this.eventFlags[flagIdx++] ? 16 : 0));
            b = (byte)(b + (this.eventFlags[flagIdx++] ? 8 : 0));
            b = (byte)(b + (this.eventFlags[flagIdx++] ? 4 : 0));
            b = (byte)(b + (this.eventFlags[flagIdx++] ? 2 : 0));
            b = (byte)(b + (this.eventFlags[flagIdx++] ? 1 : 0));
            out.writeByte(b);
         }
      }

      bytesOut.flush();
      byte[] result = bytesOut.toByteArray();

      try {
         out.close();
      } catch (Exception e) {
      }

      return result;
   }

   // Computes the pending move target (pendingTileX/Y/Level/Facing) for
   // direction 1-4 (N/E/S/W) or the two turn actions (3=turn right,
   // 4=turn left), including the cross-level-boundary math when a move
   // would step off the current level's edge (using the target level's
   // own geomin.dat neighbor row to figure out where you land), and the
   // duplicated "remove roaming gehen" cleanup once a boundary is
   // actually crossed.
   void computeMoveTarget(int direction) {
      byte delta = -1;
      switch (direction) {
         case 1:
            delta = 1;
         case 2:
            this.pendingFacing = this.facing;
            if (this.facing == 1) {
               this.pendingTileX = this.tileX;
               this.pendingTileY = (byte)(this.tileY - delta);
            } else if (this.facing == 3) {
               this.pendingTileX = this.tileX;
               this.pendingTileY = (byte)(this.tileY + delta);
            } else if (this.facing == 2) {
               this.pendingTileX = (byte)(this.tileX + delta);
               this.pendingTileY = this.tileY;
            } else if (this.facing == 4) {
               this.pendingTileX = (byte)(this.tileX - delta);
               this.pendingTileY = this.tileY;
            }

            Dungeon level = ESGame.dungeons[this.currentLevel - 1];
            if (this.pendingTileX < 0) {
               this.levelChanged = true;
               this.pendingLevel = level.neighbors[3];
               Dungeon target = ESGame.dungeons[this.pendingLevel - 1];
               if (this.pendingLevel != 1 && this.currentLevel != 1) {
                  this.pendingTileX = (byte)(target.width - 1);
               } else {
                  this.pendingTileX = (byte)(target.width - 1);
                  this.pendingTileY = (byte)(this.pendingTileY + (target.height - level.height) / 2);
               }
            } else if (this.pendingTileX >= level.width) {
               this.levelChanged = true;
               this.pendingLevel = level.neighbors[1];
               Dungeon target = ESGame.dungeons[this.pendingLevel - 1];
               if (this.pendingLevel != 1 && this.currentLevel != 1) {
                  this.pendingTileX = 0;
               } else {
                  this.pendingTileX = 0;
                  this.pendingTileY = (byte)(this.pendingTileY + (target.height - level.height) / 2);
               }
            } else if (this.pendingTileY < 0) {
               this.levelChanged = true;
               this.pendingLevel = level.neighbors[0];
               Dungeon target = ESGame.dungeons[this.pendingLevel - 1];
               if (this.pendingLevel != 1 && this.currentLevel != 1) {
                  this.pendingTileY = (byte)(target.height - 1);
               } else {
                  this.pendingTileX = (byte)(this.pendingTileX + (target.width - level.width) / 2);
                  this.pendingTileY = (byte)(target.height - 1);
               }
            } else if (this.pendingTileY >= level.height) {
               this.levelChanged = true;
               this.pendingLevel = ESGame.dungeons[this.currentLevel - 1].neighbors[2];
               Dungeon target = ESGame.dungeons[this.pendingLevel - 1];
               if (this.pendingLevel != 1 && this.currentLevel != 1) {
                  this.pendingTileY = 0;
               } else {
                  this.pendingTileX = (byte)(this.pendingTileX + (target.width - level.width) / 2);
                  this.pendingTileY = 0;
               }
            } else {
               this.levelChanged = false;
               this.pendingLevel = this.currentLevel;
            }

            if (this.levelChanged && this.roamingSpecialMonsterPresent) {
               Hashtable levelMonsters = ESGame.monsters[this.currentLevel - 1];
               if (levelMonsters != null) {
                  Monster scratch = new Monster();
                  Enumeration monsters = levelMonsters.elements();

                  while (monsters.hasMoreElements()) {
                     byte[] rec = (byte[])monsters.nextElement();
                     Monster.fromBytes(scratch, rec);
                     if (scratch.monsterType == 41) {
                        this.roamingSpecialMonsterPresent = false;
                        ESGame.removeMonster(this.currentLevel, scratch.x, scratch.y);
                        break;
                     }
                  }
               }

               if (this.roamingSpecialMonsterPresent) {
                  System.out.println("Remove roaming gehen failed");
               }
            }
            break;
         case 3:
            this.pendingLevel = this.currentLevel;
            this.levelChanged = false;
            this.pendingFacing = (byte)(this.facing + 1);
            if (this.pendingFacing > 4) {
               this.pendingFacing = 1;
            }

            this.pendingTileX = this.tileX;
            this.pendingTileY = this.tileY;
            break;
         case 4:
            this.pendingLevel = this.currentLevel;
            this.levelChanged = false;
            this.pendingFacing = (byte)(this.facing - 1);
            if (this.pendingFacing < 1) {
               this.pendingFacing = 4;
            }

            this.pendingTileX = this.tileX;
            this.pendingTileY = this.tileY;
      }
   }

   // Validates and (if valid) commits a move in direction 1-4, or a
   // strafe pair when `strafe` is true and direction is 3/4 (turn-then-
   // step, falling back to a plain turn if the step fails, unless
   // suppressStrafeAdjust is set). Returns whether the move happened.
   boolean move(int direction, boolean strafe) {
      Dungeon level = this.currentDungeon();
      if (this.coreStats[6] <= 0) {
         return false;
      }

      boolean savedLevelChanged = false;
      boolean moved = false;
      if (strafe && direction == 4) {
         this.commitMove(4);
         moved = this.commitMove(1);
         if (!this.suppressStrafeAdjust) {
            savedLevelChanged = this.levelChanged;
            moved = this.commitMove(3);
            this.levelChanged = savedLevelChanged;
         }
      } else if (strafe && direction == 3) {
         this.commitMove(3);
         moved = this.commitMove(1);
         if (!this.suppressStrafeAdjust) {
            savedLevelChanged = this.levelChanged;
            moved = this.commitMove(4);
            this.levelChanged = savedLevelChanged;
         }
      } else {
         moved = this.commitMove(direction);
      }

      this.suppressStrafeAdjust = false;
      return moved;
   }

   // Applies computeMoveTarget(direction) if the resulting tile is
   // walkable: updates position/facing, refreshes the level's
   // "just visited" marker, spends Fatigue (real moves only, not
   // turns), auto-collects a fully-looted dropped-item pile, and
   // refreshes the view (or triggers death if the tile was instant-
   // lethal, bit 8).
   boolean commitMove(int direction) {
      if (this.coreStats[6] <= 0) {
         return false;
      }

      if (direction == 0) {
         return false;
      }

      this.computeMoveTarget(direction);
      if (this.pendingLevel <= 0) {
         return false;
      }

      Dungeon target = ESGame.dungeons[this.pendingLevel - 1];
      if (!target.populated) {
         return false;
      }

      byte tile = target.tiles[this.pendingTileX][this.pendingTileY];
      if (!this.isWalkable(tile)) {
         return false;
      }

      this.currentLevel = this.pendingLevel;
      this.prevTileX = this.tileX;
      this.prevTileY = this.tileY;
      this.tileX = this.pendingTileX;
      this.tileY = this.pendingTileY;
      this.facing = this.pendingFacing;
      target.visited = true;
      if (direction == 1 || direction == 2) {
         if (Shop.showDeathGreeting) {
            Shop.showDeathGreeting = false;
         }

         this.coreStats[6] = (short)(this.coreStats[6] - 1 * this.fatigueCostMultiplier());
         this.coreStats[6] = (short)Math.max(this.coreStats[6], 0);
      }

      boolean hasDroppedItems = (tile & 4) != 0;
      if (hasDroppedItems && (direction == 1 || direction == 2)) {
         boolean allLooted = true;
         Vector items = target.droppedItemsAt(this.tileX, this.tileY);
         Enumeration it = items.elements();

         while (it.hasMoreElements()) {
            byte[] rec = (byte[])it.nextElement();
            boolean looted = this.tryAddDroppedItem(rec);
            if (looted) {
               target.removeDroppedItem(rec);
               if ((rec[6] & 2) == 0) {
                  int itemIdx = rec[2] - 1;
                  if (Item.category[itemIdx] == 11) {
                     this.giftPointsFound = (short)(this.giftPointsFound + Item.subtype[itemIdx]);
                  }
               }
            } else {
               allLooted = false;
            }
         }

         if (allLooted) {
            target.clearDroppedItemFlag(this.tileX, this.tileY);
         }
      }

      if ((tile & 8) == 0 || direction != 1 && direction != 2) {
         this.refreshCorridorView();
      } else {
         this.markCampAndReturnToTown(false);
      }

      return true;
   }

   // Wall(bit0)/blocked(bit5)/monster(bit1) test for a move target tile.
   boolean isWalkable(byte tileBits) {
      if ((tileBits & 1) != 0) {
         return false;
      } else {
         return (tileBits & 32) != 0 ? false : (tileBits & 2) == 0;
      }
   }

   // Resolves an attack against `target`: rolls hit tier via
   // rollOutcome(attackAccuracy, target's evasion-ish stat), applies
   // damage (weaponDamage - target defense, min 4, scaled by the
   // target's type multiplier), and on a strong hit awards weapon-skill
   // exp.
   void attack(Monster target) {
      this.combatTargetSpawnId = target.spawnId;
      byte type = target.monsterType;
      int power = this.weaponDamage();
      int targetDef = target.stat(7);
      int diff = power - targetDef;
      diff = Math.min(diff, target.stat(2));
      if (this.isEffectActive(10)) {
         if (target.scratch[8] == 0) {
            this.clearEffect(10);
         } else {
            diff += target.scratch[8];
         }
      }

      int defChance = target.stat(6) - diff * 5;
      int atkChance = this.attackAccuracy() + diff * 5;
      defChance = Math.min(Math.max(defChance, 10), 95);
      atkChance = Math.min(Math.max(atkChance, 10), 95);
      int outcome = rollOutcome(atkChance, defChance);
      if (outcome != 0) {
         int damage = this.weaponDamage();
         int targetArmor = target.stat(8);
         if (this.isEffectActive(13)) {
            if (target.scratch[5] == 0) {
               this.effectDurations[12] = 0;
            } else {
               targetArmor -= target.scratch[5];
            }
         }

         if (outcome == 1) {
            targetArmor = 2 * targetArmor;
         } else if (outcome == 3) {
            damage = 2 * damage;
         }

         int rawDamage = damage - targetArmor;
         rawDamage = Math.max(rawDamage, 4);
         int scaled = rawDamage * target.stat(14) / 100;
         target.takeDamage(scaled);
         target.store();
         if (this.isEffectActive(7)) {
            if (target.scratch[1] == 0) {
               this.clearEffect(7);
            } else {
               int fatigueDmg = target.scratch[1];
               fatigueDmg = Math.max(fatigueDmg, 4);
               scaled = fatigueDmg * target.stat(14) / 100;
               target.takeDamage(scaled);
            }
         }

         if (outcome >= 2) {
            this.gainSkillExp(this.activeWeaponSkillIndex(), 1);
         }

         if (!this.isEffectActive(7)) {
            this.coreStats[6] = (short)(this.coreStats[6] - 7 * this.fatigueCostMultiplier());
            this.coreStats[6] = (short)Math.max(this.coreStats[6], 0);
         }

         if (this.hasAilment(6)) {
            int drain = 2 * this.coreStats[3] / 100;
            if (drain < 1) {
               drain = 1;
            }

            this.coreStats[2] = (short)(this.coreStats[2] - (short)drain);
         }
      }
   }

   // Marks a dropped-item or monster-loot record as looted: kind 1
   // (monster) also stashes its display name and flags it collected;
   // kind 2 (dropped item) just flags it collected.
   void markLooted(int kind, Object obj) {
      byte[] rec = null;
      Object name = null;
      switch (kind) {
         case 1:
            rec = (byte[])obj;
            rec[6] = 1;
            Monster scratch = new Monster();
            Monster.fromBytes(scratch, rec);
            name = String.valueOf(scratch.spawnId);
            scratch.flag = true;
            scratch.store();
            break;
         case 2:
            rec = (byte[])obj;
            rec[6] = (byte)(rec[6] | 1);
      }
   }

   // Rebuilds visibleObjects for the current level/tile every tick:
   // dead monsters get marked looted, chests/dropped items get placed,
   // and the current level's shop NPC(s) get tagged into view (all 5
   // hub peddlers in the hub town, or the single named shopkeeper for
   // levels 3/12/21/30).
   void tickVisibleObjects(boolean unused) {
      this.refreshVisibleObjects();
      Hashtable monsters = ESGame.monsters[this.currentLevel - 1];
      if (monsters != null) {
         Enumeration it = monsters.elements();

         while (it.hasMoreElements()) {
            byte[] rec = (byte[])it.nextElement();
            if (this.placeVisibleObject(1, rec)) {
               this.markLooted(1, rec);
            }
         }
      }

      Hashtable chests = ESGame.chests[this.currentLevel - 1];
      if (chests != null) {
         Enumeration it = chests.elements();

         while (it.hasMoreElements()) {
            byte[] rec = (byte[])it.nextElement();
            this.placeVisibleObject(4, rec);
         }
      }

      Vector dropped = ESGame.droppedItems[this.currentLevel - 1];
      if (dropped != null) {
         Enumeration it = dropped.elements();

         while (it.hasMoreElements()) {
            byte[] rec = (byte[])it.nextElement();
            if (this.placeVisibleObject(2, rec)) {
               this.markLooted(2, rec);
            }
         }
      }

      if (this.currentLevel == 1) {
         for (int i = 0; i < 5; i++) {
            this.placeVisibleObject(6, Shop.NAMES[i]);
         }
      } else if (this.currentLevel == 3) {
         this.placeVisibleObject(6, "A");
      } else if (this.currentLevel == 12) {
         this.placeVisibleObject(6, "B");
      } else if (this.currentLevel == 21) {
         this.placeVisibleObject(6, "C");
      } else if (this.currentLevel == 30) {
         this.placeVisibleObject(6, "D");
      }
   }

   // CLASS_MAP.md's flagged "strange formula" lookup table accessor --
   // structure (a 9x5 grid indexed by a recentered offset) is clear,
   // but exactly what each cell of corridorView stores in this call
   // shape isn't pinned down. Kept exactly as found.
   byte lookupUnconfirmedTable(int a, int b) {
      return b < 4 ? this.corridorView[a + b + 1][b] : this.corridorView[a + b][b];
   }

   // Same formula/table as lookupUnconfirmedTable, but this is the
   // shape GameCanvas actually calls (as `tileAt(dx, dy)`) to test
   // occlusion bits for the corridor renderer and minimap -- confirming
   // corridorView is the tile-occlusion view grid, not whatever
   // lookupUnconfirmedTable's original call sites use it for.
   byte tileAt(int dx, int dy) {
      return dy < 4 ? this.corridorView[dx + dy + 1][dy] : this.corridorView[dx + dy][dy];
   }

   // Rebuilds visibleObjects' 13 slots from scratch by sampling
   // corridorView at each of the fixed forward/diagonal offsets used by
   // the 3D view, marking any wall-blocked slot as WALL_BLOCKED_SLOT and
   // then propagating occlusion to the slots that sit behind it.
   void refreshVisibleObjects() {
      byte bits = 0;

      for (int i = 0; i < 13; i++) {
         visibleObjects.setElementAt(EMPTY_SLOT, i);
      }

      bits = this.tileAt(-1, 1);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 0);
      }

      bits = this.tileAt(0, 1);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 1);
      }

      bits = this.tileAt(1, 1);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 2);
      }

      bits = this.tileAt(-2, 2);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 3);
      }

      bits = this.tileAt(-1, 2);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 4);
      }

      bits = this.tileAt(0, 2);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 5);
      }

      bits = this.tileAt(1, 2);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 6);
      }

      bits = this.tileAt(2, 2);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 7);
      }

      bits = this.tileAt(-2, 3);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 8);
      }

      bits = this.tileAt(-1, 3);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 9);
      }

      bits = this.tileAt(0, 3);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 10);
      }

      bits = this.tileAt(1, 3);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 11);
      }

      bits = this.tileAt(2, 3);
      if (Util.testBit((byte)1, bits)) {
         visibleObjects.setElementAt(WALL_BLOCKED_SLOT, 12);
      }

      if (isOccludedSlot(visibleObjects.elementAt(0))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 4);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 8);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 9);
      }

      if (isOccludedSlot(visibleObjects.elementAt(1))) {
         for (int i = 0; i < 13; i++) {
            if (i != 1) {
               visibleObjects.setElementAt(OCCLUDED_SLOT, i);
            }
         }
      }

      if (isOccludedSlot(visibleObjects.elementAt(2))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 6);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 11);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 12);
      }

      if (isOccludedSlot(visibleObjects.elementAt(3))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 8);
      }

      if (isOccludedSlot(visibleObjects.elementAt(4))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 8);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 9);
      }

      if (isOccludedSlot(visibleObjects.elementAt(5))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 9);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 10);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 11);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 4);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 6);
      }

      if (isOccludedSlot(visibleObjects.elementAt(6))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 11);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 12);
      }

      if (isOccludedSlot(visibleObjects.elementAt(7))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 12);
      }

      if (isOccludedSlot(visibleObjects.elementAt(9))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 8);
      }

      if (isOccludedSlot(visibleObjects.elementAt(10))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 9);
         visibleObjects.setElementAt(OCCLUDED_SLOT, 11);
      }

      if (isOccludedSlot(visibleObjects.elementAt(11))) {
         visibleObjects.setElementAt(OCCLUDED_SLOT, 12);
      }
   }

   // Places `obj` (a monster/chest/dropped-item record, or an NPC name
   // String for kind 6) into the correct visibleObjects slot based on
   // its position relative to the player's facing. Returns whether it
   // was placed (false if that slot was already occupied).
   boolean placeVisibleObject(int kind, Object obj) {
      Dungeon level = this.currentDungeon();
      Object rec = null;
      byte objX = 0;
      byte objY = 0;
      if (kind == 1) {
         rec = (byte[])obj;
         objX = (byte)((Object[])rec)[4];
         objY = (byte)((Object[])rec)[5];
      } else if (kind == 4) {
         rec = (byte[])obj;
         objX = (byte)((Object[])rec)[0];
         objY = (byte)((Object[])rec)[1];
      } else if (kind == 6) {
         if (this.currentLevel == 1) {
            String shopName = (String)obj;

            for (int i = 0; i < 5; i++) {
               if (shopName.equals(Shop.NAMES[i])) {
                  objX = Shop.SHOP_X[i];
                  objY = Shop.SHOP_Y[i];
                  i = 5;
               }
            }
         } else if (this.currentLevel == 3) {
            objX = Shop.SHOP_X[5];
            objY = Shop.SHOP_Y[5];
         } else if (this.currentLevel == 12) {
            objX = Shop.SHOP_X[6];
            objY = Shop.SHOP_Y[6];
         } else if (this.currentLevel == 21) {
            objX = Shop.SHOP_X[7];
            objY = Shop.SHOP_Y[7];
         } else if (this.currentLevel == 30) {
            objX = Shop.SHOP_X[8];
            objY = Shop.SHOP_Y[8];
         }
      } else {
         rec = (byte[])obj;
         objX = (byte)((Object[])rec)[0];
         objY = (byte)((Object[])rec)[1];
      }

      int col = 0;
      int row = 0;
      if (this.facing == 1 || this.facing == 3) {
         byte sign = -1;
         if (this.facing == 1) {
            sign = 1;
         }

         col = sign * (objX - this.tileX) + 3;
         row = sign * (objY - this.tileY) + 3;
      } else if (this.facing == 2 || this.facing == 4) {
         byte sign = -1;
         if (this.facing == 2) {
            sign = 1;
         }

         col = sign * (objY - this.tileY) + 3;
         row = 3 - sign * (objX - this.tileX);
      }

      boolean placed = false;
      if (col == 3 && row == 2) {
         if (visibleObjects.elementAt(1) == EMPTY_SLOT) {
            placed = true;
            visibleObjects.setElementAt(obj, 1);
         }
      } else if (col == 2 && row == 1) {
         if (visibleObjects.elementAt(4) == EMPTY_SLOT) {
            placed = true;
            visibleObjects.setElementAt(obj, 4);
         }
      } else if (col == 3 && row == 1) {
         if (visibleObjects.elementAt(5) == EMPTY_SLOT) {
            placed = true;
            visibleObjects.setElementAt(obj, 5);
         }
      } else if (col == 4 && row == 1) {
         if (visibleObjects.elementAt(6) == EMPTY_SLOT) {
            placed = true;
            visibleObjects.setElementAt(obj, 6);
         }
      } else if (col == 1 && row == 0) {
         if (visibleObjects.elementAt(8) == EMPTY_SLOT) {
            placed = true;
            visibleObjects.setElementAt(obj, 8);
         }
      } else if (col == 2 && row == 0) {
         if (visibleObjects.elementAt(9) == EMPTY_SLOT) {
            placed = true;
            visibleObjects.setElementAt(obj, 9);
         }
      } else if (col == 3 && row == 0) {
         if (visibleObjects.elementAt(10) == EMPTY_SLOT) {
            placed = true;
            visibleObjects.setElementAt(obj, 10);
         }
      } else if (col == 4 && row == 0) {
         if (visibleObjects.elementAt(11) == EMPTY_SLOT) {
            placed = true;
            visibleObjects.setElementAt(obj, 11);
         }
      } else if (col == 5 && row == 0 && visibleObjects.elementAt(12) == EMPTY_SLOT) {
         placed = true;
         visibleObjects.setElementAt(obj, 12);
      }

      return placed;
   }

   private static boolean isOccludedSlot(Object marker) {
      if (marker instanceof Integer) {
         Integer v = (Integer)marker;
         if (v == 1) {
            return true;
         }

         if (v == -1) {
            return true;
         }
      }

      return false;
   }

   // Shared hit-tier roll for attack/castOnMonster/castOnSelf: two
   // independent percentile rolls against `atkChance`/`defChance`.
   // Returns 0=miss, 1=defender's roll wins on a tie-ish double-hit,
   // 2/1=single hit (order broken by which roll is higher), 3=crit.
   static int rollOutcome(int atkChance, int defChance) {
      int atkRoll = Util.randomInt(100);
      int defRoll = Util.randomInt(100);
      boolean defHit = atkRoll <= defChance;
      lastRollCrit = defRoll <= atkChance;
      byte outcome = 0;
      if (lastRollCrit && !defHit) {
         outcome = 3;
      } else if (lastRollCrit && defHit) {
         if (defRoll >= atkRoll) {
            outcome = 2;
         } else {
            outcome = 1;
         }
      } else if (lastRollCrit || defHit) {
         outcome = 0;
      } else if (defRoll >= atkRoll) {
         outcome = 2;
      } else {
         outcome = 1;
      }

      return outcome;
   }

   // Skill rank, optionally plus 1/3 of the governing attribute's bonus
   // component, plus a bard's-tale-ish +4 if a 2-handed weapon is
   // equipped (skill index 11) while a shield skill (index 1) is also
   // trained, minus 1 if Fatigue is critically low, plus 4 if the
   // StarFrost bonus is active.
   int skillValue(int skillIndex, boolean withAttributeBonus) {
      int value = this.skills[skillIndex][0];
      if (withAttributeBonus) {
         int attrSlot = 1 + skillAttributeIndex[skillIndex];
         value += this.attributes[attrSlot] / 3;
      }

      if (skillIndex == 11 && this.isEffectActive(3)) {
         value += this.skills[1][0];
      }

      if (this.coreStats[6] < 7) {
         value--;
      }

      if (this.starFrostBonusActive) {
         value += 4;
      }

      return value;
   }

   int skillBonus(int skillIndex) {
      return this.skills[skillIndex][1];
   }

   // Dispatches to skillValue(5,...) or skillValue(7,...) depending on
   // the equipped offhand/shield item's category.
   int weaponSkillValue(boolean withAttributeBonus) {
      if (this.equippedItems[1] != 0) {
         int category = Item.column(1, this.equippedItems[1]);
         category = Math.abs(category);
         return category == 5 ? this.skillValue(5, withAttributeBonus) : this.skillValue(7, withAttributeBonus);
      } else {
         return 0;
      }
   }

   // Base evasion-ish stat used opposite Monster's detection roll
   // (Monster.tick's `chanceB`); 20 with nothing equipped in the
   // offhand slot.
   int baseEvasion() {
      if (this.equippedItems[1] != 0) {
         int category = Item.column(1, this.equippedItems[1]);
         category = Math.abs(category);
         return category == 5 ? this.skillBonus(5) : this.skillBonus(7);
      } else {
         return 20;
      }
   }

   // Picks whichever of the 4 armor-category skills (0/2/8/12) is
   // currently highest-ranked.
   int bestArmorSkillIndex() {
      byte best = 0;
      int bestValue = this.skillValue(0, false);
      int candidate = this.skillValue(2, false);
      if (candidate > bestValue) {
         bestValue = candidate;
         best = 2;
      }

      candidate = this.skillValue(8, false);
      if (candidate > bestValue) {
         bestValue = candidate;
         best = 8;
      }

      candidate = this.skillValue(12, false);
      if (candidate > bestValue) {
         best = 12;
      }

      return best;
   }

   // Which skill index currently governs offense/defense: a shield buff
   // (effect 6) picks bestArmorSkillIndex; otherwise the equipped
   // weapon's category (ag[0]) maps to skill 0/2/8/12; -1 with nothing
   // equipped.
   int activeWeaponSkillIndex() {
      if (this.isEffectActive(6)) {
         return this.bestArmorSkillIndex();
      }

      if (this.equippedItems[0] != 0) {
         int category = Item.column(1, this.equippedItems[0]);
         category = Math.abs(category);
         if (category == 1) {
            return 0;
         } else if (category == 2) {
            return 2;
         } else {
            return category == 3 ? 8 : 12;
         }
      } else {
         return -1;
      }
   }

   // Attack-power component (paired with attackAccuracy) for
   // rollOutcome: from a buff (effect 14 or the shield buff, effect 6),
   // or the equipped weapon's skill value; plus an effect-1 bonus.
   int attackPower(boolean withAttributeBonus) {
      if (this.isEffectActive(14)) {
         return 5 + this.skillValue(4, false);
      }

      if (this.isEffectActive(6)) {
         int skillIdx = this.bestArmorSkillIndex();
         return this.skillValue(skillIdx, withAttributeBonus);
      }

      int value = 0;
      if (this.equippedItems[0] != 0) {
         int category = Item.column(1, this.equippedItems[0]);
         category = Math.abs(category);
         if (category == 1) {
            value = this.skillValue(0, withAttributeBonus);
         } else if (category == 2) {
            value = this.skillValue(2, withAttributeBonus);
         } else if (category == 3) {
            value = this.skillValue(8, withAttributeBonus);
         } else {
            value = this.skillValue(12, withAttributeBonus);
         }
      } else {
         value = 0;
      }

      if (this.isEffectActive(5)) {
         value += this.skillValue(1, false);
      }

      return value;
   }

   // Attack-accuracy component (paired with attackPower) for
   // rollOutcome, mirroring attackPower but via skillBonus instead of
   // skillValue.
   int attackAccuracy() {
      if (this.isEffectActive(6) || this.isEffectActive(14)) {
         int skillIdx = this.bestArmorSkillIndex();
         return this.skillBonus(skillIdx);
      }

      if (this.equippedItems[0] != 0) {
         int category = Item.column(1, this.equippedItems[0]);
         category = Math.abs(category);
         if (category == 1) {
            return this.skillBonus(0);
         } else if (category == 2) {
            return this.skillBonus(2);
         } else {
            return category == 3 ? this.skillBonus(8) : this.skillBonus(12);
         }
      } else {
         return 20;
      }
   }

   // Weapon damage: from a buff (effect 14 or 6), or the equipped
   // weapon's questFlags column (reused here as a raw damage magnitude,
   // not the 2-bit-per-shop packing Shop.questFlagsFor decodes
   // elsewhere for items in Shop-facing contexts -- apparently a
   // dual-purpose column like Item.subtype's "gift point value" reuse;
   // see CLASS_MAP.md), plus an effect-1 bonus, plus +25 if
   // increaseHarmBuff is active.
   int weaponDamage() {
      int damage = 0;
      if (this.isEffectActive(14)) {
         damage = 5 + this.skillValue(4, false);
      } else if (this.isEffectActive(6)) {
         damage = 20 + this.skillValue(3, false);
      } else if (this.equippedItems[0] != 0) {
         damage = Item.column(3, this.equippedItems[0]);
      } else {
         damage = 0;
      }

      if (this.isEffectActive(1)) {
         damage += 10 + this.skillValue(1, false);
      }

      if (this.increaseHarmBuff) {
         damage += 25;
      }

      return damage;
   }

   // Which skill index governs the offhand/shield item (5 or 7); -1
   // with nothing equipped.
   int offhandSkillIndex() {
      if (this.equippedItems[1] != 0) {
         int category = Item.column(1, this.equippedItems[1]);
         category = Math.abs(category);
         return category == 5 ? 5 : 7;
      } else {
         return -1;
      }
   }

   // Total armor value: weighted sum of each equip slot's questFlags
   // column (see weaponDamage's note on this column's apparent
   // dual-purpose reuse as a raw magnitude here), divided by 10, plus
   // an effect-2 bonus, plus tempArmorBonus (effect 17), plus +15 if
   // increaseArmorBuff is active.
   int armorValue() {
      int total = 0;
      int slotValue = 0;
      if (this.equippedItems[1] != 0) {
         slotValue = Item.column(3, this.equippedItems[1]);
         total += 4 * slotValue;
      }

      if (this.equippedItems[2] != 0) {
         slotValue = Item.column(3, this.equippedItems[2]);
         total += 2 * slotValue;
      }

      if (this.equippedItems[3] != 0) {
         slotValue = Item.column(3, this.equippedItems[3]);
         total += 2 * slotValue;
      }

      if (this.equippedItems[4] != 0) {
         slotValue = Item.column(3, this.equippedItems[4]);
         total += slotValue;
      }

      if (this.equippedItems[5] != 0) {
         slotValue = Item.column(3, this.equippedItems[5]);
         total += slotValue;
      }

      total /= 10;
      if (this.isEffectActive(2)) {
         total += 10 + this.skillValue(1, false);
      }

      if (this.isEffectActive(17)) {
         total += this.tempArmorBonus;
      }

      if (this.increaseArmorBuff) {
         total += 15;
      }

      return total;
   }

   // Auto-picks-up a chest-opened item, or drops it back to the floor
   // if the inventory is full (also awarding giftPointsFound for
   // category-11 "gift" items either way). Returns 1 if picked up, 0 if
   // dropped.
   int pickUpDroppedItem(byte[] rec) {
      rec[2] = 2;
      if (this.hasInventorySpace()) {
         byte itemId = rec[4];
         int packed = (rec[5] << 8) + rec[6];
         byte charge = rec[7];
         this.addInventoryItem(itemId, packed, charge);
         ESGame.dungeons[this.currentLevel - 1].removeChest(rec);
         int itemIdx = itemId - 1;
         if (Item.category[itemIdx] == 11) {
            this.giftPointsFound = (short)(this.giftPointsFound + Item.subtype[itemIdx]);
         }

         return 1;
      } else {
         byte[] floorRec = new byte[]{rec[0], rec[1], rec[4], rec[5], rec[6], rec[7], 1};
         this.currentDungeon().addDroppedItem(floorRec);
         ESGame.dungeons[this.currentLevel - 1].removeChest(rec);
         return 0;
      }
   }

   // effectDurations[-1]==-1: active until cured; ==-2: active while
   // combatTargetSpawnId != 0 (i.e. while in combat); >0: still
   // counting down.
   private boolean isEffectActive(int effectId) {
      if (this.effectDurations[effectId - 1] == -1) {
         return true;
      } else {
         return this.effectDurations[effectId - 1] == -2 ? this.combatTargetSpawnId != 0 : this.effectDurations[effectId - 1] > 0;
      }
   }

   void clearEffect(int effectId) {
      this.effectDurations[effectId - 1] = 0;
   }

   // Nearest attackable monster in front, or null.
   Monster nearestAttackableMonster() {
      this.computeMoveTarget(1);
      if (this.pendingLevel <= 0) {
         return null;
      } else {
         Object rec = ESGame.monsters[this.pendingLevel - 1].get(Util.posKey((int)this.pendingTileX, (int)this.pendingTileY));
         if (rec != null) {
            Monster m = new Monster();
            Monster.fromBytes(m, (byte[])rec);
            return m;
         } else {
            return null;
         }
      }
   }

   // Chest directly in front, or null.
   byte[] chestInFront() {
      this.computeMoveTarget(1);
      if (this.pendingLevel <= 0) {
         return null;
      }

      byte level = this.pendingLevel;
      Hashtable chests = ESGame.chests[level - 1];
      if (chests == null) {
         return null;
      }

      Object rec = chests.get(Util.posKey((int)this.pendingTileX, (int)this.pendingTileY));
      return rec == null ? null : (byte[])rec;
   }

   // NPC shop id directly in front (5-8 for the named shopkeepers inside
   // levels 3/12/21/30, or Shop.hubShopAt's lookup in the hub town), or
   // -1.
   int npcInFront() {
      this.computeMoveTarget(1);
      if (this.pendingLevel <= 0) {
         return -1;
      } else {
         byte level = this.pendingLevel;
         if (level == 3 && this.pendingTileX == Shop.SHOP_X[5] && this.pendingTileY == Shop.SHOP_Y[5]) {
            return 5;
         } else if (level == 12 && this.pendingTileX == Shop.SHOP_X[6] && this.pendingTileY == Shop.SHOP_Y[6]) {
            return 6;
         } else if (level == 21 && this.pendingTileX == Shop.SHOP_X[7] && this.pendingTileY == Shop.SHOP_Y[7]) {
            return 7;
         } else if (level == 30 && this.pendingTileX == Shop.SHOP_X[8] && this.pendingTileY == Shop.SHOP_Y[8]) {
            return 8;
         } else {
            return level != 1 ? -1 : Shop.hubShopAt(this.pendingTileX, this.pendingTileY);
         }
      }
   }

   // Maps a spell id to a governing skill index (id 1-5->1, 6-10->3,
   // 11-15->4, 16-20->6, 41/42->10) -- a hardcoded parallel to
   // Spell.skillRequired rather than reading it directly.
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

   // Casts `spellId` on self: rolls rollOutcome(magicka-skill,
   // spell-power-skill), spends Magicka scaled by the outcome, then
   // applies the spell's effect by id (buffs via effectDurations,
   // direct Health/Magicka restores, status cures, a "cure random
   // ailment" (case 6, item id 101 check), or a level-exp grant).
   void castOnSelf(int spellId) {
      int skillIdx = this.spellSkillIndexFor(spellId);
      int atkSkill = this.skillValue(skillIdx, true);
      int atkBonus = this.skillBonus(skillIdx);
      byte cost = Spell.byId(spellId).icon;
      byte durationMult = Spell.byId(spellId).durationMultiplier;
      byte power = Spell.byId(spellId).magickaCost;
      byte school = Spell.byId(spellId).power;
      int diff = atkSkill - cost;
      int atkChance = atkBonus + diff * 5;
      int defChance = durationMult - diff * 5;
      atkChance = Math.min(Math.max(atkChance, 10), 95);
      defChance = Math.min(Math.max(defChance, 10), 95);
      int outcome = rollOutcome(atkChance, defChance);
      byte multiplier = 1;
      if (outcome == 0) {
         this.coreStats[4] = (short)(this.coreStats[4] - 3 * school);
      } else if (outcome == 1) {
         this.coreStats[4] = (short)(this.coreStats[4] - 3 * school / 2);
      } else if (outcome == 2) {
         this.coreStats[4] = (short)(this.coreStats[4] - school);
      } else if (outcome == 3) {
         this.coreStats[4] = (short)(this.coreStats[4] - school);
         multiplier = 2;
      }

      this.coreStats[4] = (short)Math.max(this.coreStats[4], 0);
      if (outcome >= 2) {
         this.gainSkillExp(skillIdx, 1);
      }

      switch (spellId) {
         case 1:
         case 2:
         case 3:
         case 5:
            this.effectDurations[spellId - 1] = (byte)(durationMult * multiplier);
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
            byte unused = 0;
            if (this.addInventoryItem(101, unused, 0) && this.equipLastPickedUpItem(true)) {
               this.effectDurations[spellId - 1] = (byte)(durationMult * multiplier);
            }
            break;
         case 21:
            int healAmount = 6 + this.skillValue(10, false);
            this.coreStats[2] = (short)(this.coreStats[2] + multiplier * healAmount);
            this.coreStats[2] = (short)Math.min(this.coreStats[2], this.coreStats[3]);
            break;
         case 23:
            this.effectDurations[spellId - 1] = -2;
            break;
         case 24:
            this.effectDurations[spellId - 1] = -4;
            break;
         case 25:
            for (int i = 1; i <= multiplier; i++) {
               this.cureRandomAilment();
            }
      }

      this.coreStats[6] = (short)(this.coreStats[6] - 5 * this.fatigueCostMultiplier());
      this.coreStats[6] = (short)Math.max(this.coreStats[6], 0);
      if (this.hasAilment(6)) {
         int drain = 2 * this.coreStats[3] / 100;
         if (drain < 1) {
            drain = 1;
         }

         this.coreStats[2] = (short)(this.coreStats[2] - (short)drain);
      }
   }

   // Casts `spellId` on `target` (an offensive spell): same skill roll
   // shape as castOnSelf but against the target's own evasion, then a
   // big switch on spell id covering direct damage/status effects on
   // the target (fear/knockback/stun-ish flags in target.scratch[],
   // duration debuffs, or a self-buff like effect 17's tempArmorBonus).
   void castOnMonster(int spellId, Monster target) {
      int skillIdx = this.spellSkillIndexFor(spellId);
      int atkSkill = this.skillValue(skillIdx, true);
      int atkBonus = this.skillBonus(skillIdx);
      int targetEvasion = target.stat(10);
      int targetDef = target.stat(9);
      byte power = Spell.byId(spellId).magickaCost;
      byte school = Spell.byId(spellId).power;
      int diff = atkSkill - targetEvasion;
      int targetHp = target.stat(2);
      diff = Math.min(diff, targetHp);
      int atkChance = atkBonus + diff * 5;
      int defChance = targetDef - diff * 5;
      atkChance = Math.min(Math.max(atkChance, 10), 95);
      defChance = Math.min(Math.max(defChance, 10), 95);
      int outcome = rollOutcome(atkChance, defChance);
      byte multiplier = 1;
      if (outcome == 0) {
         this.coreStats[4] = (short)(this.coreStats[4] - 3 * power);
      } else if (outcome == 1) {
         this.coreStats[4] = (short)(this.coreStats[4] - 3 * power / 2);
      } else if (outcome == 2) {
         this.coreStats[4] = (short)(this.coreStats[4] - power);
      } else if (outcome == 3) {
         this.coreStats[4] = (short)(this.coreStats[4] - power);
         multiplier = 2;
      }

      this.coreStats[4] = (short)Math.max(this.coreStats[4], 0);
      if (outcome >= 2) {
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
            int val7 = 10 + this.skillValue(3, false);
            target.scratch[1] = (byte)val7;
            break;
         case 8:
            int val8 = this.skillValue(3, false);
            int dmg8 = 12 + 2 * val8;
            target.takeDamage(dmg8);
            this.coreStats[6] = (short)(this.coreStats[6] + val8);
            this.coreStats[6] = (short)Math.min(this.coreStats[6], this.coreStats[7]);
            this.coreStats[2] = (short)(this.coreStats[2] + val8);
            this.coreStats[2] = (short)Math.min(this.coreStats[2], this.coreStats[3]);
            this.coreStats[4] = (short)(this.coreStats[4] + 12);
            this.coreStats[4] = (short)Math.min(this.coreStats[4], this.coreStats[5]);
            break;
         case 9:
            if (target.isUndead()) {
               int base9 = 60 * multiplier;
               int rawDmg9 = base9 - target.stat(8);
               rawDmg9 = Math.max(rawDmg9, 4);
               int scaled9 = rawDmg9 * target.stat(14) / 100;
               target.takeDamage(scaled9);
            }
            break;
         case 10:
            this.effectDurations[spellId - 1] = -2;
            target.scratch[8] = (byte)(2 * multiplier);
            break;
         case 11:
            int base11 = 25 + this.skillValue(4, false);
            int raw11 = base11 * multiplier;
            int dmg11 = raw11 - target.stat(8);
            dmg11 = Math.max(dmg11, 4);
            int scaled11 = dmg11 * target.stat(14) / 100;
            target.takeDamage(scaled11);
            target.store();
            break;
         case 12:
            this.effectDurations[spellId - 1] = -2;
            int val12 = multiplier * (10 + this.skillValue(4, false));
            val12 = Math.min(val12, 255);
            target.scratch[4] = (byte)val12;
            target.store();
            break;
         case 13:
            this.effectDurations[spellId - 1] = -2;
            int val13 = multiplier * (10 + this.skillValue(4, false));
            val13 = Math.min(val13, 255);
            target.scratch[5] = (byte)val13;
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
            int val16 = 10 - targetEvasion;
            if (val16 > 0) {
               val16 = multiplier * val16;
               this.effectDurations[spellId - 1] = (byte)val16;
               target.scratch[6] = 1;
            }
            break;
         case 17:
            this.effectDurations[spellId - 1] = -2;
            this.tempArmorBonus = (short)(10 + this.skillValue(6, false));
            break;
         case 18:
            this.effectDurations[spellId - 1] = -2;
            target.scratch[0] = (byte)(3 * multiplier);
            break;
         case 19:
            this.effectDurations[spellId - 1] = -2;
            int val19 = multiplier * (60 - 5 * targetEvasion);
            val19 = Math.min(Math.max(val19, 0), 100);
            target.scratch[3] = (byte)val19;
            break;
         case 20:
            int base20 = 80 - 5 * targetEvasion;
            int raw20 = base20 * multiplier;
            int dmg20 = raw20 - target.stat(8);
            dmg20 = Math.max(dmg20, 4);
            int scaled20 = dmg20 * target.stat(14) / 100;
            target.takeDamage(scaled20);
      }

      this.coreStats[6] = (short)(this.coreStats[6] - 5 * this.fatigueCostMultiplier());
      this.coreStats[6] = (short)Math.max(this.coreStats[6], 0);
      if (this.hasAilment(6)) {
         int drain = 2 * this.coreStats[3] / 100;
         if (drain < 1) {
            drain = 1;
         }

         this.coreStats[2] = (short)(this.coreStats[2] - (short)drain);
      }
   }

   // Awards `amount` exp toward skillIndex's next rank; each full rank-
   // up flags that skill's governing attribute as eligible for a +1
   // (attributeIncreaseFlags) and, once total level-exp reaches 10,
   // triggers a level-up (levelUpPending).
   void gainSkillExp(int skillIndex, int amount) {
      if (skillIndex >= 0 && skillIndex < 14) {
         for (this.skills[skillIndex][2] = (short)(this.skills[skillIndex][2] + amount); this.skills[skillIndex][2] > 10; this.coreStats[1]++) {
            this.skills[skillIndex][2] = (short)(this.skills[skillIndex][2] - 10);
            this.skills[skillIndex][0]++;
            short attrIdx = skillAttributeIndex[skillIndex];
            int bit = attrIdx / 2;
            this.attributeIncreaseFlags = (byte)(this.attributeIncreaseFlags | 1 << bit);
         }

         if (this.coreStats[1] >= 10) {
            this.coreStats[0]++;
            this.levelUpPending = true;
         }
      }
   }

   // Unpacks a dropped-item record and tries addInventoryItem.
   boolean tryAddDroppedItem(byte[] rec) {
      byte itemId = rec[2];
      int packed = (rec[3] << 8) + rec[4];
      byte charge = rec[5];
      return this.addInventoryItem(itemId, packed, charge);
   }

   // Grants the hardcoded item id 100 ("StarFrost"), evicting the
   // lowest-priority non-equipped inventory item (by Item's sell-price
   // column) to make room if the inventory is full.
   void grantStarFrostItem() {
      this.starFrostBonusActive = true;
      short spawnId = Item.nextSpawnId();
      boolean added = this.addInventoryItem(100, spawnId, 0);
      if (!added) {
         int lowestValue = 10000;
         int evictSlot = -1;
         int starFrostSlot = -1;

         for (int slot = 0; slot < this.inventoryCount; slot++) {
            int itemId = Math.abs(this.inventoryItemIds[slot]);
            if (itemId == 87) {
               starFrostSlot = slot;
               break;
            }

            int value = Item.column(5, itemId);
            if (!this.isEquipped(slot) && value < lowestValue && value > 0) {
               evictSlot = slot;
               lowestValue = value;
            }
         }

         if (starFrostSlot > -1) {
            this.removeInventorySlot(starFrostSlot);
         } else {
            this.removeInventorySlot(evictSlot);
         }

         added = this.addInventoryItem(100, spawnId, 0);
         if (!added) {
            System.out.println("Still can't add StarFrost");
         }
      }
   }

   // Appends an item to the first free inventory slot; false if full.
   boolean addInventoryItem(int itemId, int spawnIdOrPacked, int charge) {
      if (this.inventoryCount < 24) {
         this.inventoryItemIds[this.inventoryCount] = (byte)itemId;
         int packed = (spawnIdOrPacked << 16) + (byte)charge;
         this.inventoryItemData[this.inventoryCount] = packed;
         this.inventoryCount++;
         return true;
      } else {
         return false;
      }
   }

   // Unequips (if equipped) and removes inventory slot, compacting the
   // remaining slots down.
   boolean removeInventorySlot(int slot) {
      if (slot >= this.inventoryCount) {
         return false;
      }

      this.unequipInventorySlot(slot);
      this.inventoryItemIds[slot] = 0;

      for (int i = slot; i < this.inventoryCount - 1; i++) {
         this.inventoryItemIds[i] = this.inventoryItemIds[i + 1];
         this.inventoryItemData[i] = this.inventoryItemData[i + 1];
      }

      this.inventoryCount--;
      return true;
   }

   // Drops the item in `slot` onto the current tile (unless it's the
   // non-droppable StarFrost, id 101 -- note: elsewhere in this class
   // StarFrost is granted as id 100; this special-cased 101 looks like
   // either a second related item or a transcription quirk in the
   // original, preserved as found) and removes it from the inventory.
   void dropInventoryItem(int slot) {
      int itemId = Math.abs(this.inventoryItemIds[slot]);
      if (itemId != 101) {
         byte[] rec = new byte[]{this.tileX, this.tileY, (byte)itemId, 0, 0, (byte)(this.inventoryItemData[slot] & 0xFF), 0};
         int packed = this.inventoryItemData[slot] >>> 16 & 65535;
         rec[3] = (byte)(packed >> 8 & 0xFF);
         rec[4] = (byte)(packed & 0xFF);
         rec[6] = 3;
         this.currentDungeon().addDroppedItem(rec);
         this.removeInventorySlot(slot);
      } else {
         this.removeInventorySlot(slot);
      }
   }

   boolean isEquipped(int slot) {
      byte itemId = this.inventoryItemIds[slot];
      return !Item.isEquippable(Math.abs(itemId)) ? false : itemId < 0;
   }

   // Gates ESGame's "Equip"/"Unequip" inventory-item menu option: true
   // for weapon (1-4), armor (5-10), and "special weapon" (15)
   // categories. Kept as its own category switch (rather than reusing
   // Item.isEquippable's equipSlot-based check) to stay faithful to the
   // original.
   boolean canEquipOrUnequip(int slot) {
      int itemId = Math.abs(this.inventoryItemIds[slot]);
      switch (Item.column(1, itemId)) {
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
         case 15:
            return true;
         default:
            return false;
      }
   }

   // Equips the item in `slot`; if that equip slot is already occupied,
   // either fails (autoUnequipConflict=false) or unequips the conflict
   // first.
   boolean equipItem(int slot, boolean autoUnequipConflict) {
      byte itemId = this.inventoryItemIds[slot];
      if (itemId < 0) {
         return false;
      }

      if (!Item.isEquippable(itemId)) {
         return false;
      }

      int equipSlot = Item.equipSlotOf(itemId);
      if (this.equippedItems[equipSlot] != 0) {
         if (!autoUnequipConflict) {
            return false;
         }

         this.unequipItemInSlot(equipSlot);
      }

      this.equippedItems[equipSlot] = itemId;
      this.inventoryItemIds[slot] = (byte)(-Math.abs(this.inventoryItemIds[slot]));
      return true;
   }

   // Finds whichever inventory slot currently occupies `equipSlot` and
   // unequips it.
   private void unequipItemInSlot(int equipSlot) {
      for (int slot = 0; slot < this.inventoryCount; slot++) {
         byte itemId = this.inventoryItemIds[slot];
         itemId = (byte)Math.abs(itemId);
         int slotOf = Item.equipSlotOf(itemId);
         if (slotOf == equipSlot) {
            this.unequipInventorySlot(slot);
         }
      }
   }

   void addGold(int amount) {
      this.gold += amount;
   }

   // Equips the most-recently-added inventory item.
   boolean equipLastPickedUpItem(boolean autoUnequipConflict) {
      int slot = this.inventoryCount - 1;
      return this.equipItem(slot, autoUnequipConflict);
   }

   // Unequips whatever's in `slot` (if it is equipped), flipping its
   // inventoryItemIds sign back positive and clearing the matching
   // equippedItems entry.
   void unequipInventorySlot(int slot) {
      if (this.isEquipped(slot)) {
         if (slot >= 0 && slot <= 23) {
            byte itemId = this.inventoryItemIds[slot];
            itemId = (byte)Math.abs(itemId);
            this.inventoryItemIds[slot] = itemId;

            for (int i = 0; i < 7; i++) {
               if (this.equippedItems[i] == itemId) {
                  this.equippedItems[i] = 0;
                  break;
               }
            }
         }
      }
   }

   // Finds the inventory slot currently holding `itemId` equipped
   // (negative-encoded), or -1.
   int findInventorySlotOf(int itemId) {
      int found = -1;
      int negId = -Math.abs(itemId);

      for (int slot = 0; slot < this.inventoryCount; slot++) {
         if (negId == this.inventoryItemIds[slot]) {
            found = slot;
            break;
         }
      }

      return found;
   }

   boolean hasInventorySpace() {
      return this.inventoryCount < 24;
   }

   // Item-info tooltip for `slot`: name + category-specific detail line
   // (weapon/armor value, spell name + known status, gift flavor text,
   // or the plain description for anything else).
   String itemTooltip(int slot) {
      int itemId = Math.abs(this.inventoryItemIds[slot]);
      byte category = Item.category[itemId - 1];
      String text = null;
      switch (category) {
         case 1:
         case 2:
         case 3:
         case 4:
            text = Item.name[itemId - 1] + '\n' + Item.categoryNames[category - 1];
            int weaponValue = Item.questFlags[itemId - 1] + (this.inventoryItemData[slot] & 0xFF);
            text = text + "\nWeapon value: " + weaponValue;
            break;
         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
            text = Item.name[itemId - 1] + '\n' + Item.categoryNames[category - 1];
            int armorValue = Item.questFlags[itemId - 1] + (this.inventoryItemData[slot] & 0xFF);
            text = text + "\nArmor value: " + armorValue;
            break;
         case 11:
            text = Item.name[itemId - 1] + '\n' + Item.categoryNames[category - 1];
            break;
         case 12:
            text = Item.name[itemId - 1] + '\n' + "Spell: ";
            int spellId = this.inventoryItemData[slot] & 0xFF;
            text = text + Spell.all[spellId - 1].name;
            if ((this.knownSpellsMask & 1 << spellId - 1) != 0) {
               text = text + " (known)";
            }
            break;
         case 13:
            int giftIdx = itemId - 87;
            text = Item.name[itemId - 1] + '\n' + Item.categoryNames[category - 1] + '\n' + Item.specialEffectText[giftIdx];
            break;
         case 14:
         default:
            text = Item.name[itemId - 1] + '\n' + Item.categoryNames[category - 1];
            break;
         case 15:
            text = Item.name[itemId - 1] + '\n' + Item.categoryNames[category - 1];
            int bonus = this.skillValue(3, false);
            int value15 = 20 + bonus;
            text = text + "\nWeapon value: " + value15;
      }

      return text;
   }

   // Gates ESGame's "Learn" inventory-item menu option: true for a
   // not-already-known spell scroll (category 12) whose required skill
   // the player has at least 1 point in.
   boolean canLearnSpell(int slot) {
      int itemId = Math.abs(this.inventoryItemIds[slot]);
      if (Item.column(1, itemId) != 12) {
         return false;
      }

      int spellId = this.inventoryItemData[slot] & 0xFF;
      byte requiredSkill = Spell.all[spellId - 1].skillRequired;
      if ((this.knownSpellsMask & 1 << spellId - 1) != 0) {
         return false;
      } else {
         return this.skills[requiredSkill][0] > 0;
      }
   }

   // Learns the spell encoded on a scroll-type item (category 12) in
   // `slot`, then consumes the scroll.
   boolean learnSpellFromScroll(int slot) {
      int itemId = Math.abs(this.inventoryItemIds[slot]);
      byte category = Item.category[itemId - 1];
      int spellData = this.inventoryItemData[slot] & 0xFF;
      int spellBit = spellData - 1;
      this.knownSpellsMask = Util.setBit(spellBit, this.knownSpellsMask);
      this.removeInventorySlot(slot);
      return true;
   }

   // Gates ESGame's "Use" inventory-item menu option: true only for
   // the 87-99 "gift"/special-consumable category (13).
   boolean canUseItem(int slot) {
      int itemId = Math.abs(this.inventoryItemIds[slot]);
      return Item.column(1, itemId) == 13;
   }

   // 1-arg overload called from ESGame's inventory screen (no monster
   // target in that context): delegates to the 2-arg overload with
   // GameCanvas's current combat target. Note this reaches across to
   // GameCanvas's own static `targetMonster` field, already the real
   // Monster type there.
   void useItem(int slot) {
      this.useItem(slot, GameCanvas.targetMonster);
   }

   // Uses the 13 "gift"/special consumable items (ids 87-99) by exact
   // id: 87=warp to camp, 88=cure random ailment, 89=full heal HP,
   // 90=full heal Magicka, 91=Fatigue+3xMagicka, 92=+1 level-exp,
   // 93=full HP+Magicka, 94=Increase Harm buff, 95=Increase Armor buff,
   // 96=Safe Camping flag, 97/98/99=instant-kill scrolls gated on
   // `target`'s difficulty stat (<=13/22/29).
   void useItem(int slot, Monster target) {
      int itemId = Math.abs(this.inventoryItemIds[slot]);
      byte category = Item.category[itemId - 1];
      if (category == 13) {
         boolean consume = true;
         switch (itemId) {
            case 87:
               if (this.currentLevel == 1 && this.hasCampMark()) {
                  this.warpToCampMark();
                  break;
               }

               this.markCampAndReturnToTown(false);
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
                  int def = target.stat(4);
                  int evasion = target.stat(10);
                  if (def <= 13 && evasion <= 13) {
                     target.hp = 0;
                     target.store();
                  }
               }
               break;
            case 98:
               if (target != null) {
                  int def = target.stat(4);
                  int evasion = target.stat(10);
                  if (def <= 22 && evasion <= 22) {
                     target.hp = 0;
                     target.store();
                  }
               }
               break;
            case 99:
               if (target != null) {
                  int def = target.stat(4);
                  int evasion = target.stat(10);
                  if (def <= 29 && evasion <= 29) {
                     target.hp = 0;
                     target.store();
                  }
               }
         }

         if (consume) {
            this.removeInventorySlot(slot);
         }
      }
   }

   // Applies a pending level-up: clears attributeIncreaseFlags, resets
   // Shop state (e.g. shop restocking), and consumes 10 level-exp.
   void levelUp() {
      this.attributeIncreaseFlags = 0;
      Shop.reset();
      this.coreStats[1] = (short)(this.coreStats[1] - 10);
   }

   // Attribute names currently eligible for a +1 increase (from
   // attributeIncreaseFlags), or null if none.
   String[] availableAttributeIncreases() {
      Vector out = new Vector();

      for (int i = 0; i < 8; i++) {
         if ((this.attributeIncreaseFlags & 1 << i) != 0) {
            int slot = i * 2;
            out.addElement(attributeNames[slot]);
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

   // Grants raceIndex's starting item pair and auto-equips each one.
   private void grantStartingItems() {
      short spawnId = Item.nextSpawnId();
      int[] items = STARTING_ITEMS[this.raceIndex];

      for (int i = 0; i < items.length; i++) {
         this.addInventoryItem(items[i], spawnId, 0);
         int slot = this.inventoryCount - 1;
         this.equipItem(slot, true);
      }
   }

   // Passive Fatigue regen over `elapsedMs`, scaled by the average of
   // two attributes (indices 10/11 -- Endurance-ish), capped at max.
   void tickFatigueRegen(long elapsedMs) {
      int gain = (int)(elapsedMs * (this.attributes[10] + this.attributes[11]) / 2000L);
      this.coreStats[6] = (short)(this.coreStats[6] + gain);
      if (this.coreStats[6] > this.coreStats[7]) {
         this.coreStats[6] = this.coreStats[7];
      }
   }

   // The named shopkeepers' (5-8) quest-turn-in outcome roll: skill 13
   // ("Speechcraft"?) vs. that shop's questState1, +3 bonus if
   // action==3 (a specific dialogue choice).
   int rollShopOutcome(int shopId, int action) {
      int skill = this.skillValue(13, true);
      if (action == 3) {
         skill += 3;
      }

      short threshold = Shop.questState1[shopId - 5];
      int diff = skill - threshold;
      int defChance = 20 - diff * 5;
      int atkChance = 20 + this.attributes[12] / 2 + diff * 5;
      defChance = Math.min(Math.max(defChance, 10), 95);
      atkChance = Math.min(Math.max(atkChance, 10), 95);
      return rollOutcome(atkChance, defChance);
   }

   // Bookmarks the current position as the camp/warp point and returns
   // to the hub town. Always called with skipMark=false in this build
   // (the skipMark=true path is never exercised) -- from walking onto a
   // camp-marker tile (tile bit 8, see commitMove) or from using the
   // "Warp to Camp" item while not already in town.
   void markCampAndReturnToTown(boolean skipMark) {
      if (!skipMark) {
         this.campLevel = this.currentLevel;
         this.campX = this.tileX;
         this.campY = this.tileY;
         this.campFacing = this.facing;
      }

      this.resetToHubPosition(true);
      this.suppressStrafeAdjust = true;
   }

   // Warps to the bookmarked camp/warp point (see markCampAndReturnToTown).
   void warpToCampMark() {
      this.currentLevel = this.pendingLevel = this.campLevel;
      this.tileX = this.pendingTileX = this.campX;
      this.tileY = this.pendingTileY = this.campY;
      this.facing = this.pendingFacing = this.campFacing;
      this.refreshCorridorView();
      this.suppressStrafeAdjust = true;
      this.game.gameCanvas.refreshChestInSight();
      this.game.gameCanvas.refreshNpcInSight();
   }

   boolean hasCampMark() {
      return this.campLevel > 0;
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

   // Clears a random currently-active ailment bit.
   void cureRandomAilment() {
      int active = this.activeAilmentCount();
      if (active > 0) {
         int pick = 0;
         if (active == 1) {
            pick = 1;
         } else {
            pick = Util.randomInt(active);
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

   // Fatigue-cost multiplier for actions: 3x while "Frost Limbs"
   // (ailment bit 0) is active, else 1x.
   int fatigueCostMultiplier() {
      return (this.ailmentMask & 1) == 1 ? 3 : 1;
   }

   Dungeon currentDungeon() {
      return ESGame.dungeons[this.currentLevel - 1];
   }

   // Repopulates corridorView from the current position/facing.
   void refreshCorridorView() {
      this.currentDungeon().sampleCorridorView(this.tileX, this.tileY, this.facing, this.corridorView);
   }

   // Full character-sheet string: name/race/level/HP/Magicka/Fatigue,
   // active ailments, gift points found, and attributes. Used by
   // ESGame's "Stats" popup.
   String buildCharacterSheet() {
      StringBuffer out = new StringBuffer(900);
      String sp = " ";
      String colon = ": ";
      out.append(this.name);
      out.append('\n');
      out.append(raceNames[this.raceIndex]);
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
      out.append("Magicka: ");
      out.append(this.effectiveStat(4));
      out.append('/');
      out.append(this.coreStats[5]);
      out.append('\n');
      out.append("Fatigue: ");
      out.append(this.effectiveStat(6));
      out.append('/');
      out.append(this.coreStats[7]);
      out.append('\n');
      out.append("  ");
      out.append('\n');
      out.append("Status ailments: ");
      int count = 0;

      for (int i = 1; i <= 8; i++) {
         if (this.hasAilment(i)) {
            out.append('\n');
            out.append(AILMENT_NAMES[i - 1]);
            count++;
         }
      }

      if (count == 0) {
         out.append('\n');
         out.append("None");
      }

      out.append('\n');
      out.append("  ");
      out.append('\n');
      out.append("Gift points found: ");
      out.append(this.giftPointsFound);
      out.append('\n');
      out.append("  ");
      out.append('\n');
      out.append("Attributes:");
      out.append('\n');

      for (int i = 0; i < 8; i++) {
         int slot = 2 * i;
         out.append(attributeNames[slot]);
         out.append(colon);
         out.append(this.attributes[slot]);
         out.append('\n');
      }

      return out.toString();
   }

   static Vector newVisibleObjectsVector() {
      Vector v = new Vector();

      for (int i = 0; i < 13; i++) {
         v.addElement(new Object());
      }

      return v;
   }

   // "Skill: rank" strings for every skill with rank > 0.
   String[] knownSkillsSummary() {
      Vector out = new Vector();

      for (int i = 0; i < 14; i++) {
         if (this.skills[i][0] > 0) {
            String line = skillNames[i] + ": " + this.skills[i][0];
            out.addElement(line);
         }
      }

      int count = out.size();
      String[] result = new String[count];

      for (int i = 0; i < count; i++) {
         result[i] = (String)out.elementAt(i);
      }

      return result;
   }

   // The `index`-th skill with rank > 0 (matching knownSkillsSummary's
   // ordering), or -1.
   int nthKnownSkillIndex(int index) {
      int seen = 0;

      for (int i = 0; i < 14; i++) {
         if (this.skills[i][0] > 0) {
            if (seen == index) {
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

   // "Spell name" strings for every known spell, prefixing the
   // currently-selected one with "R: ".
   Vector knownSpellsSummary() {
      Vector out = new Vector();

      for (int i = 0; i < Spell.count; i++) {
         if ((this.knownSpellsMask & 1 << i) != 0) {
            int spellId = i + 1;
            String line = Spell.all[i].name;
            if (spellId == this.selectedSpellId) {
               line = "R: " + line;
            }

            out.addElement(line);
         }
      }

      return out;
   }

   // The `index`-th known spell's id (matching knownSpellsSummary's
   // ordering), or -1.
   int nthKnownSpellId(int index) {
      int seen = 0;

      for (int i = 0; i < Spell.count; i++) {
         if ((this.knownSpellsMask & 1 << i) != 0) {
            int spellId = i + 1;
            if (seen == index) {
               return i;
            }

            seen++;
         }
      }

      return -1;
   }

   // Cycles selectedSpellId to the next known spell after it (wrapping
   // around), or the first known spell if the current selection isn't
   // valid; returns 0 if no spells are known.
   int cycleSelectedSpell() {
      if (!Spell.isValidId(this.selectedSpellId)) {
         int first = this.nthKnownSpellId(0);
         return first < 0 ? 0 : first + 1;
      }

      int start = this.selectedSpellId - 1;
      int i = start + 1;
      if (i == Spell.count) {
         i = 0;
      }

      while (i != start) {
         if ((this.knownSpellsMask & 1 << i) != 0) {
            return i + 1;
         }

         if (++i == Spell.count) {
            i = 0;
         }
      }

      return this.selectedSpellId;
   }

   String spellTooltip(int spellId) {
      int i = spellId;
      String text = Spell.all[i].name + '\n';
      text = text + skillNames[Spell.all[i].skillRequired] + '\n';
      text = text + "Cost: " + Spell.all[i].magickaCost + '\n';
      return text + Spell.all[i].description;
   }

   // Applies rest: HP/Magicka/Fatigue regen (2/3 if disturbed, i.e.
   // `!fullRest`; 3/4 of that again if ailment 8 "Winter Worn" active),
   // clears the buff flags, a 10% chance to consume the Safe Camping
   // item if held, and (for every ailment but 4/5, which have their own
   // timers) a 25% chance per ailment to cure it. Also runs the
   // duplicated "remove roaming gehen" cleanup.
   void rest(boolean fullRest) {
      if (this.roamingSpecialMonsterPresent) {
         Hashtable levelMonsters = ESGame.monsters[this.currentLevel - 1];
         if (levelMonsters != null) {
            Monster scratch = new Monster();
            Enumeration monsters = levelMonsters.elements();

            while (monsters.hasMoreElements()) {
               byte[] rec = (byte[])monsters.nextElement();
               Monster.fromBytes(scratch, rec);
               if (scratch.monsterType == 41) {
                  this.roamingSpecialMonsterPresent = false;
                  ESGame.removeMonster(this.currentLevel, scratch.x, scratch.y);
                  break;
               }
            }
         }

         if (this.roamingSpecialMonsterPresent) {
            System.out.println("Remove roaming gehen failed");
         }
      }

      short hpGain = (short)(this.coreStats[3] - this.coreStats[2]);
      short magickaGain = (short)(this.coreStats[5] - this.coreStats[4]);
      short fatigueGain = (short)(this.coreStats[7] - this.coreStats[6]);
      if (!fullRest) {
         hpGain = (short)(2 * hpGain / 3);
         magickaGain = (short)(2 * magickaGain / 3);
         fatigueGain = (short)(2 * fatigueGain / 3);
      }

      this.coreStats[9] = 0;
      this.coreStats[8] = 0;
      if (this.hasAilment(8)) {
         hpGain = (short)(3 * hpGain / 4);
         magickaGain = (short)(3 * magickaGain / 4);
         fatigueGain = (short)(3 * fatigueGain / 4);
      }

      this.coreStats[2] = (short)(this.coreStats[2] + hpGain);
      this.coreStats[4] = (short)(this.coreStats[4] + magickaGain);
      this.coreStats[6] = (short)(this.coreStats[6] + fatigueGain);
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

      for (int ailment = 0; ailment < 8; ailment++) {
         int num = ailment + 1;
         if (num != 4 && num != 5) {
            roll = Util.randomInt(100);
            if (roll <= 25) {
               this.ailmentMask = (byte)Util.clearBit(ailment, this.ailmentMask);
            }
         }
      }
   }

   boolean hasAilment(int ailmentNumber) {
      int bit = ailmentNumber - 1;
      return (this.ailmentMask & 1 << bit) != 0;
   }

   // Current value of a coreStats slot (2/4/6 = curHP/Magicka/Fatigue),
   // boosted by skillValue(10) while effect 23 is active and capped at
   // the matching max.
   int effectiveStat(int index) {
      int value = this.coreStats[index];
      if (this.isEffectActive(23)) {
         if (index == 2) {
            value += this.skillValue(10, false);
            if (value > this.coreStats[3]) {
               value = this.coreStats[3];
            }
         } else if (index == 6) {
            value += this.skillValue(10, false);
            if (value > this.coreStats[7]) {
               value = this.coreStats[7];
            }
         } else if (index == 4) {
            value += this.skillValue(10, false);
            if (value > this.coreStats[5]) {
               value = this.coreStats[5];
            }
         }
      }

      return value;
   }
}
