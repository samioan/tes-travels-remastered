// Renamed from decompiled/c.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (geomin.dat).
import java.io.DataInputStream;
import java.util.Random;
import java.util.Vector;

public class DungeonGenerator {
   private byte[][] geomRows;

   // Coarse 1/3/5 difficulty class per tier (indexed by Dungeon.tier-1),
   // reused inside placeChests as the guaranteed first chest's
   // gift-item subtype selector. NOT a room count despite the original
   // shape suggesting one -- populateLevel always tries for exactly 15
   // rooms regardless of this table.
   static final int[] ZONE_TIER_CLASS = new int[]{
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 3, 1, 3, 3, 1, 3, 3, 3, 3, 5, 3, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5
   };

   Vector connectedSet;
   Vector unconnectedSet;
   Vector roomList;
   Random rng;
   static int hubWidth;
   static int hubHeight;
   static byte[][] hubGrid;

   public DungeonGenerator(Dungeon[] dungeons, LoadingScreen progress) {
      this.buildHubGrid();

      try {
         this.loadGeomRows();
      } catch (Exception e) {
      }

      dungeons[0] = new Dungeon((byte)1, this.geomRows[0], hubWidth, hubHeight, hubGrid);

      for (int i = 1; i < 37; i++) {
         dungeons[i] = new Dungeon((byte)(i + 1), this.geomRows[i]);
         this.populateLevel(dungeons[i]);
         progress.percent = 60 + i;
         System.gc();
      }
   }

   // Hand-carved 19x19 tile template for level 1 (the hub town) --
   // starts as all walls, then specific corridors and rooms are cleared.
   // Not randomly generated, unlike every other level.
   private void buildHubGrid() {
      hubWidth = 19;
      hubHeight = 19;
      hubGrid = new byte[hubHeight][hubWidth];

      for (int x = 0; x < hubHeight; x++) {
         for (int y = 0; y < hubWidth; y++) {
            hubGrid[x][y] = 1;
         }
      }

      for (int x = 0; x < hubWidth; x++) {
         hubGrid[x][9] = 0;
      }

      for (int y = 0; y < hubHeight; y++) {
         hubGrid[9][y] = 0;
      }

      for (int i = 4; i < 15; i++) {
         hubGrid[4][i] = 0;
         hubGrid[14][i] = 0;
      }

      for (int i = 4; i < 15; i++) {
         hubGrid[i][4] = 0;
         hubGrid[i][14] = 0;
      }

      hubGrid[5][6] = 0;
      hubGrid[6][6] = 0;
      hubGrid[7][6] = 0;
      hubGrid[7][5] = 0;
      hubGrid[7][7] = 0;
      hubGrid[12][6] = 0;
      hubGrid[13][6] = 0;
      hubGrid[12][8] = 0;
      hubGrid[8][8] = 0;
      hubGrid[10][8] = 0;
      hubGrid[8][10] = 0;
      hubGrid[10][10] = 0;
      hubGrid[11][12] = 0;
      hubGrid[11][13] = 0;
      hubGrid[12][12] = 0;
      hubGrid[5][11] = 0;
      hubGrid[6][11] = 0;
   }

   private void loadGeomRows() throws Exception {
      DataInputStream in = ESGame.getResource("geomin.dat");
      this.geomRows = new byte[37][6];

      for (int level = 0; level < 37; level++) {
         for (int col = 0; col < 6; col++) {
            this.geomRows[level][col] = in.readByte();
         }
      }

      in.close();
   }

