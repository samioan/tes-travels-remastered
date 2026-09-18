// Renamed from decompiled/d.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (monstersin.dat).
//
// Same overall shape as dawnstar's Monster.java (../../dawnstar/src/
// Monster.java) -- monstersin.dat type table, a packed 28-byte
// toBytes/fromBytes record plus a separate stream readFrom/writeTo format,
// stat()'s masked (&0xFF) reads vs tick()/onDeath()'s raw-signed-byte
// reads of the same typeStats table -- but NOT a blind copy: two real
// differences confirmed while transcribing this file directly rather than
// assuming dawnstar's field semantics carried over:
//
// 1. `spawnId` (was `a`) is the per-spawn unique-id counter value (from
//    nextSpawnId()/the static counter), NOT a "type id" as dawnstar's
//    CLASS_MAP labels its own `a` -- confirmed by spawn()'s constructor
//    call `new Monster(nextSpawnId(), typeIndex, level.levelNumber)`.
// 2. store() keys the live per-level registry (`ESGame.monsters[level]`,
//    a Hashtable) by `String.valueOf(spawnId)`, not a "x,y" position key
//    like dawnstar's Util.posKey -- a real architecture difference, not a
//    transcription slip.
//
// Dungeon-side names referenced here (width/height/tiles/isWalkable/
// stairsUpDir/stairsDownDir/tier/levelNumber/MONSTER_TYPE_BY_TIER/
// spawnAmbushMonsters) are now CONFIRMED against ../src/Dungeon.java
// (i.java is a merged Dungeon+generator class -- see that file's own
// header comment) -- all match what this file already used, with the
// ambush hook (was `i.c(int)`) renamed from its earlier `unconfirmed*`
// placeholder to its real name, spawnAmbushMonsters(int).
//
// Player-side names in tick() are likewise now CONFIRMED against
// ../src/Player.java: defenseSkillValue(boolean)/baseEvasion()/
// armorValue()/gainSkillExp(int,int)+defenseSkillIndex() (retyped from
// this file's earlier TODO_*/unconfirmedTimestamp-adjacent placeholders),
// and the two ailment timers are vampirismTimer/manaBurnTimer (Stormhold's
// own ailment names, not dawnstar's trollThirstTimer/glacierCurseTimer --
// fixed here after initially carrying the dawnstar names over by mistake).
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.Random;

public class Monster {
   private static int typeCount;
   private static String[] typeNames;
   private static byte[][] typeStats;

   short spawnId;
   byte typeIndex;
   byte currentHp;
   byte tileX;
   byte tileY;
   boolean unconfirmedFlag;
   byte dungeonLevel;
   byte[] scratch;
   byte chaseCadence;
   byte aiPhase;
   long unconfirmedTimestamp;

   static Monster scratchInstance = new Monster();
   static short nextSpawnIdCounter;

   static short nextSpawnId() {
      nextSpawnIdCounter++;
      return nextSpawnIdCounter;
   }

   byte[] toBytes() {
      byte[] out = new byte[28];
      out[0] = (byte)(this.spawnId >>> 8 & 0xFF);
      out[1] = (byte)(this.spawnId & 0xFF);
      out[2] = this.typeIndex;
      out[3] = this.currentHp;
      out[4] = this.tileX;
      out[5] = this.tileY;
      out[6] = (byte)(this.unconfirmedFlag ? 1 : 0);
      out[7] = this.dungeonLevel;
      out[8] = this.chaseCadence;
      out[9] = this.aiPhase;
      out[10] = (byte)(this.unconfirmedTimestamp >>> 56 & 255L);
      out[11] = (byte)(this.unconfirmedTimestamp >>> 48 & 255L);
      out[12] = (byte)(this.unconfirmedTimestamp >>> 40 & 255L);
      out[13] = (byte)(this.unconfirmedTimestamp >>> 32 & 255L);
      out[14] = (byte)(this.unconfirmedTimestamp >>> 24 & 255L);
      out[15] = (byte)(this.unconfirmedTimestamp >>> 16 & 255L);
      out[16] = (byte)(this.unconfirmedTimestamp >>> 8 & 255L);
      out[17] = (byte)(this.unconfirmedTimestamp & 255L);

      for (int i = 0; i < 10; i++) {
         out[18 + i] = this.scratch[i];
      }

      return out;
   }

