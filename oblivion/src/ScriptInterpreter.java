/**
 * Renamed from decompiled/e.java (see docs/CLASS_MAP.md, docs/rename.map).
 * Loads a level's .scr resource into its record tables and interprets the
 * bytecode that follows them (a call stack of program counters, so scripts
 * may CALL each other). Game (`b`) is not renamed yet; members used here are
 * mapped in docs/rename.map.
 */
public final class ScriptInterpreter {
   // .scr bytecode opcodes (see docs/SCR_OPCODES.md for operand layouts)
   public static final int OP_INVALID0 = 0;
   public static final int OP_INVALID1 = 1;
   public static final int OP_NOP_B = 2;
   public static final int OP_SAY = 3;
   public static final int OP_SET_SCREEN_SIZE = 4;
   public static final int OP_NOP_B2 = 5;
   public static final int OP_NOP_B3 = 6;
   public static final int OP_SET_PLAYER_COLLIDES = 7;
   public static final int OP_LOAD_MAP = 8;
   public static final int OP_SKIP_STRING = 9;
   public static final int OP_END_LEVEL = 10;
   public static final int OP_WAIT = 11;
   public static final int OP_SET_STATE_PLAYING = 12;
   public static final int OP_NOP_B4 = 13;
   public static final int OP_SET_KEY_HOOK = 14;
   public static final int OP_SPAWN_ACTOR = 15;
   public static final int OP_SET_TRIGGER = 16;
   public static final int OP_MOVE_ACTOR_TO = 17;
   public static final int OP_SET_TILE = 18;
   public static final int OP_SET_INPUT_ENABLED = 19;
   public static final int OP_REMOVE_ACTOR = 20;
   public static final int OP_WAIT_ACTORS_STOP = 21;
   public static final int OP_SET_COLLISION = 22;
   public static final int OP_CALL = 23;
   public static final int OP_SET_ANIM_STATE = 24;
   public static final int OP_CAMERA_TO = 25;
   public static final int OP_CAMERA_FOLLOW = 26;
   public static final int OP_CLEAR_TRIGGER = 27;
   public static final int OP_CLEAR_KEY_HOOK = 28;
   public static final int OP_LOAD_LEVEL = 29;
   public static final int OP_SET_DEATH_SCRIPT = 32;
   public static final int OP_CLEAR_DEATH_SCRIPT = 33;
   public static final int OP_SET_STAT = 34;
   public static final int OP_NOP_B5 = 35;
   public static final int OP_SET_POSITION = 36;
   public static final int OP_GIVE_ITEM = 37;
   public static final int OP_REMOVE_ITEM = 38;
   public static final int OP_SHOW_MESSAGE = 39;
   public static final int OP_HIDE_MESSAGE = 40;
   public static final int OP_MOVE_ACTOR_X = 41;
   public static final int OP_MOVE_ACTOR_Y = 42;
   public static final int OP_LOAD_HUD_SPRITES = 43;
   public static final int OP_OPEN_MENU = 44;
   public static final int OP_OPEN_SHOP_MENU = 45;
   public static final int OP_SET_STATUS_ICON = 46;
   public static final int OP_GENERATE_DUNGEON = 47;
   public static final int OP_CLEAR_LAYERS = 48;
   public static final int OP_PLACE_ITEM = 49;
   public static final int OP_SET_TRIGGER_RECT = 50;
   public static final int OP_CLEAR_TRIGGER_RECT = 51;
   public static final int OP_WALK_CUTSCENE = 52;
   public static final int OP_TALK = 53;
   public static final int OP_NOP54 = 54;
   public static final int OP_NOP55 = 55;
   public static final int OP_LOAD_LANG = 56;
   public static final int OP_NOP57 = 57;
   public static final int OP_SCALE_MONSTER = 58;
   public static final int OP_SET_DROPS_LOOT = 59;
   public static final int OP_WAIT_KEY = 60;
   public static final int OP_SET_STATE_9 = 61;
   public static final int OP_NOP62 = 62;
   public static final int OP_NOP63 = 63;
   public static final int OP_SET_BACKGROUND_COLOR = 64;
   public static final int OP_LEVEL_UP_TO = 65;
   public static final int OP_SHOW_TEXT_SCREEN = 66;
   public static final int OP_RESTORE_MONSTER_TYPE = 67;
   public static final int OP_SPAWN_PROJECTILE = 68;
   public static final int OP_SPAWN_TIMED_PROJECTILE = 69;
   public static final int OP_CLEAR_PROJECTILE_AT = 70;
   public static final int OP_SET_POINT = 71;
   public static final int OP_EVICT_SPRITES = 72;
   public static final int OP_BEGIN_FADE = 73;
   public static final int OP_END_FADE = 74;
   public static final int OP_TOGGLE_INVULNERABLE = 75;
   public static final int OP_SET_HUD_VISIBLE = 76;
   public static final int OP_SET_STATE_4 = 77;
   public static final int OP_SET_AI_ACTIVE = 78;

   private static final byte[][] archetypeGrowth = new byte[][]{{3, 1, 1, 2, 0, 1, 1}, {2, 2, 2, 1, 0, 1, 1}, {1, 3, 3, 1, 0, 2, 3}};
   private int[] pcStack = new int[10];
   private int[] scriptStack = new int[10];
   private int[] keyHooks = new int[8];
   private int[] scriptOffsets = new int[256];
   private int[] waitActors = null;
   private int[] pairTable = new int[100];
   private int[] spawnIds = new int[10];
   private int[] walkTarget = null;
   public int[][] monsterTypes = new int[25][21];
   public int[][] monsterTypesBackup = new int[25][21];
   public int[][] weapons = new int[37][8];
   public int[][] armors = new int[42][10];
   public int[][] consumables = new int[11][14];
   public int[][] spawnGroups = new int[10][21];
   public int[][] table6 = new int[25][7];
   public int[][] classBase = new int[9][15];
   public int[][] classItemTypes = new int[9][15];
   public int[][] classLists = new int[9][15];
   public int[][] specials = new int[10][15];
   public int[][] lootTable = new int[30][4];
   private int depth = 0;
   private int waitElapsed = 0;
   private int waitDuration = -1;
   private int stringCount = 0;
   private int spawnIdCount = 0;
   private b game = null;
   private String[] strings = new String[255];
   private byte[] code = null;
   private static ScriptInterpreter instance = null;
   private byte walkAxis = 0;
   private byte walkActor = 0;
   private byte walkPhase = 0;
   private byte talkActor = -1;
   public boolean waitingForKey = false;
   private boolean firstLoad = true;

