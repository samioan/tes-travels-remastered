import blt.Main;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Random;
import java.util.Vector;
import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;
import javax.microedition.rms.RecordStore;
import javax.microedition.rms.RecordStoreNotFoundException;

/**
 * Renamed from decompiled/b.java (see docs/CLASS_MAP.md, docs/rename.map).
 * The whole engine in one class: Canvas, game-loop thread, resource loader,
 * .jtm map loader, procedural dungeon generator, renderer, input dispatcher,
 * menus/help screens and the save game (RecordStore "ESO").
 *
 * `state` is the screen/mode id (see setState and paint): 0 playing, 1 shop,
 * 2 inventory, 3 menu, 4 intro text, 5 key bindings, 6/7 loading bar, 8
 * full-screen cutscene image, 9/10/17/21/23 scrolling text screens, 11
 * "Continue?" after death, 12 exit, 13 "Game Saved", 14 load-game prompt, 15
 * "Please Wait", 16 overwrite-save prompt, 18 help pages, 19 quit prompt, 20
 * "Key Already Taken", 22 pause. `menuId` picks the menu list in `menus`.
 * Local variables are still decompiler-numbered (varN).
 */
public final class Game extends Canvas implements Runnable {
   private boolean newGameLocked = false;
   private static byte tileWidth = 32;
   private static byte tileHeight = 16;
   public static byte keyRightSoft = 22;
   public static byte keyLeftSoft = 21;
   public static byte keyOk = 23;
   public static byte keyD = -104;
   public static byte keyE = -105;
   public static final Random random = new Random();
   private static byte[] menuSelection = new byte[8];
   private static byte[] keyBindings = new byte[]{55, 57, 51};
   private static byte[] keyBindingsEdit = new byte[keyBindings.length];
   private static byte menuId = -1;
   private static byte messageStyle = 0;
   private static int messageDuration = 0;
   private static int messageElapsed = 0;
   private static int messageColor = 0;
   private static int messageX = 0;
   private static int messageY = 0;
   private static int messageBlinkTimer = 0;
   private static int messageScrollTimer = 0;
   private static int optionsCursor = 0;
   private static int blinkTimer = -1;
   public static int keyState = -286331154;
   private static boolean blinkOn = true;
   public static short screenWidth = 0;
   public static short screenHeight = 0;
   public static byte gridWidth = 0;
   public static byte gridHeight = 0;
   public static boolean stateChangesEnabled = true;
   private static byte state = -1;
   private static byte pausedState = -1;
   public static byte[] collision = null;
   public static byte[] resourceBuffer = null;
   public static boolean textScrollEnded = true;
   private static boolean editingKey = false;
   private static boolean messageBlank = false;
   public static boolean playerCollides = true;
   public static Actor[] actors = new Actor[25];
   public static Game instance = null;
   public static ScriptInterpreter script = null;
   public static Vector layers = new Vector();
   private static String[][] menus = (String[][])null;
   private static String[] optionLabels = null;
   private static String[] keyLabels = new String[100];
   private static String message = null;
   private static String currentLevel = null;
   private byte[] viewMin = new byte[]{0, 0};
   private byte[] viewMax = new byte[]{0, 0};
   private byte stateBeforeLoading = -1;
   private byte keyHandled = 0;
   private byte cameraActor = 0;
   private byte loadProgress = 0;
   private int[] cameraOffset = new int[]{0, 0};
   private int[] unusedJ = new int[]{0, 0};
   private int[] unusedK = new int[]{0, 0};
   private int[] unusedL = new int[]{-12345, -12345};
   private short[] cellScreenPos = null;
   private byte[] enterLayer = null;
   private byte[] leaveLayer = null;
   public static byte[] zoneLayer = null;
   private int[] branchPoints = new int[50];
   private byte branchCount = 0;
   private byte branchPointCount = 0;
   private byte segmentCount = 0;
   public static int gold = 100;
   private short centerX = 0;
   private short centerY = 0;
   private short cellSize = 128;
   private short cellCount = 0;
   private int cutsceneSprite = 0;
   private int cutsceneColor = 0;
   private int maxActorSlot = 0;
   private boolean inputEnabled = true;
   private boolean running = true;
   private boolean redraw = true;
   private boolean soundEnabled = false;
   private long now = 0L;
   private long lastTick = 0L;
   private SpriteFrame tileSprites = null;
   private SpriteFrame hudSprites = null;
   private DialogueScreen dialogue = null;
   private Actor player = null;
   private Graphics offscreenGraphics = null;
   private Image offscreenImage = null;
   private Main midlet = null;
   private static String version = null;
   private String errorText = null;
   public static String errorCode = null;
   private static int[] stageTrace = new int[255];
   private static int stageTraceLen = 0;
   private int crashKeyCount = 0;
   private Vector[] textLines = null;
   private short textScrollY = 0;
   private short textScrollTimer = 0;
   private boolean textAtEnd = false;
   private static int[] tmpWorld = new int[2];
   public static int backgroundColor = 0;
   private static short respawnX = 0;
   private static short respawnY = 0;
   public static Font fontSmall = Font.getFont(0, 0, 8);
   public static Font fontMedium = Font.getFont(0, 0, 0);
   public static Font fontSmallBold = Font.getFont(0, 1, 8);
   public static Font fontLargeBold = Font.getFont(0, 1, 16);
   public boolean suspended = false;
   private static SpriteFrame playerSprites = null;
   private short spinnerTimer = 0;
   public static byte[] pickups = new byte[75];
   public static byte pickupCount = 0;
   public static boolean hudVisible = true;
   private static String[][] helpPages = (String[][])null;
   private static String[][] helpWeapons = (String[][])null;
   private static String[][] helpArmor = (String[][])null;
   private static String[][] helpSpells = (String[][])null;
   private static String[][] helpItems = (String[][])null;
   private static String[][] helpClasses = (String[][])null;
   private static String[][] helpOverview = (String[][])null;
   private static byte helpPage = 0;
   private static byte helpScroll = 0;
   private static short helpTitleId = 0;
   private static boolean helpHasMore = false;
   private static byte menuReturn = 0;
   private static DialogueNode equippedWeaponNode = null;
   private static DialogueNode equippedSpellNode = null;
   private static Image splashImage = null;
   private static Image titleImage = null;
   private static char[] langChars0 = null;
   private static short[] langOffsets0 = null;
   private static char[] langChars1 = null;
   private static short[] langOffsets1 = null;
   public static boolean stateFlagF = false;
   public int[] screenCorner0 = new int[]{0, 0};
   public int[] screenCorner1 = new int[]{0, 0};
   public int[] screenCorner2 = new int[]{0, 0};
   public int[] screenCorner3 = new int[]{0, 0};
   public int[] viewCell0 = new int[]{0, 0};
   public int[] viewCell1 = new int[]{0, 0};
   public int[] viewCell2 = new int[]{0, 0};
   public int[] viewCell3 = new int[]{0, 0};
   private boolean dialogueAtEnd = false;
   public boolean dialogueOpen = false;
   private Vector dialogueLines = new Vector();
   private int dialogueRight = 0;
   private int dialogueLeft = 12;
   private int dialogueTop = 7;
   private int dialogueScroll = 0;
   private int dialogueTextWidth = 0;
   private int dialogueHeight = 0;
   private long dialogueOpenedAt = 0L;
   private static String speakerName = null;

   public Game(Main var1, String var2, String var3, String var4) {
      this.setFullScreenMode(true);
      this.midlet = var1;
      version = var4;
      instance = this;
      resetStatics();
      script = new ScriptInterpreter(this);
      this.setScreenSize(this.getWidth(), this.getHeight() + 25);
      SpriteRenderer.clearImageCache();
      this.textScrollY = (short)(screenHeight - (fontSmallBold.getHeight() << 3));
      this.textScrollTimer = 0;
      pickupCount = 0;
      this.dialogue = new DialogueScreen(var3, this);
      playerSprites = SpriteRenderer.load("/oh_pc.cml");
      this.loadLevel(var2);
      this.loadGame(false);
   }

   private static final void resetStatics() {
      boolean var0 = false;
      keyBindings[0] = 55;
      keyBindings[1] = 57;
      keyBindings[2] = 51;
      menuId = -1;
      messageDuration = 0;
      messageElapsed = 0;
      messageColor = 0;
      messageStyle = 0;
      messageX = -1;
      messageY = -1;
      messageBlinkTimer = 0;
      messageScrollTimer = 0;
      optionsCursor = 0;
      blinkTimer = -1;
      gold = 100;
      screenWidth = 0;
      screenHeight = 0;
      gridWidth = 0;
      gridHeight = 0;
      collision = null;
      splashImage = null;
      editingKey = false;
      messageBlank = false;
      script = null;
      menus = (String[][])null;
      optionLabels = null;
      message = null;
      currentLevel = null;

      for (int var1 = 0; var1 < menuSelection.length; var1++) {
         menuSelection[var1] = 0;
      }

      for (int var2 = 0; var2 < keyBindingsEdit.length; var2++) {
         keyBindingsEdit[var2] = 0;
      }

      for (int var3 = 0; var3 < actors.length; var3++) {
         actors[var3] = null;
      }

      for (int var4 = 0; var4 < keyLabels.length; var4++) {
         keyLabels[var4] = null;
      }

      layers.removeAllElements();
   }

   private void dropHelpCaches() {
      helpPages = (String[][])null;
      helpWeapons = (String[][])null;
      helpArmor = (String[][])null;
      helpSpells = (String[][])null;
      helpItems = (String[][])null;
      helpClasses = (String[][])null;
      helpOverview = (String[][])null;
      System.gc();
   }

   private final void buildMenus() {
      int var1 = 0;
      boolean var2 = false;
      menus = new String[][]{
         null,
         null,
         {"Level 1", "Level 2", "Level 3", "Level 4", "Level 5", "Level 6", "Level 7", "Level 8", "Level 9", "Level 10", "Level 11", "Level 12"},
         {
               "/l01_1.scr",
               "/l02_2_1.scr",
               "/l03_3.scr",
               "/l04_4.scr",
               "/l05_5.scr",
               "/l06_6_cr.scr",
               "/l07_7_cr.scr",
               "/l08_8_cr.scr",
               "/l09_9_cr.scr",
               "/l10_10_cr.scr",
               "/l11_11_cr.scr",
               "/l12_12.scr"
         },
         {getString(18), getString(19), getString(20)},
         null,
         {getString(457), getString(458), getString(573), getString(522), getString(459), getString(460), getString(461), getString(462)}
      };
      menus[0] = new String[this.hasSavedGame() ? 5 : 4];
      menus[5] = new String[this.hasSavedGame() ? 6 : 5];
      int var3 = 0;
      String[] var10000 = menus[0];
      var3++;
      var10000[0] = getString(2);
      if (this.hasSavedGame()) {
         var10000 = menus[0];
         var3++;
         var10000[1] = getString(3);
      }

      menus[0][var3++] = getString(456);
      menus[0][var3++] = getString(6);
      menus[0][var3] = getString(22);
      var3 = 0;
      var10000 = menus[5];
      var3++;
      var10000[0] = getString(21);
      var10000 = menus[5];
      var3++;
      var10000[1] = getString(2);
      if (this.hasSavedGame()) {
         var10000 = menus[5];
         var3++;
         var10000[2] = getString(3);
      }

      menus[5][var3++] = getString(456);
      menus[5][var3++] = getString(6);
      menus[5][var3] = getString(22);

      for (int var5 = 0; var5 < script.classBase.length; var5++) {
         if (script.classBase[var5][0] > 0) {
            var1++;
         }
      }

      menus[1] = new String[var1];
      var1 = 0;

      for (int var6 = 0; var6 < script.classBase.length; var6++) {
         if (script.classBase[var6][0] > 0) {
            menus[1][var1++] = script.getItemName(script.classBase[var6][1]);
         }
      }

      optionLabels = new String[]{getString(292), getString(293), getString(463), getString(294)};
      keyLabels[1] = getString(281);
      keyLabels[6] = getString(282);
      keyLabels[2] = getString(283);
      keyLabels[5] = getString(284);
      keyLabels[8] = getString(285);
      keyLabels[48] = "# 0";
      keyLabels[49] = "# 1";
      keyLabels[50] = "# 2";
      keyLabels[51] = "# 3";
      keyLabels[52] = "# 4";
      keyLabels[53] = "# 5";
      keyLabels[54] = "# 6";
      keyLabels[55] = "# 7";
      keyLabels[56] = "# 8";
      keyLabels[57] = "# 9";
      keyLabels[35] = getString(286);
      keyLabels[42] = getString(287);
   }

   public final void loadLevel(String var1) {
      boolean var2 = false;
      collectGarbage();
      setState((byte)6);
      currentLevel = var1;
      collision = null;
      this.enterLayer = null;
      this.leaveLayer = null;
      zoneLayer = null;
      this.cellScreenPos = null;
      this.inputEnabled = true;
      this.running = true;
      playerCollides = true;
      this.maxActorSlot = 0;
      this.loadProgress = -1;
      this.cameraOffset[0] = 0;
      this.cameraOffset[1] = 0;
      this.unusedL[0] = -12345;
      this.unusedL[1] = -12345;
      this.unusedJ[0] = 0;
      this.unusedJ[1] = 0;
      this.unusedK[0] = 0;
      this.unusedK[1] = 0;
      menuId = -1;
      keyState = -286331154;
      this.keyHandled = 0;
      this.textScrollY = (short)(screenHeight - (fontSmallBold.getHeight() << 3));
      this.textScrollTimer = 0;
      this.dialogueOpen = false;
      ProjectileManager.clearAll();
      if (this.player != null) {
         this.player.summon = null;
      }

      for (int var3 = 0; var3 < 25; var3++) {
         actors[var3] = null;
      }

      collectGarbage();
      script.load(var1);
   }

   public final void setScreenSize(int var1, int var2) {
      screenWidth = (short)var1;
      screenHeight = (short)var2;
      this.centerX = (short)(screenWidth >> 1);
      this.centerY = (short)(screenHeight >> 1);
      this.offscreenImage = Image.createImage(screenWidth, screenHeight);
      this.offscreenGraphics = this.offscreenImage.getGraphics();
   }

   private final void finishMapLoad() {
      int[] var1 = new int[]{0, 0};
      int[] var2 = new int[]{0, 0};
      int var3 = tileWidth >> 1;
      boolean var4 = false;
      boolean var5 = false;
      this.redraw = true;

      for (int var6 = 0; var6 < gridWidth; var6++) {
         for (int var8 = 0; var8 < gridHeight; var8++) {
            worldToIso(var1, var2);
            this.cellScreenPos[var6 * gridHeight + 0 * this.cellCount + var8] = (short)(var2[0] - var3);
            this.cellScreenPos[var6 * gridHeight + 1 * this.cellCount + var8] = (short)var2[1];
            var1[1] += this.cellSize;
         }

         var1[0] += this.cellSize;
         var1[1] = 0;
      }

      for (int var7 = 1; var7 < 25; var7++) {
         actors[var7] = null;
      }

      if (actors[0] != null) {
         ActorSystem.revive(actors[0]);
         ActorSystem.updateCells(actors[0]);
      }

      collectGarbage();
   }

   private final void carveCell(byte[] var1, int var2, int var3, int var4) {
      int var5;
      if ((var5 = var2 * gridHeight + var3) < collision.length && var5 >= 0) {
         var1[var5] = (byte)var4;
         collision[var5] = 0;
      }
   }