   public Monster() {
      this.scratch = new byte[10];
      this.unconfirmedFlag = false;
   }

   public Monster(int spawnId, int typeIndex, int dungeonLevel) {
      this.spawnId = (short)spawnId;
      this.typeIndex = (byte)typeIndex;
      this.currentHp = typeStats[this.typeIndex - 1][14];
      this.scratch = new byte[10];
      this.unconfirmedFlag = false;
      this.dungeonLevel = (byte)dungeonLevel;
      this.aiPhase = 0;
   }

   // Reads a packed 28-byte record into the shared scratch instance --
   // matches the original's single-static-buffer reuse rather than
   // allocating a fresh Monster per read.
   static Monster fromBytesShared(byte[] data) {
      short hi = (short)(data[0] & 0xFF);
      short lo = (short)(data[1] & 0xFF);
      scratchInstance.spawnId = (short)(hi << 8 | lo);
      scratchInstance.typeIndex = data[2];
      scratchInstance.currentHp = data[3];
      scratchInstance.tileX = data[4];
      scratchInstance.tileY = data[5];
      scratchInstance.unconfirmedFlag = data[6] != 0;
      scratchInstance.dungeonLevel = data[7];
      scratchInstance.chaseCadence = data[8];
      scratchInstance.aiPhase = data[9];
      scratchInstance.unconfirmedTimestamp = Util.readLongAt(data, 10);

      for (int i = 0; i < 10; i++) {
         scratchInstance.scratch[i] = data[18 + i];
      }

      return scratchInstance;
   }

   static Monster fromBytesInto(Monster target, byte[] data) {
      short hi = (short)(data[0] & 0xFF);
      short lo = (short)(data[1] & 0xFF);
      target.spawnId = (short)(hi << 8 | lo);
      target.typeIndex = data[2];
      target.currentHp = data[3];
      target.tileX = data[4];
      target.tileY = data[5];
      target.unconfirmedFlag = data[6] != 0;
      target.dungeonLevel = data[7];
      target.chaseCadence = data[8];
      target.aiPhase = data[9];
      target.unconfirmedTimestamp = Util.readLongAt(data, 10);

      for (int i = 0; i < 10; i++) {
         target.scratch[i] = data[18 + i];
      }

      return target;
   }

   // Stores this monster into ESGame.monsters[dungeonLevel-1], keyed by
   // its own unique spawnId (not a tile-position key).
   void store() {
      ESGame.monsters[this.dungeonLevel - 1].put(String.valueOf(this.spawnId), this.toBytes());
   }

   String typeName() {
      return typeNames[this.typeIndex - 1];
   }

   // Masked (&0xFF) column read -- the "public"-style accessor.
   int stat(int column) {
      return typeStats[this.typeIndex - 1][column] & 0xFF;
   }

   boolean isUndead() {
      return this.typeIndex >= 6 && this.typeIndex <= 8;
   }

   void takeDamage(int amount) {
      int hp = this.currentHp & 255;
      if (amount > hp) {
         amount = hp;
      }

      hp -= amount;
      this.currentHp = (byte)hp;
   }

   // Steps this monster one tile in compass direction `dir` (1=N,2=E,3=S,
   // 4=W), blocked by out-of-bounds, a stairway tile (isStairwayTile), or
   // an unwalkable tile. Does NOT call store() itself -- unlike dawnstar's
   // Monster.move, the registry write here is left to the caller.
   boolean move(int dir) {
      byte step = 1;
      byte newX = this.tileX;
      byte newY = this.tileY;
      Dungeon level = ESGame.dungeons[this.dungeonLevel - 1];
      switch (dir) {
         case 1:
            step = -1;
         case 3:
            newX = this.tileX;
            newY = (byte)(this.tileY + step);
            break;
         case 4:
            step = -1;
         case 2:
            newY = this.tileY;
            newX = (byte)(this.tileX + step);
            break;
         default:
            return false;
      }

      if (newX < 0 || newY < 0) {
         return false;
      }

      if (newX >= level.width || newY >= level.height) {
         return false;
      }

      if (this.isStairwayTile(newX, newY)) {
         return false;
      }

      if (!level.isWalkable(newX, newY)) {
         return false;
      }

      level.tiles[this.tileX][this.tileY] = Util.clearBit((byte)2, level.tiles[this.tileX][this.tileY]);
      level.tiles[newX][newY] = Util.setBit((byte)2, level.tiles[newX][newY]);
      this.tileX = newX;
      this.tileY = newY;
      return true;
   }