   public ScriptInterpreter(b var1) {
      this.game = var1;
      instance = this;
      this.reset();
   }

   private final void reset() {
      boolean var1 = false;
      boolean var2 = false;
      this.keyHooks[3] = -1;
      this.keyHooks[4] = -1;
      this.keyHooks[5] = -1;
      this.keyHooks[6] = -1;
      this.keyHooks[7] = -1;
      this.pairTable[0] = -1;
      this.depth = 0;
      this.waitElapsed = 0;
      this.waitDuration = -1;
      this.stringCount = 0;
      this.waitActors = null;
      this.code = null;
      this.spawnIdCount = 0;
      this.walkTarget = null;
      this.talkActor = -1;
      this.waitingForKey = false;

      for (int var3 = 0; var3 < this.spawnIds.length; var3++) {
         this.spawnIds[var3] = 0;
      }

      for (int var4 = 0; var4 < 10; var4++) {
         this.pcStack[var4] = 0;
         this.scriptStack[var4] = 0;
      }

      for (int var5 = 0; var5 < this.scriptOffsets.length; var5++) {
         this.scriptOffsets[var5] = 0;
      }

      for (int var6 = 0; var6 < 30; var6++) {
         for (int var7 = 0; var7 < 4; var7++) {
            this.lootTable[var6][var7] = 0;
         }
      }
   }

   public final void load(String var1) {
      int var2 = 0;
      boolean var3 = false;
      if (var1 != null) {
         this.reset();
         int var4 = b.loadResource(var1);

         for (var2 = 1; var2 < b.resourceBuffer[0] * 3; var2 += 3) {
            this.scriptOffsets[b.resourceBuffer[var2]] = ((char)b.resourceBuffer[var2 + 1] & 255) << 8 | ((char)b.resourceBuffer[var2 + 2] & 255) << 0;
         }

         while (b.resourceBuffer[var2++] == 30) {
            if (b.resourceBuffer[var2] == 0) {
               var2 = this.parseMonsterRecord(++var2);
            } else if (b.resourceBuffer[var2] == 1) {
               var2 = this.parseArmorRecord(++var2);
            } else if (b.resourceBuffer[var2] == 2) {
               var2 = this.parseConsumableRecord(++var2);
            } else if (b.resourceBuffer[var2] == 4) {
               var2 = this.parseWeaponRecord(++var2);
            } else if (b.resourceBuffer[var2] == 5) {
               var2 = this.parseClassRecord(++var2);
            } else if (b.resourceBuffer[var2] == 6) {
               var2 = this.parseTable6Record(++var2);
            } else if (b.resourceBuffer[var2] == 7) {
               var2 = this.parsePairList(++var2);
            } else if (b.resourceBuffer[var2] == 8) {
               var2 = this.parseSpecialRecord(++var2);
            } else if (b.resourceBuffer[var2] == 9) {
               var2 = this.parseSpawnGroupRecord(++var2);
            } else if (b.resourceBuffer[var2] == 10) {
               var2 = this.parseLootRecord(++var2);
            }
         }

         if (this.firstLoad) {
            for (var2 = 0; var2 < this.monsterTypes.length; var2++) {
               System.arraycopy(this.monsterTypes[var2], 0, this.monsterTypesBackup[var2], 0, this.monsterTypesBackup[var2].length);
            }

            this.firstLoad = false;
         }

         var2 += 2;

         for (int var17 = 0; var17 < this.scriptOffsets.length; var17++) {
            if (this.scriptOffsets[var17] != 0) {
               this.scriptOffsets[var17] = this.scriptOffsets[var17] - var2;
            }
         }

         this.code = new byte[var4 - var2];
         System.arraycopy(b.resourceBuffer, var2, this.code, 0, this.code.length);
         runScript(1);
      }
   }

