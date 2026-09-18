// Renamed from decompiled/i.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (dungnamesin.dat).
//
// UNLIKE dawnstar, Stormhold has NO separate DungeonGenerator class --
// confirmed by reading decompiled/c.java directly (38 lines, a thin
// FullCanvas UI delegator, completely unrelated to dungeon generation).
// The room-carving/corridor-connection/monster-placement/chest-placement
// algorithm lives directly on this class, fused with the live per-level
// state (tiles, dimensions, stairway dirs) dawnstar splits into a separate
// Dungeon. Confirmed by reading every method in decompiled/i.java: the
// static generate()/rollRoomRect()/tryPlaceRoom()/connectRooms()/
// carveStairwell()/carveCorridorBetween() methods build this SAME
// instance's own `tiles` grid, not a separate object.
//
// Two real, confirmed differences from dawnstar's generator worth noting
// up front (not guessed -- read directly off this file):
// 1. Per-level RNG seed is `levelNumber * 5000` here, not dawnstar's
//    `levelNumber * 8000`.
// 2. Level 37's LAST placed room gets a forced monster type 41 (a
//    scripted encounter, matching the "end-game monster" pattern
//    dawnstar's own CLASS_MAP.md flags for its port at level 37 with a
//    different forced type, 42) -- see generate()/spawnRoomMonsters().
//
// Field/method names below are held with high confidence (each traced to
// a concrete call site, several cross-checked directly against
// Monster.java's TODO placeholders and Shop.java's k.j[]/k.i[]/k.d/k.b[]
// NPC-position/Warden fields) but two things are explicitly left
// unconfirmed rather than guessed: the 2nd column of `NAMES` (dungnamesin.dat
// stores 2 UTF strings per level; only ever seen read as a whole array via
// displayNames(), never indexed [0] vs [1] separately in what's been
// traced here) and tile bit 3 (mask 8)'s exact purpose (blocks both
// isWalkable() and chest placement, consistent with dawnstar's own
// "no-spawn/special" bit3, but what marks it during generation was not
// pinned down further -- see generate()'s doc comment).
import java.io.DataInputStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Random;
import java.util.Vector;

public class Dungeon {
   // dungnamesin.dat: 37 levels x 2 UTF strings each. 2nd column's exact
   // meaning unconfirmed (see class header).
   static String[][] NAMES;

   // [zone][rarityBucket] monster type index, zone = tier-1 clamped to
   // [0,36]. Confirmed caller: Monster.spawn().
   static final int[][] MONSTER_TYPE_BY_TIER = new int[][]{
      {1, 2, 1, 3},
      {6, 7, 8, 6},
      {1, 2, 3, 1},
      {6, 7, 8, 7},
      {3, 4, 11, 12},
      {8, 9, 11, 12},
      {3, 4, 12, 13},
      {8, 9, 12, 13},
      {4, 5, 12, 13},
      {9, 10, 12, 13},
      {4, 5, 13, 14},
      {9, 10, 13, 14},
      {12, 13, 4, 5},
      {12, 13, 9, 10},
      {13, 14, 15, 16},
      {14, 15, 16, 17},
      {15, 16, 17, 18},
      {16, 17, 18, 21},
      {17, 18, 19, 26},
      {18, 19, 20, 21},
      {19, 20, 26, 27},
      {21, 22, 26, 27},
      {21, 22, 27, 28},
      {22, 23, 27, 28},
      {22, 23, 28, 29},
      {23, 24, 28, 29},
      {23, 24, 29, 30},
      {24, 25, 29, 30},
      {26, 27, 28, 31},
      {27, 28, 31, 32},
      {28, 29, 32, 33},
      {29, 30, 33, 34},
      {31, 32, 34, 35},
      {32, 33, 36, 37},
      {34, 35, 37, 38},
      {38, 39, 40, 35},
      {38, 39, 40, 35}
   };

   // Permuted difficulty-tier index in [1,36] for levels 2-37, indexed by
   // [levelNumber-2]. Same role as dawnstar's DIFFICULTY_TIER_LOOKUP.
   static final byte[] DIFFICULTY_TIER_LOOKUP = new byte[]{
      1, 5, 9, 13, 14, 15, 22, 23, 24, 2, 6, 10, 19, 20, 21, 31, 32, 33, 3, 7, 11, 16, 17, 18, 28, 29, 30, 4, 8, 12, 25, 26, 27, 34, 35, 36
   };

   // Indexed by [tier-1]: the "gift" item subtype rolled for chest #1 (the
   // one guaranteed special item per level, see placeChests()). Values are
   // only ever 1, 3, or 5.
   static final int[] CHEST_GIFT_SUBTYPE_BY_TIER = new int[]{
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 3, 1, 3, 3, 1, 3, 3, 3, 3, 5, 3, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5
   };

   byte levelNumber;
   byte tier;
   short width;
   short height;
   byte[][] tiles;

   // Reusable 2-element [x,y] unpack scratch buffer for unpackXY().
   short[] unpackScratch;
   // Reusable 6-element [x0,y0,x1,y1,doorX,doorY] room-rect scratch used by
   // rollRoomRect()/tryPlaceRoom() during generation.
   short[] roomRectScratch;

   // Room-placement bounds (interior margin), default 3..31 on a 35x35
   // level -- minY/maxX/maxY/minX respectively (matches rollRoomRect()'s
   // own usage, not their declaration order).
   short minY;
   short minX;
   short maxY;
   short maxX;

   short stairsUpDir;
   short stairsDownDir;

   // True once this level's tiles are actually built -- either by
   // allocateAndGenerate() or immediately in the hub-town constructor.
   // Same role as dawnstar's Dungeon.populated flag, consulted by tileAt()
   // when stitching into a neighbor that hasn't been generated yet.
   boolean populated = false;

   Vector allRoomIds;
   Vector unconnectedRoomIds;
   // Placed room rects, each a 6-short [x0,y0,x1,y1,doorX,doorY] array.
   Vector rooms;