   private final void carvePath(byte[] var1, int[] var2, int[] var3, int var4, int var5, int[] var6) {
      boolean var7 = false;
      boolean var8 = false;
      int[] var9 = new int[]{var2[0], var2[1]};
      int var10 = var5;
      boolean var11 = false;
      int var12 = 0;
      int var13 = 0;

      while (!var8) {
         if (random.nextInt() % var6[16] == 0 && ++this.branchCount < var6[15]) {
            while ((var12 = Math.abs(random.nextInt()) % 4 + 1) == var10) {
            }

            var13 = Math.abs(random.nextInt() % Math.min(gridWidth, gridHeight));
            if (var12 == 2) {
               this.carvePath(var1, var9, new int[]{var13, 3}, var4, var12, var6);
            } else if (var12 == 1) {
               this.carvePath(var1, var9, new int[]{var13, gridHeight - 3}, var4, var12, var6);
            } else if (var12 == 3) {
               this.carvePath(var1, var9, new int[]{gridWidth - 3, var13}, var4, var12, var6);
            } else if (var12 == 4) {
               this.carvePath(var1, var9, new int[]{3, var13}, var4, var12, var6);
            }
         }

         if (var10 != 1 && var10 != 2) {
            if (var10 == 3 || var10 == 4) {
               var7 = random.nextInt() % var6[19] == 0;
               this.segmentCount++;

               for (int var17 = 0; var17 < var4; var17++) {
                  this.carveCell(var1, var9[0], var9[1] + var17, var6[4]);
               }

               if (var7 && this.branchPointCount < 50 && this.segmentCount > 10) {
                  this.branchPoints[this.branchPointCount++] = var9[0];
                  this.branchPoints[this.branchPointCount++] = var9[1] + 1;
               }
            }
         } else {
            var7 = random.nextInt() % var6[19] == 0;
            this.segmentCount++;

            for (int var16 = 0; var16 < var4; var16++) {
               this.carveCell(var1, var9[0] + var16, var9[1], var6[4]);
            }

            if (var7 && this.branchPointCount < 50 && this.segmentCount > 10) {
               this.branchPoints[this.branchPointCount++] = var9[0] + 1;
               this.branchPoints[this.branchPointCount++] = var9[1];
            }
         }

         while (true) {
            var12 = Math.abs(random.nextInt() % 4);
            if (var3[0] < var9[0]) {
               if (var12 == 1) {
                  var10 = 2;
               }

               if (var12 == 2) {
                  var10 = 1;
               }

               if (var12 == 3) {
                  var10 = 4;
               }
            } else if (var3[0] > var9[0]) {
               if (var12 == 1) {
                  var10 = 2;
               }

               if (var12 == 2) {
                  var10 = 1;
               }

               if (var12 == 3) {
                  var10 = 3;
               }
            } else if (var12 < 2) {
               var10 = 2;
            } else {
               var10 = 1;
            }

            if (var10 == 1 && var9[1] < var3[1]) {
               var9[1]++;
               break;
            }

            if (var10 == 3 && var9[0] < var3[0]) {
               var9[0]++;
               break;
            }

            if (var10 == 2 && var9[1] > var3[1]) {
               var9[1]--;
               break;
            }

            if (var10 == 4 && var9[0] > var3[0]) {
               var9[0]--;
               break;
            }
         }

         var8 = var9[0] == var3[0] && var9[1] == var3[1];
      }

      if (var10 != 1 && var10 != 2) {
         if (var10 == 3 || var10 == 4) {
            this.segmentCount++;

            for (int var19 = 0; var19 < var4; var19++) {
               this.carveCell(var1, var9[0], var9[1] + var19, var6[4]);
            }
         }
      } else {
         this.segmentCount++;

         for (int var18 = 0; var18 < var4; var18++) {
            this.carveCell(var1, var9[0] + var18, var9[1], var6[4]);
         }
      }
   }

   public final void pickWallTiles(byte[] var1, byte[] var2, int[] var3) {
      boolean var4 = false;
      boolean var5 = false;

      for (int var6 = 0; var6 < gridWidth; var6++) {
         for (int var7 = 0; var7 < gridHeight; var7++) {
            if (var6 * gridHeight + var7 <= var1.length - 1
               && var6 * gridHeight + var7 >= 0
               && var6 * gridHeight + (var7 - 1) <= var1.length - 1
               && var6 * gridHeight + (var7 - 1) >= 0
               && var6 * gridHeight + var7 + 1 <= var1.length - 1
               && var6 * gridHeight + var7 + 1 >= 0
               && (var6 - 1) * gridHeight + var7 <= var1.length - 1
               && (var6 - 1) * gridHeight + var7 >= 0
               && (var6 + 1) * gridHeight + var7 <= var1.length - 1
               && (var6 + 1) * gridHeight + var7 >= 0
               && var1[var6 * gridHeight + var7] == var3[4]) {
               if (var1[var6 * gridHeight + (var7 - 1)] == var3[3]) {
                  if (var1[(var6 - 1) * gridHeight + var7] == var3[3]) {
                     var2[var6 * gridHeight + var7] = (byte)var3[9];
                  } else if (var1[(var6 + 1) * gridHeight + var7] == var3[3]) {
                     var2[var6 * gridHeight + var7] = (byte)var3[10];
                  } else {
                     var2[var6 * gridHeight + var7] = (byte)var3[5];
                  }
               } else if (var1[var6 * gridHeight + var7 + 1] == var3[3]) {
                  if (var1[(var6 - 1) * gridHeight + var7] == var3[3]) {
                     var2[var6 * gridHeight + var7] = (byte)var3[11];
                  } else if (var1[(var6 + 1) * gridHeight + var7] == var3[3]) {
                     var2[var6 * gridHeight + var7] = (byte)var3[12];
                  } else {
                     var2[var6 * gridHeight + var7] = (byte)var3[6];
                  }
               } else if (var1[(var6 + 1) * gridHeight + var7] == var3[3]) {
                  var2[var6 * gridHeight + var7] = (byte)var3[7];
               } else if (var1[(var6 - 1) * gridHeight + var7] == var3[3]) {
                  var2[var6 * gridHeight + var7] = (byte)var3[8];
               }
            }
         }
      }
   }

   private final void carveEndpoints(byte[] var1, int[] var2, int[] var3, int[] var4) {
      int var5 = var1.length;
      byte var6 = gridHeight;
      boolean var7 = false;
      boolean var8 = false;
      boolean var9 = false;
      byte[][] var10;
      int var11 = (var10 = new byte[][]{{-1, 1}, {-1, 0}, {-1, -1}, {0, 1}, {0, 0}, {0, -1}, {1, 1}, {1, 0}, {1, -1}}).length;

      for (int var14 = 0; var14 < var11; var14++) {
         int var12 = var2[0] + var10[var14][0];
         int var13 = var2[1] + var10[var14][1];
         if (var12 * var6 + var13 < var5 && var12 * var6 + var13 >= 0) {
            var1[var12 * var6 + var13] = (byte)var4[4];
            collision[var12 * var6 + var13] = 0;
         }
      }

      for (int var15 = 0; var15 < var11; var15++) {
         int var16 = var3[0] + var10[var15][0];
         int var17 = var3[1] + var10[var15][1];
         if (var16 * var6 + var17 < var5 && var16 * var6 + var17 >= 0) {
            var1[var16 * var6 + var17] = (byte)var4[4];
         }
      }
   }

   public final void generateDungeon(int[] var1, int[] var2, int var3, int var4) {
      byte[] var5 = null;
      int var6 = 0;
      boolean var7 = false;
      boolean var8 = false;
      byte var9 = 0;
      int[] var10 = null;
      int[] var11 = null;
      collision = null;
      this.enterLayer = null;
      this.leaveLayer = null;
      zoneLayer = null;
      this.cellScreenPos = null;
      collectGarbage();

      for (int var16 = 1; var16 < actors.length; var16++) {
         actors[var16] = null;
      }

      if (this.player != null) {
         this.player.summon = null;
      }

      this.branchCount = 0;
      this.branchPointCount = 0;
      this.segmentCount = 0;
      pickupCount = 0;
      layers.removeAllElements();
      gridWidth = (byte)var1[1];
      gridHeight = (byte)var1[2];
      this.cellCount = (short)(gridWidth * gridHeight);
      collision = new byte[this.cellCount];
      this.enterLayer = new byte[this.cellCount];
      this.leaveLayer = new byte[this.cellCount];
      zoneLayer = new byte[this.cellCount];
      this.cellScreenPos = new short[this.cellCount * 2];

      for (int var17 = 0; var17 < gridWidth; var17++) {
         for (int var20 = 0; var20 < gridHeight; var20++) {
            this.enterLayer[var17 * gridHeight + var20] = -1;
            this.leaveLayer[var17 * gridHeight + var20] = -1;
            zoneLayer[var17 * gridHeight + var20] = -1;
         }
      }

      var5 = new byte[this.cellCount];
      layers.addElement(var5);

      for (int var18 = 0; var18 < gridWidth; var18++) {
         for (int var21 = 0; var21 < gridHeight; var21++) {
            var5[var18 * gridHeight + var21] = (byte)var1[3];
            collision[var18 * gridHeight + var21] = 1;
         }
      }

      var10 = new int[]{2, 2};
      var11 = new int[]{gridWidth - 2, gridHeight - 2};
      this.carvePath(var5, var10, var11, var1[14], 1, var1);
      this.carveEndpoints(var5, var10, var11, var1);
      byte[] var12 = var5;
      var5 = new byte[this.cellCount];
      layers.addElement(var5);
      this.pickWallTiles(var12, var5, var1);
      var5 = new byte[this.cellCount];
      layers.addElement(var5);
      var5[var10[0] * gridHeight + var10[1]] = (byte)var1[13];
      var5[var11[0] * gridHeight + var11[1]] = (byte)var1[13];
      this.enterLayer[var11[0] * gridHeight + 0 * this.cellCount + var11[1]] = (byte)var4;
      this.leaveLayer[var10[0] * gridHeight + var10[1]] = -2;
      this.enterLayer[var10[0] * gridHeight + var10[1]] = -2;
      zoneLayer[var10[0] * gridHeight + var10[1]] = (byte)var3;
      this.finishMapLoad();

      for (byte var19 = 0; var19 < this.branchPointCount && var19 < var1[18]; var19 += 2) {
         if (var2[var6] != 0) {
            this.spawnItem(var2[var6], false, this.branchPoints[var19], this.branchPoints[var19 + 1]);
            var6++;
         }

         var9 = 2;

         while (actors[var9] != null) {
            var9++;
         }

         this.spawnActorInSlot(
            null,
            script.getItemName(script.monsterTypes[var1[17]][1]),
            var9,
            this.branchPoints[var19] << 7,
            this.branchPoints[var19 + 1] << 7,
            script.monsterTypes[var1[17]]
         );
      }
   }

   public final void clearLayers() {
      layers.removeAllElements();
      this.redraw = true;
      if (actors[0] != null) {
         ActorSystem.revive(actors[0]);
         ActorSystem.updateCells(actors[0]);
      }
   }

   public final void loadMap(String var1) throws Exception {
      collision = null;
      this.enterLayer = null;
      this.leaveLayer = null;
      zoneLayer = null;
      this.cellScreenPos = null;
      collectGarbage();
      byte[] var2 = null;
      char var3 = '\u0000';
      boolean var4 = false;
      boolean var5 = false;
      int var6 = 0;
      int var7 = 0;
      int var8 = 0;
      int var9 = 0;
      int var10 = loadResource(var1);
      if (this.player != null) {
         this.player.summon = null;
      }

      layers.removeAllElements();
      ProjectileManager.clearAll();
      var7++;
      gridWidth = resourceBuffer[0];
      var7++;
      gridHeight = resourceBuffer[1];
      this.cellCount = (short)(gridWidth * gridHeight);
      pickupCount = 0;
      collision = new byte[this.cellCount];
      this.enterLayer = new byte[this.cellCount];
      this.leaveLayer = new byte[this.cellCount];
      zoneLayer = new byte[this.cellCount];
      this.cellScreenPos = new short[this.cellCount * 2];

      for (int var14 = 0; var14 < gridWidth; var14++) {
         for (int var17 = 0; var17 < gridHeight; var17++) {
            this.enterLayer[var14 * gridHeight + var17] = -1;
            this.leaveLayer[var14 * gridHeight + var17] = -1;
            zoneLayer[var14 * gridHeight + var17] = -1;
         }
      }

      var9 = 0;
      var8 = -1;
      var6 = -1;
      var3 = '\u0000';

      for (int var18 = 0; var18 < gridHeight; var18++) {
         for (int var15 = 0; var15 < gridWidth; var15++) {
            if (var8 == -1) {
               var8 = (char)(resourceBuffer[var7++] & 0xFF);
            }

            if (var8 == 255) {
               if (var9 == 0) {
                  var3 = (char)(resourceBuffer[var7++] & 0xFF);
                  var6 = (char)(resourceBuffer[var7++] & 0xFF);
               }

               if (++var9 < var3) {
                  collision[var15 * gridHeight + var18] = (byte)var6;
               } else {
                  collision[var15 * gridHeight + var18] = (byte)var6;
                  var8 = -1;
                  var9 = 0;
               }
            } else {
               collision[var15 * gridHeight + var18] = (byte)var8;
               var8 = -1;
               var9 = 0;
            }
         }
      }

      while (var7 < var10) {
         var2 = new byte[this.cellCount];
         layers.addElement(var2);
         var9 = 0;
         var8 = -1;
         var6 = -1;
         var3 = '\u0000';

         for (int var19 = 0; var19 < gridHeight; var19++) {
            for (int var16 = 0; var16 < gridWidth; var16++) {
               if (var8 == -1) {
                  var8 = (char)(resourceBuffer[var7++] & 0xFF);
               }

               if (var8 == 255) {
                  if (var9 == 0) {
                     var3 = (char)(resourceBuffer[var7++] & 0xFF);
                     var6 = (char)(resourceBuffer[var7++] & 0xFF);
                  }

                  if (++var9 < var3) {
                     var2[var16 * gridHeight + var19] = (byte)var6;
                  } else {
                     var2[var16 * gridHeight + var19] = (byte)var6;
                     var8 = -1;
                     var9 = 0;
                  }
               } else {
                  var2[var16 * gridHeight + var19] = (byte)var8;
                  var8 = -1;
                  var9 = 0;
               }
            }
         }
      }

      resourceBuffer = null;
      this.finishMapLoad();
   }

   private final void drawTileLayers(Graphics var1) {
      byte[] var2 = null;
      int var3 = 0;
      int var4 = 0;
      int var5 = 0;
      boolean var6 = false;
      boolean var7 = false;
      boolean var8 = false;
      if (this.redraw) {
         this.redraw = false;
         this.offscreenGraphics.setColor(backgroundColor);
         this.offscreenGraphics.fillRect(0, 0, screenWidth, screenHeight);

         for (int var14 = 0; var14 < layers.size() - 1; var14++) {
            var2 = (byte[])layers.elementAt(var14);

            for (int var12 = this.viewMin[0]; var12 <= this.viewMax[0] && var12 < gridWidth; var12++) {
               for (int var13 = this.viewMin[1]; var13 <= this.viewMax[1] && var13 < gridHeight; var13++) {
                  if (var12 >= 0 && var13 >= 0) {
                     var4 = this.cellScreenPos[var12 * gridHeight + 0 * this.cellCount + var13] + this.cameraOffset[0];
                     var5 = this.cellScreenPos[var12 * gridHeight + 1 * this.cellCount + var13] + this.cameraOffset[1];
                     if (var2[var12 * gridHeight + var13] != 0) {
                        var3 = SpriteRenderer.getHeight(this.tileSprites, (int)var2[var12 * gridHeight + var13]);
                     }

                     if (var4 > -tileWidth
                        && var4 < screenWidth
                        && var5 > -tileHeight
                        && var5 < screenHeight + var3
                        && var2[var12 * gridHeight + var13] != 0) {
                        SpriteRenderer.draw(this.offscreenGraphics, this.tileSprites, (int)var2[var12 * gridHeight + var13], var4, var5);
                     }
                  }
               }
            }
         }

         this.lastTick = System.currentTimeMillis();
      }
   }