   // The room/corridor/loot generator for one (non-hub) level.
   public void populateLevel(Dungeon level) {
      long seed = level.number * 8000;
      level.tiles = new byte[level.width][level.height];
      level.stairsUpDir = level.neighbors[4];
      level.stairsDownDir = level.neighbors[5];
      this.roomList = new Vector();
      int stairwayRoom = -1;
      this.rng = new Random(seed);

      for (int x = 0; x < 35; x++) {
         for (int y = 0; y < 35; y++) {
            level.tiles[x][y] = 1;
         }
      }

      this.connectedSet = new Vector();
      this.unconnectedSet = new Vector();
      if (this.isCardinalDirection(level.stairsUpDir)) {
         stairwayRoom = this.carveStairwayCorridor(level, level.stairsUpDir);
         if (stairwayRoom >= 0) {
            this.registerRoom(stairwayRoom);
         }
      }

      if (this.isCardinalDirection(level.stairsDownDir)) {
         stairwayRoom = this.carveStairwayCorridor(level, level.stairsDownDir);
         if (stairwayRoom >= 0) {
            this.registerRoom(stairwayRoom);
         }
      }

      boolean pickedSpecialRoom = false;
      int placed = 0;

      while (placed < 15) {
         short[] rect = this.randomRoomRect();
         if (this.tryPlaceRoom(rect, level)) {
            placed++;
            int doorCoord = this.packCoord(rect[4], rect[5]);
            this.registerRoom(doorCoord);
            // The 2nd successfully-placed room becomes this level's one
            // "special" room: on the 4 key levels (3/12/21/30 -- by
            // LEVEL NUMBER, not tier) its center tile is marked bit 32
            // and recorded as that key level's shopkeeper position
            // (Shop.SHOP_X/SHOP_Y[5..8]); on every other level it's
            // just marked bit 8 (no-spawn) instead.
            if (placed >= 2 && !pickedSpecialRoom) {
               short doorX = rect[4];
               short doorY = rect[5];
               short w = (short)(rect[2] - rect[0] + 1);
               short h = (short)(rect[3] - rect[1] + 1);
               if (w >= 3 && h >= 3) {
                  short centerX = (short)(rect[0] + w / 2);
                  short centerY = (short)(rect[1] + h / 2);
                  if (centerX != doorX || centerY != doorY) {
                     if (level.number == 3) {
                        level.tiles[centerX][centerY] = (byte)(level.tiles[centerX][centerY] | 32);
                        Shop.SHOP_X[5] = (byte)centerX;
                        Shop.SHOP_Y[5] = (byte)centerY;
                     } else if (level.number == 12) {
                        level.tiles[centerX][centerY] = (byte)(level.tiles[centerX][centerY] | 32);
                        Shop.SHOP_X[6] = (byte)centerX;
                        Shop.SHOP_Y[6] = (byte)centerY;
                     } else if (level.number == 21) {
                        level.tiles[centerX][centerY] = (byte)(level.tiles[centerX][centerY] | 32);
                        Shop.SHOP_X[7] = (byte)centerX;
                        Shop.SHOP_Y[7] = (byte)centerY;
                     } else if (level.number == 30) {
                        level.tiles[centerX][centerY] = (byte)(level.tiles[centerX][centerY] | 32);
                        Shop.SHOP_X[8] = (byte)centerX;
                        Shop.SHOP_Y[8] = (byte)centerY;
                     } else {
                        level.tiles[centerX][centerY] = (byte)(level.tiles[centerX][centerY] | 8);
                     }

                     pickedSpecialRoom = true;
                  }
               }
            }
         }
      }

      this.connectRooms(level);
      int roomCount = this.roomList.size();

      // One monster spawned in every room, at its door position.
      Monster m = null;

      for (int i = 0; i < roomCount; i++) {
         short[] room = (short[])this.roomList.elementAt(i);
         m = Monster.spawn(this.rng, level.tier, level.number, -1);
         m.x = (byte)room[4];
         m.y = (byte)room[5];
         level.tiles[m.x][m.y] = (byte)(level.tiles[m.x][m.y] | 2);
         m.store();
      }

      System.gc();
      roomCount = this.roomList.size();
      int[] roomIndex = new int[roomCount];
      int[] roomWeight = new int[roomCount];

      for (int i = 0; i < roomCount; i++) {
         roomIndex[i] = i;
         roomWeight[i] = Util.randomInt(this.rng, 1000);
      }

      // Insertion sort, descending by roomWeight.
      for (int i = 1; i < roomCount; i++) {
         int w = roomWeight[i];
         int idx = roomIndex[i];
         int j;
         for (j = i - 1; j >= 0 && roomWeight[j] < w; j--) {
            roomWeight[j + 1] = roomWeight[j];
            roomIndex[j + 1] = roomIndex[j];
         }

         roomWeight[j + 1] = w;
         roomIndex[j + 1] = idx;
      }

      int[] top5 = new int[5];

      for (int i = 0; i < 5; i++) {
         top5[i] = roomIndex[i];
      }

      this.placeChests(top5, level);
      this.roomList = null;
      this.connectedSet = null;
      this.unconnectedSet = null;
      System.gc();
   }