   Random rng;
   // geomin.dat row: [north,east,south,west neighbor level ids, upStairDir,
   // downStairDir].
   byte[] neighbors;

   // Reusable 2-element scratch for relativeViewOffset()'s return value.
   int[] viewOffsetScratch = new int[2];

   // Declared, set false in both constructors, never read anywhere traced
   // in this file -- left unconfirmed rather than guessed (same treatment
   // dawnstar gives its own never-read fields).
   boolean unconfirmedH = false;

   public Dungeon() {
   }

   // The standard per-level constructor -- fixed 35x35, tiles NOT
   // allocated yet (done later by allocateAndGenerate()).
   public Dungeon(byte levelNumber, byte[] neighbors) {
      this();
      this.levelNumber = levelNumber;
      this.initTier();
      this.width = 35;
      this.height = 35;
      this.unpackScratch = new short[2];
      this.roomRectScratch = new short[6];
      this.neighbors = neighbors;
      this.stairsUpDir = this.neighbors[4];
      this.stairsDownDir = this.neighbors[5];
   }

   // The hub-town (level 1) constructor: takes an EXTERNALLY built tile
   // grid (ESGame's own hand-carved 19x19 grid, `ESGame.hubTiles` -- see
   // ESGame.java, ~line 1326: `af=19; aO=19; i=new byte[aO][af];`) instead
   // of generating one, and immediately marks every active Shop NPC's
   // world position with tile bit 5 (mask 32, "blocked") -- 6 NPCs
   // normally, 7 when Shop's Warden mechanic is active (Shop.wardenPresent).
   // Sets populated=true immediately, skipping generate() entirely.
   public Dungeon(byte levelNumber, byte[] neighbors, int width, int height, byte[][] tiles) {
      this();
      this.levelNumber = levelNumber;
      this.initTier();
      this.width = (short)width;
      this.height = (short)height;
      this.tiles = tiles;
      this.unpackScratch = new short[2];
      this.roomRectScratch = new short[6];
      this.neighbors = neighbors;
      this.stairsUpDir = this.neighbors[4];
      this.stairsDownDir = this.neighbors[5];
      int shopCount = 6;
      if (Shop.wardenPresent) {
         shopCount++;
      }

      for (int i = 0; i < shopCount; i++) {
         this.tiles[Shop.SHOP_X[i]][Shop.SHOP_Y[i]] = (byte)(this.tiles[Shop.SHOP_X[i]][Shop.SHOP_Y[i]] | 32);
      }

      this.populated = true;
   }

   // tier = 1 for the hub town (level 1) and any level number outside
   // [2,37]; otherwise DIFFICULTY_TIER_LOOKUP[levelNumber-2].
   void initTier() {
      this.tier = 1;
      if (this.levelNumber >= 2 && this.levelNumber <= 37) {
         this.tier = DIFFICULTY_TIER_LOOKUP[this.levelNumber - 2];
      }
   }

   // One monster spawned at the center of every placed room. Level 37's
   // LAST room is a forced monster type 41 (see class header) -- every
   // other room/level rolls a type via Monster.spawn(rng, this, -1).
   void spawnRoomMonsters() {
      int count = this.rooms.size();
      short packed = (short)(this.levelNumber << 8);
      Monster m = null;

      for (int i = 0; i < count; i++) {
         short[] room = (short[])this.rooms.elementAt(i);
         if (this.levelNumber == 37 && i == count - 1) {
            m = Monster.spawn(this.rng, this, 41);
         } else {
            m = Monster.spawn(this.rng, this, -1);
         }

         this.placeMonsterAt(m, room);
         m.store();
      }

      System.gc();
   }

   void placeMonsterAt(Monster m, short[] room) {
      m.tileX = (byte)room[4];
      m.tileY = (byte)room[5];
      this.tiles[m.tileX][m.tileY] = (byte)(this.tiles[m.tileX][m.tileY] | 2);
   }

   // Supplemental spawner: drops `count` EXTRA monsters at random walkable
   // positions inside random rooms (not necessarily room centers).
   // Confirmed caller: Monster.tick()'s ailment-2 branch (a "swarm curse"
   // -style on-hit status effect) -- this is what Monster.java's
   // `unconfirmedAmbushHook(int)` TODO placeholder actually is; see this
   // file's own reconciliation note at the bottom.
   void spawnAmbushMonsters(int count) {
      int roomCount = this.rooms.size();

      for (int i = 0; i < count; i++) {
         Monster m;
         short x;
         short y;
         do {
            int roomIdx = Math.abs(this.rng.nextInt() % roomCount);
            m = Monster.spawn(this.rng, this, -1);
            short[] room = (short[])this.rooms.elementAt(roomIdx);
            short w = (short)(room[2] - room[0] + 1);
            short h = (short)(room[3] - room[1] + 1);
            x = (short)(room[0] + Math.abs(this.rng.nextInt() % w));
            y = (short)(room[1] + Math.abs(this.rng.nextInt() % h));
         } while (!this.isWalkable(x, y));

         m.tileX = (byte)x;
         m.tileY = (byte)y;
         this.tiles[m.tileX][m.tileY] = Util.setBit((byte)2, this.tiles[m.tileX][m.tileY]);
         m.store();
      }
   }

   // Spawns one monster adjacent to `player` (was `a(Player)`), tried at up
   // to 5 candidate offsets in a fixed order (W,E,N,S,S-again -- the loop's
   // own `var6<2`/else split, transcribed as found) until one is walkable;
   // no-op in the hub town (levelNumber==1) or if none of the 5 candidates
   // is walkable. Confirmed sole caller: GameCanvas's camp-interruption
   // handling in run() (was `this.ax.b().a(this.ax)`), i.e. this is the
   // "a monster appears when your rest is disturbed" behavior. A local
   // boolean flag in the original (always true, never read) is dead code,
   // not transcribed.
   void spawnAmbushMonsterNearPlayer(Player player) {
      if (this.levelNumber != 1) {
         Monster m = Monster.spawn(this);
         byte px = player.tileX;
         byte py = player.tileY;

         for (int i = 0; i <= 4; i++) {
            int x = px;
            int y = py;
            if (i < 2) {
               x += 2 * i - 1;
            } else {
               y += 2 * i - 5;
            }

            if (this.isWalkable(x, y)) {
               m.tileX = (byte)x;
               m.tileY = (byte)y;
               m.store();
               this.tiles[m.tileX][m.tileY] = Util.setBit((byte)2, this.tiles[m.tileX][m.tileY]);
               break;
            }
         }
      }
   }