   public final void paint(Graphics var1) {
      byte[] var2 = null;
      int var3 = 0;
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      boolean var7 = false;
      boolean var8 = false;
      String var9 = null;
      if (this.errorText != null) {
         var1.setColor(16777215);
         var1.fillRect(0, 0, screenWidth, screenHeight);
         var1.setColor(0);
         var1.drawString(this.errorText, 5, 10, 0);
         var1.drawString(errorCode, 5, 25, 0);
         var1.drawString("Press any key 3x to quit...", 5, 40, 0);
      } else if (splashImage != null) {
         var1.setColor(0);
         var1.fillRect(0, 0, screenWidth, screenHeight);
         var1.drawImage(splashImage, this.centerX - (splashImage.getWidth() >> 1), this.centerY - (splashImage.getHeight() >> 1), 0);
      } else {
         switch (state) {
            case 0:
               this.updateCamera();
               this.updateViewport();
               this.drawTileLayers(var1);
               var1.drawImage(this.offscreenImage, 0, 0, 0);
               if (layers.size() != 0) {
                  var2 = (byte[])layers.elementAt(layers.size() - 1);

                  for (int var33 = this.viewMin[0]; var33 <= this.viewMax[0] && var33 < gridWidth; var33++) {
                     for (int var35 = this.viewMin[1]; var35 <= this.viewMax[1] && var35 < gridHeight; var35++) {
                        if (var33 >= 0 && var35 >= 0) {
                           var4 = this.cellScreenPos[var33 * gridHeight + 0 * this.cellCount + var35] + this.cameraOffset[0];
                           var5 = this.cellScreenPos[var33 * gridHeight + 1 * this.cellCount + var35] + this.cameraOffset[1];
                           if (var2[var33 * gridHeight + var35] != 0) {
                              var3 = SpriteRenderer.getHeight(this.tileSprites, (int)var2[var33 * gridHeight + var35]);
                           }

                           if (var4 > -tileWidth && var4 < screenWidth && var5 > -tileHeight && var5 < screenHeight + var3) {
                              if (var2[var33 * gridHeight + var35] != 0) {
                                 SpriteRenderer.draw(var1, this.tileSprites, (int)var2[var33 * gridHeight + var35], var4, var5);
                              }

                              for (int var36 = 0; var36 < actors.length; var36++) {
                                 if (actors[var36] != null && actors[var36].sortCell[0] == var33 && actors[var36].sortCell[1] == var35) {
                                    ActorSystem.draw(actors[var36], var1, this.cameraOffset);
                                 }
                              }
                           }
                        }
                     }
                  }

                  if (hudVisible && this.player != null) {
                     if (this.player.dead == 0) {
                        int var41 = Math.min(70, 70 * this.player.hp / this.player.maxHp);
                        int var11 = Math.min(70, 70 * this.player.mp / this.player.maxMp);
                        var1.setColor(16711680);
                        var1.fillRect(18, 10, var41, 7);
                        var1.setColor(255);
                        var1.fillRect(18, 18, var11, 7);
                     }

                     SpriteRenderer.draw(var1, this.tileSprites, -56, 0, 0);
                     if (this.player.attackIcon != -1) {
                        SpriteRenderer.draw(
                           var1, this.hudSprites, this.player.attackIcon, screenWidth - SpriteRenderer.getWidth(this.hudSprites, this.player.attackIcon) - 2, 2
                        );
                     }

                     if (this.player.effectIcon != -1) {
                        SpriteRenderer.draw(
                           var1,
                           this.hudSprites,
                           this.player.effectIcon,
                           screenWidth - (SpriteRenderer.getWidth(this.hudSprites, this.player.attackIcon) << 1) - 4,
                           2
                        );
                     }
                  }

                  var1.setFont(fontSmallBold);
                  var1.setColor(16777215);
                  var1.drawString(getString(422).toUpperCase(), 2, screenHeight - fontSmallBold.getHeight() - 2, 0);
                  if (hudVisible) {
                     var1.drawString(
                        getString(421).toUpperCase(),
                        screenWidth - fontSmallBold.stringWidth(getString(421)) - 2,
                        screenHeight - fontSmallBold.getHeight() - 2,
                        0
                     );
                  }

                  var1.setFont(fontSmall);
                  if (message != null) {
                     messageY = screenHeight - fontMedium.getHeight() - 5;
                     var1.setFont(fontMedium);
                     var1.setColor(0);
                     var1.fillRect(0, messageY - 5, screenWidth, screenHeight);
                     if (!messageBlank) {
                        if (messageX == -1) {
                           if (messageStyle == 0 || messageStyle == 1) {
                              messageX = (screenWidth >> 1) - (fontMedium.stringWidth(message) >> 1);
                           } else if (messageStyle == 2) {
                              messageX = -fontMedium.stringWidth(message);
                           } else if (messageStyle == 3) {
                              messageX = screenWidth;
                           }
                        }

                        var1.setColor(messageColor);
                        var1.drawString(message, messageX, messageY, 0);
                     }
                  }

                  ProjectileManager.draw(var1, this.cameraOffset);
                  if (this.dialogueOpen && this.dialogue.open == 0) {
                     this.drawDialogue(var1);
                  }
               }
            case 1:
            case 2:
            case 12:
            default:
               break;
            case 3:
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(0);
               var1.setFont(fontLargeBold);
               if (titleImage != null) {
                  var1.drawImage(titleImage, this.centerX - (titleImage.getWidth() >> 1), 0, 0);
                  if (menuId == 1) {
                     var1.setColor(1044480);
                     var1.drawString(getString(423), (screenWidth >> 1) - (fontLargeBold.stringWidth(getString(423)) >> 1), titleImage.getHeight() + 1, 0);
                  }
               }

               var1.setColor(16711680);
               var1.drawString("<<", 0, (titleImage != null ? titleImage.getHeight() : 0) + 25, 0);
               var1.drawString(">>", screenWidth - fontLargeBold.stringWidth(">>"), (titleImage != null ? titleImage.getHeight() : 0) + 25, 0);
               var1.setColor(16777215);
               int var10 = screenWidth - fontLargeBold.stringWidth("<<  >>");
               if (fontLargeBold.stringWidth(menus[menuId][menuSelection[menuId]]) >= var10 && menus[menuId][menuSelection[menuId]].indexOf(32) != -1) {
                  String var42 = menus[menuId][menuSelection[menuId]].substring(0, menus[menuId][menuSelection[menuId]].indexOf(32));
                  String var43 = menus[menuId][menuSelection[menuId]].substring(menus[menuId][menuSelection[menuId]].indexOf(32) + 1);
                  var1.drawString(
                     var42,
                     (screenWidth >> 1) - (fontLargeBold.stringWidth(var42) >> 1),
                     (titleImage != null ? titleImage.getHeight() : 0) + 25 - (fontLargeBold.getHeight() >> 1),
                     0
                  );
                  var1.drawString(
                     var43,
                     (screenWidth >> 1) - (fontLargeBold.stringWidth(var43) >> 1),
                     (titleImage != null ? titleImage.getHeight() : 0) + 25 + (fontLargeBold.getHeight() >> 1),
                     0
                  );
               } else {
                  var1.drawString(
                     menus[menuId][menuSelection[menuId]],
                     (screenWidth >> 1) - (fontLargeBold.stringWidth(menus[menuId][menuSelection[menuId]]) >> 1),
                     (titleImage != null ? titleImage.getHeight() : 0) + 25,
                     0
                  );
               }

               if (menuId != 0 && menuId != 5 && menuId != 4) {
                  var1.setFont(fontSmallBold);
                  var1.drawString(getString(449).toUpperCase(), 2, screenHeight - fontSmallBold.getHeight() - 2, 0);
                  var1.setFont(fontLargeBold);
               }
               break;
            case 4:
            case 9:
            case 10:
            case 17:
            case 21:
            case 23:
               var1.setFont(fontSmallBold);
               var1.setColor(state != 4 && state != 21 && state != 23 && state != 17 ? 15327683 : 0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(state != 4 && state != 21 && state != 23 && state != 17 ? 0 : 16777215);
               boolean var14 = false;
               if (state == 21) {
                  var14 = true;
                  var1.setFont(fontSmall);
               }

               if ((state == 23 || state == 17) && this.textScrollY > 20) {
                  this.textScrollY = 20;
               }

               if (state == 21) {
                  this.textScrollY--;
               }

               var5 = 3 + this.textScrollY;

               for (int var31 = 0; var31 < this.textLines.length; var31++) {
                  for (int var34 = 0; var34 < this.textLines[var31].size(); var34++) {
                     int var15 = state != 4 && state != 21 && state != 23 && state != 17 ? 0 : 16777215;
                     var4 = 2;
                     if (!var14) {
                        var5 += fontSmallBold.getHeight() + 1;
                     } else {
                        var14 = false;
                     }

                     if ((var9 = (String)this.textLines[var31].elementAt(var34)).charAt(1) == '~') {
                        if (var9.charAt(0) == '1') {
                           var1.setFont(fontSmallBold);
                           var15 = 11184640;
                        } else if (var9.charAt(0) == '3') {
                           var15 = 11141120;
                           var4 = (screenWidth >> 1) - (fontSmallBold.stringWidth(var9.substring(var9.indexOf(126) + 1)) >> 1);
                        }

                        var9 = var9.substring(var9.indexOf(126) + 1);
                     }

                     var1.setColor(var15);
                     var1.drawString(var9, var4, var5, 0);
                  }

                  if (this.textLines[var31].size() == 0) {
                     var5 += fontSmallBold.getHeight() + 1;
                  }
               }

               if (state == 23 || state == 17) {
                  var1.setColor(0);
                  var1.setFont(fontSmallBold);
                  var1.fillRect(0, 0, screenWidth, fontSmallBold.getHeight() + 20);
                  var1.setColor(16777215);
                  var1.fillRect(5, 5, screenWidth - 10, fontSmallBold.getHeight() + 10);
                  var1.setColor(14483456);
                  var1.drawString(getString(helpTitleId), (screenWidth >> 1) - (fontSmallBold.stringWidth(getString(helpTitleId)) >> 1), 10, 0);
                  var1.setColor(16777215);
               }

               int var44 = screenHeight - fontSmallBold.getHeight();
               if (state == 10 || state == 23 || state == 4 || state == 17) {
                  var44 -= fontSmallBold.getHeight() * 3;
               }

               if (state == 4 || state == 23 || state == 17) {
                  var1.setColor(0);
                  var1.fillRect(0, screenHeight - fontSmallBold.getHeight() - 8, screenWidth, fontSmallBold.getHeight() + 8);
                  var1.setColor(16777215);
                  var1.setFont(fontSmallBold);
                  if (state != 4 && state != 23 && state != 17) {
                     var1.drawString(getString(21).toUpperCase(), 2, screenHeight - fontSmallBold.getHeight() - 2, 0);
                  } else {
                     var1.drawString(getString(449).toUpperCase(), 2, screenHeight - fontSmallBold.getHeight() - 2, 0);
                  }

                  var1.setFont(fontLargeBold);
                  int var46 = 0;
                  if (state == 4) {
                     var46 = screenHeight - (fontSmallBold.getHeight() << 2);
                  } else if (state == 23 || state == 17) {
                     var46 = 10;
                  }

                  if (this.textScrollY < var46) {
                     SpriteRenderer.draw(
                        var1,
                        this.hudSprites,
                        54,
                        this.centerX - (SpriteRenderer.getWidth(this.hudSprites, 54) >> 1),
                        screenHeight - fontSmallBold.getHeight() - 7
                     );
                  }

                  if (var5 > var44) {
                     SpriteRenderer.draw(
                        var1,
                        this.hudSprites,
                        53,
                        this.centerX - (SpriteRenderer.getWidth(this.hudSprites, 53) >> 1),
                        screenHeight - fontSmallBold.getHeight() - 5 + SpriteRenderer.getHeight(this.hudSprites, 54)
                     );
                  }
               }

               if (state == 10) {
                  var1.setColor(15327683);
                  var1.fillRect(0, screenHeight - fontSmallBold.getHeight() - 8, screenWidth, fontSmallBold.getHeight() + 8);
                  var1.setColor(0);
                  if (this.textScrollY < screenHeight - (fontSmallBold.getHeight() << 2)) {
                     SpriteRenderer.draw(
                        var1,
                        this.hudSprites,
                        54,
                        this.centerX - (SpriteRenderer.getWidth(this.hudSprites, 54) >> 1),
                        screenHeight - fontSmallBold.getHeight() - 7
                     );
                  }

                  if (var5 > var44) {
                     SpriteRenderer.draw(
                        var1,
                        this.hudSprites,
                        53,
                        this.centerX - (SpriteRenderer.getWidth(this.hudSprites, 53) >> 1),
                        screenHeight - fontSmallBold.getHeight() - 5 + SpriteRenderer.getHeight(this.hudSprites, 54)
                     );
                  }
               }

               if (var5 < var44) {
                  if (textScrollEnded) {
                     textScrollEnded = false;
                  } else {
                     textScrollEnded = true;
                     if (state != 21 && state != 23 && state != 17) {
                        try {
                           Thread.sleep(3000L);
                        } catch (Exception var19) {
                        }
                     }

                     if (state == 23 || state == 17) {
                        this.textAtEnd = true;
                     }

                     if (state == 9) {
                        this.lastTick = System.currentTimeMillis();
                        this.textScrollY = (short)(screenHeight - (fontSmallBold.getHeight() << 3));
                        this.textScrollTimer = 0;
                        menuSelection = new byte[7];
                        if (stateFlagF) {
                           menuId = 5;
                        } else {
                           menuId = 0;
                        }

                        for (int var32 = 0; var32 < actors.length; var32++) {
                           actors[var32] = null;
                        }

                        setState((byte)4);
                     } else if (state == 10) {
                        setState((byte)0);
                     } else if (state == 4) {
                        setState((byte)3);
                        if (stateFlagF) {
                           menuId = 5;
                        } else {
                           menuId = 0;
                        }
                     }
                  }
               } else if (state == 23 || state == 17) {
                  this.textAtEnd = false;
               }
               break;
            case 5:
               var1.setColor(0);
               var1.setFont(fontSmallBold);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.fillRect(5, 5, screenWidth - 10, 20);
               var1.setColor(14483456);
               if (editingKey) {
                  var9 = getString(424);
               } else {
                  var9 = getString(425);
               }

               var1.drawString(var9, (screenWidth >> 1) - (fontSmallBold.stringWidth(var9) >> 1), 7, 0);
               var1.setColor(16777215);
               var5 = 30;

               for (int var30 = 0; var30 < optionLabels.length; var30++) {
                  var4 = 5;
                  if (var30 != optionsCursor) {
                     var4 = 5 + fontSmallBold.stringWidth("> ");
                  }

                  if (optionLabels[var30].equals(getString(294))) {
                     var5 += fontSmallBold.getHeight();
                  }

                  var1.drawString((var30 == optionsCursor ? "> " : "") + optionLabels[var30], var4, var5, 0);
                  var5 += fontSmallBold.getHeight();
               }

               if (!optionLabels[optionsCursor].equals(getString(294))) {
                  if (editingKey) {
                     var1.setColor(16711680);
                  }

                  var1.drawString(
                     keyLabels[keyBindingsEdit[optionsCursor]],
                     (screenWidth >> 1) - (fontSmallBold.stringWidth(keyLabels[keyBindingsEdit[optionsCursor]]) >> 1),
                     screenHeight - fontSmallBold.getHeight() - 2,
                     0
                  );
               }

               var1.drawString(getString(449).toUpperCase(), 2, screenHeight - fontSmallBold.getHeight() - 2, 0);
               break;
            case 6:
            case 7:
               int var12 = screenWidth - 20;
               int var13 = this.loadProgress * var12 / 100;
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16711680);
               var1.fillRect(10, 30, var13, 10);
               var1.setColor(16777215);
               var1.drawRect(10, 30, var12, 10);
               if ((var9 = getString(39)).length() == 0) {
                  var9 = new Strings().loadStartupLine((byte)0);
               }

               var1.drawString(var9 + "...", 10, 10, 0);
               break;
            case 8:
               var1.setColor(this.cutsceneColor);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               SpriteRenderer.draw(
                  var1,
                  this.hudSprites,
                  this.cutsceneSprite,
                  this.centerX - (SpriteRenderer.getWidth(this.hudSprites, this.cutsceneSprite) >> 1),
                  this.centerY - (SpriteRenderer.getHeight(this.hudSprites, this.cutsceneSprite) >> 1)
               );
               if (script.waitingForKey) {
                  if (blinkOn) {
                     var9 = new Strings().loadStartupLine((byte)1);
                     var1.setFont(fontSmallBold);
                     var1.setColor(0);
                     var1.drawString(
                        var9,
                        (screenWidth >> 1) - (fontSmallBold.stringWidth(var9) >> 1),
                        this.centerY + (SpriteRenderer.getHeight(this.hudSprites, this.cutsceneSprite) >> 1) + 12,
                        0
                     );
                  }

                  if (blinkTimer == -1) {
                     blinkTimer = 500;
                  }
               }
               break;
            case 11:
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.setFont(fontLargeBold);
               var1.drawString(
                  getString(428),
                  (screenWidth >> 1) - (fontLargeBold.stringWidth(getString(428)) >> 1),
                  (screenHeight >> 1) - (fontLargeBold.getHeight() >> 1),
                  0
               );
               var1.drawString(getString(427).toUpperCase(), 2, screenHeight - fontLargeBold.getHeight() - 2, 0);
               var1.drawString(
                  getString(426).toUpperCase(), screenWidth - fontLargeBold.stringWidth(getString(426)) - 2, screenHeight - fontLargeBold.getHeight() - 2, 0
               );
               break;
            case 13:
               var5 = (screenHeight >> 1) - (fontLargeBold.getHeight() >> 1);
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.setFont(fontLargeBold);
               var1.drawString(getString(450), (screenWidth >> 1) - (fontLargeBold.stringWidth(getString(450)) >> 1), var5, 0);
               var1.setFont(fontSmallBold);
               var1.drawString(
                  getString(401), (screenWidth >> 1) - (fontSmallBold.stringWidth(getString(401)) >> 1), var5 + (fontLargeBold.getHeight() << 1), 0
               );
               break;
            case 14:
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.setFont(fontLargeBold);
               var1.drawString(
                  getString(451),
                  (screenWidth >> 1) - (fontLargeBold.stringWidth(getString(451)) >> 1),
                  (screenHeight >> 1) - (fontLargeBold.getHeight() >> 1),
                  0
               );
               var1.drawString(getString(427).toUpperCase(), 2, screenHeight - fontLargeBold.getHeight() - 2, 0);
               var1.drawString(
                  getString(426).toUpperCase(), screenWidth - fontLargeBold.stringWidth(getString(426)) - 2, screenHeight - fontLargeBold.getHeight() - 2, 0
               );
               break;
            case 15:
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.setFont(fontLargeBold);
               var1.drawString(
                  "Please Wait...",
                  (screenWidth >> 1) - (fontLargeBold.stringWidth("Please Wait...") >> 1),
                  (screenHeight >> 1) - (fontLargeBold.getHeight() >> 1),
                  0
               );
               SpriteRenderer.draw(
                  var1,
                  playerSprites,
                  5,
                  (screenWidth >> 1) - (SpriteRenderer.getWidth(playerSprites, 5) >> 1),
                  (screenHeight >> 1) + fontLargeBold.getHeight()
               );
               break;
            case 16:
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.setFont(fontLargeBold);
               var1.drawString(
                  getString(455),
                  (screenWidth >> 1) - (fontLargeBold.stringWidth(getString(455)) >> 1),
                  (screenHeight >> 1) - (fontLargeBold.getHeight() >> 1),
                  0
               );
               var1.drawString(
                  getString(464),
                  (screenWidth >> 1) - (fontLargeBold.stringWidth(getString(464)) >> 1),
                  (screenHeight >> 1) - (fontLargeBold.getHeight() >> 1) + fontLargeBold.getHeight(),
                  0
               );
               var1.drawString(getString(427).toUpperCase(), 2, screenHeight - fontLargeBold.getHeight() - 2, 0);
               var1.drawString(
                  getString(426).toUpperCase(), screenWidth - fontLargeBold.stringWidth(getString(426)) - 2, screenHeight - fontLargeBold.getHeight() - 2, 0
               );
               break;
            case 18:
               var1.setColor(0);
               var1.setFont(fontSmallBold);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.fillRect(5, 5, screenWidth - 10, fontSmallBold.getHeight() + 10);
               var1.setColor(14483456);
               var1.drawString(getString(helpTitleId), (screenWidth >> 1) - (fontSmallBold.stringWidth(getString(helpTitleId)) >> 1), 10, 0);
               var1.setColor(16777215);
               var5 = 35;

               for (var6 = helpScroll; var6 < helpPages[helpPage].length && helpPages[helpPage][var6] != null; var6++) {
                  String var45;
                  String var49 = var45 = helpPages[helpPage][var6];

                  int var18;
                  for (var18 = var45.length(); fontSmallBold.stringWidth(var45) > screenWidth - 20; var45 = var45.substring(0, var18)) {
                     var18 = var45.lastIndexOf(32, var18 - 1);
                  }

                  var1.drawString(var45, 10, var5, 0);
                  var5 += fontSmallBold.getHeight();
                  if (var18 < var49.length()) {
                     if (var5 + (fontSmallBold.getHeight() << 1) >= screenHeight) {
                        break;
                     }

                     var1.drawString(var49.substring(var18), 15, var5, 0);
                     var5 += fontSmallBold.getHeight();
                  }

                  if (var5 + (fontSmallBold.getHeight() << 1) >= screenHeight) {
                     break;
                  }
               }

               var1.drawString(getString(449).toUpperCase(), 2, screenHeight - fontSmallBold.getHeight() - 2, 0);
               if (helpScroll != 0) {
                  SpriteRenderer.draw(var1, this.hudSprites, 54, screenWidth - SpriteRenderer.getWidth(this.hudSprites, 54) - 2, 35);
               }

               if (var6 < helpPages[helpPage].length && helpPages[helpPage][var6] != null) {
                  SpriteRenderer.draw(
                     var1,
                     this.hudSprites,
                     53,
                     screenWidth - SpriteRenderer.getWidth(this.hudSprites, 53) - 2,
                     screenHeight
                        - fontSmallBold.getHeight()
                        - SpriteRenderer.getHeight(this.hudSprites, 53)
                        - SpriteRenderer.getHeight(this.hudSprites, 55)
                        - 4
                  );
                  helpHasMore = true;
               } else {
                  helpHasMore = false;
               }

               if (helpTitleId != 573) {
                  SpriteRenderer.draw(
                     var1, this.hudSprites, 56, 2, screenHeight - fontSmallBold.getHeight() - SpriteRenderer.getHeight(this.hudSprites, 53) - 4
                  );
                  SpriteRenderer.draw(
                     var1,
                     this.hudSprites,
                     55,
                     screenWidth - SpriteRenderer.getWidth(this.hudSprites, 55) - 2,
                     screenHeight - fontSmallBold.getHeight() - SpriteRenderer.getHeight(this.hudSprites, 53) - 4
                  );
               }
               break;
            case 19:
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.setFont(fontLargeBold);
               var1.drawString(
                  getString(475),
                  (screenWidth >> 1) - (fontLargeBold.stringWidth(getString(451)) >> 1),
                  (screenHeight >> 1) - (fontLargeBold.getHeight() >> 1),
                  0
               );
               var1.drawString(getString(427).toUpperCase(), 2, screenHeight - fontLargeBold.getHeight() - 2, 0);
               var1.drawString(
                  getString(426).toUpperCase(), screenWidth - fontLargeBold.stringWidth(getString(426)) - 2, screenHeight - fontLargeBold.getHeight() - 2, 0
               );
               break;
            case 20:
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.setFont(fontSmallBold);
               var1.drawString(
                  getString(566),
                  (screenWidth >> 1) - (fontSmallBold.stringWidth(getString(566)) >> 1),
                  (screenHeight >> 1) - (fontSmallBold.getHeight() >> 1),
                  0
               );
               var1.drawString(getString(567).toUpperCase(), 2, screenHeight - fontSmallBold.getHeight() - 2, 0);
               break;
            case 22:
               var1.setColor(0);
               var1.fillRect(0, 0, screenWidth, screenHeight);
               var1.setColor(16777215);
               var1.setFont(fontLargeBold);
               if (getString(571).length() == 0) {
                  Strings var16;
                  String var17 = (var16 = new Strings()).loadStartupLine((byte)2);
                  var1.drawString(
                     var17, (screenWidth >> 1) - (fontLargeBold.stringWidth(var17) >> 1), (screenHeight >> 1) - (fontLargeBold.getHeight() >> 1), 0
                  );
                  var17 = var16.loadStartupLine((byte)4);
                  var1.drawString(var17.toUpperCase(), 2, screenHeight - fontLargeBold.getHeight() - 2, 0);
                  var17 = var16.loadStartupLine((byte)3);
                  var1.drawString(var17.toUpperCase(), screenWidth - fontLargeBold.stringWidth(var17) - 2, screenHeight - fontLargeBold.getHeight() - 2, 0);
               } else {
                  var1.drawString(
                     getString(571),
                     (screenWidth >> 1) - (fontLargeBold.stringWidth(getString(571)) >> 1),
                     (screenHeight >> 1) - (fontLargeBold.getHeight() >> 1),
                     0
                  );
                  var1.drawString(getString(22).toUpperCase(), 2, screenHeight - fontLargeBold.getHeight() - 2, 0);
                  var1.drawString(
                     getString(426).toUpperCase(), screenWidth - fontLargeBold.stringWidth(getString(426)) - 2, screenHeight - fontLargeBold.getHeight() - 2, 0
                  );
               }
         }

         if (this.dialogue.open == 1) {
            this.dialogue.paint(var1);
         }
      }
   }