   private final int parseMonsterRecord(int var1) {
      byte var2 = 0;
      int[] var3 = new int[21];

      while (b.resourceBuffer[var1] != 31) {
         if (b.resourceBuffer[var1] == 1) {
            this.strings[this.stringCount] = new String(b.resourceBuffer, var1 + 2, b.resourceBuffer[var1 + 1]);
            var3[b.resourceBuffer[var1]] = this.stringCount++;
            var1 += b.resourceBuffer[var1 + 1] + 1;
         } else if (b.resourceBuffer[var1] != 7 && b.resourceBuffer[var1] != 14 && b.resourceBuffer[var1] != 15) {
            if (b.resourceBuffer[var1] == 0) {
               var2 = b.resourceBuffer[var1 + 1];
            }

            var3[b.resourceBuffer[var1++]] = (char)(b.resourceBuffer[var1] & 0xFF);
         } else {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
            var1 += 2;
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.monsterTypes[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int parseArmorRecord(int var1) {
      byte var2 = 0;
      int[] var3 = new int[10];

      while (b.resourceBuffer[var1] != 31) {
         if (b.resourceBuffer[var1] == 1) {
            byte var4;
            if (((var4 = b.resourceBuffer[var1 + 1]) & 240) == 240) {
               var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.strings[this.stringCount] = new String(b.resourceBuffer, var1 + 2, b.resourceBuffer[var1 + 1]);
               var3[b.resourceBuffer[var1]] = this.stringCount++;
               var1 += b.resourceBuffer[var1 + 1] + 1;
            }
         } else if (b.resourceBuffer[var1] == 5) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 16
               | ((char)b.resourceBuffer[var1 + 2] & 255) << 8
               | ((char)b.resourceBuffer[var1 + 3] & 255) << 0;
            var1 += 3;
         } else if (b.resourceBuffer[var1] == 9) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.resourceBuffer[var1] == 6) {
            var3[b.resourceBuffer[var1]] = 1;
         } else {
            if (b.resourceBuffer[var1] == 0) {
               var2 = b.resourceBuffer[var1 + 1];
            }

            var3[b.resourceBuffer[var1++]] = b.resourceBuffer[var1];
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.armors[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int parseConsumableRecord(int var1) {
      byte var2 = 0;
      int[] var3 = new int[14];

      while (b.resourceBuffer[var1] != 31) {
         if (b.resourceBuffer[var1] == 1) {
            byte var4;
            if (((var4 = b.resourceBuffer[var1 + 1]) & 240) == 240) {
               var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.strings[this.stringCount] = new String(b.resourceBuffer, var1 + 2, b.resourceBuffer[var1 + 1]);
               var3[b.resourceBuffer[var1]] = this.stringCount++;
               var1 += b.resourceBuffer[var1 + 1] + 1;
            }
         } else if (b.resourceBuffer[var1] == 5) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 16
               | ((char)b.resourceBuffer[var1 + 2] & 255) << 8
               | ((char)b.resourceBuffer[var1 + 3] & 255) << 0;
            var1 += 3;
         } else if (b.resourceBuffer[var1] == 13) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.resourceBuffer[var1] == 4) {
            var3[b.resourceBuffer[var1]] = 1;
         } else {
            if (b.resourceBuffer[var1] == 0) {
               var2 = b.resourceBuffer[var1 + 1];
            }

            var3[b.resourceBuffer[var1++]] = (char)b.resourceBuffer[var1] & 255;
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.consumables[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int parseWeaponRecord(int var1) {
      byte var2 = 0;
      int[] var3 = new int[8];

      while (b.resourceBuffer[var1] != 31) {
         if (b.resourceBuffer[var1] == 1) {
            byte var4;
            if (((var4 = b.resourceBuffer[var1 + 1]) & 240) == 240) {
               var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.strings[this.stringCount] = new String(b.resourceBuffer, var1 + 2, b.resourceBuffer[var1 + 1]);
               var3[b.resourceBuffer[var1]] = this.stringCount++;
               var1 += b.resourceBuffer[var1 + 1] + 1;
            }
         } else if (b.resourceBuffer[var1] == 7) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
            var1 += 2;
         } else {
            if (b.resourceBuffer[var1] == 0) {
               var2 = b.resourceBuffer[var1 + 1];
            }

            var3[b.resourceBuffer[var1++]] = b.resourceBuffer[var1];
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.weapons[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int parseClassRecord(int var1) {
      byte var2 = 0;
      int[] var3 = new int[15];
      int[] var4 = new int[15];
      int[] var5 = new int[15];
      int var6 = 0;
      int var7 = 0;

      while (b.resourceBuffer[var1] != 31) {
         if (b.resourceBuffer[var1] == 1) {
            byte var8;
            if (((var8 = b.resourceBuffer[var1 + 1]) & 240) == 240) {
               var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.strings[this.stringCount] = new String(b.resourceBuffer, var1 + 2, b.resourceBuffer[var1 + 1]);
               var3[b.resourceBuffer[var1]] = this.stringCount++;
               var1 += b.resourceBuffer[var1 + 1] + 1;
            }
         } else if (b.resourceBuffer[var1] == 6 || b.resourceBuffer[var1] == 13 || b.resourceBuffer[var1] == 14) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.resourceBuffer[var1] == 2) {
            var4[var6++] = b.resourceBuffer[++var1];
         } else if (b.resourceBuffer[var1] == 3) {
            var5[var7++] = b.resourceBuffer[++var1];
         } else {
            if (b.resourceBuffer[var1] == 0) {
               var2 = b.resourceBuffer[var1 + 1];
            }

            var3[b.resourceBuffer[var1++]] = b.resourceBuffer[var1];
         }

         var1++;
      }

      var4[var6] = -1;
      var5[var7] = -1;
      System.arraycopy(var3, 0, this.classBase[var2], 0, var3.length);
      System.arraycopy(var5, 0, this.classLists[var2], 0, var5.length);
      System.arraycopy(var4, 0, this.classItemTypes[var2], 0, var4.length);
      return var1 + 1;
   }

   private final int parseTable6Record(int var1) {
      byte var2 = 0;
      int[] var3 = new int[7];

      while (b.resourceBuffer[var1] != 31) {
         if (b.resourceBuffer[var1] == 2) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
            var1 += 2;
         } else {
            if (b.resourceBuffer[var1] == 0) {
               var2 = b.resourceBuffer[var1 + 1];
            }

            var3[b.resourceBuffer[var1++]] = (char)b.resourceBuffer[var1] & 255;
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.table6[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int parsePairList(int var1) {
      int var2 = 0;

      while (b.resourceBuffer[var1] != 31) {
         this.pairTable[var2++] = b.resourceBuffer[var1++];
         this.pairTable[var2++] = b.resourceBuffer[var1++];
      }

      this.pairTable[var2] = -1;
      return var1 + 1;
   }

   private final int parseSpecialRecord(int var1) {
      byte var2 = 0;
      int[] var3 = new int[15];

      while (b.resourceBuffer[var1] != 31) {
         if (b.resourceBuffer[var1] == 1) {
            byte var4;
            if (((var4 = b.resourceBuffer[var1 + 1]) & 240) == 240) {
               var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.strings[this.stringCount] = new String(b.resourceBuffer, var1 + 2, b.resourceBuffer[var1 + 1]);
               var3[b.resourceBuffer[var1]] = this.stringCount++;
               var1 += b.resourceBuffer[var1 + 1] + 1;
            }
         } else if (b.resourceBuffer[var1] == 14) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.resourceBuffer[var1] == 6) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 16
               | ((char)b.resourceBuffer[var1 + 2] & 255) << 8
               | ((char)b.resourceBuffer[var1 + 3] & 255) << 0;
            var1 += 3;
         } else {
            if (b.resourceBuffer[var1] == 0) {
               var2 = b.resourceBuffer[var1 + 1];
            }

            var3[b.resourceBuffer[var1++]] = b.resourceBuffer[var1];
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.specials[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int parseSpawnGroupRecord(int var1) {
      byte var2 = 0;
      int[] var3 = new int[21];

      while (b.resourceBuffer[var1] != 31) {
         if (b.resourceBuffer[var1] == 1 || b.resourceBuffer[var1] == 2) {
            var3[b.resourceBuffer[var1]] = ((char)b.resourceBuffer[var1 + 1] & 255) << 8 | ((char)b.resourceBuffer[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.resourceBuffer[var1] == 20) {
            this.spawnIds[this.spawnIdCount++] = (char)b.resourceBuffer[++var1] & 255;
         } else {
            if (b.resourceBuffer[var1] == 0) {
               var2 = b.resourceBuffer[var1 + 1];
            }

            var3[b.resourceBuffer[var1++]] = (char)b.resourceBuffer[var1] & 255;
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.spawnGroups[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int parseLootRecord(int var1) {
      byte var2 = 0;
      int[] var3 = new int[4];

      while (b.resourceBuffer[var1] != 31) {
         if (b.resourceBuffer[var1] == 0) {
            var2 = b.resourceBuffer[var1 + 1];
         }

         var3[b.resourceBuffer[var1++]] = (char)b.resourceBuffer[var1] & 255;
         var1++;
      }

      System.arraycopy(var3, 0, this.lootTable[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int readByte() {
      int var1 = (char)this.code[this.pcStack[this.depth - 1]] & 255;
      this.pcStack[this.depth - 1]++;
      return var1;
   }

   private final int readInt24() {
      int var1 = ((char)this.code[this.pcStack[this.depth - 1] + 0] & 255) << 16
         | ((char)this.code[this.pcStack[this.depth - 1] + 1] & 255) << 8
         | ((char)this.code[this.pcStack[this.depth - 1] + 2] & 255) << 0;
      this.pcStack[this.depth - 1] = this.pcStack[this.depth - 1] + 3;
      return var1;
   }

   private final int readShort() {
      int var1 = ((char)this.code[this.pcStack[this.depth - 1] + 0] & 255) << 8 | ((char)this.code[this.pcStack[this.depth - 1] + 1] & 255) << 0;
      this.pcStack[this.depth - 1] = this.pcStack[this.depth - 1] + 2;
      return var1;
   }

   private final String readString(int var1) {
      String var2 = new String(this.code, this.pcStack[this.depth - 1], var1);
      this.pcStack[this.depth - 1] = this.pcStack[this.depth - 1] + var1;
      return var2;
   }

   private final void step(long var1) {
      String var3 = null;
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      int var7 = this.pcStack[this.depth - 1];
      byte var8 = this.code[var7];
      int var9 = 0;
      int var10 = 0;
      int var11 = 0;
      int var12 = 0;
      int var13 = 0;
      int var14 = 0;
      int var15 = 0;
      int var16 = 0;
      int var17 = 0;
      int var18 = 0;
      int var19 = 0;
      int var20 = 0;
      int var21 = 0;
      int var22 = 0;
      int var23 = 0;
      int var24 = 0;
      int var25 = 0;
      int var26 = 0;
      int var27 = 0;
      int var28 = 0;
      int var29 = 0;
      int var30 = 0;
      int var31 = 0;
      if (!this.waitingForKey) {
         if (!this.game.dialogueOpen) {
            if (this.talkActor >= 0) {
               ActorSystem.setStatusIcon(b.actors[this.talkActor], (byte)0);
               this.talkActor = -1;
            }

            if (!this.game.isBusyState()) {
               if (this.waitDuration >= 0) {
                  this.waitElapsed = (int)(this.waitElapsed + var1);
                  if (this.waitElapsed < this.waitDuration) {
                     return;
                  }

                  this.waitDuration = -1;
               }

               if (this.waitActors != null) {
                  for (int var51 = 0; var51 < this.waitActors.length; var51++) {
                     if (b.actors[this.waitActors[var51]] != null && b.actors[this.waitActors[var51]].moveTarget[0] != -1) {
                        return;
                     }
                  }

                  this.waitActors = null;
               }

               if (this.walkTarget != null) {
                  switch (this.walkPhase) {
                     case OP_INVALID0:
                        this.game.setInputEnabled(false);
                        this.game.cameraFollow(this.walkActor);
                        if (this.walkAxis != 0 && this.walkAxis != 1) {
                           ActorSystem.setMoveTarget(b.actors[this.walkActor], this.walkTarget[0], b.actors[this.walkActor].pos[1]);
                        } else {
                           ActorSystem.setMoveTarget(b.actors[this.walkActor], b.actors[this.walkActor].pos[0], this.walkTarget[1]);
                        }

                        ActorSystem.setAnimState(b.actors[this.walkActor], (byte)2);
                        ActorSystem.setStat(b.actors[this.walkActor], 7, 900, this);
                        this.walkPhase = 1;
                        return;
                     case OP_INVALID1:
                        if (b.actors[this.walkActor].moveTarget[0] == -1) {
                           ActorSystem.setAnimState(b.actors[this.walkActor], (byte)3);
                           ActorSystem.setStat(b.actors[this.walkActor], 7, 400, this);
                           ActorSystem.setMoveTarget(b.actors[this.walkActor], this.walkTarget[0], this.walkTarget[1]);
                           this.walkPhase = 2;
                           return;
                        }
                        break;
                     case OP_NOP_B:
                        if (b.actors[this.walkActor].moveTarget[0] == -1) {
                           this.game.setInputEnabled(true);
                           this.walkTarget = null;
                        }
                  }
               } else {
                  this.pcStack[this.depth - 1]++;
                  switch (var8) {
                     case OP_INVALID0:
                        System.err.println("1) Never should have gotten here!!!");
                        return;
                     case OP_INVALID1:
                        System.err.println("2) Never should have gotten here!!!");
                        return;
                     case OP_NOP_B:
                        this.returnFromScript();
                        return;
                     case OP_SAY:
                        var20 = this.readShort();
                        b.setSpeakerName(null);
                        if ((var20 & 61440) == 61440) {
                           var3 = b.getString(var20 & 4095);
                        } else {
                           var3 = this.readString(var20);
                        }

                        this.game.showDialogue(var3);
                        this.game.beginDialogue();
                        return;
                     case OP_SET_SCREEN_SIZE:
                        this.game.setScreenSize(this.readByte(), this.readByte());
                        return;
                     case OP_NOP_B2:
                        this.readByte();
                        return;
                     case OP_NOP_B3:
                        this.readByte();
                        return;
                     case OP_SET_PLAYER_COLLIDES:
                        b.playerCollides = this.readByte() == 1;
                        return;
                     case OP_LOAD_MAP:
                        try {
                           this.game.loadMap(this.readString(this.readByte()));
                           this.game.loadTileSprites(this.readString(this.readByte()));
                           return;
                        } catch (Exception var36) {
                           var36.printStackTrace();
                           return;
                        }
                     case OP_SKIP_STRING:
                        this.readString(this.readByte());
                        return;
                     case OP_END_LEVEL:
                        this.game.endLevel(this.readByte(), this.readInt24());
                        return;
                     case OP_WAIT:
                        this.waitDuration = this.readShort();
                        this.waitElapsed = 0;
                        return;
                     case OP_SET_STATE_PLAYING:
                        b.setState((byte)0);
                        return;
                     case OP_NOP_B4:
                        this.readByte();
                        return;
                     case OP_SET_KEY_HOOK:
                        switch (this.readByte()) {
                           case 0:
                              this.keyHooks[3] = this.readByte();
                              break;
                           case 1:
                              this.keyHooks[4] = this.readByte();
                              break;
                           case 2:
                              this.keyHooks[5] = this.readByte();
                              break;
                           case 3:
                              this.keyHooks[6] = this.readByte();
                              break;
                           case 4:
                              this.keyHooks[7] = this.readByte();
                        }

                        return;
                     case OP_SPAWN_ACTOR:
                        if ((var20 = this.readShort()) != 0) {
                           if ((var20 & 61440) == 61440) {
                              var3 = b.getString(var20 & 4095);
                           } else {
                              var3 = this.readString(var20);
                           }
                        }

                        var14 = this.readByte();
                        var11 = this.readByte();
                        var12 = this.readShort();
                        var13 = this.readShort();
                        this.game.spawnActorInSlot(var3, this.strings[this.monsterTypes[var11][1]], (byte)var14, var12, var13, this.monsterTypes[var11]);
                        return;
                     case OP_SET_TRIGGER:
                        this.game.setTrigger(this.readByte(), this.readByte(), this.readByte(), this.readByte(), this.readByte());
                        return;
                     case OP_MOVE_ACTOR_TO:
                        var17 = this.readByte();
                        var15 = this.readShort();
                        var16 = this.readShort();
                        if (b.actors[var17] != null) {
                           ActorSystem.setMoveTarget(b.actors[var17], var15, var16);
                           return;
                        }
                        break;
                     case OP_SET_TILE:
                        var15 = this.readByte();
                        var16 = this.readByte();
                        var10 = this.readByte();
                        var9 = this.readByte();
                        this.game.setTile(var15, var16, var10, var9);
                        return;
                     case OP_SET_INPUT_ENABLED:
                        this.game.setInputEnabled(this.readByte() == 1);
                        return;
                     case OP_REMOVE_ACTOR:
                        b.removeActor(this.readByte());
                        return;
                     case OP_WAIT_ACTORS_STOP:
                        this.waitActors = new int[this.readByte()];

                        for (int var60 = 0; var60 < this.waitActors.length; var60++) {
                           this.waitActors[var60] = this.readByte();
                        }
                        break;
                     case OP_SET_COLLISION:
                        this.game.setCollision(this.readByte(), this.readByte(), this.readByte() == 1);
                        return;
                     case OP_CALL:
                        runScript(this.readByte());
                        return;
                     case OP_SET_ANIM_STATE:
                        ActorSystem.setAnimState(b.actors[this.readByte()], (byte)this.readByte());
                        return;
                     case OP_CAMERA_TO:
                        this.game.cameraTo(this.readShort(), this.readShort());
                        return;
                     case OP_CAMERA_FOLLOW:
                        this.game.cameraFollow(this.readByte());
                        return;
                     case OP_CLEAR_TRIGGER:
                        this.game.setTrigger(this.readByte(), this.readByte(), 255, 255, 255);
                        return;
                     case OP_CLEAR_KEY_HOOK:
                        switch (this.readByte()) {
                           case 0:
                              this.keyHooks[3] = -1;
                              break;
                           case 1:
                              this.keyHooks[4] = -1;
                              break;
                           case 2:
                              this.keyHooks[5] = -1;
                              break;
                           case 3:
                              this.keyHooks[6] = -1;
                              break;
                           case 4:
                              this.keyHooks[7] = -1;
                        }

                        return;
                     case OP_LOAD_LEVEL:
                        this.game.loadLevel(this.readString(this.readByte()));
                        return;
                     case 30:
                     case 31:
                     default:
                        break;
                     case OP_SET_DEATH_SCRIPT:
                        ActorSystem.setDeathScript(b.actors[this.readByte()], this.readByte(), this.readByte());
                        return;
                     case OP_CLEAR_DEATH_SCRIPT:
                        ActorSystem.clearDeathScript(b.actors[this.readByte()], this.readByte());
                        return;
                     case OP_SET_STAT:
                        var17 = this.readByte();
                        var21 = this.readByte();
                        var22 = 0;
                        switch (var21) {
                           case 2:
                           case 3:
                           case 4:
                           case 5:
                           case 6:
                           case 8:
                           case 9:
                           case 10:
                           case 11:
                           case 12:
                           case 13:
                           case 18:
                           case 19:
                           case 20:
                              var22 = this.readByte();
                              break;
                           case 7:
                           case 14:
                           case 15:
                              var22 = this.readShort();
                           case 16:
                           case 17:
                        }

                        if (b.actors[var17] != null) {
                           ActorSystem.setStat(b.actors[var17], var21, var22, this);
                           return;
                        }
                        break;
                     case OP_NOP_B5:
                        this.readByte();
                        return;
                     case OP_SET_POSITION:
                        var17 = this.readByte();
                        var15 = this.readShort();
                        var16 = this.readShort();
                        if (b.actors[var17] != null) {
                           ActorSystem.setPosition(b.actors[var17], var15, var16);
                           return;
                        }
                        break;
                     case OP_GIVE_ITEM:
                        var17 = this.readByte();
                        var18 = this.readByte();
                        var19 = this.readByte();
                        if (b.actors[var17] != null) {
                           switch (var18) {
                              case 0:
                                 ActorSystem.addItem(b.actors[var17], var18, this.weapons[var19]);
                                 break;
                              case 1:
                                 ActorSystem.addItem(b.actors[var17], var18, this.armors[var19]);
                                 break;
                              case 2:
                                 ActorSystem.addItem(b.actors[var17], var18, this.consumables[var19]);
                           }

                           return;
                        }
                        break;
                     case OP_REMOVE_ITEM:
                        var17 = this.readByte();
                        var18 = this.readByte();
                        var19 = this.readByte();
                        if (b.actors[var17] != null) {
                           switch (var18) {
                              case 0:
                                 ActorSystem.removeItem(b.actors[var17], var18, this.weapons[var19]);
                                 break;
                              case 1:
                                 ActorSystem.removeItem(b.actors[var17], var18, this.armors[var19]);
                                 break;
                              case 2:
                                 ActorSystem.removeItem(b.actors[var17], var18, this.consumables[var19]);
                           }

                           return;
                        }
                        break;
                     case OP_SHOW_MESSAGE:
                        if (((var20 = this.readShort()) & 61440) == 61440) {
                           var3 = b.getString(var20 & 4095);
                        } else {
                           var3 = this.readString(var20);
                        }

                        var4 = this.readByte();
                        var5 = this.readByte();
                        var6 = this.readByte();
                        b.showMessage(var3, var4, var5, var6);
                        return;
                     case OP_HIDE_MESSAGE:
                        b.showMessage(null, 0, 0, 0);
                        return;
                     case OP_MOVE_ACTOR_X:
                        var17 = this.readByte();
                        var15 = this.readShort();
                        if (b.actors[var17] != null) {
                           ActorSystem.setMoveTarget(b.actors[var17], var15, b.actors[var17].pos[1]);
                           return;
                        }
                        break;
                     case OP_MOVE_ACTOR_Y:
                        var17 = this.readByte();
                        var16 = this.readShort();
                        if (b.actors[var17] != null) {
                           ActorSystem.setMoveTarget(b.actors[var17], b.actors[var17].pos[0], var16);
                           return;
                        }
                        break;
                     case OP_LOAD_HUD_SPRITES:
                        this.game.loadHudSprites(this.readString(this.readByte()));
                        return;
                     case OP_OPEN_MENU:
                        this.game.openMenu();
                        return;
                     case OP_OPEN_SHOP_MENU:
                        this.game.openShopMenu();
                        return;
                     case OP_SET_STATUS_ICON:
                        var17 = this.readByte();
                        var23 = this.readByte();
                        if (b.actors[var17] != null) {
                           ActorSystem.setStatusIcon(b.actors[var17], (byte)var23);
                           return;
                        }
                        break;
                     case OP_GENERATE_DUNGEON:
                        this.game.generateDungeon(this.getRow(9, this.readByte()), this.spawnIds, this.readByte(), this.readByte());
                        return;
                     case OP_CLEAR_LAYERS:
                        this.game.clearLayers();
                        return;
                     case OP_PLACE_ITEM:
                        this.game.spawnItem(this.readByte(), true, this.readByte(), this.readByte());
                        return;
                     case OP_SET_TRIGGER_RECT:
                        var24 = this.readByte();
                        var25 = this.readByte();
                        var26 = this.readByte();
                        var27 = this.readByte();
                        var28 = this.readByte();
                        var29 = this.readByte();
                        var30 = this.readByte();

                        for (int var57 = var24; var57 <= var26; var57++) {
                           for (int var69 = var25; var69 <= var27; var69++) {
                              this.game.setTrigger(var57, var69, var28, var29, var30);
                           }
                        }
                        break;
                     case OP_CLEAR_TRIGGER_RECT:
                        var24 = this.readByte();
                        var25 = this.readByte();
                        var26 = this.readByte();
                        var27 = this.readByte();

                        for (int var56 = var24; var56 <= var26; var56++) {
                           for (int var68 = var25; var68 <= var27; var68++) {
                              this.game.setTrigger(var56, var68, 255, 255, 255);
                           }
                        }
                        break;
                     case OP_WALK_CUTSCENE:
                        this.walkActor = (byte)this.readByte();
                        this.walkAxis = (byte)this.readByte();
                        this.walkTarget = new int[]{this.readShort(), this.readShort()};
                        this.walkPhase = 0;
                        return;
                     case OP_TALK:
                        this.talkActor = (byte)this.readByte();
                        var23 = this.readByte();
                        var20 = this.readShort();
                        this.game.cameraFollow(this.talkActor);
                        ActorSystem.setStatusIcon(b.actors[this.talkActor], (byte)var23);
                        if ((var20 & 61440) == 61440) {
                           this.game.showDialogue(b.getString(var20 & 4095));
                        } else {
                           this.game.showDialogue(this.readString(var20));
                        }

                        this.game.beginDialogue();
                        return;
                     case OP_NOP54:
                        return;
                     case OP_NOP55:
                        return;
                     case OP_LOAD_LANG:
                        System.err.println("load lang command");
                        String var32 = this.readString(this.readByte());
                        int var33;
                        if ((var33 = this.readByte()) == 0) {
                           var33 = 65535;
                        }

                        b.loadLangPack(var32, var33);
                        return;
                     case OP_NOP57:
                        return;
                     case OP_SCALE_MONSTER:
                        var14 = this.readByte();
                        int var112 = this.readByte();
                        this.monsterTypes[var14][2] = var112;

                        for (int var67 = 3; var67 <= 9; var67++) {
                           this.monsterTypes[var14][var67] = this.monsterTypes[var14][var67]
                              + var112 * archetypeGrowth[this.monsterTypes[var14][17]][var67 - 3];
                        }
                        break;
                     case OP_SET_DROPS_LOOT:
                        var17 = this.readByte();
                        int var35 = this.readByte();
                        if (b.actors[var17] != null) {
                           ActorSystem.setDropsLoot(b.actors[var17], var35 == 1);
                           return;
                        }
                        break;
                     case OP_WAIT_KEY:
                        this.waitingForKey = true;
                        return;
                     case OP_SET_STATE_9:
                        b.setState((byte)9);
                        return;
                     case OP_NOP62:
                        return;
                     case OP_NOP63:
                        return;
                     case OP_SET_BACKGROUND_COLOR:
                        b.backgroundColor = this.readInt24();
                        return;
                     case OP_LEVEL_UP_TO:
                        var17 = this.readByte();
                        int var34 = this.readByte();
                        if (b.actors[var17] != null) {
                           ActorSystem.levelUpTo(b.actors[var17], var34);
                           return;
                        }
                        break;
                     case OP_SHOW_TEXT_SCREEN:
                        if (((var20 = this.readShort()) & 61440) == 61440) {
                           b.showTextScreen(b.getString(var20 & 4095));
                           return;
                        }

                        b.showTextScreen(this.readString(var20));
                        return;
                     case OP_RESTORE_MONSTER_TYPE:
                        var14 = this.readByte();
                        System.arraycopy(this.monsterTypesBackup[var14], 0, this.monsterTypes[var14], 0, this.monsterTypes[var14].length);
                        return;
                     case OP_SPAWN_PROJECTILE:
                        var31 = this.readByte();
                        var15 = this.readShort();
                        var16 = this.readShort();
                        if (var31 == 0) {
                           var31 = 8;
                        } else if (var31 == 2) {
                           var31 = 10;
                        } else if (var31 == 1) {
                           var31 = 9;
                        }

                        ProjectileManager.spawn(var31, var15, var16);
                        return;
                     case OP_SPAWN_TIMED_PROJECTILE:
                        var31 = this.readByte();
                        var15 = this.readShort();
                        var16 = this.readShort();
                        var4 = this.readByte();
                        if (var31 == 0) {
                           var31 = 8;
                        } else if (var31 == 2) {
                           var31 = 10;
                        } else if (var31 == 1) {
                           var31 = 9;
                        }

                        ProjectileManager.spawnFixed(var31, var15, var16, var4 * 1000);
                        return;
                     case OP_CLEAR_PROJECTILE_AT:
                        var15 = this.readShort();
                        var16 = this.readShort();
                        ProjectileManager.clearAt(var15, var16);
                        return;
                     case OP_SET_POINT:
                        var15 = this.readShort();
                        var16 = this.readShort();
                        b.setPoint(var15, var16);
                        return;
                     case OP_EVICT_SPRITES:
                        SpriteRenderer.evict(this.readString(this.readShort()));
                        return;
                     case OP_BEGIN_FADE:
                        b.setState((byte)15);
                        b.stateChangesEnabled = false;
                        return;
                     case OP_END_FADE:
                        b.stateChangesEnabled = true;
                        return;
                     case OP_TOGGLE_INVULNERABLE:
                        var17 = this.readByte();
                        if (b.actors[var17] != null) {
                           b.actors[var17].invulnerable = (byte)(b.actors[var17].invulnerable == 1 ? 0 : 1);
                           return;
                        }
                        break;
                     case OP_SET_HUD_VISIBLE:
                        b.hudVisible = this.readByte() == 1;
                        return;
                     case OP_SET_STATE_4:
                        b.setState((byte)4);
                        b.stateFlagF = false;
                        return;
                     case OP_SET_AI_ACTIVE:
                        var17 = this.readByte();
                        if (b.actors[var17] != null) {
                           b.actors[var17].aiActive = (byte)this.readByte();
                        }
                  }
               }
            }
         }
      }
   }

   public static final void runScript(int var0) {
      if (instance.depth < instance.scriptStack.length - 2 && instance.depth < instance.pcStack.length - 2) {
         instance.scriptStack[instance.depth] = var0;
         instance.pcStack[instance.depth++] = instance.scriptOffsets[var0];
      }
   }

   private final void returnFromScript() {
      this.depth--;
   }

   public final void tick(long var1) {
      if (this.depth > 0) {
         this.step(var1);
      }
   }

   public final int[] getRow(int var1, int var2) {
      if (var2 < 0) {
         return null;
      }

      switch (var1) {
         case 0:
            if (var2 < this.monsterTypes.length) {
               return this.monsterTypes[var2];
            }
         case 1:
            if (var2 < this.armors.length) {
               return this.armors[var2];
            }
         case 2:
            if (var2 < this.consumables.length) {
               return this.consumables[var2];
            }
         case 4:
            if (var2 < this.weapons.length) {
               return this.weapons[var2];
            }
         case 7:
            return this.pairTable;
         case 9:
            if (var2 < this.spawnGroups.length) {
               return this.spawnGroups[var2];
            }
         case 6:
            if (var2 < this.table6.length) {
               return this.table6[var2];
            }
         case 5:
            if (var2 < this.classBase.length) {
               return this.classBase[var2];
            }
         case 8:
            if (var2 < this.specials.length) {
               return this.specials[var2];
            }
         case 10:
            if (var2 < this.lootTable.length) {
               return this.lootTable[var2];
            }
         case 3:
         default:
            return null;
      }
   }

   public final int[] getRowChecked(int var1, int var2) {
      switch (var1) {
         case 0:
            return this.weapons[var2];
         case 1:
            return this.armors[var2];
         case 2:
            return this.consumables[var2];
         default:
            return null;
      }
   }

   public final void keyPressed(int var1) {
      if (this.waitingForKey) {
         this.waitingForKey = false;
         b.keyState = -286331154;
      } else if (var1 == 3 && this.keyHooks[3] >= 0) {
         runScript(this.keyHooks[3]);
         this.keyHooks[3] = -1;
      } else if (var1 == 4 && this.keyHooks[4] >= 0) {
         runScript(this.keyHooks[4]);
         this.keyHooks[4] = -1;
      } else if (var1 == 5 && this.keyHooks[5] >= 0) {
         runScript(this.keyHooks[5]);
         this.keyHooks[5] = -1;
      } else if (var1 == 6 && this.keyHooks[6] >= 0) {
         runScript(this.keyHooks[6]);
         this.keyHooks[6] = -1;
      } else {
         if (var1 == 7 && this.keyHooks[7] >= 0) {
            runScript(this.keyHooks[7]);
            this.keyHooks[7] = -1;
         }
      }
   }

   public final String getItemName(int var1) {
      return (var1 & 61440) == 61440 ? b.getString(var1 & 4095) : this.strings[var1];
   }

   public final int findString(String var1) {
      int var2 = 0;
      boolean var3 = false;

      for (int var5 = 0; var5 < this.stringCount; var5++) {
         if (this.strings[var5].equals(var1)) {
            return var5;
         }
      }

      return (var2 = b.stringToId(var1)) == -1 ? var2 : 61440 | var2;
   }

   public final int[] findByName(String var1) {
      int var2 = this.findString(var1);
      boolean var3 = false;
      if (var2 != -1) {
         for (int var4 = 0; var4 < this.weapons.length; var4++) {
            if (this.weapons[var4] != null && this.weapons[var4][1] == var2) {
               return this.weapons[var4];
            }
         }

         for (int var5 = 0; var5 < this.consumables.length; var5++) {
            if (this.consumables[var5] != null && this.consumables[var5][1] == var2) {
               return this.consumables[var5];
            }
         }

         for (int var6 = 0; var6 < this.armors.length; var6++) {
            if (this.armors[var6] != null && this.armors[var6][1] == var2) {
               return this.armors[var6];
            }
         }

         for (int var7 = 0; var7 < this.classBase.length; var7++) {
            if (this.classBase[var7] != null && this.classBase[var7][1] == var2) {
               return this.classBase[var7];
            }
         }

         for (int var8 = 0; var8 < this.specials.length; var8++) {
            if (this.specials[var8] != null && this.specials[var8][1] == var2) {
               return this.specials[var8];
            }
         }
      }

      return null;
   }

   public final int itemCategory(String var1) {
      int var2 = this.findString(var1);
      boolean var3 = false;
      if (var2 != -1) {
         for (int var4 = 0; var4 < this.weapons.length; var4++) {
            if (this.weapons[var4] != null && this.weapons[var4][1] == var2) {
               return 0;
            }
         }

         for (int var5 = 0; var5 < this.consumables.length; var5++) {
            if (this.consumables[var5] != null && this.consumables[var5][1] == var2) {
               return 2;
            }
         }

         for (int var6 = 0; var6 < this.armors.length; var6++) {
            if (this.armors[var6] != null && this.armors[var6][1] == var2) {
               return 1;
            }
         }
      }

      return -1;
   }

   public final int rollLoot() {
      int var1 = b.random.nextInt();

      for (int var2 = 1; this.lootTable[var2][1] != 0; var2++) {
         if (var1 % this.lootTable[var2][2] == 0 && this.lootTable[var2][3] > 0) {
            this.lootTable[var2][3]--;
            return this.lootTable[var2][1];
         }
      }

      return 0;
   }

   public final boolean classAllows(int var1, int var2) {
      boolean var3 = false;

      for (int var4 = 0; var4 < this.classItemTypes[var1].length; var4++) {
         if (this.classItemTypes[var1][var4] == var2) {
            return true;
         }
      }

      return false;
   }

   public static final String getSkillName(int var0) {
      switch (var0) {
         case 0:
            return b.getString(523);
         case 1:
            return b.getString(524);
         case 2:
            return b.getString(525);
         case 3:
            return b.getString(526);
         case 4:
            return b.getString(527);
         case 5:
            return b.getString(528);
         case 6:
            return b.getString(529);
         case 7:
            return b.getString(530);
         case 8:
            return b.getString(531);
         case 9:
            return b.getString(532);
         case 10:
            return b.getString(533);
         case 11:
            return b.getString(534);
         case 12:
            return b.getString(535);
         case 13:
            return b.getString(536);
         case 14:
            return b.getString(537);
         default:
            return null;
      }
   }
}
