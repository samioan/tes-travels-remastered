// Renamed from decompiled/ESGame.java. See ../docs/CLASS_MAP.md.
//
// Unlike dawnstar's ESGame (whose own member names were already readable),
// THIS build's ESGame is single-letter obfuscated throughout, same as
// every other class here -- needed a full hand-trace, not a light retype.
// Confirmed overall shape, same broad responsibilities as dawnstar's own
// ESGame despite the renamed identifiers all being independently derived:
// owns every UIScreen instance as a named field, owns the world's static
// per-level arrays (dungeons/monsters/chests/droppedItems), getResource/
// createImage, a run() state machine over `helperThreadState` (matches
// dawnstar's exact convention: 1=unused legacy download path,
// 2=runAppload, 4=createNewGame, 5=save, 6=load), and a definite startup
// load order (runAppload -> allocateESGame -> allocAllDungeons ->
// allocateAllUIs).
//
// Real, confirmed differences from dawnstar's ESGame:
// - No datfiles.lmp archive: getResource(String) reads a resource
//   straight off the classpath and buffers it fully -- see Util.java's
//   header comment.
// - Dungeon geometry (geomin.dat, field `geomRows`) and monster image file
//   names (monsterfilenamesin.dat, field `monsterImageFileNames`) are
//   loaded directly by ESGame itself, not delegated to Dungeon/a data
//   class.
// - The hub town's fixed 19x19 tile template (dawnstar keeps this inside
//   DungeonGenerator) is built here, in `buildHubTileTemplate()` --
//   consistent with there being no separate DungeonGenerator class
//   anywhere in this codebase (see Dungeon.java's own header comment).
// - Two Player fields, not one: `player` (the live/active character) and
//   `newCharacterDraft` (a scratch Player used during character creation,
//   assigned into `player` only once the player confirms their choice).
// - A from-scratch RecordStore-based save format (not seen in dawnstar's
//   own ESGame writeup at all): `player` is one record, then a "master
//   lists" record (writeMasterLists/readMasterLists -- Item/Monster's
//   spawn-id counters plus most of Shop's static per-shop state), then
//   one record per dungeon level for each of monsters/chests/dropped
//   items (writeAllLevelRegistries/readAllLevelRegistries). Chests are
//   keyed by tile position (Util.posKey) in their Hashtable; monsters are
//   keyed by spawnId (see Monster.java); dropped items are unkeyed
//   (Vector).
// - Many UI-screen factory methods still carry their ORIGINAL debug
//   println-based names almost verbatim (e.g. "Start of newSkillsListUI"),
//   which is how most of the ~50 UIScreen fields below got confidently
//   named -- read every factory method's own println/title string before
//   assuming a field's purpose from its type alone.
//
// NOT fully resolved (left as TODO_/unconfirmed rather than guessed,
// same discipline as every other class in this codebase): a handful of
// Player-side single-arg calls whose exact method wasn't confidently
// matched against Player.java's real API (see individual call sites
// below), and three static fields (`unusedScratchBuffer`/`unusedThread`/
// `unusedCounter`) that are declared but have NO other read/write site
// anywhere in this file -- confirmed dead code via grep, not assumed.
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

public class ESGame extends ngame.midlet.RegisteredMIDlet implements Runnable, CommandListener {
   private static int lifecycleState;
   // Declared, never read/written anywhere else in this file -- confirmed
   // dead code via grep, not guessed.
   static byte[] unusedScratchBuffer = new byte[2000];
   static String serverUrl = null;
   Form errorForm;
   StringItem errorText;
   Form debugForm;
   StringItem debugText;
   private static final Command cmdMenu = new Command("Menu", 7, 0);
   private static final Command cmdExitApp = new Command("Exit", 7, 0);
   static int hubWidth;
   static int hubHeight;
   static byte[][] hubTileTemplate;
   // helperThreadState: 1=unused legacy download path, 2=runAppload boot,
   // 4=createNewGame, 5=save, 6=load -- same convention dawnstar's own
   // ESGame.run() uses.
   static int helperThreadState = 1;
   static String lastDebugMessage = "";
   // Declared, never read/written anywhere else in this file.
   private static Thread unusedThread = null;
   // Declared, never read/written anywhere else in this file (shadowed by
   // the unrelated private static method also named `E` in the original).
   private static int unusedCounter = 0;
   private static byte[][] geomRows;
   // Zone -> member dungeon-level-number list, indexed by
   // getGameAdvancementLevel()'s return value (0-8).
   private static int[][] zoneLevels = new int[][]{
      {2, 3, 4, 11, 12, 13, 20, 21, 22, 29, 30, 31}, {5, 6, 7}, {23, 24, 25}, {14, 15, 16}, {8, 9, 10}, {32, 33, 34}, {26, 27, 28}, {17, 18, 19}, {35, 36, 37}
   };
   private static String[][] monsterImageFileNames;
   // Monster-image loading chunk table: {startTypeIndex, chunkSize} per
   // chunk, consumed by runMonsterImageLoader.
   private static final int[][] monsterImageChunks = new int[][]{{0, 7}, {7, 7}, {14, 7}, {21, 7}, {28, 5}};
   static final String[] requiredResourceNames = new String[]{
      "/charin.dat", "/droppeditemsin.dat", "/itemsin.dat", "/monstersin.dat", "/npcstrings.dat", "/spellsin.dat", "/geomin.dat", "/dungnamesin.dat"
   };
   // The 3 attribute indices chosen across newLevelUpUI's 3 steps.
   private static int[] levelUpAttributeChoices = new int[3];
   // Flavor text for the game's vampirism-transformation narrative beat
   // (ailment 4 is "Vampirism" -- see Player.java's ailment-names list).
   // Not yet confirmed which method displays these; declared and read
   // nowhere else traced in this file.
   private static final String[] vampirismFlavorText = new String[]{
      "Your fingers look gnarled to you.",
      "The scales on your arms and back itch.",
      "Your ears dissolve back into your skull.",
      "Your jaw hurts as it elongates, and your teeth seem to completely fill your mouth.",
      "Walking hurts, and the camp denizens are sure looking tasty. \n\nYour dreams are filled with the screams of overseers and others as you follow the delicious scent of blood throughout the camp. You awake cold, curled up, with Vander's head tucked under your arm."
   };
   // Separate from the inherited RegisteredMIDlet.display -- this build
   // keeps its own copy, (re)assigned in initSplash()/showScreen(Object).
   Display gameDisplay;
   private Form doneForm;
   static Image mformaLogoImage = null;
   static Image vir2lLogoImage = null;
   // Splash-screen state (was `aG`/`ac` on ESGame/`h.java`'s own runSplash-
   // equivalent, see UIScreen.paintSplash/runSplash): `showCredits`
   // (static) toggles the credits variant (mformaLogoImage/vir2lLogoImage +
   // scrolling credits text) on for the middle phase of the splash fade;
   // `splashFadeStage` (per-ESGame-instance, matching the original's own
   // instance-field placement) toggles which of the two "normal splash"
   // logo images (mformaLogoImage2/vir2lLogoImage2) plus whether the
   // loading-percent bar is drawn underneath them.
   static boolean showCredits = false;
   boolean splashFadeStage = false;
   private UIScreen introUI;
   private UIScreen newGameProgressUI;
   private static UIScreen saveProgressUI;
   private static UIScreen loadProgressUI;
   private UIScreen splashUI;
   private UIScreen mainMenuUI;
   private UIScreen classSelectUI;
   private UIScreen classConfirmUI;
   private UIScreen classInfoUI;
   private UIScreen characterCreatedUI;
   private UIScreen welcomeUI;
   private UIScreen noSavedGameUI;
   // mode4/id399, built via a no-op unusedHook()-style call; selecting Ok
   // on the "Quit?" confirm shows this before exiting -- likely a blank/
   // goodbye transition screen.
   private UIScreen exitingUI;
   UIScreen monsterImageLoadProgressUI;
   UIScreen npcHelloUI;
   UIScreen rumorsUI;
   // Shops 0-5's own "aid"/action choice menu (index = shopId; shop 6,
   // Varus, has no entry here -- see UIScreen.java... it uses
   // newWardenSpeaksUI instead).
   UIScreen[] npcChoicesUI;
   UIScreen giveWhatMenuUI;
   // mode4/id23. Initially built as the "Oracle" screen, but dynamically
   // REPURPOSED at runtime as a generic single-shot NPC response popup
   // (e.g. "You have nothing to give me!", give-item confirmation) --
   // both uses share this one id/field. Confirmed by reading both
   // allocateAllUIs() (initial "Oracle" setup) and the screenGroup==22
   // dispatch branch (reassigns it via npcResponsePopup(...)) directly.
   UIScreen npcResponseUI;
   UIScreen trainWhatUI;
   UIScreen trainResultUI;
   UIScreen takeWhatUI;
   UIScreen enchantWhatUI;
   UIScreen befriendResultUI;
   UIScreen threatenResultUI;
   UIScreen killResultUI;
   UIScreen blessResultUI;
   UIScreen cureResultUI;
   UIScreen warpResultUI;
   UIScreen recoveryResultUI;
   UIScreen optionsUI;
   UIScreen statsUI;
   UIScreen inventoryUI;
   private UIScreen inventoryActionUI;
   UIScreen skillsListUI;
   UIScreen skillInfoUI;
   UIScreen spellsListUI;
   UIScreen spellInfoUI;
   UIScreen levelUpUI;
   UIScreen quitConfirmUI;
   UIScreen helpMenuUI;
   // Declared but no confirmed assignment site found anywhere in this
   // file -- left unconfirmed rather than guessed at.
   UIScreen unconfirmedScreenAV;
   // Confirmed (phase-3 port M41): assigned from GameCanvas.
   // resolveMovementSideEffects() (was decompiled/e.java's n()), which
   // shows newEndOfGameUI() through this field the moment Player.
   // pendingLockedItemFlag comes back true after a move -- Stormhold's own
   // end-of-game/victory trigger. The assignment site lives in GameCanvas,
   // not here, which is why an earlier pass reading only this file never
   // found it.
   UIScreen unconfirmedScreenAP;
   UIScreen creditsUI;
   private UIScreen helpTopicUI;
   UIScreen saveErrorUI;
   private GameCanvas gameCanvas;
   private Form enterNameForm;
   private static String[] helpTopicBodies = new String[12];
   private static String[] helpTopicTitles = new String[12];
   private static String creditsText = null;
   public Player player;
   // Scratch Player used during character creation (class select -> class
   // info confirm), assigned into `player` only on final confirmation.
   public Player newCharacterDraft;
   static Dungeon[] dungeons;
   Thread backgroundThread;
   boolean imageLoaderRunning;
   boolean imageLoaderCancelRequested;
   // Which dungeon's monster images are currently loaded/loading.
   byte currentImageLoadDungeonId;
   boolean imageLoadComplete;
   static Hashtable[] monsters;
   static Hashtable[] chests;
   static Vector[] droppedItems;
   static UIScreen activeScreen = null;
   int selectedInventorySlot;
   int selectedSpellIndex;
   static boolean unconfirmedLoadingFlag;
   static Image mformaLogoImage2;
   static Image vir2lLogoImage2;
   boolean splashActive;
   static Random rng;
   // Selects which progress UI (loadProgressUI vs the boot-time
   // monsterImageLoadProgressUI) the monster-image loader reports to.
   private static boolean loadingSavedGame = false;

   public ESGame() {
      super.appName = "The Elder Scrolls";
      this.gameDisplay = null;
      serverUrl = this.getAppProperty("Pluto-Server-URL");
      if (serverUrl != null) {
         System.out.println("FOUND Pluto-Server-URL in JAR! Adding prefix gives: " + serverUrl);
      } else {
         serverUrl = "http://localhost/essm";
         System.out.println("Did not find Pluto-Server-URL in JAR! Using default of http://localhost/essm");
      }

      String userId = this.getAppProperty("Mserver-User-Id");
      if (userId != null) {
         // TODO: original wrote this into a static field on the
         // not-yet-renamed-at-transcription-time class `j` before Player
         // was confirmed; Player.java's own pass didn't find a matching
         // "serverUserId"-shaped field to retype this against. Kept as a
         // println-only effect (matches dawnstar's own "vestigial,
         // written but never read back" finding for the same mechanism).
         System.out.println("User ID is " + userId);
      } else {
         System.out.println("User ID is NULL!");
      }

      lifecycleState = 1;
   }

