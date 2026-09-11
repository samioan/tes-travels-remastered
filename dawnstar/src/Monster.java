// Renamed from decompiled/d.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (monstersin.dat).
//
// `Player` here is the real, already-renamed Player class -- this file
// was updated alongside Player's own rename pass to integrate directly.
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.Random;

public class Monster {
   private static int typeCount;
   private static String[] typeName;
   // 17-byte stat row per monster type. Not every column is pinned down
   // yet -- see CLASS_MAP.md for the ones that are.
   private static byte[][] typeStats;
   static short nextSpawnIdCounter;

   short spawnId;
   byte monsterType;
   byte hp;
   byte x;
   byte y;
   boolean flag;
   byte dungeonLevel;
   byte[] scratch;
   byte moveCooldown;
   byte aiPhase;
   long timestamp;

   static short nextSpawnId() {
      nextSpawnIdCounter++;
      return nextSpawnIdCounter;
   }

   byte[] toBytes() {
      byte[] out = new byte[28];
      out[0] = (byte)(this.spawnId >>> 8 & 0xFF);
      out[1] = (byte)(this.spawnId & 0xFF);
      out[2] = this.monsterType;
      out[3] = this.hp;
      out[4] = this.x;
      out[5] = this.y;
      out[6] = (byte)(this.flag ? 1 : 0);
      out[7] = this.dungeonLevel;
      out[8] = this.moveCooldown;
      out[9] = this.aiPhase;
      out[10] = (byte)(this.timestamp >>> 56 & 255L);
      out[11] = (byte)(this.timestamp >>> 48 & 255L);
      out[12] = (byte)(this.timestamp >>> 40 & 255L);
      out[13] = (byte)(this.timestamp >>> 32 & 255L);
      out[14] = (byte)(this.timestamp >>> 24 & 255L);
      out[15] = (byte)(this.timestamp >>> 16 & 255L);
      out[16] = (byte)(this.timestamp >>> 8 & 255L);
      out[17] = (byte)(this.timestamp & 255L);

      for (int i = 0; i < 10; i++) {
         out[18 + i] = this.scratch[i];
      }

      return out;
   }

   public Monster() {
      this.scratch = new byte[10];
      this.flag = false;
   }

   // Spawns a new monster of `monsterType` onto dungeon level `dungeonLevel`.
   public Monster(int spawnId, int monsterType, int dungeonLevel) {
      this.spawnId = (short)spawnId;
      this.monsterType = (byte)monsterType;
      this.hp = typeStats[this.monsterType - 1][14];
      this.scratch = new byte[10];
      this.flag = false;
      this.dungeonLevel = (byte)dungeonLevel;
      this.aiPhase = 0;
   }

   static void fromBytes(Monster out, byte[] in) {
      short hi = (short)(in[0] & 0xFF);
      short lo = (short)(in[1] & 0xFF);
      out.spawnId = (short)(hi << 8 | lo);
      out.monsterType = in[2];
      out.hp = in[3];
      out.x = in[4];
      out.y = in[5];
      out.flag = in[6] != 0;
      out.dungeonLevel = in[7];
      out.moveCooldown = in[8];
      out.aiPhase = in[9];
      out.timestamp = Util.readLongAt(in, 10);

      for (int i = 0; i < 10; i++) {
         out.scratch[i] = in[18 + i];
      }
   }

   // Writes this instance back into ESGame.monsters[dungeonLevel-1],
   // keyed by its current tile.
   void store() {
      ESGame.monsters[this.dungeonLevel - 1].put(Util.posKey(this.x, this.y), this.toBytes());
   }

   String typeName() {
      return typeName[this.monsterType - 1];
   }

   int stat(int column) {
      return typeStats[this.monsterType - 1][column] & 0xFF;
   }

   // Monster types on the "undead" band (type ids 6-8).
   boolean isUndead() {
      return this.monsterType >= 6 && this.monsterType <= 8;
   }

   void takeDamage(int amount) {
      int hp = this.hp & 255;
      if (amount > hp) {
         amount = hp;
      }

      hp -= amount;
      this.hp = (byte)hp;
   }

   // One step of movement in dungeon direction 1-4 (N/E/S/W, matching
   // Player's facing convention). Fails (returns false) if the target
   // tile isn't walkable, or is one of the level's 4 fixed stairway
   // tiles (monsters don't use stairs).
   boolean move(int direction) {
      byte delta = 1;
      byte newX = this.x;
      byte newY = this.y;
      Dungeon level = ESGame.dungeons[this.dungeonLevel - 1];
      switch (direction) {
         case 1:
            delta = -1;
         case 3:
            newX = this.x;
            newY = (byte)(this.y + delta);
            break;
         case 4:
            delta = -1;
         case 2:
            newY = this.y;
            newX = (byte)(this.x + delta);
            break;
         default:
            return false;
      }

      if (!level.isWalkable(newX, newY)) {
         return false;
      }

      if (this.isStairwayTile(newX, newY)) {
         return false;
      }

      ESGame.monsters[this.dungeonLevel - 1].remove(Util.posKey(this.x, this.y));
      level.tiles[this.x][this.y] = Util.clearBit((byte)2, level.tiles[this.x][this.y]);
      level.tiles[newX][newY] = Util.setBit((byte)2, level.tiles[newX][newY]);
      this.x = newX;
      this.y = newY;
      this.store();
      return true;
   }