   // Chase-AI dispatcher, called once per nearby-monster scan. Only takes
   // an actual step 1 time in 5 (chaseCadence cycles 0..4); the other 4
   // calls are no-ops apart from advancing the counter.
   void chase(Player player) {
      if (this.isWithinRange(player)) {
         if (this.chaseCadence == 0) {
            this.chaseStep(player);
            this.chaseCadence++;
         } else if (this.chaseCadence >= 4) {
            this.chaseCadence = 0;
         } else {
            this.chaseCadence++;
         }
      }
   }

   // Picks the axis with the larger distance-to-player and tries that
   // direction first, falling back to the other axis; on an exact tie,
   // rolls a coin. Matches dawnstar's Monster.chase 1:1 in structure.
   private void chaseStep(Player player) {
      int dx = Math.abs(player.tileX - this.tileX);
      int dy = Math.abs(player.tileY - this.tileY);
      byte xDir;
      if (this.tileX < player.tileX) {
         xDir = 2;
      } else if (this.tileX > player.tileX) {
         xDir = 4;
      } else {
         xDir = -1;
      }

      byte yDir;
      if (this.tileY < player.tileY) {
         yDir = 3;
      } else if (this.tileY > player.tileY) {
         yDir = 1;
      } else {
         yDir = -1;
      }

      byte first;
      byte second;
      if (dx > dy) {
         first = xDir;
         second = yDir;
      } else if (dx < dy) {
         first = yDir;
         second = xDir;
      } else {
         int coin = ESGame.lingoRandomInt(2);
         if (coin == 0) {
            first = xDir;
            second = yDir;
         } else {
            first = yDir;
            second = xDir;
         }
      }

      if (!this.move(first)) {
         this.move(second);
      }
   }

   // The cascading N/S/W/E stairway-tile check, keyed off the level's own
   // up/down stairway direction fields -- same "only the highest-priority
   // direction ever matches" property dawnstar's port docs flagged for its
   // own isStairwayTile (see dawnstar's PORT_ROADMAP.md M15). Preserved
   // byte-for-byte, not "fixed" into an order-independent check.
   private boolean isStairwayTile(int x, int y) {
      Dungeon level = ESGame.dungeons[this.dungeonLevel - 1];
      if (level.stairsUpDir != 1 && level.stairsDownDir != 1) {
         if (level.stairsUpDir != 3 && level.stairsDownDir != 3) {
            if (level.stairsUpDir != 4 && level.stairsDownDir != 4) {
               if ((level.stairsUpDir == 2 || level.stairsDownDir == 2) && x == 30 && y == 17) {
                  return true;
               }
            } else if (x == 5 && y == 17) {
               return true;
            }
         } else if (x == 17 && y == 30) {
            return true;
         }
      } else if (x == 17 && y == 5) {
         return true;
      }

      return false;
   }

   boolean isWithinRange(Player player) {
      return this.distanceTo(player) <= 3;
   }

   int distanceTo(Player player) {
      int dx = Math.abs(player.tileX - this.tileX);
      int dy = Math.abs(player.tileY - this.tileY);
      return dx + dy;
   }

   // True only when exactly adjacent (manhattan distance 1); otherwise
   // resets aiPhase to idle as a side effect.
   boolean isAdjacent(Player player) {
      if (this.distanceTo(player) == 1) {
         return true;
      }

      this.aiPhase = 0;
      return false;
   }