   // Places exactly 5 chests, one in each of the 5 highest-(random-)weighted
   // rooms. The first is guaranteed to contain a "gift"-category item of
   // subtype ZONE_TIER_CLASS[level.tier-1]; the rest get a regular 2-roll
   // loot pick.
   void placeChests(int[] topRooms, Dungeon level) {
      int itemId = 0;
      int subtype = ZONE_TIER_CLASS[level.tier - 1];
      boolean first = true;

      for (int i = 0; i < 5; i++) {
         int roomIdx = topRooms[i];
         short[] room = (short[])this.roomList.elementAt(roomIdx);
         if (first) {
            itemId = Item.randomGiftItemOfSubtype(this.rng, subtype);
            first = false;
         } else {
            itemId = Item.rollLoot(this.rng, level.tier, 2);
         }

         short w = (short)(room[2] - room[0] + 1);
         short h = (short)(room[3] - room[1] + 1);
         short x = (short)(room[0] + Math.abs(this.rng.nextInt() % w));
         short y;
         // Re-rolls while landing on a tile that has BOTH bit 8 and bit
         // 32 set -- preserved exactly as in the original, though no
         // tile ever has both bits set simultaneously (the special-room
         // marking above always sets at most one), so this loop body
         // never actually re-rolls in practice. Likely meant `||`.
         for (y = (short)(room[1] + Math.abs(this.rng.nextInt() % h));
            (level.tiles[x][y] & 8) != 0 && (level.tiles[x][y] & 32) != 0;
            y = (short)(room[1] + Math.abs(this.rng.nextInt() % h))
         ) {
            x = (short)(room[0] + Math.abs(this.rng.nextInt() % w));
         }

         byte[] chest = new byte[8];
         chest[0] = (byte)x;
         chest[1] = (byte)y;
         chest[2] = (byte)(first ? 1 : 0);
         byte packed = (byte)(Math.abs(this.rng.nextInt() % 3) << 6);
         chest[3] = (byte)(packed | level.tier);
         byte low = (byte)(itemId & 0xFF);
         byte high = 0;
         if (low == 86) {
            high = (byte)(itemId >>> 8 & 0xFF);
         }

         chest[4] = low;
         chest[7] = high;
         short spawnId = Item.nextSpawnId();
         low = (byte)(spawnId >>> 8 & 0xFF);
         high = (byte)(spawnId & 0xFF);
         chest[5] = low;
         chest[6] = high;
         byte cx = chest[0];
         byte cy = chest[1];
         ESGame.chests[level.number - 1].put(Util.posKey(cx, cy), chest);
         level.tiles[cx][cy] = (byte)(level.tiles[cx][cy] | 16);
      }
   }

   // Random room in [3,31]x[3,31] with width/height in [2,5] and a
   // random interior "door" point. Returns [x0, y0, x1, y1, doorX, doorY].
   private short[] randomRoomRect() {
      short[] rect = new short[6];
      byte origin = 3;
      byte max = 31;
      byte sizeRange = 4;
      int w = 2 + Math.abs(this.rng.nextInt()) % sizeRange;
      int h = 2 + Math.abs(this.rng.nextInt()) % sizeRange;
      int xSpan = max - origin + 1 - (w - 1);
      int ySpan = max - origin + 1 - (h - 1);
      rect[0] = (short)(origin + Math.abs(this.rng.nextInt()) % xSpan);
      rect[1] = (short)(origin + Math.abs(this.rng.nextInt()) % ySpan);
      rect[2] = (short)(rect[0] + (w - 1));
      rect[3] = (short)(rect[1] + (h - 1));
      rect[4] = (short)(rect[0] + Math.abs(this.rng.nextInt()) % w);
      rect[5] = (short)(rect[1] + Math.abs(this.rng.nextInt()) % h);
      return rect;
   }

   // Rejects (returns false) if any tile in `rect`, expanded by a 1-tile
   // margin, is already carved (non-wall). Otherwise carves it and, if
   // non-degenerate, records it in roomList.
   private boolean tryPlaceRoom(short[] rect, Dungeon level) {
      int x0 = rect[0] - 1 >= 0 ? rect[0] - 1 : 0;
      int x1 = rect[2] + 1 <= 34 ? rect[2] + 1 : 34;
      int y0 = rect[1] - 1 >= 0 ? rect[1] - 1 : 0;
      int y1 = rect[3] + 1 <= 34 ? rect[3] + 1 : 34;

      for (int x = x0; x <= x1; x++) {
         for (int y = y0; y <= y1; y++) {
            if (level.tiles[x][y] == 0) {
               return false;
            }
         }
      }

      this.carveRect(level, rect[0], (short)(rect[2] - rect[0] + 1), rect[1], (short)(rect[3] - rect[1] + 1));
      if (rect[2] != rect[0] && rect[3] != rect[1]) {
         short[] copy = new short[6];

         for (int i = 0; i < 6; i++) {
            copy[i] = rect[i];
         }

         this.roomList.addElement(copy);
      }

      return true;
   }

