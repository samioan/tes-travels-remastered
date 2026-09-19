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
// against real content and looks WRONG: `paintFloor()`'s claimed
// `b(Graphics)` reads from `player.ad`-shaped data (looks like an
// object/visibleObjects renderer, i.e. paintObjects()'s real body), and
// separately `paintMessagePopup()`'s claimed `e(Graphics)` reads/writes
// the `unconfirmed_S`/`unconfirmed_ao`/`unconfirmed_am` flash flags (a
// monster-hit/spell-hit/self-spell flash overlay, not a message popup).
// Every remaining stub's mapping should be independently re-verified
// against its actual decompiled body before being trusted, not assumed
// correct just because a plausible-looking name/signature was already
// filled in.
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
   private static final Font unusedFont_aj = Font.getFont(64, 0, 8);
   private static final Font deadScreenFont = Font.getFont(64, 2, 16);
   private static final Font campScreenFont = Font.getFont(64, 2, 16);
   private static final Font unusedFont_K = Font.getFont(64, 1, 16);

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
   // TODO: 41 rows x 2 cols, mostly {0,0}/{0,3}/{0,2} -- purpose
   // unconfirmed, likely per-position-code render flags of some kind
   // (shape suggestive of dawnstar's OBJECT_EXTRA_FLAGS, not confirmed).
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
   private static int unconfirmed_X = 0;
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

   // Main game-view paint: black background, corridor walls, floor,
   // (conditionally) something gated by unconfirmed_W using player.
   // player.questShopAtPendingTile(), monsters (try/caught separately -- the
   // original wraps just this call so a bad monster paint can't blank the
   // whole frame), objects, HUD, a message popup, and the active hotbar
   // (context 1 or 2) when no dialogue-equivalent Screen is blocking key 3.
   //
   // TODO: every paint* method called from here is a stub below --
   // signatures preserved, bodies not yet transcribed.
   private void paintGameView(Graphics g) {
      g.setColor(0);
      g.fillRect(0, 0, this.getWidth(), this.getHeight());
      this.paintWalls(g);
      this.paintFloor(g);
      if (unconfirmed_W) {
         int stat = this.player.questShopAtPendingTile();
         this.paintUnknown_b(g, stat);
      }

      try {
         this.paintMonsters(g);
      } catch (Throwable t) {
         System.out.println("Error in paintMonsters: " + t);
      }

      this.paintObjects(g);
      this.paintHud(g);
      this.paintUnknown_l(g);
      this.paintMessagePopup(g);
      if (popupMessageActive) {
         this.paintPopupText(g);
      }

      if (hotbarActionSet == 1 && !this.player.hasAilment(3)) {
         this.paintHotbar1(g);
      }

      if (hotbarActionSet == 2 && !this.player.hasAilment(3)) {
         this.paintHotbar2(g);
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
   // Whether floorTexture/wallTexture are loaded from a `.cus` file (M7's
   // RawImage format) or a plain MIDP-native Image resource is NOT
   // determined by this pass -- none of M7's own 37 confirmed `.cus`
   // files read as a wall/floor texture by name, so this is flagged as an
   // open question for whichever milestone actually wires image loading,
   // not resolved here.
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

   private void paintFloor(Graphics g) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's b(Graphics))");
   }

   private void paintMonsters(Graphics g) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's g(Graphics))");
   }

   private void paintObjects(Graphics g) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's a(Graphics))");
   }

   private void paintHud(Graphics g) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's d(Graphics))");
   }

   private void paintUnknown_l(Graphics g) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's l(Graphics))");
   }

   private void paintMessagePopup(Graphics g) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's e(Graphics))");
   }

   private void paintUnknown_b(Graphics g, int stat) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's b(Graphics,int))");
   }

   private void paintHotbar1(Graphics g) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's k(Graphics))");
   }

   private void paintHotbar2(Graphics g) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's h(Graphics))");
   }

   // Shows a message popup (2-line String[]) if priority `pri` beats
   // whatever's currently showing -- signature preserved from a confirmed
   // run() call site (`this.a(N, 1)`/`this.showMessage(MSG_REST_DISTURBED,
   // 1)`), body not yet transcribed.
   private boolean showMessage(String[] lines, int priority) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's a(String[],int))");
   }

   // Per-tick status-effect countdowns -- confirmed call site in run()
   // (`this.c(var5)`), body not yet transcribed.
   private void tickStatusCountdowns(long deltaMs) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's c(long))");
   }

   // Once-per-real-second passive regen/drain, and the Warden-visit gate
   // -- confirmed call site in run() (`this.l()`), body not yet
   // transcribed.
   private void tickPerSecond() {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's l())");
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
                     unconfirmed_X = 0;
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

                  this.tickStatusCountdowns_b(frameStart);
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

               this.setSomeFlag(false);
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
                  unconfirmed_X = 0;
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

   private boolean rollCampInterrupted() {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's o())");
   }

   private boolean isNpcDialogueDue() {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's d())");
   }

   private void tickStatusCountdowns_b(long now) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's b(long))");
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

   private void setSomeFlag(boolean value) {
      throw new UnsupportedOperationException("TODO: not yet transcribed (was e.java's a(boolean))");
   }
}