   private static final void isoToCell(int[] var0, int[] var1) {
      isoToWorld(tmpWorld, var0);
      var1[0] = tmpWorld[0] >> 7;
      var1[1] = tmpWorld[1] >> 7;
   }

   public static final void worldToIso(int[] var0, int[] var1) {
      var1[0] = var0[0] - var0[1] >> 3;
      var1[1] = var0[0] + var0[1] >> 4;
   }

   public static final void isoToWorld(int[] var0, int[] var1) {
      var0[0] = (var1[0] << 2) + (var1[1] << 3);
      var0[1] = (var1[1] << 3) - (var1[0] << 2);
   }

   public final void run() {
      long var1 = 0L;
      boolean var3 = false;
      boolean var4 = false;
      int[] var5 = new int[2];
      boolean var6 = false;
      this.lastTick = System.currentTimeMillis();

      try {
         while (this.running) {
            this.now = System.currentTimeMillis();
            var1 = this.now - this.lastTick;
            this.lastTick = this.now;
            if (this.suspended) {
               Thread.sleep(1000L);
            } else {
               if (state == 12) {
                  break;
               }

               if (this.dialogue.open == 1) {
                  this.dialogue.tick(var1);
               } else if (state != 3 && state != 10 && state != 9 && state != 13) {
                  ProjectileManager.tick(var1);
                  script.tick(var1);
               }

               this.handleInput(var1);
               if (blinkTimer >= 0) {
                  blinkTimer = (int)(blinkTimer - var1);
                  if (blinkTimer <= 0) {
                     blinkTimer = 500;
                     blinkOn = !blinkOn;
                  }
               }

               this.updateMessage(var1);

               for (int var14 = 0; var14 <= this.maxActorSlot && state == 0; var14++) {
                  if (actors[var14] != null) {
                     byte var7 = 0;
                     byte var8 = 0;
                     byte var9 = 0;
                     if (var14 == 0) {
                        var7 = actors[var14].enterScript;
                        var8 = actors[var14].leaveScript;
                     }

                     ActorSystem.update(actors[var14], var1, !this.dialogueOpen && this.inputEnabled);
                     if (actors[var14] != null && var14 == 0) {
                        if ((var9 = ActorSystem.checkTriggerTiles(actors[var14], this.enterLayer, this.leaveLayer)) != var7) {
                           if (var8 != 0 && var8 != -1 && var8 != -2) {
                              ScriptInterpreter.runScript((char)var8);
                           }

                           if (var9 != 0 && var9 != -1 && var9 != -2) {
                              ScriptInterpreter.runScript((char)var9);
                           } else if (var9 == -2) {
                              showMessage(getString(24), 60, 4, 0);
                           }
                        }

                        if (message == null || message.equals(getString(363))) {
                           var6 = false;
                        }

                        for (byte var15 = 0; !var6 && var15 < pickupCount; var15 += 3) {
                           var5[0] = pickups[var15 + 0] << 7;
                           var5[1] = pickups[var15 + 1] << 7;
                           if (Math.abs(ActorSystem.distance(var5, this.player.pos)) < 350) {
                              showMessage(getString(363), 60, 4, 0);
                              var6 = true;
                           }
                        }

                        if ((message == null || message.equals(getString(363))) && !var6) {
                           showMessage(null, 0, 0, 0);
                        }
                     }
                  }
               }

               if (state == 9 || state == 10 || state == 4 || state == 21) {
                  if (this.textScrollTimer > 100) {
                     this.textScrollY--;
                     this.textScrollTimer = 0;
                  }

                  this.textScrollTimer = (short)(this.textScrollTimer + var1);
               }

               if (state == 15) {
                  this.spinnerTimer = (short)(this.spinnerTimer + var1);
                  if (this.spinnerTimer >= 200) {
                     SpriteRenderer.advanceFrame(playerSprites, 5);
                     this.spinnerTimer = 0;
                  }
               }

               this.repaint();
               this.serviceRepaints();
               System.gc();
            }
         }
      } catch (Exception var11) {
         this.errorText = "Code: " + errorCode;

         for (int var13 = 0; var13 < stageTraceLen - 1; var13++) {
            this.errorText = this.errorText + (var13 != 0 ? "." : "") + stageTrace[var13];
         }

         errorCode = var11.toString();
         this.repaint();
         this.serviceRepaints();
         var11.printStackTrace();
      }
   }

   private final void updateMessage(long var1) {
      if (message != null) {
         if (messageStyle == 1) {
            if (messageBlinkTimer >= 500) {
               messageBlank = !messageBlank;
               messageBlinkTimer = 0;
            }

            messageBlinkTimer = (int)(messageBlinkTimer + var1);
         } else if (messageStyle == 2) {
            if (messageScrollTimer >= 50) {
               if ((messageX += 2) == -1) {
                  messageX++;
               }

               messageScrollTimer = 0;
            }

            messageScrollTimer = (int)(messageScrollTimer + var1);
         } else if (messageStyle == 3) {
            if (messageScrollTimer >= 50) {
               if ((messageX -= 2) == -1) {
                  messageX--;
               }

               messageScrollTimer = 0;
            }

            messageScrollTimer = (int)(messageScrollTimer + var1);
         }

         if (messageElapsed > messageDuration) {
            message = null;
            messageElapsed = 0;
            messageDuration = 0;
            messageColor = 0;
            messageX = -1;
            messageY = -1;
            return;
         }

         messageElapsed = (int)(messageElapsed + var1);
      }
   }

