// Renamed from decompiled/i.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (geomin.dat).
//
// `Player` here is the real, already-renamed Player class, and this
// class IS reachable from GameCanvas/Player: ESGame.dungeons[] is typed
// `Dungeon[]` and Player.currentDungeon() returns `Dungeon`, both fixed
// alongside ESGame's own later rename pass (see Player.java's own class
// header note) -- this comment used to say otherwise and is now stale,
// left over from before that pass landed.
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

public class Dungeon {
   // Display name for every non-hub level (index 0 = level 2, "North
   // Creek"). Confirms the world layout: hub town "Dawnstar" (level 1)
   // + 12 zones x 3 levels each.
   static final String[] NAMES = new String[]{
      "Dawnstar",
      "North Creek",
      "North Creek 2",
      "North Creek 3",
      "Ice Spike",
      "Ice Spike 2",
      "Ice Spike 3",
      "Blind Fjord",
      "Blind Fjord 2",
      "Blind Fjord 3",
      "Slipneck Fjord",
      "Slipneck Fjord 2",
      "Slipneck Fjord 3",
      "Troll Pace",
      "Troll Pace 2",
      "Troll Pace 3",
      "Ice Tribe Haven",
      "Ice Tribe Haven 2",
      "Ice Tribe Haven 3",
      "Dawnstar Run",
      "Dawnstar Run 2",
      "Dawnstar Run 3",
      "Massacre Caves",
      "Massacre Caves 2",
      "Massacre Caves 3",
      "Frostheim",
      "Frostheim 2",
      "Frostheim 3",
      "Glacier Run",
      "Glacier Run 2",
      "Glacier Run 3",
      "Troll Hole",
      "Troll Hole 2",
      "Troll Hole 3",
      "Ice Council",
      "Ice Council 2",
      "Ice Council 3"
   };