   // Spawns exactly one monster adjacent to `player` (checks the 4
   // orthogonal neighbor tiles via a fixed +/-1 offset scan), skipped
   // entirely in the hub town (levelNumber==1). Caller not confirmed in
   // this pass -- likely a "camp interrupted" or scripted-event hook in
   // ESGame/GameCanvas, not yet renamed.
   void spawnMonsterNearPlayer(Player player) {
      if (this.levelNumber != 1) {
         Monster m = Monster.spawn(this.rng, this, -1);
         byte px = player.tileX;
         byte py = player.tileY;

         for (int i = 0; i <= 4; i++) {
            int x = px;
            int y = py;
            if (i < 2) {
               x += 2 * i - 1;
            } else {
               y += 2 * i - 5;
            }

            if (this.isWalkable(x, y)) {
               m.tileX = (byte)x;
               m.tileY = (byte)y;
               m.store();
               this.tiles[m.tileX][m.tileY] = Util.setBit((byte)2, this.tiles[m.tileX][m.tileY]);
               break;
            }
         }
      }
   }

   // Descending-order comparator (ascending=false sorts largest-first) for
   // randomRoomIndices()'s random-key insertion sort.
   static int compare(int a, int b, boolean ascending) {
      if (ascending) {
         if (a < b) {
            return -1;
         } else {
            return a > b ? 1 : 0;
         }
      } else if (a > b) {
         return -1;
      } else {
         return a < b ? 1 : 0;
      }
   }

   // Picks `count` distinct random room indices: assigns every room a
   // random key in [0,1000), insertion-sorts descending, takes the first
   // `count` -- equivalent to sampling `count` rooms without replacement.
   int[] randomRoomIndices(int count) {
      int n = this.rooms.size();
      int[] indices = new int[n];
      int[] keys = new int[n];

      for (int i = 0; i < n; i++) {
         indices[i] = i;
         keys[i] = Util.randomInt(this.rng, 1000);
      }

      for (int i = 1; i < n; i++) {
         int key = keys[i];
         int idx = indices[i];

         int j;
         for (j = i - 1; j >= 0 && compare(keys[j], key, false) > 0; j--) {
            keys[j + 1] = keys[j];
            indices[j + 1] = indices[j];
         }

         keys[j + 1] = key;
         indices[j + 1] = idx;
      }

      int[] result = new int[count];

      for (int i = 0; i < count; i++) {
         result[i] = indices[i];
      }

      return result;
   }

   // Places exactly 5 chests in 5 random rooms. The FIRST is a guaranteed
   // "gift"/special item (Item.randomGiftItemOfSubtype, subtype from
   // CHEST_GIFT_SUBTYPE_BY_TIER[tier-1]); the other 4 roll normal loot
   // (Item.rollLoot with 2 bonus rolls). Avoids any tile already marked
   // with bit 3 (mask 8, see class header).
   void placeChests() {
      int lootRoll = 0;
      int[] roomIdx = this.randomRoomIndices(5);
      int giftSubtype = CHEST_GIFT_SUBTYPE_BY_TIER[this.tier - 1];
      boolean first = true;

      for (int i = 0; i < 5; i++) {
         short[] room = (short[])this.rooms.elementAt(roomIdx[i]);
         if (first) {
            lootRoll = Item.randomGiftItemOfSubtype(this.rng, giftSubtype);
            first = false;
         } else {
            lootRoll = Item.rollLoot(this.rng, this.tier, 2);
         }

         short w = (short)(room[2] - room[0] + 1);
         short h = (short)(room[3] - room[1] + 1);
         short x = (short)(room[0] + Math.abs(this.rng.nextInt() % w));

         short y;
         for (y = (short)(room[1] + Math.abs(this.rng.nextInt() % h)); (this.tiles[x][y] & 8) != 0; y = (short)(room[1] + Math.abs(this.rng.nextInt() % h))) {
            x = (short)(room[0] + Math.abs(this.rng.nextInt() % w));
         }

         byte[] record = new byte[8];
         record[0] = (byte)x;
         record[1] = (byte)y;
         record[2] = (byte)(first ? 1 : 0);
         byte tierBits = (byte)(Math.abs(this.rng.nextInt() % 3) << 6);
         record[3] = (byte)(tierBits | this.tier);
         byte low = (byte)(lootRoll & 0xFF);
         byte high = 0;
         if (low == 86) {
            high = (byte)(lootRoll >>> 8 & 0xFF);
         }

         record[4] = low;
         record[7] = high;
         short spawnId = Item.nextSpawnId();
         low = (byte)(spawnId >>> 8 & 0xFF);
         high = (byte)(spawnId & 0xFF);
         record[5] = low;
         record[6] = high;
         this.storeChest(record);
      }
   }

   void storeChest(byte[] record) {
      byte x = record[0];
      byte y = record[1];
      ESGame.chests[this.levelNumber - 1].put(Util.posKey(x, y), record);
      this.tiles[x][y] = (byte)(this.tiles[x][y] | 16);
   }

   void addDroppedItem(byte[] record) {
      byte x = record[0];
      byte y = record[1];
      ESGame.droppedItems[this.levelNumber - 1].addElement(record);
      this.tiles[x][y] = (byte)(this.tiles[x][y] | 4);
   }