   // Nearest-neighbor-chain corridor connection: for each registered
   // room (in registration order), finds its closest still-unconnected
   // room and carves a corridor to it. Not a true MST.
   private void connectRooms(Dungeon level) {
      int total = this.connectedSet.size();

      for (int i = 0; i < total; i++) {
         Integer from = (Integer)this.connectedSet.elementAt(i);
         int poolSize = this.unconnectedSet.size();
         int best = Integer.MAX_VALUE;
         Integer nearest = null;
         int selfIndex = -1;

         for (int j = 0; j < poolSize; j++) {
            Integer candidate = (Integer)this.unconnectedSet.elementAt(j);
            if (!candidate.equals(from)) {
               int dist = this.squaredDistance(from, candidate);
               if (dist < best) {
                  best = dist;
                  nearest = candidate;
               }
            } else {
               selfIndex = j;
            }
         }

         if (nearest != null) {
            this.carveCorridorBetween(level, from, nearest);
         }

         if (selfIndex != -1) {
            this.unconnectedSet.removeElementAt(selfIndex);
         }
      }
   }

   // Carves the fixed-position stairway corridor for compass direction
   // `dir` (1=N,2=E,3=S,4=W) and returns the packed coordinate of its
   // inner endpoint, for room-connection purposes.
   private int carveStairwayCorridor(Dungeon level, short dir) {
      int endpoint = -1;
      if (dir == 1) {
         this.carveRect(level, 17, 1, 0, 5);
         endpoint = this.packCoord((short)17, (short)4);
      } else if (dir == 3) {
         this.carveRect(level, 17, 1, 30, 5);
         endpoint = this.packCoord((short)17, (short)30);
      } else if (dir == 4) {
         this.carveRect(level, 0, 5, 17, 1);
         endpoint = this.packCoord((short)4, (short)17);
      } else if (dir == 2) {
         this.carveRect(level, 30, 5, 17, 1);
         endpoint = this.packCoord((short)30, (short)17);
      }

      return endpoint;
   }

   // L-shaped corridor between two packed coordinates, orientation
   // (horizontal-then-vertical vs. vertical-then-horizontal) chosen
   // randomly.
   private void carveCorridorBetween(Dungeon level, int fromPacked, int toPacked) {
      short[] from = this.unpackCoord(fromPacked);
      short fx = from[0];
      short fy = from[1];
      short[] to = this.unpackCoord(toPacked);
      short tx = to[0];
      short ty = to[1];
      int orientation = Math.abs(this.rng.nextInt() % 2);
      if (orientation == 0) {
         if (tx > fx) {
            this.carveRect(level, fx, tx - fx + 1, fy, 1);
         } else {
            this.carveRect(level, tx, fx - tx + 1, fy, 1);
         }

         if (ty > fy) {
            this.carveRect(level, tx, 1, fy, ty - fy + 1);
         } else {
            this.carveRect(level, tx, 1, ty, fy - ty + 1);
         }
      } else {
         if (ty > fy) {
            this.carveRect(level, fx, 1, fy, ty - fy + 1);
         } else {
            this.carveRect(level, fx, 1, ty, fy - ty + 1);
         }

         if (tx > fx) {
            this.carveRect(level, fx, tx - fx + 1, ty, 1);
         } else {
            this.carveRect(level, tx, fx - tx + 1, ty, 1);
         }
      }
   }

   // Clears the wall bit over a w x h rectangle at (x, y), preserving
   // the bit-8/bit-32 special-room markers.
   private void carveRect(Dungeon level, int x, int w, int y, int h) {
      for (int i = x; i < x + w; i++) {
         for (int j = y; j < y + h; j++) {
            if (level.tiles[i][j] != 8 && level.tiles[i][j] != 32) {
               level.tiles[i][j] = 0;
            }
         }
      }
   }

   private int squaredDistance(int packedA, int packedB) {
      short[] a = this.unpackCoord(packedA);
      short ax = a[0];
      short ay = a[1];
      short[] b = this.unpackCoord(packedB);
      short bx = b[0];
      short by = b[1];
      return (bx - ax) * (bx - ax) + (by - ay) * (by - ay);
   }

   private int packCoord(short x, short y) {
      return x << 16 | y;
   }

   private short[] unpackCoord(int packed) {
      return new short[]{(short)((-65536 & packed) >>> 16), (short)(65535 & packed)};
   }

   private void registerRoom(int packedCoord) {
      Integer coord = new Integer(packedCoord);
      this.connectedSet.addElement(coord);
      this.unconnectedSet.addElement(coord);
   }

   boolean isCardinalDirection(short dir) {
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
}
