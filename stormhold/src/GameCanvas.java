// Renamed from decompiled/e.java. See ../docs/CLASS_MAP.md.
//
// PARTIAL PASS -- unlike this codebase's other renamed classes, this is
// NOT a complete transcription. `e.java` is 1823 lines; this pass fully
// confirmed and transcribed its outer structure (fields, constants,
// message-string table, constructor, the paint() dispatcher's top level,
// the full 250ms run() tick loop, keyPressed/keyReleased input handling,
// and game-thread start/stop) but did NOT read or transcribe the ~15
// private paint helper methods that do the actual pixel-level corridor/
// object/monster/HUD rendering (roughly lines 280-1000 and 1250-1823 of
// the original) -- those are left below as explicit TODO stubs with their
// real signatures (return type, name role, parameter shapes) preserved so
// the class is structurally complete and callable, but their BODIES are
// not yet ported. A follow-up pass needs to do for those methods what
// dawnstar's own `e`/GameCanvas.java pass did for its whole file.
//
// **Phase-3 M21 update:** one of those ~15 stubs, `paintWalls()` (was
// `j(Graphics)`), is now for real -- see its own doc comment. That pass
// also turned up evidence that at least one OTHER stub's placeholder
// "(was e.java's X(Graphics))" mapping was never actually verified
// against real content and looked WRONG.
//
// **Phase-3 M22 update:** every remaining stubbed paint* method (and the
// non-paint helpers they directly depend on) is now transcribed for
// real. Confirmed, in the process, that TWO of the previous pass's
// placeholder mappings were indeed swapped/wrong, and fixed them here
// rather than just flagging them:
//   - the old `paintFloor()` (`b(Graphics)`) never painted a floor at
//     all -- `paintWalls()` already does that itself (see its own doc
//     comment) -- its real body renders visible chests/dropped items on
//     corridor tiles. Renamed to `paintObjects()`.
//   - the old `paintObjects()` (`a(Graphics)`) didn't render objects --
//     its real body draws the three HP/Magicka/Fatigue HUD bars. Renamed
//     to `paintStatusBars()`.
//   - the old `paintMessagePopup()` (`e(Graphics)`) never touched a
//     message popup -- its real body is the monster-hit/spell-hit/
//     self-spell flash overlay (`unconfirmed_S`/`_ao`/`_am`). Renamed to
//     `paintFlashOverlays()`.
//   - the old `paintUnknown_l()` (`l(Graphics)`) is the REAL message
//     popup (the `messageLines`/`unconfirmed_ad`-gated rounded box).
//     Renamed to `paintMessagePopup()`, replacing the wrongly-named one
//     above.
//   - `paintHotbar1()`/`paintHotbar2()` (`k(Graphics)`/`h(Graphics)`)
//     turned out to be the two minimap zoom levels (drawing
//     `minimapTileGrid`/`visibleTileGrid`), not a hotbar at all --
//     renamed to `paintMinimapZoomedOut()`/`paintMinimapNormal()`. Their
//     dispatch in `paintGameView()` also had a real bug from the
//     earlier partial pass: it gated on `hotbarActionSet` (decompiled
//     `aq`, the numeric-hotkey selector) instead of `hotbarContext`
//     (decompiled `f`, the field actually cycled by the `*` key and
//     actually tested in the decompiled dispatcher) -- fixed here.
//   - `paintHud()` (`d(Graphics)`) and `paintUnknown_b()` (`b(Graphics,
//     int)`) turned out to be correctly named/mapped already; only their
//     bodies were missing.
// **Confirmed by phase-3 M27** (was "a byproduct worth tracking" as of
// M22): `paintMonsters()`'s real body gates each visible-object-slot
// render on the record's own `byte[6] != 0` (a live Monster record's
// `unconfirmedFlag`). Reading `Player.placeVisibleObject()` directly
// shows this flag is set (unconditionally, `data[6] = 1`, an overwrite
// not just an OR) the very first tick a monster is placed into a visible
// slot, and NOTHING anywhere in `../src/` ever clears it back to 0/false
// again (confirmed by grepping every write site). So it does NOT mean
// "alive/renderable" or track combat state at all -- it means "has the
// player ever seen this monster", permanently, matching dawnstar's own
// identical confirmed finding for its equivalent field. Still not
// renamed here (would ripple through M14/M17/M18/M20/M27's own code and
// this port's `MonsterState::unconfirmedFlag`), same reasoning as before.
//
// Confirmed identity: `extends com.nokia.mid.ui.FullCanvas implements
// Runnable`, with a `5x6x4 int[][][]` table (`wallSegmentTable`, was `n`)
// in the exact shape of dawnstar's own confirmed `GameCanvas.
// CORRIDOR_WALL_TABLE` -- this is Stormhold's real first-person corridor-
// view renderer + HUD + main tick loop, i.e. dawnstar's GameCanvas role,
// SEPARATE from the UIScreen/ScreenCanvas pair (which handle menus/
// dialogs/loading screens via a completely different, LCDUI-widget-backed
// architecture -- see UIScreen.java's header comment). `player` (was `ax`,
// type `j`) independently reconfirms `j` == Player.
//
// Field-name/class-name collision warning (same problem dawnstar's own
// CLASS_MAP.md documents for its `e`/`j`): this file declares two OWN
// fields, `static Monster nearbyMonsterScratch` (was `k`) and
// `static Monster targetMonster` (was `j`), whose original single-letter
// names are ALSO the original single-letter names of the `Shop` (`k`) and
// `Player` (`j`) classes. Several call sites in run() (`k.a(...)`/`k.c()`)
// are confirmed, by cross-checking against the already-renamed
// `Shop.shouldWardenVisit(int)`/`Shop.wardenArrives()` (see CLASS_MAP.md's
// `k`->Shop section), to be static calls to the Shop CLASS, not reads of
// this file's own `nearbyMonsterScratch` field -- printed decompiled text
// alone cannot disambiguate these, exactly the trap dawnstar flagged.
// Every remaining un-transcribed method below should be re-checked for
// the same ambiguity before assuming a bare `k`/`j` token means this
// file's own fields.
import com.nokia.mid.ui.DirectGraphics;
import com.nokia.mid.ui.DirectUtils;
import com.nokia.mid.ui.FullCanvas;
import java.util.Enumeration;
import java.util.Hashtable;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

public class GameCanvas extends FullCanvas implements Runnable {
   // M22: renamed from unusedFont_aj/unusedFont_K -- confirmed NOT
   // unused once paintHud()/paintMessagePopup()/paintMinimapZoomedOut()/
   // paintMinimapNormal() were transcribed for real; all four use one
   // of these two.
   private static final Font smallFont = Font.getFont(64, 0, 8);
   private static final Font deadScreenFont = Font.getFont(64, 2, 16);
   private static final Font campScreenFont = Font.getFont(64, 2, 16);
   private static final Font minimapFont = Font.getFont(64, 1, 16);

