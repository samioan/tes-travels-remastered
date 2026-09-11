// Renamed from decompiled/e.java. See ../docs/CLASS_MAP.md.
//
// The renderer + input handler + main tick loop: paints the first-person
// corridor view (CORRIDOR_WALL_TABLE), the HUD (3-bar status meter,
// numeric hotbar, 2-line message popup, compass + minimap thumbnail),
// and drives the ~4Hz game loop from its own background Thread (run())
// -- movement, combat, spellcasting, camping, death/respawn, and the
// scripted "overstayed in one place" ambush spawner (see tickPerSecond).
//
// NOTE: `j` (Player), `ESGame`, `d` (Monster) and `i` (Dungeon) below are
// NOT the already-renamed Player/Monster/Dungeon classes -- every value
// that flows through Player's own (unrenamed) API keeps Player's
// original single-letter member names, because Player itself hasn't
// been mechanically renamed yet (see CLASS_MAP.md's "why e/j aren't
// renamed yet"). In particular `targetMonster` is typed `d`, not the
// real `Monster` class, because Player.n()/a(d)/b(int,d) all still
// declare that type -- assigning/passing it as `Monster` would not
// compile against the untouched j.java. Only fully GameCanvas-owned
// locals (the scratch Monster built fresh each second in tickPerSecond,
// never handed to Player) use the real, already-renamed Monster class.
// Likewise `Item`/`Spell`/`Shop`/`Util` calls below use their real,
// already-renamed names -- those calls only cross primitives or values
// GameCanvas itself constructs, so there's no such boundary problem.
//
// Like Dungeon.java before it, this file will NOT compile drop-in
// against the untouched decompiled/ESGame.java: ESGame's own field is
// still declared `e gameCanvas;`, and its `.Y =`/`.v`/`.an` etc. writes
// target the OLD `e` class, not this one. That's expected integration
// debt for ESGame's own future rename pass -- see ../src/README.md.
import com.nokia.mid.ui.FullCanvas;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

public class GameCanvas extends FullCanvas implements Runnable {
   private static final Font SMALL_FONT = Font.getFont(64, 0, 8);
   // Declared, never referenced anywhere in this class -- vestigial,
   // same as several fields below.
   private static final Font UNUSED_FONT = Font.getFont(64, 2, 16);
   // Used for the full-screen "You're Dead!" / "CAMPING" messages.
   private static final Font BIG_MESSAGE_FONT = Font.getFont(64, 2, 16);
   // Compass glyph font used only while the minimap is zoomed out.
   private static final Font COMPASS_FONT_ZOOMED = Font.getFont(64, 1, 16);