   private final void openInventory() {
      int[] var1 = null;
      int[] var2 = null;
      boolean var3 = false;
      int var4 = 0;
      char var5 = '\u0000';
      char var6 = '\u0000';
      boolean var7 = false;
      DialogueNode[] var8 = new DialogueNode[]{
         new DialogueNode(getString(25), null, false),
         new DialogueNode(getString(26), null, false),
         new DialogueNode(getString(27), null, false),
         new DialogueNode(getString(394), null, false)
      };
      DialogueNode[] var9 = new DialogueNode[]{
         new DialogueNode(getString(28), null, false),
         new DialogueNode(getString(29), null, false),
         new DialogueNode(getString(30), null, false),
         new DialogueNode(getString(31), null, false),
         new DialogueNode(getString(32), null, false),
         new DialogueNode(getString(33), null, false),
         new DialogueNode(getString(34), null, false),
         new DialogueNode(getString(35), null, false)
      };

      for (int var24 = 0; var24 < var9.length; var24++) {
         DialogueNode var10 = var9[var24];
         DialogueNode var11;
         (var11 = var8[1]).children.addElement(var10);
         var10.parent = var11;
      }

      String var28 = null;
      if (this.player.classId == 4) {
         var28 = getString(12);
      } else if (this.player.classId == 3) {
         var28 = getString(11);
      } else if (this.player.classId == 8) {
         var28 = getString(16);
      } else if (this.player.classId == 5) {
         var28 = getString(13);
      } else if (this.player.classId == 1) {
         var28 = getString(9);
      } else if (this.player.classId == 2) {
         var28 = getString(10);
      } else if (this.player.classId == 7) {
         var28 = getString(15);
      } else if (this.player.classId == 6) {
         var28 = getString(14);
      }

      var8[3].answerLines = new String[]{
         getString(443) + ": ",
         var28,
         getString(17) + ": ",
         Integer.toString(this.player.level),
         getString(441) + ": ",
         Integer.toString(this.player.xp),
         getString(442) + ": ",
         this.player.level < ActorSystem.xpForLevel.length - 1 ? Integer.toString(ActorSystem.xpForLevel[this.player.level + 1] - this.player.xp) : "0",
         getString(415) + ": ",
         Integer.toString(this.player.strength * 3),
         getString(416) + ": ",
         Integer.toString(this.player.intelligence * 3),
         getString(417) + ": ",
         Integer.toString(this.player.willpower * 3),
         getString(418) + ": ",
         Integer.toString(this.player.agility * 3),
         getString(419) + ": ",
         Integer.toString(this.player.endurance * 3),
         getString(420) + ": ",
         Integer.toString(this.player.personality * 3),
         getString(431) + ": ",
         Integer.toString(this.player.defenseRating * 3),
         getString(432) + ": ",
         Integer.toString(this.player.attackRating * 3),
         getString(563) + ": ",
         "42",
         getString(562) + ": ",
         "40",
         getString(38) + ": ",
         Integer.toString(gold)
      };
      boolean[] var29 = new boolean[]{false, false, false, false, false, false, false, false, false};
      boolean var12 = false;
      DialogueNode var13 = null;
      boolean var14 = false;
      boolean var15 = false;
      if (this.player != null) {
         for (; this.player.inventory[var4] != 0; var4++) {
            var5 = (char)(this.player.inventory[var4] >> 8 & 0xFF);
            var6 = (char)(this.player.inventory[var4] >> 0 & 0xFF);
            switch (var5) {
               case '\u0000':
                  if ((var2 = script.getRow(4, var6))[2] == 4) {
                     (var13 = new DialogueNode(
                           getString(400) + script.getItemName(var2[1]),
                           getString(432) + ": " + var2[3],
                           ActorSystem.isWeaponRowEquipped(this.player, var2, false) && !var12
                        ))
                        .available = ActorSystem.canUseItem(this.player, 0, var2);
                     DialogueNode var35;
                     (var35 = var8[0]).children.addElement(var13);
                     var13.parent = var35;
                  } else {
                     var13 = new DialogueNode(
                        getString(305) + script.getItemName(var2[1]),
                        getString(432) + ": " + var2[3],
                        ActorSystem.isWeaponRowEquipped(this.player, var2, false) && !var12
                     );
                     if (ActorSystem.isWeaponRowEquipped(this.player, var2, false)) {
                        equippedWeaponNode = var13;
                     }

                     var13.available = ActorSystem.canUseItem(this.player, 0, var2);
                     DialogueNode var34;
                     (var34 = var8[0]).children.addElement(var13);
                     var13.parent = var34;
                  }

                  var12 |= ActorSystem.isWeaponRowEquipped(this.player, var2, false);
                  break;
               case '\u0001':
                  var2 = script.getRow(1, var6);
                  boolean var16 = ActorSystem.hasArmor(this.player, var2[0]);
                  (var13 = new DialogueNode(script.getItemName(var2[1]), getString(444) + ": " + var2[4], var16 & !var29[var2[3]])).available = ActorSystem.canUseItem(
                     this.player, 1, var2
                  );
                  DialogueNode var17;
                  (var17 = var9[var2[3]]).children.addElement(var13);
                  var13.parent = var17;
                  var29[var2[3]] = var29[var2[3]] | var16;
                  break;
               case '\u0002':
                  var2 = script.getRow(2, var6);
                  DialogueNode var18 = new DialogueNode(script.getItemName(var2[1]), null, false);
                  if (var2 == this.player.hpPotion && !var14) {
                     var18.marked = true;
                     var14 = true;
                  }

                  if (var2 == this.player.mpPotion && !var15) {
                     var18.marked = true;
                     var15 = true;
                  }

                  DialogueNode var19;
                  (var19 = var8[2]).children.addElement(var18);
                  var18.parent = var19;
            }
         }

         for (int var27 = 0; var27 < this.player.classList.length && this.player.classList[var27] != -1; var27++) {
            var1 = script.getRow(8, this.player.classList[var27]);
            var13 = new DialogueNode(getString(304) + script.getItemName(var1[1]), null, ActorSystem.isWeaponRowEquipped(this.player, var1, true));
            DialogueNode var36;
            (var36 = var8[0]).children.addElement(var13);
            var13.parent = var36;
            if (ActorSystem.isWeaponRowEquipped(this.player, var1, true)) {
               equippedSpellNode = var13;
            }
         }
      }

      this.dialogue.openTabs(new byte[]{4, 1, 2, 3, 18}, var8, null, this.offscreenImage, this.offscreenGraphics);
      this.redraw = true;
   }

   private final void openShop() {
      DialogueNode[] var1 = new DialogueNode[]{new DialogueNode(getString(36), null, false), new DialogueNode(getString(37), null, false)};
      int[] var2 = script.getRow(7, 0);
      int[] var3 = null;
      int var4 = 0;
      DialogueNode var5 = null;

      while (var2[var4] != -1) {
         switch (var2[var4++]) {
            case 0:
               var3 = script.getRow(4, var2[var4++]);
               (var5 = new DialogueNode(script.getItemName(var3[1]) + " : " + var3[7] + " " + getString(38), getString(432) + ": " + var3[3], false)).available = ActorSystem.canUseItem(
                  this.player, 0, var3
               );
               DialogueNode var6;
               (var6 = var1[0]).children.addElement(var5);
               var5.parent = var6;
               break;
            case 1:
               var3 = script.getRow(1, var2[var4++]);
               (var5 = new DialogueNode(script.getItemName(var3[1]) + " : " + var3[9] + " " + getString(38), getString(444) + ": " + var3[4], false)).available = ActorSystem.canUseItem(
                  this.player, 1, var3
               );
               DialogueNode var7;
               (var7 = var1[0]).children.addElement(var5);
               var5.parent = var7;
               break;
            case 2:
               var3 = script.getRow(2, var2[var4++]);
               DialogueNode var8 = new DialogueNode(script.getItemName(var3[1]) + " : " + var3[13] + " " + getString(38), null, false);
               DialogueNode var9;
               (var9 = var1[0]).children.addElement(var8);
               var8.parent = var9;
               break;
            case 3:
               var3 = script.getRow(3, var2[var4++]);
               DialogueNode var10 = new DialogueNode(script.getItemName(var3[1]) + " : " + var3[2] + " " + getString(38), null, false);
               DialogueNode var11;
               (var11 = var1[0]).children.addElement(var10);
               var10.parent = var11;
         }
      }

      if (this.player != null) {
         for (int var19 = 0; this.player.inventory[var19] != 0; var19++) {
            char var24 = (char)(this.player.inventory[var19] >> 8 & 0xFF);
            char var25 = (char)(this.player.inventory[var19] >> 0 & 0xFF);
            switch (var24) {
               case '\u0000':
                  var3 = script.getRow(4, var25);
                  (var5 = new DialogueNode(script.getItemName(var3[1]) + " : " + (var3[7] >> 2) + " " + getString(38), getString(432) + ": " + var3[3], false)).available = ActorSystem.canUseItem(
                     this.player, 0, var3
                  );
                  DialogueNode var26;
                  (var26 = var1[1]).children.addElement(var5);
                  var5.parent = var26;
                  break;
               case '\u0001':
                  var3 = script.getRow(1, var25);
                  (var5 = new DialogueNode(script.getItemName(var3[1]) + " : " + (var3[9] >> 2) + " " + getString(38), getString(444) + ": " + var3[4], false)).available = ActorSystem.canUseItem(
                     this.player, 1, var3
                  );
                  DialogueNode var27;
                  (var27 = var1[1]).children.addElement(var5);
                  var5.parent = var27;
                  break;
               case '\u0002':
                  var3 = script.getRow(2, var25);
                  DialogueNode var28 = new DialogueNode(script.getItemName(var3[1]) + " : " + (var3[13] >> 2) + " " + getString(38), null, false);
                  DialogueNode var29;
                  (var29 = var1[1]).children.addElement(var28);
                  var28.parent = var29;
            }
         }
      }

      this.dialogue.openTabs(new byte[]{17, 15, 16}, var1, getString(38) + " : " + gold, this.offscreenImage, this.offscreenGraphics);
      this.redraw = true;
   }

   public final void keyReleased(int var1) {
      this.keyHandled = 1;
   }

   public static final void setState(byte var0) {
      if (stateChangesEnabled) {
         if (state != 12) {
            if (state == 0) {
               stateFlagF = true;
            }

            byte var1 = state;
            state = var0;
            if (state == 3 && titleImage == null) {
               if (!stateFlagF && menuId != 4) {
                  try {
                     titleImage = Image.createImage("/main.png");
                  } catch (IOException var3) {
                     var3.printStackTrace();
                  }
               }
            } else {
               if (var1 != 22) {
                  titleImage = null;
               }

               collectGarbage();
            }

            if (state == 9) {
               stateFlagF = false;
               setTextScreen(getString(547));
            } else if (state == 4) {
               setTextScreen(getString(548));
               instance.textScrollY = (short)(screenHeight - (fontSmallBold.getHeight() << 2));
               instance.textScrollTimer = 0;
            } else if (state == 21) {
               setTextScreen(new Strings().loadCopywrite());
               instance.textScrollY = 0;
            } else if (state == 23) {
               setTextScreen(getString(574));
               instance.textScrollY = 15;
               instance.textAtEnd = false;
            } else {
               if (state == 17) {
                  setTextScreen(getString(465));
                  instance.textScrollY = 15;
                  instance.textAtEnd = false;
               }
            }
         }
      }
   }

   private static final void setTextScreen(String var0) {
      boolean var1 = false;
      int var2 = 0;
      int var3 = 0;
      Vector var4 = new Vector();
      if ((var2 = var0.indexOf("VERSION")) != -1) {
         var0 = var0.substring(0, var2) + version + var0.substring(var2 + 7);
      }

      while ((var2 = var0.indexOf("\\n", var3)) != -1) {
         var4.addElement(var0.substring(var3, var2));
         var3 = var2 + 2;
      }

      var4.addElement(var0.substring(var3));
      instance.textLines = new Vector[var4.size()];
      setSpeakerName(null);

      for (int var5 = 0; var5 < var4.size(); var5++) {
         instance.textLines[var5] = new Vector();
         wrapText((String)var4.elementAt(var5), instance.textLines[var5], screenWidth - 10);
      }
   }

   public static final void collectGarbage() {
      System.gc();
      Runtime.getRuntime().gc();

      try {
         Thread.sleep(100L);
      } catch (Exception var1) {
      }

      System.gc();
      Runtime.getRuntime().gc();
   }

   private final int mapKey(int var1) {
      int var2 = -1122868;
      if (var1 == 1 || var1 == 50) {
         var2 = 3;
      } else if (var1 == 6 || var1 == 56) {
         var2 = 4;
      } else if (var1 == 2 || var1 == 52) {
         var2 = 5;
      } else if (var1 == 5 || var1 == 54) {
         var2 = 6;
      } else if (var1 == 8 || var1 == 20 || var1 == 53) {
         var2 = 7;
      } else if (var1 == keyBindings[0]) {
         var2 = 0;
      } else if (var1 == keyBindings[1]) {
         var2 = 1;
      } else if (var1 == keyBindings[2]) {
         var2 = 2;
      }

      return var2;
   }

   public final void keyPressed(int var1) {
      keyState = var1;
      if (this.errorText != null && ++this.crashKeyCount == 3) {
         this.quit();
      }
   }

   private void resetMenu() {
      boolean var1 = false;

      for (int var2 = 0; var2 < menuSelection.length; var2++) {
         menuSelection[var2] = 0;
      }

      if (stateFlagF) {
         menuId = 5;
      } else {
         menuId = 0;
      }
   }