   // True if (x, y) is one of this level's 4 fixed stairway tiles --
   // (17,5)=north, (30,17)=east, (17,30)=south, (5,17)=west -- and this
   // level actually has a stairway in that direction
   // (Dungeon.stairsUpDir/stairsDownDir, a compass direction code 1-4,
   // matches). Distinct from Dungeon.neighbors[0..3], the plain
   // edge-to-edge level transitions.
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

   // Chases (targetX, targetY): moves once every 5 ticks (moveCooldown
   // 0..4), picking the axis with the larger distance first (ties broken
   // randomly), falling back to the other axis if the primary is
   // blocked.
   boolean chase(int targetX, int targetY) {
      boolean moved = false;
      if (this.moveCooldown == 0) {
         this.faceToward(targetX, targetY);
         this.moveCooldown++;
         moved = true;
      } else if (this.moveCooldown >= 4) {
         this.moveCooldown = 0;
      } else {
         this.moveCooldown++;
      }

      this.aiPhase = 0;
      this.store();
      return moved;
   }

   private void faceToward(int targetX, int targetY) {
      int dx = Math.abs(targetX - this.x);
      int dy = Math.abs(targetY - this.y);
      byte horiz;
      if (this.x < targetX) {
         horiz = 2;
      } else if (this.x > targetX) {
         horiz = 4;
      } else {
         horiz = -1;
      }

      byte vert;
      if (this.y < targetY) {
         vert = 3;
      } else if (this.y > targetY) {
         vert = 1;
      } else {
         vert = -1;
      }

      byte primary;
      byte secondary;
      if (dx > dy) {
         primary = horiz;
         secondary = vert;
      } else if (dx < dy) {
         primary = vert;
         secondary = horiz;
      } else {
         int coinFlip = ESGame.nextInt(2);
         if (coinFlip == 0) {
            primary = horiz;
            secondary = vert;
         } else {
            primary = vert;
            secondary = horiz;
         }
      }

      if (!this.move(primary)) {
         this.move(secondary);
      }
   }

   // Player combat/AI tick for this monster, called once per tick for
   // every monster within interaction range (see Dungeon.tickNearbyMonsters).
   // Alternates an 800ms "wind-up" phase (aiPhase 0->1) and an action
   // phase (1->2->1) so an attack doesn't land the instant a monster
   // notices the player. Returns true once an attack actually lands.
   boolean tick(Player player, long now) {
      boolean act = false;
      if (this.aiPhase == 0) {
         this.timestamp = now;
         this.aiPhase = 1;
      } else if (this.aiPhase == 1 && now - this.timestamp > 800L) {
         act = true;
      }

      if (!act) {
         return false;
      }

      this.aiPhase = 2;
      this.timestamp = now;
      byte detectionStat = typeStats[this.monsterType - 1][4];
      int stealth = player.weaponSkillValue(true);
      int diff = stealth - detectionStat;
      diff = Math.min(diff, typeStats[this.monsterType - 1][2]);
      int chanceA = typeStats[this.monsterType - 1][3] - diff * 5;
      int chanceB = player.baseEvasion() + diff * 5;
      chanceA = Math.min(Math.max(chanceA, 10), 95);
      chanceB = Math.min(Math.max(chanceB, 10), 95);
      int rollA = Util.randomInt(100);
      int rollB = Util.randomInt(100);
      boolean detectedA = rollA <= chanceA;
      boolean detectedB = rollB <= chanceB;
      byte outcome = 0;
      if (detectedA && !detectedB) {
         outcome = 3;
      } else if (detectedA && detectedB) {
         if (rollA >= rollB) {
            outcome = 2;
         } else {
            outcome = 1;
         }
      } else if (detectedA || detectedB) {
         outcome = 0;
      } else if (rollA >= rollB) {
         outcome = 2;
      } else {
         outcome = 1;
      }

      if (outcome == 0) {
         this.aiPhase = 1;
         return false;
      }

      byte attackStat = typeStats[this.monsterType - 1][5];
      int defense = player.armorValue();
      if (outcome == 1) {
         defense = 2 * defense;
      }

      int power = attackStat - defense;
      power = Math.max(power, 4);
      int damage = power * player.coreStats[3] / 100;
      player.coreStats[2] = (short)(player.coreStats[2] - damage);
      player.coreStats[2] = (short)Math.max(player.coreStats[2], 0);
      if (detectedB) {
         player.gainSkillExp(player.activeWeaponSkillIndex(), 1);
      }

      if (outcome < 3) {
         this.aiPhase = 1;
         return true;
      }

      if (Util.randomInt(100) <= 30) {
         byte ailment = typeStats[this.monsterType - 1][11];
         if (ailment > 0) {
            int bit = ailment - 1;
            player.ailmentMask = (byte)(player.ailmentMask | 1 << bit);
            if (ailment != 1) {
               if (ailment == 2) {
                  Dungeon level = ESGame.dungeons[this.dungeonLevel - 1];
                  level.populateRandomMonsters(3);
               } else if (ailment != 3) {
                  if (ailment == 4) {
                     player.trollThirstTimer = 30000;
                  } else if (ailment == 5) {
                     player.glacierCurseTimer = 30000;
                  } else if (ailment != 6 && ailment != 7 && ailment == 8) {
                  }
               }
            }
         }
      }

      this.aiPhase = 1;
      return true;
   }