   // Wall-segment draw commands for the 5 possible forward-visibility
   // steps (how far you can see down a straight corridor before a
   // wall/junction) x 6 candidate offsets to test x {cmd, column, dx,
   // dy}. cmd is fed into resolveWallFrame (12 = pass the column
   // through unchanged, 11 = mirror it) -- the exact geometric meaning
   // of the two command codes isn't fully pinned down, preserved
   // exactly as found.
   static final int[][][] CORRIDOR_WALL_TABLE = new int[][][]{
      {{12, 0, 0, 1}, {11, 0, -1, 1}, {12, 1, -1, 2}, {12, 2, -1, 3}, {11, 2, -2, 3}, {12, 3, -2, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {11, 1, -1, 2}, {12, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {11, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {11, 3, -1, 4}, {11, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {12, 3, 0, 4}, {12, 3, 0, 4}}
   };
   // Object/decoration placement data keyed by a "position code" (1-42)
   // via positionBucketFor, consumed by paintObjectAtPosition. Structure
   // is clear (per-bucket base position + up to 4 extra decorations, each
   // as an (dx, dy, icon) triple) but the exact per-column meaning isn't
   // fully pinned down -- see CLASS_MAP.md's open questions.
   private static final byte[][] OBJECT_DRAW_TABLE = new byte[][]{
      {1, 5, 43, 48, 0, 2, 23, 7, 1, 3, 0, 64, 2, 18, 27, 3, 24, 19, 4, 0, 0, 0},
      {6, 10, 43, 49, 7, 2, 18, 8, 8, 3, 3, 84, 11, 17, 24, 10, 0, 80, 9, 0, 0, 0},
      {11, 25, 40, 50, 14, 3, 0, 0, -1, -1, 9, 29, 16, 11, 0, 15, 41, 41, 17},
      {26, 40, 37, 50, 20, 3, 0, 0, -1, -1}
   };
   // [positionCode - 1] -> {iconIndex, frameOverride}, consumed by
   // paintObjectAtPosition.
   private static final byte[][] OBJECT_ICON_TABLE = new byte[][]{
      {0, 0}, {1, 0}, {0, 1}, {1, 2}, {0, 2}, {0, 1}, {1, 1}, {0, 2}, {1, 2}, {0, 2},
      {0, 0}, {1, 0}, {0, 0}, {1, 0}, {0, 0}, {2, 0}, {0, 0}, {2, 0}, {0, 0}, {2, 0},
      {1, 0}, {2, 0}, {1, 0}, {2, 0}, {2, 0}, {0, 0}, {1, 0}, {0, 0}, {1, 0}, {0, 0},
      {2, 0}, {0, 0}, {2, 0}, {0, 0}, {2, 0}, {1, 0}, {2, 0}, {1, 0}, {2, 0}, {1, 0},
      {0, 0}
   };
   // [positionCode - 1][0..3] -> whether each of 4 extra decorations at
   // that position is drawn, consumed by paintObjectAtPosition.
   private static final boolean[][] OBJECT_EXTRA_FLAGS = new boolean[][]{
      {false, false, false, false}, {true, true, false, false}, {false, false, true, false},
      {true, false, false, false}, {true, true, true, false}, {false, false, false, false},
      {true, false, false, false}, {false, false, true, false}, {false, true, false, false},
      {true, true, false, false}, {false, false, false, false}, {false, false, false, false},
      {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
      {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
      {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
      {false, false, false, false}, {false, false, false, false}, {false, false, false, false},
      {false, false, false, false}, {true, false, true, false}, {true, false, true, false},
      {true, false, true, false}, {true, false, true, false}, {true, false, true, false},
      {true, false, true, false}, {true, false, true, false}, {true, false, true, false},
      {true, true, false, false}, {true, true, false, false}, {true, true, false, false},
      {true, true, false, false}, {true, true, false, false}, {true, true, false, false},
      {true, true, false, false}, {false, false, false, false}
   };
   // Hotbar digit glyphs in context order: '1','3','5','7','9','0'.
   static final char[] HOTBAR_DIGIT_CHARS = new char[]{'1', '3', '5', '7', '9', '0'};
   // Compass glyph per Player.aw facing (index 0 unused): N/E/S/W.
   static final char[] COMPASS_GLYPHS = new char[]{'0', 'N', 'E', 'S', 'W'};
   // Declared, never read anywhere in this class (and unreachable
   // externally, private static final) -- dead data, purpose unknown.
   private static final int[][] UNUSED_TABLE = new int[][]{{0, 0, 0}, {0, 1, 0}, {0, 2, 1}};
   // Per-corridor-depth-column x-offset for wall/gate sprite placement.
   private static final int[] WALL_DRAW_OFFSETS = new int[]{0, 0, 36, 72, 90, 108, 126, 144, 158, 176, 194, 212};

   static final String[] MSG_CANNOT_CAMP = new String[]{"Cannot", "Camp!"};
   static final String[] MSG_NO_SPELLS = new String[]{"No spells!", ""};
   static final String[] MSG_NOT_ENOUGH_MAGICKA = new String[]{"Not enough", "magicka!"};
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
   static final String[] MSG_ENEMY_ARRIVED = new String[]{"Enemy", "arrived!"};

   // --- instance fields -----------------------------------------------
   private ESGame game;
   // The MIDP game-action code of the currently held movement key.
   private int heldGameAction;
   // Mirrors heldGameAction on every key event, but never read back
   // anywhere in this class -- write-only/vestigial.
   private int lastGameAction;
   private Thread gameThread;
   private boolean threadPaused;
   private boolean threadRunning;
   private boolean threadKillRequested;
   // Player. Kept typed `j` (unrenamed) -- see the class header note.
   j player;
   // 1 = alive, 2 = HP just hit 0 (one-tick pause), 3 = "You're Dead!"
   // screen showing, waiting to respawn.
   byte deathState;
   // Gates dispatchTickActions's movement-input branch: while true, a
   // pending move key is ignored for this tick. Set after camping ends,
   // dying, or respawning, so a leftover keypress isn't replayed.
   boolean suppressMoveInput;
   // 0 = not camping, 1 = camping (rolling for interruption each
   // 2500ms), 2 = camping safely (no interruption roll, just waiting
   // 5000ms to complete -- entered directly when Player's Safe Camping
   // buff is active, or when camping in the hub town), 3 = a rare
   // scripted "disturbed" camp (10% roll on level > 3, outside the
   // Safe Camping/hub-town cases) that skips straight to the
   // interruption outcome.
   byte campState;
   long campStartTime;
   long deathTime;
   long lastAttackTime;
   long lastSpellCastTime;
   // Set for the '4'/'6' alternate movement keys; passed through to
   // Player.a(dir, strafe) as the strafe flag.
   boolean strafeMove;
   // Pending movement direction (1-4, N/E/S/W), consumed by commitMove.
   int pendingMoveDir;
   // Dedup toggles in drawWallSegment, preventing a wall-segment strip
   // from being drawn twice when both the forward and mirrored corridor
   // scan (in paintCorridorWalls) reach the same screen column -- exact
   // geometry not fully traced, behavior preserved as found.
   private boolean wallDrawnNear;
   private boolean wallDrawnMid;
   Screen activeScreen;
   // Set so the background thread nudges one more repaint while a
   // Screen dialog is up (see Screen.requestRepaint / run()).
   boolean repaintPending;
   int screenHeight;
   int screenWidth;
   // Set after any move/respawn/minimap-zoom-toggle; run() rebuilds the
   // minimap thumbnail (refreshMinimap) whenever this is true.
   public boolean minimapDirty = false;
   Image minimapImage;

   // --- shared/static state --------------------------------------------
   static boolean monsterTargeted;
   static boolean chestInSight;
   static int npcInSight = -1;
   // floor3.png / floorIce.png / wallsr.png / wallsi.png / gate.png --
   // the hub town (dungeon number 1) uses the plain floor/wall textures,
   // every other level (2-37, all icy) uses the Ice-suffixed ones; gate
   // texture marks the tile-bit-6 (edge/transition) case specifically
   // (confirmed via paintCorridorWalls / drawWallSegment).
   static Image floorTexture;
   static Image floorIceTexture;
   static Image wallTexture;
   static Image wallIceTexture;
   static Image gateTexture;
   static Image[] objectSprites;
   // chestnearclosed.png / chestmidclosed.png / chestfarclosed.png.
   static Image[] chestSprites;
   // baglarge.png / bagmid.png / bagsmall.png (dropped-item bag, by
   // on-screen distance: near/mid/far).
   static Image[] itemBagSprites;
   static Image iconsSprite;
   static Image panelImage;
   // The occlusion/tile-type sample grid populated by
   // Dungeon.sampleCorridorView/sampleSquareView (17x17, or a 7x7
   // sub-window when zoomed out) that both the corridor renderer and
   // the minimap read from.
   private static byte[][] visibleTileGrid;
   private static boolean minimapZoomedOut = false;
   // One-shot flash triggers, drawn once by paintActionFlashes then
   // cleared: player damaged the target monster / player's spell hit
   // its target / player cast a spell on themself.
   static boolean monsterHitFlash = false;
   static boolean spellHitFlash = false;
   static boolean selfSpellFlash = false;
   // 0 = exploring, 1 = monster targeted (combat hotbar), 2 = chest or
   // NPC in sight (interact hotbar) -- see computeHotbarContext.
   static int hotbarContext;
   static boolean attackRequested = false;
   static boolean interactRequested = false;
   // Set from key '9' when hotbarContext isn't the chest/NPC context --
   // consumed by dispatchTickActions but that branch is empty, so this
   // request is a no-op. Vestigial.
   static boolean unusedKey9Request = false;
   // Declared, never used anywhere in this class -- dead.
   static int unusedP = 0;
   static int unusedAI = 0;
   static boolean campRequested = false;
   static boolean optionsRequested = false;
   static boolean castSpellRequested = false;
   static boolean spellCycleRequested = false;
   static boolean messageVisible = false;
   static long messageShownAt = 0L;
   static String[] messageLines = null;
   // "Priority" of the currently-shown message -- a new message only
   // replaces it if its own priority is higher (see showMessage).
   private static int messagePriority = 0;
   // Declared, never used anywhere in this class -- dead.
   static boolean unusedAe = false;
   // False while a Screen dialog owns the display (it repaints itself);
   // true again once control returns to the 3D view (see showNotify()).
   private static boolean canvasActive = true;
   // True once per tick after any player action (move/attack/cast) --
   // suppresses the passive per-tick regen call in processIdleTick.
   private static boolean actionTakenThisTick = false;
   static boolean showLevelNameMessage = false;
   // True while the nearby target monster's attack-animation flag
   // (targetMonster.c[6] equivalent) is set -- gates one of the timed
   // status-ailment countdowns in tickStatusCountdowns.
   static boolean monsterAttacking = false;
   // The player's current combat target, refreshed by refreshTargetMonster.
   // Typed `d` (unrenamed Monster), not the real Monster class -- see the
   // class header note.
   static d targetMonster = null;
   // Set every tick to the previous tick's processing duration; never
   // read back anywhere in this class -- write-only diagnostic.
   private static long lastTickDuration = 0L;
   private static boolean errorState = false;
   private static String errorMessage = null;

   public GameCanvas(ESGame game) {
      this.gameThread = new Thread(this);
      this.game = game;
      this.screenHeight = this.getHeight();
      this.screenWidth = this.getWidth();
      this.lastGameAction = 0;
      this.heldGameAction = 0;
      this.threadRunning = false;
      this.threadPaused = false;
      this.threadKillRequested = false;
      this.player = null;
      this.deathState = 1;
      this.suppressMoveInput = false;
      this.campState = 0;
      this.deathTime = 0L;
      this.campStartTime = 0L;
      this.pendingMoveDir = 0;
      this.strafeMove = false;
      monsterTargeted = false;
      chestInSight = false;
      npcInSight = -1;
      visibleTileGrid = new byte[17][17];
      hotbarContext = 0;
      this.lastAttackTime = 0L;
      this.lastSpellCastTime = 0L;
      this.activeScreen = null;
      this.repaintPending = false;
      this.minimapImage = Image.createImage(89, 89);
   }

   public void paint(Graphics g) {
      if (this.activeScreen != null) {
         try {
            this.activeScreen.paint(g);
         } catch (Exception e) {
            this.game.displayDebug();
            System.out.println("paint " + e.toString());
         }
      } else if (this.deathState == 3) {
         this.paintDeathScreen(g);
      } else if (this.campState != 1 && this.campState != 3 && this.campState != 2) {
         this.paintGameView(g);
      } else {
         this.paintCampingScreen(g);
      }
   }

   private void paintDeathScreen(Graphics g) {
      g.setColor(0);
      g.fillRect(0, 0, this.screenWidth, this.screenHeight);
      g.setColor(16777215);
      g.setFont(BIG_MESSAGE_FONT);
      g.drawString("You're Dead!", this.screenWidth >> 1, this.screenHeight >> 1, 33);
   }

   private void paintCampingScreen(Graphics g) {
      g.setColor(0);
      g.fillRect(0, 0, this.screenWidth, this.screenHeight);
      g.setColor(16777215);
      g.setFont(BIG_MESSAGE_FONT);
      g.drawString("CAMPING", this.screenWidth >> 1, this.screenHeight >> 1, 33);
   }

   private void paintGameView(Graphics g) {
      g.setColor(0);
      g.fillRect(0, 0, this.screenWidth, this.screenHeight);
      this.paintCorridorWalls(g);

      try {
         this.paintVisibleObjects(g);
      } catch (Throwable t) {
         System.out.println("Error in paintMonsters: " + t);
      }

      if (npcInSight >= 0) {
         this.paintNpcPortrait(g, npcInSight);
      }

      this.paintStatusBars(g);
      this.paintHotbar(g);
      this.paintMessagePopup(g);
      this.paintActionFlashes(g);
      if (errorState) {
         this.paintErrorOverlay(g);
      }

      if (!this.player.i(3)) {
         g.setColor(16777215);
         if (!minimapZoomedOut) {
            g.setFont(SMALL_FONT);
            g.drawChar(COMPASS_GLYPHS[this.player.aw], 16, 10, 20);
            g.setClip(10, 20, 23, 23);
            g.drawImage(this.minimapImage, 10, 20, 20);
         } else {
            g.setFont(COMPASS_FONT_ZOOMED);
            g.drawChar(COMPASS_GLYPHS[this.player.aw], 58, 10, 20);
            g.drawImage(this.minimapImage, 15, 25, 20);
         }
      }
   }

   private void paintErrorOverlay(Graphics g) {
      g.setColor(16777215);
      g.drawString(errorMessage, 60, 10, 17);
   }

   private void paintActionFlashes(Graphics g) {
      if (monsterHitFlash) {
         int x = 40 + Util.randomInt(30);
         int y = 50 + Util.randomInt(20);
         this.drawHotbarIcon(g, 6, x, y);
         monsterHitFlash = false;
      }

      if (spellHitFlash) {
         int x = 40 + Util.randomInt(30);
         int y = 50 + Util.randomInt(22);
         this.drawHotbarIcon(g, 8, x, y);
         spellHitFlash = false;
      }

      if (selfSpellFlash) {
         int x = 50 + Util.randomInt(2);
         int y = 80 + Util.randomInt(2);
         this.drawHotbarIcon(g, 7, x, y);
         selfSpellFlash = false;
      }

      g.setClip(0, 0, this.screenWidth, this.screenHeight);
   }

   // Paints the two backdrop bands (floor/ceiling) and then, per corridor
   // depth step, the nearest wall or gate segment on both the forward and
   // mirrored side, by scanning CORRIDOR_WALL_TABLE's candidate offsets
   // for the first occluding (wall, bit0) or transition (edge, bit6) tile.
   private void paintCorridorWalls(Graphics g) {
      i dungeon = this.player.a();
      if (!this.player.i(3)) {
         if (this.player.i(4)) {
            g.setColor(10485760);
            g.fillRect(0, 0, this.screenWidth, floorTexture.getHeight());
         } else {
            for (int col = 0; col < 5; col++) {
               if (dungeon.e != 1) {
                  g.drawImage(floorIceTexture, col * 36, 0, 20);
               } else {
                  g.drawImage(floorTexture, col * 36, 0, 20);
               }
            }
         }
      }

      this.wallDrawnNear = false;
      this.wallDrawnMid = false;

      for (int step = 0; step < 5; step++) {
         int x = step * 18;

         for (int row = 0; row < 6; row++) {
            int cmd = CORRIDOR_WALL_TABLE[step][row][0];
            int column = CORRIDOR_WALL_TABLE[step][row][1];
            int dx = CORRIDOR_WALL_TABLE[step][row][2];
            int dy = CORRIDOR_WALL_TABLE[step][row][3];
            if (Util.testBit((byte)1, this.player.a(dx, dy))) {
               int frame = this.resolveWallFrame(cmd, column, -1);
               this.drawWallSegment(g, frame, x, dungeon.e);
               break;
            }

            if (Util.testBit((byte)64, this.player.a(dx, dy))) {
               int frame = this.resolveWallFrame(cmd, column, -1);
               this.drawWallSegment(g, frame, x, -1);
               break;
            }
         }
      }

      for (int step = 5; step < 10; step++) {
         int x = step * 18;

         for (int row = 0; row < 6; row++) {
            int cmd = CORRIDOR_WALL_TABLE[9 - step][row][0];
            int column = CORRIDOR_WALL_TABLE[9 - step][row][1];
            int dx = -CORRIDOR_WALL_TABLE[9 - step][row][2];
            int dy = CORRIDOR_WALL_TABLE[9 - step][row][3];
            if (Util.testBit((byte)1, this.player.a(dx, dy))) {
               int frame = this.resolveWallFrame(cmd, column, 1);
               this.drawWallSegment(g, frame, x, dungeon.e);
               break;
            }

            if (Util.testBit((byte)64, this.player.a(dx, dy))) {
               int frame = this.resolveWallFrame(cmd, column, 1);
               this.drawWallSegment(g, frame, x, -1);
               break;
            }
         }
      }

      g.setClip(0, 0, this.screenWidth, this.screenHeight);
   }

   // Draws one 18px-wide wall/gate column: wallType is the dungeon
   // number (>1 draws the ice wall texture, ==1 draws the regular one)
   // or -1 for a gate/transition tile. wallDrawnNear/wallDrawnMid
   // suppress a duplicate draw when the forward and mirrored corridor
   // scans (in paintCorridorWalls) land on the same segment.
   private void drawWallSegment(Graphics g, int frame, int x, int wallType) {
      g.setClip(x, 0, 18, this.screenHeight);
      if (frame != 0 && frame != 1) {
         this.wallDrawnNear = false;
         if (frame == 2) {
            if (this.wallDrawnMid) {
               this.wallDrawnMid = false;
               if (wallType == -1) {
                  g.drawImage(gateTexture, x - WALL_DRAW_OFFSETS[frame] - 18, 8, 20);
               } else if (wallType != 1) {
                  g.drawImage(wallIceTexture, x - WALL_DRAW_OFFSETS[frame] - 18, 0, 20);
               } else {
                  g.drawImage(wallTexture, x - WALL_DRAW_OFFSETS[frame] - 18, 0, 20);
               }

               return;
            }

            this.wallDrawnMid = true;
         }
      } else {
         this.wallDrawnMid = false;
         if (this.wallDrawnNear) {
            this.wallDrawnNear = false;
            if (wallType == -1) {
               g.drawImage(gateTexture, x - WALL_DRAW_OFFSETS[frame] - 18, 8, 20);
            } else if (wallType != 1) {
               g.drawImage(wallIceTexture, x - WALL_DRAW_OFFSETS[frame] - 18, 0, 20);
            } else {
               g.drawImage(wallTexture, x - WALL_DRAW_OFFSETS[frame] - 18, 0, 20);
            }

            return;
         }

         this.wallDrawnNear = true;
      }

      if (wallType == -1) {
         g.drawImage(gateTexture, x - WALL_DRAW_OFFSETS[frame], 8, 20);
      } else if (wallType != 1) {
         g.drawImage(wallIceTexture, x - WALL_DRAW_OFFSETS[frame], 0, 20);
      } else {
         g.drawImage(wallTexture, x - WALL_DRAW_OFFSETS[frame], 0, 20);
      }
   }

   // Word-wraps `text` to fit `maxWidth` pixels in `font`, honoring
   // embedded '\n' as hard line breaks. Called by Screen.wrapText.
   public String[] wordWrap(String text, int maxWidth, Font font) {
      int newlineAt;
      if ((newlineAt = text.indexOf(10, 0)) >= 0) {
         if (newlineAt != text.length() - 1) {
            String[] firstLines;
            if (newlineAt == 0) {
               firstLines = new String[]{" "};
            } else {
               firstLines = this.wordWrap(text.substring(0, newlineAt), maxWidth, font);
            }

            String[] restLines = this.wordWrap(text.substring(newlineAt + 1), maxWidth, font);
            String[] combined = new String[firstLines.length + restLines.length];
            int split = firstLines.length;
            System.arraycopy(firstLines, 0, combined, 0, split);
            System.arraycopy(restLines, 0, combined, split, restLines.length);
            return combined;
         }

         text = text.substring(0, text.length() - 1);
      }

      if (font.stringWidth(text) < maxWidth) {
         return new String[]{text};
      }

      text = text + " ";
      Vector lines = new Vector();
      int lineStart = 0;
      maxWidth -= 8;

      int spaceAt;
      while ((spaceAt = text.indexOf(" ", lineStart + 1)) > 0) {
         if (font.substringWidth(text, 0, spaceAt) < maxWidth) {
            lineStart = spaceAt;
         } else {
            if (lineStart == 0) {
               for (int w = 0; w < maxWidth; lineStart++) {
                  w += font.charWidth(text.charAt(lineStart));
               }

               lines.addElement(text.substring(0, lineStart));
               lineStart--;
            } else {
               lines.addElement(text.substring(0, lineStart));
            }

            text = text.substring(lineStart + 1);
            lineStart = 0;
         }
      }

      if (text.length() > 0 && !text.equals(" ")) {
         lines.addElement(text);
      }

      String[] result = new String[lines.size()];

      for (int i = result.length - 1; i >= 0; i--) {
         result[i] = (String)lines.elementAt(i);
      }

      return result;
   }

   // Word-wraps `text` to exactly 2 lines (padding with "" if it fit on
   // one), using SMALL_FONT at the fixed popup width -- the shape every
   // message-popup caller needs.
   private String[] wrapToTwoLines(String text) {
      if (text == null) {
         text = "";
      }

      String[] result = new String[2];
      String[] wrapped = this.wordWrap(text, 69, SMALL_FONT);
      if (wrapped.length == 1) {
         result[0] = new String(wrapped[0]);
         result[1] = "";
      } else {
         System.arraycopy(wrapped, 0, result, 0, 2);
      }

      return result;
   }

   // Shows `lines` in the message popup unless a higher-priority message
   // is already showing. `priority` < 0 means "always show, max priority".
   // Returns whether the message was actually accepted.
   private boolean showMessage(String[] lines, int priority) {
      if (priority <= messagePriority && priority >= 0) {
         return false;
      }

      messageLines = lines;
      if (priority < 0) {
         messagePriority = 10;
      } else {
         messagePriority = priority;
      }

      return true;
   }

   // Detects whether an NPC is directly in front of the player (tile
   // bit 5, "blocked marker" used for shop-room tiles) and, if so, sets
   // npcInSight and shows that shop's greeting.
   public void refreshNpcInSight() {
      byte tileBits = this.player.a(0, 1);
      if (Util.testBit((byte)32, tileBits)) {
         int shopId = this.player.p();
         if (shopId == -1) {
            npcInSight = -1;
            System.out.println("NPC infront is not defined!!!");
            return;
         }

         npcInSight = shopId;
         if (this.showMessage(this.wrapToTwoLines(Shop.NAMES[shopId]), 1)) {
            messageShownAt = System.currentTimeMillis();
            messageVisible = true;
         }
      } else {
         npcInSight = -1;
      }
   }

   // Resolves a CORRIDOR_WALL_TABLE {cmd, column} pair plus scan side
   // (-1 forward, 1 mirrored) to the actual wall-frame column index used
   // by WALL_DRAW_OFFSETS/drawWallSegment.
   private int resolveWallFrame(int cmd, int column, int side) {
      if (cmd == 12) {
         return column;
      } else {
         return side == -1 ? 8 + column : 7 - column;
      }
   }

   // Draws the NPC portrait for shop `shopId` at its canned on-screen
   // position, reusing the generic corridor object renderer.
   private void paintNpcPortrait(Graphics g, int shopId) {
      switch (shopId) {
         case 0:
            this.paintObjectAtPosition(g, 1, 2);
            break;
         case 1:
            this.paintObjectAtPosition(g, 4, 1);
            break;
         case 2:
            this.paintObjectAtPosition(g, 7, 0);
            break;
         case 3:
            this.paintObjectAtPosition(g, 6, 2);
            break;
         case 4:
            this.paintObjectAtPosition(g, 8, 1);
            break;
         case 5:
            this.paintObjectAtPosition(g, 9, 0);
            break;
         case 6:
            this.paintObjectAtPosition(g, 10, 2);
            break;
         case 7:
            this.paintObjectAtPosition(g, 2, 0);
            break;
         case 8:
            this.paintObjectAtPosition(g, 3, 2);
      }
   }

   // Draws the up/down stairs icon (objectSprites[23], 2 frames) at the
   // far/special position code.
   private void paintStairsIcon(Graphics g, boolean up) {
      byte x = 33;
      byte y = 48;
      this.drawSpriteFrame(g, objectSprites[23], up ? 0 : 1, 2, x, y);
      g.setClip(0, 0, this.screenWidth, this.screenHeight);
   }

   // Paints every visible monster/chest/NPC object cached in Player's
   // static visibleObjects Vector (j.al, 13 slots) -- far slots (8-12)
   // via the "far" drawers, mid slots (4-6) via the "mid" drawers, and
   // the single closest slot (1) via the shared position-code renderer.
   private void paintVisibleObjects(Graphics g) {
      monsterAttacking = false;

      for (int slot = 8; slot <= 12; slot++) {
         Object obj = j.al.elementAt(slot);
         if (obj instanceof byte[]) {
            byte[] rec = (byte[])obj;
            if (rec.length == 28) {
               if (rec[6] != 0) {
                  monsterAttacking = true;
                  this.drawMonsterFar(g, this.farMonsterIconFor(rec[2]), slot);
               }
            } else if (rec.length == 8 || rec.length == 7) {
               this.drawFarLootIcon(g, rec.length == 8, slot);
            }
         } else if (obj instanceof String) {
            String tag = (String)obj;
            if (tag.equals("W")) {
               this.drawMonsterFar(g, 25, slot);
            } else if (!tag.equals("C") && !tag.equals("D") && !tag.equals(Shop.NAMES[0]) && !tag.equals(Shop.NAMES[1])) {
               this.drawMonsterFar(g, 13, slot);
            } else {
               this.drawMonsterFar(g, 6, slot);
            }
         }
      }

      for (int slot = 4; slot <= 6; slot++) {
         Object obj = j.al.elementAt(slot);
         if (obj instanceof byte[]) {
            byte[] rec = (byte[])obj;
            if (rec.length == 28) {
               if (rec[6] != 0) {
                  monsterAttacking = true;
                  this.drawMonsterMid(g, this.midMonsterIconFor(rec[2]), slot);
               }
            } else if (rec.length == 8 || rec.length == 7) {
               this.drawMidLootIcon(g, rec.length == 8, slot);
            }
         } else if (obj instanceof String) {
            String tag = (String)obj;
            if (tag.equals("W")) {
               this.drawMonsterMid(g, 24, slot);
            } else if (!tag.equals("C") && !tag.equals("D") && !tag.equals(Shop.NAMES[0]) && !tag.equals(Shop.NAMES[1])) {
               this.drawMonsterMid(g, 12, slot);
            } else {
               this.drawMonsterMid(g, 5, slot);
            }
         }
      }

      Object obj = j.al.elementAt(1);
      if (obj instanceof byte[]) {
         byte[] rec = (byte[])obj;
         if (rec.length == 28) {
            if (rec[6] != 0) {
               monsterAttacking = true;
               this.paintObjectAtPosition(g, rec[2], -1);
            }
         } else if (rec.length == 8 || rec.length == 7) {
            this.drawCenterLootIcon(g, rec.length == 8);
         }
      }
   }

   private void drawCenterLootIcon(Graphics g, boolean isChest) {
      byte x = 60;
      int y = 110;
      if (isChest) {
         g.drawImage(chestSprites[0], x, y, 20);
      } else {
         y += 14;
         g.drawImage(itemBagSprites[0], x, y, 20);
      }
   }

   private void drawMidLootIcon(Graphics g, boolean isChest, int slot) {
      short x = 0;
      int y = 97;
      switch (slot) {
         case 4:
            x = 14;
            break;
         case 5:
            x = isChest ? 68 : 73;
            break;
         case 6:
            x = isChest ? 125 : 142;
      }

      if (isChest) {
         g.drawImage(chestSprites[1], x, y, 20);
      } else {
         y += 8;
         g.drawImage(itemBagSprites[1], x, y, 20);
      }
   }

   private void drawFarLootIcon(Graphics g, boolean isChest, int slot) {
      short x = 0;
      byte y = 87;
      switch (slot) {
         case 8:
            x = 10;
            break;
         case 9:
            x = 46;
            break;
         case 10:
            x = 84;
            break;
         case 11:
            x = 120;
            break;
         case 12:
            x = 156;
      }

      if (isChest) {
         g.drawImage(chestSprites[2], x, y, 20);
      } else {
         g.drawImage(itemBagSprites[2], x, y, 20);
      }
   }

   // Position-code (1-42) -> mid-slot monster icon index.
   private int midMonsterIconFor(int posCode) {
      if (posCode >= 1 && posCode <= 5) {
         return 5;
      } else if (posCode >= 6 && posCode <= 10) {
         return 12;
      } else if (posCode >= 11 && posCode <= 25) {
         return 21;
      } else if (posCode >= 26 && posCode <= 40) {
         return 18;
      } else {
         return posCode != 41 && posCode != 42 ? -1 : 24;
      }
   }

   // Position-code (1-42) -> far-slot monster icon index.
   private int farMonsterIconFor(int posCode) {
      if (posCode >= 1 && posCode <= 5) {
         return 6;
      } else if (posCode >= 6 && posCode <= 10) {
         return 13;
      } else if (posCode >= 11 && posCode <= 25) {
         return 22;
      } else if (posCode >= 26 && posCode <= 40) {
         return 19;
      } else {
         return posCode != 41 && posCode != 42 ? -1 : 25;
      }
   }

   // Position-code (1-42) -> OBJECT_DRAW_TABLE row (0-4; 4 = the
   // special "stairs" bucket handled directly by paintObjectAtPosition).
   private int positionBucketFor(int posCode) {
      if (posCode >= 1 && posCode <= 5) {
         return 0;
      } else if (posCode >= 6 && posCode <= 10) {
         return 1;
      } else if (posCode >= 11 && posCode <= 25) {
         return 3;
      } else if (posCode >= 26 && posCode <= 40) {
         return 2;
      } else {
         return posCode != 41 && posCode != 42 ? -1 : 4;
      }
   }

   // Shared "draw one object at a canned corridor position" renderer,
   // used for the closest monster slot, all 9 NPC portraits, and the
   // stairs icon. `frameOverride` >= 0 overrides the table's default
   // sprite frame (used by the NPC portrait dispatcher); -1 uses the
   // table default (used for the closest monster).
   private void paintObjectAtPosition(Graphics g, int posCode, int frameOverride) {
      int bucket = this.positionBucketFor(posCode);
      if (bucket == 4) {
         this.paintStairsIcon(g, posCode == 41);
      } else {
         if (bucket >= 0) {
            byte baseX = OBJECT_DRAW_TABLE[bucket][2];
            byte baseY = OBJECT_DRAW_TABLE[bucket][3];
            byte iconA = OBJECT_DRAW_TABLE[bucket][4];
            byte frameCountA = OBJECT_DRAW_TABLE[bucket][5];
            int secondX = baseX + OBJECT_DRAW_TABLE[bucket][6];
            int secondY = baseY + OBJECT_DRAW_TABLE[bucket][7];
            byte iconB = OBJECT_DRAW_TABLE[bucket][8];
            byte frameCountB = OBJECT_DRAW_TABLE[bucket][9];
            boolean hasSecond;
            if (iconB >= 0) {
               hasSecond = true;
            } else {
               hasSecond = false;
            }

            byte frameA = OBJECT_ICON_TABLE[posCode - 1][0];
            int frameB = OBJECT_ICON_TABLE[posCode - 1][1];
            if (frameOverride >= 0) {
               frameB = frameOverride;
            }

            boolean extra0 = OBJECT_EXTRA_FLAGS[posCode - 1][0];
            boolean extra1 = OBJECT_EXTRA_FLAGS[posCode - 1][1];
            boolean extra2 = OBJECT_EXTRA_FLAGS[posCode - 1][2];
            boolean extra3 = OBJECT_EXTRA_FLAGS[posCode - 1][3];
            this.drawSpriteFrame(g, objectSprites[iconA], frameA, frameCountA, baseX, baseY);
            if (hasSecond) {
               this.drawSpriteFrame(g, objectSprites[iconB], frameB, frameCountB, secondX, secondY);
            }

            if (extra0) {
               int x = baseX + OBJECT_DRAW_TABLE[bucket][10];
               int y = baseY + OBJECT_DRAW_TABLE[bucket][11];
               byte icon = OBJECT_DRAW_TABLE[bucket][12];
               this.drawSpriteFrame(g, objectSprites[icon], 0, 1, x, y);
            }

            if (extra1) {
               int x = baseX + OBJECT_DRAW_TABLE[bucket][13];
               int y = baseY + OBJECT_DRAW_TABLE[bucket][14];
               byte icon = OBJECT_DRAW_TABLE[bucket][15];
               this.drawSpriteFrame(g, objectSprites[icon], 0, 1, x, y);
            }

            if (extra2) {
               int x = baseX + OBJECT_DRAW_TABLE[bucket][16];
               int y = baseY + OBJECT_DRAW_TABLE[bucket][17];
               byte icon = OBJECT_DRAW_TABLE[bucket][18];
               this.drawSpriteFrame(g, objectSprites[icon], 0, 1, x, y);
            }

            if (extra3) {
               int x = baseX + OBJECT_DRAW_TABLE[bucket][19];
               int y = baseY + OBJECT_DRAW_TABLE[bucket][20];
               byte icon = OBJECT_DRAW_TABLE[bucket][21];
               this.drawSpriteFrame(g, objectSprites[icon], 0, 1, x, y);
            }
         }

         g.setClip(0, 0, this.screenWidth, this.screenHeight);
      }
   }

   private void drawMonsterMid(Graphics g, int iconIdx, int slot) {
      byte x = 0;
      byte y = 38;
      switch (slot) {
         case 4:
            x = 10;
            break;
         case 5:
            x = 62;
            break;
         case 6:
            x = 112;
      }

      g.drawImage(objectSprites[iconIdx], x, y, 20);
   }

   private void drawMonsterFar(Graphics g, int iconIdx, int slot) {
      short x = 0;
      byte y = 44;
      switch (slot) {
         case 8:
            x = 10;
            break;
         case 9:
            x = 44;
            break;
         case 10:
            x = 79;
            break;
         case 11:
            x = 112;
            break;
         case 12:
            x = 146;
      }

      g.drawImage(objectSprites[iconIdx], x, y, 20);
   }

   // The 3-bar Health/Magicka/Fatigue(?) HUD meter.
   private void paintStatusBars(Graphics g) {
      g.setColor(16776960);
      g.fillRect(5, 130, 40, 7);
      g.fillRect(5, 138, 40, 7);
      g.fillRect(5, 146, 40, 7);
      g.setColor(16711680);
      int fill = this.player.l(2) * 38 / this.player.E[3];
      g.fillRect(6, 131, fill, 5);
      g.setColor(65280);
      fill = this.player.l(4) * 38 / this.player.E[5];
      g.fillRect(6, 139, fill, 5);
      g.setColor(255);
      fill = this.player.l(6) * 38 / this.player.E[7];
      if (fill > 40) {
         fill = 40;
      }

      g.fillRect(6, 147, fill, 5);
   }

   private void paintMessagePopup(Graphics g) {
      if (messageVisible) {
         g.setColor(13080935);
         g.fillRoundRect(96, 118, 75, 35, 5, 5);
         g.setFont(SMALL_FONT);
         g.setColor(0);
         g.drawString(messageLines[0], 100, 122, 20);
         if (messageLines.length > 1) {
            g.drawString(messageLines[1], 100, 134, 20);
         }
      }
   }

   // The bottom panel + numeric hotbar, whose digits/labels depend on
   // computeHotbarContext (0 = explore, 1 = combat, 2 = chest/NPC).
   private void paintHotbar(Graphics g) {
      g.setFont(SMALL_FONT);
      g.setClip(0, 0, this.screenWidth, this.screenHeight);
      g.drawImage(panelImage, 0, 156, 20);
      g.setColor(0);
      int context = this.computeHotbarContext();
      hotbarContext = context;
      if (context == 0) {
         g.drawChar(HOTBAR_DIGIT_CHARS[1], 26, 191, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[2], 66, 191, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[3], 106, 191, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[5], 146, 191, 20);
         g.setColor(16777215);
         g.drawChar(HOTBAR_DIGIT_CHARS[1], 25, 190, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[2], 65, 190, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[3], 105, 190, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[5], 145, 190, 20);
         this.drawHotbarIcon(g, 1, 13, 164);
         this.drawHotbarIcon(g, 2, 53, 164);
         this.drawHotbarIcon(g, 3, 93, 164);
         this.drawHotbarIcon(g, 5, 133, 164);
      } else if (context == 1) {
         g.drawChar(HOTBAR_DIGIT_CHARS[0], 26, 191, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[1], 66, 191, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[2], 106, 191, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[3], 146, 191, 20);
         g.setColor(16777215);
         g.drawChar(HOTBAR_DIGIT_CHARS[0], 25, 190, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[1], 65, 190, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[2], 105, 190, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[3], 145, 190, 20);
         this.drawHotbarIcon(g, 0, 13, 164);
         this.drawHotbarIcon(g, 1, 53, 164);
         this.drawHotbarIcon(g, 2, 93, 164);
         this.drawHotbarIcon(g, 3, 133, 164);
      } else if (context == 2) {
         g.drawChar(HOTBAR_DIGIT_CHARS[1], 26, 191, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[2], 66, 191, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[3], 106, 191, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[4], 146, 191, 20);
         g.setColor(16777215);
         g.drawChar(HOTBAR_DIGIT_CHARS[1], 25, 190, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[2], 65, 190, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[3], 105, 190, 20);
         g.drawChar(HOTBAR_DIGIT_CHARS[4], 145, 190, 20);
         this.drawHotbarIcon(g, 1, 13, 164);
         this.drawHotbarIcon(g, 2, 53, 164);
         this.drawHotbarIcon(g, 3, 93, 164);
         this.drawHotbarIcon(g, 4, 133, 164);
      }

      g.setClip(0, 0, this.screenWidth, this.screenHeight);
   }

   private void drawHotbarIcon(Graphics g, int iconIdx, int x, int y) {
      g.setClip(x, y, 30, 24);
      g.drawImage(iconsSprite, x - 30 * iconIdx, y, 20);
   }

   // 0 = exploring, 1 = monsterTargeted (combat), 2 = chestInSight or
   // npcInSight (interact).
   private int computeHotbarContext() {
      if (monsterTargeted) {
         return 1;
      } else {
         return !chestInSight && npcInSight < 0 ? 0 : 2;
      }
   }

   // Generic "draw one frame of a horizontally-tiled sprite sheet,
   // clipped to its own frame cell" helper.
   private void drawSpriteFrame(Graphics g, Image sheet, int frame, int frameCount, int x, int y) {
      int frameWidth = sheet.getWidth() / frameCount;
      int frameHeight = sheet.getHeight();
      g.setClip(x, y, frameWidth, frameHeight);
      g.drawImage(sheet, x - frame * frameWidth, y, 20);
   }

   // Paints the minimap's colored-square grid (from visibleTileGrid)
   // into whatever Graphics it's given -- used by refreshMinimap to
   // render into the offscreen minimapImage.
   private void paintMinimapGrid(Graphics g, int border, int origin, int gridSize, int cellSize) {
      g.setColor(0);
      g.fillRect(0, 0, gridSize * cellSize + border << 1, gridSize * cellSize + border << 1);
      g.setColor(16777215);
      g.drawRect(0, 0, gridSize * cellSize + border << 0, gridSize * cellSize + border << 0);
      if (border == 2) {
         g.drawRect(1, 1, gridSize * cellSize + border << -1, gridSize * cellSize + border << -1);
      }

      int center = gridSize / 2;

      for (int row = 0; row < gridSize; row++) {
         int y = origin + row * cellSize;

         for (int col = 0; col < gridSize; col++) {
            int x = border + col * cellSize;
            if (row == center && col == center) {
               g.setColor(65280);
               g.fillRect(x, y, cellSize, cellSize);
            } else {
               byte tile = visibleTileGrid[col][row];
               if (tile != 1) {
                  if (tile == 0) {
                     g.setColor(16777215);
                     g.fillRect(x, y, cellSize, cellSize);
                  } else if ((tile & 2) != 0) {
                     g.setColor(16711680);
                     g.fillRect(x, y, cellSize, cellSize);
                  } else if ((tile & 4) != 0) {
                     g.setColor(255);
                     g.fillRect(x, y, cellSize, cellSize);
                  } else if ((tile & 8) != 0) {
                     g.setColor(13369599);
                     g.fillRect(x, y, cellSize, cellSize);
                  }
               }
            }
         }
      }
   }

   // Rebuilds the minimap thumbnail by sampling the Dungeon's view grid
   // around the player (normal: 7x7 at 3px/cell; zoomed out: 17x17 at
   // 5px/cell) and painting it into minimapImage's own Graphics.
   void refreshMinimap() {
      this.minimapDirty = false;
      byte x = this.player.x;
      byte y = this.player.w;
      byte facing = this.player.aw;
      if (!minimapZoomedOut) {
         this.player.a().a(x, y, facing, 7, visibleTileGrid);
         this.paintMinimapGrid(this.minimapImage.getGraphics(), 1, 1, 7, 3);
      } else {
         this.player.a().a(x, y, facing, 17, visibleTileGrid);
         this.paintMinimapGrid(this.minimapImage.getGraphics(), 2, 2, 17, 5);
      }
   }

   public void keyPressed(int key) {
      if (this.activeScreen != null) {
         this.activeScreen.handleKey(key);
      } else {
         this.lastGameAction = this.heldGameAction;
         if (key == 49) {
            if (hotbarContext == 1) {
               attackRequested = true;
            }
         } else if (key == 50) {
            this.pendingMoveDir = 1;
         } else if (key == 51) {
            castSpellRequested = true;
         } else if (key == 52) {
            this.strafeMove = true;
            this.pendingMoveDir = 4;
         } else if (key == 53) {
            spellCycleRequested = true;
         } else if (key == 54) {
            this.strafeMove = true;
            this.pendingMoveDir = 3;
         } else if (key == 55) {
            optionsRequested = true;
         } else if (key == 56) {
            this.pendingMoveDir = 2;
         } else if (key == 57) {
            if (hotbarContext == 2) {
               interactRequested = true;
            } else {
               unusedKey9Request = true;
            }
         } else if (key == 48) {
            if (hotbarContext == 0) {
               campRequested = true;
            }
         } else if (key == 42) {
            minimapZoomedOut = !minimapZoomedOut;
            this.minimapDirty = true;
         } else {
            this.strafeMove = false;
            this.heldGameAction = this.getGameAction(key);
            switch (this.heldGameAction) {
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
   }

   public void keyReleased(int key) {
      int action = this.getGameAction(key);
      this.lastGameAction = this.heldGameAction;
      this.heldGameAction = 0;
   }

   void stopGameThread() {
      if (this.gameThread != null) {
         this.threadRunning = false;
         if (this.gameThread.isAlive()) {
            System.out.println("Killing game thread");
            this.threadKillRequested = true;

            try {
               this.gameThread.join();
            } catch (Exception e) {
            }

            System.out.println("Done killing game thread");
         }

         this.gameThread = null;
         this.threadRunning = false;
         System.gc();
      }
   }

   void startGameThread() {
      try {
         this.stopGameThread();
         this.gameThread = new Thread(this);
         this.threadRunning = true;
         this.gameThread.start();
      } catch (Throwable t) {
         System.out.println(" start error:");
         t.printStackTrace();
         this.repaint();
         this.serviceRepaints();
      }
   }

   // The ~4Hz main game loop: dispatches camping/death/respawn state,
   // then (if none of those consumed the tick) input actions, monster
   // AI, and idle regen; repaints if canvasActive; and paces itself to a
   // steady 250ms tick via Thread.sleep.
   public void run() {
      long now = System.currentTimeMillis();
      long elapsed = 0L;
      long secondAccum = 0L;

      try {
         while (this.threadRunning) {
            if (this.activeScreen != null && this.repaintPending) {
               this.repaintPending = false;
               this.repaint();
               this.serviceRepaints();

               try {
                  Thread.sleep(100L);
               } catch (Exception e) {
               }
            } else if (this.threadPaused) {
               long pauseStart = System.currentTimeMillis();

               try {
                  Thread.sleep(250L);
               } catch (Exception e) {
               }

               if (this.threadKillRequested) {
                  this.threadKillRequested = false;
                  return;
               }
            } else {
               boolean runTick = true;
               actionTakenThisTick = false;
               System.gc();
               if (this.campState == 1 || this.campState == 3) {
                  runTick = false;
                  if (now - this.campStartTime > 2500L) {
                     if (this.campState != 3 && !this.rollCampInterrupted()) {
                        this.campState = 2;
                     } else {
                        this.campStartTime = 0L;
                        this.suppressMoveInput = true;
                        this.player.h(false);
                        if (this.campState == 3) {
                           if (!this.player.a().a(this.player.x, this.player.w, 41)) {
                           }
                        } else {
                           this.player.a().a(this.player.x, this.player.w, -1);
                        }

                        this.campState = 0;
                        runTick = true;
                        if (this.showMessage(MSG_REST_DISTURBED, 1)) {
                           messageShownAt = now;
                           messageVisible = true;
                        }
                     }
                  }
               } else if (this.campState == 2) {
                  runTick = false;
                  if (now - this.campStartTime > 5000L) {
                     this.campState = 0;
                     this.campStartTime = 0L;
                     this.suppressMoveInput = true;
                     this.player.h(true);
                     if (this.showMessage(MSG_REST_COMPLETE, 1)) {
                        messageShownAt = now;
                        messageVisible = true;
                     }

                     runTick = true;
                  }
               } else if (this.deathState != 1) {
                  if (this.deathState == 2) {
                     this.deathState = 3;
                     messageVisible = false;
                     messagePriority = 0;
                  }

                  runTick = false;
                  if (now - this.deathTime > 5000L) {
                     System.out.println("Restart after dead");
                     this.player.a(this.player.E);

                     for (int slot = this.player.aq - 1; slot >= 0; slot--) {
                        if (!this.player.A(slot)) {
                           this.player.w(slot);
                        }
                     }

                     this.player.T = false;
                     this.player.f(true);
                     this.deathTime = 0L;
                     this.deathState = 1;
                     Shop.showDeathGreeting = true;
                     this.suppressMoveInput = true;
                     this.minimapDirty = true;
                     showLevelNameMessage = true;
                     runTick = true;
                     if (showLevelNameMessage) {
                        if (this.showMessage(this.wrapToTwoLines(this.player.a().a()), 1)) {
                           messageShownAt = System.currentTimeMillis();
                           messageVisible = true;
                        }

                        showLevelNameMessage = false;
                     }
                  }
               }

               if (runTick) {
                  byte flags = this.player.a().a(now, this.player);
                  if ((flags & 1) != 0) {
                     this.minimapDirty = true;
                  }

                  if ((flags & 2) != 0 && this.showMessage(MSG_CREATURE_ATTACKS, 2)) {
                     messageShownAt = now;
                     messageVisible = true;
                  }

                  this.dispatchTickActions(now);
                  this.processIdleTick(now, elapsed);
                  if (this.player.I) {
                     this.player.I = false;
                     this.pauseThread();
                     this.game.LevelUpUI = this.game.newLevelUpUI(1);
                     this.game.setCurrentDisplay(this.game.LevelUpUI);
                     canvasActive = false;
                  }

                  this.player.c(false);
                  if (this.minimapDirty) {
                     this.refreshMinimap();
                  }

                  this.suppressMoveInput = false;
               }

               if (canvasActive) {
                  this.repaint();
                  this.serviceRepaints();
               }

               long tickDuration = System.currentTimeMillis() - now;
               lastTickDuration = tickDuration;

               try {
                  if (tickDuration < 250L) {
                     long sleepFor = 250L - tickDuration;
                     Thread.sleep(250L - tickDuration);
                  }
               } catch (Exception e) {
               }

               if (this.threadKillRequested) {
                  this.threadKillRequested = false;
                  return;
               }

               long prevNow = now;
               now = System.currentTimeMillis();
               elapsed = now - prevNow;
               this.tickStatusCountdowns(elapsed);
               secondAccum += elapsed;
               if (secondAccum > 1000L) {
                  secondAccum -= 1000L;
                  this.tickPerSecond();
               }

               if (now - messageShownAt > 3000L) {
                  messageVisible = false;
                  messagePriority = 0;
               }
            }
         }
      } catch (OutOfMemoryError e) {
         System.out.println("Out of memory");
      } catch (Throwable t) {
         System.out.println("ERROR: An error was thrown in GameCanvas run method!");
         System.out.println(t);
         t.printStackTrace();
         errorState = true;
         errorMessage = String.valueOf(t);
         this.repaint();
         this.serviceRepaints();

         try {
            Thread.sleep(10000L);
         } catch (Throwable e) {
         }

         this.game.exit();
      }
   }

   // Dispatches whichever single requested action (camp/cast/rest-check/
   // spell-cycle/attack/options/interact) is pending this tick, in
   // priority order, falling back to movement if none is pending.
   private void dispatchTickActions(long now) {
      if (campRequested) {
         if (monsterAttacking) {
            if (this.showMessage(MSG_CANNOT_CAMP, 1)) {
               messageShownAt = now;
               messageVisible = true;
            }

            campRequested = false;
         } else {
            this.enterCampState(now);
         }
      } else if (interactRequested) {
         this.processInteract(now);
      } else if (castSpellRequested) {
         this.processSpellCast(now);
      } else if (spellCycleRequested) {
         this.cycleSelectedSpell(now);
      } else if (attackRequested) {
         this.processAttack(now);
      } else if (optionsRequested) {
         this.openOptionsMenu();
      } else if (unusedKey9Request) {
         unusedKey9Request = false;
      } else if ((this.pendingMoveDir != 0 || this.suppressMoveInput) && !this.suppressMoveInput) {
         this.commitMove();
      }

      this.refreshTargetMonster();
      this.resolveMonsterDeath();
   }

   private void processAttack(long now) {
      if (now - this.lastAttackTime >= 500L && targetMonster != null) {
         actionTakenThisTick = true;
         byte hpBefore = targetMonster.g;
         this.player.a(targetMonster);
         this.lastAttackTime = now;
         if (hpBefore > targetMonster.g) {
            monsterHitFlash = true;
         }
      }

      attackRequested = false;
   }

   // Handles the target monster dying: end-of-game triggers for the two
   // special monster-type ids (41/42), otherwise a normal death-loot
   // roll, then removes it from the level and applies the Player's
   // "Increase Harm" kill-heal bonus if active.
   private void resolveMonsterDeath() {
      if (targetMonster != null && targetMonster.g <= 0) {
         if (targetMonster.l == 41) {
            this.player.aj = true;
            this.player.M = false;
         }

         if (targetMonster.l == 42) {
            this.game.endOfGameUI = this.game.newEndOfGameUI();
            this.game.setCurrentDisplay(this.game.endOfGameUI);
         } else {
            targetMonster.a(false);
         }

         ESGame.removeMonster(this.player.ao, targetMonster.o, targetMonster.m);
         if (this.player.i(4)) {
            this.player.E[2] = (short)(this.player.E[2] + 3 * this.player.E[3] / 10);
            this.player.E[2] = (short)Math.min(this.player.E[2], this.player.E[3]);
         }

         if (this.showMessage(MSG_CREATURE_DEAD, 1)) {
            messageShownAt = System.currentTimeMillis();
            messageVisible = true;
         }

         targetMonster = null;
         monsterTargeted = false;
         this.minimapDirty = true;
      }
   }

   // Re-queries Player for the nearest attackable monster and shows its
   // name the moment one comes into range.
   private void refreshTargetMonster() {
      targetMonster = this.player.n();
      if (targetMonster != null) {
         monsterTargeted = true;
         if (this.showMessage(this.wrapToTwoLines(targetMonster.a()), 1)) {
            messageShownAt = System.currentTimeMillis();
            messageVisible = true;
         }
      } else {
         monsterTargeted = false;
      }
   }

   // Re-queries Player for a chest in front and shows the "Chest" popup
   // the moment one comes into sight.
   public void refreshChestInSight() {
      byte[] chest = this.player.g();
      if (chest != null) {
         chestInSight = true;
      } else {
         chestInSight = false;
      }

      if (chestInSight && this.showMessage(MSG_CHEST, 1)) {
         messageShownAt = System.currentTimeMillis();
         messageVisible = true;
      }
   }

   // Applies the pending move (Player.a(dir, strafe)), then handles the
   // fallout: end-of-game trigger, level-name message on a level change,
   // "found N items" message on auto-pickup, and refreshes NPC/chest
   // sighting for the new tile.
   private void commitMove() {
      if (this.pendingMoveDir != 0) {
         byte slotsBefore = this.player.aq;
         actionTakenThisTick = true;
         this.player.a(this.pendingMoveDir, this.strafeMove);
         if (j.R) {
            this.game.endOfGameUI = this.game.newEndOfGameUI();
            this.game.setCurrentDisplay(this.game.endOfGameUI);
            canvasActive = false;
         } else {
            if (this.player.L) {
               showLevelNameMessage = true;
               if (showLevelNameMessage) {
                  if (this.showMessage(this.wrapToTwoLines(this.player.a().a()), 1)) {
                     messageShownAt = System.currentTimeMillis();
                     messageVisible = true;
                  }

                  showLevelNameMessage = false;
               }
            }

            if (this.strafeMove) {
               this.strafeMove = false;
            }
         }

         this.pendingMoveDir = 0;
         int pickedUp = this.player.aq - slotsBefore;
         if (pickedUp == 1) {
            if (this.showMessage(this.buildFoundItemMessage(), -1)) {
               messageShownAt = System.currentTimeMillis();
               messageVisible = true;
            }
         } else if (pickedUp > 1 && this.showMessage(MSG_FOUND_SEVERAL_ITEMS, -1)) {
            messageShownAt = System.currentTimeMillis();
            messageVisible = true;
         }

         this.refreshChestInSight();
         this.refreshNpcInSight();
         this.minimapDirty = true;
      }
   }

   private void processSpellCast(long now) {
      if (castSpellRequested) {
         byte spellId = this.player.c;
         if (!Spell.isValidId(spellId)) {
            castSpellRequested = false;
            return;
         }

         if (Spell.byId(spellId).magickaCost > this.player.l(4)) {
            if (this.showMessage(MSG_NOT_ENOUGH_MAGICKA, 3)) {
               messageShownAt = now;
               messageVisible = true;
            }
         } else if (now - this.lastSpellCastTime >= 500L) {
            actionTakenThisTick = true;
            if (Spell.isOffensive(spellId)) {
               if (!monsterTargeted) {
                  if (this.showMessage(MSG_NO_MONSTER, 1)) {
                     messageShownAt = now;
                     messageVisible = true;
                  }
               } else {
                  this.player.b(spellId, targetMonster);
                  spellHitFlash = true;
               }
            } else {
               this.player.m(spellId);
               selfSpellFlash = true;
            }

            this.lastSpellCastTime = now;
         }

         castSpellRequested = false;
      }
   }

   private void cycleSelectedSpell(long now) {
      if (spellCycleRequested) {
         int spellId = this.player.k();
         if (spellId == 0) {
            if (this.showMessage(MSG_NO_SPELLS, -1)) {
               messageShownAt = now;
               messageVisible = true;
            }
         } else {
            this.player.c = (byte)spellId;
            if (this.showMessage(this.wrapToTwoLines(Spell.byId(spellId).name), -1)) {
               messageShownAt = now;
               messageVisible = true;
            }
         }

         spellCycleRequested = false;
      }
   }

   // Opens the NPC-in-front's dialogue, or loots the chest in front:
   // whichever's actually present (npcInSight takes priority).
   private void processInteract(long now) {
      if (npcInSight >= 0) {
         this.openNpcDialogue(npcInSight);
      } else if (chestInSight) {
         byte[] chest = this.player.g();
         int result = this.player.a(chest);
         if (result == -1) {
            if (this.showMessage(MSG_CHEST_LOCKED, 4)) {
               messageShownAt = now;
               messageVisible = true;
            }
         } else if (result == 0) {
            if (this.showMessage(MSG_INVENTORY_FULL, -1)) {
               messageShownAt = now;
               messageVisible = true;
            }
         } else {
            chestInSight = false;
            this.minimapDirty = true;
            if (this.showMessage(this.buildFoundItemMessage(), -1)) {
               messageShownAt = now;
               messageVisible = true;
            }
         }
      }

      interactRequested = false;
   }

   // Builds the "you found <item>" popup body for the item that was
   // just added to the last inventory slot.
   private String[] buildFoundItemMessage() {
      int slot = this.player.aq - 1;
      int itemId = Math.abs(this.player.af[slot]);
      return this.wrapToTwoLines(Item.nameOf(itemId));
   }

   private void openOptionsMenu() {
      this.game.setCurrentDisplay(this.game.OptionsUI);
      canvasActive = false;
      optionsRequested = false;
   }

   // Enters a new camp cycle: campState 1 by default (roll for
   // interruption after 2500ms), 2 if the Safe Camping buff is active or
   // we're in the hub town (no roll, just wait 5000ms), or 3 for the
   // rare scripted "disturbed" camp event (10% roll, character level >
   // 3, quest flag not yet set).
   private void enterCampState(long now) {
      this.campState = 1;
      if (this.player.b) {
         this.campState = 2;
      }

      if (!this.player.aj && this.player.E[0] > 3 && Util.randomInt(10) == 1) {
         this.campState = 3;
      }

      if (this.player.ao == 1) {
         this.campState = 2;
      }

      this.campStartTime = now;
      campRequested = false;
   }

   // Opens shop/NPC dialogue for `shopId`: either the generic greeting
   // popup (GenericInfoUI), or -- for Jakar's (shop 4) -- the full
   // choice menu.
   void openNpcDialogue(int shopId) {
      String line = Shop.dialogue(this.player, shopId, 1, 0);
      if (line != null) {
         this.game.GenericInfoUI.a(8);
         this.game.GenericInfoUI.a(Shop.NAMES[shopId], line);
         this.game.GenericInfoUI.i = shopId;
         this.game.setAidPointsForNPC(shopId);
         this.game.setCurrentDisplay(this.game.GenericInfoUI);
         canvasActive = false;
      } else if (shopId == 4) {
         this.game.setCurrentDisplay(this.game.NPCChoicesUI[4]);
         canvasActive = false;
      }
   }

   // Checks for HP hitting 0 (starts the death sequence) and, if no
   // action was taken this tick, runs Player's own passive per-tick
   // update.
   private void processIdleTick(long now, long elapsed) {
      int hp = this.player.l(2);
      if (hp <= 0) {
         monsterTargeted = false;
         this.deathState = 2;
         this.deathTime = now;
      }

      if (!actionTakenThisTick) {
         this.player.a(elapsed);
      }
   }

   public void pauseThread() {
      this.threadPaused = true;
   }

   public void resumeThread() {
      this.threadPaused = false;
   }

   protected void showNotify() {
      if (this.activeScreen == null) {
         this.player.v();
         this.minimapDirty = true;
         this.refreshChestInSight();
         this.refreshNpcInSight();
         this.refreshTargetMonster();
         canvasActive = true;
         this.resumeThread();
         if (showLevelNameMessage) {
            if (this.showMessage(this.wrapToTwoLines(this.player.a().a()), 1)) {
               messageShownAt = System.currentTimeMillis();
               messageVisible = true;
            }

            showLevelNameMessage = false;
         }
      }
   }

   // 10% chance, used both for the camp-interruption roll and the
   // scripted campState==3 event roll.
   private boolean rollCampInterrupted() {
      int roll = Util.randomInt(10);
      return roll == 1;
   }

   // Counts down 3 of Player's timed status ailments (bits 3/4/6 of
   // Player.r) and applies the matching debuff bit once each expires.
   // The bit-6 ailment (Terrified) only counts down while a monster is
   // actively attacking (monsterAttacking).
   private void tickStatusCountdowns(long elapsed) {
      if (this.player.i(4)) {
         this.player.ar = (short)(this.player.ar - elapsed);
         if (this.player.ar < 0) {
            this.player.ar = 0;
            byte bit = 3;
            this.player.r = (byte)Util.setBit((int)bit, (int)this.player.r);
         }
      }

      if (this.player.i(5)) {
         this.player.O = (short)(this.player.O - elapsed);
         if (this.player.O < 0) {
            this.player.O = 0;
            byte bit = 4;
            this.player.r = (byte)Util.setBit((int)bit, (int)this.player.r);
         }
      }

      if (this.player.i(7) && monsterAttacking) {
         this.player.J = (short)(this.player.J - elapsed);
         if (this.player.J < 0) {
            this.player.J = 0;
            byte bit = 6;
            this.player.r = (byte)Util.setBit((int)bit, (int)this.player.r);
         }
      }
   }

   // Once-per-real-second passive tick: HP/Magicka regen-or-drain,
   // Player.D[] effect-duration countdowns, nearby monsters' lingering
   // attack-cooldown scratch bytes, and the scripted "overstayed in one
   // place" ambush spawner (Player.Q, two different checkpoint
   // schedules depending on Player.ah -- presumably normal vs. "New
   // Game+").
   private void tickPerSecond() {
      if (this.player.i(4)) {
         int drain = 2 * this.player.E[3] / 100;
         drain = Math.max(drain, 0);
         this.player.E[2] = (short)(this.player.E[2] - drain);
      }

      if (this.player.i(5)) {
         int regen = this.player.E[5] / 10;
         this.player.E[4] = (short)(this.player.E[4] + regen);
         if (this.player.E[4] >= this.player.E[5]) {
            this.player.E[4] = 0;
            int drain = this.player.E[5] / 10;
            this.player.E[2] = (short)(this.player.E[2] - drain);
         }
      }

      for (int i = 0; i < 25; i++) {
         if (this.player.D[i] > 0) {
            this.player.D[i]--;
            if (this.player.D[i] <= 0) {
               this.player.D[i] = 0;
               if (i == 5) {
                  int slot = this.player.n(101);
                  if (slot != -1) {
                     this.player.w(slot);
                  }
               }
            }
         }
      }

      Hashtable levelMonsters = ESGame.monsters[this.player.ao - 1];
      if (levelMonsters != null) {
         Enumeration monsters = levelMonsters.elements();
         Monster scratch = new Monster();

         while (monsters.hasMoreElements()) {
            byte[] rec = (byte[])monsters.nextElement();
            Monster.fromBytes(scratch, rec);
            if (scratch.scratch[6] != 0) {
               scratch.scratch[7]--;
               if (scratch.scratch[7] < 0) {
                  scratch.scratch[7] = 0;
                  scratch.scratch[6] = 0;
               }
            }
         }
      }

      if (this.player.Q >= 0) {
         int elapsedSeconds = ++this.player.Q;
         byte spawnType = -1;
         if (this.player.ah) {
            switch (elapsedSeconds) {
               case 3:
                  spawnType = 4;
                  break;
               case 20:
                  spawnType = 16;
                  break;
               case 35:
                  spawnType = 7;
                  break;
               case 38:
                  spawnType = 18;
                  break;
               case 53:
                  spawnType = 12;
                  break;
               case 68:
                  spawnType = 20;
                  break;
               case 70:
                  spawnType = 22;
                  break;
               case 85:
                  spawnType = 24;
                  break;
               case 100:
                  spawnType = 26;
                  break;
               case 115:
                  spawnType = 28;
                  break;
               case 117:
                  spawnType = 30;
                  break;
               case 127:
                  spawnType = 31;
                  break;
               case 132:
                  spawnType = 32;
            }
         } else {
            switch (elapsedSeconds) {
               case 5:
                  spawnType = 4;
                  break;
               case 16:
                  spawnType = 16;
                  break;
               case 28:
                  spawnType = 8;
                  break;
               case 38:
                  spawnType = 20;
                  break;
               case 42:
                  spawnType = 21;
                  break;
               case 55:
                  spawnType = 22;
                  break;
               case 66:
                  spawnType = 23;
                  break;
               case 77:
                  spawnType = 24;
                  break;
               case 88:
                  spawnType = 25;
                  break;
               case 99:
                  spawnType = 26;
                  break;
               case 105:
                  spawnType = 27;
                  break;
               case 115:
                  spawnType = 28;
                  break;
               case 118:
                  spawnType = 29;
                  break;
               case 127:
                  spawnType = 30;
                  break;
               case 132:
                  spawnType = 31;
            }
         }

         if (elapsedSeconds == 140) {
            spawnType = 42;
         }

         if (spawnType > 0) {
            int spawnX = 1 + Util.randomInt(17);

            for (int spawnY = 1 + Util.randomInt(17); !this.player.a().a(spawnX, spawnY, spawnType); spawnY = 1 + Util.randomInt(17)) {
               spawnX = 1 + Util.randomInt(17);
            }

            if (ESGame.monsters[this.player.ao - 1].size() > 5) {
               this.game.endOfGameUI = this.game.newGameOverUI();
               this.game.setCurrentDisplay(this.game.endOfGameUI);
            } else if (this.showMessage(MSG_ENEMY_ARRIVED, 3)) {
               messageShownAt = System.currentTimeMillis();
               messageVisible = true;
            }
         }
      }
   }

   public void addCommand(Command c) {
      if (this.activeScreen != null) {
         this.activeScreen.addCommand(c);
      }
   }

   public void removeCommand(Command c) {
      if (this.activeScreen != null) {
         this.activeScreen.removeCommand(c);
      }
   }

   public void setCommandListener(CommandListener listener) {
      if (this.activeScreen != null) {
         this.activeScreen.setCommandListener(listener);
      }
   }

   boolean isRunning() {
      return this.threadRunning;
   }
}