   // Allocates the tile grid and runs generation for a standard level
   // (called by whatever builds ESGame.dungeons[] for levels 2-37 --
   // NOTE the seed formula: levelNumber*5000, not dawnstar's *8000).
   public void allocateAndGenerate() {
      long seed = this.levelNumber * 5000;
      this.tiles = new byte[this.width][this.height];
      short up = this.neighbors[4];
      short down = this.neighbors[5];
      this.rooms = new Vector();
      this.generate(seed, up, down);
   }

   void populate() {
      this.spawnRoomMonsters();
      this.placeChests();
   }

   // Rebuilds transient tile-presence bits (monster/chest/dropped-item,
   // plus the hub town's shop-blocked bit) from ESGame's live registries --
   // used to resync tiles after a save/load or similar full-state refresh.
   void refreshTileFlagsFromRegistries() {
      Hashtable monsters = ESGame.monsters[this.levelNumber - 1];
      if (monsters != null) {
         Enumeration e = monsters.elements();

         while (e.hasMoreElements()) {
            byte[] data = (byte[])e.nextElement();
            Monster m = Monster.fromBytesShared(data);
            this.tiles[m.tileX][m.tileY] = (byte)(this.tiles[m.tileX][m.tileY] | 2);
         }

         System.gc();
      }

      Hashtable chests = ESGame.chests[this.levelNumber - 1];
      if (chests != null) {
         Enumeration e = chests.elements();

         while (e.hasMoreElements()) {
            byte[] c = (byte[])e.nextElement();
            this.tiles[c[0]][c[1]] = (byte)(this.tiles[c[0]][c[1]] | 16);
         }
      }

      Enumeration e = ESGame.droppedItems[this.levelNumber - 1].elements();

      while (e.hasMoreElements()) {
         byte[] d = (byte[])e.nextElement();
         this.tiles[d[0]][d[1]] = (byte)(this.tiles[d[0]][d[1]] | 4);
      }

      if (this.levelNumber == 1 && Shop.wardenPresent) {
         byte x = Shop.SHOP_X[6];
         byte y = Shop.SHOP_Y[6];
         this.tiles[x][y] = (byte)(this.tiles[x][y] | 32);
      }
   }

   static boolean isValidDirection(short dir) {
      switch (dir) {
         case 1:
         case 2:
         case 3:
         case 4:
            return true;
         default:
            return false;
      }
   }

   // The real generation algorithm: seeds the per-level RNG, fills the
   // whole grid to wall(1), carves the up/down stairway corridors at fixed
   // border positions (if valid directions), places up to 15
   // non-overlapping rooms (rollRoomRect+tryPlaceRoom), and connects them
   // with a nearest-neighbor corridor pass (connectRooms). After placing
   // >=2 rooms, the first room found that's at least 3x3 (and whose center
   // isn't already the room's own door tile) gets its center tile marked
   // with bit 3 (mask 8) -- exact purpose of this marker not confirmed
   // (see class header), but it's set exactly once per level.
   public void generate(long seed, short stairsUp, short stairsDown) {
      this.stairsUpDir = stairsUp;
      this.stairsDownDir = stairsDown;
      int lastStairRoomId = -1;
      this.rng = new Random(seed);

      for (int x = 0; x < 35; x++) {
         for (int y = 0; y < 35; y++) {
            this.tiles[x][y] = 1;
         }
      }

      this.allRoomIds = new Vector();
      this.unconnectedRoomIds = new Vector();
      this.minY = 3;
      this.minX = 3;
      this.maxY = 31;
      this.maxX = 31;
      if (isValidDirection(stairsUp)) {
         lastStairRoomId = this.carveStairwell(stairsUp);
         if (lastStairRoomId >= 0) {
            this.registerRoomId(lastStairRoomId);
         }
      }

      if (isValidDirection(stairsDown)) {
         lastStairRoomId = this.carveStairwell(stairsDown);
         if (lastStairRoomId >= 0) {
            this.registerRoomId(lastStairRoomId);
         }
      }

      boolean markedCenter = false;
      int placed = 0;

      while (placed < 15) {
         this.roomRectScratch = this.rollRoomRect();
         if (this.tryPlaceRoom(this.roomRectScratch)) {
            placed++;
            int roomId = this.packXY(this.roomRectScratch[4], this.roomRectScratch[5]);
            this.registerRoomId(roomId);
            if (placed >= 2 && !markedCenter) {
               short doorX = this.roomRectScratch[4];
               short doorY = this.roomRectScratch[5];
               short w = (short)(this.roomRectScratch[2] - this.roomRectScratch[0] + 1);
               short h = (short)(this.roomRectScratch[3] - this.roomRectScratch[1] + 1);
               if (w >= 3 && h >= 3) {
                  short cx = (short)(this.roomRectScratch[0] + w / 2);
                  short cy = (short)(this.roomRectScratch[1] + h / 2);
                  if (cx != doorX || cy != doorY) {
                     this.tiles[cx][cy] = (byte)(this.tiles[cx][cy] | 8);
                     markedCenter = true;
                  }
               }
            }
         }
      }

      this.connectRooms();
   }

   // Rolls a random room rect: 2-5 tiles wide/tall, positioned within
   // [minX,maxX]x[minY,maxY], plus a random interior "door" point --
   // writes into and returns roomRectScratch.
   private short[] rollRoomRect() {
      byte maxSize = 4;
      int w = 2 + Math.abs(this.rng.nextInt()) % maxSize;
      int h = 2 + Math.abs(this.rng.nextInt()) % maxSize;
      int xRange = this.maxX - this.minX + 1 - (w - 1);
      int yRange = this.maxY - this.minY + 1 - (h - 1);
      this.roomRectScratch[0] = (short)(this.minX + Math.abs(this.rng.nextInt()) % xRange);
      this.roomRectScratch[1] = (short)(this.minY + Math.abs(this.rng.nextInt()) % yRange);
      this.roomRectScratch[2] = (short)(this.roomRectScratch[0] + (w - 1));
      this.roomRectScratch[3] = (short)(this.roomRectScratch[1] + (h - 1));
      this.roomRectScratch[4] = (short)(this.roomRectScratch[0] + Math.abs(this.rng.nextInt()) % w);
      this.roomRectScratch[5] = (short)(this.roomRectScratch[1] + Math.abs(this.rng.nextInt()) % h);
      return this.roomRectScratch;
   }