   // [tierIndex][difficultyBucket 0-3] -> monster type id. tierIndex is
   // Dungeon.tier - 1 (the *permuted* tier value, not the level number --
   // see `tier`'s doc comment).
   static final int[][] MONSTER_TABLE = new int[][]{
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

   // Maps level index (level number - 2, i.e. levels 2..37) to a
   // "difficulty tier" value in [1,36] -- NOT the level number itself,
   // a permutation grouped into 4 bands of 9 (bands don't obviously line
   // up with the 4 zone-groups; exact banding rationale unconfirmed).
   // This is what DungeonGenerator's room-count-weight table and
   // Monster's type/loot tables actually key off, rather than the raw
   // level number.
   static final byte[] DIFFICULTY_TIER_LOOKUP = new byte[]{
      1, 5, 9, 13, 14, 15, 22, 23, 24, 2, 6, 10, 19, 20, 21, 31, 32, 33, 3, 7, 11, 16, 17, 18, 28, 29, 30, 4, 8, 12, 25, 26, 27, 34, 35, 36
   };

   byte number;
   byte tier;
   short width;
   short height;
   byte[][] tiles;
   // Compass direction code (1=N,2=E,3=S,4=W; 0=none) for this level's
   // internal stairway, at the fixed tile position for that direction
   // ((17,5)=N, (30,17)=E, (17,30)=S, (5,17)=W). Distinct from
   // `neighbors[0..3]`, the plain edge-to-edge level transitions --
   // see Monster.isStairwayTile.
   short stairsUpDir;
   short stairsDownDir;
   boolean populated = true;
   // geomin.dat's raw 6-byte row for this level: [northId, eastId,
   // southId, westId, stairsUpDir, stairsDownDir] -- confirmed from
   // tileAt's boundary-crossing logic (x<0 uses neighbors[3], x>=width
   // uses neighbors[1], y<0 uses neighbors[0], y>=height uses
   // neighbors[2], with y increasing southward). Same N=1,E=2,S=3,W=4
   // convention as Player facing / Monster.isStairwayTile.
   byte[] neighbors;
   int[] unused1 = new int[2];
   // Set true by Player.commitMove on every real (non-turn) move that
   // lands on this level -- confirmed write site (matches this class's
   // own doc language calling it a "just visited" marker), but still no
   // confirmed read site anywhere.
   boolean visited = false;

   public Dungeon() {
   }

   // Standard 35x35 level.
   public Dungeon(byte number, byte[] geomRow) {
      this();
      this.number = number;
      this.initTier();
      this.width = 35;
      this.height = 35;
      this.neighbors = geomRow;
      this.stairsUpDir = this.neighbors[4];
      this.stairsDownDir = this.neighbors[5];
   }

   // Level 1 (the hub town): fixed-size, shares a pre-built tile grid
   // (DungeonGenerator's hand-carved 19x19 template) instead of
   // generating one, and marks the 5 fixed hub shop tiles as special
   // (bit 32) on it.
   public Dungeon(byte number, byte[] geomRow, int width, int height, byte[][] sharedTiles) {
      this();
      this.number = number;
      this.initTier();
      this.width = (short)width;
      this.height = (short)height;
      this.tiles = sharedTiles;
      this.neighbors = geomRow;
      this.stairsUpDir = this.neighbors[4];
      this.stairsDownDir = this.neighbors[5];

      for (int i = 0; i < 5; i++) {
         this.tiles[Shop.SHOP_X[i]][Shop.SHOP_Y[i]] = (byte)(this.tiles[Shop.SHOP_X[i]][Shop.SHOP_Y[i]] | 32);
      }

      this.populated = true;
   }

   void initTier() {
      this.tier = 1;
      if (this.number >= 2 && this.number <= 37) {
         this.tier = DIFFICULTY_TIER_LOOKUP[this.number - 2];
      }
   }

   // Spawns `count` random monsters at random walkable positions.
   void populateRandomMonsters(int count) {
      for (int i = 0; i < count; i++) {
         while (!this.trySpawnMonsterNear(ESGame.nextInt(this.width), ESGame.nextInt(this.height), -1)) {
         }
      }
   }

   // Tries to spawn a monster adjacent to (x, y) (checks west, east,
   // north, south, then +3 south as a last resort -- matches the
   // original's exact offsets, the last one is not actually adjacent;
   // preserved as-is rather than "fixed"). `forcedTypeOrSentinel`: 41/42
   // are passed straight through to Monster.spawn as the spawn-id
   // sentinel; any other negative value means "roll a random type for
   // this level's tier"; non-negative forces that exact monster type.
   boolean trySpawnMonsterNear(int x, int y, int forcedTypeOrSentinel) {
      int sentinel = -1;
      int typeArg = forcedTypeOrSentinel;
      if (typeArg == 42 || typeArg == 41) {
         sentinel = typeArg;
         typeArg = this.tier;
      }

      if (typeArg < 0) {
         typeArg = this.tier;
      }

      boolean spawned = false;

      for (int i = 0; i <= 4; i++) {
         int candidateX = x;
         int candidateY = y;
         if (i < 2) {
            candidateX += 2 * i - 1;
         } else {
            candidateY += 2 * i - 5;
         }

         if (this.isWalkable(candidateX, candidateY)) {
            Monster m = Monster.spawn(ESGame.r, typeArg, this.number, sentinel);
            spawned = true;
            m.x = (byte)candidateX;
            m.y = (byte)candidateY;
            m.store();
            this.tiles[m.x][m.y] = Util.setBit((byte)2, this.tiles[m.x][m.y]);
            break;
         }
      }

      return spawned;
   }

   void addDroppedItem(byte[] record) {
      byte x = record[0];
      byte y = record[1];
      ESGame.droppedItems[this.number - 1].addElement(record);
      this.tiles[x][y] = (byte)(this.tiles[x][y] | 4);
   }

   // Rebuilds the monster/chest/dropped-item presence bits on every tile
   // from the live collections (ESGame.monsters/chests/droppedItems) --
   // called after loading a save or returning to a level whose bits may
   // be stale.
   void refreshTileFlags() {
      for (int x = 0; x < this.width; x++) {
         for (int y = 0; y < this.height; y++) {
            this.tiles[x][y] = Util.clearBit((byte)2, this.tiles[x][y]);
            this.tiles[x][y] = Util.clearBit((byte)16, this.tiles[x][y]);
            this.tiles[x][y] = Util.clearBit((byte)4, this.tiles[x][y]);
         }
      }

      Enumeration monsterKeys = ESGame.monsters[this.number - 1].keys();

      while (monsterKeys.hasMoreElements()) {
         String key = (String)monsterKeys.nextElement();
         int comma = key.indexOf(44, 0);
         int x = Integer.parseInt(key.substring(0, comma));
         int y = Integer.parseInt(key.substring(comma + 1));
         this.tiles[x][y] = (byte)(this.tiles[x][y] | 2);
      }

      Hashtable chests = ESGame.chests[this.number - 1];
      if (chests != null) {
         Enumeration chestValues = chests.elements();

         while (chestValues.hasMoreElements()) {
            byte[] chest = (byte[])chestValues.nextElement();
            this.tiles[chest[0]][chest[1]] = (byte)(this.tiles[chest[0]][chest[1]] | 16);
         }
      }

      Enumeration droppedValues = ESGame.droppedItems[this.number - 1].elements();

      while (droppedValues.hasMoreElements()) {
         byte[] item = (byte[])droppedValues.nextElement();
         this.tiles[item[0]][item[1]] = (byte)(this.tiles[item[0]][item[1]] | 4);
      }
   }

   // In-bounds, not a wall, not monster-occupied, not marked no-spawn,
   // not the "blocked" special marker.
   boolean isWalkable(int x, int y) {
      if (x >= 0 && y >= 0 && x < this.width && y < this.height) {
         byte flags = this.tiles[x][y];
         if (Util.testBit((byte)1, flags)) {
            return false;
         } else if (Util.testBit((byte)2, flags)) {
            return false;
         } else {
            return Util.testBit((byte)8, flags) ? false : !Util.testBit((byte)32, flags);
         }
      } else {
         return false;
      }
   }

   // Per-tick monster AI driver: scans every tile within Manhattan
   // distance 3 of the player, running Monster.tick (attack attempt) on
   // adjacent (distance 1) monsters and Monster.chase (one step toward
   // the player) on distance-2/3 monsters. Returns bit flags: 2 = a
   // monster attacked, 1 = a monster moved.
   byte tickNearbyMonsters(long now, Player player) {
      byte px = player.tileX;
      byte py = player.tileY;
      boolean attacked = false;
      boolean moved = false;
      Monster scratch = new Monster();
      Object record = null;

      for (int dx = -3; dx < 4; dx++) {
         if (px + dx >= 0 && px + dx < this.width) {
            for (int dy = -3; dy < 4; dy++) {
               if (py + dy >= 0 && py + dy < this.height) {
                  int dist = (dx >= 0 ? dx : -dx) + (dy >= 0 ? dy : -dy);
                  if (dist != 0 && dist <= 3) {
                     byte flags = this.tiles[px + dx][py + dy];
                     if (Util.testBit((byte)2, flags)) {
                        record = ESGame.monsters[this.number - 1].get(Util.posKey(px + dx, py + dy));
                        if (record != null) {
                           Monster.fromBytes(scratch, (byte[])record);
                           if (dist == 1) {
                              if (scratch.tick(player, now)) {
                                 attacked = true;
                              }

                              scratch.store();
                           } else if (scratch.chase(px, py)) {
                              moved = true;
                           }
                        }
                     }
                  }
               }
            }
         }
      }

      byte result = 0;
      if (attacked) {
         result = (byte)(result + 2);
      }

      if (moved) {
         result++;
      }

      return result;
   }

   // Clears a chest from the world once its contents are fully looted.
   void removeChest(byte[] record) {
      byte x = record[0];
      byte y = record[1];
      byte flags = this.tiles[x][y];
      if (!Util.testBit((byte)1, flags)) {
         if (Util.testBit((byte)16, flags)) {
            ESGame.chests[this.number - 1].remove(Util.posKey(x, y));
            this.tiles[x][y] = Util.clearBit((byte)16, this.tiles[x][y]);
         }
      }
   }

   void removeDroppedItem(byte[] record) {
      byte x = record[0];
      byte y = record[1];
      byte flags = this.tiles[x][y];
      if (!Util.testBit((byte)1, flags)) {
         if (Util.testBit((byte)4, flags)) {
            ESGame.droppedItems[this.number - 1].removeElement(record);
         }
      }
   }

   void clearDroppedItemFlag(int x, int y) {
      this.tiles[x][y] = Util.clearBit((byte)4, this.tiles[x][y]);
   }

   Vector droppedItemsAt(int x, int y) {
      Vector result = new Vector(5);
      Enumeration items = ESGame.droppedItems[this.number - 1].elements();

      while (items.hasMoreElements()) {
         byte[] item = (byte[])items.nextElement();
         if (item[0] == x && item[1] == y) {
            result.addElement(item);
         }
      }

      return result;
   }

   // Widening-diamond sample of tile bits ahead of (x, y) facing
   // `direction`, for the first-person corridor renderer's per-column
   // occlusion test. `out` rows widen 3,5,7,9,9 with distance.
   void sampleCorridorView(int x, int y, int direction, byte[][] out) {
      if (direction != 1 && direction != 3) {
         if (direction == 2 || direction == 4) {
            byte sign = -1;
            if (direction == 2) {
               sign = 1;
            }

            out[0][0] = this.tileAt(x, y - sign);
            out[1][0] = this.tileAt(x, y);
            out[2][0] = this.tileAt(x, y + sign);
            int fx = x + sign;

            for (int i = 0; i < 5; i++) {
               out[i][1] = this.tileAt(fx, y + (i - 2) * sign);
            }

            fx = x + 2 * sign;

            for (int i = 0; i < 7; i++) {
               out[i][2] = this.tileAt(fx, y + (i - 3) * sign);
            }

            fx = x + 3 * sign;

            for (int i = 0; i < 9; i++) {
               out[i][3] = this.tileAt(fx, y + (i - 4) * sign);
            }

            fx = x + 4 * sign;

            for (int i = 0; i < 9; i++) {
               out[i][4] = this.tileAt(fx, y + (i - 4) * sign);
            }
         }
      } else {
         byte sign = -1;
         if (direction == 1) {
            sign = 1;
         }

         out[0][0] = this.tileAt(x - sign, y);
         out[1][0] = this.tileAt(x, y);
         out[2][0] = this.tileAt(x + sign, y);
         int fy = y - sign;

         for (int i = 0; i < 5; i++) {
            out[i][1] = this.tileAt(x + (i - 2) * sign, fy);
         }

         fy = y - 2 * sign;

         for (int i = 0; i < 7; i++) {
            out[i][2] = this.tileAt(x + (i - 3) * sign, fy);
         }

         fy = y - 3 * sign;

         for (int i = 0; i < 9; i++) {
            out[i][3] = this.tileAt(x + (i - 4) * sign, fy);
         }

         fy = y - 4 * sign;

         for (int i = 0; i < 9; i++) {
            out[i][4] = this.tileAt(x + (i - 4) * sign, fy);
         }
      }
   }

   // Fixed size x size square sample centered ahead of (x, y) facing
   // `direction`, used for the minimap. Also flags monster presence
   // (bit 2 in the output) only when the monster's own `flag` field is
   // set (unconfirmed exact meaning -- see CLASS_MAP.md).
   void sampleSquareView(int x, int y, int direction, int size, byte[][] out) {
      int half = size / 2;
      Monster scratch = new Monster();
      if (direction != 1 && direction != 3) {
         if (direction == 2 || direction == 4) {
            byte sign = direction == 2 ? (byte)1 : (byte)-1;

            for (int row = 0; row < size; row++) {
               for (int col = 0; col < size; col++) {
                  byte tile = this.tileAt(x - (row - half) * sign, y + (col - half) * sign);
                  out[col][row] = (byte)(tile & 1);
                  if ((out[col][row] & 1) == 0) {
                     if ((tile & 4) == 0 && (tile & 16) == 0 && (tile & 32) == 0) {
                        out[col][row] = (byte)(tile & 8);
                     } else {
                        out[col][row] = (byte)(out[col][row] | 4);
                     }

                     if ((tile & 2) != 0) {
                        byte[] record = (byte[])ESGame.monsters[this.number - 1]
                           .get(Util.posKey(x - (row - half) * sign, y + (col - half) * sign));
                        if (record != null) {
                           Monster.fromBytes(scratch, record);
                           if (scratch.flag) {
                              out[col][row] = (byte)(out[col][row] | 2);
                           }
                        }
                     }
                  }
               }
            }
         }
      } else {
         byte sign = direction == 1 ? (byte)1 : (byte)-1;

         for (int row = 0; row < size; row++) {
            for (int col = 0; col < size; col++) {
               byte tile = this.tileAt(x + (col - half) * sign, y + (row - half) * sign);
               out[col][row] = (byte)(tile & 1);
               if ((out[col][row] & 1) == 0) {
                  if ((tile & 4) == 0 && (tile & 16) == 0 && (tile & 32) == 0) {
                     out[col][row] = (byte)(tile & 8);
                  } else {
                     out[col][row] = (byte)(out[col][row] | 4);
                  }

                  if ((tile & 2) != 0) {
                     byte[] record = (byte[])ESGame.monsters[this.number - 1]
                        .get(Util.posKey(x + (col - half) * sign, y + (row - half) * sign));
                     if (record != null) {
                        Monster.fromBytes(scratch, record);
                        if (scratch.flag) {
                           out[col][row] = (byte)(out[col][row] | 2);
                        }
                     }
                  }
               }
            }
         }
      }
   }

   // Tile lookup that transparently crosses into a neighboring level's
   // own grid when (x, y) falls outside this level's bounds, using
   // `neighbors[0..3]` (west/east/north/south level ids). Returns 1
   // ("wall") for an out-of-bounds request with no neighbor in that
   // direction, or 64 ("edge marker") for the exact boundary tile one
   // step before an unpopulated neighbor level.
   byte tileAt(int x, int y) {
      byte neighborLevel = this.number;
      boolean atEdgeMarker = false;
      Dungeon neighbor = null;
      if (x < 0) {
         neighborLevel = this.neighbors[3];
         if (neighborLevel <= 0) {
            return 1;
         }

         neighbor = ESGame.dungeons[neighborLevel - 1];
         x = (byte)(neighbor.width + x);
         if (neighborLevel == 1 || this.number == 1) {
            y = (byte)(y + (neighbor.height - this.height) / 2);
            if (x == neighbor.width - 2) {
               atEdgeMarker = true;
            }
         }
      } else if (x >= this.width) {
         neighborLevel = this.neighbors[1];
         if (neighborLevel <= 0) {
            return 1;
         }

         neighbor = ESGame.dungeons[neighborLevel - 1];
         x -= this.width;
         if (neighborLevel == 1 || this.number == 1) {
            y = (byte)(y + (neighbor.height - this.height) / 2);
            if (x == 1) {
               atEdgeMarker = true;
            }
         }
      } else if (y < 0) {
         neighborLevel = this.neighbors[0];
         if (neighborLevel <= 0) {
            return 1;
         }

         neighbor = ESGame.dungeons[neighborLevel - 1];
         y = (byte)(neighbor.height + y);
         if (neighborLevel == 1 || this.number == 1) {
            x = (byte)(x + (neighbor.width - this.width) / 2);
            if (y == neighbor.height - 2) {
               atEdgeMarker = true;
            }
         }
      } else if (y >= this.height) {
         neighborLevel = this.neighbors[2];
         if (neighborLevel <= 0) {
            return 1;
         }

         neighbor = ESGame.dungeons[neighborLevel - 1];
         y -= this.height;
         if (neighborLevel == 1 || this.number == 1) {
            x = (byte)(x + (neighbor.width - this.width) / 2);
            if (y == 1) {
               atEdgeMarker = true;
            }
         }
      }

      if (neighborLevel != this.number) {
         if (x < 0 || x >= neighbor.width) {
            return 1;
         }

         if (y >= 0 && y < neighbor.height) {
            if (atEdgeMarker && neighbor.tiles[x][y] == 0) {
               return 64;
            } else {
               return neighbor.populated ? neighbor.tiles[x][y] : 1;
            }
         } else {
            return 1;
         }
      } else {
         return this.tiles[x][y];
      }
   }

   String displayName() {
      return NAMES[this.number - 1];
   }
}