   protected void startRegisteredApp() {
      if (this.gameDisplay == null) {
         debugLog("Very start of startapp");
         this.gameDisplay = Display.getDisplay(this);
         this.selectedInventorySlot = -1;
         this.selectedSpellIndex = -1;
         this.backgroundThread = new Thread(this);
         this.imageLoaderRunning = false;
         this.imageLoaderCancelRequested = false;
         this.currentImageLoadDungeonId = 0;
         this.imageLoadComplete = false;
         debugLog("Before error form");
         this.initErrorForm();
         debugLog("After error form");
         this.initSplash();
         lifecycleState = 2;
      }
   }

   private void initSplash() {
      try {
         mformaLogoImage = Image.createImage("/mformaLogo.png");
         vir2lLogoImage = Image.createImage("/vir2lLogo.png");
         Thread thread = new Thread(this);
         mformaLogoImage2 = this.createImage("/splashtop.png");
         vir2lLogoImage2 = this.createImage("/splashbot.png");
         this.splashUI = new UIScreen(this, 2, 1);
         this.splashUI.onEnter();
         this.splashUI.nextScreen = this.errorForm;
         helperThreadState = 2;
         this.splashActive = true;
         unconfirmedLoadingFlag = false;
         this.showScreen((Object)this.splashUI);
         thread.start();
      } catch (Exception e) {
         System.out.println("Barfed in initSplash");
         this.gameDisplay.setCurrent(this.errorForm);
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
            this.showScreen(this.gameCanvas);
         } else {
            this.showScreen((Object)this.saveErrorUI);
         }
      } else if (helperThreadState == 6) {
         if (this.loadGameState()) {
            this.startMonsterImageLoadForCurrentLevel();
            this.currentImageLoadDungeonId = this.player.currentLevel;
            this.imageLoaderRunning = true;
            loadingSavedGame = true;
            loadingSavedGame = false;
            this.imageLoaderRunning = false;
            loadProgressUI.progressPercent = 100;
            loadProgressUI.onEnter();
            loadProgressUI.onExit();
            this.player.refreshCorridorView();
            this.gameCanvas.player = this.player;
            this.gameCanvas.unconfirmed_v = true;
            this.gameCanvas.startGameThread();
            this.showScreen(this.gameCanvas);
         } else {
            this.showScreen((Object)this.noSavedGameUI);
         }
      } else {
         this.startMonsterImageLoadForCurrentLevel();
         this.showScreen(this.gameCanvas);
      }
   }

   private void runAppload() {
      try {
         this.splashActive = true;
         unconfirmedLoadingFlag = false;

         try {
            Thread.sleep(1000L);
         } catch (Exception e) {
         }

         this.splashActive = false;
         this.splashUI.progressPercent = 0;
         this.allocateESGame();
         this.allocAllDungeons();
         this.allocateAllUIs();
         this.splashUI.progressPercent = 100;
      } catch (Throwable t) {
         System.out.println("ERROR: CANNOT LOAD APP!!");
         System.out.println(t);
         this.logProgress("" + t, true);
         this.showScreen(this.errorForm);

         try {
            Thread.sleep(15000L);
         } catch (Exception e) {
         }
      }
   }

   private void allocateESGame() throws Exception {
      this.logProgress("Start of allocateESGame", true);
      this.logProgress("Start of allocateESGame", true);
      loadDungeonGeometryRows();
      Dungeon.loadNames();
      this.logProgress("Right before character load", true);
      Player.ensureCharDataLoaded();
      this.logProgress("ESPersonality load", true);
      Shop.loadDialogue();
      this.splashUI.progressPercent = 5;
      this.logProgress("Item load", true);
      Item.load();
      this.logProgress("Spell load", true);
      Spell.load();
      this.logProgress("Monster load", true);
      Monster.loadTypes();
      this.splashUI.progressPercent = 10;
      this.logProgress("End of allocateESGame", true);
   }

   private void allocAllDungeons() {
      dungeons = null;
      System.gc();
      System.out.println(" >>>> CREATING CAMP DUNGEON <<<<<<");
      debugLog("    Before dungeon vector");
      dungeons = new Dungeon[37];
      debugLog("    After dungeon vector");
      this.splashUI.progressPercent = 62;
      dungeons[0] = new Dungeon((byte)1, geomRows[0], hubWidth, hubHeight, hubTileTemplate);
      debugLog("    After camp dungeon before GC");
      System.gc();
      debugLog("    After camp dungeon after GC");

      for (int i = 1; i < 37; i++) {
         dungeons[i] = new Dungeon((byte)(i + 1), geomRows[i]);
         debugLog("Before dungeon " + i);
         dungeons[i].allocateAndGenerate();
         debugLog("    After dungeon " + i + " before GC");
         System.gc();
         debugLog("    After dungeon " + i);
         this.splashUI.progressPercent++;
      }

      System.out.println(" After creating dungeons");
      debugLog(" After creating dungeons, before GC");
      System.gc();
      debugLog(" After creating dungeons");
   }

   private void createNewGame() {
      System.out.println("Start of createNewGame");
      this.newGameProgressUI.progressPercent = 0;
      int zone = getGameAdvancementLevel(0);

      for (int i = 0; i <= zone; i++) {
         this.checkOpenAndPopulateDungeons(i);
      }

      this.showScreen((Object)this.classSelectUI);
   }

   private void enterCurrentZone() {
      int zone = getGameAdvancementLevel(this.player.giftPointsFound);
      openAndPopulateAllUpTo(zone);
   }

   private void allocateAllUIs() throws Exception {
      System.out.println("Starting allocateAllUIs");
      this.logProgress("Start of allocateAllUIs", true);
      System.gc();
      debugLog("Start of allocateAllUIs");
      this.initDoneForm();
      this.splashUI.progressPercent = 20;
      this.bumpLoadProgress();
      this.gameCanvas = new GameCanvas(this);
      debugLog("Before floors and walls");
      GameCanvas.floorTexture = this.createImage("floor3.png");
      GameCanvas.wallTexture = this.createImage("newwallsnok.png");
      debugLog("After walls");
      this.logProgress("After floor and wall images", true);
      GameCanvas.monsterImages = new RawImage[33];

      for (int i = 0; i < 33; i++) {
         GameCanvas.monsterImages[i] = null;
      }

      this.logProgress("After alloc monster images", true);
      this.currentImageLoadDungeonId = 1;
      this.imageLoaderRunning = true;
      this.logProgress("before runImageLoader", true);
      this.monsterImageLoadProgressUI = new UIScreen(this, 11, 304);
      this.monsterImageLoadProgressUI.onExit();
      debugLog("Before load camp monster images ");
      this.loadWardenImages();
      this.logProgress("After monster images", true);
      debugLog("After monster images ");
      this.bumpLoadProgress();
      GameCanvas.bagImages = new RawImage[3];
      GameCanvas.bagImages[0] = RawImage.load("baglarge.cus");
      GameCanvas.bagImages[1] = RawImage.load("bagmid.cus");
      GameCanvas.bagImages[2] = RawImage.load("bagsmall.cus");
      debugLog("After bag images ");
      System.gc();
      this.logProgress("After bag images ", true);
      GameCanvas.crystalImages = new RawImage[3];
      GameCanvas.crystalImages[0] = RawImage.load("crystalnear.cus");
      GameCanvas.crystalImages[1] = RawImage.load("crystalmid.cus");
      GameCanvas.crystalImages[2] = RawImage.load("crystalfar.cus");
      debugLog("After crystal images ");
      this.logProgress("After crystal images ", true);
      this.bumpLoadProgress();
      debugLog("After oracle images ");
      this.logProgress("After oracle images ", true);
      GameCanvas.effectImages = new Image[3];

      for (int i = 0; i < 3; i++) {
         GameCanvas.effectImages[i] = null;
      }

      GameCanvas.effectImages[0] = this.createImage("blood1.png");
      GameCanvas.effectImages[1] = this.createImage("monsterspell.png");
      GameCanvas.effectImages[2] = this.createImage("selfspell.png");
      System.gc();
      debugLog("After spell images ");
      this.logProgress("After effects images ", true);
      GameCanvas.chestImages = new RawImage[3];
      GameCanvas.chestImages[0] = RawImage.load("chestnearclosed.cus");
      GameCanvas.chestImages[1] = RawImage.load("chestmidclosed.cus");
      GameCanvas.chestImages[2] = RawImage.load("chestfarclosed.cus");
      this.logProgress("After chest images ", true);
      this.bumpLoadProgress();
      GameCanvas.hotbarIcons = new Image[6];
      GameCanvas.hotbarIcons[0] = this.createImage("icon_attack.png");
      GameCanvas.hotbarIcons[1] = this.createImage("icon_cast.png");
      GameCanvas.hotbarIcons[2] = this.createImage("icon_change.png");
      GameCanvas.hotbarIcons[3] = this.createImage("icon_option.png");
      GameCanvas.hotbarIcons[4] = this.createImage("icon_action.png");
      GameCanvas.hotbarIcons[5] = this.createImage("icon_camp.png");
      this.logProgress("After monster and icon images", true);
      System.gc();
      this.bumpLoadProgress();
      this.loadHelpTopicBodies();
      this.logProgress("After HELP STRINGS", true);
      this.loadHelpTopicTitles();
      this.logProgress("After HELP TITLES", true);
      creditsText = this.creditsText();
      this.logProgress("After CREDITS", true);
      this.mainMenuUI = new UIScreen(this, 3, 2);
      String[] mainMenuItems = new String[]{"New Game", "Continue Game", "Help", "Credits", "Exit"};
      Object unused = null;
      this.mainMenuUI.setupList("Main Menu", mainMenuItems, (Vector)unused, false);
      this.classSelectUI = new UIScreen(this, 5, 3);
      String[] classNames = Player.classNames;
      this.classSelectUI.setupPromptList("New Game", "Select a Class:", classNames, null);
      this.splashUI.progressPercent = 35;
      this.logProgress("After newGameUI", true);
      this.classConfirmUI = new UIScreen(this, 6, 4);
      String[] classConfirmItems = new String[]{"See Class Info", "Create Character"};
      Object unused2 = null;
      this.classConfirmUI.setupPromptList("Character", "You selected:", "", classConfirmItems, (Vector)unused2);
      this.classInfoUI = new UIScreen(this, 4, 5);
      this.classInfoUI.setupMessage("Info", "");
      this.characterCreatedUI = new UIScreen(this, 4, 6);
      this.characterCreatedUI.setupMessage("New Character", "Character Created!\n \nPress 'select' to enter a name");
      this.characterCreatedUI.setCancelCommand(UIScreen.cmdBack);
      this.characterCreatedUI.setOkCommand(UIScreen.cmdOk);
      this.welcomeUI = new UIScreen(this, 4, 7);
      this.welcomeUI.setupMessage("Welcome", "Welcome to The Elder Scrolls Travels!");
      debugLog("After all the welcome screens");
      this.logProgress("After all the welcome screens", true);
      this.saveErrorUI = new UIScreen(this, 4, 499);
      this.saveErrorUI
         .setupMessage(
            "Save Error",
            "There was an error in saving your character record. Your previous character record is still saved. Try turning your phone off then on again to clear the memory."
         );
      debugLog("Before NPCHelloUI");
      this.npcHelloUI = new UIScreen(this, 4, 8);
      this.npcHelloUI.setupMessage("NPC name here", "NPC text here", true);
      this.rumorsUI = new UIScreen(this, 4, 360);
      this.rumorsUI.setupMessage("Rumors", "Rumors text here", true);
      debugLog("After NPCHelloUI");
      this.logProgress("After helloUI", true);
      this.npcChoicesUI = new UIScreen[6];

      for (int i = 0; i < 4; i++) {
         this.npcChoicesUI[i] = new UIScreen(this, 5, 9 + i);
         String[] items = new String[]{"Train", "Give", "Befriend", "Threaten", "Kill"};
         this.npcChoicesUI[i].setupPromptList("Name", "Aid: <TAG>", items, null);
         this.npcChoicesUI[i].nextScreen = this.gameCanvas;
      }

      this.npcChoicesUI[4] = new UIScreen(this, 5, 13);
      String[] benecaItems = new String[]{"Give Item", "Take Crystal"};
      this.npcChoicesUI[4].setupPromptList("Beneca", "Aid: <TAG>", benecaItems, null);
      this.npcChoicesUI[4].nextScreen = this.gameCanvas;
      this.npcChoicesUI[5] = new UIScreen(this, 5, 14);
      String[] helgaItems = new String[]{"Rumors", "Give Crystal", "Enchant", "Bless", "Cure", "Warp", "Recovery"};
      this.npcChoicesUI[5].setupPromptList("Helga", "Aid: <TAG>", helgaItems, null);
      this.npcChoicesUI[5].nextScreen = this.gameCanvas;
      this.logProgress("After choicesUI", true);
      this.npcResponseUI = new UIScreen(this, 4, 23);
      this.npcResponseUI.setupMessage("Oracle", "NPC text here", true);
      this.helpMenuUI = new UIScreen(this, 3, 31);
      String[] optionsItems = new String[]{"Stats", "Inventory", "Skills", "Spells", "Save Game", "Load Game", "Help", "Quit Game"};
      this.optionsUI = new UIScreen(this, 3, 31);
      this.optionsUI.setupList("Options", optionsItems, null, false);
      this.optionsUI.setCancelCommand(UIScreen.cmdBack);
      this.logProgress("After OptionsUI", true);
      this.splashUI.progressPercent = 42;
      this.enterNameForm = new Form("Enter name");
      StringItem enterNamePrompt = new StringItem(null, "Enter a name for your character");
      this.enterNameForm.append(enterNamePrompt);
      TextField enterNameField = new TextField(null, null, 10, 0);
      this.enterNameForm.append(enterNameField);
      this.enterNameForm.addCommand(UIScreen.cmdOk);
      this.enterNameForm.addCommand(UIScreen.cmdBack);
      this.enterNameForm.setCommandListener(this);
      this.noSavedGameUI = new UIScreen(this, 4, 305);
      this.noSavedGameUI.setupMessage("Unavailable", "No game is available for loading. Press OK to return to main menu.");
      this.noSavedGameUI.setOkCommand(UIScreen.cmdOk);
      this.noSavedGameUI.setCommandListener(this);
      this.noSavedGameUI.nextScreen = this.mainMenuUI;
      this.logProgress("After NoSavedGameUI", true);
      this.splashUI.nextScreen = this.mainMenuUI;
      this.classSelectUI.nextScreen = this.mainMenuUI;
      this.classConfirmUI.nextScreen = this.classSelectUI;
      this.classInfoUI.nextScreen = this.classConfirmUI;
      this.classInfoUI.backTarget = this.classConfirmUI;
      this.characterCreatedUI.nextScreen = this.classConfirmUI;
      this.exitingUI = new UIScreen(this, 4, 399);
      this.exitingUI.repaint();
      this.splashUI.progressPercent = 55;
      debugLog("End of loading UI and images");
      this.logProgress("End of allocateAllUIs", true);
   }

   // "Give What?" catalog UI for shop `shopId`, listing the player's own
   // equippable inventory (each prefixed "E:" if currently equipped).
   private UIScreen giveWhatMenu(int shopId) {
      System.gc();
      UIScreen ui = new UIScreen(this, 5, 22);
      ui.contextIndex = shopId;
      String[] items = new String[this.player.inventoryCount];

      for (int i = 0; i < this.player.inventoryCount; i++) {
         int itemId = Math.abs(this.player.inventoryItemIds[i]);
         if (this.player.isEquippedSlot(i)) {
            items[i] = "E:" + Item.nameOf(itemId);
         } else {
            items[i] = Item.nameOf(itemId);
         }
      }

      ui.setupPromptList(Shop.NAMES[shopId], "Give What?", items, null, true);
      ui.nextScreen = this.gameCanvas;
      return ui;
   }

   // "Train What?" skill catalog for shop `shopId` -- only skills
   // Shop.isValidShopAction(shopId, skillIndex) allows are listed, each
   // annotated with its current gold-cost via Player.rollShopOutcome(...).
   private UIScreen trainWhatMenu(int shopId) {
      System.gc();
      debugLog("Start of newTrainWhat");
      UIScreen ui = new UIScreen(this, 5, 20);
      ui.contextIndex = shopId;
      String[] items = new String[3];
      int out = 0;

      for (int skillIndex = 0; skillIndex < 14; skillIndex++) {
         int cost = this.player.skillValue(skillIndex, false);
         String label = Player.skillNames[skillIndex] + " (<TAG>)";
         if (Shop.isValidShopAction(shopId, skillIndex)) {
            items[out++] = Util.replace(label, "<TAG>", cost);
         }
      }

      ui.setupPromptList(Shop.NAMES[shopId], "Train What?", items, null, true);
      ui.nextScreen = this.gameCanvas;
      return ui;
   }

   // "Take What?" -- Beneca's crystal-exchange catalog, listing the 13
   // "gift"/special item names (ids 87-99).
   private UIScreen takeWhatMenu(int shopId) {
      System.gc();
      debugLog("Start of newTakeWhat");
      UIScreen ui = new UIScreen(this, 5, 27);
      ui.contextIndex = shopId;
      String[] items = Item.specialItemNames();
      ui.setupPromptList(Shop.NAMES[shopId], "Take What?", items, null, true);
      ui.nextScreen = null;
      return ui;
   }

   // "Enchant What?" -- Helga's catalog, the player's own equippable
   // inventory (unprefixed, unlike giveWhatMenu).
   private UIScreen enchantWhatMenu(int shopId) {
      System.gc();
      debugLog("Start of newEnchantWhat");
      UIScreen ui = new UIScreen(this, 5, 350);
      ui.contextIndex = shopId;
      String[] items = new String[this.player.inventoryCount];

      for (int i = 0; i < this.player.inventoryCount; i++) {
         int itemId = Math.abs(this.player.inventoryItemIds[i]);
         items[i] = Item.nameOf(itemId);
      }

      ui.setupPromptList(Shop.NAMES[shopId], "Enchant What?", items, null, true);
      ui.nextScreen = this.gameCanvas;
      return ui;
   }

   private UIScreen newStatsUI() {
      UIScreen ui = new UIScreen(this, 4, 32);
      ui.setupMessage("Stats", this.gameCanvas.player.characterSheetText());
      return ui;
   }

   private UIScreen newInventoryUI() {
      System.gc();
      UIScreen ui = new UIScreen(this, 5, 33);
      String[] items = new String[this.player.inventoryCount];

      for (int i = 0; i < this.player.inventoryCount; i++) {
         byte itemId = this.player.inventoryItemIds[i];
         System.out.println("itemid is " + itemId);
         if (itemId < 0) {
            items[i] = "E: " + Item.nameOf(Math.abs(itemId));
            System.out.println("item is " + items[i]);
         } else {
            items[i] = Item.nameOf(itemId);
            System.out.println("item is " + items[i]);
         }
      }

      ui.setupPromptList("Inventory", "Items:", items, null, true);
      ui.nextScreen = this.optionsUI;
      return ui;
   }

   private UIScreen newQuitConfirmUI(UIScreen backTarget) {
      System.gc();
      UIScreen ui = new UIScreen(this, 5, 202);
      String[] items = new String[]{"Yes", "No"};
      ui.setupPromptList("Quit?", "Are you sure?", items, null, true);
      ui.nextScreen = backTarget;
      return ui;
   }

   private UIScreen newHelpMenuUI(UIScreen backTarget) {
      System.gc();
      UIScreen ui = new UIScreen(this, 3, 203);
      ui.setupList("Help", helpTopicTitles, null);
      ui.nextScreen = backTarget;
      return ui;
   }

   private UIScreen newHelpTopicUI(int topicIndex) {
      System.gc();
      UIScreen ui = new UIScreen(this, 4, 206);
      ui.setupMessage(helpTopicTitles[topicIndex], helpTopicBodies[topicIndex], true);
      ui.nextScreen = this.statsUI;
      return ui;
   }

   private UIScreen newCreditsUI(UIScreen backTarget) {
      System.gc();
      UIScreen ui = new UIScreen(this, 4, 204);
      ui.setupMessage("Credits", creditsText, true);
      ui.nextScreen = backTarget;
      return ui;
   }

   public void pauseApp() {
      this.gameCanvas.pauseTicking();
      lifecycleState = 3;
   }

   public void destroyApp(boolean unconditional) {
      lifecycleState = 4;
   }

   // The single commandAction dispatcher for every UIScreen/Form/Alert in
   // the game, keyed first on activeScreen (`ax`/`activeScreen`) being
   // non-null, then its `screenGroup` field (originally `B`). Transcribed
   // faithfully; branch bodies keep their original call shapes (including
   // several still-TODO Player/GameCanvas members -- see each file's own
   // header for what's confirmed vs placeholder).
   public void commandAction(Command cmd, Displayable src) {
      if (activeScreen != null) {
         if (cmd == UIScreen.cmdBack && activeScreen.nextScreen != null) {
            this.showScreen(activeScreen.nextScreen);
            return;
         }

         if (activeScreen.screenGroup == 2) {
            if (cmd == UIScreen.cmdOk) {
               int choice = activeScreen.selectedIndex();
               String[] items = activeScreen.visibleItemStrings();
               switch (choice) {
                  case 0:
                     System.gc();
                     this.newGameProgressUI = new UIScreen(this, 8, 301);
                     this.newGameProgressUI.onExit();
                     Thread newGameThread = new Thread(this);
                     helperThreadState = 4;
                     newGameThread.start();
                     this.showScreen((Object)this.newGameProgressUI);
                     break;
                  case 1:
                     System.gc();
                     this.gameCanvas.stopGameThread();
                     loadProgressUI = new UIScreen(this, 9, 302);
                     loadProgressUI.onExit();
                     Thread loadThread = new Thread(this);
                     helperThreadState = 6;
                     loadThread.start();
                     this.showScreen((Object)loadProgressUI);
                     break;
                  case 2:
                     this.helpMenuUI = this.newHelpMenuUI(activeScreen);
                     this.showScreen((Object)this.helpMenuUI);
                     break;
                  case 3:
                     this.creditsUI = this.newCreditsUI(activeScreen);
                     this.showScreen((Object)this.creditsUI);
                     break;
                  case 4:
                     this.unconfirmedScreenAV = this.newQuitConfirmUI(activeScreen);
                     this.showScreen((Object)this.unconfirmedScreenAV);
               }
            }
         } else if (activeScreen.screenGroup == 3) {
            if (cmd == UIScreen.cmdOk) {
               int classIndex = activeScreen.selectedIndex();
               String[] items = activeScreen.visibleItemStrings();
               this.newCharacterDraft = null;
               System.gc();
               this.newCharacterDraft = new Player(this);
               this.newCharacterDraft.applyClassTemplate(classIndex);
               this.newCharacterDraft.resetState(classIndex, false);
               this.classConfirmUI.setItemText(1, items[classIndex]);
               this.showScreen((Object)this.classConfirmUI);
            }
         } else if (activeScreen.screenGroup == 4) {
            if (cmd == UIScreen.cmdOk) {
               int choice = activeScreen.selectedIndex();
               String[] items = activeScreen.visibleItemStrings();
               if (choice == 0) {
                  String info = this.newCharacterDraft.characterSummaryShort();
                  this.classInfoUI.setItemText(0, info);
                  this.classInfoUI.windowStart = 0;
                  this.showScreen((Object)this.classInfoUI);
               } else {
                  this.player = this.newCharacterDraft;
                  this.showScreen((Object)this.characterCreatedUI);
               }
            }
         } else if (activeScreen.screenGroup == 5) {
            if (cmd == UIScreen.cmdBack) {
               this.showScreen(activeScreen.nextScreen);
            }
         } else if (activeScreen.screenGroup == 6) {
            if (cmd == UIScreen.cmdOk) {
               this.showScreen(this.enterNameForm);
            }
         } else if (activeScreen.screenGroup == 7) {
            this.introUI = new UIScreen(this, 4, 101);
            this.introUI.setupMessage("Introduction", Shop.dialogue[7][3], true);
            this.showScreen((Object)this.introUI);
         } else if (activeScreen.screenGroup == 101) {
            if (cmd == UIScreen.cmdOk) {
               this.gameCanvas.player = this.player;
               this.gameCanvas.startGameThread();
               this.showScreen(this.gameCanvas);
            }
         } else if (activeScreen.screenGroup != 8 && activeScreen.screenGroup != 360) {
            if (activeScreen.screenGroup >= 9 && activeScreen.screenGroup <= 14) {
               if (cmd == UIScreen.cmdBack) {
                  this.showScreen(activeScreen.nextScreen);
               } else {
                  this.dispatchNpcChoice(activeScreen);
               }
            } else if (activeScreen.screenGroup == 20) {
               if (cmd == UIScreen.cmdOk) {
                  int shopId = activeScreen.contextIndex;
                  int skillChoice = activeScreen.selectedIndex();
                  int cost = Shop.shopActionCode(shopId, skillChoice);
                  this.trainResultUI = this.npcResponsePopup(activeScreen, shopId, 21, 5, cost);
                  this.showScreen((Object)this.trainResultUI);
               } else if (cmd == UIScreen.cmdBack) {
                  int shopId = activeScreen.contextIndex;
                  this.refreshChoicesMenuGiftLabel(shopId);
                  this.showScreen((Object)this.npcChoicesUI[shopId]);
               }
            } else if (activeScreen.screenGroup == 22) {
               if (cmd == UIScreen.cmdOk) {
                  System.out.println("Found give what select");
                  int shopId = activeScreen.contextIndex;
                  int itemIndex = activeScreen.selectedIndex();
                  if (itemIndex >= 0) {
                     this.npcResponseUI = this.npcResponsePopup(activeScreen, shopId, 23, 4, itemIndex);
                     this.showScreen((Object)this.npcResponseUI);
                  }
               } else if (cmd == UIScreen.cmdBack) {
                  int shopId = activeScreen.contextIndex;
                  this.refreshChoicesMenuGiftLabel(shopId);
                  this.showScreen((Object)this.npcChoicesUI[shopId]);
               }
            } else if (activeScreen.screenGroup == 27) {
               if (cmd == UIScreen.cmdOk) {
                  int shopId = activeScreen.contextIndex;
                  int takeItemId = activeScreen.selectedIndex() + 87;
                  this.killResultUI = this.npcResponsePopup(activeScreen, shopId, 28, 7, takeItemId);
                  this.showScreen((Object)this.killResultUI);
               } else if (cmd == UIScreen.cmdBack) {
                  int shopId = activeScreen.contextIndex;
                  this.refreshChoicesMenuGiftLabel(shopId);
                  this.showScreen((Object)this.npcChoicesUI[shopId]);
               }
            } else if (activeScreen.screenGroup == 350) {
               if (cmd == UIScreen.cmdOk) {
                  int shopId = activeScreen.contextIndex;
                  int enchantChoice = activeScreen.selectedIndex();
                  if (enchantChoice >= 0) {
                     this.enchantWhatUI = this.npcResponsePopup(activeScreen, shopId, 351, 8, enchantChoice);
                     this.showScreen((Object)this.enchantWhatUI);
                  }
               } else if (cmd == UIScreen.cmdBack) {
                  int shopId = activeScreen.contextIndex;
                  this.refreshChoicesMenuGiftLabel(shopId);
                  this.showScreen((Object)this.npcChoicesUI[shopId]);
               }
            } else if (activeScreen.screenGroup != 23 && activeScreen.screenGroup != 21 && activeScreen.screenGroup != 24 && activeScreen.screenGroup != 25 && activeScreen.screenGroup != 28) {
               if (activeScreen.screenGroup == 26) {
                  this.showScreen(this.gameCanvas);
               } else if (activeScreen.screenGroup != 351 && activeScreen.screenGroup != 352 && activeScreen.screenGroup != 353 && activeScreen.screenGroup != 355) {
                  if (activeScreen.screenGroup == 30) {
                     if (cmd == UIScreen.cmdOk) {
                        this.showScreen(this.gameCanvas);
                     }
                  } else if (activeScreen.screenGroup == 41) {
                     if (cmd == UIScreen.cmdOk) {
                        this.player.justMarkedCamp = false;
                        this.showScreen(this.gameCanvas);
                     }
                  } else if (activeScreen.screenGroup == 31) {
                     if (cmd == UIScreen.cmdOk) {
                        int choice = activeScreen.selectedIndex();
                        String[] items = activeScreen.visibleItemStrings();
                        switch (choice) {
                           case 0:
                              this.statsUI = this.newStatsUI();
                              this.showScreen((Object)this.statsUI);
                              break;
                           case 1:
                              this.inventoryUI = this.newInventoryUI();
                              this.showScreen((Object)this.inventoryUI);
                              break;
                           case 2:
                              this.skillsListUI = this.newSkillsListUI();
                              this.showScreen((Object)this.skillsListUI);
                              break;
                           case 3:
                              this.spellsListUI = this.newSpellsListUI();
                              this.showScreen((Object)this.spellsListUI);
                              break;
                           case 4:
                              saveProgressUI = new UIScreen(this, 10, 303);
                              saveProgressUI.onExit();
                              Thread saveThread = new Thread(this);
                              helperThreadState = 5;
                              saveThread.start();
                              this.showScreen((Object)saveProgressUI);
                              break;
                           case 5:
                              System.gc();
                              this.gameCanvas.stopGameThread();
                              loadProgressUI = new UIScreen(this, 9, 302);
                              loadProgressUI.onExit();
                              Thread loadThread2 = new Thread(this);
                              helperThreadState = 6;
                              loadThread2.start();
                              this.showScreen((Object)loadProgressUI);
                              break;
                           case 6:
                              debugLog("Help");
                              this.helpMenuUI = this.newHelpMenuUI(activeScreen);
                              this.showScreen((Object)this.helpMenuUI);
                              break;
                           case 7:
                              this.creditsUI = this.newCreditsUI(activeScreen);
                              this.showScreen((Object)this.creditsUI);
                              break;
                           case 8:
                              this.showDebugForm();
                              this.showScreen(this.debugForm);
                        }
                     } else if (cmd == UIScreen.cmdBack) {
                        this.showScreen(this.gameCanvas);
                     }
                  } else if (activeScreen.screenGroup == 32) {
                     if (cmd == UIScreen.cmdOk) {
                        this.showScreen((Object)this.optionsUI);
                     }
                  } else if (activeScreen.screenGroup == 33) {
                     if (cmd == UIScreen.cmdOk) {
                        int slot = activeScreen.selectedIndex();
                        if (slot >= 0) {
                           this.inventoryActionUI = this.newInventoryItemUI(slot);
                           this.selectedInventorySlot = slot;
                           this.showScreen((Object)this.inventoryActionUI);
                        }
                     }
                  } else if (activeScreen.screenGroup == 34) {
                     if (cmd == UIScreen.cmdOk) {
                        int choiceIndex = activeScreen.selectedIndex();
                        Integer actionCode = (Integer)activeScreen.itemUserData.elementAt(choiceIndex);
                        int action = actionCode;
                        if (action == 0) {
                           this.player.dropInventoryItem(this.selectedInventorySlot);
                           this.inventoryUI = this.newInventoryUI();
                           this.showScreen((Object)this.inventoryUI);
                        } else if (action == 1) {
                           if (!this.player.isEquippedSlot(this.selectedInventorySlot)) {
                              this.player.equipItem(this.selectedInventorySlot, true);
                           } else {
                              this.player.unequipSlot(this.selectedInventorySlot);
                           }

                           this.inventoryUI = this.newInventoryUI();
                           this.showScreen((Object)this.inventoryUI);
                        } else if (action == 2) {
                           this.player.learnSpellFromScroll(this.selectedInventorySlot);
                           this.inventoryUI = this.newInventoryUI();
                           this.showScreen((Object)this.inventoryUI);
                        } else if (action == 3) {
                           this.player.useItem(this.selectedInventorySlot, null);
                           if (this.player.endOfGameTriggered) {
                              this.player.endOfGameTriggered = false;
                              this.showScreen(this.gameCanvas);
                           } else {
                              this.inventoryUI = this.newInventoryUI();
                              this.showScreen((Object)this.inventoryUI);
                           }
                        }

                        this.selectedInventorySlot = -1;
                     }
                  } else if (activeScreen.screenGroup == 35) {
                     if (cmd == UIScreen.cmdOk) {
                        int skillRow = activeScreen.selectedIndex();
                        this.skillInfoUI = this.newSkillInfoUI(skillRow);
                        this.showScreen((Object)this.skillInfoUI);
                     }
                  } else if (activeScreen.screenGroup == 36) {
                     if (cmd == UIScreen.cmdBack) {
                        this.showScreen((Object)this.skillsListUI);
                     }
                  } else if (activeScreen.screenGroup == 37) {
                     if (cmd == UIScreen.cmdOk) {
                        int spellRow = activeScreen.selectedIndex();
                        if (spellRow >= 0) {
                           this.spellInfoUI = this.newSpellInfoUI(spellRow);
                           this.selectedSpellIndex = spellRow;
                           this.showScreen((Object)this.spellInfoUI);
                        }
                     }
                  } else if (activeScreen.screenGroup == 38) {
                     if (cmd == UIScreen.cmdOk) {
                        int spellId = this.player.nthKnownSpellId(this.selectedSpellIndex);
                        this.player.selectedSpellId = (byte)(spellId + 1);
                        this.spellsListUI = this.newSpellsListUI();
                        this.showScreen((Object)this.spellsListUI);
                        this.selectedSpellIndex = -1;
                     }
                  } else if (activeScreen.screenGroup == 39) {
                     if (cmd == UIScreen.cmdOk) {
                        String chosenLabel = activeScreen.messageBody();
                        levelUpAttributeChoices[activeScreen.contextIndex] = -1;

                        for (int i = 0; i < Player.attributeNames.length; i++) {
                           if (chosenLabel.equals(Player.attributeNames[i])) {
                              levelUpAttributeChoices[activeScreen.contextIndex] = i;
                              break;
                           }
                        }

                        if (activeScreen.contextIndex < 2) {
                           int nextStep = activeScreen.contextIndex + 1;
                           this.levelUpUI = this.newLevelUpUI(nextStep + 1);
                           this.showScreen((Object)this.levelUpUI);
                        } else {
                           this.player.attributes[levelUpAttributeChoices[0]] = (short)(this.player.attributes[levelUpAttributeChoices[0]] + 3);
                           this.player.attributes[levelUpAttributeChoices[1]] = (short)(this.player.attributes[levelUpAttributeChoices[1]] + 2);
                           this.player.attributes[levelUpAttributeChoices[2]]++;
                           this.player.computeDerivedStats();
                           this.player.consumeLevelExp();
                           this.showScreen(this.gameCanvas);
                           this.gameCanvas.resumeTicking();
                        }
                     }
                  } else if (activeScreen.screenGroup == 202) {
                     if (cmd == UIScreen.cmdOk) {
                        int choice = activeScreen.selectedIndex();
                        if (choice == 0) {
                           this.showScreen((Object)this.exitingUI);
                        } else {
                           this.showScreen(activeScreen.nextScreen);
                        }
                     }
                  } else if (activeScreen.screenGroup == 202) {
                     this.exit();
                  } else if (activeScreen.screenGroup == 40) {
                     this.showScreen(this.gameCanvas);
                     this.gameCanvas.resumeTicking();
                  } else if (activeScreen.screenGroup == 102) {
                     this.showScreen(this.gameCanvas);
                     this.gameCanvas.resumeTicking();
                  } else if (activeScreen.screenGroup == 203) {
                     if (cmd == UIScreen.cmdOk) {
                        int topicIndex = activeScreen.selectedIndex();
                        this.helpTopicUI = this.newHelpTopicUI(topicIndex);
                        this.showScreen((Object)this.helpTopicUI);
                     } else {
                        this.showScreen(activeScreen.nextScreen);
                     }
                  } else if (activeScreen.screenGroup == 206) {
                     this.showScreen(activeScreen.nextScreen);
                  } else if (activeScreen.screenGroup == 204) {
                     this.showScreen(activeScreen.nextScreen);
                  } else if (activeScreen.screenGroup == 305) {
                     this.showScreen(activeScreen.nextScreen);
                  } else if (activeScreen.screenGroup == 205) {
                     this.showScreen(activeScreen.nextScreen);
                  } else if (activeScreen.screenGroup == 200 || activeScreen.screenGroup == 201) {
                     this.showScreen(activeScreen.backTarget);
                  } else if (activeScreen.screenGroup == 399) {
                     this.exit();
                  } else if (activeScreen.screenGroup == 499) {
                     this.exit();
                  }
               } else if (cmd == UIScreen.cmdBack) {
                  int shopId = activeScreen.contextIndex;
                  this.refreshChoicesMenuGiftLabel(shopId);
                  this.showScreen((Object)this.npcChoicesUI[shopId]);
               }
            } else if (cmd == UIScreen.cmdBack) {
               int shopId = activeScreen.contextIndex;
               this.refreshChoicesMenuGiftLabel(shopId);
               this.showScreen((Object)this.npcChoicesUI[shopId]);
            }
         } else if (cmd == UIScreen.cmdOk) {
            if (activeScreen.backTarget == null) {
               System.out.println("ERROR: next is null!");
            } else {
               UIScreen next = (UIScreen)activeScreen.backTarget;
               if (next == null) {
                  System.out.println("uic.next is null!");
               }
            }

            this.showScreen(activeScreen.backTarget);
         }
      } else if (src == this.errorForm) {
         this.exit();
      } else if (src == this.debugForm) {
         this.showScreen(this.gameCanvas);
      } else if (src == this.doneForm) {
         if (cmd == cmdExitApp) {
            this.exit();
         }
      } else if (src == this.enterNameForm) {
         if (cmd == UIScreen.cmdOk) {
            TextField nameField = (TextField)this.enterNameForm.get(1);
            String name = nameField.getString();
            if (name.length() < 3) {
               Alert alert = new Alert("Error", Util.replace("Your character name must be at least <TAG> letters", "<TAG>", 3), null, AlertType.ERROR);
               alert.setTimeout(-2);
               this.showScreen(alert);
            } else {
               this.player.playerName = name;
               this.showScreen((Object)this.welcomeUI);
            }
         } else if (cmd == UIScreen.cmdBack) {
            this.showScreen((Object)this.characterCreatedUI);
         }
      }
   }

   // The shared handler for shops' generic "aid" choice list (screenGroup
   // 9-12, one of Train/Give/Befriend/Threaten/Kill), and Beneca's (13)/
   // Helga's (14) own bespoke variants.
   private void dispatchNpcChoice(UIScreen ui) {
      int choice = ui.selectedIndex();
      int shopGroup = ui.screenGroup - 9;
      switch (shopGroup) {
         case 0:
         case 1:
         case 2:
         case 3:
            if (choice == 0) {
               this.trainWhatUI = this.trainWhatMenu(shopGroup);
               this.showScreen((Object)this.trainWhatUI);
            } else if (choice == 1) {
               if (this.player.inventoryCount <= 0) {
                  this.npcResponseUI.setItemText(0, Shop.NAMES[shopGroup]);
                  this.npcResponseUI.setMessageBody("You have nothing to give me!");
                  this.showScreen((Object)this.npcResponseUI);
               } else {
                  this.giveWhatMenuUI = this.giveWhatMenu(shopGroup);
                  this.showScreen((Object)this.giveWhatMenuUI);
               }
            } else if (choice == 2) {
               this.befriendResultUI = this.npcResponsePopup(ui, shopGroup, 24, 2, 0);
               this.showScreen((Object)this.befriendResultUI);
            } else if (choice == 3) {
               this.threatenResultUI = this.npcResponsePopup(ui, shopGroup, 25, 3, 0);
               this.showScreen((Object)this.threatenResultUI);
            } else if (choice == 4) {
               this.killResultUI = this.npcResponsePopup(ui, shopGroup, 26, 6, 0);
               this.showScreen((Object)this.killResultUI);
            }
            break;
         case 4:
            if (choice == 0) {
               if (this.player.inventoryCount <= 0) {
                  this.npcResponseUI.setItemText(0, Shop.NAMES[shopGroup]);
                  this.npcResponseUI.setMessageBody("You have nothing to give me!");
                  this.showScreen((Object)this.npcResponseUI);
               } else {
                  this.giveWhatMenuUI = this.giveWhatMenu(shopGroup);
                  this.showScreen((Object)this.giveWhatMenuUI);
               }
            } else if (choice == 1) {
               this.takeWhatUI = this.takeWhatMenu(shopGroup);
               this.showScreen((Object)this.takeWhatUI);
            }
            break;
         case 5:
            if (choice == 0) {
               this.showRumors();
            } else if (choice == 1) {
               if (this.player.inventoryCount <= 0) {
                  this.npcResponseUI.setItemText(0, Shop.NAMES[shopGroup]);
                  this.npcResponseUI.setMessageBody("You have nothing to give me!");
                  this.showScreen((Object)this.npcResponseUI);
               } else {
                  this.giveWhatMenuUI = this.giveWhatMenu(shopGroup);
                  this.showScreen((Object)this.giveWhatMenuUI);
               }
            } else if (choice == 2) {
               this.enchantWhatUI = this.enchantWhatMenu(shopGroup);
               this.showScreen((Object)this.enchantWhatUI);
            } else if (choice == 3) {
               this.blessResultUI = this.npcResponsePopup(ui, shopGroup, 352, 9, 0);
               this.showScreen((Object)this.blessResultUI);
            } else if (choice == 4) {
               this.cureResultUI = this.npcResponsePopup(ui, shopGroup, 353, 10, 0);
               this.showScreen((Object)this.cureResultUI);
            } else if (choice == 5) {
               this.warpResultUI = this.npcResponsePopup(ui, shopGroup, 41, 11, 0);
               this.showScreen((Object)this.warpResultUI);
            } else if (choice == 6) {
               this.recoveryResultUI = this.npcResponsePopup(ui, shopGroup, 355, 12, 0);
               this.showScreen((Object)this.recoveryResultUI);
            }
      }
   }

   // Generic "NPC response" popup factory (id `screenId`), running
   // Shop.dialogue(player, shopId, action, extra) and showing the result.
   private UIScreen npcResponsePopup(UIScreen source, int shopId, int screenId, int action, int extra) {
      UIScreen ui = new UIScreen(this, 4, screenId);
      ui.setupMessage("NPC name here", "NPC text here", true);
      String result = Shop.dialogue(this.player, shopId, action, extra);
      ui.setItemText(0, Shop.NAMES[shopId]);
      ui.setMessageBody(result);
      ui.contextIndex = shopId;
      return ui;
   }

   private Image createImage(String name) throws Exception {
      return !name.startsWith("/") ? Image.createImage("/" + name) : Image.createImage(name);
   }

   // RecordStore-based load: player record, then the master-lists record
   // (readMasterLists), then per-level monster/chest/dropped-item records
   // (readAllLevelRegistries). Returns success.
   private boolean loadGameState() {
      boolean ok = true;
      RecordStore store = null;
      loadProgressUI.progressPercent = 0;
      String name = this.findMostRecentSaveName();

      try {
         if (name == null) {
            throw new Exception("No valid record store!");
         }

         store = RecordStore.openRecordStore(name, false);
         int numRecords = store.getNumRecords();
         byte[] playerRecord = store.getRecord(1);
         this.player = Player.fromBytes(playerRecord, true);
         this.player.gameRef = this;
         loadProgressUI.progressPercent = 20;
         loadProgressUI.onExit();
         loadProgressUI.repaint();
         int nextRecordId = readPerLevelRecords(store, 2);
         System.out.println("Read the master lists from RecordStore");
         byte[] masterListsRecord = store.getRecord(nextRecordId);
         readMasterLists(masterListsRecord);
      } catch (Exception e) {
         System.out.println("Exception in loadGameState");
         System.out.println(e);
         ok = false;
      } finally {
         if (store != null) {
            try {
               store.closeRecordStore();
            } catch (Exception e) {
            }
         }
      }

      return ok;
   }

   // RecordStore-based save: writes player, master lists, then per-level
   // registries; deletes any older saves once done.
   private boolean saveGameState() {
      boolean ok = true;
      RecordStore store = null;
      saveProgressUI.progressPercent = 0;
      String name = this.generateUniqueSaveName();

      try {
         store = RecordStore.openRecordStore(name, true);
         byte[] playerRecord = this.player.toBytes(true);
         saveProgressUI.progressPercent = 20;
         saveProgressUI.onExit();
         saveProgressUI.repaint();
         store.addRecord(playerRecord, 0, playerRecord.length);
         System.gc();
         writeAllLevelRegistries(store);
         byte[] masterListsRecord = writeMasterLists();
         store.addRecord(masterListsRecord, 0, masterListsRecord.length);
         Object unused = null;
         System.gc();
         store.closeRecordStore();
         store = null;
         this.deleteOtherSaves();
         saveProgressUI.progressPercent = 100;
         saveProgressUI.onExit();
         saveProgressUI.repaint();
      } catch (Throwable t) {
         System.out.println("Exception in saveGameState");
         System.out.println(t);

         try {
            RecordStore.deleteRecordStore(name);
         } catch (Exception e) {
         }

         ok = false;
      } finally {
         if (store != null) {
            try {
               store.closeRecordStore();
            } catch (Exception e) {
            }
         }
      }

      return ok;
   }

   private void initErrorForm() {
      this.errorForm = new Form("Error");
      this.errorText = new StringItem("Error: ", "Cannot load game");
      this.errorForm.append(this.errorText);
      Command ok = new Command("Ok", 4, 1);
      this.errorForm.addCommand(ok);
      this.errorForm.setCommandListener(this);
   }

   private void logProgress(String message, boolean unused) {
      this.setDebugMessage(message);
      boolean append = false;
      if (append) {
         String text = this.errorText.getText();
         text = text + "\n" + memoryDebugString(message);
         this.errorText.setText(text);
      } else {
         this.errorText.setText(memoryDebugString(message));
      }
   }

   void handleOutOfMemory() {
      this.logProgress("Out of memory", true);
      this.gameDisplay.setCurrent(this.errorForm);
   }

   // Buckets total "gift points" collected into a 0-8 game-advancement
   // level -- gates which zones are open (see checkOpenAndPopulateDungeons/
   // openAndPopulateAllUpTo). Confirmed name from the original's own debug
   // println.
   static int getGameAdvancementLevel(int giftPoints) {
      System.out.println("In getGameAdvancementLevel, giftPoints = " + giftPoints);
      if (giftPoints < 9) {
         return 0;
      } else if (giftPoints < 13) {
         return 1;
      } else if (giftPoints < 17) {
         return 2;
      } else if (giftPoints < 23) {
         return 3;
      } else if (giftPoints < 28) {
         return 4;
      } else if (giftPoints < 34) {
         return 5;
      } else if (giftPoints < 40) {
         return 6;
      } else {
         return giftPoints < 48 ? 7 : 8;
      }
   }

   // Opens (marks populated) every dungeon level in zone `gameAdvLevel`,
   // updating newGameProgressUI's percent as it goes. Confirmed name from
   // the original's own debug println. Uses Dungeon's lightweight "open"
   // hook (Dungeon.populate()) -- see checkOpenAndPopulateDungeons's
   // sibling openAndPopulateAllUpTo for the other (Dungeon.
   // refreshTileFlagsFromRegistries()) variant used when
   // catching up multiple zones at once during a load.
   void checkOpenAndPopulateDungeons(int gameAdvLevel) {
      System.out.println("In checkOpenAndPopulateDungeons, gameAdvLevel = " + gameAdvLevel);
      int[] levels = zoneLevels[gameAdvLevel];
      int count = levels.length;

      for (int i = 0; i < count; i++) {
         int levelNumber = zoneLevels[gameAdvLevel][i];
         int idx = levelNumber - 1;
         if (!dungeons[idx].populated) {
            dungeons[idx].populated = true;
            dungeons[idx].populate();
         }

         if (this.newGameProgressUI != null) {
            this.newGameProgressUI.progressPercent = 100 * (i + 1) / count;
            if (this.newGameProgressUI.progressPercent > 100) {
               this.newGameProgressUI.progressPercent = 100;
            }

            this.newGameProgressUI.onExit();
            this.newGameProgressUI.repaint();
         }
      }
   }

   // Opens every zone's levels up to and including `gameAdvLevel` in one
   // pass, reporting progress on loadProgressUI -- used when resuming a
   // save whose advancement level may be several zones ahead of a fresh
   // game's zone 0.
   static void openAndPopulateAllUpTo(int gameAdvLevel) {
      int done = 0;
      int total = 0;

      for (int zone = 0; zone <= gameAdvLevel; zone++) {
         int[] levels = zoneLevels[zone];
         total += levels.length;
      }

      for (int zone = 0; zone <= gameAdvLevel; zone++) {
         int[] levels = zoneLevels[zone];
         int count = levels.length;

         for (int i = 0; i < count; i++) {
            int levelNumber = zoneLevels[zone][i];
            int idx = levelNumber - 1;
            dungeons[idx].populated = true;
            dungeons[idx].refreshTileFlagsFromRegistries();
            done++;
            loadProgressUI.progressPercent = 100 * done / total;
            if (loadProgressUI.progressPercent > 100) {
               loadProgressUI.progressPercent = 100;
            }
         }
      }
   }

   static void enterCurrentZoneStatic() throws Exception {
      buildHubTileTemplate();
   }

   // Hand-carves the hub town's fixed 19x19 walkable grid, then closes off
   // specific corridors to non-walkable (0) -- the hub is NOT randomly
   // generated, matching dawnstar's own DungeonGenerator precedent (this
   // codebase has no separate DungeonGenerator class; see Dungeon.java).
   private static void buildHubTileTemplate() {
      hubWidth = 19;
      hubHeight = 19;
      hubTileTemplate = new byte[hubHeight][hubWidth];

      for (int y = 0; y < hubHeight; y++) {
         for (int x = 0; x < hubWidth; x++) {
            hubTileTemplate[y][x] = 1;
         }
      }

      for (int x = 0; x < hubWidth; x++) {
         hubTileTemplate[x][9] = 0;
      }

      for (int y = 0; y < hubHeight; y++) {
         hubTileTemplate[9][y] = 0;
      }

      for (int dx = 0; dx < 3; dx++) {
         for (int dy = 0; dy < 3; dy++) {
            hubTileTemplate[8 + dx][8 + dy] = 0;
         }
      }

      hubTileTemplate[4][8] = 0;
      hubTileTemplate[4][7] = 0;
      hubTileTemplate[3][7] = 0;
      hubTileTemplate[16][8] = 0;
      hubTileTemplate[16][7] = 0;
      hubTileTemplate[15][7] = 0;
      hubTileTemplate[8][3] = 0;
      hubTileTemplate[7][3] = 0;
      hubTileTemplate[7][2] = 0;
      hubTileTemplate[10][4] = 0;
      hubTileTemplate[11][4] = 0;
      hubTileTemplate[12][4] = 0;
      hubTileTemplate[12][3] = 0;
      hubTileTemplate[6][14] = 0;
      hubTileTemplate[7][14] = 0;
      hubTileTemplate[8][14] = 0;
      hubTileTemplate[9][14] = 0;
      hubTileTemplate[10][14] = 0;
      hubTileTemplate[11][14] = 0;
      hubTileTemplate[12][14] = 0;
      hubTileTemplate[6][13] = 0;
      hubTileTemplate[12][13] = 0;
   }

   private static void initRegistries() {
      monsters = new Hashtable[37];

      for (int i = 1; i < 37; i++) {
         monsters[i] = new Hashtable();
      }

      chests = new Hashtable[37];

      for (int i = 1; i < 37; i++) {
         chests[i] = new Hashtable();
      }

      droppedItems = new Vector[37];

      for (int i = 0; i < 37; i++) {
         droppedItems[i] = new Vector();
      }
   }

   // Item.nextSpawnId/Monster.nextSpawnIdCounter plus most of Shop's
   // static per-shop state -- see writeMasterLists()'s field order for the
   // authoritative field list, both must stay in sync.
   // Reads every level's monster/chest/dropped-item records starting at
   // `firstRecordId`, one RecordStore record per level per registry
   // (monsters: levels 1-36, chests: levels 1-36, droppedItems: levels
   // 0-36), updating loadProgressUI as it goes. Returns the next unused
   // record id -- the caller then reads one more record from there for
   // the actual (differently-named-despite-the-similar-name)
   // readMasterLists(byte[]) below. Was `a(RecordStore,int)`.
   private static int readPerLevelRecords(RecordStore store, int firstRecordId) throws Exception {
      int recordId = firstRecordId;

      for (int level = 1; level < 37; level++) {
         byte[] data = store.getRecord(recordId++);
         DataInputStream in = new DataInputStream(new ByteArrayInputStream(data, 0, data.length));
         monsters[level].clear();
         int count = in.readInt();

         for (int i = 0; i < count; i++) {
            Monster m = Monster.readFrom(in);
            monsters[level].put(String.valueOf(m.spawnId), m.toBytes());
         }

         try {
            in.close();
         } catch (Exception ignored) {
         }

         System.gc();
         loadProgressUI.progressPercent = 20 + 30 * (level + 1) / 37;
         loadProgressUI.repaint();
         loadProgressUI.serviceRepaints();
      }

      for (int level = 1; level < 37; level++) {
         byte[] data = store.getRecord(recordId++);
         DataInputStream in = new DataInputStream(new ByteArrayInputStream(data, 0, data.length));
         chests[level].clear();
         int count = in.readInt();

         for (int i = 0; i < count; i++) {
            byte[] record = readBytes(in, 8);
            String key = Util.posKey(record[0], record[1]);
            chests[level].put(key, record);
         }

         try {
            in.close();
         } catch (Exception ignored) {
         }

         System.gc();
         loadProgressUI.progressPercent = 50 + 30 * (level + 1) / 37;
         loadProgressUI.repaint();
         loadProgressUI.serviceRepaints();
      }

      for (int level = 0; level < 37; level++) {
         byte[] data = store.getRecord(recordId++);
         DataInputStream in = new DataInputStream(new ByteArrayInputStream(data, 0, data.length));
         droppedItems[level].removeAllElements();
         int count = in.readInt();

         for (int i = 0; i < count; i++) {
            byte[] record = readBytes(in, 7);
            droppedItems[level].addElement(record);
         }

         try {
            in.close();
         } catch (Exception ignored) {
         }

         System.gc();
         loadProgressUI.progressPercent = 80 + 19 * (level + 1) / 37;
         loadProgressUI.repaint();
         loadProgressUI.serviceRepaints();
      }

      return recordId;
   }

   private static void readMasterLists(byte[] data) throws Exception {
      DataInputStream in = new DataInputStream(new ByteArrayInputStream(data, 0, data.length));
      Item.nextSpawnId = in.readShort();
      Monster.nextSpawnIdCounter = in.readShort();

      for (int i = 0; i < 7; i++) {
         Shop.firstVisit[i] = in.readBoolean();
      }

      for (int i = 0; i < 7; i++) {
         Shop.questRewardClaimable[i] = in.readBoolean();
      }

      for (int i = 0; i < 4; i++) {
         Shop.interactionCount[i] = in.readShort();
      }

      for (int i = 0; i < 4; i++) {
         Shop.rewardsGiven[i] = in.readShort();
      }

      for (int i = 0; i < 4; i++) {
         Shop.unconfirmedCooldownH[i] = in.readShort();
      }

      for (int i = 0; i < 4; i++) {
         Shop.questState1[i] = in.readByte();
      }

      for (int i = 0; i < 4; i++) {
         Shop.questState2[i] = in.readByte();
      }

      Shop.wardenVisitCount = in.readByte();
      Shop.wardenPresent = in.readBoolean();
      Shop.benecaPoints = in.readShort();
      Shop.helgaPoints = in.readShort();
      Shop.showSpecialGreeting = in.readBoolean();
   }

   private static byte[] writeMasterLists() throws Exception {
      ByteArrayOutputStream buf = new ByteArrayOutputStream(60);
      DataOutputStream out = new DataOutputStream(buf);
      out.writeShort(Item.nextSpawnId);
      out.writeShort(Monster.nextSpawnIdCounter);

      for (int i = 0; i < 7; i++) {
         out.writeBoolean(Shop.firstVisit[i]);
      }

      for (int i = 0; i < 7; i++) {
         out.writeBoolean(Shop.questRewardClaimable[i]);
      }

      for (int i = 0; i < 4; i++) {
         out.writeShort(Shop.interactionCount[i]);
      }

      for (int i = 0; i < 4; i++) {
         out.writeShort(Shop.rewardsGiven[i]);
      }

      for (int i = 0; i < 4; i++) {
         out.writeShort(Shop.unconfirmedCooldownH[i]);
      }

      for (int i = 0; i < 4; i++) {
         out.writeByte(Shop.questState1[i]);
      }

      for (int i = 0; i < 4; i++) {
         out.writeByte(Shop.questState2[i]);
      }

      out.writeByte(Shop.wardenVisitCount);
      out.writeBoolean(Shop.wardenPresent);
      out.writeShort(Shop.benecaPoints);
      out.writeShort(Shop.helgaPoints);
      out.writeBoolean(Shop.showSpecialGreeting);
      return buf.toByteArray();
   }

   // One record per dungeon level for each of monsters (packed 28-byte
   // Monster records via Monster.toBytes/writeTo), chests (raw 8-byte
   // records via readBytes/writeBytes), and dropped items (raw 7-byte
   // records) -- in that order, 37 records each. Reports progress on
   // saveProgressUI.
   private static void writeAllLevelRegistries(RecordStore store) throws Exception {
      for (int level = 1; level < 37; level++) {
         int count = monsters[level].size();
         int size = 4 + count * 28;
         ByteArrayOutputStream buf = new ByteArrayOutputStream(size);
         DataOutputStream out = new DataOutputStream(buf);
         out.writeInt(count);
         Enumeration e = monsters[level].elements();

         while (e.hasMoreElements()) {
            byte[] record = (byte[])e.nextElement();
            Monster m = Monster.fromBytesShared(record);
            m.writeTo(out);
         }

         byte[] bytes = buf.toByteArray();
         store.addRecord(bytes, 0, bytes.length);

         try {
            out.close();
         } catch (Exception e2) {
         }

         System.gc();
         saveProgressUI.progressPercent = 20 + 30 * (level + 1) / 37;
         saveProgressUI.onExit();
         saveProgressUI.repaint();
      }

      for (int level = 1; level < 37; level++) {
         int count = chests[level].size();
         int size = 4 + count * 8;
         ByteArrayOutputStream buf = new ByteArrayOutputStream(size);
         DataOutputStream out = new DataOutputStream(buf);
         out.writeInt(count);
         Enumeration e = chests[level].elements();

         while (e.hasMoreElements()) {
            byte[] record = (byte[])e.nextElement();
            writeBytes(out, record, 8);
         }

         byte[] bytes = buf.toByteArray();
         store.addRecord(bytes, 0, bytes.length);

         try {
            out.close();
         } catch (Exception e2) {
         }

         System.gc();
         saveProgressUI.progressPercent = 50 + 30 * (level + 1) / 37;
         saveProgressUI.onExit();
         saveProgressUI.repaint();
      }

      for (int level = 0; level < 37; level++) {
         int count = droppedItems[level].size();
         int size = 4 + count * 7;
         ByteArrayOutputStream buf = new ByteArrayOutputStream(size);
         DataOutputStream out = new DataOutputStream(buf);
         out.writeInt(count);
         Enumeration e = droppedItems[level].elements();

         while (e.hasMoreElements()) {
            byte[] record = (byte[])e.nextElement();
            writeBytes(out, record, 7);
         }

         byte[] bytes = buf.toByteArray();
         store.addRecord(bytes, 0, bytes.length);

         try {
            out.close();
         } catch (Exception e2) {
         }

         System.gc();
         saveProgressUI.progressPercent = 80 + 19 * (level + 1) / 37;
         saveProgressUI.onExit();
         saveProgressUI.repaint();
      }
   }

   // Reads the same 37+37+37 records writeAllLevelRegistries wrote,
   // starting at `firstRecordId`; returns the next free record id.
   // Monsters keyed by spawnId (String.valueOf), chests keyed by tile
   // position (Util.posKey) -- confirmed real difference, see Monster.java's
   // own header comment.
   private static int readAllLevelRegistries(RecordStore store, int firstRecordId) throws Exception {
      int recordId = firstRecordId;

      for (int level = 1; level < 37; level++) {
         byte[] bytes = store.getRecord(recordId++);
         DataInputStream in = new DataInputStream(new ByteArrayInputStream(bytes, 0, bytes.length));
         monsters[level].clear();
         int count = in.readInt();

         for (int i = 0; i < count; i++) {
            Monster m = Monster.readFrom(in);
            String key = String.valueOf(m.spawnId);
            monsters[level].put(key, m.toBytes());
         }

         try {
            in.close();
         } catch (Exception e) {
         }

         System.gc();
         loadProgressUI.progressPercent = 20 + 30 * (level + 1) / 37;
         loadProgressUI.onExit();
         loadProgressUI.repaint();
      }

      for (int level = 1; level < 37; level++) {
         byte[] bytes = store.getRecord(recordId++);
         DataInputStream in = new DataInputStream(new ByteArrayInputStream(bytes, 0, bytes.length));
         chests[level].clear();
         int count = in.readInt();

         for (int i = 0; i < count; i++) {
            byte[] record = readBytes(in, 8);
            String key = Util.posKey(record[0], record[1]);
            chests[level].put(key, record);
         }

         try {
            in.close();
         } catch (Exception e) {
         }

         System.gc();
         loadProgressUI.progressPercent = 50 + 30 * (level + 1) / 37;
         loadProgressUI.onExit();
         loadProgressUI.repaint();
      }

      for (int level = 0; level < 37; level++) {
         byte[] bytes = store.getRecord(recordId++);
         DataInputStream in = new DataInputStream(new ByteArrayInputStream(bytes, 0, bytes.length));
         droppedItems[level].removeAllElements();
         int count = in.readInt();

         for (int i = 0; i < count; i++) {
            byte[] record = readBytes(in, 7);
            droppedItems[level].addElement(record);
         }

         try {
            in.close();
         } catch (Exception e) {
         }

         System.gc();
         loadProgressUI.progressPercent = 80 + 19 * (level + 1) / 37;
         loadProgressUI.onExit();
         loadProgressUI.repaint();
      }

      return recordId;
   }

   private static byte[] readBytes(DataInputStream in, int count) throws Exception {
      byte[] out = new byte[count];

      for (int i = 0; i < count; i++) {
         out[i] = in.readByte();
      }

      return out;
   }

   private static void writeBytes(DataOutputStream out, byte[] data, int count) throws Exception {
      for (int i = 0; i < count; i++) {
         out.writeByte(data[i]);
      }
   }

   // No-op in this build -- confirmed empty body, not a transcription
   // omission.
   public static void debugLog(String message) {
   }

   static String memoryDebugString(String label) {
      if (label == null) {
         label = "";
      }

      Runtime rt = Runtime.getRuntime();
      long free = rt.freeMemory();
      long total = rt.totalMemory();
      return ">>> MEMORY: " + label + ": Free memory is " + free + ", Total memory is " + total;
   }

   private void initDoneForm() {
      this.doneForm = new Form("Done");
      StringItem doneText = new StringItem(null, "I am done");
      this.doneForm.append(doneText);
      this.doneForm.addCommand(cmdExitApp);
      this.doneForm.setCommandListener(this);
   }

   // Preloads Varus's Warden images unconditionally (all 5 image chunks)
   // -- confirmed via the original's own debug println.
   private void loadWardenImages() {
      byte[] chunks = new byte[5];
      System.out.println("LOADING WARDEN IMAGES");
      chunks[0] = 1;
      chunks[1] = 1;
      chunks[2] = 1;
      chunks[3] = 1;
      chunks[4] = 1;
      this.currentImageLoadDungeonId = 1;
      this.runMonsterImageLoader(chunks);
   }

   // Figures out which of the 5 monster-image chunks the current dungeon's
   // live monster population actually needs (bucketed by type index range)
   // and kicks off runMonsterImageLoader for just those chunks -- except
   // for the hub town (dungeon 1), which always loads via loadWardenImages
   // instead.
   void startMonsterImageLoadForCurrentLevel() {
      System.out.println("Running image loader thread for new dungeon ID: " + this.currentImageLoadDungeonId);
      this.imageLoadComplete = false;
      if (loadingSavedGame) {
         loadProgressUI.progressPercent = 80;
      } else {
         this.monsterImageLoadProgressUI.progressPercent = 0;
      }

      byte[] chunks = new byte[5];

      for (int i = 0; i < 5; i++) {
         chunks[i] = 0;
      }

      if (this.currentImageLoadDungeonId == 1) {
         this.unloadAllMonsterImages();
         this.loadWardenImages();
      } else {
         Hashtable levelMonsters = monsters[this.currentImageLoadDungeonId - 1];
         if (levelMonsters == null) {
            return;
         }

         Enumeration e = levelMonsters.elements();

         while (e.hasMoreElements()) {
            byte[] record = (byte[])e.nextElement();
            Monster m = Monster.fromBytesShared(record);
            if (m.typeIndex >= 1 && m.typeIndex <= 5) {
               chunks[0]++;
            } else if (m.typeIndex >= 6 && m.typeIndex <= 10) {
               chunks[1]++;
            } else if (m.typeIndex >= 11 && m.typeIndex <= 25) {
               chunks[2]++;
            } else if (m.typeIndex >= 26 && m.typeIndex <= 40) {
               chunks[3]++;
            } else {
               chunks[4]++;
            }
         }

         this.unloadAllMonsterImages();
         this.runMonsterImageLoader(chunks);
      }

      GameCanvas.unconfirmed_E = true;
   }

   // True for monster type indices 4/11/18/23/30 -- excluded from image
   // loading by runMonsterImageLoader below. Exact reason not confirmed
   // (possibly types that reuse another type's image, or ones with no art
   // at all in this build).
   private boolean isImagelessMonsterType(int typeIndex) {
      switch (typeIndex) {
         case 4:
         case 11:
         case 18:
         case 23:
         case 30:
            return true;
         default:
            return false;
      }
   }

   // Loads each requested chunk's monster .cus images into
   // GameCanvas.monsterImages, reporting progress on whichever of
   // loadProgressUI/monsterImageLoadProgressUI is active (see
   // `loadingSavedGame`). Bails out early if imageLoaderRunning is
   // cleared or imageLoaderCancelRequested is set mid-chunk.
   void runMonsterImageLoader(byte[] chunkCounts) {
      this.imageLoadComplete = false;
      this.logProgress("Inside runImageLoader", true);

      try {
         for (int chunk = 0; chunk < 5; chunk++) {
            if (chunkCounts[chunk] > 0) {
               this.logProgress("Handling ichunk = " + chunk, true);
               int start = monsterImageChunks[chunk][0];
               int count = monsterImageChunks[chunk][1];

               for (int i = 0; i < count; i++) {
                  int typeIndex = start + i;
                  if (!this.isImagelessMonsterType(typeIndex)) {
                     GameCanvas.monsterImages[typeIndex] = RawImage.load(monsterImageFileNames[chunk][i]);
                  }

                  if (!this.imageLoaderRunning) {
                     return;
                  }

                  if (this.imageLoaderCancelRequested) {
                     this.imageLoaderCancelRequested = false;
                     return;
                  }
               }
            }

            if (loadingSavedGame) {
               loadProgressUI.progressPercent = 80 + (chunk + 1) * 20 / 5;
               loadProgressUI.onExit();
               loadProgressUI.repaint();
            } else {
               this.monsterImageLoadProgressUI.progressPercent = (chunk + 1) * 100 / 5;
               this.monsterImageLoadProgressUI.onExit();
               this.monsterImageLoadProgressUI.repaint();
            }
         }

         this.imageLoadComplete = true;
         System.out.println("SUCCESSFULLY LOADED MONSTER IMAGES!!");
         this.imageLoaderRunning = false;
      } catch (Throwable t) {
         System.out.println("ERROR in image loader: " + t);
         this.gameDisplay.setCurrent(this.errorForm);
      }
   }

   void unloadAllMonsterImages() {
      int count = GameCanvas.monsterImages.length;

      for (int i = 0; i < count; i++) {
         if (GameCanvas.monsterImages[i] != null) {
            GameCanvas.monsterImages[i].pixels = null;
            GameCanvas.monsterImages[i] = null;
         }
      }

      System.gc();
      debugLog("After unloading all monster images");
   }

   // Removes a monster from dungLevel's live registry and clears the
   // "monster present" tile bit at its last known position. Confirmed
   // name from the original's own debug println.
   static void killMonster(int dungeonLevel, int spawnId) {
      System.out.println("In killMonster! dungid is " + dungeonLevel);
      byte[] record = (byte[])monsters[dungeonLevel - 1].remove(String.valueOf(spawnId));
      byte x = record[4];
      byte y = record[5];
      Dungeon level = dungeons[dungeonLevel - 1];
      if (record != null) {
         level.tiles[x][y] = Util.clearBit((byte)2, level.tiles[x][y]);
      }

      System.out.println("End of killMonster, size of HT is " + monsters[dungeonLevel - 1].size());
   }

   // Inventory-slot action menu (Drop/Equip-or-Unequip/Learn/Use),
   // conditionally including each option based on the item's real state.
   // Confirmed name from the original's own debug println.
   private UIScreen newInventoryItemUI(int slot) {
      UIScreen ui = new UIScreen(this, 5, 34);
      System.out.println("In newInventoryItemUI: getting item " + slot);
      String itemName = this.player.itemTooltip(slot);
      Vector labels = new Vector();
      Vector codes = new Vector();
      labels.addElement("Drop");
      codes.addElement(new Integer(0));
      if (this.player.canEquipOrUnequip(slot)) {
         if (!this.player.isEquippedSlot(slot)) {
            labels.addElement("Equip");
         } else {
            labels.addElement("Unequip");
         }

         codes.addElement(new Integer(1));
      }

      if (this.player.canLearnSpellFromScroll(slot)) {
         labels.addElement("Learn");
         codes.addElement(new Integer(2));
      }

      if (this.player.canUseItem(slot)) {
         labels.addElement("Use");
         codes.addElement(new Integer(3));
      }

      String[] items = new String[labels.size()];

      for (int i = 0; i < labels.size(); i++) {
         items[i] = (String)labels.elementAt(i);
      }

      ui.setupPromptList("Item", itemName, items, codes);
      ui.itemUserData = codes;
      ui.nextScreen = this.inventoryUI;
      return ui;
   }

   private UIScreen newSkillsListUI() {
      System.gc();
      debugLog("Start of newSkillsListUI");
      UIScreen ui = new UIScreen(this, 5, 35);
      Vector summary = this.player.skillSummaryList();
      int count = summary.size();
      String[] items = new String[count];

      for (int i = 0; i < count; i++) {
         items[i] = (String)summary.elementAt(i);
      }

      ui.setupPromptList("Skills", "Your Skills:", items, null, true);
      ui.nextScreen = this.optionsUI;
      return ui;
   }

   private UIScreen newSkillInfoUI(int row) {
      System.gc();
      debugLog("Start of newSkillInfoUI");
      UIScreen ui = new UIScreen(this, 4, 36);
      int skillIndex = this.player.nthLearnedSkillIndex(row);
      String tooltip = this.player.skillTooltip(skillIndex);
      ui.setupMessage("Skill Info", tooltip);
      ui.nextScreen = this.skillsListUI;
      return ui;
   }

   private UIScreen newSpellsListUI() {
      System.gc();
      debugLog("Start of newSpellsListUI");
      UIScreen ui = new UIScreen(this, 5, 37);
      Vector summary = this.player.knownSpellsSummary();
      int count = summary.size();
      String[] items = new String[count];

      for (int i = 0; i < count; i++) {
         items[i] = (String)summary.elementAt(i);
      }

      ui.setupPromptList("Spells", "Your Spells:", items, null, true);
      ui.nextScreen = this.optionsUI;
      return ui;
   }

   private UIScreen newSpellInfoUI(int row) {
      System.gc();
      debugLog("Start of newSpellInfoUI");
      UIScreen ui = new UIScreen(this, 5, 38);
      int spellId = this.player.nthKnownSpellId(row);
      String tooltip = this.player.spellTooltip(spellId);
      String[] items = new String[]{"Ready Spell"};
      ui.setupPromptList("Spell Info", tooltip, items, null);
      ui.itemUserData = null;
      ui.nextScreen = this.spellsListUI;
      return ui;
   }

   UIScreen newLevelUpUI(int step) {
      System.gc();
      debugLog("Start of newLevelUpUI: index= " + step);
      UIScreen ui = new UIScreen(this, 5, 39);
      String[] attrNames = this.player.pendingLevelUpAttributeNames();
      String prompt = null;
      if (step == 1) {
         ui.contextIndex = 0;
         prompt = "Select an attribute to \nincrease 3 points:";
      } else if (step == 2) {
         prompt = "Select an attribute to \nincrease 2 points:";
         ui.contextIndex = 1;
      } else if (step == 3) {
         ui.contextIndex = 2;
         prompt = "Select an attribute to \nincrease 1 point:";
      }

      ui.setupPromptList("Level Up", prompt, attrNames, null);
      ui.removeCommand(UIScreen.cmdBack);
      ui.itemUserData = null;
      ui.nextScreen = ui;
      return ui;
   }

   UIScreen newWardenSpeaksUI(String text) {
      System.gc();
      debugLog("Start of newWardenSpeaksUI");
      UIScreen ui = new UIScreen(this, 4, 102);
      ui.setupMessage("Varus", text);
      return ui;
   }

   UIScreen newEndOfGameUI() {
      System.gc();
      debugLog("Start of newEndOfGameUI");
      String text = Shop.dialogue[7][4];
      UIScreen ui = new UIScreen(this, 4, 200);
      ui.setupMessage("Victory!", text);
      ui.backTarget = this.newGameOverUI();
      return ui;
   }

   private UIScreen newGameOverUI() {
      System.gc();
      debugLog("Start of newGameOverUI");
      String text = Shop.dialogue[7][5];
      UIScreen ui = new UIScreen(this, 4, 201);
      ui.setupMessage("Game Over", text);
      ui.backTarget = this.mainMenuUI;
      return ui;
   }

   static DataInputStream getResource(String name) throws Exception {
      InputStream in = new Object().getClass().getResourceAsStream(Util.ensureLeadingSlash(name));
      if (in == null) {
         return null;
      }

      byte[] bytes = Util.readAll(in.available(), in);
      return new DataInputStream(new ByteArrayInputStream(bytes));
   }

   static int lingoRandomInt(int bound) {
      return Math.abs(rng.nextInt() % bound);
   }

   static int nextInt(int bound) {
      return 1 + Math.abs(rng.nextInt() % bound);
   }

   static int randomInt(Random rng, int bound) {
      return 1 + Math.abs(rng.nextInt() % bound);
   }

   // Sets the currently-shown Displayable: closes the previous UIScreen
   // (if any), wraps a UIScreen in a fresh ScreenCanvas (UIScreen doesn't
   // itself extend FullCanvas -- see ScreenCanvas.java's header comment),
   // and shows it; otherwise shows an arbitrary Displayable (Form/Alert/
   // the real GameCanvas) directly.
   void showScreen(Object target) {
      if (activeScreen != null) {
         if (target instanceof UIScreen) {
            UIScreen next = (UIScreen)target;
            if (activeScreen != next) {
               activeScreen.onExit();
            }
         } else {
            activeScreen.onExit();
         }
      }

      if (this.gameCanvas != null) {
         this.gameCanvas.pauseTicking();
      }

      if (this.gameDisplay == null) {
         this.gameDisplay = Display.getDisplay(this);
      }

      if (target instanceof UIScreen) {
         activeScreen = (UIScreen)target;
         ScreenCanvas canvas = UIScreen.activeCanvas();
         canvas.owner = activeScreen;
         activeScreen.targetDisplayable = canvas;
         this.gameDisplay.setCurrent(activeScreen.targetDisplayable);
         activeScreen.onEnter();
         activeScreen.repaint();
         activeScreen.serviceRepaints();
      } else if (target instanceof Displayable) {
         Displayable d = (Displayable)target;
         activeScreen = null;
         this.gameDisplay.setCurrent(d);
      }
   }

   private static void loadDungeonGeometryRows() throws Exception {
      DataInputStream in = Util.openResource("/geomin.dat");
      geomRows = new byte[37][6];

      for (int i = 0; i < 37; i++) {
         for (int c = 0; c < 6; c++) {
            geomRows[i][c] = in.readByte();
         }
      }
   }

   private static void loadMonsterImageFileNames() throws Exception {
      InputStream in = new Object().getClass().getResourceAsStream("/monsterfilenamesin.dat");
      byte[] bytes = Util.readAll(in.available(), in);
      DataInputStream data = new DataInputStream(new ByteArrayInputStream(bytes));
      monsterImageFileNames = new String[5][7];

      for (int chunk = 0; chunk < 5; chunk++) {
         for (int i = 0; i < 7; i++) {
            monsterImageFileNames[chunk][i] = data.readUTF();
         }
      }
   }

   private void showDebugForm() {
      this.debugForm = new Form("Debug");
      String text = this.gameCanvas.player.debugSummary();
      this.debugText = new StringItem("Debug: ", text);
      this.debugForm.append(this.debugText);
      Command ok = new Command("Ok", 4, 1);
      this.debugForm.addCommand(ok);
      this.debugForm.setCommandListener(this);
   }

   synchronized void setDebugMessage(String message) {
      lastDebugMessage = message;
   }

   // Confirmed via Shop.dialogue[7]'s own row indices (the "help" NPC
   // group) -- one title per help topic, 12 total.
   private void loadHelpTopicTitles() {
      helpTopicTitles[0] = Shop.dialogue[7][6];
      helpTopicTitles[1] = Shop.dialogue[7][8];
      helpTopicTitles[2] = Shop.dialogue[7][11];
      helpTopicTitles[3] = Shop.dialogue[7][13];
      helpTopicTitles[4] = Shop.dialogue[7][19];
      helpTopicTitles[5] = Shop.dialogue[7][21];
      helpTopicTitles[6] = Shop.dialogue[7][24];
      helpTopicTitles[7] = Shop.dialogue[7][29];
      helpTopicTitles[8] = Shop.dialogue[7][31];
      helpTopicTitles[9] = Shop.dialogue[7][34];
      helpTopicTitles[10] = Shop.dialogue[7][37];
      helpTopicTitles[11] = Shop.dialogue[7][39];
   }

   // Concatenates 1-5 consecutive dialogue[7] rows per help topic into
   // helpTopicBodies -- mirrors dawnstar's own "12 help topics from a flat
   // fragment pool via hardcoded index lists" ESGame.loadHelpStrings.
   private void loadHelpTopicBodies() {
      StringBuffer buf = new StringBuffer(1200);
      buf.append(Shop.dialogue[7][7]);
      helpTopicBodies[0] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][9]);
      buf.append(Shop.dialogue[7][10]);
      helpTopicBodies[1] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][12]);
      helpTopicBodies[2] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][14]);
      buf.append(Shop.dialogue[7][15]);
      buf.append(Shop.dialogue[7][16]);
      buf.append(Shop.dialogue[7][17]);
      buf.append(Shop.dialogue[7][18]);
      helpTopicBodies[3] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][20]);
      helpTopicBodies[4] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][22]);
      buf.append(Shop.dialogue[7][23]);
      helpTopicBodies[5] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][25]);
      buf.append(Shop.dialogue[7][26]);
      buf.append(Shop.dialogue[7][27]);
      buf.append(Shop.dialogue[7][28]);
      helpTopicBodies[6] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][30]);
      helpTopicBodies[7] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][32]);
      buf.append(Shop.dialogue[7][33]);
      helpTopicBodies[8] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][35]);
      buf.append(Shop.dialogue[7][36]);
      helpTopicBodies[9] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][38]);
      helpTopicBodies[10] = buf.toString();
      buf.delete(0, 1200);
      buf.append(Shop.dialogue[7][40]);
      helpTopicBodies[11] = buf.toString();
      buf.delete(0, 1200);
   }

   private String creditsText() {
      StringBuffer buf = new StringBuffer(400);
      buf.append("Game Design: Anthony Gill and Greg Gorden");
      buf.append('\n');
      buf.append("Art: Mark Jones");
      buf.append('\n');
      buf.append("Programming: Marc Ilgen");
      buf.append('\n');
      buf.append("Technical Director: Andrew Friedman");
      buf.append('\n');
      buf.append("(C) 2003 Vir2L Studos, a ZeniMax Media company. The Elder Scrolls and Vir2L are ");
      buf.append("registered trademarks of ZeniMax Media Inc. All rights reserved.");
      buf.append('\n');
      return buf.toString();
   }

   // Rebuilds a shop's own choices-menu "<TAG>" points/gold display (e.g.
   // "Aid: <TAG>") after a points-changing action, for shops that carry a
   // per-shop points value (rewardsGiven for shops 0-3, benecaPoints for
   // shop 4, helgaPoints for shop 5).
   private void refreshChoicesMenuGiftLabel(int shopId) {
      String template = this.npcChoicesUI[shopId].tagTemplate;
      String prompt = this.npcChoicesUI[shopId].messageBody();
      short value = 0;
      if (Shop.isQuestShop(shopId)) {
         value = Shop.rewardsGiven[shopId];
      } else if (shopId == 4) {
         value = Shop.benecaPoints;
      } else if (shopId == 5) {
         value = Shop.helgaPoints;
      }

      prompt = Util.replace(template, "<TAG>", value);
      this.npcChoicesUI[shopId].setMessageBody(prompt);
   }

   // Finds a RecordStore name not already in use, for a brand-new save.
   private String generateUniqueSaveName() {
      String[] existing = RecordStore.listRecordStores();
      int count = existing == null ? 0 : existing.length;
      int suffix = nextInt(10000);
      String name = "es_gamestate" + suffix;

      while (true) {
         boolean taken = false;

         for (int i = 0; i < count; i++) {
            if (name.equals(existing[i])) {
               taken = true;
            }
         }

         if (!taken) {
            return name;
         }

         suffix = nextInt(10000);
         name = "es_gamestate" + suffix;
      }
   }

   // Finds the most-recently-modified RecordStore -- the one to load from.
   private String findMostRecentSaveName() {
      String[] existing = RecordStore.listRecordStores();
      int count = existing == null ? 0 : existing.length;
      String best = null;
      if (count == 0) {
         return null;
      }

      long bestTime = 0L;
      RecordStore store = null;

      for (int i = 0; i < count; i++) {
         try {
            store = RecordStore.openRecordStore(existing[i], false);
            int numRecords = store.getNumRecords();
            long modified = store.getLastModified();
            if (modified > bestTime) {
               best = existing[i];
               bestTime = modified;
            }
         } catch (Throwable t) {
         } finally {
            try {
               if (store != null) {
                  store.closeRecordStore();
               }

               Object unused = null;
            } catch (Exception e) {
            }
         }
      }

      return best;
   }

   // Keeps only the most-recently-modified save, deleting every other
   // RecordStore -- called after a successful save.
   private void deleteOtherSaves() {
      String keep = this.findMostRecentSaveName();
      String[] existing = RecordStore.listRecordStores();
      int count = existing == null ? 0 : existing.length;
      if (count != 0) {
         for (int i = 0; i < count; i++) {
            if (keep == null || !keep.equals(existing[i])) {
               try {
                  RecordStore.deleteRecordStore(existing[i]);
               } catch (Exception e) {
               }
            }
         }
      }
   }

   // Helga's own rumor line, shown via rumorsUI. Confirmed shopId 5 (the
   // fixed literal in the original), reusing the same "<TAG>" points
   // template substitution as refreshChoicesMenuGiftLabel above.
   void showRumors() {
      String text = Shop.dialogue(this.player, 5, 13, 0);
      if (text == null) {
         text = "No rumors!";
      }

      this.rumorsUI.setItemText(0, Shop.NAMES[5]);
      this.rumorsUI.setMessageBody(text);
      this.rumorsUI.nextScreen = this.npcChoicesUI[5];
      this.rumorsUI.contextIndex = 5;
      UIScreen choicesUI = (UIScreen)this.rumorsUI.nextScreen;
      String template = choicesUI.tagTemplate;
      String prompt = choicesUI.messageBody();
      short value = Shop.helgaPoints;
      prompt = Util.replace(template, "<TAG>", value);
      choicesUI.setMessageBody(prompt);
      this.showScreen((Object)this.rumorsUI);
   }

   // Calls the inherited RegisteredMIDlet.exit() once destroyApp() has set
   // lifecycleState to 4 -- a guard against calling it from other states.
   private void bumpLoadProgress() {
      if (lifecycleState == 4) {
         this.exit();
      }
   }

   static {
      try {
         rng = new Random(System.currentTimeMillis());
         enterCurrentZoneStatic();
         loadMonsterImageFileNames();
         initRegistries();
      } catch (Exception e) {
         System.out.println("ERROR: problem with loading camp or image record HT");
      }
   }
}