   // Succeeds only if every tile in the room's bounding box (+1 tile
   // margin, clamped to [0,34]) is still untouched wall (tile value == 0
   // would mean already-carved floor). On success, carves the room and --
   // if it isn't a single point -- registers it in `rooms`.
   private boolean tryPlaceRoom(short[] room) {
      int x0 = room[0] - 1 >= 0 ? room[0] - 1 : 0;
      int x1 = room[2] + 1 <= 34 ? room[2] + 1 : 34;
      int y0 = room[1] - 1 >= 0 ? room[1] - 1 : 0;
      int y1 = room[3] + 1 <= 34 ? room[3] + 1 : 34;

      for (int x = x0; x <= x1; x++) {
         for (int y = y0; y <= y1; y++) {
            if (this.tiles[x][y] == 0) {
               return false;
            }
         }
      }

      this.carveRect(room[0], room[2] - room[0] + 1, room[1], room[3] - room[1] + 1);
      if (room[2] != room[0] && room[3] != room[1]) {
         short[] saved = new short[6];

         for (int i = 0; i < 6; i++) {
            saved[i] = room[i];
         }

         this.rooms.addElement(saved);
      }

      return true;
   }

   // Nearest-neighbor nearest-unconnected-room heuristic (not a true MST):
   // for every room in allRoomIds, finds the closest room still in
   // unconnectedRoomIds (by distanceSquared) and carves a corridor to it.
   private void connectRooms() {
      int total = this.allRoomIds.size();

      for (int i = 0; i < total; i++) {
         Integer from = (Integer)this.allRoomIds.elementAt(i);
         int poolSize = this.unconnectedRoomIds.size();
         int best = Integer.MAX_VALUE;
         Integer nearest = null;
         int selfIndex = -1;

         for (int j = 0; j < poolSize; j++) {
            Integer candidate = (Integer)this.unconnectedRoomIds.elementAt(j);
            if (!candidate.equals(from)) {
               int d = this.distanceSquared(from, candidate);
               if (d < best) {
                  best = d;
                  nearest = candidate;
               }
            } else {
               selfIndex = j;
            }
         }

         if (nearest != null) {
            this.carveCorridorBetween(from, nearest);
         }

         if (selfIndex != -1) {
            this.unconnectedRoomIds.removeElementAt(selfIndex);
         }
      }
   }

   // Carves a fixed 1-wide stairway corridor from the level border to a
   // point 5 tiles in, in compass direction `dir` (1=N,2=E,3=S,4=W, same
   // convention as neighbors[4]/[5]), and returns the packed room-id of
   // its inner endpoint (NOT yet registered -- the caller does that).
   private int carveStairwell(short dir) {
      int roomId = -1;
      if (dir == 1) {
         this.carveRect(17, 1, 0, 5);
         roomId = this.packXY((short)17, (short)4);
      } else if (dir == 3) {
         this.carveRect(17, 1, 30, 5);
         roomId = this.packXY((short)17, (short)30);
      } else if (dir == 4) {
         this.carveRect(0, 5, 17, 1);
         roomId = this.packXY((short)4, (short)17);
      } else if (dir == 2) {
         this.carveRect(30, 5, 17, 1);
         roomId = this.packXY((short)30, (short)17);
      }

      return roomId;
   }

   // Unpacks both room-ids to (x,y) centers, picks a random L-shape order
   // (horizontal segment first or vertical first), and carves the two
   // connecting rectangle strips.
   private void carveCorridorBetween(int fromId, int toId) {
      short[] from = this.unpackXY(fromId);
      short fx = from[0];
      short fy = from[1];
      from = this.unpackXY(toId);
      short tx = from[0];
      short ty = from[1];
      int horizontalFirst = Math.abs(this.rng.nextInt() % 2);
      if (horizontalFirst == 0) {
         if (tx > fx) {
            this.carveRect(fx, tx - fx + 1, fy, 1);
         } else {
            this.carveRect(tx, fx - tx + 1, fy, 1);
         }

         if (ty > fy) {
            this.carveRect(tx, 1, fy, ty - fy + 1);
         } else {
            this.carveRect(tx, 1, ty, fy - ty + 1);
         }
      } else {
         if (ty > fy) {
            this.carveRect(fx, 1, fy, ty - fy + 1);
         } else {
            this.carveRect(fx, 1, ty, fy - ty + 1);
         }

         if (tx > fx) {
            this.carveRect(fx, tx - fx + 1, ty, 1);
         } else {
            this.carveRect(tx, fx - tx + 1, ty, 1);
         }
      }
   }

   // Clears every tile in the rect to floor(0), EXCEPT tiles already
   // marked with bit 3 (mask 8) -- those are preserved untouched.
   private void carveRect(int x, int w, int y, int h) {
      for (int i = x; i < x + w; i++) {
         for (int j = y; j < y + h; j++) {
            if (this.tiles[i][j] != 8) {
               this.tiles[i][j] = 0;
            }
         }
      }
   }

   private int packXY(short x, short y) {
      return x << 16 | y;
   }

   private short[] unpackXY(int packed) {
      this.unpackScratch[0] = (short)((-65536 & packed) >>> 16);
      this.unpackScratch[1] = (short)(65535 & packed);
      return this.unpackScratch;
   }

   private void registerRoomId(int packed) {
      Integer id = new Integer(packed);
      this.allRoomIds.addElement(id);
      this.unconnectedRoomIds.addElement(id);
   }

   private int distanceSquared(int idA, int idB) {
      short[] a = this.unpackXY(idA);
      short ax = a[0];
      short ay = a[1];
      short[] b = this.unpackXY(idB);
      short bx = b[0];
      short by = b[1];
      return (bx - ax) * (bx - ax) + (by - ay) * (by - ay);
   }