   static void load() throws Exception {
      DataInputStream in = ESGame.getResource("monstersin.dat");
      nextSpawnIdCounter = 0;
      typeCount = in.readInt();
      typeName = new String[typeCount];
      typeStats = new byte[typeCount][17];

      for (int i = 0; i < typeCount; i++) {
         typeName[i] = in.readUTF();
      }

      for (int i = 0; i < typeCount; i++) {
         for (int c = 0; c < 17; c++) {
            typeStats[i][c] = in.readByte();
         }
      }

      in.close();
   }

   static Monster readFrom(DataInputStream in) throws Exception {
      Monster m = new Monster();
      m.spawnId = in.readShort();
      m.monsterType = in.readByte();
      m.hp = in.readByte();
      m.x = in.readByte();
      m.y = in.readByte();
      m.flag = in.readBoolean();
      m.dungeonLevel = in.readByte();
      m.moveCooldown = in.readByte();
      m.aiPhase = in.readByte();
      m.timestamp = in.readLong();

      for (int i = 0; i < 10; i++) {
         m.scratch[i] = in.readByte();
      }

      return m;
   }

   void writeTo(DataOutputStream out) throws Exception {
      out.writeShort(this.spawnId);
      out.writeByte(this.monsterType);
      out.writeByte(this.hp);
      out.writeByte(this.x);
      out.writeByte(this.y);
      out.writeBoolean(this.flag);
      out.writeByte(this.dungeonLevel);
      out.writeByte(this.moveCooldown);
      out.writeByte(this.aiPhase);
      out.writeLong(this.timestamp);

      for (int i = 0; i < 10; i++) {
         out.writeByte(this.scratch[i]);
      }
   }

   // Rolls and drops this monster's death loot. `guaranteed` bypasses the
   // normal drop-chance roll (used for scripted/quest kills).
   void onDeath(boolean guaranteed) {
      byte dropChance = typeStats[this.monsterType - 1][15];
      if (guaranteed) {
         dropChance = 100;
      }

      byte lootRow = typeStats[this.monsterType - 1][16];
      int roll = Util.randomInt(100);
      boolean drops = roll <= dropChance;
      if (drops || guaranteed) {
         Dungeon level = ESGame.dungeons[this.dungeonLevel - 1];
         byte depth = level.tier;
         int itemId = Item.rollLoot(ESGame.r, depth, lootRow);
         byte low = (byte)(itemId & 0xFF);
         byte high = 0;
         if (low == 86) {
            high = (byte)(itemId >>> 8 & 0xFF);
         }

         byte[] drop = new byte[]{this.x, this.y, low, 0, 0, high, 0};
         short spawnId = Item.nextSpawnId();
         low = (byte)(spawnId >>> 8 & 0xFF);
         high = (byte)(spawnId & 0xFF);
         drop[3] = low;
         drop[4] = high;
         drop[6] = 1;
         if (guaranteed) {
            drop[6] = (byte)(drop[6] | 4);
         }

         level.addDroppedItem(drop);
      }
   }

   // Spawns a new monster on dungeon level `dungeonLevel`. `difficultyTier`
   // is Dungeon.tier (a permuted 1-36 index -- NOT the level number, see
   // Dungeon.tier's doc comment), used to look up a random monster type
   // from Dungeon.MONSTER_TABLE when `forcedType` is negative (weighted by
   // a percentile roll into 4 difficulty buckets).
   static Monster spawn(Random rng, int difficultyTier, int dungeonLevel, int forcedType) {
      short spawnId = nextSpawnId();
      int type = forcedType;
      if (type < 0) {
         int tierIndex = difficultyTier - 1;
         if (tierIndex < 0) {
            tierIndex = 0;
         }

         if (tierIndex > 36) {
            tierIndex = 36;
         }

         int tierRoll = ESGame.lingoRandomInt(rng, 10);
         byte bucket;
         if (tierRoll <= 4) {
            bucket = 0;
         } else if (tierRoll <= 7) {
            bucket = 1;
         } else if (tierRoll <= 9) {
            bucket = 2;
         } else {
            bucket = 3;
         }

         type = Dungeon.MONSTER_TABLE[tierIndex][bucket];
      }

      return new Monster(spawnId, type, dungeonLevel);
   }
}