   // Per-tick combat step: rolls attacker/defender percentile chances off
   // typeStats (RAW signed-byte reads, not stat()'s masked ones -- see
   // this file's header comment), resolves a 4-tier outcome (miss/graze/
   // hit/crit-ish), applies damage on a hit, and on a strong hit rolls a
   // 30% chance to inflict a status effect from typeStats column 11.
   //
   // CONFIRMED against ../src/Player.java: `var1.f(true)` is
   // defenseSkillValue(true) (the offhand/shield-slot-keyed skill value --
   // this player is the DEFENDER here), `var1.I()` is baseEvasion(),
   // `var1.v()` is armorValue(), and `var1.a(var1.y(), 1)` is
   // gainSkillExp(defenseSkillIndex(), 1) -- fires only when the
   // *defender* roll also hit, i.e. the player successfully blocked and
   // gains shield-skill exp for it.
   void tick(Player player, long now) {
      this.aiPhase = 2;
      this.unconfirmedTimestamp = now;
      byte baseDefense = typeStats[this.typeIndex - 1][4];
      int playerOffense = player.defenseSkillValue(true);
      int diff = playerOffense - baseDefense;
      diff = Math.min(diff, typeStats[this.typeIndex - 1][2]);
      int attackChance = typeStats[this.typeIndex - 1][3] - diff * 5;
      int defendChance = player.baseEvasion() + diff * 5;
      attackChance = Math.min(Math.max(attackChance, 10), 95);
      defendChance = Math.min(Math.max(defendChance, 10), 95);
      int rollA = ESGame.nextInt(100);
      int rollB = ESGame.nextInt(100);
      boolean hitA = rollA <= attackChance;
      boolean hitB = rollB <= defendChance;
      byte tier;
      if (hitA && !hitB) {
         tier = 3;
      } else if (hitA && hitB) {
         tier = (byte)(rollA >= rollB ? 2 : 1);
      } else if (hitA || hitB) {
         tier = 0;
      } else {
         tier = (byte)(rollA >= rollB ? 2 : 1);
      }

      if (tier == 0) {
         this.aiPhase = 1;
      } else {
         byte basePower = typeStats[this.typeIndex - 1][5];
         int armor = player.armorValue();
         if (tier == 1) {
            armor = 2 * armor;
         }

         int damage = basePower - armor;
         damage = Math.max(damage, 4);
         int scaled = damage * player.coreStats[3] / 100;
         player.coreStats[2] = (short)(player.coreStats[2] - scaled);
         player.coreStats[2] = (short)Math.max(player.coreStats[2], 0);
         if (hitB) {
            player.gainSkillExp(player.defenseSkillIndex(), 1);
         }

         if (tier < 3) {
            this.aiPhase = 1;
         } else {
            if (ESGame.nextInt(100) <= 30) {
               byte ailmentId = typeStats[this.typeIndex - 1][11];
               if (ailmentId > 0) {
                  int bit = ailmentId - 1;
                  player.ailmentMask = (byte)(player.ailmentMask | 1 << bit);
                  if (ailmentId != 1) {
                     if (ailmentId == 2) {
                        Dungeon level = ESGame.dungeons[this.dungeonLevel - 1];
                        level.spawnAmbushMonsters(3);
                     } else if (ailmentId != 3) {
                        if (ailmentId == 4) {
                           player.vampirismTimer = 30000;
                        } else if (ailmentId == 5) {
                           player.manaBurnTimer = 30000;
                        } else if (ailmentId != 6 && ailmentId != 7 && ailmentId == 8) {
                        }
                     }
                  }
               }
            }

            this.aiPhase = 1;
         }
      }
   }

   static void loadTypes() throws Exception {
      DataInputStream in = Util.openResource("/monstersin.dat");
      nextSpawnIdCounter = 0;
      typeCount = in.readInt();
      typeNames = new String[typeCount];
      typeStats = new byte[typeCount][17];

      for (int i = 0; i < typeCount; i++) {
         typeNames[i] = in.readUTF();
      }

      for (int i = 0; i < typeCount; i++) {
         for (int c = 0; c < 17; c++) {
            typeStats[i][c] = in.readByte();
         }
      }
   }