   // Walkable iff not wall(bit0), not monster-occupied(bit1), not the
   // special/reserved bit3 tile, and not blocked(bit5) -- same bit
   // numbering as dawnstar's tile bitflags (wall/monster/droppedItem/
   // special/chest/blocked = bits 0-5).
   boolean isWalkable(int x, int y) {
      byte tile = this.tiles[x][y];
      if (Util.testBit((byte)1, tile)) {
         return false;
      } else if (Util.testBit((byte)2, tile)) {
         return false;
      } else {
         return Util.testBit((byte)8, tile) ? false : !Util.testBit((byte)32, tile);
      }
   }

   // Returns the live Monster at (x,y) if the monster-bit is set, else
   // null -- scans ESGame.monsters[level] (no position index).
   Monster monsterAt(int x, int y) {
      byte tile = this.tiles[x][y];
      if (Util.testBit((byte)1, tile)) {
         return null;
      }

      if (!Util.testBit((byte)2, tile)) {
         return null;
      }

      Enumeration e = ESGame.monsters[this.levelNumber - 1].elements();

      while (e.hasMoreElements()) {
         byte[] data = (byte[])e.nextElement();
         Monster m = Monster.fromBytesShared(data);
         if (m.tileX == x && m.tileY == y) {
            return m;
         }
      }

      return null;
   }

   void removeChest(byte[] record) {
      byte x = record[0];
      byte y = record[1];
      byte tile = this.tiles[x][y];
      if (!Util.testBit((byte)1, tile)) {
         if (Util.testBit((byte)16, tile)) {
            ESGame.chests[this.levelNumber - 1].remove(Util.posKey(x, y));
            this.tiles[x][y] = Util.clearBit((byte)16, this.tiles[x][y]);
         }
      }
   }

   void removeDroppedItem(byte[] record) {
      byte x = record[0];
      byte y = record[1];
      byte tile = this.tiles[x][y];
      if (!Util.testBit((byte)1, tile)) {
         if (Util.testBit((byte)4, tile)) {
            ESGame.droppedItems[this.levelNumber - 1].removeElement(record);
            if (this.countDroppedItemsAt(x, y) == 0) {
               this.tiles[x][y] = Util.clearBit((byte)4, this.tiles[x][y]);
            }
         }
      }
   }

   int countDroppedItemsAt(int x, int y) {
      int count = 0;
      Enumeration e = ESGame.droppedItems[this.levelNumber - 1].elements();

      while (e.hasMoreElements()) {
         byte[] record = (byte[])e.nextElement();
         if (record[0] == x && record[1] == y) {
            count++;
         }
      }

      return count;
   }

   byte[] firstDroppedItemAt(int x, int y) {
      Enumeration e = ESGame.droppedItems[this.levelNumber - 1].elements();

      while (e.hasMoreElements()) {
         byte[] record = (byte[])e.nextElement();
         if (record[0] == x && record[1] == y) {
            return record;
         }
      }

      return null;
   }

   Vector droppedItemsAt(int x, int y) {
      Vector result = new Vector(5);
      Enumeration e = ESGame.droppedItems[this.levelNumber - 1].elements();

      while (e.hasMoreElements()) {
         byte[] record = (byte[])e.nextElement();
         if (record[0] == x && record[1] == y) {
            result.addElement(record);
         }
      }

      return result;
   }

   // Dispatches to sampleView() with a 5-row forward-facing view cone
   // (row widths 1,5,7,9,9) along whichever axis `facing` implies --
   // the corridor 3D-renderer's visibility sample, same role as
   // dawnstar's Dungeon.sampleCorridorView.
   void sampleCorridorView(int x, int y, int facing, byte[][] outGrid) {
      if (facing != 1 && facing != 3) {
         if (facing == 2 || facing == 4) {
            byte step = facing == 2 ? (byte)1 : (byte)-1;
            outGrid[0][0] = this.tileAt(x, y - step);
            outGrid[1][0] = 0;
            outGrid[2][0] = this.tileAt(x, y + step);
            int fx = x + step;

            for (int i = 0; i < 5; i++) {
               outGrid[i][1] = this.tileAt(fx, y + (i - 2) * step);
            }

            fx = x + 2 * step;

            for (int i = 0; i < 7; i++) {
               outGrid[i][2] = this.tileAt(fx, y + (i - 3) * step);
            }

            fx = x + 3 * step;

            for (int i = 0; i < 9; i++) {
               outGrid[i][3] = this.tileAt(fx, y + (i - 4) * step);
            }

            fx = x + 4 * step;

            for (int i = 0; i < 9; i++) {
               outGrid[i][4] = this.tileAt(fx, y + (i - 4) * step);
            }
         }
      } else {
         byte step = facing == 1 ? (byte)1 : (byte)-1;
         outGrid[0][0] = this.tileAt(x - step, y);
         outGrid[1][0] = this.tileAt(x, y);
         outGrid[2][0] = this.tileAt(x + step, y);
         int fy = y - step;

         for (int i = 0; i < 5; i++) {
            outGrid[i][1] = this.tileAt(x + (i - 2) * step, fy);
         }

         fy = y - 2 * step;

         for (int i = 0; i < 7; i++) {
            outGrid[i][2] = this.tileAt(x + (i - 3) * step, fy);
         }

         fy = y - 3 * step;

         for (int i = 0; i < 9; i++) {
            outGrid[i][3] = this.tileAt(x + (i - 4) * step, fy);
         }

         fy = y - 4 * step;

         for (int i = 0; i < 9; i++) {
            outGrid[i][4] = this.tileAt(x + (i - 4) * step, fy);
         }
      }
   }

   void sampleSquareView7(int x, int y, int facing, byte[][] outGrid) {
      this.sampleView(x, y, facing, 7, outGrid);
   }

   // Likely the minimap sampler (17x17).
   void sampleSquareView17(int x, int y, int facing, byte[][] outGrid) {
      this.sampleView(x, y, facing, 17, outGrid);
   }

