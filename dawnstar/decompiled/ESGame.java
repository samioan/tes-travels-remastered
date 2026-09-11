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
   private g introUI;
   private g restartUI;
   private g mainMenuUI;
   private g newGameUI;
   private g characterMainUI;
   private g noSavedGameUI;
   private static h saveGameUI;
   private static h loadGameUI;
   private h splashUI;
   h loadDungeonUI;
   private h createGameUI;
   g[] NPCChoicesUI;
   g NPCGiveWhatUI;
   g WarpWhereUI;
   g GenericInfoUI;
   g NPCTrainWhatUI;
   g NPCTakeWhatUI;
   g NPCQuestionWhatUI;
   g NPCQuestionWhomUI;
   g NPCSellWhatUI;
   g NPCSellSureUI;
   g NPCBuyWhatUI;
   g NPCWarpUI;
   g OptionsUI;
   g InventoryUI;
   g InventoryItemUI;
   g SkillsListUI;
   g SpellsListUI;
   g SpellInfoUI;
   g ClueUI;
   g LevelUpUI;
   g endOfGameUI;
   g gameOverUI;
   g confirmQuitUI;
   g helpUI;
   g RevealUI;
   e gameCanvas;
   private Form charNameTextForm;
   private static String[] helpStrings = new String[12];
   private static String[] helpTitles = new String[12];
   private static String creditsString = null;
   public j character;
   static i[] dungeons;
   Thread imgloadThread;
   boolean imgloadRunning;
   boolean killThread;
   byte loadingDungeonID;
   boolean imgsLoaded;
   static Hashtable[] monsters;
   static Hashtable[] chests;
   static Vector[] droppedItems;
   static g uic = null;
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
         j.N = var1;
         System.out.println("User ID is " + j.N);
      } else {
         System.out.println("User ID is NULL!");
      }

      theMIDletState = 1;
   }

   public void startRegisteredApp() {
      if (this.display == null) {
         printMemory("Very start of startapp");
         this.display = Display.getDisplay(this);
         this.gameCanvas = new e(this);
         this.GenericInfoUI = new g(this, 4, 410);
         if (checkFirstTime()) {
            this.GenericInfoUI.a(410);
            this.GenericInfoUI
               .a(
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
         this.splashUI = new h(this, 2, 1);
         this.splashUI.c();
         this.splashUI.t = this.errorForm;
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
            this.GenericInfoUI.a(499);
            this.GenericInfoUI
               .a(
                  "Save Error",
                  "There was an error in saving your character record. Your previous character record is still saved. Try turning your phone off then on again to clear the memory."
               );
            this.setCurrentDisplay(this.GenericInfoUI);
         }
      } else if (helperThreadState == 6) {
         if (this.loadGameState()) {
            this.resumeGame();
            this.loadingDungeonID = this.character.ao;
            this.imgloadRunning = true;
            reloadGame = true;
            reloadGame = false;
            this.imgloadRunning = false;
            loadGameUI.G = 100;
            this.gameCanvas.repaint();
            this.gameCanvas.serviceRepaints();
            this.gameCanvas.aD = this.character;
            this.gameCanvas.r = true;
            this.gameCanvas.i();
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
         this.splashUI.G = 0;
         this.createImageFromFile();
         this.splashUI.G = 5;
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
         this.splashUI.G = 100;
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
      j.s();
      loadHelpStrings();
      this.splashUI.G = 10;
      a.d();
      b.a();
      d.f();
      System.gc();
      this.splashUI.G = 15;
   }

   public static final DataInputStream getResource(String var0) {
      int var1 = -1;
      int var2 = -1;
      int var3 = 0;
      DataInputStream var8 = null;

      try {
         var8 = f.a("/datfiles.lmp");

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
      dungeons = new i[37];
      this.splashUI.G = 60;
      new c(dungeons, this.splashUI);
      Object var1 = null;
      System.gc();
   }

   private void createNewGame() {
      this.createGameUI.G = 5;
      System.gc();
      this.mainMenuUI = null;
      this.newGameUI = null;
      this.characterMainUI = null;
      this.charNameTextForm = null;
      this.splashUI = null;
      this.createGameUI.G = 10;
      k.c();
      System.gc();
      this.createGameUI.G = 100;
      this.setCurrentDisplay(this.GenericInfoUI);
   }

   private void resumeGame() {
      int var1 = getGameAdvancementLevel(this.character.av);
      openAndRepopulateDungeons(var1);
   }

   private void allocateAllUIs() throws Exception {
      System.gc();
      this.splashUI.G = 20;
      this.checkDestroyed();
      e.h = this.createImage("floor3.png");
      e.aE = this.createImage("floorIce.png");
      e.W = this.createImage("wallsr.png");
      e.ax = this.createImage("wallsi.png");
      e.w = this.createImage("gate.png");
      e.o = new Image[26];

      for (int var1 = 0; var1 < 26; var1++) {
         e.o[var1] = null;
      }

      this.loadingDungeonID = 1;
      this.imgloadRunning = true;
      this.loadDungeonUI = new h(this, 11, 304);
      this.loadDungeonUI.j();
      this.loadCampMonsters();
      this.checkDestroyed();
      e.au = new Image[3];
      e.au[0] = this.createImage("baglarge.png");
      e.au[1] = this.createImage("bagmid.png");
      e.au[2] = this.createImage("bagsmall.png");
      System.gc();
      this.checkDestroyed();
      e.aa = new Image[3];
      e.aa[0] = this.createImage("chestnearclosed.png");
      e.aa[1] = this.createImage("chestmidclosed.png");
      e.aa[2] = this.createImage("chestfarclosed.png");
      this.checkDestroyed();
      e.al = this.createImage("icons.png");
      e.R = this.createImage("panel.png");
      System.gc();
      this.checkDestroyed();
      creditsString = this.getCreditsString();
      this.mainMenuUI = new g(this, 3, 2);
      String[] var2 = new String[]{"New Game", "Continue Game", "Help", "Credits", "Exit"};
      this.mainMenuUI.a("Main Menu", var2, false);
      this.NPCQuestionWhatUI = new g(this, 5, 27);
      String[] var3 = new String[]{"North wall defense", "East wall defense", "Arguing with governor", "Imperial aid", "Ice tribes", "Gates before attack"};
      this.NPCQuestionWhatUI.a("", "Ask about what?", var3);
      this.newGameUI = new g(this, 5, 3);
      String[] var4 = j.i;
      this.newGameUI.a("New Game", "Select a Class:", var4);
      this.splashUI.G = 35;
      this.characterMainUI = new g(this, 6, 4);
      String[] var5 = new String[]{"See Class Info", "Create Character"};
      this.characterMainUI.a("Character", "You selected:", "", var5);
      this.NPCChoicesUI = new g[9];
      String[] var6 = new String[]{"Buy", "Sell"};

      for (int var7 = 0; var7 < 4; var7++) {
         this.NPCChoicesUI[var7] = new g(this, 5, 9 + var7);
         this.NPCChoicesUI[var7].a(k.r[var7], "Your gold: <TAG>", var6);
         this.NPCChoicesUI[var7].v = this.gameCanvas;
      }

      this.NPCChoicesUI[4] = new g(this, 5, 13);
      String[] var8 = new String[]{"Rumors", "Cure", "Warp", "Recovery"};
      this.NPCChoicesUI[4].a("Eustacia", "Welcome", var8);
      this.NPCChoicesUI[4].v = this.gameCanvas;
      String[] var9 = new String[]{"Train", "Give", "Befriend", "Threaten", "Ask a question", "Warp"};

      for (int var10 = 5; var10 < 9; var10++) {
         this.NPCChoicesUI[var10] = new g(this, 5, 9 + var10);
         this.NPCChoicesUI[var10].a(k.r[var10], "Aid: <TAG>", var9);
         this.NPCChoicesUI[var10].v = this.gameCanvas;
      }

      this.OptionsUI = new g(this, 3, 31);
      String[] var11 = new String[]{"Stats", "Inventory", "Clue Log", "Skills", "Spells", "Save Game", "Load Game", "Help", "Reveal Traitor", "Quit Game"};
      this.OptionsUI.a("Options", var11, false);
      this.OptionsUI.a(backCommand);
      this.ClueUI = new g(this, 5, 60);
      String[] var12 = new String[]{"Alhavara", "Beatrice", "Chung", "Delacroix", "Rumors"};
      this.ClueUI.a("Clue Log", "", var12);
      this.ClueUI.v = this.OptionsUI;
      this.helpUI = new g(this, 3, 203);
      this.helpUI.a("Help", helpTitles, true);
      this.splashUI.G = 42;
      this.charNameTextForm = new Form("Enter name");
      StringItem var13 = new StringItem(null, "Enter a name for your character");
      this.charNameTextForm.append(var13);
      TextField var14 = new TextField(null, null, 10, 0);
      this.charNameTextForm.append(var14);
      this.charNameTextForm.addCommand(okCommand);
      this.charNameTextForm.setCommandListener(this);
      this.noSavedGameUI = new g(this, 4, 305);
      this.noSavedGameUI.a("Unavailable", "No game is available for loading. Press OK to return to main menu.");
      this.noSavedGameUI.v = this.mainMenuUI;
      this.splashUI.t = this.mainMenuUI;
      this.newGameUI.v = this.mainMenuUI;
      this.characterMainUI.v = this.newGameUI;
      this.splashUI.G = 55;
      System.gc();
   }

   private g newGiveWhat(int var1) {
      System.gc();
      g var2 = new g(this, 5, 22);
      var2.i = var1;
      String[] var3 = new String[this.character.aq];

      for (int var4 = 0; var4 < this.character.aq; var4++) {
         int var5 = Math.abs(this.character.af[var4]);
         if (this.character.A(var4)) {
            var3[var4] = "E:" + a.c(var5);
         } else {
            var3[var4] = a.c(var5);
         }
      }

      var2.a(k.r[var1], "Give What?", var3);
      var2.v = this.NPCChoicesUI[var1];
      return var2;
   }

   private g newSellWhat(int var1) {
      System.gc();
      g var2 = new g(this, 5, 52);
      var2.i = var1;
      String[] var3 = new String[this.character.aq];

      for (int var4 = 0; var4 < this.character.aq; var4++) {
         int var5 = Math.abs(this.character.af[var4]);
         if (this.character.A(var4)) {
            var3[var4] = "E:" + a.c(var5) + " (" + a.a(5, var5) + ")";
         } else {
            var3[var4] = a.c(var5) + " (" + a.a(5, var5) + ")";
         }
      }

      var2.a(k.r[var1], "Sell What?", var3);
      var2.v = null;
      return var2;
   }

   private g newBuyWhat(int var1) {
      System.gc();
      g var2 = new g(this, 5, 50);
      var2.i = var1;
      int var3 = k.n[var1].length;
      String[] var4 = new String[var3];

      for (int var5 = 0; var5 < var3; var5++) {
         int var6 = Math.abs(k.n[var1][var5]);
         var4[var5] = a.c(var6) + " (" + a.a(4, var6) + ")";
      }

      var2.a(k.r[var1], "Buy What?", var4);
      return var2;
   }

   private g newTrainWhat(int var1) {
      System.gc();
      g var2 = new g(this, 5, 20);
      var2.i = var1;
      String[] var3 = new String[3];
      int var4 = 0;

      for (int var5 = 0; var5 < 14; var5++) {
         if (k.c(var1, var5)) {
            int var6 = this.character.b(var5, false);
            String var7 = j.ax[var5] + " (<TAG>)";
            var3[var4++] = f.a(var7, "<TAG>", var6);
         }
      }

      var2.a(k.r[var1], "Train What?", var3);
      var2.v = this.NPCChoicesUI[var1];
      return var2;
   }

   private g newWarpWhere() {
      System.gc();
      g var1 = new g(this, 5, 29);
      var1.i = 4;
      Vector var2 = new Vector();
      var2.addElement("Your last location");

      for (int var3 = 0; var3 < 4; var3++) {
         if (!k.p[5 + var3]) {
            var2.addElement(k.r[5 + var3]);
         }
      }

      int var4 = var2.size();
      String[] var5 = new String[var4];

      for (int var6 = 0; var6 < var4; var6++) {
         var5[var6] = (String)var2.elementAt(var6);
      }

      var1.a("Eustacia", "", var5);
      var1.v = this.NPCChoicesUI[4];
      return var1;
   }

   private g newInventoryUI() {
      System.gc();
      g var1 = new g(this, 5, 33);
      String[] var2 = new String[this.character.aq];

      for (int var3 = 0; var3 < this.character.aq; var3++) {
         byte var4 = this.character.af[var3];
         if (var4 < 0) {
            var2[var3] = "E: " + a.c(Math.abs(var4));
         } else {
            var2[var3] = a.c(var4);
         }
      }

      String var5 = f.a("Your gold: <TAG>", "<TAG>", this.character.o);
      var1.a("Inventory", var5, var2);
      var1.v = this.OptionsUI;
      return var1;
   }

   private g newConfirmQuitUI(g var1) {
      System.gc();
      g var2 = new g(this, 5, 202);
      String[] var3 = new String[]{"Yes", "No"};
      var2.a("Quit?", "Are you sure?", var3);
      var2.b(cancelCommand);
      var2.v = var1;
      return var2;
   }

   public void pauseApp() {
      this.gameCanvas.d();
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
         if (var1 == cancelCommand && uic.v != null) {
            this.setCurrentDisplay(uic.v);
            return;
         }

         if (uic.s == 410) {
            this.exit();
         } else if (uic.s == 2) {
            if (var1 == selectCommand) {
               int var3 = uic.a();
               switch (var3) {
                  case 0:
                     System.gc();
                     this.setCurrentDisplay(this.newGameUI);
                     break;
                  case 1:
                     System.gc();
                     this.gameCanvas.g();
                     loadGameUI = new h(this, 9, 302);
                     loadGameUI.j();
                     Thread var4 = new Thread(this);
                     helperThreadState = 6;
                     this.noSavedGameUI.v = this.mainMenuUI;
                     this.setCurrentDisplay(loadGameUI);
                     var4.start();
                     break;
                  case 2:
                     this.helpUI.v = this.mainMenuUI;
                     this.setCurrentDisplay(this.helpUI);
                     break;
                  case 3:
                     this.GenericInfoUI.a(204);
                     this.GenericInfoUI.a("Credits", creditsString);
                     this.setCurrentDisplay(this.GenericInfoUI);
                     break;
                  case 4:
                     this.confirmQuitUI = this.newConfirmQuitUI(uic);
                     this.setCurrentDisplay(this.confirmQuitUI);
               }
            }
         } else if (uic.s == 3) {
            if (var1 == selectCommand) {
               int var9 = uic.a();
               String var42 = uic.k();
               this.character = null;
               System.gc();
               this.character = new j(this);
               this.character.c(var9);
               this.characterMainUI.a(1, var42);
               this.setCurrentDisplay(this.characterMainUI);
            }
         } else if (uic.s == 4) {
            if (var1 == selectCommand) {
               int var10 = uic.a();
               if (var10 == 0) {
                  String var43 = this.character.m();
                  this.GenericInfoUI.a(5);
                  this.GenericInfoUI.a("Info", var43);
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else {
                  this.GenericInfoUI.a(6);
                  this.GenericInfoUI.a("New Character", "Character Created!\n \nPress 'Ok' to enter a name");
                  this.setCurrentDisplay(this.GenericInfoUI);
               }
            }
         } else if (uic.s == 5) {
            if (var1 == okCommand) {
               this.setCurrentDisplay(this.characterMainUI);
            }
         } else if (uic.s == 6) {
            if (var1 == okCommand) {
               this.setCurrentDisplay(this.charNameTextForm);
            }
         } else if (uic.s == 7) {
            System.gc();
            printMemory("Going into game");
            this.GenericInfoUI.a(101);
            this.GenericInfoUI.a("Introduction", k.g[9][3]);
            this.setCurrentDisplay(this.GenericInfoUI);
         } else if (uic.s == 101) {
            this.GenericInfoUI.a(102);
            this.GenericInfoUI.a("Introduction", k.g[9][4] + k.g[9][5]);
            this.setCurrentDisplay(this.GenericInfoUI);
         } else if (uic.s == 102) {
            if (var1 == okCommand) {
               this.gameCanvas.aD = this.character;
               this.character.f(false);
               this.gameCanvas.i();
               this.setCurrentDisplay(this.gameCanvas);
            }
         } else if (uic.s != 8 && uic.s != 360) {
            if (uic.s >= 9 && uic.s <= 17) {
               if (var1 == cancelCommand) {
                  this.setCurrentDisplay(uic.v);
               } else {
                  this.handleNPCChoices(uic);
               }
            } else if (uic.s == 20) {
               if (var1 == selectCommand) {
                  int var11 = uic.i;
                  int var44 = uic.a();
                  int var5 = k.b(var11, var44);
                  this.handleNPCAction(var11, 21, 5, var5);
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else if (var1 == cancelCommand) {
                  int var12 = uic.i;
                  this.setAidPointsForNPC(var12);
                  this.setCurrentDisplay(this.NPCChoicesUI[var12]);
               }
            } else if (uic.s == 52) {
               if (var1 == selectCommand) {
                  int var13 = uic.i;
                  int var45 = uic.a();
                  if (var45 >= 0) {
                     this.currentItemIndex = var45;
                     if (this.character.A(var45)) {
                        int var62 = Math.abs(this.character.af[var45]);
                        String[] var6 = new String[]{"No", "Yes"};
                        this.NPCSellSureUI = new g(this, 5, 54);
                        this.NPCSellSureUI.i = var13;
                        String var7 = "You may sell " + a.c(var62) + " for " + a.a(5, var62) + ". Confirm?";
                        this.NPCSellSureUI.a(k.r[var13], var7, var6);
                        this.NPCSellSureUI.b(cancelCommand);
                        this.setCurrentDisplay(this.NPCSellSureUI);
                     } else {
                        this.handleNPCAction(var13, 53, 15, var45);
                        this.setCurrentDisplay(this.GenericInfoUI);
                     }
                  }
               } else if (var1 == cancelCommand) {
                  int var14 = uic.i;
                  this.setAidPointsForNPC(var14);
                  this.setCurrentDisplay(this.NPCChoicesUI[var14]);
               }
            } else if (uic.s == 54) {
               if (var1 == selectCommand) {
                  int var15 = uic.a();
                  if (var15 == 0) {
                     this.setCurrentDisplay(this.NPCSellWhatUI);
                  } else {
                     int var46 = uic.i;
                     this.handleNPCAction(var46, 53, 15, this.currentItemIndex);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               }
            } else if (uic.s == 50) {
               if (var1 == selectCommand) {
                  int var16 = uic.i;
                  int var47 = uic.a();
                  if (var47 >= 0) {
                     this.currentItemIndex = var47;
                     this.handleNPCAction(var16, 51, 14, var47);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (var1 == cancelCommand) {
                  int var17 = uic.i;
                  this.setAidPointsForNPC(var17);
                  this.setCurrentDisplay(this.NPCChoicesUI[var17]);
               }
            } else if (uic.s == 27) {
               if (var1 == selectCommand) {
                  System.gc();
                  this.currentQWhat = uic.a();
                  int var18 = uic.i;
                  this.NPCQuestionWhomUI = new g(this, 5, 28);
                  this.NPCQuestionWhomUI.i = var18;
                  String[] var48 = new String[3];
                  int var63 = 0;

                  for (int var73 = 0; var73 < 4; var73++) {
                     if (var73 + 5 != var18) {
                        var48[var63] = k.r[var73 + 5];
                        var63++;
                     }
                  }

                  this.NPCQuestionWhomUI.a(k.r[var18], "Ask about whom?", var48);
                  this.NPCQuestionWhomUI.v = this.NPCChoicesUI[var18];
                  this.setCurrentDisplay(this.NPCQuestionWhomUI);
               } else if (var1 == cancelCommand) {
                  int var19 = uic.i;
                  this.setAidPointsForNPC(var19);
                  this.setCurrentDisplay(this.NPCChoicesUI[var19]);
               }
            } else if (uic.s == 28) {
               if (var1 == selectCommand) {
                  this.currentQWhom = uic.a();
                  int var20 = uic.i;
                  int var49 = (var20 - 5) * 18 + this.currentQWhat * 3 + this.currentQWhom;
                  String var64 = "";
                  int var74 = this.currentQWhom;
                  if (var74 >= var20 - 5) {
                     var74++;
                  }

                  if (this.character.ad[var49]) {
                     var49 = this.currentQWhat * 4 + var74;
                     if (this.character.ad[72 + this.currentQWhat * 3 + this.currentQWhom]) {
                        var64 = k.g[9][5 + k.i[var49]];
                     } else {
                        var64 = k.g[9][5 + k.a[var49]];
                     }
                  } else {
                     this.character.ad[var49] = true;
                     k.o[var20 - 5]--;
                     var49 = this.currentQWhat * 4 + var74;
                     if (var20 - 5 == this.character.ai) {
                        if (this.character.B < 3) {
                           this.character.B++;
                        }

                        if (this.character.B == 2 || this.character.B == 3 && nextInt(100) < 20) {
                           var64 = k.g[9][5 + k.i[var49]];
                           var49 = 72 + this.currentQWhat * 3 + this.currentQWhom;
                           this.character.ad[var49] = true;
                        }
                     }

                     if (var64 == "") {
                        var64 = k.g[9][5 + k.a[var49]];
                     }
                  }

                  this.GenericInfoUI.a(26);
                  this.GenericInfoUI.a(k.r[var20], var64);
                  this.GenericInfoUI.i = var20;
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else if (var1 == cancelCommand) {
                  int var21 = uic.i;
                  this.setAidPointsForNPC(var21);
                  this.setCurrentDisplay(this.NPCChoicesUI[var21]);
               }
            } else if (uic.s == 22) {
               if (var1 == selectCommand) {
                  int var22 = uic.i;
                  int var52 = uic.a();
                  if (var52 >= 0) {
                     this.handleNPCAction(var22, 23, 4, var52);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (var1 == cancelCommand) {
                  int var23 = uic.i;
                  this.setAidPointsForNPC(var23);
                  this.setCurrentDisplay(this.NPCChoicesUI[var23]);
               }
            } else if (uic.s == 29) {
               if (var1 == selectCommand) {
                  int var24 = uic.a();
                  if (var24 == 0) {
                     this.handleNPCAction(4, 41, 11, 0);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  } else {
                     int var53 = var24 - 1;

                     for (int var65 = 0; var65 < 4; var65++) {
                        if (!k.p[5 + var65]) {
                           if (var53 == 0) {
                              var24 = var65;
                              break;
                           }

                           var53--;
                        }
                     }

                     if (var24 == 0) {
                        this.character.ao = this.character.h = 3;
                     } else if (var24 == 1) {
                        this.character.ao = this.character.h = 12;
                     } else if (var24 == 2) {
                        this.character.ao = this.character.h = 21;
                     } else if (var24 == 3) {
                        this.character.ao = this.character.h = 30;
                     }

                     this.character.x = this.character.k = k.f[5 + var24];
                     this.character.w = this.character.j = (byte)(k.e[5 + var24] + 1);
                     this.character.aw = 1;
                     this.character.v();
                     this.setCurrentDisplay(this.gameCanvas);
                  }
               }
            } else if (uic.s != 23 && uic.s != 21 && uic.s != 24 && uic.s != 25 && uic.s != 26 && uic.s != 353 && uic.s != 355) {
               if (uic.s == 53) {
                  if (var1 == okCommand) {
                     this.NPCSellWhatUI = this.newSellWhat(uic.i);
                     this.NPCSellWhatUI.b(this.currentItemIndex);
                     this.setCurrentDisplay(this.NPCSellWhatUI);
                  }
               } else if (uic.s == 51) {
                  if (var1 == okCommand) {
                     this.NPCBuyWhatUI = this.newBuyWhat(uic.i);
                     this.NPCBuyWhatUI.b(this.currentItemIndex);
                     this.setCurrentDisplay(this.NPCBuyWhatUI);
                  }
               } else if (uic.s == 69) {
                  if (var1 == selectCommand) {
                     int var26 = uic.a();
                     if (var26 == 0) {
                        this.character.a(true);
                        this.character.ab = false;
                        this.setCurrentDisplay(this.gameCanvas);
                     } else {
                        this.setCurrentDisplay(uic.v);
                     }
                  }
               } else if (uic.s == 41) {
                  if (var1 == okCommand) {
                     this.character.ab = false;
                     this.setCurrentDisplay(this.gameCanvas);
                  }
               } else if (uic.s == 31) {
                  if (var1 == selectCommand) {
                     int var27 = uic.a();
                     switch (var27) {
                        case 0:
                           this.GenericInfoUI.a(32);
                           this.GenericInfoUI.a("Stats", this.gameCanvas.aD.i());
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
                           saveGameUI = new h(this, 10, 303);
                           saveGameUI.j();
                           Thread var54 = new Thread(this);
                           helperThreadState = 5;
                           this.setCurrentDisplay(saveGameUI);
                           var54.start();
                           break;
                        case 6:
                           System.gc();
                           this.gameCanvas.g();
                           loadGameUI = new h(this, 9, 302);
                           loadGameUI.j();
                           Thread var66 = new Thread(this);
                           helperThreadState = 6;
                           this.noSavedGameUI.v = this.OptionsUI;
                           this.setCurrentDisplay(loadGameUI);
                           var66.start();
                           break;
                        case 7:
                           this.helpUI.v = this.OptionsUI;
                           this.setCurrentDisplay(this.helpUI);
                           break;
                        case 8:
                           this.GenericInfoUI.a(68);
                           this.GenericInfoUI.a("Reveal Traitor", k.g[9][66]);
                           this.setCurrentDisplay(this.GenericInfoUI);
                           break;
                        case 9:
                           this.confirmQuitUI = this.newConfirmQuitUI(uic);
                           this.setCurrentDisplay(this.confirmQuitUI);
                     }
                  } else if (var1 == backCommand) {
                     this.setCurrentDisplay(this.gameCanvas);
                  }
               } else if (uic.s == 32) {
                  if (var1 == okCommand) {
                     this.setCurrentDisplay(this.OptionsUI);
                  }
               } else if (uic.s == 33) {
                  try {
                     debugCode = 1;
                     if (var1 == selectCommand) {
                        debugCode = 2;
                        int var28 = uic.a();
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
               } else if (uic.s == 34) {
                  if (var1 == selectCommand) {
                     int var29 = uic.a();
                     if (var29 == 0) {
                        this.character.h(this.currentItemIndex);
                     } else {
                        if (this.character.u(this.currentItemIndex)) {
                           if (--var29 == 0) {
                              if (!this.character.A(this.currentItemIndex)) {
                                 this.character.c(this.currentItemIndex, true);
                              } else {
                                 this.character.y(this.currentItemIndex);
                              }
                           }
                        }

                        if (var29 > 0 && this.character.e(this.currentItemIndex)) {
                           if (--var29 == 0) {
                              this.character.p(this.currentItemIndex);
                           }
                        }

                        if (var29 > 0 && this.character.t(this.currentItemIndex)) {
                           if (--var29 == 0) {
                              this.character.a(this.currentItemIndex);
                           }
                        }
                     }

                     if (this.character.ab) {
                        this.character.ab = false;
                        this.setCurrentDisplay(this.gameCanvas);
                     } else {
                        this.InventoryUI = this.newInventoryUI();
                        this.InventoryUI.b(this.currentItemIndex);
                        this.setCurrentDisplay(this.InventoryUI);
                     }

                     this.currentItemIndex = -1;
                  }
               } else if (uic.s == 60) {
                  if (var1 == selectCommand) {
                     int var31 = uic.a();
                     this.newClueLogUI(var31);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (uic.s == 61) {
                  if (var1 == okCommand) {
                     this.setCurrentDisplay(this.ClueUI);
                  }
               } else if (uic.s == 35) {
                  if (var1 == selectCommand) {
                     int var32 = uic.a();
                     this.GenericInfoUI.a(36);
                     int var56 = this.character.j(var32);
                     String var68 = this.character.k(var56);
                     this.GenericInfoUI.a("Skill Info", var68);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (uic.s == 36) {
                  if (var1 == okCommand) {
                     this.setCurrentDisplay(this.SkillsListUI);
                  }
               } else if (uic.s == 37) {
                  if (var1 == selectCommand) {
                     int var33 = uic.a();
                     if (var33 >= 0) {
                        this.SpellInfoUI = this.newSpellInfoUI(var33);
                        this.currentSpellIndex = var33;
                        this.setCurrentDisplay(this.SpellInfoUI);
                     }
                  }
               } else if (uic.s == 38) {
                  if (var1 == selectCommand) {
                     int var34 = this.character.z(this.currentSpellIndex);
                     this.character.c = (byte)(var34 + 1);
                     this.SpellsListUI = this.newSpellsListUI();
                     this.SpellsListUI.b(this.currentSpellIndex);
                     this.setCurrentDisplay(this.SpellsListUI);
                     this.currentSpellIndex = -1;
                  }
               } else if (uic.s == 68) {
                  this.RevealUI = this.newRevealUI();
                  this.setCurrentDisplay(this.RevealUI);
               } else if (uic.s == 65) {
                  if (var1 == selectCommand) {
                     int var35 = uic.a();
                     if (var35 == 0) {
                        this.RevealUI = this.newRevealWhomUI();
                        this.setCurrentDisplay(this.RevealUI);
                     } else {
                        this.setCurrentDisplay(uic.v);
                     }
                  }
               } else if (uic.s == 66) {
                  if (var1 == selectCommand) {
                     int var36 = uic.a();
                     StringBuffer var57 = new StringBuffer();
                     var57.append(k.g[9][68]);
                     var57.append("\n");
                     var57.append(k.g[9][69]);
                     var57.append("\n");
                     if (var36 == this.character.ai) {
                        this.character.ah = true;
                        this.character.l();
                        var57.append(k.g[9][70]);
                     } else {
                        var57.append(f.a(k.g[9][72], "<TAG>", k.r[5 + this.character.ai]));
                     }

                     this.GenericInfoUI.a(67);
                     this.GenericInfoUI.a("Reveal Traitor", var57.toString());
                     this.character.e(false);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  }
               } else if (uic.s == 67) {
                  this.character.Q = 1;
                  this.character.aj = true;
                  this.setCurrentDisplay(this.gameCanvas);
               } else if (uic.s == 39) {
                  if (var1 == selectCommand) {
                     String var37 = uic.k();
                     attribIncr[uic.i] = -1;

                     for (int var58 = 0; var58 < j.u.length; var58++) {
                        if (var37.equals(j.u[var58])) {
                           attribIncr[uic.i] = var58;
                           break;
                        }
                     }

                     if (uic.i < 2) {
                        int var69 = uic.i + 1;
                        this.LevelUpUI = this.newLevelUpUI(var69 + 1);
                        this.setCurrentDisplay(this.LevelUpUI);
                     } else {
                        this.character.n[attribIncr[0]] = (short)(this.character.n[attribIncr[0]] + 3);
                        this.character.n[attribIncr[1]] = (short)(this.character.n[attribIncr[1]] + 2);
                        this.character.n[attribIncr[2]]++;
                        this.character.f();
                        this.character.c();
                        this.setCurrentDisplay(this.gameCanvas);
                        this.gameCanvas.b();
                     }
                  }
               } else if (uic.s == 202) {
                  if (var1 == selectCommand) {
                     int var38 = uic.a();
                     if (var38 == 0) {
                        this.GenericInfoUI.a(399);
                        String var59 = "";

                        for (int var70 = 0; var70 < copyString.length; var70++) {
                           var59 = var59 + copyString[var70];
                        }

                        this.GenericInfoUI.a("Exiting", var59);
                        this.GenericInfoUI.b(okCommand);
                        this.GenericInfoUI.a(exitCommand);
                        this.setCurrentDisplay(this.GenericInfoUI);
                     } else {
                        this.setCurrentDisplay(uic.v);
                     }
                  }
               } else if (uic.s == 202) {
                  this.exit();
               } else if (uic.s == 40) {
                  this.setCurrentDisplay(this.gameCanvas);
                  this.gameCanvas.b();
               } else if (uic.s == 203) {
                  if (var1 == selectCommand) {
                     int var39 = uic.a();
                     this.GenericInfoUI.a(206);
                     this.GenericInfoUI.a(helpTitles[var39], helpStrings[var39]);
                     this.setCurrentDisplay(this.GenericInfoUI);
                  } else {
                     this.setCurrentDisplay(uic.v);
                  }
               } else if (uic.s == 206) {
                  this.setCurrentDisplay(this.helpUI);
               } else if (uic.s == 204) {
                  this.setCurrentDisplay(this.mainMenuUI);
               } else if (uic.s == 305) {
                  if (uic.v == this.OptionsUI) {
                     this.gameCanvas.i();
                  }

                  this.setCurrentDisplay(uic.v);
               } else if (uic.s == 200 || uic.s == 201) {
                  this.GenericInfoUI.a(399);
                  String var40 = "";

                  for (int var60 = 0; var60 < copyString.length; var60++) {
                     var40 = var40 + copyString[var60];
                  }

                  this.GenericInfoUI.a("Exiting", var40);
                  this.GenericInfoUI.b(okCommand);
                  this.GenericInfoUI.a(exitCommand);
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else if (uic.s == 399) {
                  this.exit();
               } else if (uic.s == 499) {
                  this.exit();
               }
            } else if (var1 == okCommand) {
               int var25 = uic.i;
               this.setAidPointsForNPC(var25);
               this.setCurrentDisplay(this.NPCChoicesUI[var25]);
            }
         } else if (var1 == okCommand) {
            this.setCurrentDisplay(this.NPCChoicesUI[uic.i]);
         }
      } else if (var2 == this.errorForm) {
         this.exit();
      } else if (var2 == this.charNameTextForm && var1 == okCommand) {
         TextField var41 = (TextField)this.charNameTextForm.get(1);
         String var61 = var41.getString();
         if (var61.length() < 3) {
            Alert var71 = new Alert("Error", f.a("Your character name must be at least <TAG> letters", "<TAG>", 3), null, AlertType.ERROR);
            var71.setTimeout(-2);
            this.setCurrentDisplay(var71);
         } else {
            this.character.Y = var61;
            this.GenericInfoUI.a(7);
            this.GenericInfoUI.a("Welcome", "Welcome to The Elder Scrolls Travels!");
            this.createGameUI = new h(this, 8, 301);
            this.createGameUI.j();
            Thread var72 = new Thread(this);
            helperThreadState = 4;
            this.setCurrentDisplay(this.createGameUI);
            var72.start();
         }
      }
   }

   private void handleNPCChoices(g var1) {
      int var2 = var1.a();
      int var3 = var1.s - 9;
      switch (var3) {
         case 0:
         case 1:
         case 2:
         case 3:
            if (var2 == 0) {
               this.NPCBuyWhatUI = this.newBuyWhat(var3);
               this.setCurrentDisplay(this.NPCBuyWhatUI);
            } else if (var2 == 1) {
               if (this.character.aq <= 0) {
                  this.GenericInfoUI.a(53);
                  this.GenericInfoUI.a(k.r[var3], "You have nothing to give me!");
                  this.GenericInfoUI.i = var3;
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else {
                  this.NPCSellWhatUI = this.newSellWhat(var3);
                  this.setCurrentDisplay(this.NPCSellWhatUI);
               }
            }
            break;
         case 4:
            if (var2 == 0) {
               String var6 = k.a(this.character, 4, 13, 0);
               if (var6 == null) {
                  var6 = "I have no new rumors.";
               }

               this.GenericInfoUI.a(360);
               this.GenericInfoUI.a(k.r[4], var6);
               this.GenericInfoUI.i = 4;
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
               if (this.character.aq <= 0) {
                  this.GenericInfoUI.a(23);
                  this.GenericInfoUI.a(k.r[var3], "You have nothing to give me!");
                  this.GenericInfoUI.i = var3;
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
               if (k.o[var3 - 5] == 0) {
                  String var4 = k.g[var3][15];
                  this.GenericInfoUI.a(26);
                  this.GenericInfoUI.a(k.r[var3], var4);
                  this.GenericInfoUI.i = var3;
                  this.setCurrentDisplay(this.GenericInfoUI);
               } else {
                  this.NPCQuestionWhatUI.i = var3;
                  this.NPCQuestionWhatUI.a(k.r[var3]);
                  this.NPCQuestionWhatUI.v = this.NPCChoicesUI[var3];
                  this.setCurrentDisplay(this.NPCQuestionWhatUI);
               }
            } else if (var2 == 5) {
               this.NPCWarpUI = new g(this, 5, 69);
               this.NPCWarpUI.i = var3;
               String[] var5 = new String[]{"Yes", "No"};
               this.NPCWarpUI.a(k.r[var3], "Warp to camp", var5);
               this.NPCWarpUI.v = this.NPCChoicesUI[var3];
               this.setCurrentDisplay(this.NPCWarpUI);
            }
      }
   }

   private void handleNPCAction(int var1, int var2, int var3, int var4) {
      this.GenericInfoUI.a(var2);
      String var5 = k.a(this.character, var1, var3, var4);
      this.GenericInfoUI.a(k.r[var1], var5);
      this.GenericInfoUI.i = var1;
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
         var10 = f.a("/imgfiles.lmp");

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
      loadGameUI.G = 0;
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

         this.character = j.a(var6, true);
         byte[] var20 = null;
         this.character.K = this;
         loadGameUI.G = 20;
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
            k.c();
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
      saveGameUI.G = 0;
      String var4 = this.getRSNameNotInUse();

      try {
         var3 = RecordStore.openRecordStore(var4, true);
         byte[] var5 = this.character.j(true);
         saveGameUI.G = 20;
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
         saveGameUI.G = 100;
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
         dungeons[var3].b = true;
         dungeons[var3].b();
         var1++;
         loadGameUI.G = 100 * var1 / 37;
         if (loadGameUI.G > 100) {
            loadGameUI.G = 100;
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
      a.i = var2.readShort();
      d.j = var2.readShort();

      for (int var3 = 0; var3 < 9; var3++) {
         k.p[var3] = var2.readBoolean();
      }

      for (int var4 = 0; var4 < 4; var4++) {
         k.q[var4] = var2.readShort();
      }

      for (int var5 = 0; var5 < 4; var5++) {
         k.o[var5] = var2.readShort();
      }

      for (int var6 = 0; var6 < 4; var6++) {
         k.b[var6] = var2.readByte();
      }

      for (int var7 = 0; var7 < 4; var7++) {
         k.k[var7] = var2.readByte();
      }

      k.h = var2.readBoolean();

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
      B var2 = null;

      try {
         var1.writeShort(a.i);
         var1.writeShort(d.j);

         for (int var3 = 0; var3 < 9; var3++) {
            var1.writeBoolean(k.p[var3]);
         }

         for (int var4 = 0; var4 < 4; var4++) {
            var1.writeShort(k.q[var4]);
         }

         for (int var5 = 0; var5 < 4; var5++) {
            var1.writeShort(k.o[var5]);
         }

         for (int var6 = 0; var6 < 4; var6++) {
            var1.writeByte(k.b[var6]);
         }

         for (int var7 = 0; var7 < 4; var7++) {
            var1.writeByte(k.k[var7]);
         }

         var1.writeBoolean(k.h);
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

      return (byte[])var2;
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
            d var9 = new d();
            byte[] var10 = null;

            while (var8.hasMoreElements()) {
               var10 = (byte[])var8.nextElement();
               d.a(var9, var10);
               var9.a(var4);
            }

            var4.flush();
            var5 = var3.toByteArray();
            var1.addRecord(var5, 0, var5.length);
            var3.reset();
            var5 = null;
            saveGameUI.G = 20 + 30 * (var6 + 1) / 37;
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
            saveGameUI.G = 50 + 30 * (var30 + 1) / 37;
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
            saveGameUI.G = 80 + 19 * (var32 + 1) / 37;
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
      d var8 = null;
      boolean var9 = false;

      for (int var10 = 1; var10 < 37; var10++) {
         var1.getRecord(var7++, var3, 0);
         monsters[var10].clear();
         int var11 = var5.readInt();

         for (int var12 = 0; var12 < var11; var12++) {
            var8 = d.a(var5);
            var8.c();
         }

         loadGameUI.G = 20 + 30 * (var10 + 1) / 37;
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
            String var15 = f.b((int)var14[0], (int)var14[1]);
            chests[var24].put(var15, var14);
         }

         loadGameUI.G = 50 + 30 * (var24 + 1) / 37;
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

         loadGameUI.G = 80 + 19 * (var26 + 1) / 37;
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
         loadGameUI.G = 80;
      } else {
         this.loadDungeonUI.G = 0;
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
         d var5 = new d();

         while (var4.hasMoreElements()) {
            byte[] var6 = (byte[])var4.nextElement();
            d.a(var5, var6);
            if (var5.l >= 1 && var5.l <= 5) {
               var1[0]++;
            } else if (var5.l >= 6 && var5.l <= 10) {
               var1[1]++;
            } else if (var5.l >= 11 && var5.l <= 25) {
               var1[2]++;
            } else if (var5.l >= 26 && var5.l <= 40) {
               var1[3]++;
            } else {
               var1[4]++;
            }
         }

         this.unloadAllMonsterImages();
         this.runImageLoader(var1);
      }

      e.B = true;
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
                     e.o[var6] = this.createImage(monster_filenames[var2][var5]);
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
               loadGameUI.G = 80 + (var2 + 1) * 20 / 5;
               this.gameCanvas.repaint();
               this.gameCanvas.serviceRepaints();
            } else {
               this.loadDungeonUI.G = (var2 + 1) * 100 / 5;
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
      int var1 = e.o.length;

      for (int var2 = 0; var2 < var1; var2++) {
         if (e.o[var2] != null) {
            e.o[var2] = null;
         }
      }

      System.gc();
   }

   static void removeMonster(int var0, int var1, int var2) {
      byte[] var3 = (byte[])monsters[var0 - 1].remove(f.b(var1, var2));
      i var4 = dungeons[var0 - 1];
      if (var3 != null) {
         var4.d[var1][var2] = f.c((byte)2, var4.d[var1][var2]);
      }
   }

   private g newInventoryItemUI(int var1) {
      g var2 = new g(this, 5, 34);
      String var3 = this.character.b(var1);
      Vector var4 = new Vector();
      var4.addElement("Drop");
      if (this.character.u(var1)) {
         if (!this.character.A(var1)) {
            var4.addElement("Equip");
         } else {
            var4.addElement("Unequip");
         }
      }

      if (this.character.e(var1)) {
         var4.addElement("Learn");
      }

      if (this.character.t(var1)) {
         var4.addElement("Use");
      }

      String[] var5 = new String[var4.size()];

      for (int var6 = 0; var6 < var4.size(); var6++) {
         var5[var6] = (String)var4.elementAt(var6);
      }

      var2.a("Item", var3, var5);
      var2.v = this.InventoryUI;
      return var2;
   }

   private void newClueLogUI(int var1) {
      System.gc();
      StringBuffer var2 = new StringBuffer();
      String var3;
      if (var1 == 4) {
         for (int var4 = 0; var4 < 6; var4++) {
            if (this.character.ad[90 + var4]) {
               var2.append(k.g[9][5 + k.m[this.character.ai][var4]]);
               var2.append("\n");
            }
         }

         var3 = "Rumors";
      } else {
         int var9 = 18 * var1;
         boolean var5 = var1 == this.character.ai;
         byte var6 = 0;

         for (int var7 = 0; var7 < 6; var7++) {
            var6 = 0;

            for (int var8 = 0; var8 < 3; var8++) {
               if (this.character.ad[var9 + var7 * 3 + var8]) {
                  if (var8 >= var1) {
                     var6 = 1;
                  }

                  if (var5 && this.character.ad[72 + var7 * 3 + var8]) {
                     var2.append(k.g[9][5 + k.i[var7 * 4 + var8 + var6]]);
                  } else {
                     var2.append(k.g[9][5 + k.a[var7 * 4 + var8 + var6]]);
                  }

                  var2.append("\n");
               }
            }
         }

         var3 = k.r[5 + var1];
      }

      if (var2.length() == 0) {
         var2.append("You have no information yet.");
      }

      this.GenericInfoUI.a(61);
      this.GenericInfoUI.a(var3, var2.toString());
   }

   private g newRevealUI() {
      System.gc();
      g var1 = new g(this, 5, 65);
      String[] var2 = new String[]{"Yes", "No"};
      var1.a("Reveal Traitor", k.g[9][67], var2);
      var1.b(cancelCommand);
      var1.v = this.OptionsUI;
      return var1;
   }

   private g newRevealWhomUI() {
      System.gc();
      g var1 = new g(this, 5, 66);
      String[] var2 = new String[]{"Alhavara", "Beatrice", "Chung", "Delacroix"};
      var1.a("Reveal Traitor", "Who is the Traitor?", var2);
      var1.v = this.OptionsUI;
      return var1;
   }

   private g newSkillsListUI() {
      System.gc();
      g var1 = new g(this, 5, 35);
      String[] var2 = this.character.e();
      var1.a("Skills", "Your Skills:", var2);
      var1.v = this.OptionsUI;
      return var1;
   }

   private g newSpellsListUI() {
      System.gc();
      g var1 = new g(this, 5, 37);
      Vector var2 = this.character.G();
      int var3 = var2.size();
      String[] var4 = new String[var3];

      for (int var5 = 0; var5 < var3; var5++) {
         var4[var5] = (String)var2.elementAt(var5);
      }

      var1.a("Spells", "Your Spells:", var4);
      var1.v = this.OptionsUI;
      return var1;
   }

   private g newSpellInfoUI(int var1) {
      System.gc();
      g var2 = new g(this, 5, 38);
      int var3 = this.character.z(var1);
      String var4 = this.character.q(var3);
      String[] var5 = new String[]{"Ready Spell"};
      var2.a("Spell Info", var4, var5);
      var2.v = this.SpellsListUI;
      return var2;
   }

   g newLevelUpUI(int var1) {
      System.gc();
      g var2 = new g(this, 5, 39);
      String[] var3 = this.character.o();
      String var4 = null;
      if (var1 == 1) {
         var2.i = 0;
         var4 = "Select an attribute to increase 3 points:";
      } else if (var1 == 2) {
         var4 = "Select an attribute to increase 2 points:";
         var2.i = 1;
      } else if (var1 == 3) {
         var2.i = 2;
         var4 = "Select an attribute to increase 1 point:";
      }

      var2.a("Level Up", var4, var3);
      var2.b(cancelCommand);
      var2.v = var2;
      return var2;
   }

   g newEndOfGameUI() {
      System.gc();
      String var1 = k.g[9][74] + "\n" + k.g[9][75] + "\n" + k.g[9][76];
      g var2 = new g(this, 4, 200);
      var2.a("Victory!", var1);
      return var2;
   }

   public g newGameOverUI() {
      System.gc();
      String var1 = k.g[9][73];
      g var2 = new g(this, 4, 201);
      var1 = f.a(var1, "<TAG>", k.r[5 + this.character.ai]);
      var2.a("Game Over", var1);
      return var2;
   }

   static DataInputStream getDataInputStream(String var0) throws Exception {
      InputStream var1 = new Object().getClass().getResourceAsStream(f.b(var0));
      if (var1 == null) {
         return null;
      }

      byte[] var2 = f.a(var1.available(), var1);
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
            if (var1 instanceof g) {
               g var2 = (g)var1;
               if (uic != var2) {
                  uic.l();
               }
            } else {
               uic.l();
            }
         }
      } catch (Throwable var4) {
         System.out.println("ERROR in hideNotify " + var4);
      }

      if (this.gameCanvas != null) {
         this.gameCanvas.d();
      }

      if (this.display == null) {
         this.display = Display.getDisplay(this);
      }

      if (var1 instanceof g) {
         uic = (g)var1;
         this.gameCanvas.Y = uic;
         this.display.setCurrent(this.gameCanvas);
         uic.e();
         if (this.gameCanvas.e()) {
            this.gameCanvas.an = true;
         } else {
            this.gameCanvas.repaint();
            this.gameCanvas.serviceRepaints();
         }
      } else if (var1 instanceof e) {
         try {
            uic = null;
            this.gameCanvas.Y = null;
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
         String var2 = this.NPCChoicesUI[var1].p;
         String var3 = this.NPCChoicesUI[var1].n();
         int var4 = 0;
         if (k.a(var1)) {
            var4 = k.o[var1 - 5];
         } else if (k.b(var1)) {
            var4 = this.character.o;
         }

         var3 = f.a(var2, "<TAG>", var4);
         this.NPCChoicesUI[var1].c(var3);
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

      int var3 = f.a(10000);
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

         var3 = f.a(10000);
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