   // Explicit stream serialization -- distinct from toBytes()/fromBytes*'s
   // packed 28-byte in-memory record, same fields either way.
   static Monster readFrom(DataInputStream in) throws Exception {
      Monster m = new Monster();
      m.spawnId = in.readShort();
      m.typeIndex = in.readByte();
      m.currentHp = in.readByte();
      m.tileX = in.readByte();
      m.tileY = in.readByte();
      m.unconfirmedFlag = in.readBoolean();
      m.dungeonLevel = in.readByte();
      m.chaseCadence = in.readByte();
      m.aiPhase = in.readByte();
      m.unconfirmedTimestamp = in.readLong();

      for (int i = 0; i < 10; i++) {
         m.scratch[i] = in.readByte();
      }

      return m;
   }

   void writeTo(DataOutputStream out) throws Exception {
      out.writeShort(this.spawnId);
      out.writeByte(this.typeIndex);
      out.writeByte(this.currentHp);
      out.writeByte(this.tileX);
      out.writeByte(this.tileY);
      out.writeBoolean(this.unconfirmedFlag);
      out.writeByte(this.dungeonLevel);
      out.writeByte(this.chaseCadence);
      out.writeByte(this.aiPhase);
      out.writeLong(this.unconfirmedTimestamp);

      for (int i = 0; i < 10; i++) {
         out.writeByte(this.scratch[i]);
      }
   }

   // Declared, empty, no parameters -- dead code preserved as a no-op
   // rather than guessed at (same treatment dawnstar gave its own
   // confirmed-unused hooks).
   void unusedHook() {
   }

   // Death-drop roll: typeStats column 15 is the base drop-chance percent
   // (100% if guaranteedDrop), column 16 is the loot-table row offset fed
   // into Item.rollLoot. On a hit, builds a 7-byte dropped-item record and
   // adds it to the level's dropped-item registry via an as-yet-unnamed
   // Dungeon method (`c(byte[])` in the original).
   void onDeath(boolean guaranteedDrop) {
      byte dropChance = typeStats[this.typeIndex - 1][15];
      if (guaranteedDrop) {
         dropChance = 100;
      }

      byte lootBonus = typeStats[this.typeIndex - 1][16];
      int roll = ESGame.nextInt(100);
      boolean dropped = roll <= dropChance;
      if (dropped || guaranteedDrop) {
         Dungeon level = ESGame.dungeons[this.dungeonLevel - 1];
         byte depth = level.tier;
         int itemRoll = Item.rollLoot(ESGame.rng, depth, lootBonus);
         byte low = (byte)(itemRoll & 0xFF);
         byte high = 0;
         if (low == 86) {
            high = (byte)(itemRoll >>> 8 & 0xFF);
         }

         byte[] record = new byte[]{this.tileX, this.tileY, low, 0, 0, high, 0};
         short dropSpawnId = Item.nextSpawnId();
         low = (byte)(dropSpawnId >>> 8 & 0xFF);
         high = (byte)(dropSpawnId & 0xFF);
         record[3] = low;
         record[4] = high;
         record[6] = 1;
         if (guaranteedDrop) {
            record[6] = (byte)(record[6] | 4);
         }

         level.addDroppedItem(record);
      }
   }

   static Monster spawn(Dungeon level) {
      return spawn(ESGame.rng, level, -1);
   }

   // Picks a monster type by tier/rarity-bucket roll when forcedTypeIndex
   // is negative: clamps the level's tier-1 index to [0,36], rolls a
   // 1-10 rarity bucket (<=4:0, <=7:1, <=9:2, else:3), and looks the type
   // index up from Dungeon's own [zone][bucket] table.
   static Monster spawn(Random rng, Dungeon level, int forcedTypeIndex) {
      short spawnId = nextSpawnId();
      int typeIndex = forcedTypeIndex;
      if (typeIndex < 0) {
         int zone = level.tier - 1;
         if (zone < 0) {
            zone = 0;
         }

         if (zone > 36) {
            zone = 36;
         }

         int bucketRoll = ESGame.randomInt(rng, 10);
         byte bucket;
         if (bucketRoll <= 4) {
            bucket = 0;
         } else if (bucketRoll <= 7) {
            bucket = 1;
         } else if (bucketRoll <= 9) {
            bucket = 2;
         } else {
            bucket = 3;
         }

         typeIndex = Dungeon.MONSTER_TYPE_BY_TIER[zone][bucket];
      }

      return new Monster(spawnId, typeIndex, level.levelNumber);
   }
}