   // Shared implementation for sampleSquareView7/17: samples wall/special
   // bits (mask 1 or 8) via tileAt() into a `size`x`size` grid centered on
   // (x,y) and rotated for `facing`, then overlays live monster positions
   // (bit 2 in the output, gated on Monster.unconfirmedFlag -- see
   // Monster.java) and chest/dropped-item positions (also bit 4 in the
   // output). In the hub town (levelNumber==1) the "monster" overlay pass
   // is replaced with Shop NPC positions instead (originally k.b[]/k.j[]/
   // k.i[]) -- gated per-NPC on Shop.questRewardClaimable[i], NOT a
   // general visibility/active flag. Surprising but read directly off
   // i.java: the same boolean array Shop.java documents as gating the
   // one-time quest-reward-collection branch (action 6) is also the exact
   // condition an NPC's world-map/view-grid marker is drawn under here.
   // Preserved as found rather than "corrected" to a more sensible-looking
   // gate. Skips the Warden slot (index 6) unless Shop.wardenPresent.
   void sampleView(int x, int y, int facing, int size, byte[][] outGrid) {
      int half = size / 2;
      if (facing == 1 || facing == 3) {
         byte step = facing == 1 ? (byte)1 : (byte)-1;

         for (int row = 0; row < size; row++) {
            for (int col = 0; col < size; col++) {
               outGrid[col][row] = (byte)(this.tileAt(x + (col - half) * step, y + (row - half) * step) & 1);
               if ((outGrid[col][row] & 1) == 0) {
                  outGrid[col][row] = (byte)(this.tileAt(x + (col - half) * step, y + (row - half) * step) & 8);
               }
            }
         }

         if (this.levelNumber > 1) {
            Enumeration me = ESGame.monsters[this.levelNumber - 1].elements();

            while (me.hasMoreElements()) {
               byte[] data = (byte[])me.nextElement();
               Monster m = Monster.fromBytesShared(data);
               int col = step * (m.tileX - x) + half;
               int row = step * (m.tileY - y) + half;
               if (col >= 0 && col < size && row >= 0 && row < size && m.unconfirmedFlag) {
                  outGrid[col][row] = (byte)(outGrid[col][row] | 2);
               }
            }

            Enumeration ce = ESGame.chests[this.levelNumber - 1].elements();

            while (ce.hasMoreElements()) {
               byte[] c = (byte[])ce.nextElement();
               int col = step * (c[0] - x) + half;
               int row = step * (c[1] - y) + half;
               if (col >= 0 && col < size && row >= 0 && row < size) {
                  outGrid[col][row] = (byte)(outGrid[col][row] | 4);
               }
            }

            Enumeration de = ESGame.droppedItems[this.levelNumber - 1].elements();

            while (de.hasMoreElements()) {
               byte[] d = (byte[])de.nextElement();
               int col = step * (d[0] - x) + half;
               int row = step * (d[1] - y) + half;
               boolean visible = (d[6] & 1) != 0;
               if (col >= 0 && col < size && row >= 0 && row < size && visible) {
                  outGrid[col][row] = (byte)(outGrid[col][row] | 4);
               }
            }
         } else {
            for (int i = 0; i < 7 && (i != 6 || Shop.wardenPresent); i++) {
               if (Shop.questRewardClaimable[i]) {
                  int col = step * (Shop.SHOP_X[i] - x) + half;
                  int row = step * (Shop.SHOP_Y[i] - y) + half;
                  if (col >= 0 && col < size && row >= 0 && row < size) {
                     outGrid[col][row] = (byte)(outGrid[col][row] | 4);
                  }
               }
            }
         }
      } else if (facing == 2 || facing == 4) {
         byte step = facing == 2 ? (byte)1 : (byte)-1;

         for (int row = 0; row < size; row++) {
            for (int col = 0; col < size; col++) {
               outGrid[col][row] = (byte)(this.tileAt(x - (row - half) * step, y + (col - half) * step) & 1);
               if ((outGrid[col][row] & 1) == 0) {
                  outGrid[col][row] = (byte)(this.tileAt(x - (row - half) * step, y + (col - half) * step) & 8);
               }
            }
         }

         if (this.levelNumber > 1) {
            Enumeration me = ESGame.monsters[this.levelNumber - 1].elements();

            while (me.hasMoreElements()) {
               byte[] data = (byte[])me.nextElement();
               Monster m = Monster.fromBytesShared(data);
               int col = step * (m.tileY - y) + half;
               int row = half - step * (m.tileX - x);
               if (col >= 0 && col < size && row >= 0 && row < size && m.unconfirmedFlag) {
                  outGrid[col][row] = (byte)(outGrid[col][row] | 2);
               }
            }

            Enumeration ce = ESGame.chests[this.levelNumber - 1].elements();

            while (ce.hasMoreElements()) {
               byte[] c = (byte[])ce.nextElement();
               int col = step * (c[1] - y) + half;
               int row = half - step * (c[0] - x);
               if (col >= 0 && col < size && row >= 0 && row < size) {
                  outGrid[col][row] = (byte)(outGrid[col][row] | 4);
               }
            }

            Enumeration de = ESGame.droppedItems[this.levelNumber - 1].elements();

            while (de.hasMoreElements()) {
               byte[] d = (byte[])de.nextElement();
               int col = step * (d[1] - y) + half;
               int row = half - step * (d[0] - x);
               boolean visible = (d[6] & 1) != 0;
               if (col >= 0 && col < size && row >= 0 && row < size && visible) {
                  outGrid[col][row] = (byte)(outGrid[col][row] | 4);
               }
            }
         } else {
            for (int i = 0; i < 7 && (i != 6 || Shop.wardenPresent); i++) {
               if (Shop.questRewardClaimable[i]) {
                  int col = step * (Shop.SHOP_Y[i] - y) + half;
                  int row = half - step * (Shop.SHOP_X[i] - x);
                  if (col >= 0 && col < size && row >= 0 && row < size) {
                     outGrid[col][row] = (byte)(outGrid[col][row] | 4);
                  }
               }
            }
         }
      }
   }

