// Renamed from decompiled/ESGame.java. See ../docs/CLASS_MAP.md and
// ../src/README.md.
//
// ESGame's own member names were already readable in the decompiled
// output -- no mechanical-rename blocker like e/j had -- so this pass
// only retypes its fields/locals from the old single-letter classes
// (Screen, LoadingScreen, GameCanvas, Player, Dungeon, Item, Spell,
// DungeonGenerator, Monster, Shop, Util) to their real renamed names,
// and updates every call site to match. This is the last of the 13
// decompiled classes to get a pass -- with it done, src/ is a single
// coherent tree and decompiled/*.java is no longer needed to compile
// against.
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Random;
import java.util.Vector;
import javax.microedition.lcdui.Alert;
import javax.microedition.lcdui.AlertType;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.StringItem;
import javax.microedition.lcdui.TextField;
import javax.microedition.rms.RecordStore;
import ngame.midlet.RegisteredMIDlet;

public class ESGame extends RegisteredMIDlet implements Runnable, CommandListener {
   private static int theMIDletState;
   private static final int MIDLET_STATE_UNKNOWN = 0;
   private static final int MIDLET_STATE_CREATED = 1;
   private static final int MIDLET_STATE_ACTIVE = 2;
   private static final int MIDLET_STATE_PAUSED = 3;
   private static final int MIDLET_STATE_DESTROYED = 4;
   static String SERVER_DATAFILE_BASE_URL = null;
   Form errorForm;
   StringItem errItem;
   static final Command okCommand = new Command("Ok", 3, 0);
   static final Command selectCommand = new Command("Select", 3, 0);
   static final Command cancelCommand = new Command("Cancel", 4, 0);
   static final Command backCommand = new Command("Back", 4, 0);
   static final Command exitCommand = new Command("Exit", 7, 0);
   static int helperThreadState = 1;
   static String downloadStatus = "";
   private static Thread downloadImagesThread = null;
   private static int debugExceptionIndex = 0;
   private static String[][] monster_filenames;
   private static final int[][] monster_image_index_info = new int[][]{{0, 7}, {7, 7}, {14, 6}, {20, 3}, {23, 3}};
   static final String[] game_datafile_names = new String[]{"/datfiles.lmp", "/imgfiles.lmp", "/npcstrings.dat"};
   private static int[] attribIncr = new int[3];
   static final String[] copyString = new String[]{
      "(c) 2003 Vir2L Studios, ",
      "a ZeniMax Media company. ",
      "The Elder Scrolls and Vir2L ",
      "are registered trademarks ",
      "of ZeniMax Media Inc. ",
      "All rights reserved."
   };
   Display display;
   static Image splashImageTop = null;
   static Image splashImageBot = null;
   private Screen introUI;
   private Screen restartUI;
   private Screen mainMenuUI;
   private Screen newGameUI;
   private Screen characterMainUI;
   private Screen noSavedGameUI;
   private static LoadingScreen saveGameUI;
   private static LoadingScreen loadGameUI;
   private LoadingScreen splashUI;
   LoadingScreen loadDungeonUI;
   private LoadingScreen createGameUI;
   Screen[] NPCChoicesUI;
   Screen NPCGiveWhatUI;
   Screen WarpWhereUI;
   Screen GenericInfoUI;
   Screen NPCTrainWhatUI;
   Screen NPCTakeWhatUI;
   Screen NPCQuestionWhatUI;
   Screen NPCQuestionWhomUI;
   Screen NPCSellWhatUI;
   Screen NPCSellSureUI;
   Screen NPCBuyWhatUI;
   Screen NPCWarpUI;
   Screen OptionsUI;
   Screen InventoryUI;
   Screen InventoryItemUI;
   Screen SkillsListUI;
   Screen SpellsListUI;
   Screen SpellInfoUI;
   Screen ClueUI;
   Screen LevelUpUI;
   Screen endOfGameUI;
   Screen gameOverUI;
   Screen confirmQuitUI;
   Screen helpUI;
   Screen RevealUI;
   GameCanvas gameCanvas;
   private Form charNameTextForm;
   private static String[] helpStrings = new String[12];
   private static String[] helpTitles = new String[12];
   private static String creditsString = null;
   public Player character;
   static Dungeon[] dungeons;
   Thread imgloadThread;
   boolean imgloadRunning;
   boolean killThread;
   byte loadingDungeonID;
   boolean imgsLoaded;
   static Hashtable[] monsters;
   static Hashtable[] chests;
   static Vector[] droppedItems;
   static Screen uic = null;
   static int debugCode = 0;
   int currentItemIndex;
   int currentSpellIndex;
   static boolean showCarrierLogo;
   static Image carrierLogoImage;
   static Image vir2lLogoImage;
   boolean showSplash;
   Hashtable imageCreater;
   int currentQWhat;
   int currentQWhom;
   static Random r;
   private static boolean reloadGame = false;

   public ESGame() {
      super.appName = "The Elder Scrolls";
      this.display = null;
      SERVER_DATAFILE_BASE_URL = this.getAppProperty("Pluto-Server-URL");
      if (SERVER_DATAFILE_BASE_URL != null) {
         System.out.println("FOUND Pluto-Server-URL in JAR! Adding prefix gives: " + SERVER_DATAFILE_BASE_URL);
      } else {
         SERVER_DATAFILE_BASE_URL = "http://localhost/essm";
         System.out.println("Did not find Pluto-Server-URL in JAR! Using default of http://localhost/essm");
      }

      String var1 = this.getAppProperty("Mserver-User-Id");
      if (var1 != null) {
         Player.serverUserId = var1;
         System.out.println("User ID is " + Player.serverUserId);
      } else {
         System.out.println("User ID is NULL!");
      }

      theMIDletState = 1;
   }

   public void startRegisteredApp() {
      if (this.display == null) {
         printMemory("Very start of startapp");
         this.display = Display.getDisplay(this);
         this.gameCanvas = new GameCanvas(this);
         this.GenericInfoUI = new Screen(this, 4, 410);
         if (checkFirstTime()) {
            this.GenericInfoUI.setSecondaryParam(410);
            this.GenericInfoUI
               .setupMessage(
                  "Welcome",
                  "You must recycle the power on your phone before playing The Elder Scrolls Travels for the first time. Please turn the power off, then on, then restart the game."
               );
            this.setCurrentDisplay(this.GenericInfoUI);
         } else {
            this.currentItemIndex = -1;
            this.currentSpellIndex = -1;
            this.imgloadThread = new Thread(this);
            this.imgloadRunning = false;
            this.killThread = false;
            this.loadingDungeonID = 0;
            this.imgsLoaded = false;
            this.createErrorForm();
            this.initSplash();
            theMIDletState = 2;
         }
      } else {
         this.gameCanvas.repaint();
         this.display.setCurrent(this.gameCanvas);
      }
   }

   private void initSplash() {
      try {
         splashImageTop = Image.createImage("/splashtop.png");
         splashImageBot = Image.createImage("/splashbot.png");
         System.gc();
         Thread var1 = new Thread(this);
         this.splashUI = new LoadingScreen(this, 2, 1);
         this.splashUI.unusedHook1();
         this.splashUI.returnDisplay = this.errorForm;
         helperThreadState = 2;
         this.showSplash = true;
         showCarrierLogo = false;
         this.setCurrentDisplay(this.splashUI);
         var1.start();
      } catch (Exception var2) {
         System.out.println("Barfed in initSplash");
         this.display.setCurrent(this.errorForm);
      }
   }

   public void run() {
      if (helperThreadState == 1) {
         System.out.println("run() Initial download, no longer implemented");
      } else if (helperThreadState == 2) {
         this.runAppload();
      } else if (helperThreadState == 4) {
         this.createNewGame();
      } else if (helperThreadState == 5) {
         if (this.saveGameState()) {
            this.setCurrentDisplay(this.gameCanvas);
         } else {
            this.GenericInfoUI.setSecondaryParam(499);
            this.GenericInfoUI
               .setupMessage(
                  "Save Error",
                  "There was an error in saving your character record. Your previous character record is still saved. Try turning your phone off then on again to clear the memory."
               );
            this.setCurrentDisplay(this.GenericInfoUI);
         }
      } else if (helperThreadState == 6) {
         if (this.loadGameState()) {
            this.resumeGame();
            this.loadingDungeonID = this.character.currentLevel;
            this.imgloadRunning = true;
            reloadGame = true;
            reloadGame = false;
            this.imgloadRunning = false;
            loadGameUI.percent = 100;
            this.gameCanvas.repaint();
            this.gameCanvas.serviceRepaints();
            this.gameCanvas.player = this.character;
            // Original set GameCanvas's `r` field true here. Confirmed
            // dead: GameCanvas's run() loop now models that as a
            // `runTick` local reset fresh every tick before it is ever
            // read, so presetting it externally has no observable
            // effect -- dropped rather than reintroducing a field
            // whose only writer was this line.
            this.gameCanvas.startGameThread();
            this.setCurrentDisplay(this.gameCanvas);
         } else {
            this.setCurrentDisplay(this.noSavedGameUI);
         }
      } else {
         this.runImageLoader();
         this.setCurrentDisplay(this.gameCanvas);
      }
   }

   private void runAppload() {
      try {
         System.gc();
         this.showSplash = true;
         showCarrierLogo = false;

         try {
            Thread.sleep(1000L);
         } catch (Exception var4) {
         }

         this.showSplash = false;
         this.splashUI.percent = 0;
         this.createImageFromFile();
         this.splashUI.percent = 5;
         carrierLogoImage = this.createImage("mformaLogo.png");
         vir2lLogoImage = this.createImage("vir2lLogo.png");
         this.allocateESGame();
         this.allocateAllUIs();
         if (this.imageCreater != null) {
            this.imageCreater.clear();
            this.imageCreater = null;
            System.gc();
         }

         this.allocAllDungeons();
         this.splashUI.percent = 100;
         System.gc();
         printMemory("end of startapp");
      } catch (Throwable var5) {
         System.out.println("ERROR: CANNOT LOAD APP!!");
         System.out.println(var5);

         try {
            Thread.sleep(15000L);
         } catch (Exception var3) {
         }
      }
   }

   private void allocateESGame() throws Exception {
      System.gc();
      System.gc();
      Player.ensureCharDataLoaded();
      loadHelpStrings();
      this.splashUI.percent = 10;
      Item.load();
      Spell.load();
      Monster.load();
      System.gc();
      this.splashUI.percent = 15;
   }

   public static final DataInputStream getResource(String var0) {
      int var1 = -1;
      int var2 = -1;
      int var3 = 0;
      DataInputStream var8 = null;

      try {
         var8 = Util.openResource("/datfiles.lmp");

         while (var1 == -1) {
            String var9 = "";
            int var4 = var8.read();
            var3++;

            while (var4 != 45) {
               var9 = var9 + (char)var4;
               var4 = var8.read();
               var3++;
            }

            if (var9.compareTo(var0) == 0) {
               var2 = ((var8.read() & 0xFF) << 24) + ((var8.read() & 0xFF) << 16) + ((var8.read() & 0xFF) << 8) + (var8.read() & 0xFF);
               var1 = ((var8.read() & 0xFF) << 8) + (var8.read() & 0xFF) + 1;
               var3 += 6;
            }
         }

         int var12 = (int)var8.skip(var2 - var3);
         if (var12 != var2 - var3) {
            System.out.println("skip failed " + var12 + " not " + (var2 - var3));
         }

         var3 += var12;
      } catch (Exception var10) {
      }

      System.gc();
      return var8;
   }