   // TODO: confirmed shape (5x6x4) matches dawnstar's CORRIDOR_WALL_TABLE
   // exactly, but the actual row/column semantics of THIS table's values
   // haven't been cross-checked against dawnstar's -- transcribed
   // verbatim, not yet reinterpreted.
   static final int[][][] wallSegmentTable = new int[][][]{
      {{12, 0, 0, 1}, {11, 0, -1, 1}, {12, 1, -1, 2}, {12, 2, -1, 3}, {11, 2, -2, 3}, {12, 3, -2, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {11, 1, -1, 2}, {12, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {11, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {11, 3, -1, 4}, {11, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {12, 3, 0, 4}, {12, 3, 0, 4}}
   };
   // TODO: not yet cross-checked against anything in dawnstar; purpose
   // unconfirmed beyond its shape (4 rows x 22 cols).
   private static final byte[][] unconfirmedTable_ae = new byte[][]{
      {1, 5, 31, 53, 0, 1, 40, -39, 1, 4, 13, -2, 3, 6, 71, 4, 30, 64, 2, 0, 0, 0},
      {6, 10, 31, 53, 7, 1, 27, -35, 8, 4, 1, 69, 11, 27, 66, 9, 33, 10, 10, 0, 0, 0},
      {11, 25, 31, 20, 14, 1, 0, 0, -1, -1, 2, 25, 15, 81, 8, 16, 9, 0, 17, 60, 57, 18},
      {26, 40, 31, 32, 21, 1, 0, 0, -1, -1, 43, 44, 22, 50, 25, 23, -36, 9, 24, -25, 44, 25}
   };
   // **Phase-3 M28 correction:** an earlier pass's comment here claimed
   // "41 rows x 2 cols" -- counted directly, this literal actually has
   // only 31 rows. Purpose still unconfirmed beyond its shape (mostly
   // {0,0}/{0,3}/{0,2} -- shape suggestive of dawnstar's
   // OBJECT_EXTRA_FLAGS, not confirmed), but the row count is NOT a
   // cosmetic detail: renderMonsterOrIconSprite() indexes this table at
   // `[typeIndex - 1]` for every typeIndex whose monsterNearZoneRow()
   // resolves >= 0, which includes the WHOLE 26-40 range -- not just
   // 26-31 (the only part this 31-row table can actually cover).
   // typeIndex 41 is intercepted earlier by its own `if` branch and
   // never reaches this table, but typeIndex 32-40 are ordinary,
   // confirmed-spawnable monster types (../port/src/world/
   // dungeon_generator.cpp's own kMonsterTypeByTier references types up
   // to 40 at real dungeon tiers) with no such interception. So this is
   // a REAL, REACHABLE original-game bug, not dead code the way M10's
   // leftLevelZone or M19's unreachable neighbor-throw are: any of those
   // 9 monster types walking into the player's near-view slot (1,
   // directly ahead) makes the real game evaluate
   // `unconfirmedTable_a[31..39]` on a 31-entry array --
   // ArrayIndexOutOfBoundsException on real MIDP hardware. Preserved as
   // a real crash condition in the port too (render/
   // visible_object_renderer.h throws rather than silently clamping or
   // reading garbage), not "fixed" by extending the table with invented
   // data -- there is no way to recover what real values, if any, the
   // original ever had for indices 31-39.
   private static final byte[][] unconfirmedTable_a = new byte[][]{
      {0, 0}, {0, 0}, {0, 3}, {0, 3}, {0, 3}, {0, 2}, {0, 2}, {0, 3}, {0, 3}, {0, 3}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
      {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
   };
   // TODO: 41 rows x 4 cols of booleans -- purpose unconfirmed.
   private static final boolean[][] unconfirmedTable_J = new boolean[][]{
      {false, false, false, false}, {true, false, false, false}, {false, false, false, false},
      {false, false, true, false}, {true, false, true, false}, {false, false, false, false},
      {false, false, false, false}, {false, true, false, false}, {false, false, true, false},
      {false, true, true, false}, {true, false, false, false}, {false, true, false, false},
      {true, false, true, false}, {false, true, true, false}, {true, true, true, false},
      {true, false, false, false}, {false, true, false, false}, {true, false, true, false},
      {false, true, true, false}, {true, true, true, false}, {true, true, false, false},
      {true, true, false, false}, {true, true, false, false}, {true, true, true, false},
      {true, true, true, false}, {false, false, false, false}, {false, false, false, false},
      {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
      {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
      {false, false, false, false}, {false, false, false, false}, {false, true, false, true},
      {false, true, false, true}, {true, false, false, true}, {true, false, true, false},
      {false, true, true, false}, {false, false, false, false}
   };
   // Numeric-key hotbar glyphs (1,3,5,7,9,0 -- odd keys only, plus 0) and
   // compass-direction glyphs (0=none,N,E,S,W).
   private static final char[] hotbarKeyGlyphs = new char[]{'1', '3', '5', '7', '9', '0'};
   private static final char[] compassGlyphs = new char[]{'0', 'N', 'E', 'S', 'W'};
   private static final int[][] unconfirmedTable_o = new int[][]{{0, 0, 0}, {0, 1, 0}, {0, 2, 1}};

   private ESGame game;
   private int previousGameAction;
   private int gameAction;
   private Thread gameThread = new Thread(this);
   private boolean gameThreadRunning;
   private boolean threadStarted;
   private boolean stopRequested;
   // The live Player instance this canvas renders/drives. Independently
   // reconfirms `j` == Player (see this class's header comment).
   Player player;
   byte facing;
   boolean unconfirmed_v;
   // Camp state: 0=not camping,1=rolling for interruption,2=safe/
   // undisturbed wait -- same 3-state shape dawnstar's own `campState`
   // documents (dawnstar also has a 4th "rare scripted disturbed" state
   // not yet confirmed present here).
   byte campState;
   long campRollAt;
   long unconfirmed_s;
   long unconfirmed_B;
   long unconfirmed_V;
   boolean strafeFlag;
   // Pending move direction (1-4), set by keyPressed, matches dawnstar's
   // `pendingMoveDir`.
   int pendingMoveDir;
   static boolean unconfirmed_aa;
   static boolean unconfirmed_m;
   static boolean unconfirmed_R;
   static boolean unconfirmed_W;
   static Image floorTexture;
   static Image wallTexture;
   static RawImage[] monsterImages;
   static Image[] effectImages;
   static RawImage[] chestImages;
   static RawImage[] bagImages;
   static RawImage[] crystalImages;
   static Image[] hotbarIcons;
   // 7x7 and 17x17 tile-occlusion scratch grids -- same role as dawnstar's
   // GameCanvas.visibleTileGrid (zoomed-out vs. normal minimap sampling).
   private static byte[][] minimapTileGrid;
   private static byte[][] visibleTileGrid;
   // Hotbar context selector: 1 or 2, cycled by numeric key '9' when
   // aq(hotbarActionSet)==2 -- gates which of two hotbar paint methods
   // (paintHotbar1/paintHotbar2 below) runs each frame.
   private static int hotbarContext = 1;
   static boolean unconfirmed_S = false;
   static boolean unconfirmed_ao = false;
   static boolean unconfirmed_am = false;
   // Selects which of three numeric-key actions (0/1/9) is currently
   // "armed" -- 0 gates key '0', 1 gates key '1', 2 gates key '9'. See
   // keyPressed().
   static int hotbarActionSet;
   static boolean unconfirmed_av = false;
   static boolean unconfirmed_ay = false;
   static boolean unconfirmed_I = false;
   static boolean unconfirmed_Z = false;
   static boolean unconfirmed_ap = false;
   static boolean unconfirmed_U = false;
   static boolean unconfirmed_ad = false;
   // Timestamp a message popup was last shown -- gates its 3s auto-hide,
   // matches dawnstar's `messageShownAt`.
   static long messageShownAt = 0L;
   static String[] messageLines = null;
   // Confirmed (phase-3 port M30): the message popup's own current
   // priority -- showMessage()'s priority-gate state (`X` in decompiled/
   // e.java), matches dawnstar's `messagePriority`.
   private static int messagePriority = 0;
   static boolean unconfirmed_af = false;
   private static boolean autoRepaintEnabled = true;
   private static boolean unconfirmed_at = false;
   static boolean unconfirmed_E = false;
   static boolean unconfirmed_A = false;

   // Message-popup string table -- same convention as dawnstar's `MSG_*`
   // constants (2-line String[2] each), confirmed 1:1 by content for every
   // entry dawnstar also has, PLUS two Stormhold-only entries
   // (MSG_WARDENS_CAMP/MSG_OUTER_CAMP) tied to the Warden mechanic
   // Shop.java's own section documents.
   static final String[] MSG_WARDENS_CAMP = new String[]{"Warden's", "Camp"};
   static final String[] MSG_OUTER_CAMP = new String[]{"Outer", "Camp"};
   static final String[] MSG_CANNOT_CAMP = new String[]{"Cannot", "Camp!"};
   static final String[] MSG_NO_SPELLS = new String[]{"No spells!", ""};
   static final String[] MSG_NOT_ENOUGH_MAGICKA = new String[]{"Not enough", "magic!"};
   static final String[] MSG_NO_MONSTER = new String[]{"No monster", "here!"};
   static final String[] MSG_REST_DISTURBED = new String[]{"Rest", "disturbed!"};
   static final String[] MSG_REST_COMPLETE = new String[]{"Rest", "complete!"};
   static final String[] MSG_CREATURE_DEAD = new String[]{"Creature", "is dead!"};
   static final String[] MSG_CREATURE_ATTACKS = new String[]{"Creature", "attacks!"};
   static final String[] MSG_CHEST = new String[]{"Chest", ""};
   static final String[] MSG_CHEST_LOCKED = new String[]{"Chest", "locked!"};
   static final String[] MSG_INVENTORY_FULL = new String[]{"Inventory", "full!"};
   static final String[] MSG_FOUND_ITEM = new String[]{"Found", "item!"};
   static final String[] MSG_FOUND_SEVERAL_ITEMS = new String[]{"Several", "items!"};

   // Confirmed duplicate of Shop.NAMES[0..6] -- likely used to paint an
   // NPC's name near its dialogue/portrait. String[7][2] like Item's
   // specialEffectText (pre-split into up to 2 display lines), though
   // every entry here only ever fills the first line.
   static final String[][] npcNameLines = new String[][]{
      {"Arantamo", ""}, {"Celegil", ""}, {"Favela Dralor", ""}, {"Vander", ""}, {"Beneca", ""}, {"Helga", ""}, {"Varus", ""}
   };

   // Field-name/class-name collision -- see this file's header comment.
   // NOT the Shop/Player classes despite reusing their original letters.
   static Monster nearbyMonsterScratch = new Monster();
   static Monster targetMonster = null;
   private static long unconfirmed_ai = 0L;
   private static boolean popupMessageActive = false;
   private static String popupMessageText = null;

   public GameCanvas(ESGame game) {
      this.game = game;
      this.previousGameAction = 0;
      this.gameAction = 0;
      this.gameThreadRunning = false;
      this.threadStarted = false;
      this.stopRequested = false;
      this.player = null;
      this.facing = 1;
      this.unconfirmed_v = false;
      this.campState = 0;
      this.campRollAt = 0L;
      this.unconfirmed_s = 0L;
      this.pendingMoveDir = 0;
      this.strafeFlag = false;
      unconfirmed_aa = false;
      unconfirmed_m = false;
      unconfirmed_R = false;
      unconfirmed_W = false;
      minimapTileGrid = new byte[7][7];
      visibleTileGrid = new byte[17][17];
      hotbarActionSet = 0;
      this.unconfirmed_B = 0L;
      this.unconfirmed_V = 0L;
   }

   // Top-level paint dispatch: dead-screen (facing/aD==3), camp screen
   // (campState 1 or 2), else the real 3D game view.
   public void paint(Graphics g) {
      if (this.facing == 3) {
         this.paintDeadScreen(g);
      } else if (this.campState != 1 && this.campState != 2) {
         this.paintGameView(g);
      } else {
         this.paintCampScreen(g);
      }
   }

   private void paintDeadScreen(Graphics g) {
      g.setColor(0);
      g.fillRect(0, 0, this.getWidth(), this.getHeight());
      g.setColor(16777215);
      g.setFont(deadScreenFont);
      g.drawString("You're Dead!", this.getWidth() / 2, this.getHeight() / 2, 33);
   }

   private void paintCampScreen(Graphics g) {
      g.setColor(0);
      g.fillRect(0, 0, this.getWidth(), this.getHeight());
      g.setColor(16777215);
      g.setFont(campScreenFont);
      g.drawString("CAMPING", this.getWidth() / 2, this.getHeight() / 2, 33);
   }

   // Main game-view paint: black background, corridor walls (which also
   // paint the floor themselves, see paintWalls()'s own comment),
   // objects (chests/dropped items), (conditionally) the quest-shop
   // status icon gated by unconfirmed_W using player.
   // questShopAtPendingTile(), monsters (try/caught separately -- the
   // original wraps just this call so a bad monster paint can't blank
   // the whole frame), the HP/Magicka/Fatigue status bars, the HUD panel,
   // the message popup, the hit/spell flash overlay, the popup-text
   // overlay, and one of the two minimap zoom levels (selected by
   // hotbarContext, cycled by the `*` key) when no dialogue-equivalent
   // Screen is blocking key 3.
   private void paintGameView(Graphics g) {
      g.setColor(0);
      g.fillRect(0, 0, this.getWidth(), this.getHeight());
      this.paintWalls(g);
      this.paintObjects(g);
      if (unconfirmed_W) {
         int stat = this.player.questShopAtPendingTile();
         this.paintUnknown_b(g, stat);
      }

      try {
         this.paintMonsters(g);
      } catch (Throwable t) {
         System.out.println("Error in paintMonsters: " + t);
      }

      this.paintStatusBars(g);
      this.paintHud(g);
      this.paintMessagePopup(g);
      this.paintFlashOverlays(g);
      if (popupMessageActive) {
         this.paintPopupText(g);
      }

      // Bug carried over from the earlier partial pass, fixed in M22:
      // decompiled/e.java's dispatcher gates these two on `f`
      // (hotbarContext, cycled by the `*` key), not `aq`
      // (hotbarActionSet, the numeric-hotkey selector) -- confirmed by
      // direct comparison of e.java's m(Graphics) body.
      if (hotbarContext == 1 && !this.player.hasAilment(3)) {
         this.paintMinimapZoomedOut(g);
      }

      if (hotbarContext == 2 && !this.player.hasAilment(3)) {
         this.paintMinimapNormal(g);
      }
   }

   private void paintPopupText(Graphics g) {
      g.setColor(16777215);
      g.drawString(popupMessageText, 60, 10, 17);
   }

   // ---- Unread rendering internals below: signatures only, not yet
   // transcribed from decompiled/e.java. See this file's header comment. ----

   // M21 (phase 3 port session): fully transcribed from decompiled/e.java's
   // j(Graphics) -- confirmed by structure/field access (wallSegmentTable,
   // player.corridorView via Dungeon.viewGridAt, Player.hasAilment(3)/(4))
   // to be dawnstar's own paintCorridorWalls() equivalent: draws the
   // corridor floor, then per forward-visibility step (0-9, forward scan
   // then a mirrored backward scan), the nearest wall segment found by
   // testing wallSegmentTable's candidate {cmd,column,dx,dy} offsets in
   // order and breaking on the first occluding tile.
   //
   // Two confirmed, genuine simplifications vs. dawnstar's own
   // paintCorridorWalls() -- not gaps in this transcription, verified by
   // reading this method and its two helpers (drawWallSegment/
   // resolveWallFrame, below) in full: (1) only ONE wall bit is ever
   // tested (bit 1, plain wall) -- there is no dawnstar-style bit-64
   // "gate/edge" branch anywhere in this method at all; (2) there is no
   // per-dungeon-number texture switch either -- floorTexture/wallTexture
   // are each a SINGLE shared Image (matching this class's own field
   // declarations, `static Image floorTexture; static Image wallTexture;`
   // -- no separate ice/plain/gate Image fields the way dawnstar has 5).
   // **Resolved by phase-3 M24:** floorTexture/wallTexture are plain
   // MIDP-native Image resources, NOT M7's RawImage/`.cus` format --
   // confirmed directly by ESGame.java's own asset-loading call sites
   // (`GameCanvas.floorTexture = this.createImage("floor3.png");`/
   // `GameCanvas.wallTexture = this.createImage("newwallsnok.png");`).
   // M24's own decoded-PNG smoke test cross-checks this against the real
   // files too: floor3.png is exactly 36px wide per the `col * 36` tiling
   // this method's own floor-drawing loop uses, and newwallsnok.png is
   // exactly 144px wide -- 8 real 18px-wide frames, matching
   // drawWallSegment()'s own "frame > 7 mirrors frame-8" logic exactly
   // (8 physical frames covering a logical 0-15 frame range via
   // mirroring, not 16 separately stored frames).
   //
   // **NOTE, NOT fixed here:** `paintFloor()` (still a stub immediately
   // below) is called separately by paintGameView() right after this
   // method -- but this method ALREADY draws the floor itself (the
   // ailment-3/4-gated block below). That makes paintFloor()'s existing
   // "(was e.java's b(Graphics))" placeholder mapping look wrong:
   // decompiled `b(Graphics)`'s real body reads from `player.ad`-shaped
   // data at specific index ranges (8-12, 4-6, 1), which looks far more
   // like an object/visibleObjects renderer (paintObjects()'s real
   // counterpart) than a floor painter. This pass only confirms
   // paintWalls() itself -- re-identifying every other still-stubbed
   // paint method's real decompiled counterpart (there are ~14 more) is a
   // separate, larger follow-up pass, same as this class's own header
   // comment already calls for.
   private void paintWalls(Graphics g) {
      Dungeon dungeon = this.player.currentDungeon();
      byte[][] view = this.player.corridorView;

      if (!this.player.hasAilment(3)) {
         if (this.player.hasAilment(4)) {
            g.setColor(10485760);
            g.fillRect(0, 0, this.getWidth(), floorTexture.getHeight());
         } else {
            for (int col = 0; col < 5; col++) {
               g.drawImage(floorTexture, col * 36, 0, 20);
            }
         }
      }

      for (int step = 0; step < 5; step++) {
         int x = step * 18;

         for (int row = 0; row < 6; row++) {
            int cmd = wallSegmentTable[step][row][0];
            int column = wallSegmentTable[step][row][1];
            int dx = wallSegmentTable[step][row][2];
            int dy = wallSegmentTable[step][row][3];
            if (Util.testBit((byte)1, dungeon.viewGridAt(dx, dy, view))) {
               int frame = this.resolveWallFrame(cmd, column, -1);
               this.drawWallSegment(g, frame, x);
               break;
            }
         }
      }

      for (int step = 5; step < 10; step++) {
         int x = step * 18;

         for (int row = 0; row < 6; row++) {
            int cmd = wallSegmentTable[9 - step][row][0];
            int column = wallSegmentTable[9 - step][row][1];
            int dx = -wallSegmentTable[9 - step][row][2];
            int dy = wallSegmentTable[9 - step][row][3];
            if (Util.testBit((byte)1, dungeon.viewGridAt(dx, dy, view))) {
               int frame = this.resolveWallFrame(cmd, column, 1);
               this.drawWallSegment(g, frame, x);
               break;
            }
         }
      }
   }

   // Draws one 18px-wide wall column from the single shared wallTexture
   // spritesheet: frames 0-7 draw directly at a `frame`-th 18px offset
   // into it; frames 8-15 draw the SAME spritesheet horizontally mirrored
   // (Nokia DirectGraphics' flag 8192 = FLIP_HORIZONTAL, i.e.
   // TRANS_MIRROR) at offset `frame-8` -- there is no second, separately-
   // stored mirrored-frame image. Unlike dawnstar's own drawWallSegment(),
   // there is no wallType parameter (no ice/plain or gate/wall texture
   // switch -- see paintWalls()'s own comment) and no wallDrawnNear/
   // wallDrawnMid dedup state either -- confirmed by reading this
   // method's full body directly, not a simplification made in this port.
   private void drawWallSegment(Graphics g, int frame, int x) {
      g.setClip(x, 0, 18, this.getHeight());
      if (frame > 7) {
         int mirroredFrame = frame - 8;
         DirectGraphics dg = DirectUtils.getDirectGraphics(g);
         dg.drawImage(wallTexture, x - mirroredFrame * 18, 0, 20, 8192);
      } else {
         g.drawImage(wallTexture, x - frame * 18, 0, 20);
      }

      g.setClip(0, 0, this.getWidth(), this.getHeight());
   }

   // cmd==12: pass `column` straight through as the frame index. Any
   // other cmd (11, in wallSegmentTable): frame is 8+column (forward
   // scan, side==-1) or 7-column (mirrored/backward scan, side==1) -- the
   // exact geometric meaning of the two command codes isn't pinned down
   // further (same open question dawnstar's own CORRIDOR_WALL_TABLE
   // comment already flags for its identical {cmd,column} shape),
   // preserved exactly as found.
   private int resolveWallFrame(int cmd, int column, int side) {
      if (cmd == 12) {
         return column;
      } else {
         return side == -1 ? 8 + column : 7 - column;
      }
   }

   // M22: fully transcribed from decompiled/e.java's b(Graphics) --
   // see this file's header comment for why this replaces the earlier
   // pass's wrong "paintFloor()" name/mapping. Draws visible chests
   // (8-byte records) and dropped items (7-byte records, same length
   // M17/M18 already confirmed) from Player.visibleObjects at the same
   // three slot zones paintMonsters() below also uses (near=slot 1,
   // mid=slots 4-6, far=slots 8-12).
   private void paintObjects(Graphics g) {
      for (int slot = 8; slot <= 12; slot++) {
         Object entry = Player.visibleObjects.elementAt(slot);
         if (entry instanceof byte[]) {
            byte[] record = (byte[])entry;
            if (record.length == 8 || record.length == 7) {
               this.renderObjectAt(g, record, slot);
            }
         }
      }

      for (int slot = 4; slot <= 6; slot++) {
         Object entry = Player.visibleObjects.elementAt(slot);
         if (entry instanceof byte[]) {
            byte[] record = (byte[])entry;
            if (record.length == 8 || record.length == 7) {
               this.renderObjectAt(g, record, slot);
            }
         }
      }

      Object entry = Player.visibleObjects.elementAt(1);
      if (entry instanceof byte[]) {
         byte[] record = (byte[])entry;
         if (record.length == 8 || record.length == 7) {
            this.renderObjectAt(g, record, 1);
         }
      }
   }

   // slot==1 (near): renderObjectNear. slots 4-6 (mid): renderObjectMid.
   // slots 8-12 (far): renderObjectFar. Byte-for-byte from e.java's
   // a(Graphics,byte[],int).
   private void renderObjectAt(Graphics g, byte[] record, int slot) {
      if (slot == 1) {
         this.renderObjectNear(g, record, false);
      } else if (slot >= 4 && slot <= 6) {
         this.renderObjectMid(g, record, false, slot);
      } else if (slot >= 8 && slot <= 12) {
         this.renderObjectFar(g, record, false, slot);
      }
   }

   // TODO: real trigger condition, not just "false". Only ever true for
   // 7-byte (dropped-item) records with byte[6] bit 2 (value 4) set --
   // a DIFFERENT bit of the same byte M17 already confirmed bit 1
   // (value 2) of for gift-points eligibility, so this is genuinely a
   // second, separate flag in that byte, not a collision with M17's own
   // finding. Named for the sprite array it selects (crystalImages);
   // its real in-game meaning (a glowing/special dropped item?) is not
   // confirmed. Byte-for-byte from e.java's static a(byte[]).
   private static boolean hasCrystalGlow(byte[] record) {
      return record.length == 7 && (record[6] & 4) != 0;
   }

   // Byte-for-byte from e.java's a(Graphics,byte[],boolean) -- the
   // `crystalGlow` param is always passed false from renderObjectAt()
   // above (hasCrystalGlow() is never actually called at any real call
   // site in the decompiled source, only declared -- preserved exactly
   // as found, not "fixed" to call it).
   private void renderObjectNear(Graphics g, byte[] record, boolean crystalGlow) {
      if (crystalGlow) {
         this.drawRawImageFull(g, crystalImages[0], 45, 65);
      } else {
         int x = 60;
         int y = 94;
         if (record.length == 8) {
            this.drawRawImageFull(g, chestImages[0], x, y);
         } else if (record.length == 7) {
            y += 14;
            this.drawRawImageFull(g, bagImages[0], x, y);
         }
      }
   }

   // Byte-for-byte from e.java's b(Graphics,byte[],boolean,int).
   private void renderObjectMid(Graphics g, byte[] record, boolean crystalGlow, int slot) {
      int x = 0;
      int y = 0;
      boolean special = hasCrystalGlow(record);
      switch (slot) {
         case 4:
            x = 14;
            y = 80;
            if (special) {
               x = 14;
               y = 55;
            }
            break;
         case 5:
            x = 68;
            y = 80;
            if (special) {
               x = 73;
               y = 55;
            } else if (record.length == 7) {
               x = 73;
               y = 80;
            }
            break;
         case 6:
            x = 122;
            y = 80;
            if (special) {
               x = 125;
               y = 55;
            } else if (record.length == 7) {
               x = 132;
               y = 80;
            }
      }

      if (special) {
         y += 13;
         this.drawRawImageFull(g, crystalImages[1], x, y);
      } else if (record.length == 8) {
         y += 17;
         this.drawRawImageFull(g, chestImages[1], x, y);
      } else if (record.length == 7) {
         y += 20;
         this.drawRawImageFull(g, bagImages[1], x, y);
      }
   }

   // Byte-for-byte from e.java's a(Graphics,byte[],boolean,int).
   private void renderObjectFar(Graphics g, byte[] record, boolean crystalGlow, int slot) {
      int x = 0;
      int y = 0;
      boolean special = hasCrystalGlow(record);
      switch (slot) {
         case 8:
            x = 10;
            y = 59;
            if (special) {
               x = 10;
               y = 52;
            }
            break;
         case 9:
            x = 44;
            y = 59;
            if (special) {
               x = 44;
               y = 52;
            }
            break;
         case 10:
            x = 79;
            y = 59;
            if (special) {
               x = 79;
               y = 52;
            }
            break;
         case 11:
            x = 112;
            y = 59;
            if (special) {
               x = 112;
               y = 52;
            }
            break;
         case 12:
            x = 146;
            y = 59;
            if (special) {
               x = 146;
               y = 52;
            }
      }

      if (special) {
         y += 20;
         this.drawRawImageFull(g, crystalImages[2], x, y);
      } else if (record.length == 8) {
         y += 28;
         this.drawRawImageFull(g, chestImages[2], x, y);
      } else if (record.length == 7) {
         y += 28;
         this.drawRawImageFull(g, bagImages[2], x, y);
      }
   }

   // Simple, unclipped single-frame RawImage blit -- byte-for-byte from
   // e.java's final method, a(Graphics,g,int,int) (was the very last
   // method in the file, no clip/frame-slicing, unlike
   // drawRawImageFrame() below).
   private void drawRawImageFull(Graphics g, RawImage img, int x, int y) {
      DirectGraphics dg = DirectUtils.getDirectGraphics(g);
      dg.drawPixels(img.pixels, true, 0, img.widthAgain, x, y, img.width, img.height, 0, 4444);
   }

   // M22: fully transcribed from decompiled/e.java's g(Graphics) -- the
   // mapping/name were already right, only the body was missing. Loops
   // the same three slot zones paintObjects() above uses; a Monster's
   // live 28-byte record (M14/M20's confirmed layout: record[2] ==
   // typeIndex) triggers a monster sprite render when record[6] != 0
   // (see this file's header comment on what that may really mean); a
   // String "W" entry in the same Vector slot triggers a Warden-icon
   // render instead -- by reusing renderMonsterFarZoneSprite()/
   // renderMonsterMidZoneSprite() below directly with the LITERAL row
   // constants 32/31, i.e. the exact same rows monsterFarZoneRow(41)/
   // monsterMidZoneRow(41) resolve to for the level-37 type-41
   // "roaming" monster (M19) -- the Warden visually reuses that
   // monster's far/mid sprite rows rather than having its own, per
   // e.java's own literal-constant call sites (not a row-lookup call).
   private void paintMonsters(Graphics g) {
      unconfirmed_A = false;

      for (int slot = 8; slot <= 12; slot++) {
         Object entry = Player.visibleObjects.elementAt(slot);
         if (entry instanceof byte[]) {
            byte[] record = (byte[])entry;
            if (record.length == 28 && record[6] != 0) {
               unconfirmed_A = true;
               this.renderMonsterSpriteForSlot(g, record[2], slot);
            }
         } else if (entry instanceof String) {
            if (((String)entry).equals("W")) {
               this.renderMonsterFarZoneSprite(g, 32, slot);
            }
         }
      }

      for (int slot = 4; slot <= 6; slot++) {
         Object entry = Player.visibleObjects.elementAt(slot);
         if (entry instanceof byte[]) {
            byte[] record = (byte[])entry;
            if (record.length == 28 && record[6] != 0) {
               unconfirmed_A = true;
               this.renderMonsterSpriteForSlot(g, record[2], slot);
            }
         } else if (entry instanceof String) {
            if (((String)entry).equals("W")) {
               this.renderMonsterMidZoneSprite(g, 31, slot);
            }
         }
      }

      Object entry = Player.visibleObjects.elementAt(1);
      if (entry instanceof byte[]) {
         byte[] record = (byte[])entry;
         if (record.length == 28 && record[6] != 0) {
            unconfirmed_A = true;
            this.renderMonsterSpriteForSlot(g, record[2], 1);
         }
      }
      // else if (entry instanceof String): e.java's own branch here is
      // empty (an `else if` with no body) -- preserved exactly.
   }

   // Byte-for-byte from e.java's c(Graphics,int,int): dispatches to one
   // of three zone-specific renderers by slot, then resets the clip
   // region each of them narrows via drawRawImageFrame() below.
   private void renderMonsterSpriteForSlot(Graphics g, int typeIndex, int slot) {
      if (slot == 1) {
         this.renderMonsterNearSprite(g, typeIndex);
      } else if (slot >= 4 && slot <= 6) {
         this.renderMonsterMidZoneSprite(g, monsterMidZoneRow(typeIndex), slot);
      } else if (slot >= 8 && slot <= 12) {
         this.renderMonsterFarZoneSprite(g, monsterFarZoneRow(typeIndex), slot);
      }

      g.setClip(0, 0, this.getWidth(), this.getHeight());
   }

   // typeIndex range -> monsterImages row for the mid zone (slots 4-6).
   // Byte-for-byte from e.java's c(int).
   private static int monsterMidZoneRow(int typeIndex) {
      if (typeIndex >= 1 && typeIndex <= 5) {
         return 5;
      } else if (typeIndex >= 6 && typeIndex <= 10) {
         return 12;
      } else if (typeIndex >= 11 && typeIndex <= 25) {
         return 19;
      } else if (typeIndex >= 26 && typeIndex <= 40) {
         return 26;
      } else {
         return typeIndex == 41 ? 31 : -1;
      }
   }

   // typeIndex range -> monsterImages row for the far zone (slots
   // 8-12). Byte-for-byte from e.java's a(int).
   private static int monsterFarZoneRow(int typeIndex) {
      if (typeIndex >= 1 && typeIndex <= 5) {
         return 6;
      } else if (typeIndex >= 6 && typeIndex <= 10) {
         return 13;
      } else if (typeIndex >= 11 && typeIndex <= 25) {
         return 20;
      } else if (typeIndex >= 26 && typeIndex <= 40) {
         return 27;
      } else {
         return typeIndex == 41 ? 32 : -1;
      }
   }

   // typeIndex range -> unconfirmedTable_ae row for the near zone (slot
   // 1). Byte-for-byte from e.java's b(int).
   private static int monsterNearZoneRow(int typeIndex) {
      if (typeIndex >= 1 && typeIndex <= 5) {
         return 0;
      } else if (typeIndex >= 6 && typeIndex <= 10) {
         return 1;
      } else if (typeIndex >= 11 && typeIndex <= 25) {
         return 2;
      } else {
         return typeIndex >= 26 && typeIndex <= 40 ? 3 : -1;
      }
   }

   // Byte-for-byte from e.java's c(Graphics,int): the near-zone (slot 1)
   // entry point, always with no column override.
   private void renderMonsterNearSprite(Graphics g, int typeIndex) {
      this.renderMonsterOrIconSprite(g, typeIndex, -1);
   }

   // The near-zone monster sprite renderer, reused for paintUnknown_b()'s
   // small NPC-icon indices too (see that method's own comment) --
   // byte-for-byte from e.java's e(Graphics,int,int). typeIndex==41 (the
   // level-37 "roaming" monster M19 already confirmed) reuses the Warden
   // compass icon renderer instead of the normal sprite table.
   // `columnOverride` >= 0 substitutes for unconfirmedTable_a's own
   // secondary-sprite frame value (its column 1); -1 (every monster call
   // site -- via renderMonsterNearSprite() above) means "use the
   // table's own value"; only paintUnknown_b()'s NPC-icon call sites
   // ever pass a real override.
   private void renderMonsterOrIconSprite(Graphics g, int typeIndex, int columnOverride) {
      if (typeIndex == 41) {
         this.renderWardenCompassIcon(g, 2);
      } else {
         int row = monsterNearZoneRow(typeIndex);
         if (row >= 0) {
            byte baseX = unconfirmedTable_ae[row][2];
            byte baseY = unconfirmedTable_ae[row][3];
            byte primaryImageIndex = unconfirmedTable_ae[row][4];
            byte primaryFrameCount = unconfirmedTable_ae[row][5];
            int overlayBaseX = baseX + unconfirmedTable_ae[row][6];
            int overlayBaseY = baseY + unconfirmedTable_ae[row][7];
            byte secondaryImageIndex = unconfirmedTable_ae[row][8];
            byte secondaryFrameCount = unconfirmedTable_ae[row][9];
            boolean hasSecondary = secondaryImageIndex >= 0;

            byte primaryFrame = unconfirmedTable_a[typeIndex - 1][0];
            int secondaryFrame = unconfirmedTable_a[typeIndex - 1][1];
            if (columnOverride >= 0) {
               secondaryFrame = columnOverride;
            }

            boolean overlay1 = unconfirmedTable_J[typeIndex - 1][0];
            boolean overlay2 = unconfirmedTable_J[typeIndex - 1][1];
            boolean overlay3 = unconfirmedTable_J[typeIndex - 1][2];
            boolean overlay4 = unconfirmedTable_J[typeIndex - 1][3];
            this.drawRawImageFrame(g, monsterImages[primaryImageIndex], primaryFrame, primaryFrameCount, baseX, baseY);
            if (hasSecondary) {
               this.drawRawImageFrame(g, monsterImages[secondaryImageIndex], secondaryFrame, secondaryFrameCount, overlayBaseX, overlayBaseY);
            }

            if (overlay1) {
               int x = baseX + unconfirmedTable_ae[row][10];
               int y = baseY + unconfirmedTable_ae[row][11];
               byte img = unconfirmedTable_ae[row][12];
               this.drawRawImageFrame(g, monsterImages[img], 0, 1, x, y);
            }

            if (overlay2) {
               int x = baseX + unconfirmedTable_ae[row][13];
               int y = baseY + unconfirmedTable_ae[row][14];
               byte img = unconfirmedTable_ae[row][15];
               this.drawRawImageFrame(g, monsterImages[img], 0, 1, x, y);
            }

            if (overlay3) {
               int x = baseX + unconfirmedTable_ae[row][16];
               int y = baseY + unconfirmedTable_ae[row][17];
               byte img = unconfirmedTable_ae[row][18];
               this.drawRawImageFrame(g, monsterImages[img], 0, 1, x, y);
            }

            if (overlay4) {
               int x = baseX + unconfirmedTable_ae[row][19];
               int y = baseY + unconfirmedTable_ae[row][20];
               byte img = unconfirmedTable_ae[row][21];
               this.drawRawImageFrame(g, monsterImages[img], 0, 1, x, y);
            }
         }
      }
   }

   // Byte-for-byte from e.java's d(Graphics,int,int) -- wraps
   // drawMonsterZoneFrame() below with frame 0 of 1 (a single static
   // frame, no animation, unlike the near-zone renderer above).
   private void renderMonsterMidZoneSprite(Graphics g, int spriteRow, int slot) {
      this.drawMonsterZoneFrame(g, spriteRow, slot, 0, 1);
   }

   // Byte-for-byte from e.java's a(Graphics,int,int,int,int): the mid
   // zone's fixed per-slot screen position table.
   private void drawMonsterZoneFrame(Graphics g, int spriteRow, int slot, int frame, int frameCount) {
      int x = 0;
      int y = 0;
      switch (slot) {
         case 4:
            x = 10;
            y = 38;
            break;
         case 5:
            x = 62;
            y = 38;
            break;
         case 6:
            x = 112;
            y = 38;
      }

      this.drawRawImageFrame(g, monsterImages[spriteRow], frame, frameCount, x, y);
   }

   // Byte-for-byte from e.java's b(Graphics,int,int): the far zone draws
   // straight through DirectGraphics, not via drawRawImageFrame() (no
   // per-frame width slicing -- the far-zone sprites are single-frame).
   private void renderMonsterFarZoneSprite(Graphics g, int spriteRow, int slot) {
      int x = 0;
      int y = 0;
      switch (slot) {
         case 8:
            x = 10;
            y = 44;
            break;
         case 9:
            x = 44;
            y = 44;
            break;
         case 10:
            x = 79;
            y = 44;
            break;
         case 11:
            x = 112;
            y = 44;
            break;
         case 12:
            x = 146;
            y = 44;
      }

      DirectGraphics dg = DirectUtils.getDirectGraphics(g);
      RawImage img = monsterImages[spriteRow];
      dg.drawPixels(img.pixels, true, 0, img.widthAgain, x, y, img.width, img.height, 0, 4444);
   }

   // M22: fully transcribed from decompiled/e.java's a(Graphics) -- see
   // this file's header comment for why this replaces the earlier
   // pass's wrong "paintObjects()" name/mapping. Three colored bars
   // (HP/Magicka/Fatigue, matching coreStats indices 2-3/4-5/6-7) scaled
   // to a 38px-wide track via Player.effectiveStat()/coreStats.
   private void paintStatusBars(Graphics g) {
      g.setColor(16776960);
      g.fillRect(5, 130, 40, 7);
      g.fillRect(5, 138, 40, 7);
      g.fillRect(5, 146, 40, 7);
      g.setColor(16711680);
      int width = this.player.effectiveStat(2) * 38 / this.player.coreStats[3];
      g.fillRect(6, 131, width, 5);
      g.setColor(65280);
      width = this.player.effectiveStat(4) * 38 / this.player.coreStats[5];
      g.fillRect(6, 139, width, 5);
      g.setColor(255);
      width = this.player.effectiveStat(6) * 38 / this.player.coreStats[7];
      if (width > 40) {
         width = 40;
      }

      g.fillRect(6, 147, width, 5);
   }

   // M22: fully transcribed from decompiled/e.java's d(Graphics) -- the
   // mapping/name were already right, only the body was missing. Draws
   // the bottom HUD panel (a rounded-rect backdrop) then one of three
   // hotkey-icon/glyph rows selected by resolveHudIconSet() below.
   private void paintHud(Graphics g) {
      g.setFont(smallFont);
      g.setClip(0, 0, this.getWidth(), this.getHeight());
      g.setColor(0);
      g.fillRect(0, 156, this.getWidth(), 52);
      g.setColor(13080935);
      g.fillRoundRect(2, 158, this.getWidth() - 4, 48, 5, 5);
      g.setColor(0);
      int iconSet = this.resolveHudIconSet();
      hotbarActionSet = iconSet;
      if (iconSet == 0) {
         g.drawImage(hotbarIcons[1], 14, 174, 20);
         g.drawImage(hotbarIcons[2], 62, 174, 20);
         g.drawImage(hotbarIcons[3], 104, 174, 20);
         g.drawImage(hotbarIcons[5], 144, 174, 20);
         g.drawChar(hotbarKeyGlyphs[1], 5, 180, 20);
         g.drawChar(hotbarKeyGlyphs[2], 53, 180, 20);
         g.drawChar(hotbarKeyGlyphs[3], 96, 180, 20);
         g.drawChar(hotbarKeyGlyphs[5], 135, 180, 20);
      } else if (iconSet == 1) {
         g.drawImage(hotbarIcons[0], 14, 174, 20);
         g.drawImage(hotbarIcons[1], 62, 174, 20);
         g.drawImage(hotbarIcons[2], 104, 174, 20);
         g.drawImage(hotbarIcons[3], 144, 174, 20);
         g.drawChar(hotbarKeyGlyphs[0], 5, 180, 20);
         g.drawChar(hotbarKeyGlyphs[1], 53, 180, 20);
         g.drawChar(hotbarKeyGlyphs[2], 96, 180, 20);
         g.drawChar(hotbarKeyGlyphs[3], 135, 180, 20);
      } else if (iconSet == 2) {
         g.drawImage(hotbarIcons[1], 14, 174, 20);
         g.drawImage(hotbarIcons[2], 62, 174, 20);
         g.drawImage(hotbarIcons[3], 104, 174, 20);
         g.drawImage(hotbarIcons[4], 144, 174, 20);
         g.drawChar(hotbarKeyGlyphs[1], 5, 180, 20);
         g.drawChar(hotbarKeyGlyphs[2], 53, 180, 20);
         g.drawChar(hotbarKeyGlyphs[3], 96, 180, 20);
         g.drawChar(hotbarKeyGlyphs[4], 135, 180, 20);
      }
   }

   // Byte-for-byte from e.java's j(): resolves which of paintHud()'s
   // three icon rows to show (also stashed into hotbarActionSet, which
   // keyPressed() then reads for numeric-hotkey dispatch -- so this
   // recomputes that selector every frame from other flags, it isn't
   // itself set by a key).
   private int resolveHudIconSet() {
      if (unconfirmed_aa) {
         return 1;
      } else if (unconfirmed_m || unconfirmed_R) {
         return 2;
      } else {
         return unconfirmed_W && !this.isNpcDialogueDue() ? 2 : 0;
      }
   }

   // M22: fully transcribed from decompiled/e.java's l(Graphics) -- see
   // this file's header comment for why this REPLACES the earlier
   // pass's wrong "paintMessagePopup()" (which was actually
   // paintFlashOverlays() below). This is the real message-popup box:
   // a 2-line rounded rect showing messageLines[0]/[1] while
   // unconfirmed_ad is set (by showMessage(), confirmed and transcribed
   // -- phase-3 port M30).
   private void paintMessagePopup(Graphics g) {
      if (unconfirmed_ad) {
         g.setColor(13080935);
         g.fillRoundRect(96, 118, 75, 35, 5, 5);
         g.setFont(smallFont);
         g.setColor(0);
         g.drawString(messageLines[0], 100, 122, 20);
         if (messageLines.length > 1) {
            g.drawString(messageLines[1], 100, 134, 20);
         }
      }
   }

   // M22: fully transcribed from decompiled/e.java's e(Graphics) -- see
   // this file's header comment for why this replaces the earlier
   // pass's wrong "paintMessagePopup()" name/mapping. Three independent
   // one-shot flash overlays (monster-hit/spell-hit/self-spell-hit),
   // each self-clearing its own flag once drawn, at a small random
   // jittered position via Util.randomInt() (byte-for-byte the same
   // random-offset idiom paintWalls()'s own dungeon generation cousins
   // use elsewhere in this project).
   private void paintFlashOverlays(Graphics g) {
      if (unconfirmed_S) {
         int x = 40 + Util.randomInt(30);
         int y = 50 + Util.randomInt(20);
         g.drawImage(effectImages[0], x, y, 20);
         unconfirmed_S = false;
      }

      if (unconfirmed_ao) {
         int x = 40 + Util.randomInt(30);
         int y = 50 + Util.randomInt(22);
         g.drawImage(effectImages[1], x, y, 20);
         unconfirmed_ao = false;
      }

      if (unconfirmed_am) {
         int x = 50 + Util.randomInt(2);
         int y = 80 + Util.randomInt(2);
         g.drawImage(effectImages[2], x, y, 20);
         unconfirmed_am = false;
      }
   }

   // M22: fully transcribed from decompiled/e.java's b(Graphics,int) --
   // the mapping/name were already right, only the body was missing.
   // `stat` is Player.questShopAtPendingTile()'s return value (see
   // paintGameView()'s own call site). Cases 0-5 reuse
   // renderMonsterOrIconSprite() (see that method's own comment) with
   // small literal indices instead of real monster typeIndexes -- an
   // NPC/shop-portrait sprite sheet apparently laid out in the same
   // row-index space as the monster table, not independently confirmed
   // beyond that both fall in valid table ranges. Case 6 shows the
   // Warden compass icon instead, scaled by how many times the Warden
   // has visited (Shop.wardenVisitCount, capped at 3).
   private void paintUnknown_b(Graphics g, int stat) {
      switch (stat) {
         case 0:
            this.renderMonsterOrIconSprite(g, 1, 1);
            break;
         case 1:
            this.renderMonsterOrIconSprite(g, 6, 1);
            break;
         case 2:
            this.renderMonsterOrIconSprite(g, 7, 1);
            break;
         case 3:
            this.renderMonsterOrIconSprite(g, 2, 1);
            break;
         case 4:
            this.renderMonsterOrIconSprite(g, 3, 2);
            break;
         case 5:
            this.renderMonsterOrIconSprite(g, 8, 0);
            break;
         case 6:
            int tier = Math.min(Shop.wardenVisitCount, 3) - 1;
            this.renderWardenCompassIcon(g, tier);
      }

      g.setClip(0, 0, this.getWidth(), this.getHeight());
   }

   // Byte-for-byte from e.java's a(Graphics,int): the Warden's own
   // compass-relative icon, drawn as a normal frame plus a horizontally
   // mirrored copy (the same DirectGraphics flag-8192 mirror trick
   // drawWallSegment() already uses) plus a small secondary badge.
   // monsterImages[28]/[29] are reused here (see this file's header
   // comment on the Warden/monster sprite-sheet overlap).
   private void renderWardenCompassIcon(Graphics g, int tier) {
      byte x = 15;
      byte y = 32;
      this.drawRawImageFrame(g, monsterImages[28], unconfirmedTable_o[tier][0], 1, x, y);
      int frameWidth = monsterImages[28].width();
      this.drawRawImageFrame(g, monsterImages[28], unconfirmedTable_o[tier][0], 1, x + frameWidth, y, 8192);
      this.drawRawImageFrame(g, monsterImages[29], unconfirmedTable_o[tier][1], 3, x + 45, y + -22);
   }

   // Single-frame-of-many RawImage blit, clipped to that frame's own
   // column -- byte-for-byte from e.java's a(Graphics,g,int,int,int,
   // int), which itself forwards to the 7-arg overload below with
   // transform 0 (no mirroring).
   private void drawRawImageFrame(Graphics g, RawImage img, int frame, int frameCount, int x, int y) {
      this.drawRawImageFrame(g, img, frame, frameCount, x, y, 0);
   }

   // Byte-for-byte from e.java's a(Graphics,g,int,int,int,int,int).
   private void drawRawImageFrame(Graphics g, RawImage img, int frame, int frameCount, int x, int y, int transform) {
      int frameWidth = img.width() / frameCount;
      int frameHeight = img.height();
      g.setClip(x, y, frameWidth, frameHeight);
      DirectGraphics dg = DirectUtils.getDirectGraphics(g);
      dg.drawPixels(img.pixels, true, 0, img.widthAgain, x - frame * frameWidth, y, img.width, img.height, transform, 4444);
   }

   // Confirmed (phase-3 port M32): byte-for-byte from decompiled/e.java's
   // q(). Refreshes minimapTileGrid from the player's own current
   // position/facing -- the zoomed-out minimap's own populate step,
   // paintMinimapZoomedOut()'s data source. Both real call sites (e.java
   // lines 1288/1693) live inside still-untranscribed tick-loop helpers
   // (`a(boolean)`/its own analog below) -- not reachable on its own
   // yet, same gap this file's other confirmed-but-unwired methods
   // already have (see showMessage()'s own M30 note).
   private void populateMinimapGrid() {
      this.player.currentDungeon().sampleSquareView7(this.player.tileX, this.player.tileY, this.player.facing,
         minimapTileGrid);
   }

   // Confirmed (phase-3 port M32): byte-for-byte from decompiled/e.java's
   // p(). Same role as populateMinimapGrid() above, for the normal-zoom
   // (17x17 visibleTileGrid) minimap instead.
   private void populateVisibleGrid() {
      this.player.currentDungeon().sampleSquareView17(this.player.tileX, this.player.tileY, this.player.facing,
         visibleTileGrid);
   }

   // M22: fully transcribed from decompiled/e.java's k(Graphics) -- see
   // this file's header comment for why this replaces the earlier
   // pass's wrong "paintHotbar1()" name/mapping (this is the zoomed-out,
   // 7x7 minimapTileGrid view, not a hotbar). Draws the compass glyph
   // for the player's own facing (Player.facing, NOT this class's own
   // same-named dead-state field), a black backdrop square, then the
   // grid itself via drawMinimapGrid() below.
   private void paintMinimapZoomedOut(Graphics g) {
      g.setFont(smallFont);
      g.setColor(16777215);
      g.drawChar(compassGlyphs[this.player.facing], 16, 10, 20);
      g.setColor(0);
      g.fillRect(10, 20, 23, 23);
      this.drawMinimapGrid(g, 10, 20, 7, 3, 1, minimapTileGrid);
   }

   // M22: fully transcribed from decompiled/e.java's h(Graphics) -- see
   // this file's header comment for why this replaces the earlier
   // pass's wrong "paintHotbar2()" name/mapping (the normal-zoom, 17x17
   // visibleTileGrid view). Same shape as paintMinimapZoomedOut() above,
   // larger cell size/backdrop, a different Font.
   private void paintMinimapNormal(Graphics g) {
      g.setFont(minimapFont);
      g.setColor(16777215);
      g.drawChar(compassGlyphs[this.player.facing], 58, 10, 20);
      byte size = 89;
      g.fillRect(15, 25, size, size);
      this.drawMinimapGrid(g, 15, 25, 17, 5, 2, visibleTileGrid);
   }

   // Draws a `gridSize`x`gridSize` occlusion grid as `cellPx`-square
   // colored cells (1=black/wall, 0=white/floor, else bit 2=red,
   // bit 4=blue, bit 8=cyan-ish -- unconfirmed per-bit meaning beyond
   // "not plain wall/floor"), plus a green marker at dead center
   // (the player's own tile). Byte-for-byte from e.java's
   // a(Graphics,int,int,int,int,int,byte[][]).
   private void drawMinimapGrid(Graphics g, int originX, int originY, int gridSize, int cellPx, int pixelOffset, byte[][] grid) {
      int center = gridSize / 2;

      for (int row = 0; row < gridSize; row++) {
         int py = originY + pixelOffset + row * cellPx;

         for (int col = 0; col < gridSize; col++) {
            int px = originX + pixelOffset + col * cellPx;
            if (grid[col][row] == 1) {
               g.setColor(0);
               g.fillRect(px, py, cellPx, cellPx);
            } else if (grid[col][row] == 0) {
               g.setColor(16777215);
               g.fillRect(px, py, cellPx, cellPx);
            } else if ((grid[col][row] & 2) != 0) {
               g.setColor(16711680);
               g.fillRect(px, py, cellPx, cellPx);
            } else if ((grid[col][row] & 4) != 0) {
               g.setColor(255);
               g.fillRect(px, py, cellPx, cellPx);
            } else if ((grid[col][row] & 8) != 0) {
               g.setColor(13369599);
               g.fillRect(px, py, cellPx, cellPx);
            }

            if (row == center && col == center) {
               g.setColor(65280);
               g.fillRect(px, py, cellPx, cellPx);
            }
         }
      }
   }

   // Confirmed (phase-3 port M30): byte-for-byte from decompiled/e.java's
   // a(String[],int). A message of priority `priority` only replaces
   // whatever's currently showing if it's strictly higher (equal or
   // lower is rejected while `priority >= 0`); a negative priority is
   // an "always show, max priority" override, stored as 10 rather than
   // the negative value itself. Every real call site immediately
   // follows a `true` return with `messageShownAt = now; unconfirmed_ad
   // = true;` (see this file's own header comment) -- not folded in
   // here, kept exactly as the original's own separate lines.
   private boolean showMessage(String[] lines, int priority) {
      if (priority <= messagePriority && priority >= 0) {
         return false;
      }

      messageLines = lines;
      messagePriority = priority < 0 ? 10 : priority;
      return true;
   }

   // Confirmed (phase-3 port M35): byte-for-byte from decompiled/e.java's
   // c(long). Per-tick ailment-timer countdowns: while each of 3 specific
   // ailments (Player.hasAilment(id), NOT Player.isEffectActive(id) --
   // decompiled/j.java's own k(int) checks ailmentMask directly, unlike
   // t(int)'s effectDurations-array countdown a much easier-to-confuse
   // similarly-shaped method) is currently active, its own dedicated
   // millisecond timer (Player.vampirismTimer/manaBurnTimer/
   // terrifiedTimer -- Monster.tick()'s own confirmed ailment-4/5
   // appliers, ../../docs/CLASS_MAP.md) counts down by `deltaMs`; once it
   // drops below 0, it's clamped to 0 and the ailment bit clears
   // (Util.clearBit(bitIndex, ailmentMask), bitIndex = ailmentId - 1,
   // same convention Player.hasAilment/cureRandomAilment already use).
   //
   // **A real, surprising coupling confirmed by reading this exact
   // method, not smoothed over:** ailment 7's own timer additionally
   // requires `unconfirmed_A` -- the SAME flag paintMonsters() (M22)
   // sets true only when it actually draws a real monster sprite that
   // same frame (never for the Warden), reset false at the top of that
   // method every call. So ailment 7's countdown only progresses on a
   // tick where a monster was ALSO just rendered -- a real dependency
   // between this port's paint and tick passes, not independently
   // confirmed anywhere else, and not "fixed" into a plain timer here.
   private void tickStatusCountdowns(long deltaMs) {
      if (this.player.hasAilment(4)) {
         this.player.vampirismTimer = (short)(this.player.vampirismTimer - deltaMs);
         if (this.player.vampirismTimer < 0) {
            this.player.vampirismTimer = 0;
            this.player.ailmentMask = (byte)Util.clearBit(3, this.player.ailmentMask);
         }
      }

      if (this.player.hasAilment(5)) {
         this.player.manaBurnTimer = (short)(this.player.manaBurnTimer - deltaMs);
         if (this.player.manaBurnTimer < 0) {
            this.player.manaBurnTimer = 0;
            this.player.ailmentMask = (byte)Util.clearBit(4, this.player.ailmentMask);
         }
      }

      if (this.player.hasAilment(7) && unconfirmed_A) {
         this.player.terrifiedTimer = (short)(this.player.terrifiedTimer - deltaMs);
         if (this.player.terrifiedTimer < 0) {
            this.player.terrifiedTimer = 0;
            this.player.ailmentMask = (byte)Util.clearBit(6, this.player.ailmentMask);
         }
      }
   }

   // Confirmed (phase-3 port M36): byte-for-byte from decompiled/e.java's
   // l(). REPLACES an earlier, less careful pass's wrong "and the
   // Warden-visit gate" header note -- there is no Shop/Warden reference
   // anywhere in this method's real body at all; that claim looks like a
   // guess made before the method was actually read.
   //
   // Three independent per-second mechanics:
   //  1. EVERY currently counting-down effectDurations[] slot (all 25,
   //     not just the 3 specific ailments tickStatusCountdowns's own
   //     millisecond timers separately track) decrements by 1. When
   //     slot 5 (effect id 6) reaches exactly zero THIS tick, item 109
   //     ("daedric weapon" per the original's own debug println, kept
   //     verbatim) is located by its equipped-slot search and removed
   //     outright (removeInventorySlot(), NOT the dropInventoryItem()
   //     path -- the weapon simply vanishes when its own temporary
   //     effect wears off, no world copy is dropped).
   //  2. hasAilment(4) ("vampirism") drains 2% of maxHP from HP every
   //     second.
   //  3. hasAilment(5) ("mana burn") regenerates 10% of maxMagicka into
   //     Magicka every second; the moment Magicka reaches or exceeds
   //     its own max, it resets to EXACTLY ZERO (not clamped to max)
   //     and HP takes a 10%-of-maxMagicka hit instead -- a real,
   //     punishing overflow-and-burn mechanic, not a clamp bug.
   //
   // **A 4th piece, transcribed here for a complete record but a
   // confirmed, provably inert dead write, NOT reproduced in the C++
   // port (see docs/PORT_ROADMAP.md):** a loop over every monster
   // currently registered on the player's own level decrements each
   // one's scratch[7] (clearing scratch[6] once it hits zero) -- but
   // `Monster.fromBytesShared()` (was decompiled/d.java's own static
   // `a(byte[])`) COPIES bytes into a shared scratch Monster instance
   // rather than aliasing the registry's own stored byte[] record, and
   // this loop never calls `store()` afterward. So every mutation here
   // is thrown away the instant the loop moves to the next monster --
   // confirmed by reading `Monster.fromBytesShared()`'s own byte-by-byte
   // copy directly, not assumed from the missing store() call alone.
   private void tickPerSecond() {
      for (int i = 0; i < 25; i++) {
         if (this.player.effectDurations[i] > 0) {
            this.player.effectDurations[i]--;
            if (this.player.effectDurations[i] <= 0) {
               this.player.effectDurations[i] = 0;
               if (i == 5) {
                  System.out.println("Removing daedric weapon!");
                  int slot = this.player.findEquippedSlotForItem(109);
                  System.out.println("Removing daedric weapon!: index is " + slot);
                  this.player.removeInventorySlot(slot);
               }
            }
         }
      }

      if (this.player.hasAilment(4)) {
         int drain = 2 * this.player.coreStats[3] / 100;
         drain = Math.max(drain, 0);
         this.player.coreStats[2] = (short)(this.player.coreStats[2] - drain);
      }

      if (this.player.hasAilment(5)) {
         int regen = this.player.coreStats[5] / 10;
         this.player.coreStats[4] = (short)(this.player.coreStats[4] + regen);
         if (this.player.coreStats[4] >= this.player.coreStats[5]) {
            this.player.coreStats[4] = 0;
            int burn = this.player.coreStats[5] / 10;
            this.player.coreStats[2] = (short)(this.player.coreStats[2] - burn);
         }
      }

      // Dead write over the per-level monster registry -- see this
      // method's own header comment above. Not transcribed: it would
      // just be `Dungeon.monstersOnThisLevel()`-shaped iteration
      // mutating a throwaway decoded copy with no observable effect
      // anywhere, matching this file's established "document, don't
      // mechanically reproduce, a provably dead branch" discipline.
   }

   // Stops the game thread (join()), starts a fresh one -- bodies ARE
   // transcribed (see below); only the paint internals above are stubs.
   void stopGameThread() {
      if (this.gameThread != null) {
         this.gameThreadRunning = true;
         if (this.gameThread.isAlive()) {
            System.out.println("Killing game thread");
            this.stopRequested = true;

            try {
               this.gameThread.join();
            } catch (Exception ignored) {
            }

            System.out.println("Done killing game thread");
         }

         this.gameThread = null;
         System.gc();
      }
   }

   // Clears threadStarted (was `b()`), resuming the real per-tick logic in
   // run()'s main loop instead of its 250ms sleep-only branch. Confirmed
   // callers: ESGame, right after returning to the game view from the
   // level-up/help/camp-dismiss screens.
   void resumeTicking() {
      this.threadStarted = false;
   }

   void startGameThread() {
      try {
         this.stopGameThread();
         this.gameThread = new Thread(this);
         this.gameThread.start();
         ESGame.debugLog("after starting game thread");
      } catch (Throwable t) {
         System.out.println(" start error:");
         t.printStackTrace();
         this.repaint();
         this.serviceRepaints();
      }
   }

   // Numeric keys 1-9,0 are hotbar/spell/item/camp/options hotkeys (mapping
   // depends on hotbarActionSet); '*' toggles the zoomed-out minimap
   // (hotbarContext 1<->2); arrow/game-action keys set pendingMoveDir
   // (strafeFlag set alongside numeric keys 4/6, which are dedicated
   // strafe-left/right hotkeys distinct from the arrow-key left/right
   // turn). Fully transcribed from decompiled/e.java.
   public void keyPressed(int key) {
      this.previousGameAction = this.gameAction;
      if (key == 49) {
         if (hotbarActionSet == 1) {
            unconfirmed_av = true;
         }
      } else if (key == 50) {
         this.pendingMoveDir = 1;
      } else if (key == 51) {
         unconfirmed_ap = true;
      } else if (key == 52) {
         this.strafeFlag = true;
         this.pendingMoveDir = 4;
      } else if (key == 53) {
         unconfirmed_U = true;
      } else if (key == 54) {
         this.strafeFlag = true;
         this.pendingMoveDir = 3;
      } else if (key == 55) {
         unconfirmed_Z = true;
      } else if (key == 56) {
         this.pendingMoveDir = 2;
      } else if (key == 57) {
         if (hotbarActionSet == 2) {
            unconfirmed_ay = true;
         }
      } else if (key == 48) {
         if (hotbarActionSet == 0) {
            unconfirmed_I = true;
         }
      } else if (key == 42) {
         hotbarContext++;
         if (hotbarContext > 2) {
            hotbarContext = 1;
         }
      } else {
         this.strafeFlag = false;
         this.gameAction = this.getGameAction(key);
         switch (this.gameAction) {
            case 1:
               this.pendingMoveDir = 1;
               break;
            case 2:
               this.pendingMoveDir = 4;
            case 3:
            case 4:
            default:
               break;
            case 5:
               this.pendingMoveDir = 3;
               break;
            case 6:
               this.pendingMoveDir = 2;
         }
      }
   }

   public void keyReleased(int key) {
      int action = this.getGameAction(key);
      this.previousGameAction = this.gameAction;
      this.gameAction = 0;
   }

   // Main tick loop: a steady 250ms cadence (matches dawnstar's own
   // GameCanvas.run() exactly). Drives the camping state machine
   // (campState 1=rolling for interruption with a 2.5s window, 2=safe
   // wait with a 5s window), the death/restart sequence (facing==3 "dead"
   // state, 5s before respawn -- reuses `facing` as the dead-state flag,
   // same field the paint dispatcher checks; note this conflates the
   // compass-facing field with a death-state flag, exactly as found, not
   // "cleaned up"), the Warden-visit check (`Shop.shouldWardenVisit`/
   // `Shop.wardenArrives`, see this file's header comment on the k/Shop
   // collision), an NPC-proximity dialogue trigger, per-tick/per-second
   // status ticks, and the repaint/frame-pacing tail. Fully transcribed
   // from decompiled/e.java; the methods it calls into
   // (tickStatusCountdowns/tickPerSecond/showMessage/paint* and several
   // Player methods) are themselves still TODO stubs above/on Player.
   public void run() {
      long frameStart = System.currentTimeMillis();
      long deltaMs = 0L;
      long secondAccumulator = 0L;

      try {
         while (this.gameThreadRunning) {
            boolean didTick = false;
            if (this.threadStarted) {
               try {
                  Thread.sleep(250L);
               } catch (Exception ignored) {
               }

               if (this.stopRequested) {
                  this.stopRequested = false;
                  return;
               }
            } else {
               boolean shouldRunTick = true;
               unconfirmed_at = false;
               System.gc();
               if (this.campState == 1) {
                  shouldRunTick = false;
                  if (frameStart - this.campRollAt > 2500L) {
                     if (this.rollCampInterrupted()) {
                        this.campState = 0;
                        this.player.currentDungeon().spawnAmbushMonsterNearPlayer(this.player);
                        this.campRollAt = 0L;
                        this.unconfirmed_v = true;
                        this.player.applyRestRecovery(false);
                        shouldRunTick = true;
                        if (this.showMessage(MSG_REST_DISTURBED, 1)) {
                           messageShownAt = frameStart;
                           unconfirmed_ad = true;
                        }
                     } else {
                        this.campState = 2;
                     }
                  }
               } else if (this.campState == 2) {
                  shouldRunTick = false;
                  if (frameStart - this.campRollAt > 5000L) {
                     this.campState = 0;
                     this.campRollAt = 0L;
                     this.unconfirmed_v = true;
                     this.player.applyRestRecovery(true);
                     if (this.showMessage(MSG_REST_COMPLETE, 1)) {
                        messageShownAt = frameStart;
                        unconfirmed_ad = true;
                     }

                     shouldRunTick = true;
                  }
               } else if (this.facing != 1) {
                  if (this.facing == 2) {
                     this.facing = 3;
                     unconfirmed_ad = false;
                     messagePriority = 0;
                  }

                  shouldRunTick = false;
                  if (frameStart - this.unconfirmed_s > 5000L) {
                     System.out.println("Restart after dead");
                     this.player.normalizeToMaxStats(this.player.coreStats);

                     for (int slot = this.player.inventoryCount - 1; slot >= 0; slot--) {
                        if (!this.player.isSlotEquipped(slot)) {
                           this.player.removeInventorySlot(slot);
                        }
                     }

                     this.player.resetState(this.player.classIndex, true);
                     this.unconfirmed_s = 0L;
                     this.facing = 1;
                     Shop.showSpecialGreeting = true;
                     this.unconfirmed_v = true;
                     unconfirmed_E = true;
                     shouldRunTick = true;
                     if (unconfirmed_E) {
                        String[] respawnMsg = null;
                        if (this.player.enteredNewLevelZone) {
                           respawnMsg = MSG_WARDENS_CAMP;
                        } else if (this.player.leftLevelZone) {
                           respawnMsg = MSG_OUTER_CAMP;
                        } else {
                           respawnMsg = this.player.currentDungeon().displayNames();
                        }

                        if (this.showMessage(respawnMsg, 1)) {
                           messageShownAt = System.currentTimeMillis();
                           unconfirmed_ad = true;
                        }

                        unconfirmed_E = false;
                     }
                  }
               }

               if (shouldRunTick) {
                  if (Shop.shouldWardenVisit(this.player.giftPointsFound)) {
                     Shop.wardenArrives();
                  }

                  if (this.isNpcDialogueDue() && Shop.wardenVisitCount > this.player.wardenLoreStep) {
                     String dialogueLine = Shop.dialogue(this.player, 6, -1, -1);
                     this.game.rumorsUI = this.game.newWardenSpeaksUI(dialogueLine);
                     didTick = true;
                  }

                  this.tickMonsterAI(frameStart);
                  this.tickStatusCountdowns_e(frameStart);
                  this.tickMovementAndAI(frameStart, deltaMs);
                  if (this.player.tryRankUpSkills()) {
                     this.pauseTicking();
                     this.game.levelUpUI = this.game.newLevelUpUI(1);
                     this.game.showScreen(this.game.levelUpUI);
                     autoRepaintEnabled = false;
                  }

                  this.unconfirmed_v = false;
               }

               this.refreshVisibleObjectsAndMinimap(false);
               if (autoRepaintEnabled) {
                  this.repaint();
                  this.serviceRepaints();
               }

               long tickDuration = System.currentTimeMillis() - frameStart;
               unconfirmed_ai = tickDuration;

               try {
                  if (tickDuration < 250L) {
                     Thread.sleep(250L - tickDuration);
                  }
               } catch (Exception ignored) {
               }

               if (this.stopRequested) {
                  this.stopRequested = false;
                  return;
               }

               long prevFrameStart = frameStart;
               frameStart = System.currentTimeMillis();
               deltaMs = frameStart - prevFrameStart;
               this.tickStatusCountdowns(deltaMs);
               secondAccumulator += deltaMs;
               if (secondAccumulator > 1000L) {
                  secondAccumulator -= 1000L;
                  this.tickPerSecond();
               }

               if (frameStart - messageShownAt > 3000L) {
                  unconfirmed_ad = false;
                  messagePriority = 0;
               }
            }
         }
      } catch (Throwable t) {
         t.printStackTrace();
      }
   }

   // ---- Helper stubs referenced only from run(), signature-only, body
   // not yet transcribed (each corresponds to a short original method
   // whose exact source wasn't reached in this pass). ----

   // Confirmed (phase-3 port M37): byte-for-byte from decompiled/e.java's
   // o() -- a flat 1-in-10 chance.
   private boolean rollCampInterrupted() {
      return Util.randomInt(10) == 1;
   }

   // M22: fully transcribed from decompiled/e.java's d() -- needed for
   // paintHud()'s resolveHudIconSet() above, so pulled into this pass
   // even though it isn't itself a paint method. True either when
   // Shop.isAdjacentToVarus() says so (decompiled Shop/k.java's own
   // `a(j)` overload -- a genuine overload collision with `a(int)` /
   // shouldWardenVisit(), NOT the same thing despite this method's call
   // site in run() being paired with Warden-visit state), or when
   // GameCanvas's own targetMonster field (the level-37 type-41
   // "roaming" monster M19 already confirmed) is set and adjacent to
   // the player while both are on level 37.
   private boolean isNpcDialogueDue() {
      if (Shop.isAdjacentToVarus(this.player)) {
         return true;
      } else if (targetMonster == null) {
         return false;
      } else if (this.player.currentLevel == 37 && targetMonster.typeIndex == 41) {
         int dx = Math.abs(this.player.tileX - targetMonster.tileX);
         int dy = Math.abs(this.player.tileY - targetMonster.tileY);
         return dx + dy == 1;
      } else {
         return false;
      }
   }

   // Confirmed (phase-3 port M37): byte-for-byte from decompiled/e.java's
   // b(long) -- RENAMED from this file's own earlier wrong
   // "tickStatusCountdowns_b" guess (it has nothing to do with status
   // countdowns at all). This is the long-flagged, previously-
   // unrecovered caller for Monster.tick()/Monster.chase()
   // (../../docs/ROADMAP.md, flagged since phase-3 M14/M15) -- finally
   // found. For every monster registered on the player's own current
   // level: not adjacent -> chase() one step (isAdjacent()'s own
   // non-adjacent call ALSO resets aiPhase to 0, a confirmed side
   // effect -- see Monster.java's own isAdjacent() header comment);
   // adjacent -> an 800ms wind-up (aiPhase 0->1 starts the timer,
   // aiPhase 1 past 800ms resolves the FIRST real attack via tick() AND
   // shows the "Creature attacks!" popup -- MSG_CREATURE_ATTACKS
   // (`ah` in the original) at priority 2, ONLY on this first
   // wind-up-to-attack transition -- and any LATER 800ms-elapsed tick
   // resolves a repeat attack with no further popup). Every branch
   // calls store() -- unlike tickPerSecond()'s own confirmed dead-write
   // loop over this exact same registry (see that method's own header
   // comment), this one really does persist, since the original
   // explicitly calls store() itself every time.
   private void tickMonsterAI(long now) {
      Hashtable levelMonsters = ESGame.monsters[this.player.currentLevel - 1];
      if (levelMonsters == null) {
         return;
      }

      Enumeration e = levelMonsters.elements();

      while (e.hasMoreElements()) {
         byte[] data = (byte[])e.nextElement();
         Monster m = Monster.fromBytesShared(data);

         if (m.isAdjacent(this.player)) {
            if (m.aiPhase == 0) {
               m.unconfirmedTimestamp = now;
               m.aiPhase = 1;
            } else if (m.aiPhase == 1 && now - m.unconfirmedTimestamp > 800L) {
               m.tick(this.player, now);
               if (this.showMessage(MSG_CREATURE_ATTACKS, 2)) {
                  messageShownAt = now;
                  unconfirmed_ad = true;
               }
            } else if (now - m.unconfirmedTimestamp > 800L) {
               m.tick(this.player, now);
            }

            m.store();
         } else {
            m.chase(this.player);
            m.store();
         }
      }
   }

   private void tickStatusCountdowns_e(long now) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's e(long))");
   }

   private void tickMovementAndAI(long now, long deltaMs) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's a(long,long))");
   }

   // Sets threadStarted (was `e()`), the exact counterpart of
   // resumeTicking()/`b()` -- pauses run()'s main loop back into its
   // 250ms sleep-only branch. Confirmed callers: run()'s own level-up
   // trigger (renamed from an earlier, wrong "onPlayerDeath" guess once
   // this method's real body was read) and ESGame.showScreen, whenever
   // navigating away from the game view to any menu/dialog screen.
   void pauseTicking() {
      this.threadStarted = true;
   }

   // Confirmed (phase-3 port M37): byte-for-byte from decompiled/e.java's
   // a(boolean) -- RENAMED from this file's own earlier wrong
   // "setSomeFlag" guess (it doesn't set any flag at all -- it's a
   // refresh trigger). Player.refreshVisibleObjects() (already fully
   // transcribed) already IS what `this.ax.d(var1)` in the original
   // calls; this wrapper just also refreshes the zoomed-out minimap grid
   // unconditionally (populateMinimapGrid(), M32) and the normal-zoom
   // one ONLY when it's the one actually on screen (hotbarContext == 2
   // -- see paintAll()'s own header comment on `f`/hotbarContext's dual
   // role as both the numeric-hotkey icon-row context AND, confusingly,
   // the minimap zoom-mode selector).
   private void refreshVisibleObjectsAndMinimap(boolean includeWarden) {
      this.player.refreshVisibleObjects(includeWarden);
      this.populateMinimapGrid();
      if (hotbarContext == 2) {
         this.populateVisibleGrid();
      }
   }
}