   // Relative (dx,dy)-in-view-space offset of (toX,toY) relative to
   // (fromX,fromY) given `facing`, offset by +3 (a 7-wide grid, center 3).
   // Writes into and returns viewOffsetScratch.
   int[] relativeViewOffset(int fromX, int fromY, int facing, int toX, int toY) {
      int dx = 0;
      int dy = 0;
      if (facing == 1 || facing == 3) {
         byte step = facing == 1 ? (byte)1 : (byte)-1;
         dx = step * (toX - fromX) + 3;
         dy = step * (toY - fromY) + 3;
      } else if (facing == 2 || facing == 4) {
         byte step = facing == 2 ? (byte)1 : (byte)-1;
         dx = step * (toY - fromY) + 3;
         dy = 3 - step * (toX - fromX);
      }

      this.viewOffsetScratch[0] = dx;
      this.viewOffsetScratch[1] = dy;
      return this.viewOffsetScratch;
   }

   // Cross-level tileAt(): for an out-of-bounds (x,y), follows the
   // appropriate neighbors[] entry into the neighboring Dungeon,
   // recentering coordinates for a hub-town(19x19)<->standard(35x35) size
   // mismatch on either side of the crossing (same approach dawnstar's
   // own tileAt confirms), and returns wall(1) if the neighbor id is <=0
   // or the neighbor hasn't been populated() yet. In-bounds, just reads
   // this level's own tiles[][] directly.
   byte tileAt(int x, int y) {
      int nx = x;
      int ny = y;
      byte neighborId = this.levelNumber;
      Dungeon neighbor = null;
      if (x < 0) {
         neighborId = this.neighbors[3];
         if (neighborId <= 0) {
            return 1;
         }

         neighbor = ESGame.dungeons[neighborId - 1];
         if (neighborId != 1 && this.levelNumber != 1) {
            nx = (byte)(neighbor.width - 1);
         } else {
            nx = (byte)(neighbor.width - 1);
            ny = (byte)(ny + (neighbor.height - this.height) / 2);
         }
      } else if (x >= this.width) {
         neighborId = this.neighbors[1];
         if (neighborId <= 0) {
            return 1;
         }

         neighbor = ESGame.dungeons[neighborId - 1];
         if (neighborId != 1 && this.levelNumber != 1) {
            nx = 0;
         } else {
            nx = 0;
            ny = (byte)(ny + (neighbor.height - this.height) / 2);
         }
      } else if (y < 0) {
         neighborId = this.neighbors[0];
         if (neighborId <= 0) {
            return 1;
         }

         neighbor = ESGame.dungeons[neighborId - 1];
         if (neighborId != 1 && this.levelNumber != 1) {
            ny = (byte)(neighbor.height - 1);
         } else {
            nx = (byte)(nx + (neighbor.width - this.width) / 2);
            ny = (byte)(neighbor.height - 1);
         }
      } else if (y >= this.height) {
         neighborId = this.neighbors[2];
         if (neighborId <= 0) {
            return 1;
         }

         neighbor = ESGame.dungeons[neighborId - 1];
         if (neighborId != 1 && this.levelNumber != 1) {
            ny = 0;
         } else {
            nx = (byte)(nx + (neighbor.width - this.width) / 2);
            ny = 0;
         }
      }

      if (neighborId != this.levelNumber) {
         if (nx < 0 || nx >= neighbor.width) {
            return 1;
         } else if (ny < 0 || ny >= neighbor.height) {
            return 1;
         } else {
            return neighbor.populated ? neighbor.tiles[nx][ny] : 1;
         }
      } else {
         return this.tiles[x][y];
      }
   }

   // The same "strange formula" pattern dawnstar found on Player
   // (Player.lookupUnconfirmedTable) -- here on Dungeon instead, taking
   // an arbitrary pyramid/cone-shaped grid directly rather than a field.
   // Likely a generic reader for the view grids sampleCorridorView()/
   // sampleView() produce, consumed by the not-yet-renamed renderer (`e`).
   byte viewGridAt(int index, int depth, byte[][] grid) {
      return depth < 4 ? grid[index + depth + 1][depth] : grid[index + depth][depth];
   }

   String[] displayNames() {
      return NAMES[this.levelNumber - 1];
   }

   static void loadNames() throws Exception {
      DataInputStream in = Util.openResource("/dungnamesin.dat");
      NAMES = new String[37][2];

      for (int i = 0; i < 37; i++) {
         for (int c = 0; c < 2; c++) {
            NAMES[i][c] = in.readUTF();
         }
      }
   }

   // Resyncs the monster-presence bit in a 9x9 window around (x,y): clears
   // it everywhere in the window (except on wall tiles), then re-sets it
   // for every live monster (from ESGame.monsters[level]) whose position
   // falls inside that same window. Likely called after monster
   // movement/ticks to keep the tile grid's monster bit accurate for
   // nearby tiles without a full refreshTileFlagsFromRegistries() pass.
   void refreshNearbyMonsterFlags(int x, int y) {
      int x0 = Math.max(x - 4, 0);
      int x1 = Math.min(x + 4, this.width - 1);
      int y0 = Math.max(y - 4, 0);
      int y1 = Math.min(y + 4, this.height - 1);

      for (int i = x0; i <= x1; i++) {
         for (int j = y0; j <= y1; j++) {
            byte tile = this.tiles[i][j];
            if (!Util.testBit((byte)1, tile)) {
               this.tiles[i][j] = Util.clearBit((byte)2, tile);
            }
         }
      }

      Hashtable monsters = ESGame.monsters[this.levelNumber - 1];
      if (monsters != null) {
         Enumeration e = monsters.elements();

         while (e.hasMoreElements()) {
            byte[] data = (byte[])e.nextElement();
            byte mx = data[4];
            byte my = data[5];
            if (mx >= x0 && mx <= x1 && my >= y0 && my <= y1) {
               this.tiles[mx][my] = (byte)(this.tiles[mx][my] | 2);
            }
         }
      }
   }
}