   static void loadHelpStrings() {
      String[] var0 = new String[35];

      try {
         DataInputStream var1 = getResource("helptext.dat");
         int var2 = var1.readInt();
         if (var2 != 35) {
            System.out.println("Unexpected number of help messages " + var2);
         }

         var0 = new String[var2];

         for (int var3 = 0; var3 < var2; var3++) {
            var0[var3] = var1.readUTF();
         }

         var1.close();
      } catch (Exception var4) {
         System.out.println("ERROR loading help text!");
      }

      helpTitles[0] = var0[0];
      helpTitles[1] = var0[2];
      helpTitles[2] = var0[5];
      helpTitles[3] = var0[7];
      helpTitles[4] = var0[13];
      helpTitles[5] = var0[15];
      helpTitles[6] = var0[18];
      helpTitles[7] = var0[23];
      helpTitles[8] = var0[25];
      helpTitles[9] = var0[28];
      helpTitles[10] = var0[31];
      helpTitles[11] = var0[33];
      StringBuffer var6 = new StringBuffer(1500);
      var6.append(var0[1]);
      helpStrings[0] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[3]);
      var6.append(var0[4]);
      helpStrings[1] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[6]);
      helpStrings[2] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[8]);
      var6.append(var0[9]);
      var6.append(var0[10]);
      var6.append(var0[11]);
      var6.append(var0[12]);
      helpStrings[3] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[14]);
      helpStrings[4] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[16]);
      var6.append(var0[17]);
      helpStrings[5] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[19]);
      var6.append(var0[20]);
      var6.append(var0[21]);
      var6.append(var0[22]);
      helpStrings[6] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[24]);
      helpStrings[7] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[26]);
      var6.append(var0[27]);
      helpStrings[8] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[29]);
      var6.append(var0[30]);
      helpStrings[9] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[32]);
      helpStrings[10] = var6.toString();
      var6.delete(0, 1500);
      var6.append(var0[34]);
      helpStrings[11] = var6.toString();
      var6.delete(0, 1500);
      var0 = null;
      System.gc();
   }

   private void allocAllDungeons() {
      dungeons = null;
      System.gc();
      dungeons = new Dungeon[37];
      this.splashUI.percent = 60;
      new DungeonGenerator(dungeons, this.splashUI);
      Object var1 = null;
      System.gc();
   }

   private void createNewGame() {
      this.createGameUI.percent = 5;
      System.gc();
      this.mainMenuUI = null;
      this.newGameUI = null;
      this.characterMainUI = null;
      this.charNameTextForm = null;
      this.splashUI = null;
      this.createGameUI.percent = 10;
      Shop.loadDialogue();
      System.gc();
      this.createGameUI.percent = 100;
      this.setCurrentDisplay(this.GenericInfoUI);
   }

   private void resumeGame() {
      int var1 = getGameAdvancementLevel(this.character.giftPointsFound);
      openAndRepopulateDungeons(var1);
   }

   private void allocateAllUIs() throws Exception {
      System.gc();
      this.splashUI.percent = 20;
      this.checkDestroyed();
      GameCanvas.floorTexture = this.createImage("floor3.png");
      GameCanvas.floorIceTexture = this.createImage("floorIce.png");
      GameCanvas.wallTexture = this.createImage("wallsr.png");
      GameCanvas.wallIceTexture = this.createImage("wallsi.png");
      GameCanvas.gateTexture = this.createImage("gate.png");
      GameCanvas.objectSprites = new Image[26];

      for (int var1 = 0; var1 < 26; var1++) {
         GameCanvas.objectSprites[var1] = null;
      }

      this.loadingDungeonID = 1;
      this.imgloadRunning = true;
      this.loadDungeonUI = new LoadingScreen(this, 11, 304);
      this.loadDungeonUI.unusedHook2();
      this.loadCampMonsters();
      this.checkDestroyed();
      GameCanvas.itemBagSprites = new Image[3];
      GameCanvas.itemBagSprites[0] = this.createImage("baglarge.png");
      GameCanvas.itemBagSprites[1] = this.createImage("bagmid.png");
      GameCanvas.itemBagSprites[2] = this.createImage("bagsmall.png");
      System.gc();
      this.checkDestroyed();
      GameCanvas.chestSprites = new Image[3];
      GameCanvas.chestSprites[0] = this.createImage("chestnearclosed.png");
      GameCanvas.chestSprites[1] = this.createImage("chestmidclosed.png");
      GameCanvas.chestSprites[2] = this.createImage("chestfarclosed.png");
      this.checkDestroyed();
      GameCanvas.iconsSprite = this.createImage("icons.png");
      GameCanvas.panelImage = this.createImage("panel.png");
      System.gc();
      this.checkDestroyed();
      creditsString = this.getCreditsString();
      this.mainMenuUI = new Screen(this, 3, 2);
      String[] var2 = new String[]{"New Game", "Continue Game", "Help", "Credits", "Exit"};
      this.mainMenuUI.setupList("Main Menu", var2, false);
      this.NPCQuestionWhatUI = new Screen(this, 5, 27);
      String[] var3 = new String[]{"North wall defense", "East wall defense", "Arguing with governor", "Imperial aid", "Ice tribes", "Gates before attack"};
      this.NPCQuestionWhatUI.setupPromptList("", "Ask about what?", var3);
      this.newGameUI = new Screen(this, 5, 3);
      String[] var4 = Player.classNames;
      this.newGameUI.setupPromptList("New Game", "Select a Class:", var4);
      this.splashUI.percent = 35;
      this.characterMainUI = new Screen(this, 6, 4);
      String[] var5 = new String[]{"See Class Info", "Create Character"};
      this.characterMainUI.setupPromptList("Character", "You selected:", "", var5);
      this.NPCChoicesUI = new Screen[9];
      String[] var6 = new String[]{"Buy", "Sell"};

      for (int var7 = 0; var7 < 4; var7++) {
         this.NPCChoicesUI[var7] = new Screen(this, 5, 9 + var7);
         this.NPCChoicesUI[var7].setupPromptList(Shop.NAMES[var7], "Your gold: <TAG>", var6);
         this.NPCChoicesUI[var7].backTarget = this.gameCanvas;
      }

      this.NPCChoicesUI[4] = new Screen(this, 5, 13);
      String[] var8 = new String[]{"Rumors", "Cure", "Warp", "Recovery"};
      this.NPCChoicesUI[4].setupPromptList("Eustacia", "Welcome", var8);
      this.NPCChoicesUI[4].backTarget = this.gameCanvas;
      String[] var9 = new String[]{"Train", "Give", "Befriend", "Threaten", "Ask a question", "Warp"};

      for (int var10 = 5; var10 < 9; var10++) {
         this.NPCChoicesUI[var10] = new Screen(this, 5, 9 + var10);
         this.NPCChoicesUI[var10].setupPromptList(Shop.NAMES[var10], "Aid: <TAG>", var9);
         this.NPCChoicesUI[var10].backTarget = this.gameCanvas;
      }

      this.OptionsUI = new Screen(this, 3, 31);
      String[] var11 = new String[]{"Stats", "Inventory", "Clue Log", "Skills", "Spells", "Save Game", "Load Game", "Help", "Reveal Traitor", "Quit Game"};
      this.OptionsUI.setupList("Options", var11, false);
      this.OptionsUI.addCommand(backCommand);
      this.ClueUI = new Screen(this, 5, 60);
      String[] var12 = new String[]{"Alhavara", "Beatrice", "Chung", "Delacroix", "Rumors"};
      this.ClueUI.setupPromptList("Clue Log", "", var12);
      this.ClueUI.backTarget = this.OptionsUI;
      this.helpUI = new Screen(this, 3, 203);
      this.helpUI.setupList("Help", helpTitles, true);
      this.splashUI.percent = 42;
      this.charNameTextForm = new Form("Enter name");
      StringItem var13 = new StringItem(null, "Enter a name for your character");
      this.charNameTextForm.append(var13);
      TextField var14 = new TextField(null, null, 10, 0);
      this.charNameTextForm.append(var14);
      this.charNameTextForm.addCommand(okCommand);
      this.charNameTextForm.setCommandListener(this);
      this.noSavedGameUI = new Screen(this, 4, 305);
      this.noSavedGameUI.setupMessage("Unavailable", "No game is available for loading. Press OK to return to main menu.");
      this.noSavedGameUI.backTarget = this.mainMenuUI;
      this.splashUI.returnDisplay = this.mainMenuUI;
      this.newGameUI.backTarget = this.mainMenuUI;
      this.characterMainUI.backTarget = this.newGameUI;
      this.splashUI.percent = 55;
      System.gc();
   }

   private Screen newGiveWhat(int var1) {
      System.gc();
      Screen var2 = new Screen(this, 5, 22);
      var2.contextIndex = var1;
      String[] var3 = new String[this.character.inventoryCount];

      for (int var4 = 0; var4 < this.character.inventoryCount; var4++) {
         int var5 = Math.abs(this.character.inventoryItemIds[var4]);
         if (this.character.isEquipped(var4)) {
            var3[var4] = "E:" + Item.nameOf(var5);
         } else {
            var3[var4] = Item.nameOf(var5);
         }
      }

      var2.setupPromptList(Shop.NAMES[var1], "Give What?", var3);
      var2.backTarget = this.NPCChoicesUI[var1];
      return var2;
   }

   private Screen newSellWhat(int var1) {
      System.gc();
      Screen var2 = new Screen(this, 5, 52);
      var2.contextIndex = var1;
      String[] var3 = new String[this.character.inventoryCount];

      for (int var4 = 0; var4 < this.character.inventoryCount; var4++) {
         int var5 = Math.abs(this.character.inventoryItemIds[var4]);
         if (this.character.isEquipped(var4)) {
            var3[var4] = "E:" + Item.nameOf(var5) + " (" + Item.column(5, var5) + ")";
         } else {
            var3[var4] = Item.nameOf(var5) + " (" + Item.column(5, var5) + ")";
         }
      }

      var2.setupPromptList(Shop.NAMES[var1], "Sell What?", var3);
      var2.backTarget = null;
      return var2;
   }

   private Screen newBuyWhat(int var1) {
      System.gc();
      Screen var2 = new Screen(this, 5, 50);
      var2.contextIndex = var1;
      int var3 = Shop.SHOP_STOCK[var1].length;
      String[] var4 = new String[var3];

      for (int var5 = 0; var5 < var3; var5++) {
         int var6 = Math.abs(Shop.SHOP_STOCK[var1][var5]);
         var4[var5] = Item.nameOf(var6) + " (" + Item.column(4, var6) + ")";
      }

      var2.setupPromptList(Shop.NAMES[var1], "Buy What?", var4);
      return var2;
   }

   private Screen newTrainWhat(int var1) {
      System.gc();
      Screen var2 = new Screen(this, 5, 20);
      var2.contextIndex = var1;
      String[] var3 = new String[3];
      int var4 = 0;

      for (int var5 = 0; var5 < 14; var5++) {
         if (Shop.isValidShopAction(var1, var5)) {
            int var6 = this.character.skillValue(var5, false);
            String var7 = Player.skillNames[var5] + " (<TAG>)";
            var3[var4++] = Util.replace(var7, "<TAG>", var6);
         }
      }

      var2.setupPromptList(Shop.NAMES[var1], "Train What?", var3);
      var2.backTarget = this.NPCChoicesUI[var1];
      return var2;
   }

   private Screen newWarpWhere() {
      System.gc();
      Screen var1 = new Screen(this, 5, 29);
      var1.contextIndex = 4;
      Vector var2 = new Vector();
      var2.addElement("Your last location");

      for (int var3 = 0; var3 < 4; var3++) {
         if (!Shop.firstVisit[5 + var3]) {
            var2.addElement(Shop.NAMES[5 + var3]);
         }
      }

      int var4 = var2.size();
      String[] var5 = new String[var4];

      for (int var6 = 0; var6 < var4; var6++) {
         var5[var6] = (String)var2.elementAt(var6);
      }

      var1.setupPromptList("Eustacia", "", var5);
      var1.backTarget = this.NPCChoicesUI[4];
      return var1;
   }

   private Screen newInventoryUI() {
      System.gc();
      Screen var1 = new Screen(this, 5, 33);
      String[] var2 = new String[this.character.inventoryCount];

      for (int var3 = 0; var3 < this.character.inventoryCount; var3++) {
         byte var4 = this.character.inventoryItemIds[var3];
         if (var4 < 0) {
            var2[var3] = "E: " + Item.nameOf(Math.abs(var4));
         } else {
            var2[var3] = Item.nameOf(var4);
         }
      }

      String var5 = Util.replace("Your gold: <TAG>", "<TAG>", this.character.gold);
      var1.setupPromptList("Inventory", var5, var2);
      var1.backTarget = this.OptionsUI;
      return var1;
   }

   private Screen newConfirmQuitUI(Screen var1) {
      System.gc();
      Screen var2 = new Screen(this, 5, 202);
      String[] var3 = new String[]{"Yes", "No"};
      var2.setupPromptList("Quit?", "Are you sure?", var3);
      var2.removeCommand(cancelCommand);
      var2.backTarget = var1;
      return var2;
   }

   public void pauseApp() {
      this.gameCanvas.pauseThread();
      if (theMIDletState != 4) {
         theMIDletState = 3;
      }
   }

   public void destroyApp(boolean var1) {
      theMIDletState = 4;
   }

   public void commandAction(Command var1, Displayable var2) {
      try {
         this.commandAction1(var1, var2);
      } catch (Throwable var4) {
         this.displayDebug();
         System.out.println("commandAction1 " + var4.toString());
      }
   }

   private void commandAction1(Command var1, Displayable var2) throws Exception {
      debugCode = 18;
      if (uic != null) {
         if (var1 == cancelCommand && uic.backTarget != null) {
            this.setCurrentDisplay(uic.backTarget);
            return;
         }

         if (uic.mode == 410) {
            this.exit();
         } else if (uic.mode == 2) {
            if (var1 == selectCommand) {
               int var3 = uic.selectedIndexOrMinusOne();
               switch (var3) {
                  case 0:
                     System.gc();
                     this.setCurrentDisplay(this.newGameUI);
                     break;
                  case 1:
                     System.gc();
                     this.gameCanvas.stopGameThread();
                     loadGameUI = new LoadingScreen(this, 9, 302);
                     loadGameUI.unusedHook2();
                     Thread var4 = new Thread(this);
                     helperThreadState = 6;
                     this.noSavedGameUI.backTarget = this.mainMenuUI;
                     this.setCurrentDisplay(loadGameUI);
                     var4.start();
                     break;
                  case 2:
                     this.helpUI.backTarget = this.mainMenuUI;
                     this.setCurrentDisplay(this.helpUI);
                     break;
                  case 3:
                     this.GenericInfoUI.setSecondaryParam(204);
                     this.GenericInfoUI.setupMessage("Credits", creditsString);
                     this.setCurrentDisplay(this.GenericInfoUI);
                     break;
                  case 4:
                     this.confirmQuitUI = this.newConfirmQuitUI(uic);
                     this.setCurrentDisplay(this.confirmQuitUI);
               }
            }
         } else if (uic.mode == 3) {
            if (var1 == selectCommand) {
               int var9 = uic.selectedIndexOrMinusOne();
               String var42 = uic.selectedItemText();
               this.character = null;
               System.gc();
               this.character = new Player(this);
               this.character.applyClassTemplate(var9);
               this.characterMainUI.setTextColumn(1, var42);
               this.setCurrentDisplay(this.characterMainUI);
            }
         } else if (uic.mode == 4) {
            if (var1 == selectCommand) {
               int var10 = uic.selectedIndexOrMinusOne();
               if (var10 == 0) {
                  String var43 = this.character.buildCreationSummary();
                  this.GenericInfoUI.setSecondaryParam(5);
                  this.GenericInfoUI.setupMessage("Info", var43);
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else {
                  this.GenericInfoUI.setSecondaryParam(6);
                  this.GenericInfoUI.setupMessage("New Character", "Character Created!\n \nPress 'Ok' to enter a name");
                  this.setCurrentDisplay(this.GenericInfoUI);
               }
            }
         } else if (uic.mode == 5) {
            if (var1 == okCommand) {
               this.setCurrentDisplay(this.characterMainUI);
            }
         } else if (uic.mode == 6) {
            if (var1 == okCommand) {
               this.setCurrentDisplay(this.charNameTextForm);
            }
         } else if (uic.mode == 7) {
            System.gc();
            printMemory("Going into game");
            this.GenericInfoUI.setSecondaryParam(101);
            this.GenericInfoUI.setupMessage("Introduction", Shop.dialogue[9][3]);
            this.setCurrentDisplay(this.GenericInfoUI);
         } else if (uic.mode == 101) {
            this.GenericInfoUI.setSecondaryParam(102);
            this.GenericInfoUI.setupMessage("Introduction", Shop.dialogue[9][4] + Shop.dialogue[9][5]);
            this.setCurrentDisplay(this.GenericInfoUI);
         } else if (uic.mode == 102) {
            if (var1 == okCommand) {
               this.gameCanvas.player = this.character;
               this.character.resetState(false);
               this.gameCanvas.startGameThread();
               this.setCurrentDisplay(this.gameCanvas);
            }
         } else if (uic.mode != 8 && uic.mode != 360) {
            if (uic.mode >= 9 && uic.mode <= 17) {
               if (var1 == cancelCommand) {
                  this.setCurrentDisplay(uic.backTarget);
               } else {
                  this.handleNPCChoices(uic);
               }
            } else if (uic.mode == 20) {
               if (var1 == selectCommand) {
                  int var11 = uic.contextIndex;
                  int var44 = uic.selectedIndexOrMinusOne();
                  int var5 = Shop.shopActionCode(var11, var44);
                  this.handleNPCAction(var11, 21, 5, var5);
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else if (var1 == cancelCommand) {
                  int var12 = uic.contextIndex;
                  this.setAidPointsForNPC(var12);
                  this.setCurrentDisplay(this.NPCChoicesUI[var12]);
               }
            } else if (uic.mode == 52) {
               if (var1 == selectCommand) {
                  int var13 = uic.contextIndex;
                  int var45 = uic.selectedIndexOrMinusOne();
                  if (var45 >= 0) {
                     this.currentItemIndex = var45;
                     if (this.character.isEquipped(var45)) {
                        int var62 = Math.abs(this.character.inventoryItemIds[var45]);
                        String[] var6 = new String[]{"No", "Yes"};
                        this.NPCSellSureUI = new Screen(this, 5, 54);
                        this.NPCSellSureUI.contextIndex = var13;
                        String var7 = "You may sell " + Item.nameOf(var62) + " for " + Item.column(5, var62) + ". Confirm?";
                        this.NPCSellSureUI.setupPromptList(Shop.NAMES[var13], var7, var6);
                        this.NPCSellSureUI.removeCommand(cancelCommand);
                        this.setCurrentDisplay(this.NPCSellSureUI);
                     } else {
                        this.handleNPCAction(var13, 53, 15, var45);
                        this.setCurrentDisplay(this.GenericInfoUI);
                     }
                  }
               } else if (var1 == cancelCommand) {
                  int var14 = uic.contextIndex;
                  this.setAidPointsForNPC(var14);
                  this.setCurrentDisplay(this.NPCChoicesUI[var14]);
               }
            } else if (uic.mode == 54) {
               if (var1 == selectCommand) {
                  int var15 = uic.selectedIndexOrMinusOne();
                  if (var15 == 0) {
                     this.setCurrentDisplay(this.NPCSellWhatUI);
                  } else {
                     int var46 = uic.contextIndex;
                     this.handleNPCAction(var46, 53, 15, this.currentItemIndex);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               }
            } else if (uic.mode == 50) {
               if (var1 == selectCommand) {
                  int var16 = uic.contextIndex;
                  int var47 = uic.selectedIndexOrMinusOne();
                  if (var47 >= 0) {
                     this.currentItemIndex = var47;
                     this.handleNPCAction(var16, 51, 14, var47);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (var1 == cancelCommand) {
                  int var17 = uic.contextIndex;
                  this.setAidPointsForNPC(var17);
                  this.setCurrentDisplay(this.NPCChoicesUI[var17]);
               }
            } else if (uic.mode == 27) {
               if (var1 == selectCommand) {
                  System.gc();
                  this.currentQWhat = uic.selectedIndexOrMinusOne();
                  int var18 = uic.contextIndex;
                  this.NPCQuestionWhomUI = new Screen(this, 5, 28);
                  this.NPCQuestionWhomUI.contextIndex = var18;
                  String[] var48 = new String[3];
                  int var63 = 0;

                  for (int var73 = 0; var73 < 4; var73++) {
                     if (var73 + 5 != var18) {
                        var48[var63] = Shop.NAMES[var73 + 5];
                        var63++;
                     }
                  }

                  this.NPCQuestionWhomUI.setupPromptList(Shop.NAMES[var18], "Ask about whom?", var48);
                  this.NPCQuestionWhomUI.backTarget = this.NPCChoicesUI[var18];
                  this.setCurrentDisplay(this.NPCQuestionWhomUI);
               } else if (var1 == cancelCommand) {
                  int var19 = uic.contextIndex;
                  this.setAidPointsForNPC(var19);
                  this.setCurrentDisplay(this.NPCChoicesUI[var19]);
               }
            } else if (uic.mode == 28) {
               if (var1 == selectCommand) {
                  this.currentQWhom = uic.selectedIndexOrMinusOne();
                  int var20 = uic.contextIndex;
                  int var49 = (var20 - 5) * 18 + this.currentQWhat * 3 + this.currentQWhom;
                  String var64 = "";
                  int var74 = this.currentQWhom;
                  if (var74 >= var20 - 5) {
                     var74++;
                  }

                  if (this.character.eventFlags[var49]) {
                     var49 = this.currentQWhat * 4 + var74;
                     if (this.character.eventFlags[72 + this.currentQWhat * 3 + this.currentQWhom]) {
                        var64 = Shop.dialogue[9][5 + Shop.UNCONFIRMED_B[var49]];
                     } else {
                        var64 = Shop.dialogue[9][5 + Shop.UNCONFIRMED_A[var49]];
                     }
                  } else {
                     this.character.eventFlags[var49] = true;
                     Shop.rewardsGiven[var20 - 5]--;
                     var49 = this.currentQWhat * 4 + var74;
                     if (var20 - 5 == this.character.traitorIndex) {
                        if (this.character.traitorSuspicionCount < 3) {
                           this.character.traitorSuspicionCount++;
                        }

                        if (this.character.traitorSuspicionCount == 2 || this.character.traitorSuspicionCount == 3 && nextInt(100) < 20) {
                           var64 = Shop.dialogue[9][5 + Shop.UNCONFIRMED_B[var49]];
                           var49 = 72 + this.currentQWhat * 3 + this.currentQWhom;
                           this.character.eventFlags[var49] = true;
                        }
                     }

                     if (var64 == "") {
                        var64 = Shop.dialogue[9][5 + Shop.UNCONFIRMED_A[var49]];
                     }
                  }

                  this.GenericInfoUI.setSecondaryParam(26);
                  this.GenericInfoUI.setupMessage(Shop.NAMES[var20], var64);
                  this.GenericInfoUI.contextIndex = var20;
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else if (var1 == cancelCommand) {
                  int var21 = uic.contextIndex;
                  this.setAidPointsForNPC(var21);
                  this.setCurrentDisplay(this.NPCChoicesUI[var21]);
               }
            } else if (uic.mode == 22) {
               if (var1 == selectCommand) {
                  int var22 = uic.contextIndex;
                  int var52 = uic.selectedIndexOrMinusOne();
                  if (var52 >= 0) {
                     this.handleNPCAction(var22, 23, 4, var52);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (var1 == cancelCommand) {
                  int var23 = uic.contextIndex;
                  this.setAidPointsForNPC(var23);
                  this.setCurrentDisplay(this.NPCChoicesUI[var23]);
               }
            } else if (uic.mode == 29) {
               if (var1 == selectCommand) {
                  int var24 = uic.selectedIndexOrMinusOne();
                  if (var24 == 0) {
                     this.handleNPCAction(4, 41, 11, 0);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  } else {
                     int var53 = var24 - 1;

                     for (int var65 = 0; var65 < 4; var65++) {
                        if (!Shop.firstVisit[5 + var65]) {
                           if (var53 == 0) {
                              var24 = var65;
                              break;
                           }

                           var53--;
                        }
                     }

                     if (var24 == 0) {
                        this.character.currentLevel = this.character.pendingLevel = 3;
                     } else if (var24 == 1) {
                        this.character.currentLevel = this.character.pendingLevel = 12;
                     } else if (var24 == 2) {
                        this.character.currentLevel = this.character.pendingLevel = 21;
                     } else if (var24 == 3) {
                        this.character.currentLevel = this.character.pendingLevel = 30;
                     }

                     this.character.tileX = this.character.pendingTileX = Shop.SHOP_X[5 + var24];
                     this.character.tileY = this.character.pendingTileY = (byte)(Shop.SHOP_Y[5 + var24] + 1);
                     this.character.facing = 1;
                     this.character.refreshCorridorView();
                     this.setCurrentDisplay(this.gameCanvas);
                  }
               }
            } else if (uic.mode != 23 && uic.mode != 21 && uic.mode != 24 && uic.mode != 25 && uic.mode != 26 && uic.mode != 353 && uic.mode != 355) {
               if (uic.mode == 53) {
                  if (var1 == okCommand) {
                     this.NPCSellWhatUI = this.newSellWhat(uic.contextIndex);
                     this.NPCSellWhatUI.setSelectedIndex(this.currentItemIndex);
                     this.setCurrentDisplay(this.NPCSellWhatUI);
                  }
               } else if (uic.mode == 51) {
                  if (var1 == okCommand) {
                     this.NPCBuyWhatUI = this.newBuyWhat(uic.contextIndex);
                     this.NPCBuyWhatUI.setSelectedIndex(this.currentItemIndex);
                     this.setCurrentDisplay(this.NPCBuyWhatUI);
                  }
               } else if (uic.mode == 69) {
                  if (var1 == selectCommand) {
                     int var26 = uic.selectedIndexOrMinusOne();
                     if (var26 == 0) {
                        this.character.markCampAndReturnToTown(true);
                        this.character.suppressStrafeAdjust = false;
                        this.setCurrentDisplay(this.gameCanvas);
                     } else {
                        this.setCurrentDisplay(uic.backTarget);
                     }
                  }
               } else if (uic.mode == 41) {
                  if (var1 == okCommand) {
                     this.character.suppressStrafeAdjust = false;
                     this.setCurrentDisplay(this.gameCanvas);
                  }
               } else if (uic.mode == 31) {
                  if (var1 == selectCommand) {
                     int var27 = uic.selectedIndexOrMinusOne();
                     switch (var27) {
                        case 0:
                           this.GenericInfoUI.setSecondaryParam(32);
                           this.GenericInfoUI.setupMessage("Stats", this.gameCanvas.player.buildCharacterSheet());
                           this.setCurrentDisplay(this.GenericInfoUI);
                           break;
                        case 1:
                           this.InventoryUI = this.newInventoryUI();
                           this.setCurrentDisplay(this.InventoryUI);
                           break;
                        case 2:
                           this.setCurrentDisplay(this.ClueUI);
                           break;
                        case 3:
                           this.SkillsListUI = this.newSkillsListUI();
                           this.setCurrentDisplay(this.SkillsListUI);
                           break;
                        case 4:
                           this.SpellsListUI = this.newSpellsListUI();
                           this.setCurrentDisplay(this.SpellsListUI);
                           break;
                        case 5:
                           saveGameUI = new LoadingScreen(this, 10, 303);
                           saveGameUI.unusedHook2();
                           Thread var54 = new Thread(this);
                           helperThreadState = 5;
                           this.setCurrentDisplay(saveGameUI);
                           var54.start();
                           break;
                        case 6:
                           System.gc();
                           this.gameCanvas.stopGameThread();
                           loadGameUI = new LoadingScreen(this, 9, 302);
                           loadGameUI.unusedHook2();
                           Thread var66 = new Thread(this);
                           helperThreadState = 6;
                           this.noSavedGameUI.backTarget = this.OptionsUI;
                           this.setCurrentDisplay(loadGameUI);
                           var66.start();
                           break;
                        case 7:
                           this.helpUI.backTarget = this.OptionsUI;
                           this.setCurrentDisplay(this.helpUI);
                           break;
                        case 8:
                           this.GenericInfoUI.setSecondaryParam(68);
                           this.GenericInfoUI.setupMessage("Reveal Traitor", Shop.dialogue[9][66]);
                           this.setCurrentDisplay(this.GenericInfoUI);
                           break;
                        case 9:
                           this.confirmQuitUI = this.newConfirmQuitUI(uic);
                           this.setCurrentDisplay(this.confirmQuitUI);
                     }
                  } else if (var1 == backCommand) {
                     this.setCurrentDisplay(this.gameCanvas);
                  }
               } else if (uic.mode == 32) {
                  if (var1 == okCommand) {
                     this.setCurrentDisplay(this.OptionsUI);
                  }
               } else if (uic.mode == 33) {
                  try {
                     debugCode = 1;
                     if (var1 == selectCommand) {
                        debugCode = 2;
                        int var28 = uic.selectedIndexOrMinusOne();
                        if (var28 >= 0) {
                           debugCode = 3;
                           this.InventoryItemUI = this.newInventoryItemUI(var28);
                           debugCode = 4;
                           this.currentItemIndex = var28;
                           debugCode = 5;
                           this.setCurrentDisplay(this.InventoryItemUI);
                        }
                     }
                  } catch (Exception var8) {
                     System.out.println(var8.toString());
                     Form var55 = new Form("Error");
                     StringItem var67 = new StringItem("Error", "code = " + debugCode + " " + var8.toString());
                     var55.append(var67);
                     var55.addCommand(okCommand);
                     var55.setCommandListener(this);
                     Display.getDisplay(this).setCurrent(var55);
                  }
               } else if (uic.mode == 34) {
                  if (var1 == selectCommand) {
                     int var29 = uic.selectedIndexOrMinusOne();
                     if (var29 == 0) {
                        this.character.dropInventoryItem(this.currentItemIndex);
                     } else {
                        if (this.character.canEquipOrUnequip(this.currentItemIndex)) {
                           if (--var29 == 0) {
                              if (!this.character.isEquipped(this.currentItemIndex)) {
                                 this.character.equipItem(this.currentItemIndex, true);
                              } else {
                                 this.character.unequipInventorySlot(this.currentItemIndex);
                              }
                           }
                        }

                        if (var29 > 0 && this.character.canLearnSpell(this.currentItemIndex)) {
                           if (--var29 == 0) {
                              this.character.learnSpellFromScroll(this.currentItemIndex);
                           }
                        }

                        if (var29 > 0 && this.character.canUseItem(this.currentItemIndex)) {
                           if (--var29 == 0) {
                              this.character.useItem(this.currentItemIndex);
                           }
                        }
                     }

                     if (this.character.suppressStrafeAdjust) {
                        this.character.suppressStrafeAdjust = false;
                        this.setCurrentDisplay(this.gameCanvas);
                     } else {
                        this.InventoryUI = this.newInventoryUI();
                        this.InventoryUI.setSelectedIndex(this.currentItemIndex);
                        this.setCurrentDisplay(this.InventoryUI);
                     }

                     this.currentItemIndex = -1;
                  }
               } else if (uic.mode == 60) {
                  if (var1 == selectCommand) {
                     int var31 = uic.selectedIndexOrMinusOne();
                     this.newClueLogUI(var31);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (uic.mode == 61) {
                  if (var1 == okCommand) {
                     this.setCurrentDisplay(this.ClueUI);
                  }
               } else if (uic.mode == 35) {
                  if (var1 == selectCommand) {
                     int var32 = uic.selectedIndexOrMinusOne();
                     this.GenericInfoUI.setSecondaryParam(36);
                     int var56 = this.character.nthKnownSkillIndex(var32);
                     String var68 = this.character.skillTooltip(var56);
                     this.GenericInfoUI.setupMessage("Skill Info", var68);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (uic.mode == 36) {
                  if (var1 == okCommand) {
                     this.setCurrentDisplay(this.SkillsListUI);
                  }
               } else if (uic.mode == 37) {
                  if (var1 == selectCommand) {
                     int var33 = uic.selectedIndexOrMinusOne();
                     if (var33 >= 0) {
                        this.SpellInfoUI = this.newSpellInfoUI(var33);
                        this.currentSpellIndex = var33;
                        this.setCurrentDisplay(this.SpellInfoUI);
                     }
                  }
               } else if (uic.mode == 38) {
                  if (var1 == selectCommand) {
                     int var34 = this.character.nthKnownSpellId(this.currentSpellIndex);
                     this.character.selectedSpellId = (byte)(var34 + 1);
                     this.SpellsListUI = this.newSpellsListUI();
                     this.SpellsListUI.setSelectedIndex(this.currentSpellIndex);
                     this.setCurrentDisplay(this.SpellsListUI);
                     this.currentSpellIndex = -1;
                  }
               } else if (uic.mode == 68) {
                  this.RevealUI = this.newRevealUI();
                  this.setCurrentDisplay(this.RevealUI);
               } else if (uic.mode == 65) {
                  if (var1 == selectCommand) {
                     int var35 = uic.selectedIndexOrMinusOne();
                     if (var35 == 0) {
                        this.RevealUI = this.newRevealWhomUI();
                        this.setCurrentDisplay(this.RevealUI);
                     } else {
                        this.setCurrentDisplay(uic.backTarget);
                     }
                  }
               } else if (uic.mode == 66) {
                  if (var1 == selectCommand) {
                     int var36 = uic.selectedIndexOrMinusOne();
                     StringBuffer var57 = new StringBuffer();
                     var57.append(Shop.dialogue[9][68]);
                     var57.append("\n");
                     var57.append(Shop.dialogue[9][69]);
                     var57.append("\n");
                     if (var36 == this.character.traitorIndex) {
                        this.character.newGamePlus = true;
                        this.character.grantStarFrostItem();
                        var57.append(Shop.dialogue[9][70]);
                     } else {
                        var57.append(Util.replace(Shop.dialogue[9][72], "<TAG>", Shop.NAMES[5 + this.character.traitorIndex]));
                     }

                     this.GenericInfoUI.setSecondaryParam(67);
                     this.GenericInfoUI.setupMessage("Reveal Traitor", var57.toString());
                     this.character.resetToHubPosition(false);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (uic.mode == 67) {
                  this.character.ambushTimer = 1;
                  this.character.specialEncounterResolved = true;
                  this.setCurrentDisplay(this.gameCanvas);
               } else if (uic.mode == 39) {
                  if (var1 == selectCommand) {
                     String var37 = uic.selectedItemText();
                     attribIncr[uic.contextIndex] = -1;

                     for (int var58 = 0; var58 < Player.attributeNames.length; var58++) {
                        if (var37.equals(Player.attributeNames[var58])) {
                           attribIncr[uic.contextIndex] = var58;
                           break;
                        }
                     }

                     if (uic.contextIndex < 2) {
                        int var69 = uic.contextIndex + 1;
                        this.LevelUpUI = this.newLevelUpUI(var69 + 1);
                        this.setCurrentDisplay(this.LevelUpUI);
                     } else {
                        this.character.attributes[attribIncr[0]] = (short)(this.character.attributes[attribIncr[0]] + 3);
                        this.character.attributes[attribIncr[1]] = (short)(this.character.attributes[attribIncr[1]] + 2);
                        this.character.attributes[attribIncr[2]]++;
                        this.character.recalcMaxStats();
                        this.character.levelUp();
                        this.setCurrentDisplay(this.gameCanvas);
                        this.gameCanvas.resumeThread();
                     }
                  }
               } else if (uic.mode == 202) {
                  if (var1 == selectCommand) {
                     int var38 = uic.selectedIndexOrMinusOne();
                     if (var38 == 0) {
                        this.GenericInfoUI.setSecondaryParam(399);
                        String var59 = "";

                        for (int var70 = 0; var70 < copyString.length; var70++) {
                           var59 = var59 + copyString[var70];
                        }

                        this.GenericInfoUI.setupMessage("Exiting", var59);
                        this.GenericInfoUI.removeCommand(okCommand);
                        this.GenericInfoUI.addCommand(exitCommand);
                        this.setCurrentDisplay(this.GenericInfoUI);
                     } else {
                        this.setCurrentDisplay(uic.backTarget);
                     }
                  }
               } else if (uic.mode == 202) {
                  this.exit();
               } else if (uic.mode == 40) {
                  this.setCurrentDisplay(this.gameCanvas);
                  this.gameCanvas.resumeThread();
               } else if (uic.mode == 203) {
                  if (var1 == selectCommand) {
                     int var39 = uic.selectedIndexOrMinusOne();
                     this.GenericInfoUI.setSecondaryParam(206);
                     this.GenericInfoUI.setupMessage(helpTitles[var39], helpStrings[var39]);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  } else {
                     this.setCurrentDisplay(uic.backTarget);
                  }
               } else if (uic.mode == 206) {
                  this.setCurrentDisplay(this.helpUI);
               } else if (uic.mode == 204) {
                  this.setCurrentDisplay(this.mainMenuUI);
               } else if (uic.mode == 305) {
                  if (uic.backTarget == this.OptionsUI) {
                     this.gameCanvas.startGameThread();
                  }

                  this.setCurrentDisplay(uic.backTarget);
               } else if (uic.mode == 200 || uic.mode == 201) {
                  this.GenericInfoUI.setSecondaryParam(399);
                  String var40 = "";

                  for (int var60 = 0; var60 < copyString.length; var60++) {
                     var40 = var40 + copyString[var60];
                  }

                  this.GenericInfoUI.setupMessage("Exiting", var40);
                  this.GenericInfoUI.removeCommand(okCommand);
                  this.GenericInfoUI.addCommand(exitCommand);
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else if (uic.mode == 399) {
                  this.exit();
               } else if (uic.mode == 499) {
                  this.exit();
               }
            } else if (var1 == okCommand) {
               int var25 = uic.contextIndex;
               this.setAidPointsForNPC(var25);
               this.setCurrentDisplay(this.NPCChoicesUI[var25]);
            }
         } else if (var1 == okCommand) {
            this.setCurrentDisplay(this.NPCChoicesUI[uic.contextIndex]);
         }
      } else if (var2 == this.errorForm) {
         this.exit();
      } else if (var2 == this.charNameTextForm && var1 == okCommand) {
         TextField var41 = (TextField)this.charNameTextForm.get(1);
         String var61 = var41.getString();
         if (var61.length() < 3) {
            Alert var71 = new Alert("Error", Util.replace("Your character name must be at least <TAG> letters", "<TAG>", 3), null, AlertType.ERROR);
            var71.setTimeout(-2);
            this.setCurrentDisplay(var71);
         } else {
            this.character.name = var61;
            this.GenericInfoUI.setSecondaryParam(7);
            this.GenericInfoUI.setupMessage("Welcome", "Welcome to The Elder Scrolls Travels!");
            this.createGameUI = new LoadingScreen(this, 8, 301);
            this.createGameUI.unusedHook2();
            Thread var72 = new Thread(this);
            helperThreadState = 4;
            this.setCurrentDisplay(this.createGameUI);
            var72.start();
         }
      }
   }

   private void handleNPCChoices(Screen var1) {
      int var2 = var1.selectedIndexOrMinusOne();
      int var3 = var1.mode - 9;
      switch (var3) {
         case 0:
         case 1:
         case 2:
         case 3:
            if (var2 == 0) {
               this.NPCBuyWhatUI = this.newBuyWhat(var3);
               this.setCurrentDisplay(this.NPCBuyWhatUI);
            } else if (var2 == 1) {
               if (this.character.inventoryCount <= 0) {
                  this.GenericInfoUI.setSecondaryParam(53);
                  this.GenericInfoUI.setupMessage(Shop.NAMES[var3], "You have nothing to give me!");
                  this.GenericInfoUI.contextIndex = var3;
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else {
                  this.NPCSellWhatUI = this.newSellWhat(var3);
                  this.setCurrentDisplay(this.NPCSellWhatUI);
               }
            }
            break;
         case 4:
            if (var2 == 0) {
               String var6 = Shop.dialogue(this.character, 4, 13, 0);
               if (var6 == null) {
                  var6 = "I have no new rumors.";
               }

               this.GenericInfoUI.setSecondaryParam(360);
               this.GenericInfoUI.setupMessage(Shop.NAMES[4], var6);
               this.GenericInfoUI.contextIndex = 4;
               this.setCurrentDisplay(this.GenericInfoUI);
            } else if (var2 == 1) {
               this.handleNPCAction(var3, 353, 10, 0);
               this.setCurrentDisplay(this.GenericInfoUI);
            } else if (var2 == 2) {
               this.WarpWhereUI = this.newWarpWhere();
               this.setCurrentDisplay(this.WarpWhereUI);
            } else if (var2 == 3) {
               this.handleNPCAction(var3, 355, 12, 0);
               this.setCurrentDisplay(this.GenericInfoUI);
            }
            break;
         case 5:
         case 6:
         case 7:
         case 8:
            if (var2 == 0) {
               this.NPCTrainWhatUI = this.newTrainWhat(var3);
               this.setCurrentDisplay(this.NPCTrainWhatUI);
            } else if (var2 == 1) {
               if (this.character.inventoryCount <= 0) {
                  this.GenericInfoUI.setSecondaryParam(23);
                  this.GenericInfoUI.setupMessage(Shop.NAMES[var3], "You have nothing to give me!");
                  this.GenericInfoUI.contextIndex = var3;
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else {
                  this.NPCGiveWhatUI = this.newGiveWhat(var3);
                  this.setCurrentDisplay(this.NPCGiveWhatUI);
               }
            } else if (var2 == 2) {
               this.handleNPCAction(var3, 24, 2, 0);
               this.setCurrentDisplay(this.GenericInfoUI);
            } else if (var2 == 3) {
               this.handleNPCAction(var3, 25, 3, 0);
               this.setCurrentDisplay(this.GenericInfoUI);
            } else if (var2 == 4) {
               if (Shop.rewardsGiven[var3 - 5] == 0) {
                  String var4 = Shop.dialogue[var3][15];
                  this.GenericInfoUI.setSecondaryParam(26);
                  this.GenericInfoUI.setupMessage(Shop.NAMES[var3], var4);
                  this.GenericInfoUI.contextIndex = var3;
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else {
                  this.NPCQuestionWhatUI.contextIndex = var3;
                  this.NPCQuestionWhatUI.setTitle(Shop.NAMES[var3]);
                  this.NPCQuestionWhatUI.backTarget = this.NPCChoicesUI[var3];
                  this.setCurrentDisplay(this.NPCQuestionWhatUI);
               }
            } else if (var2 == 5) {
               this.NPCWarpUI = new Screen(this, 5, 69);
               this.NPCWarpUI.contextIndex = var3;
               String[] var5 = new String[]{"Yes", "No"};
               this.NPCWarpUI.setupPromptList(Shop.NAMES[var3], "Warp to camp", var5);
               this.NPCWarpUI.backTarget = this.NPCChoicesUI[var3];
               this.setCurrentDisplay(this.NPCWarpUI);
            }
      }
   }

   private void handleNPCAction(int var1, int var2, int var3, int var4) {
      this.GenericInfoUI.setSecondaryParam(var2);
      String var5 = Shop.dialogue(this.character, var1, var3, var4);
      this.GenericInfoUI.setupMessage(Shop.NAMES[var1], var5);
      this.GenericInfoUI.contextIndex = var1;
   }

   private void createImageFromFile() throws Exception {
      Hashtable var1 = new Hashtable(50);
      int var2 = -1;
      int var3 = -1;
      int var4 = -1;
      int var5 = 0;
      DataInputStream var10 = null;
      Object var11 = null;

      try {
         var10 = Util.openResource("/imgfiles.lmp");

         while (var4 == -1 || var5 < var4) {
            String var12 = "";
            int var6 = var10.read();
            var5++;

            while (var6 != 45) {
               var12 = var12 + (char)var6;
               var6 = var10.read();
               var5++;
            }

            if (var12.length() > 1) {
               var3 = ((var10.read() & 0xFF) << 24) + ((var10.read() & 0xFF) << 16) + ((var10.read() & 0xFF) << 8) + (var10.read() & 0xFF);
               var2 = ((var10.read() & 0xFF) << 8) + (var10.read() & 0xFF);
               var5 += 6;
               var1.put(new Integer(var3), var12 + "-" + var2);
               if (var4 < 0) {
                  var4 = var3;
               }
            }
         }

         int var24 = var1.size();
         this.imageCreater = new Hashtable(var24);

         while (var24 > 0) {
            String var13 = (String)var1.remove(new Integer(var5));
            if (var13 == null) {
               System.out.println("can't find image");
               var5++;
            } else {
               int var14 = var13.indexOf(45, 0);
               String var15 = var13.substring(0, var14);
               var2 = Integer.parseInt(var13.substring(var14 + 1));
               byte[] var16 = new byte[var2];
               var10.read(var16);
               var11 = Image.createImage(var16, 0, var2);
               this.imageCreater.put(var15, var11);
               var5 += var2;
               var24--;
            }
         }

         var10.close();
         var1.clear();
         Object var18 = null;
      } catch (Exception var17) {
         System.out.println("createImageFromFile " + var17.toString());
      }

      System.gc();
   }

   private Image createImage(String var1) throws Exception {
      if (var1.startsWith("/")) {
         var1 = var1.substring(1);
      }

      Image var2 = (Image)this.imageCreater.get(var1);
      if (var2 == null) {
         System.out.println("can't find image " + var1);
      }

      System.gc();
      return var2;
   }

   private boolean loadGameState() {
      boolean var1 = true;
      boolean var2 = false;
      RecordStore var3 = null;
      loadGameUI.percent = 0;
      String var4 = this.getLastGoodRSName();

      try {
         if (var4 == null) {
            throw new Exception("No valid record store!");
         }

         System.out.println("Last good name is " + var4);
         var3 = RecordStore.openRecordStore(var4, false);
         int var5 = var3.getNumRecords();
         byte[] var6 = var3.getRecord(1);
         if (var6 != null) {
            System.out.println("Got character record from RecordStore, length is " + var6.length);
         }

         this.character = Player.fromBytes(var6, true);
         byte[] var20 = null;
         this.character.game = this;
         loadGameUI.percent = 20;
         this.gameCanvas.repaint();
         this.gameCanvas.serviceRepaints();
         int var7 = this.readMasterListRecords(var3, 2);
         System.out.println("Read the master lists from RecordStore");
         var20 = var3.getRecord(var7);
         readOtherStateInfo(var20);
         var20 = null;
         if (this.mainMenuUI != null) {
            System.gc();
            this.mainMenuUI = null;
            this.newGameUI = null;
            this.characterMainUI = null;
            this.charNameTextForm = null;
            this.splashUI = null;
            Shop.loadDialogue();
            System.gc();
         }
      } catch (Exception var17) {
         System.out.println("Exception in loadGameState");
         System.out.println(var17);
         var1 = false;
      } finally {
         if (var3 != null) {
            try {
               var3.closeRecordStore();
            } catch (Exception var16) {
            }

            Object var19 = null;
         }
      }

      return var1;
   }

   private boolean saveGameState() {
      boolean var1 = true;
      boolean var2 = false;
      RecordStore var3 = null;
      saveGameUI.percent = 0;
      String var4 = this.getRSNameNotInUse();

      try {
         var3 = RecordStore.openRecordStore(var4, true);
         byte[] var5 = this.character.toBytes(true);
         saveGameUI.percent = 20;
         this.gameCanvas.repaint();
         this.gameCanvas.serviceRepaints();
         var3.addRecord(var5, 0, var5.length);
         System.gc();
         this.writeMasterListsToRecordStore(var3);
         var5 = writeOtherStateInfoToBytes();
         var3.addRecord(var5, 0, var5.length);
         Object var21 = null;
         var3.closeRecordStore();
         var3 = null;
         this.cleanupRecordStores();
         saveGameUI.percent = 100;
         this.gameCanvas.repaint();
         this.gameCanvas.serviceRepaints();
      } catch (Throwable var18) {
         System.out.println("Exception in saveGameState");
         System.out.println(var18);

         try {
            RecordStore.deleteRecordStore(var4);
         } catch (Exception var17) {
         }

         var1 = false;
      } finally {
         if (var3 != null) {
            try {
               var3.closeRecordStore();
            } catch (Exception var16) {
            }
         }
      }

      return var1;
   }

   private void createErrorForm() {
      this.errorForm = new Form("Error");
      this.errItem = new StringItem("Error", "Cannot load game");
      this.errorForm.append(this.errItem);
      this.errorForm.addCommand(okCommand);
      this.errorForm.setCommandListener(this);
   }

   static int getGameAdvancementLevel(int var0) {
      if (var0 < 17) {
         return 0;
      } else if (var0 < 29) {
         return 1;
      } else if (var0 < 38) {
         return 2;
      } else if (var0 < 49) {
         return 3;
      } else {
         return var0 < 62 ? 4 : 5;
      }
   }

   static void openAndRepopulateDungeons(int var0) {
      int var1 = 0;

      for (int var2 = 0; var2 < 37; var2++) {
         int var3 = var2;
         dungeons[var3].populated = true;
         dungeons[var3].refreshTileFlags();
         var1++;
         loadGameUI.percent = 100 * var1 / 37;
         if (loadGameUI.percent > 100) {
            loadGameUI.percent = 100;
         }
      }
   }

   private static void allocateMasterLists() {
      monsters = new Hashtable[37];

      for (int var0 = 0; var0 < 37; var0++) {
         monsters[var0] = new Hashtable();
      }

      chests = new Hashtable[37];

      for (int var1 = 0; var1 < 37; var1++) {
         chests[var1] = new Hashtable();
      }

      droppedItems = new Vector[37];

      for (int var2 = 0; var2 < 37; var2++) {
         droppedItems[var2] = new Vector();
      }
   }

   private static void readOtherStateInfo(byte[] var0) throws Exception {
      ByteArrayInputStream var1 = new ByteArrayInputStream(var0, 0, var0.length);
      DataInputStream var2 = new DataInputStream(var1);
      Item.nextSpawnId = var2.readShort();
      Monster.nextSpawnIdCounter = var2.readShort();

      for (int var3 = 0; var3 < 9; var3++) {
         Shop.firstVisit[var3] = var2.readBoolean();
      }

      for (int var4 = 0; var4 < 4; var4++) {
         Shop.interactionCount[var4] = var2.readShort();
      }

      for (int var5 = 0; var5 < 4; var5++) {
         Shop.rewardsGiven[var5] = var2.readShort();
      }

      for (int var6 = 0; var6 < 4; var6++) {
         Shop.questState1[var6] = var2.readByte();
      }

      for (int var7 = 0; var7 < 4; var7++) {
         Shop.questState2[var7] = var2.readByte();
      }

      Shop.showDeathGreeting = var2.readBoolean();

      try {
         var2.close();
      } catch (Exception var9) {
      }

      Object var11 = null;
      Object var10 = null;
   }

   private static byte[] writeOtherStateInfoToBytes() throws Exception {
      ByteArrayOutputStream var0 = new ByteArrayOutputStream(60);
      DataOutputStream var1 = new DataOutputStream(var0);
      byte[] var2 = null;

      try {
         var1.writeShort(Item.nextSpawnId);
         var1.writeShort(Monster.nextSpawnIdCounter);

         for (int var3 = 0; var3 < 9; var3++) {
            var1.writeBoolean(Shop.firstVisit[var3]);
         }

         for (int var4 = 0; var4 < 4; var4++) {
            var1.writeShort(Shop.interactionCount[var4]);
         }

         for (int var5 = 0; var5 < 4; var5++) {
            var1.writeShort(Shop.rewardsGiven[var5]);
         }

         for (int var6 = 0; var6 < 4; var6++) {
            var1.writeByte(Shop.questState1[var6]);
         }

         for (int var7 = 0; var7 < 4; var7++) {
            var1.writeByte(Shop.questState2[var7]);
         }

         var1.writeBoolean(Shop.showDeathGreeting);
         var1.flush();
         var2 = var0.toByteArray();
      } catch (Exception var16) {
         throw var16;
      } finally {
         try {
            var1.close();
         } catch (Exception var15) {
         }

         Object var19 = null;
         Object var18 = null;
      }

      return var2;
   }

   private static int maxWriteSize() {
      int var0 = 0;
      int var1 = 0;
      int var2 = 0;

      for (int var3 = 1; var3 < 37; var3++) {
         var2 = monsters[var3].size();
         var1 = 4 + var2 * 28;
         if (var1 > var0) {
            var0 = var1;
         }
      }

      for (int var4 = 1; var4 < 37; var4++) {
         var2 = chests[var4].size();
         var1 = 4 + var2 * 8;
         if (var1 > var0) {
            var0 = var1;
         }
      }

      for (int var5 = 0; var5 < 37; var5++) {
         var2 = droppedItems[var5].size();
         var1 = 4 + var2 * 7;
         if (var1 > var0) {
            var0 = var1;
         }
      }

      return var0 + 50;
   }

   private void writeMasterListsToRecordStore(RecordStore var1) throws Exception {
      int var2 = maxWriteSize();
      ByteArrayOutputStream var3 = new ByteArrayOutputStream(var2);
      DataOutputStream var4 = new DataOutputStream(var3);
      byte[] var5 = null;

      try {
         for (int var6 = 1; var6 < 37; var6++) {
            int var7 = monsters[var6].size();
            var4.writeInt(var7);
            Enumeration var8 = monsters[var6].elements();
            Monster var9 = new Monster();
            byte[] var10 = null;

            while (var8.hasMoreElements()) {
               var10 = (byte[])var8.nextElement();
               Monster.fromBytes(var9, var10);
               var9.writeTo(var4);
            }

            var4.flush();
            var5 = var3.toByteArray();
            var1.addRecord(var5, 0, var5.length);
            var3.reset();
            var5 = null;
            saveGameUI.percent = 20 + 30 * (var6 + 1) / 37;
            this.gameCanvas.repaint();
            this.gameCanvas.serviceRepaints();
         }

         for (int var30 = 1; var30 < 37; var30++) {
            int var31 = chests[var30].size();
            var4.writeInt(var31);
            Enumeration var33 = chests[var30].elements();

            while (var33.hasMoreElements()) {
               byte[] var36 = (byte[])var33.nextElement();
               writeBytesToDataOututStream(var4, var36, 8);
            }

            var4.flush();
            var5 = var3.toByteArray();
            var1.addRecord(var5, 0, var5.length);
            var3.reset();
            var5 = null;
            saveGameUI.percent = 50 + 30 * (var30 + 1) / 37;
            this.gameCanvas.repaint();
            this.gameCanvas.serviceRepaints();
         }

         for (int var32 = 0; var32 < 37; var32++) {
            int var34 = droppedItems[var32].size();
            var4.writeInt(var34);
            Enumeration var37 = droppedItems[var32].elements();

            while (var37.hasMoreElements()) {
               byte[] var11 = (byte[])var37.nextElement();
               writeBytesToDataOututStream(var4, var11, 7);
            }

            var4.flush();
            var5 = var3.toByteArray();
            var1.addRecord(var5, 0, var5.length);
            var3.reset();
            var5 = null;
            saveGameUI.percent = 80 + 19 * (var32 + 1) / 37;
            this.gameCanvas.repaint();
            this.gameCanvas.serviceRepaints();
         }
      } catch (Exception var20) {
         throw var20;
      } finally {
         try {
            var4.close();
         } catch (Exception var19) {
         }

         Object var23 = null;
         Object var22 = null;
      }
   }

   private static int maxRecordSize(RecordStore var0, int var1) throws Exception {
      boolean var2 = false;
      boolean var4 = false;
      return 1500;
   }

   private int readMasterListRecords(RecordStore var1, int var2) throws Exception {
      byte[] var3 = null;
      InputStream var4 = null;
      DataInputStream var5 = null;
      int var6 = maxRecordSize(var1, var2);
      var3 = new byte[var6];
      var4 = new ByteArrayInputStream(var3);
      var5 = new DataInputStream(var4);
      int var7 = var2;
      Monster var8 = null;
      boolean var9 = false;

      for (int var10 = 1; var10 < 37; var10++) {
         var1.getRecord(var7++, var3, 0);
         monsters[var10].clear();
         int var11 = var5.readInt();

         for (int var12 = 0; var12 < var11; var12++) {
            var8 = Monster.readFrom(var5);
            var8.store();
         }

         loadGameUI.percent = 20 + 30 * (var10 + 1) / 37;
         this.gameCanvas.repaint();
         this.gameCanvas.serviceRepaints();
         var5.reset();
      }

      for (int var24 = 1; var24 < 37; var24++) {
         var1.getRecord(var7++, var3, 0);
         chests[var24].clear();
         int var25 = var5.readInt();

         for (int var13 = 0; var13 < var25; var13++) {
            byte[] var14 = readBytesFromDataInputStream(var5, 8);
            String var15 = Util.posKey((int)var14[0], (int)var14[1]);
            chests[var24].put(var15, var14);
         }

         loadGameUI.percent = 50 + 30 * (var24 + 1) / 37;
         this.gameCanvas.repaint();
         this.gameCanvas.serviceRepaints();
         var5.reset();
      }

      for (int var26 = 0; var26 < 37; var26++) {
         var1.getRecord(var7++, var3, 0);
         droppedItems[var26].removeAllElements();
         int var27 = var5.readInt();

         for (int var28 = 0; var28 < var27; var28++) {
            byte[] var29 = readBytesFromDataInputStream(var5, 7);
            droppedItems[var26].addElement(var29);
         }

         loadGameUI.percent = 80 + 19 * (var26 + 1) / 37;
         this.gameCanvas.repaint();
         this.gameCanvas.serviceRepaints();
         var5.reset();
      }

      var3 = null;
      if (var5 != null) {
         try {
            var5.close();
         } catch (Exception var16) {
         }

         var5 = null;
      }

      var4 = null;
      return var7;
   }

   private static byte[] readBytesFromDataInputStream(DataInputStream var0, int var1) throws Exception {
      byte[] var2 = new byte[var1];

      for (int var3 = 0; var3 < var1; var3++) {
         var2[var3] = var0.readByte();
      }

      return var2;
   }

   private static void writeBytesToDataOututStream(DataOutputStream var0, byte[] var1, int var2) throws Exception {
      for (int var3 = 0; var3 < var2; var3++) {
         var0.writeByte(var1[var3]);
      }
   }

   public static void printMemory(String var0) {
      if (var0 == null) {
         var0 = "";
      }

      System.out.println(">>> MEMORY: " + var0 + ": Free memory is " + Runtime.getRuntime().freeMemory());
   }

   static String memoryString(String var0) {
      if (var0 == null) {
         var0 = "";
      }

      Runtime var1 = Runtime.getRuntime();
      long var2 = var1.freeMemory();
      long var4 = var1.totalMemory();
      return ">>> MEMORY: " + var0 + ": Free memory is " + var2 + ", Total memory is " + var4;
   }

   private static void sleep(long var0) {
      try {
         Thread.sleep(var0);
      } catch (Exception var3) {
      }
   }

   void startImageLoadThread(int var1) {
      try {
         helperThreadState = 3;
         this.imgloadRunning = true;
         if (this.imgloadThread.isAlive()) {
            this.killThread = true;

            try {
               this.imgloadThread.join();
            } catch (Exception var3) {
            }
         }

         this.imgloadThread = null;
         System.gc();
         this.imgloadThread = new Thread(this);
         this.imgloadThread.setPriority(2);
         this.loadingDungeonID = (byte)var1;
         this.imgloadThread.start();
      } catch (Exception var4) {
         System.out.println("Barfed in startImageLoadThread");
      }
   }

   private void loadCampMonsters() {
      byte[] var1 = new byte[5];
      System.out.println("LOADING WARDEN IMAGES");
      var1[0] = 1;
      var1[1] = 1;
      var1[2] = 1;
      var1[3] = 1;
      var1[4] = 1;
      this.loadingDungeonID = 1;
      this.runImageLoader(var1);
   }

   void runImageLoader() {
      this.imgsLoaded = false;
      if (reloadGame) {
         loadGameUI.percent = 80;
      } else {
         this.loadDungeonUI.percent = 0;
      }

      byte[] var1 = new byte[5];

      for (int var2 = 0; var2 < 5; var2++) {
         var1[var2] = 0;
      }

      if (this.loadingDungeonID == 1) {
         this.unloadAllMonsterImages();
         this.loadCampMonsters();
      } else {
         Hashtable var3 = monsters[this.loadingDungeonID - 1];
         if (var3 == null) {
            return;
         }

         Enumeration var4 = var3.elements();
         Monster var5 = new Monster();

         while (var4.hasMoreElements()) {
            byte[] var6 = (byte[])var4.nextElement();
            Monster.fromBytes(var5, var6);
            if (var5.monsterType >= 1 && var5.monsterType <= 5) {
               var1[0]++;
            } else if (var5.monsterType >= 6 && var5.monsterType <= 10) {
               var1[1]++;
            } else if (var5.monsterType >= 11 && var5.monsterType <= 25) {
               var1[2]++;
            } else if (var5.monsterType >= 26 && var5.monsterType <= 40) {
               var1[3]++;
            } else {
               var1[4]++;
            }
         }

         this.unloadAllMonsterImages();
         this.runImageLoader(var1);
      }

      GameCanvas.showLevelNameMessage = true;
   }

   private boolean isExcludedImage(int var1) {
      return false;
   }

   void runImageLoader(byte[] var1) {
      this.imgsLoaded = false;

      try {
         for (int var2 = 0; var2 < 5; var2++) {
            if (var1[var2] > 0) {
               int var3 = monster_image_index_info[var2][0];
               int var4 = monster_image_index_info[var2][1];

               for (int var5 = 0; var5 < var4; var5++) {
                  int var6 = var3 + var5;
                  if (!this.isExcludedImage(var6)) {
                     GameCanvas.objectSprites[var6] = this.createImage(monster_filenames[var2][var5]);
                  }

                  if (!this.imgloadRunning) {
                     return;
                  }

                  if (this.killThread) {
                     this.killThread = false;
                     return;
                  }
               }
            }

            if (reloadGame) {
               loadGameUI.percent = 80 + (var2 + 1) * 20 / 5;
               this.gameCanvas.repaint();
               this.gameCanvas.serviceRepaints();
            } else {
               this.loadDungeonUI.percent = (var2 + 1) * 100 / 5;
               this.gameCanvas.repaint();
               this.gameCanvas.serviceRepaints();
            }
         }

         this.imgsLoaded = true;
         monster_filenames = null;
         this.imgloadRunning = false;
      } catch (Throwable var7) {
         System.out.println("ERROR in image loader: " + var7);
         this.display.setCurrent(this.errorForm);
      }
   }

   void unloadAllMonsterImages() {
      int var1 = GameCanvas.objectSprites.length;

      for (int var2 = 0; var2 < var1; var2++) {
         if (GameCanvas.objectSprites[var2] != null) {
            GameCanvas.objectSprites[var2] = null;
         }
      }

      System.gc();
   }

   static void removeMonster(int var0, int var1, int var2) {
      byte[] var3 = (byte[])monsters[var0 - 1].remove(Util.posKey(var1, var2));
      Dungeon var4 = dungeons[var0 - 1];
      if (var3 != null) {
         var4.tiles[var1][var2] = Util.clearBit((byte)2, var4.tiles[var1][var2]);
      }
   }

   private Screen newInventoryItemUI(int var1) {
      Screen var2 = new Screen(this, 5, 34);
      String var3 = this.character.itemTooltip(var1);
      Vector var4 = new Vector();
      var4.addElement("Drop");
      if (this.character.canEquipOrUnequip(var1)) {
         if (!this.character.isEquipped(var1)) {
            var4.addElement("Equip");
         } else {
            var4.addElement("Unequip");
         }
      }

      if (this.character.canLearnSpell(var1)) {
         var4.addElement("Learn");
      }

      if (this.character.canUseItem(var1)) {
         var4.addElement("Use");
      }

      String[] var5 = new String[var4.size()];

      for (int var6 = 0; var6 < var4.size(); var6++) {
         var5[var6] = (String)var4.elementAt(var6);
      }

      var2.setupPromptList("Item", var3, var5);
      var2.backTarget = this.InventoryUI;
      return var2;
   }

   private void newClueLogUI(int var1) {
      System.gc();
      StringBuffer var2 = new StringBuffer();
      String var3;
      if (var1 == 4) {
         for (int var4 = 0; var4 < 6; var4++) {
            if (this.character.eventFlags[90 + var4]) {
               var2.append(Shop.dialogue[9][5 + Shop.RUMOR_STRING_OFFSET[this.character.traitorIndex][var4]]);
               var2.append("\n");
            }
         }

         var3 = "Rumors";
      } else {
         int var9 = 18 * var1;
         boolean var5 = var1 == this.character.traitorIndex;
         byte var6 = 0;

         for (int var7 = 0; var7 < 6; var7++) {
            var6 = 0;

            for (int var8 = 0; var8 < 3; var8++) {
               if (this.character.eventFlags[var9 + var7 * 3 + var8]) {
                  if (var8 >= var1) {
                     var6 = 1;
                  }

                  if (var5 && this.character.eventFlags[72 + var7 * 3 + var8]) {
                     var2.append(Shop.dialogue[9][5 + Shop.UNCONFIRMED_B[var7 * 4 + var8 + var6]]);
                  } else {
                     var2.append(Shop.dialogue[9][5 + Shop.UNCONFIRMED_A[var7 * 4 + var8 + var6]]);
                  }

                  var2.append("\n");
               }
            }
         }

         var3 = Shop.NAMES[5 + var1];
      }

      if (var2.length() == 0) {
         var2.append("You have no information yet.");
      }

      this.GenericInfoUI.setSecondaryParam(61);
      this.GenericInfoUI.setupMessage(var3, var2.toString());
   }

   private Screen newRevealUI() {
      System.gc();
      Screen var1 = new Screen(this, 5, 65);
      String[] var2 = new String[]{"Yes", "No"};
      var1.setupPromptList("Reveal Traitor", Shop.dialogue[9][67], var2);
      var1.removeCommand(cancelCommand);
      var1.backTarget = this.OptionsUI;
      return var1;
   }

   private Screen newRevealWhomUI() {
      System.gc();
      Screen var1 = new Screen(this, 5, 66);
      String[] var2 = new String[]{"Alhavara", "Beatrice", "Chung", "Delacroix"};
      var1.setupPromptList("Reveal Traitor", "Who is the Traitor?", var2);
      var1.backTarget = this.OptionsUI;
      return var1;
   }

   private Screen newSkillsListUI() {
      System.gc();
      Screen var1 = new Screen(this, 5, 35);
      String[] var2 = this.character.knownSkillsSummary();
      var1.setupPromptList("Skills", "Your Skills:", var2);
      var1.backTarget = this.OptionsUI;
      return var1;
   }

   private Screen newSpellsListUI() {
      System.gc();
      Screen var1 = new Screen(this, 5, 37);
      Vector var2 = this.character.knownSpellsSummary();
      int var3 = var2.size();
      String[] var4 = new String[var3];

      for (int var5 = 0; var5 < var3; var5++) {
         var4[var5] = (String)var2.elementAt(var5);
      }

      var1.setupPromptList("Spells", "Your Spells:", var4);
      var1.backTarget = this.OptionsUI;
      return var1;
   }

   private Screen newSpellInfoUI(int var1) {
      System.gc();
      Screen var2 = new Screen(this, 5, 38);
      int var3 = this.character.nthKnownSpellId(var1);
      String var4 = this.character.spellTooltip(var3);
      String[] var5 = new String[]{"Ready Spell"};
      var2.setupPromptList("Spell Info", var4, var5);
      var2.backTarget = this.SpellsListUI;
      return var2;
   }

   Screen newLevelUpUI(int var1) {
      System.gc();
      Screen var2 = new Screen(this, 5, 39);
      String[] var3 = this.character.availableAttributeIncreases();
      String var4 = null;
      if (var1 == 1) {
         var2.contextIndex = 0;
         var4 = "Select an attribute to increase 3 points:";
      } else if (var1 == 2) {
         var4 = "Select an attribute to increase 2 points:";
         var2.contextIndex = 1;
      } else if (var1 == 3) {
         var2.contextIndex = 2;
         var4 = "Select an attribute to increase 1 point:";
      }

      var2.setupPromptList("Level Up", var4, var3);
      var2.removeCommand(cancelCommand);
      var2.backTarget = var2;
      return var2;
   }

   Screen newEndOfGameUI() {
      System.gc();
      String var1 = Shop.dialogue[9][74] + "\n" + Shop.dialogue[9][75] + "\n" + Shop.dialogue[9][76];
      Screen var2 = new Screen(this, 4, 200);
      var2.setupMessage("Victory!", var1);
      return var2;
   }

   public Screen newGameOverUI() {
      System.gc();
      String var1 = Shop.dialogue[9][73];
      Screen var2 = new Screen(this, 4, 201);
      var1 = Util.replace(var1, "<TAG>", Shop.NAMES[5 + this.character.traitorIndex]);
      var2.setupMessage("Game Over", var1);
      return var2;
   }

   static DataInputStream getDataInputStream(String var0) throws Exception {
      InputStream var1 = new Object().getClass().getResourceAsStream(Util.ensureLeadingSlash(var0));
      if (var1 == null) {
         return null;
      }

      byte[] var2 = Util.readAll(var1.available(), var1);
      return new DataInputStream(new ByteArrayInputStream(var2));
   }

   static int nextInt() {
      return r.nextInt();
   }

   static int nextInt(int var0) {
      return Math.abs(r.nextInt() % var0);
   }

   static int lingoRandomInt(int var0) {
      return 1 + Math.abs(r.nextInt() % var0);
   }

   static int lingoRandomInt(Random var0, int var1) {
      return 1 + Math.abs(var0.nextInt() % var1);
   }

   void setCurrentDisplay(Object var1) {
      try {
         if (uic != null) {
            if (var1 instanceof Screen) {
               Screen var2 = (Screen)var1;
               if (uic != var2) {
                  uic.onExit();
               }
            } else {
               uic.onExit();
            }
         }
      } catch (Throwable var4) {
         System.out.println("ERROR in hideNotify " + var4);
      }

      if (this.gameCanvas != null) {
         this.gameCanvas.pauseThread();
      }

      if (this.display == null) {
         this.display = Display.getDisplay(this);
      }

      if (var1 instanceof Screen) {
         uic = (Screen)var1;
         this.gameCanvas.activeScreen = uic;
         this.display.setCurrent(this.gameCanvas);
         uic.onEnter();
         if (this.gameCanvas.isRunning()) {
            this.gameCanvas.repaintPending = true;
         } else {
            this.gameCanvas.repaint();
            this.gameCanvas.serviceRepaints();
         }
      } else if (var1 instanceof GameCanvas) {
         try {
            uic = null;
            this.gameCanvas.activeScreen = null;
            if (this.display.getCurrent() == this.gameCanvas) {
               this.gameCanvas.showNotify();
            } else {
               this.display.setCurrent(this.gameCanvas);
            }
         } catch (Throwable var3) {
            System.out.println("ERROR in show " + var3);
         }
      } else if (var1 instanceof Displayable) {
         Displayable var5 = (Displayable)var1;
         uic = null;
         this.display.setCurrent(var5);
      }
   }

   private static void loadMonsterFilenames() throws Exception {
      DataInputStream var0 = getResource("monsterfilenamesin.dat");
      monster_filenames = new String[5][7];

      for (int var1 = 0; var1 < 5; var1++) {
         for (int var2 = 0; var2 < 7; var2++) {
            monster_filenames[var1][var2] = var0.readUTF();
         }
      }

      var0.close();
   }

   synchronized void setDownloadStatus(String var1) {
      downloadStatus = var1;
   }

   synchronized String getDownloadStatus() {
      return downloadStatus;
   }

   private String getCreditsString() {
      StringBuffer var1 = new StringBuffer(400);
      var1.append("Game Design: Anthony Gill and Greg Gorden");
      var1.append('\n');
      var1.append("Art: Mark Jones");
      var1.append('\n');
      var1.append("Programming: Marc Ilgen, Roland Kemp");
      var1.append('\n');
      var1.append("Technical Director: Andrew Friedman");
      var1.append('\n');
      var1.append("(C) 2003 Vir2L Studos, a ZeniMax Media company. The Elder Scrolls and Vir2L are ");
      var1.append("registered trademarks of ZeniMax Media Inc. All rights reserved.");
      var1.append('\n');
      return var1.toString();
   }

   public void setAidPointsForNPC(int var1) {
      if (var1 != 4) {
         String var2 = this.NPCChoicesUI[var1].rawTaggedText;
         String var3 = this.NPCChoicesUI[var1].firstLine();
         int var4 = 0;
         if (Shop.isNamedShop(var1)) {
            var4 = Shop.rewardsGiven[var1 - 5];
         } else if (Shop.isGenericPeddler(var1)) {
            var4 = this.character.gold;
         }

         var3 = Util.replace(var2, "<TAG>", var4);
         this.NPCChoicesUI[var1].setItems(var3);
      }
   }

   private String getRSNameNotInUse() {
      String[] var1 = RecordStore.listRecordStores();
      int var2 = 0;
      if (var1 == null) {
         var2 = 0;
      } else {
         var2 = var1.length;
      }

      int var3 = Util.randomInt(10000);
      String var4 = "es_gamestate" + var3;

      while (true) {
         boolean var5 = false;

         for (int var6 = 0; var6 < var2; var6++) {
            if (var4.equals(var1[var6])) {
               var5 = true;
            }
         }

         if (!var5) {
            return var4;
         }

         var3 = Util.randomInt(10000);
         var4 = "es_gamestate" + var3;
      }
   }

   private String getLastGoodRSName() {
      String[] var1 = RecordStore.listRecordStores();
      int var2 = 0;
      if (var1 == null) {
         var2 = 0;
      } else {
         var2 = var1.length;
      }

      String var3 = null;
      if (var2 == 0) {
         return null;
      }

      long var4 = 0L;
      RecordStore var6 = null;

      for (int var7 = 0; var7 < var2; var7++) {
         if (var1[var7].startsWith("es_gamestate")) {
            try {
               var6 = RecordStore.openRecordStore(var1[var7], false);
               int var8 = var6.getNumRecords();
               long var9 = var6.getLastModified();
               if (var9 > var4) {
                  var3 = var1[var7];
                  var4 = var9;
               }
            } catch (Throwable var20) {
            } finally {
               try {
                  if (var6 != null) {
                     var6.closeRecordStore();
                  }

                  Object var23 = null;
               } catch (Exception var19) {
               }
            }
         }
      }

      return var3;
   }

   private void cleanupRecordStores() {
      String var1 = this.getLastGoodRSName();
      String[] var2 = RecordStore.listRecordStores();
      int var3 = 0;
      if (var2 == null) {
         var3 = 0;
      } else {
         var3 = var2.length;
      }

      if (var3 != 0) {
         for (int var4 = 0; var4 < var3; var4++) {
            if (var2[var4].startsWith("es_gamestate") && (var1 == null || !var1.equals(var2[var4]))) {
               try {
                  RecordStore.deleteRecordStore(var2[var4]);
               } catch (Exception var6) {
               }
            }
         }
      }
   }

   private void checkDestroyed() {
      if (theMIDletState == 4) {
         this.doCompleteExit();
      }
   }

   void doCompleteExit() {
      this.display = null;
      this.destroyApp(true);
      this.notifyDestroyed();
   }

   private static boolean checkFirstTime() {
      boolean var0 = true;
      RecordStore var1 = null;

      try {
         var1 = RecordStore.openRecordStore("elder_firsttime", true);
         int var2 = var1.getNumRecords();
         if (var2 <= 0) {
            var0 = true;
            byte[] var3 = new byte[]{1, 2};
            var1.addRecord(var3, 0, var3.length);
         } else {
            var0 = false;
         }
      } catch (Exception var13) {
         var0 = true;
      } finally {
         if (var1 != null) {
            try {
               var1.closeRecordStore();
            } catch (Exception var12) {
            }
         }
      }

      return var0;
   }

   void displayDebug() {
      this.errorForm = new Form("Error");
      StringItem var1 = new StringItem("Error", "code = " + debugCode);
      this.errorForm.append(var1);
      this.errorForm.addCommand(okCommand);
      this.errorForm.setCommandListener(this);
      Display.getDisplay(this).setCurrent(this.errorForm);
   }

   static {
      try {
         r = new Random(System.currentTimeMillis());
         loadMonsterFilenames();
         allocateMasterLists();
      } catch (Exception var1) {
         System.out.println("ERROR: problem with loading camp or image record HT");
      }
   }
}