   private final void handleInput(long var1) {
      if (keyState == -keyOk || keyState == -keyRightSoft || keyState == -keyLeftSoft) {
         keyState = -keyState;
      }

      int var3 = keyState;
      int var4 = 0;

      try {
         var4 = this.getGameAction(var3);
      } catch (Exception var15) {
      }

      if (keyState == keyOk
         || keyState == keyRightSoft
         || keyState == keyLeftSoft
         || keyState == keyD
         || keyState == keyE
         || keyState >= 48 && keyState <= 57
         || var4 == 8
         || var4 == 1
         || var4 == 6
         || var4 == 2
         || var4 == 5) {
         int var5;
         if (keyState != keyD && keyState != keyE) {
            byte var6 = 0;
            byte var7 = 0;
            byte var8 = 0;
            boolean var9 = false;
            boolean var10 = false;
            if (keyState == -286331154 || keyState == keyOk) {
               this.keyHandled = 0;
               return;
            }

            var5 = keyState >= 0 ? keyState : this.getGameAction(keyState);
            switch (state) {
               case 0:
                  if (keyState == keyRightSoft) {
                     if (this.player != null && hudVisible) {
                        this.openInventory();
                        setState((byte)2);
                        keyState = -286331154;
                     }
                  } else if (keyState == keyLeftSoft) {
                     this.buildMenus();
                     menuId = 5;

                     for (int var24 = 0; var24 < menuSelection.length; var24++) {
                        menuSelection[var24] = 0;
                     }

                     setState((byte)3);
                     keyState = -286331154;
                  } else {
                     var5 = this.mapKey(var5);
                     if (this.dialogueOpen || this.dialogue.open != 0 || !this.inputEnabled || this.player == null || this.player.dead != 0) {
                        break;
                     }

                     var6 = this.player.enterScript;
                     var7 = this.player.leaveScript;
                     var8 = this.player.zoneId;
                     if (ActorSystem.handleAction(this.player, var5, var1)) {
                        keyState = -286331154;
                        this.keyHandled = 0;
                     }

                     if (var8 != this.player.zoneId && this.player.zoneId != 0 && this.player.zoneId != -1 && this.player.zoneId != -2) {
                        ScriptInterpreter.runScript((char)this.player.zoneId);
                     }

                     if (var5 == 0) {
                        ActorSystem.quaffPotion(this.player, true);
                        keyState = -286331154;
                        this.keyHandled = 0;
                     } else if (var5 == 1) {
                        ActorSystem.quaffPotion(this.player, false);
                        keyState = -286331154;
                        this.keyHandled = 0;
                     } else if (var5 == 7 && this.player.killTimer > 1000) {
                        int[] var26 = new int[2];

                        for (byte var23 = 0; var23 < pickupCount; var23 += 3) {
                           var26[0] = pickups[var23 + 0] << 7;
                           var26[1] = pickups[var23 + 1] << 7;
                           int[] var13;
                           if (ActorSystem.distance(var26, this.player.pos) < 350 && (var13 = script.getRow(6, pickups[var23 + 2])) != null) {
                              ProjectileManager.spawn(8, var26[0], var26[1] + 128);
                              if (((byte[])layers.elementAt(layers.size() - 1))[pickups[var23 + 0] * gridHeight + pickups[var23 + 1]] == -45) {
                                 ((byte[])layers.elementAt(layers.size() - 1))[pickups[var23 + 0] * gridHeight + pickups[var23 + 1]] = -44;
                              } else {
                                 ((byte[])layers.elementAt(layers.size() - 1))[pickups[var23 + 0] * gridHeight + pickups[var23 + 1]] = 0;
                              }

                              for (byte var25 = var23; var25 < pickupCount && var25 < pickups.length - 3; var25 += 3) {
                                 pickups[var25 + 0] = pickups[var25 + 3];
                                 pickups[var25 + 1] = pickups[var25 + 4];
                                 pickups[var25 + 2] = pickups[var25 + 5];
                              }

                              pickupCount = (byte)(pickupCount - 3);
                              if (var13[2] > 0) {
                                 showMessage(var13[2] + " " + getString(38), 3, 4, 0);
                                 gold = gold + var13[2];
                              } else if (var13[4] > 0) {
                                 int[] var14 = script.getRow(1, var13[4]);
                                 showMessage(script.getItemName(var14[1]), 3, 4, 0);
                                 ActorSystem.addItem(this.player, 1, var14);
                              } else if (var13[3] > 0) {
                                 int[] var27 = script.getRow(4, var13[3]);
                                 showMessage(script.getItemName(var27[1]), 3, 4, 0);
                                 ActorSystem.addItem(this.player, 0, var27);
                              } else if (var13[5] > 0) {
                                 int[] var28 = script.getRow(2, var13[5]);
                                 showMessage(script.getItemName(var28[1]), 3, 4, 0);
                                 ActorSystem.addItem(this.player, 2, var28);
                              }
                              break;
                           }
                        }
                     }

                     ActorSystem.checkTriggerTiles(this.player, this.enterLayer, this.leaveLayer);
                     if (this.player.enterScript == var6) {
                        break;
                     }

                     if (var7 != 0 && var7 != -1 && var7 != -2) {
                        ScriptInterpreter.runScript((char)var7);
                     } else if (var7 == -2) {
                        showMessage(null, 0, 0, 0);
                     }

                     if (this.player.enterScript != 0 && this.player.enterScript != -1 && this.player.enterScript != -2) {
                        ScriptInterpreter.runScript((char)this.player.enterScript);
                     } else if (this.player.enterScript == -2) {
                        showMessage(getString(24), 60, 4, 0);
                     }
                  }
                  break;
               case 1:
                  var5 = this.mapKey(var5);
                  if (keyState == keyLeftSoft) {
                     this.dialogue.open = 0;
                     setState((byte)3);
                  } else if (keyState != keyRightSoft && this.dialogue.open == 1) {
                     this.dialogue.handleKey((char)var5);
                  }

                  this.keyHandled = 1;
                  break;
               case 2:
                  var5 = this.mapKey(var5);
                  if (keyState == keyLeftSoft) {
                     if (!this.dialogue.goBack()) {
                        this.dialogue.open = 0;
                        setState((byte)0);
                     } else {
                        keyState = -286331154;
                     }
                  } else if (keyState != keyRightSoft && this.dialogue.open == 1) {
                     this.dialogue.handleKey((char)var5);
                  }

                  this.keyHandled = 1;
                  break;
               case 3:
                  if ((var5 = this.mapKey(var5)) == 5) {
                     if (--menuSelection[menuId] == -1) {
                        menuSelection[menuId] = (byte)(menus[menuId].length - 1);
                     }
                  } else if (var5 == 6) {
                     if (++menuSelection[menuId] == menus[menuId].length) {
                        menuSelection[menuId] = 0;
                     }
                  } else if (keyState == keyLeftSoft && state == 3) {
                     if (menuId == 6) {
                        menuId = menuReturn;
                     } else if (menuId == 1) {
                        if (stateFlagF) {
                           menuId = 5;
                        } else {
                           menuId = 0;
                        }
                     }
                  } else if (var5 == 7) {
                     if (menuId == 2) {
                        this.loadProgress = 0;
                        setState((byte)6);
                        this.repaint();
                        this.serviceRepaints();

                        for (int var20 = 0; var20 < actors.length; var20++) {
                           this.player = actors[var20] = null;
                        }

                        this.loadLevel(menus[3][menuSelection[menuId]]);
                     } else if (menus[menuId][menuSelection[menuId]].startsWith(getString(4))) {
                        this.soundEnabled = !this.soundEnabled;
                        this.buildMenus();
                        this.saveGame();
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(19))) {
                        this.saveGame();
                        menuSelection[menuId] = 2;
                        setState((byte)13);
                        keyState = -286331154;
                        this.keyHandled = 0;
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(3))) {
                        this.dropHelpCaches();
                        setState((byte)14);
                        keyState = -286331154;
                        this.keyHandled = 0;
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(21))) {
                        setState((byte)0);
                        this.suspended = false;
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(2))) {
                        if (this.hasSavedGame()) {
                           setState((byte)16);
                        } else {
                           menuId = 1;
                        }
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(6))) {
                        setState((byte)4);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(456))) {
                        menuReturn = menuId;
                        menuId = 6;
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(457))) {
                        helpTitleId = 457;
                        helpScroll = 0;
                        helpPage = 0;
                        setState((byte)17);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(458))) {
                        for (int var21 = 0; var21 < keyBindingsEdit.length; var21++) {
                           keyBindingsEdit[var21] = keyBindings[var21];
                        }

                        keyState = -286331154;
                        this.keyHandled = 0;
                        setState((byte)5);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(573))) {
                        helpTitleId = 573;
                        helpScroll = 0;
                        helpPage = 0;
                        helpPages = getHelpOverview();
                        setState((byte)23);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(522))) {
                        helpTitleId = 522;
                        helpScroll = 0;
                        helpPage = 0;
                        helpPages = getHelpClasses();
                        setState((byte)18);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(459))) {
                        helpTitleId = 459;
                        helpScroll = 0;
                        helpPage = 0;
                        helpPages = getHelpWeapons();
                        setState((byte)18);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(460))) {
                        helpTitleId = 460;
                        helpScroll = 0;
                        helpPage = 0;
                        helpPages = getHelpArmor();
                        setState((byte)18);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(461))) {
                        helpTitleId = 461;
                        helpScroll = 0;
                        helpPage = 0;
                        helpPages = getHelpSpells();
                        setState((byte)18);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(462))) {
                        helpTitleId = 462;
                        helpScroll = 0;
                        helpPage = 0;
                        helpPages = getHelpItems();
                        setState((byte)18);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(18))) {
                        this.openShop();
                        setState((byte)1);
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(20))) {
                        setState((byte)0);
                     } else if (menuId == 1) {
                        this.dropHelpCaches();
                        if (this.newGameLocked) {
                           menuId++;
                        } else {
                           this.loadProgress = 0;
                           setState((byte)6);
                           this.repaint();
                           this.serviceRepaints();

                           for (int var22 = 0; var22 < actors.length; var22++) {
                              this.player = actors[var22] = null;
                           }

                           this.loadLevel("/l01_1.scr");
                        }

                        gold = 100;
                     } else if (menus[menuId][menuSelection[menuId]].equals(getString(22))) {
                        setState((byte)19);
                     }
                  }

                  this.keyHandled = 1;
                  break;
               case 4:
               case 9:
               case 10:
               case 17:
               case 23:
                  var5 = this.mapKey(var5);
                  long var11 = var1 / 10L;
                  if (var5 == 3) {
                     this.textScrollTimer = 0;
                     this.textScrollY = (short)Math.min(screenHeight - (fontSmallBold.getHeight() << 2), this.textScrollY + var11);
                  } else if (var5 == 4 && (state != 23 || !this.textAtEnd) && (state != 17 || !this.textAtEnd)) {
                     this.textScrollTimer = 0;
                     this.textScrollY = (short)(this.textScrollY - var11);
                  }

                  if (state != 4 && state != 23 && state != 17 || keyState != keyLeftSoft) {
                     break;
                  }

                  if (state == 23 || state == 17) {
                     menuId = 6;
                     keyState = -286331154;
                     this.keyHandled = 0;
                  } else if (stateFlagF) {
                     menuId = 5;
                  } else {
                     menuId = 0;
                  }

                  setState((byte)3);
                  break;
               case 5:
                  var5 = this.mapKey(var5);
                  if (editingKey) {
                     if (keyState != keyLeftSoft && keyState != keyRightSoft) {
                        if (this.isBindableKey(keyState, var5)) {
                           if (keyState >= 0) {
                              keyBindingsEdit[optionsCursor] = (byte)keyState;
                           } else {
                              keyBindingsEdit[optionsCursor] = (byte)this.getGameAction(keyState);
                           }
                        } else {
                           setState((byte)20);
                        }

                        editingKey = false;
                     }
                  } else if (keyState == keyLeftSoft) {
                     setState((byte)3);
                  } else if (var5 == 3) {
                     optionsCursor = Math.max(0, optionsCursor - 1);
                  } else if (var5 == 4) {
                     optionsCursor = Math.min(optionLabels.length - 1, optionsCursor + 1);
                  } else if (var5 == 7) {
                     if (optionLabels[optionsCursor].equals(getString(294))) {
                        for (int var19 = 0; var19 < keyBindingsEdit.length; var19++) {
                           keyBindings[var19] = keyBindingsEdit[var19];
                        }

                        editingKey = false;
                        setState((byte)3);
                        this.saveGame();
                     } else {
                        editingKey = true;
                     }
                  }

                  this.keyHandled = 1;
               case 6:
               case 7:
               case 8:
               case 12:
               case 15:
               case 21:
               default:
                  break;
               case 11:
                  if (keyState == keyRightSoft) {
                     setState((byte)0);
                  } else if (keyState == keyLeftSoft) {
                     this.resetMenu();
                     setState((byte)3);
                  }

                  this.keyHandled = 1;
                  break;
               case 13:
                  setState((byte)3);
                  this.keyHandled = 1;
                  break;
               case 14:
                  if (keyState == keyRightSoft) {
                     this.loadSavedGame();
                  } else if (keyState == keyLeftSoft) {
                     this.resetMenu();
                     setState((byte)3);
                     menuSelection[0] = 1;
                  }

                  this.keyHandled = 1;
                  break;
               case 16:
                  if (keyState == keyRightSoft) {
                     menuId = 1;
                     setState((byte)3);
                     keyState = -286331154;
                     this.keyHandled = 0;
                  } else if (keyState == keyLeftSoft) {
                     setState((byte)3);
                     keyState = -286331154;
                     this.keyHandled = 0;
                  }
                  break;
               case 18:
                  var5 = this.mapKey(var5);
                  if (keyState == keyLeftSoft) {
                     setState((byte)3);
                  } else if (var5 == 5) {
                     helpScroll = 0;
                     if (--helpPage < 0) {
                        helpPage = (byte)(helpPages.length - 1);
                     }
                  } else if (var5 == 6) {
                     helpScroll = 0;
                     if (++helpPage > helpPages.length - 1) {
                        helpPage = 0;
                     }
                  } else if (var5 == 3) {
                     if (--helpScroll < 0) {
                        helpScroll = 0;
                     }
                  } else if (var5 == 4 && helpHasMore) {
                     helpScroll++;
                  }

                  keyState = -286331154;
                  this.keyHandled = 0;
                  break;
               case 19:
                  if (keyState == keyRightSoft) {
                     this.quit();
                  } else if (keyState == keyLeftSoft) {
                     setState((byte)3);
                     keyState = -286331154;
                     this.keyHandled = 0;
                  }
                  break;
               case 20:
                  if (keyState == keyLeftSoft) {
                     setState((byte)5);
                     keyState = -286331154;
                     this.keyHandled = 0;
                  }
                  break;
               case 22:
                  if (keyState == keyRightSoft) {
                     setState(pausedState);
                     pausedState = -1;
                     DialogueScreen.showPauseOverlay = false;
                     keyState = -286331154;
                     this.keyHandled = 0;
                  } else if (keyState == keyLeftSoft) {
                     this.quit();
                  }
            }

            if (keyState == -286331154) {
               return;
            }
         } else {
            if (state != 8) {
               return;
            }

            var5 = 0;
         }

         if (this.dialogue.open == 0) {
            script.keyPressed((char)var5);
            this.handleDialogueKey((char)var5);
         }

         if (this.keyHandled != 0) {
            keyState = -286331154;
            this.keyHandled = 0;
         }
      }
   }

   private final boolean isBindableKey(int var1, int var2) {
      boolean var3 = false;
      var1 = var1 >= 0 ? var1 : this.getGameAction(var1);

      for (int var5 = 0; var5 < keyBindingsEdit.length; var5++) {
         if (var5 != optionsCursor && var1 == keyBindingsEdit[var5]) {
            return false;
         }
      }

      if (var2 == 3) {
         return false;
      } else if (var2 == 4) {
         return false;
      } else if (var2 == 5) {
         return false;
      } else if (var2 == 6) {
         return false;
      } else if (var2 == 7) {
         return false;
      } else {
         return var1 == keyRightSoft ? false : var1 != keyLeftSoft;
      }
   }

   private static final String[][] getHelpOverview() {
      if (helpOverview != null) {
         return helpOverview;
      }

      helpOverview = new String[1][1];
      helpOverview[0][0] = getString(574);
      return helpOverview;
   }

   private static final String[][] getHelpClasses() {
      int var0 = 0;
      boolean var1 = false;
      boolean var2 = false;
      boolean var3 = false;
      if (helpClasses != null) {
         return helpClasses;
      }

      helpClasses = new String[script.classBase.length - 1][];

      for (int var16 = 1; var16 < script.classBase.length; var16++) {
         var0 = 0;
         helpClasses[var16 - 1] = new String[30];
         String[] var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[0] = getString(481) + script.getItemName(script.classBase[var16][1]);
         var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[1] = getString(415) + ": " + script.classBase[var16][7] * 3;
         var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[2] = getString(416) + ": " + script.classBase[var16][8] * 3;
         var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[3] = getString(417) + ": " + script.classBase[var16][9] * 3;
         var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[4] = getString(418) + ": " + script.classBase[var16][10] * 3;
         var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[5] = getString(419) + ": " + script.classBase[var16][11] * 3;
         var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[6] = getString(420) + ": " + script.classBase[var16][12] * 3;
         var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[7] = getString(305) + script.getItemName(script.weapons[script.classBase[var16][4]][1]);
         var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[8] = getString(499) + script.getItemName(script.armors[script.classBase[var16][5]][1]);
         var10000 = helpClasses[var16 - 1];
         var0++;
         var10000[9] = getString(538);

         for (int var17 = 0; var17 < 15 && script.classItemTypes[var16][var17] != -1; var17++) {
            helpClasses[var16 - 1][var0++] = "   " + ScriptInterpreter.getSkillName(script.classItemTypes[var16][var17]);
         }

         helpClasses[var16 - 1][var0++] = getString(539);

         for (int var18 = 0; var18 < script.classLists.length; var18++) {
            if (script.classLists[var16][var18] == -1) {
               if (var18 == 0) {
                  helpClasses[var16 - 1][var0] = "   " + getString(572);
               }
               break;
            }

            helpClasses[var16 - 1][var0++] = "   " + script.getItemName(script.specials[script.classLists[var16][var18]][1]);
         }
      }

      return helpClasses;
   }

   private static final String[][] getHelpItems() {
      int var0 = 0;
      boolean var1 = false;
      if (helpItems != null) {
         return helpItems;
      }

      helpItems = new String[script.consumables.length - 1][];

      for (int var6 = 1; var6 < script.consumables.length; var6++) {
         var0 = 0;
         helpItems[var6 - 1] = new String[10];
         String[] var10000 = helpItems[var6 - 1];
         var0++;
         var10000[0] = getString(481) + script.getItemName(script.consumables[var6][1]);
         var10000 = helpItems[var6 - 1];
         var0++;
         var10000[1] = getString(484) + script.consumables[var6][13];
         var10000 = helpItems[var6 - 1];
         var0++;
         var10000[2] = getString(485) + (script.consumables[var6][13] >> 2);
         if (script.consumables[var6][2] > 0) {
            var10000 = helpItems[var6 - 1];
            var0++;
            var10000[3] = getString(496) + script.consumables[var6][2];
         }

         if (script.consumables[var6][3] > 0) {
            helpItems[var6 - 1][var0++] = getString(497) + script.consumables[var6][3];
         }

         if (script.consumables[var6][6] > 0) {
            helpItems[var6 - 1][var0++] = getString(498) + script.consumables[var6][6];
         }

         if (script.consumables[var6][7] > 0) {
            helpItems[var6 - 1][var0++] = getString(499) + script.consumables[var6][7];
         }

         if (script.consumables[var6][8] > 0) {
            helpItems[var6 - 1][var0++] = getString(500) + script.consumables[var6][8];
         }

         if (script.consumables[var6][10] > 0) {
            helpItems[var6 - 1][var0++] = getString(501) + script.consumables[var6][10];
         }

         if (script.consumables[var6][5] > 0) {
            helpItems[var6 - 1][var0] = getString(502) + script.consumables[var6][5] / 1000 + getString(521);
         }
      }

      return helpItems;
   }

   private static final String[][] getHelpSpells() {
      String var0 = null;
      String var1 = null;
      boolean var2 = false;
      if (helpSpells != null) {
         return helpSpells;
      }

      helpSpells = new String[script.specials.length - 1][];

      for (int var3 = 1; var3 < script.specials.length; var3++) {
         if (script.specials[var3][2] == 0) {
            var0 = getString(437);
         } else if (script.specials[var3][2] == 3) {
            var0 = getString(439);
         } else if (script.specials[var3][2] == 6) {
            var0 = getString(503);
         } else if (script.specials[var3][2] == 5) {
            var0 = getString(433);
         } else if (script.specials[var3][2] == 4) {
            var0 = getString(504);
         } else if (script.specials[var3][2] == 1) {
            var0 = getString(437);
         } else if (script.specials[var3][2] == 2) {
            var0 = getString(505);
         }

         if (script.specials[var3][7] == 0) {
            var1 = getString(506);
         } else if (script.specials[var3][7] == 1) {
            var1 = getString(507);
         } else if (script.specials[var3][7] == 2) {
            var1 = getString(508);
         } else if (script.specials[var3][7] == 3) {
            var1 = getString(509);
         } else if (script.specials[var3][7] == 4) {
            var1 = getString(510);
         }

         helpSpells[var3 - 1] = new String[]{
            getString(481) + script.getItemName(script.specials[var3][1]),
            getString(511) + var0,
            getString(512),
            getString(513) + script.specials[var3][8],
            getString(514) + script.specials[var3][9],
            getString(515) + script.specials[var3][10],
            getString(516),
            getString(513) + Math.abs(script.specials[var3][3]),
            getString(514) + Math.abs(script.specials[var3][4]),
            getString(515) + Math.abs(script.specials[var3][5]),
            getString(517),
            getString(513) + script.specials[var3][11],
            getString(514) + script.specials[var3][12],
            getString(515) + script.specials[var3][13],
            getString(518) + script.specials[var3][6] / 1000 + getString(521),
            getString(519) + var1,
            getString(520) + script.specials[var3][14]
         };
      }

      return helpSpells;
   }

   private static final String[][] getHelpArmor() {
      String var0 = null;
      String var1 = null;
      int var2 = 0;
      byte var3 = 0;
      boolean var4 = false;
      boolean var5 = false;
      if (helpArmor != null) {
         return helpArmor;
      }

      helpArmor = new String[script.armors.length - 1][];

      for (int var7 = 1; var7 < script.armors.length; var7++) {
         var2 = 7;
         if (script.armors[var7][3] == 0) {
            var0 = getString(28);
         } else if (script.armors[var7][3] == 1) {
            var0 = getString(29);
         } else if (script.armors[var7][3] == 2) {
            var0 = getString(30);
         } else if (script.armors[var7][3] == 7) {
            var0 = getString(35);
         } else if (script.armors[var7][3] == 3) {
            var0 = getString(31);
         } else if (script.armors[var7][3] == 4) {
            var0 = getString(32);
         } else if (script.armors[var7][3] == 6) {
            var0 = getString(34);
         } else if (script.armors[var7][3] == 5) {
            var0 = getString(33);
         }

         if (script.armors[var7][2] == 2) {
            var1 = getString(487);
         } else if (script.armors[var7][2] == 1) {
            var1 = getString(488);
         } else if (script.armors[var7][2] == 0) {
            var1 = getString(489);
         }

         helpArmor[var7 - 1] = new String[]{
            getString(481) + script.getItemName(script.armors[var7][1]),
            getString(482) + var1,
            getString(490) + var0,
            getString(483) + script.armors[var7][4],
            getString(484) + script.armors[var7][9],
            getString(485) + (script.armors[var7][9] >> 2),
            getString(486),
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null
         };

         for (int var8 = 0; var8 < script.classBase.length; var8++) {
            if (script.armors[var7][2] == 2) {
               var3 = 4;
            } else if (script.armors[var7][2] == 1) {
               var3 = 3;
            } else if (script.armors[var7][2] == 0) {
               var3 = 1;
            }

            if (script.classAllows(script.classBase[var8][0], var3)) {
               helpArmor[var7 - 1][var2++] = "   " + script.getItemName(script.classBase[var8][1]);
            }
         }
      }

      return helpArmor;
   }

   private static final String[][] getHelpWeapons() {
      String var0 = null;
      int var1 = 0;
      byte var2 = 0;
      boolean var3 = false;
      boolean var4 = false;
      if (helpWeapons != null) {
         return helpWeapons;
      }

      helpWeapons = new String[script.weapons.length - 1][];

      for (int var6 = 1; var6 < script.weapons.length; var6++) {
         var1 = 6;
         if (script.weapons[var6][2] == 0) {
            var0 = getString(476);
         } else if (script.weapons[var6][2] == 1) {
            var0 = getString(477);
         } else if (script.weapons[var6][2] == 4) {
            var0 = getString(478);
         } else if (script.weapons[var6][2] == 2) {
            var0 = getString(479);
         } else if (script.weapons[var6][2] == 3) {
            var0 = getString(480);
         }

         helpWeapons[var6 - 1] = new String[]{
            getString(481) + script.getItemName(script.weapons[var6][1]),
            getString(482) + var0,
            getString(483) + script.weapons[var6][3],
            getString(484) + script.weapons[var6][7],
            getString(485) + (script.weapons[var6][7] >> 2),
            getString(486),
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null
         };

         for (int var7 = 0; var7 < script.classBase.length; var7++) {
            if (script.weapons[var6][2] == 0) {
               var2 = 14;
            } else if (script.weapons[var6][2] == 1) {
               var2 = 5;
            } else if (script.weapons[var6][2] == 4) {
               var2 = 8;
            } else if (script.weapons[var6][2] == 2) {
               var2 = 6;
            } else if (script.weapons[var6][2] == 3) {
               var2 = 7;
            }

            if (script.classAllows(script.classBase[var7][0], var2)) {
               helpWeapons[var6 - 1][var1++] = "   " + script.getItemName(script.classBase[var7][1]);
            }
         }
      }

      return helpWeapons;
   }

   private final void updateViewport() {
      this.screenCorner0[0] = -this.cameraOffset[0];
      this.screenCorner0[1] = -this.cameraOffset[1];
      this.screenCorner1[0] = -this.cameraOffset[0] + screenWidth;
      this.screenCorner1[1] = -this.cameraOffset[1];
      this.screenCorner2[0] = -this.cameraOffset[0];
      this.screenCorner2[1] = -this.cameraOffset[1] + screenHeight;
      this.screenCorner3[0] = -this.cameraOffset[0] + screenWidth;
      this.screenCorner3[1] = -this.cameraOffset[1] + screenHeight;
      isoToCell(this.screenCorner0, this.viewCell0);
      isoToCell(this.screenCorner1, this.viewCell1);
      isoToCell(this.screenCorner2, this.viewCell2);
      isoToCell(this.screenCorner3, this.viewCell3);
      this.viewMin[0] = (byte)this.viewCell0[0];
      this.viewMin[1] = (byte)this.viewCell1[1];
      this.viewMax[0] = (byte)(this.viewCell3[0] + 3);
      this.viewMax[1] = (byte)(this.viewCell2[1] + 3);
   }

   public final void quit() {
      setState((byte)12);
      this.repaint();
      this.serviceRepaints();

      try {
         Thread.sleep(2000L);
      } catch (Exception var2) {
      }

      this.running = false;
      this.midlet.notifyDestroyed();
   }

   public final void start() {
      this.running = true;
      new Thread(this).start();
   }

   public final Actor spawnActor(String var1, int var2, int var3, int[] var4) {
      byte var5 = 0;
      var5 = (byte)(actors.length - 1);

      while (var5 >= 0 && actors[var5] != null) {
         var5--;
      }

      return this.spawnActorInSlot(null, var1, var5, var2, var3, var4);
   }

   public final Actor spawnActorInSlot(String var1, String var2, byte var3, int var4, int var5, int[] var6) {
      collectGarbage();
      if (var3 >= 0 && var3 < 25) {
         if (var3 == 0 && this.player != null) {
            actors[var3] = this.player;
            ActorSystem.revive(this.player);
         } else {
            actors[var3] = ActorSystem.createFromCml(var2, (byte)(var3 + 1));
            if (var3 == 0) {
               ActorSystem.setClass(actors[var3], (byte)(menuSelection[1] + 1), false);
            }

            ActorSystem.initFromTemplate(actors[var3], var6);
            actors[var3].name = var1;
         }

         ActorSystem.setPosition(actors[var3], var4, var5);
         if (var3 == 0) {
            this.player = actors[var3];
            this.player.collides = (byte)(playerCollides ? 1 : 0);
            this.player.dropsLoot = 0;
            this.player.deathScript = -1;
            this.redraw = true;
            this.cameraActor = var3;
            this.cameraOffset[0] = this.centerX - actors[this.cameraActor].screenPos[0];
            this.cameraOffset[1] = this.centerY - actors[this.cameraActor].screenPos[1];
         }

         this.maxActorSlot = Math.max(this.maxActorSlot, var3);
         this.lastTick = System.currentTimeMillis();
         return actors[var3];
      } else {
         return null;
      }
   }

   public static final void removeActor(int var0) {
      if (actors[var0] != null) {
         if (var0 == 0) {
            ProjectileManager.clearAll();
            setState((byte)11);
            ActorSystem.revive(instance.player);
            ActorSystem.setPosition(instance.player, respawnX, respawnY);
            showMessage(null, 0, 0, 0);
         } else {
            if (var0 == instance.cameraActor) {
               setSpeakerName(null);
            }

            actors[var0] = null;
            if (var0 == instance.maxActorSlot) {
               while (var0 > 0 && actors[var0] == null) {
                  var0--;
               }

               instance.maxActorSlot = var0;
            }

            collectGarbage();
         }
      }
   }

   public final void setInputEnabled(boolean var1) {
      this.inputEnabled = var1;
      if (var1) {
         keyState = -286331154;
      }
   }

   public final void loadTileSprites(String var1) {
      this.tileSprites = SpriteRenderer.load(var1);
   }

   public final void endLevel(int var1, int var2) {
      this.cutsceneSprite = var1;
      this.cutsceneColor = var2;
      if (var1 == 4) {
         setState((byte)21);
      } else {
         setState((byte)8);
      }
   }

   public final void setCollision(int var1, int var2, boolean var3) {
      collision[var1 * gridHeight + var2] = (byte)(var3 ? 1 : 0);
   }

   public final void setTile(int var1, int var2, int var3, int var4) {
      ((byte[])layers.elementAt(var3))[var1 * gridHeight + var2] = (byte)var4;
      this.redraw = true;
   }

   public final void cameraTo(int var1, int var2) {
      int[] var3 = new int[]{0, 0};
      worldToIso(new int[]{var1, var2}, var3);
      this.cameraOffset[0] = this.centerX - var3[0];
      this.cameraOffset[1] = this.centerY - var3[1];
      this.redraw = true;
      this.cameraActor = -1;
      setSpeakerName(null);
   }

   public final void cameraFollow(int var1) {
      if (actors[var1] != null) {
         this.cameraActor = (byte)var1;
         this.redraw = true;
         this.cameraOffset[0] = this.centerX - actors[var1].screenPos[0];
         this.cameraOffset[1] = this.centerY - actors[var1].screenPos[1];
         setSpeakerName(actors[var1].name);
      }
   }

   private final void updateCamera() {
      if (this.cameraActor >= 0 && actors[this.cameraActor] != null && actors[this.cameraActor].dead == 0) {
         if (actors[this.cameraActor].screenPos[1] - ActorSystem.spriteHeight(actors[this.cameraActor]) + this.cameraOffset[1] < 0) {
            this.redraw = true;
         } else if (actors[this.cameraActor].screenPos[1] + this.cameraOffset[1] > screenHeight) {
            this.redraw = true;
         } else if (actors[this.cameraActor].screenPos[0] + this.cameraOffset[0] < 0) {
            this.redraw = true;
         } else if (actors[this.cameraActor].screenPos[0] + ActorSystem.spriteWidth(actors[this.cameraActor]) + this.cameraOffset[0] > screenWidth) {
            this.redraw = true;
         }

         if (this.redraw) {
            this.cameraOffset[0] = this.centerX - actors[this.cameraActor].screenPos[0];
            this.cameraOffset[1] = this.centerY - actors[this.cameraActor].screenPos[1] + ActorSystem.spriteHeight(actors[this.cameraActor]);
         }
      }
   }

   public final void setTrigger(int var1, int var2, int var3, int var4, int var5) {
      if (this.enterLayer != null && this.leaveLayer != null && zoneLayer != null) {
         this.enterLayer[var1 * gridHeight + var2] = (byte)var3;
         this.leaveLayer[var1 * gridHeight + var2] = (byte)var4;
         zoneLayer[var1 * gridHeight + var2] = (byte)var5;
      }
   }

   public static final int loadResource(String var0) {
      InputStream var1 = var0.getClass().getResourceAsStream(var0);
      int var2 = 0;
      int var3 = 0;
      if (resourceBuffer == null) {
         resourceBuffer = new byte[6144];
      }

      try {
         while ((var2 = var1.read(resourceBuffer, var3, resourceBuffer.length - var3)) > 0) {
            var3 += var2;
         }

         var1.close();
      } catch (Exception var5) {
         System.err.println(var0);
         var5.printStackTrace();
      }

      collectGarbage();
      return var3;
   }

   public static final void setLoadingProgress(int var0) {
      if (state != 0) {
         instance.loadProgress = (byte)var0;
         if (instance.stateBeforeLoading == -1) {
            instance.stateBeforeLoading = state;
         }

         setState((byte)7);
         instance.repaint();
         instance.serviceRepaints();
         if (var0 == 100) {
            setState(instance.stateBeforeLoading);
            instance.loadProgress = -1;
            instance.stateBeforeLoading = -1;
         }
      }
   }

   public final void openMenu() {
      this.buildMenus();
      setState((byte)3);
      menuId = 0;
   }

   public static final void showMessage(String var0, int var1, int var2, int var3) {
      message = var0;
      messageDuration = var1 * 1000;
      messageStyle = (byte)var3;
      messageX = -1;
      messageY = -1;
      messageElapsed = 0;
      messageColor = 0;
      messageBlank = false;
      messageBlinkTimer = 0;
      messageScrollTimer = 0;
      if (var2 == 0) {
         messageColor = 0;
      }

      if (var2 == 3) {
         messageColor = 255;
      }

      if (var2 == 5) {
         messageColor = 65280;
      }

      if (var2 == 2) {
         messageColor = 16711680;
      }

      if (var2 == 1) {
         messageColor = 16777215;
      }

      if (var2 == 4) {
         messageColor = 16776960;
      }
   }

   public final void loadHudSprites(String var1) {
      this.hudSprites = SpriteRenderer.load(var1);
   }

   public final void openShopMenu() {
      menuId = 4;
      setState((byte)3);
   }

   public final boolean isShopState() {
      return state == 1;
   }

   public final void menuSelected(DialogueNode var1) {
      String var2 = var1.text;
      if (var1.parent.text.equals(getString(36))) {
         int var10 = var2.indexOf(58) + 2;
         int var11 = var2.indexOf(32, var10);
         int var12 = 0;

         try {
            var12 = Integer.parseInt(var2.substring(var10, var11));
         } catch (NumberFormatException var8) {
            System.out.println("IsoMap::menuSelected() buy");
            System.out.println("string= " + var2.substring(var10, var11));
            System.exit(1);
         }

         if (gold >= var12) {
            gold -= var12;
            int var13 = script.itemCategory(var2.substring(0, var10 - 3));
            int[] var14;
            if ((var14 = script.findByName(var2.substring(0, var10 - 3))) != null && this.player != null) {
               ActorSystem.addItem(this.player, var13, var14);
               this.openShop();
            }
         }

         var1.marked = false;
         this.dialogue.caption = getString(38) + " : " + gold;
      } else if (var1.parent.text.equals(getString(37))) {
         int var3 = var2.indexOf(58) + 2;
         int var4 = var2.indexOf(32, var3);
         int var5 = 0;

         try {
            var5 = Integer.parseInt(var2.substring(var3, var4));
         } catch (NumberFormatException var9) {
            System.out.println("IsoMap::menuSelected() //sell");
            System.out.println("string= " + var2.substring(var3, var4));
            System.exit(1);
         }

         if (this.player != null) {
            int var6 = script.itemCategory(var2.substring(0, var3 - 3));
            int[] var7;
            if ((var7 = script.findByName(var2.substring(0, var3 - 3))) != null) {
               ActorSystem.removeItem(this.player, var6, var7);
            }

            if (var1.parent.children.indexOf(var1) == var1.parent.children.size() - 1) {
               this.dialogue.handleKey('\u0003');
            }

            var1.parent.children.removeElement(var1);
         }

         gold += var5;
         var1.marked = false;
         this.dialogue.caption = getString(38) + " : " + gold;
      } else if (var1.parent.text.equals(getString(26))) {
         var1.marked = false;
      } else if (var1.parent.text.equals(getString(25))) {
         if (ActorSystem.equipFromString(this.player, var2)) {
            if (equippedWeaponNode != null) {
               equippedWeaponNode.marked = true;
            }

            equippedSpellNode = var1;
         } else {
            if (equippedSpellNode != null) {
               equippedSpellNode.marked = true;
            }

            equippedWeaponNode = var1;
         }
      } else if (var1.parent.text.equals(getString(27))) {
         ActorSystem.useConsumable(this.player, script.findByName(var2));
      } else {
         ActorSystem.equipArmor(this.player, script.findByName(var2));
      }
   }

   public final void saveGame() {
      boolean var1 = false;

      try {
         ByteArrayOutputStream var2 = new ByteArrayOutputStream();
         RecordStore var3 = RecordStore.openRecordStore("ESO", true);

         for (int var5 = 0; var5 < keyBindings.length; var5++) {
            var2.write(keyBindings[var5]);
         }

         var2.write(this.soundEnabled ? 1 : 0);
         if (this.player == null) {
            var2.write(0);
         } else {
            var2.write(1);
            var2.write(currentLevel.length());
            var2.write(currentLevel.getBytes());
            ActorSystem.serialize(this.player, var2);
         }

         if (var3.getNextRecordID() == 1) {
            var3.addRecord(var2.toByteArray(), 0, var2.size());
         } else {
            var3.setRecord(1, var2.toByteArray(), 0, var2.size());
         }

         var3.closeRecordStore();
      } catch (Exception var4) {
         var4.printStackTrace();
      }
   }

   public final boolean hasSavedGame() {
      boolean var1 = false;

      try {
         RecordStore var2;
         if ((var2 = RecordStore.openRecordStore("ESO", false)) != null) {
            byte[] var3 = var2.getRecord(1);
            int var4 = 0;
            var4 = 0 + keyBindings.length;
            var4++;
            var1 = var3[var4] == 1;
         }

         var2.closeRecordStore();
      } catch (RecordStoreNotFoundException var5) {
      } catch (Exception var6) {
         var6.printStackTrace();
      }

      return var1;
   }

   public final void loadSavedGame() {
      this.loadGame(true);
   }

   public final void loadGame(boolean var1) {
      boolean var2 = false;

      try {
         RecordStore var3;
         if ((var3 = RecordStore.openRecordStore("ESO", false)) != null) {
            byte[] var4 = var3.getRecord(1);
            int var5 = 0;

            for (int var10 = 0; var10 < keyBindings.length; var10++) {
               keyBindings[var10] = var4[var5++];
            }

            this.soundEnabled = var4[var5++] == 1;
            this.loadProgress = 0;
            setState((byte)6);
            this.repaint();
            this.serviceRepaints();
            if (var4[var5++] == 1) {
               byte var6 = var4[var5++];
               String var7 = new String(var4, var5, var6);
               var5 += var6;
               if (var1) {
                  this.loadLevel(var7);
               } else {
                  currentLevel = var7;
               }

               this.player = actors[0] = ActorSystem.fromRecord(var4, var5);
            }

            var3.closeRecordStore();
         }
      } catch (RecordStoreNotFoundException var8) {
      } catch (Exception var9) {
         var9.printStackTrace();
      }
   }

   public final void spawnItem(int var1, boolean var2, int var3, int var4) {
      int var5 = var3 * gridHeight + var4;
      if (pickupCount < pickups.length - 4 && var5 < this.cellCount && var5 >= 0) {
         if (var2) {
            ((byte[])layers.elementAt(layers.size() - 1))[var5] = -45;
         } else {
            ((byte[])layers.elementAt(layers.size() - 1))[var5] = 22;
         }

         pickups[pickupCount++] = (byte)var3;
         pickups[pickupCount++] = (byte)var4;
         pickups[pickupCount++] = (byte)var1;
      }
   }

   public static final void showTextScreen(String var0) {
      setTextScreen(var0);
      setState((byte)10);
   }

   public static final void setRespawnPoint(int var0, int var1) {
      respawnX = (short)var0;
      respawnY = (short)var1;
   }

   public static final void loadLangPack(String var0, int var1) {
      if (var1 != 65535 && var1 != 0 || !Strings.firstLangLoaded) {
         short var2 = 0;
         boolean var3 = false;
         boolean var4 = false;
         boolean var5 = false;
         boolean var6 = false;
         boolean var7 = false;
         char[] var8 = null;
         short[] var9 = null;
         if (var1 == 65535) {
            var1 = 0;
         }

         langChars1 = null;
         langOffsets1 = null;
         collectGarbage();
         var8 = new Strings().load(var1);
         if (var1 == 0) {
            langChars0 = var8;
            var9 = langOffsets0 = new short[Strings.unconfirmedTableA(var1) + 1];
         } else {
            langChars1 = var8;
            var9 = langOffsets1 = new short[Strings.unconfirmedTableA(var1) + 1];
         }

         for (int var16 = 1; var16 < var9.length; var16++) {
            var9[var16] = -1;
         }

         int var11 = 0;
         int var12 = 0;
         String var13 = null;

         for (int var17 = 1; var17 <= Strings.unconfirmedTableB(var1); var17++) {
            while (var8[var12] != '|') {
               var12++;
            }

            if (var11 != 0) {
               var11 += 2;
            }

            int var14 = (var13 = new String(var8, var11, var12 - var11)).indexOf(32);
            var2 = Integer.valueOf(var13.substring(0, var14)).shortValue();
            var9[var2] = (short)(var11 + var14 + 1);
            var11 = var12++ + 1;
         }

         if (var1 == 0) {
            langOffsets0 = (short[])var9;
         } else {
            langOffsets1 = (short[])var9;
         }

         collectGarbage();
      }
   }

   public static final String getString(int var0) {
      int var1 = 0;
      if (langOffsets0 != null && var0 >= 0 && var0 < langOffsets0.length && langOffsets0[var0] != -1) {
         for (int var3 = langOffsets0[var0]; var3 < langChars0.length && langChars0[var3] != '|'; var3++) {
            var1++;
         }

         return new String(langChars0, langOffsets0[var0], var1);
      } else if (langOffsets1 != null && var0 >= 0 && var0 < langOffsets1.length && langOffsets1[var0] != -1) {
         for (int var2 = langOffsets1[var0]; var2 < langChars1.length && langChars1[var2] != '|'; var2++) {
            var1++;
         }

         return new String(langChars1, langOffsets1[var0], var1);
      } else {
         return "";
      }
   }

   public static final int stringToId(String var0) {
      char[] var1 = var0.toCharArray();
      int var2 = 0;
      boolean var3 = false;
      int var4 = 0;
      int var5 = 0;

      for (int var8 = 1; var8 < langOffsets0.length; var8++) {
         if (langOffsets0[var8] != -1) {
            var2 = 0;

            for (int var6 = langOffsets0[var8]; var6 < langChars0.length && langChars0[var6] != '|'; var6++) {
               var2++;
            }

            if (var1.length == var2) {
               var4 = langOffsets0[var8];

               for (var5 = 0; var4 < langOffsets0[var8] + var2 && var1[var5] == langChars0[var4]; var5++) {
                  var4++;
               }

               if (var5 == var2) {
                  return var8;
               }
            }
         }
      }

      return -1;
   }

   public final void showDialogue(String var1) {
      this.dialogueRight = screenWidth - 10;
      this.dialogueTextWidth = this.dialogueRight - SpriteRenderer.getWidth(this.hudSprites, 54) - 13;
      this.dialogueHeight = Math.min(screenHeight >> 1, SpriteRenderer.getHeight(this.hudSprites, 51)) - 4;
      wrapText(var1, this.dialogueLines, this.dialogueTextWidth);
   }

   public static final void wrapText(String var0, Vector var1, int var2) {
      int var3 = 0;
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      int var7 = 0;
      char var8 = '\u0000';
      if ((var7 = var0.indexOf("ACTION_KEY")) != -1) {
         var0 = var0.substring(0, var7) + keyLabels[8] + var0.substring(var7 + "ACTION_KEY".length());
      }

      if ((var7 = var0.indexOf("TOGGLE_WEAPON_KEY")) != -1) {
         var0 = var0.substring(0, var7) + keyLabels[keyBindings[2]] + var0.substring(var7 + "TOGGLE_WEAPON_KEY".length());
      }

      if ((var7 = var0.indexOf("QUICK_HEALTH_KEY")) != -1) {
         var0 = var0.substring(0, var7) + keyLabels[keyBindings[0]] + var0.substring(var7 + "QUICK_HEALTH_KEY".length());
      }

      if ((var7 = var0.indexOf("QUICK_MAGIKA_KEY")) != -1) {
         var0 = var0.substring(0, var7) + keyLabels[keyBindings[1]] + var0.substring(var7 + "QUICK_MAGIKA_KEY".length());
      }

      if (speakerName != null) {
         var0 = speakerName + ": " + var0;
      } else if (var0.indexOf(":") != -1) {
         speakerName = var0.substring(0, var0.indexOf(":"));
      }

      var1.removeAllElements();

      for (var3 = 0; var3 < var0.length() - 1; var3++) {
         var8 = var0.charAt(var3);
         var4 = fontSmallBold.substringWidth(var0, var6, var3 - var6 + 1);
         if (var8 == ' ') {
            var5 = var3;
         }

         if (var4 >= var2 && var5 > 0) {
            var1.addElement(var0.substring(var6, var5));
            var6 = var3 = var5 + 1;
            var5 = 0;
         }
      }

      if (var3 > var6) {
         var1.addElement(var0.substring(var6, var3 + 1));
      }
   }

   public final void beginDialogue() {
      this.dialogueScroll = -1;
      this.dialogueAtEnd = true;
      this.dialogueOpen = true;
      this.dialogueOpenedAt = System.currentTimeMillis();
   }

   public final void drawDialogue(Graphics var1) {
      boolean var2 = false;
      boolean var3 = true;
      int var4 = this.dialogueLeft;
      int var5 = this.dialogueTop;
      int var6 = 0;
      int var7 = 0;
      SpriteRenderer.draw(var1, this.hudSprites, 51, 5, 5);

      for (var6 = 0;
         var6
            <= (this.dialogueRight - 5 - SpriteRenderer.getWidth(this.hudSprites, 51) - SpriteRenderer.getWidth(this.hudSprites, 50))
               / SpriteRenderer.getWidth(this.hudSprites, 52);
         var6++
      ) {
         SpriteRenderer.draw(
            var1, this.hudSprites, 52, 5 + SpriteRenderer.getWidth(this.hudSprites, 51) + var6 * SpriteRenderer.getWidth(this.hudSprites, 52), 5
         );
      }

      SpriteRenderer.draw(var1, this.hudSprites, 50, 5 + SpriteRenderer.getWidth(this.hudSprites, 51) + var6 * SpriteRenderer.getWidth(this.hudSprites, 52), 5);
      var7 = var6;
      var1.setColor(16777215);
      var1.setFont(fontSmallBold);

      for (var6 = 0; var6 < this.dialogueLines.size() && var3; var6++) {
         String var8 = (String)this.dialogueLines.elementAt(var6);
         if (var5 - this.dialogueScroll >= this.dialogueTop && var5 - this.dialogueScroll <= this.dialogueTop + this.dialogueHeight - fontSmallBold.getHeight()
            )
          {
            var2 |= var6 == 0;
            if (speakerName != null && var8.startsWith(speakerName) && var6 == 0) {
               var8 = var8.substring(speakerName.length() + 2);
               var1.setColor(6684672);
               var1.drawString(speakerName + ": ", var4, var5 - this.dialogueScroll, 0);
               var1.setColor(16777215);
               var1.drawString(var8, var4 + fontSmallBold.stringWidth(speakerName + ": "), var5 - this.dialogueScroll, 0);
            } else {
               var1.drawString(var8, var4, var5 - this.dialogueScroll, 0);
            }
         }

         var3 = (var5 += fontSmallBold.getHeight() + 1) - this.dialogueScroll < this.dialogueTop + this.dialogueHeight - fontSmallBold.getHeight() - 1;
      }

      this.dialogueAtEnd = var6 == this.dialogueLines.size() && var3;
      if (!var2) {
         SpriteRenderer.draw(
            var1,
            this.hudSprites,
            54,
            5
               + SpriteRenderer.getWidth(this.hudSprites, 51)
               + var7 * SpriteRenderer.getWidth(this.hudSprites, 52)
               - SpriteRenderer.getWidth(this.hudSprites, 54)
               + 3,
            8
         );
      }

      if (!this.dialogueAtEnd) {
         SpriteRenderer.draw(
            var1,
            this.hudSprites,
            53,
            5
               + SpriteRenderer.getWidth(this.hudSprites, 51)
               + var7 * SpriteRenderer.getWidth(this.hudSprites, 52)
               - SpriteRenderer.getWidth(this.hudSprites, 53)
               + 3,
            5 + SpriteRenderer.getHeight(this.hudSprites, 51) - SpriteRenderer.getHeight(this.hudSprites, 53) - 3
         );
      }
   }

   public final void handleDialogueKey(int var1) {
      if (this.dialogueOpen) {
         if (var1 == 3 && this.dialogueScroll > -1) {
            this.dialogueScroll -= 4;
         } else if (var1 == 4 && !this.dialogueAtEnd) {
            this.dialogueScroll += 4;
         } else {
            if (var1 == 7 && System.currentTimeMillis() - this.dialogueOpenedAt >= 1000L) {
               this.dialogueOpen = false;
               this.player.zoneId = 0;
               this.keyHandled = 1;
            }
         }
      }
   }

   public static final void setSpeakerName(String var0) {
      speakerName = var0;
   }

   public final void hideNotify() {
      if (!this.isLoadingState()) {
         super.hideNotify();
         this.suspended = true;
         if (state != 8 && state != 21 && state != 15 && state != 10) {
            if (state != 22) {
               pausedState = state;
            }

            state = 22;
            DialogueScreen.showPauseOverlay = true;
         }
      }
   }

   public final void showNotify() {
      if (!this.isLoadingState()) {
         super.showNotify();
         this.suspended = false;
      }
   }

   public final boolean isLoadingState() {
      return state == 6 || state == 7 || state == 15;
   }
}
