// Renamed from decompiled/h.java. See ../docs/CLASS_MAP.md.
//
// Named UIScreen, not "Screen" -- this class itself holds a field typed
// `javax.microedition.lcdui.Screen` (the real MIDP class, used
// polymorphically to hold either a List or a Form), which would collide
// with a same-named class declared in the same default package.
//
// Despite the different name, this fills the combined role dawnstar splits
// into TWO classes: `Screen` (generic menu/dialog framework, mode fixed at
// construction) AND `LoadingScreen` (splash + progress-bar screens,
// extends Screen there). Confirmed here by the `mode` dispatch itself:
// modes 3/5/6 are list/prompt-list screens, mode 4 is a message screen,
// and modes 1/2/8-11 are the splash/progress-bar screens -- the EXACT same
// mode-number split dawnstar's g/h pair uses, just merged into one class
// with no subclassing.
//
// Real architectural difference from dawnstar's fully hand-painted UI:
// this class builds real MIDP `List`/`Form`/`ChoiceGroup`/`StringItem`
// widgets (`backingContainer`) purely as typed data containers (their
// built-in `append`/`get`/`getString` accessors), but NEVER actually
// displays them as Displayables -- all real painting is done by hand via
// Graphics (paint() below), manually reading strings back out of those
// containers. The widgets are reused as convenient typed string storage,
// not for their own rendering.
//
// Double-buffered manually: paint() draws into a static offscreen Image
// (`offscreenBuffer`) via its own Graphics, then blits that on top of the
// real Graphics passed in. `canvasInstances[2]`/`activeCanvasIndex` are two
// ScreenCanvas (FullCanvas) hosts that get alternated between (purpose of
// the alternation itself not confirmed -- possibly a workaround for a
// device-specific double-buffer/flicker quirk of this MIDP engine's era;
// dawnstar's single-canvas GameCanvas has no equivalent).
import java.util.Vector;
import javax.microedition.lcdui.ChoiceGroup;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.List;
import javax.microedition.lcdui.StringItem;

public class UIScreen implements Runnable {
   static ScreenCanvas[] canvasInstances = new ScreenCanvas[2];
   static int activeCanvasIndex = 0;
   static Image offscreenBuffer;
   Vector commands;
   CommandListener listener;
   static final Font commandFont;
   private static final Font titleFont;
   // Declared, constructed (Font.getFont(64,0,8)), but no confirmed read
   // site found in this pass -- possibly dead, possibly used by an
   // unread branch. Not renamed further than this placeholder.
   private static final Font unusedFont;
   static final Font itemFont;
   static final Font progressFont;
   static final Command cmdOk;
   static final Command cmdSelect;
   static final Command cmdCancel;
   static final Command cmdBack;
   static final Command cmdExit;
   // Copyright/credits text shown on the splash screen.
   static final String[] creditsLines;

   // Paint-dispatch mode, fixed at construction (matches dawnstar's own
   // Screen.mode design exactly -- see this file's header comment for the
   // confirmed mode-number mapping).
   int mode;
   // TODO: constructor's 3rd param, compared against
   // `ESGame.activeScreen.screenGroup` in repaint() as a "still the active
   // screen" gate. Not confirmed whether this is a per-instance identity
   // tag, a screen-group/category id, or something else -- every instance
   // just stores whatever its constructor caller passed.
   int screenGroup;
   // A reusable per-screen scratch int, repurposed by whichever screen
   // needs it: ESGame.java's own dispatcher stores a shopId in it for
   // several shop-result screens and a level-up attribute-choice index
   // for levelUpUI (confirmed by many read/write sites there), and
   // GameCanvas.talkToNpc() (M44) stores an npcId in it on npcHelloUI --
   // this comment used to claim no read/write site had been found, which
   // was already stale before M44 even added its own.
   int contextIndex;
   private ESGame game;
   // Real MIDP Screen (List or Form), used as a pure data container -- see
   // this file's header comment.
   javax.microedition.lcdui.Screen backingContainer;
   // 1 = backingContainer is a List, 2 = backingContainer is a Form.
   int layoutKind;
   // Index of the ChoiceGroup item within the Form, for modes 5/6.
   private int choiceGroupIndex;
   // TODO: stored at setup time, passed through to some caller -- purpose
   // not confirmed in this pass.
   Vector itemUserData;
   // Set to a copy of the message/prompt text when it contains the literal
   // "<TAG>" placeholder -- likely feeds NPC-dialogue-style substitution,
   // consistent with Util.replace's <TAG> convention.
   String tagTemplate;
   // TODO: declared, never seen assigned or read in what's been traced.
   String[] unconfirmedE;
   // Splash/progress-bar percent complete (0-100) -- matches dawnstar's
   // LoadingScreen.percent. Updated externally by whatever's loading.
   volatile int progressPercent;
   private boolean splashThreadRunning;
   private Thread splashThread;
   long splashElapsedMs;
   // The Displayable (a ScreenCanvas host) this screen instance targets --
   // set to canvasInstances[activeCanvasIndex] at setup time.
   Displayable targetDisplayable;
   // TODO: two untyped scratch fields, purpose not confirmed. `backTarget`
   // is nulled at construction and never seen touched again in what's been
   // traced; `nextScreen` is nulled at construction and is read once, at
   // the end of the splash thread (m()/runSplash below), passed to
   // `ESGame`'s own screen-transition method as "what to show next after
   // the splash finishes" -- so nextScreen at least is very likely a
   // "next screen" handle, just not confirmed which real type it holds.
   Object backTarget;
   Object nextScreen;
   // Text layout cursor state used while painting wrapped lines.
   int lineHeight;
   int textY;
   int textX;
   Font activeFont;
   int leftMargin;
   int rightMargin;
   // Scroll window (first/last visible item index) for list-style modes.
   int windowStart;
   int windowEnd;
   // Word-wrapped line count for message-body modes.
   int wrappedLineCount;
   // true: every visible line highlighted (yellow) in the Form+ChoiceGroup
   // combo painter; false: only the first (header) line highlighted. Modal
   // painter behavior switch, exact trigger not traced further.
   boolean highlightAllLines = false;

   public UIScreen(ESGame game, int mode, int screenGroup) {
      this.game = game;
      this.mode = mode;
      this.screenGroup = screenGroup;
      this.contextIndex = 0;
      this.backingContainer = null;
      this.layoutKind = 0;
      this.backTarget = null;
      this.nextScreen = null;
      this.tagTemplate = null;
      this.unconfirmedE = null;
      this.splashThread = null;
      this.commands = new Vector(5);
   }

   // Plain scrollable list (10 visible), no highlight override.
   void setupList(String title, String[] items, Vector callbackData) {
      this.setupList(title, items, callbackData, true);
   }

   void setupList(String title, String[] items, Vector callbackData, boolean addCancel) {
      List list = new List(title, 3);
      this.backingContainer = list;
      this.layoutKind = 1;
      int n = items.length;

      for (int i = 0; i < n; i++) {
         list.append(items[i], null);
      }

      this.itemUserData = callbackData;
      this.windowStart = 0;
      this.windowEnd = 0;
      this.choiceGroupIndex = 0;
      this.targetDisplayable = canvasInstances[activeCanvasIndex];
      this.addCommand(cmdSelect);
      if (addCancel) {
         this.addCommand(cmdCancel);
      }

      this.setCommandListener(this.game);
   }

   // Builds the "Exiting" credits form and shows it (mode 2, splash-style
   // paint), used for the game's own exit confirmation/attribution screen.
   void showExitCredits() {
      String body = "";

      for (int i = 0; i < creditsLines.length; i++) {
         body = body + creditsLines[i];
      }

      this.setupMessage("Exiting", body);
      this.removeCommand(cmdOk);
      this.addCommand(cmdExit);
   }

   // Word-wrapped body message (11 visible lines), mode 4.
   void setupMessage(String title, String body) {
      this.setupMessage(title, body, false);
   }

   void setupMessage(String title, String body, boolean unusedFlag) {
      Form form = new Form(title);
      this.backingContainer = form;
      this.layoutKind = 2;
      StringItem item = new StringItem(null, body);
      form.append(item);
      this.leftMargin = 10;
      this.rightMargin = 10;
      this.windowStart = 0;
      this.windowEnd = 0;
      this.choiceGroupIndex = 0;
      this.targetDisplayable = canvasInstances[activeCanvasIndex];
      this.addCommand(cmdOk);
      this.setCommandListener(this.game);
   }

   // Word-wrapped prompt (mode 5) plus a selectable, per-item-wrapped
   // ChoiceGroup list (15 visible).
   void setupPromptList(String title, String prompt, String[] items, Vector callbackData) {
      this.setupPromptList(title, prompt, items, callbackData, false);
   }

   void setupPromptList(String title, String prompt, String[] items, Vector callbackData, boolean unusedFlag) {
      Form form = new Form(title);
      this.backingContainer = form;
      this.layoutKind = 2;
      StringItem item = new StringItem(null, prompt);
      form.append(item);
      this.leftMargin = 15;
      this.rightMargin = 15;
      ChoiceGroup choices = new ChoiceGroup(null, 1);
      int n = items.length;

      for (int i = 0; i < n; i++) {
         choices.append(items[i], null);
      }

      this.choiceGroupIndex = 1;
      form.append(choices);
      this.itemUserData = callbackData;
      this.windowStart = 0;
      this.windowEnd = 0;
      this.choiceGroupIndex = 0;
      this.targetDisplayable = canvasInstances[activeCanvasIndex];
      this.addCommand(cmdSelect);
      this.addCommand(cmdCancel);
      this.setCommandListener(this.game);
      if (prompt != null && prompt.indexOf("<TAG>") >= 0) {
         this.tagTemplate = new String(prompt);
      }
   }

   // Word-wrapped prompt PLUS a second footer body line, above the
   // selectable ChoiceGroup list (mode 6).
   void setupPromptList(String title, String prompt, String footer, String[] items, Vector callbackData) {
      Form form = new Form(title);
      this.backingContainer = form;
      this.layoutKind = 2;
      StringItem promptItem = new StringItem(null, prompt);
      form.append(promptItem);
      StringItem footerItem = new StringItem(null, footer);
      form.append(footerItem);
      this.leftMargin = 15;
      this.rightMargin = 15;
      ChoiceGroup choices = new ChoiceGroup(null, 1);
      int n = items.length;

      for (int i = 0; i < n; i++) {
         choices.append(items[i], null);
      }

      this.choiceGroupIndex = 2;
      form.append(choices);
      this.itemUserData = callbackData;
      this.targetDisplayable = canvasInstances[activeCanvasIndex];
      this.addCommand(cmdSelect);
      this.addCommand(cmdCancel);
      this.setCommandListener(this.game);
   }

   // Top-level paint dispatcher, called by ScreenCanvas.paint(). Draws into
   // the shared offscreen buffer via its own Graphics, then blits that
   // buffer onto the real `g` passed in -- manual double buffering.
   public void paint(Graphics g) {
      Graphics buf = offscreenBuffer.getGraphics();
      switch (this.mode) {
         case 1:
            System.out.println("        IN CANVAS DOWNLOAD PAINT!");
            break;
         case 2:
            this.paintSplash(buf);
            break;
         case 3:
            this.paintList(buf);
            break;
         case 4:
            this.paintMessage(buf);
            break;
         case 5:
            this.paintPromptList(buf, 1);
            break;
         case 6:
            this.paintPromptList(buf, 2);
         case 7:
         default:
            break;
         case 8:
         case 9:
         case 10:
         case 11:
            this.paintProgressBar(buf);
      }

      this.paintCommandBar(buf);
      g.drawImage(offscreenBuffer, 0, 0, 20);
   }

   // Mode 2: the startup splash/carrier-logo sequence. Draws either the
   // "carrier download" screen (while ESGame.showCredits is set), the ZeniMax/Vir2L
   // splash (while ESGame's toggle flag `ac` is set -- see runSplash()),
   // or the plain splash with a loading-percent bar.
   private void paintSplash(Graphics g) {
      g.setColor(0);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      if (ESGame.showCredits) {
         g.setColor(16777215);
         g.fillRect(0, 0, this.width(), 20 + this.height());
         g.drawImage(ESGame.mformaLogoImage, this.width() / 2, 10, 17);
         int y = 10 + ESGame.mformaLogoImage.getHeight() + 3;
         g.setColor(0);

         for (int i = 0; i < creditsLines.length; i++) {
            g.drawString(creditsLines[i], this.width() / 2, y, 17);
            y += 14;
         }

         g.drawString("Distributed by:", this.width() / 2, 143, 17);
         g.drawImage(ESGame.vir2lLogoImage, this.width() / 2, 158, 17);
      } else if (this.game.splashFadeStage) {
         g.setColor(0);
         g.fillRect(0, 0, this.width(), 20 + this.height());
         g.drawImage(ESGame.mformaLogoImage2, this.width() / 2, 20, 17);
         g.drawImage(ESGame.vir2lLogoImage2, this.width() / 2, 100, 17);
      } else {
         g.setColor(0);
         g.fillRect(0, 0, this.width(), 20 + this.height());
         g.drawImage(ESGame.mformaLogoImage2, this.width() / 2, 20, 17);
         g.drawImage(ESGame.vir2lLogoImage2, this.width() / 2, 100, 17);
         g.setColor(16777215);
         g.fillRect(12, 165, 152, 22);
         g.setColor(10485760);
         g.fillRect(13, 166, 3 * this.progressPercent / 2, 20);
      }
   }

   // Modes 8-11: plain "<action>... Please Wait" progress bar, action text
   // selected by mode.
   private void paintProgressBar(Graphics g) {
      g.setColor(11429934);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      g.setFont(progressFont);
      g.setColor(16777215);
      int cx = this.width() / 2;
      if (this.mode == 8) {
         g.drawString("Creating New Game", cx, 30, 17);
      } else if (this.mode == 9) {
         g.drawString("Loading Game", cx, 30, 17);
      } else if (this.mode == 10) {
         g.drawString("Saving Game", cx, 30, 17);
      } else if (this.mode == 11) {
         g.drawString("Loading Dungeon", cx, 30, 17);
      }

      g.drawString("Please Wait", cx, 45, 17);
      g.setColor(16777215);
      g.fillRect((this.width() - 90) / 2, 60, 90, 20);
      int fillWidth = this.progressPercent * 88 / 100;
      g.setColor(255);
      g.fillRect((this.width() - 88) / 2, 61, fillWidth, 18);
   }

   // Shared title bar, drawn at the top of list/message/prompt screens.
   private void paintTitleBar(Graphics g, String title) {
      Font savedFont = g.getFont();
      g.setFont(titleFont);
      int savedColor = g.getColor();
      g.setColor(0);
      g.fillRect(0, 0, this.width(), 14);
      g.setColor(16777215);
      g.drawString(title, this.width() / 2, 0, 17);
      g.setColor(savedColor);
      g.setFont(savedFont);
   }

   // Mode 3: plain scrollable List, the shared highlighted-row renderer
   // (10 visible lines, selection box behind the current row).
   private void paintList(Graphics g) {
      g.setColor(11429934);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      this.paintTitleBar(g, this.backingContainer.getTitle());
      this.activeFont = itemFont;
      g.setFont(this.activeFont);
      this.lineHeight = this.activeFont.getHeight();
      this.textY = 20;
      this.textX = 15;
      String[] items = this.visibleItemStrings();
      int n = items.length;
      byte maxVisible = 10;
      int visible = Math.min(n, maxVisible);
      this.windowEnd = this.windowStart + visible - 1;

      for (int i = this.windowStart; i <= this.windowEnd; i++) {
         this.paintRow(g, items[i], i == this.selectedIndex());
      }

      if (this.windowStart > 0) {
         this.paintScrollArrow(g, 155, 180, 1);
      }

      if (this.windowEnd + 1 < n) {
         this.paintScrollArrow(g, 165, 180, 2);
      }
   }

   // Modes 5/6: the shared highlighted-row renderer, used after painting
   // the Form's own header StringItem(s) (`headerLineCount` of them) either
   // plain (first line yellow, rest white) or all-yellow-wrapped
   // (highlightAllLines).
   private void paintPromptList(Graphics g, int headerLineCount) {
      Form form = (Form)this.backingContainer;
      g.setColor(11429934);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      this.paintTitleBar(g, form.getTitle());
      this.activeFont = itemFont;
      g.setFont(this.activeFont);
      this.lineHeight = this.activeFont.getHeight();
      this.textY = 20;
      this.textX = 15;

      for (int i = 0; i < headerLineCount; i++) {
         StringItem item = (StringItem)form.get(i);
         String text = item.getText();
         if (this.highlightAllLines) {
            String[] wrapped = this.wordWrap(text);

            for (int j = 0; j < wrapped.length; j++) {
               g.setColor(16776960);
               g.drawString(wrapped[j], this.textX, this.textY, 20);
               this.textY = this.textY + this.lineHeight;
            }
         } else {
            if (i == 0) {
               g.setColor(16776960);
            } else {
               g.setColor(16777215);
            }

            g.drawString(text, this.textX, this.textY, 20);
            this.textY = this.textY + this.lineHeight;
         }
      }

      this.textY += 5;
      String[] items = this.visibleItemStrings();
      int n = items.length;
      byte maxVisible = 9;
      int visible = Math.min(n, maxVisible);
      this.windowEnd = this.windowStart + visible - 1;

      for (int i = this.windowStart; i <= this.windowEnd; i++) {
         this.paintRow(g, items[i], i == this.selectedIndex());
      }

      if (this.windowStart > 0) {
         this.paintScrollArrow(g, 155, 180, 1);
      }

      if (this.windowEnd + 1 < n) {
         this.paintScrollArrow(g, 165, 180, 2);
      }
   }

   private void paintRow(Graphics g, String text, boolean selected) {
      if (selected) {
         g.setColor(6710886);
         int w = this.width() - 2 * (this.textX - 10);
         int h = this.lineHeight + 2;
         g.fillRect(this.textX - 10, this.textY - 1, w, h);
      }

      g.setColor(16776960);
      g.drawString(text, this.textX, this.textY, 20);
      this.textY = this.textY + this.lineHeight;
      this.textY++;
   }

   // Mode 4: word-wrapped message body (11 visible lines).
   private void paintMessage(Graphics g) {
      Form form = (Form)this.backingContainer;
      g.setColor(11429934);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      this.paintTitleBar(g, form.getTitle());
      this.activeFont = itemFont;
      g.setFont(this.activeFont);
      this.lineHeight = this.activeFont.getHeight();
      this.textY = 20;
      this.textX = 5;
      StringItem item = (StringItem)form.get(0);
      String body = item.getText();
      String[] wrapped = this.wordWrap(body);
      this.wrappedLineCount = wrapped.length;
      byte maxVisible = 11;
      int visible = Math.min(this.wrappedLineCount, maxVisible);
      this.windowEnd = this.windowStart + visible - 1;
      g.setColor(16776960);

      for (int i = this.windowStart; i <= this.windowEnd; i++) {
         g.drawString(wrapped[i], this.textX, this.textY, 20);
         this.textY = this.textY + this.lineHeight;
      }

      if (this.windowStart > 0) {
         this.paintScrollArrow(g, 155, 180, 1);
      }

      if (this.windowEnd + 1 < this.wrappedLineCount) {
         this.paintScrollArrow(g, 165, 180, 2);
      }
   }

   private void paintScrollArrow(Graphics g, int x, int y, int direction) {
      int savedColor = g.getColor();
      g.setColor(0);
      byte size = 5;
      if (direction == 1) {
         for (int i = 0; i < size; i++) {
            g.drawLine(x - i, y + i, x + i, y + i);
         }
      } else {
         for (int i = 0; i < size; i++) {
            g.drawLine(x - (size - i), y + i, x + (size - i), y + i);
         }
      }
   }

   // Word-wrap: split on explicit newlines first, then word-wrap each
   // resulting line to the current drawable width.
   String[] wordWrap(String text) {
      Vector out = new Vector();

      for (String line : this.splitOnNewlines(text)) {
         this.wordWrapInto(out, line);
      }

      return toArray(out);
   }

   private String[] splitOnNewlines(String text) {
      Vector out = new Vector();
      int start = 0;
      int nl;

      do {
         nl = text.indexOf(10, start);
         if (nl < 0) {
            nl = text.length();
            out.addElement(text.substring(start, nl));
            break;
         }

         out.addElement(text.substring(start, nl));
         start = nl + 1;
      } while (start < text.length());

      return toArray(out);
   }

   private void wordWrapInto(Vector out, String line) {
      int maxWidth = this.width() - this.leftMargin - this.rightMargin;
      String[] words = Util.splitWords(line);
      int n = words.length;
      String current = "";

      for (int i = 0; i < n; i++) {
         String word = words[i];
         String candidate = current + word;
         if (this.activeFont.stringWidth(candidate) > maxWidth) {
            if (this.activeFont.stringWidth(word) <= maxWidth) {
               out.addElement(new String(current));
               candidate = word;
            } else {
               if (current.length() > 0) {
                  out.addElement(new String(current));
               }

               String[] charWrapped = this.charWrap(word);

               for (int j = 0; j < charWrapped.length - 1; j++) {
                  out.addElement(new String(charWrapped[j]));
               }

               candidate = charWrapped[charWrapped.length - 1];
            }
         }

         current = candidate;
         if (i < n - 1) {
            candidate = current + " ";
            if (this.activeFont.stringWidth(candidate) > maxWidth) {
               out.addElement(new String(current));
               current = "";
            } else {
               current = candidate;
            }
         }
      }

      out.addElement(new String(current));
   }

   // Fallback for a single word wider than the drawable width: breaks it
   // mid-word, character by character.
   private String[] charWrap(String word) {
      int maxWidth = this.width() - this.leftMargin - this.rightMargin;
      Vector out = new Vector();
      int n = word.length();
      String current = "";

      for (int i = 0; i < n; i++) {
         char c = word.charAt(i);
         String candidate = current + c;
         if (this.activeFont.stringWidth(candidate) > maxWidth) {
            out.addElement(new String(current));
            current = c + "";
         } else {
            current = candidate;
         }
      }

      out.addElement(new String(current));
      return toArray(out);
   }

   private static String[] toArray(Vector v) {
      int n = v.size();
      String[] out = new String[n];

      for (int i = 0; i < n; i++) {
         out[i] = (String)v.elementAt(i);
      }

      return out;
   }

   // FullCanvas keyPressed forwarded from ScreenCanvas. -6/-7 are the
   // soft-key game actions (left/right MIDP soft keys); everything else is
   // up/down navigation for list-shaped modes (3/5/6) or plain scroll for
   // the message mode (4).
   protected void keyPressed(int key) {
      if (key == -6) {
         Command cmd = this.leftSoftKeyCommand();
         if (cmd != null) {
            this.listener.commandAction(cmd, this.targetDisplayable);
            return;
         }
      } else if (key == -7) {
         Command cmd = this.rightSoftKeyCommand();
         if (cmd != null) {
            this.listener.commandAction(cmd, this.targetDisplayable);
            return;
         }
      }

      int action = this.gameAction(key);
      switch (action) {
         case 1:
            if (this.mode == 3 || this.mode == 5 || this.mode == 6) {
               int sel = this.selectedIndex();
               if (sel > 0) {
                  this.setSelectedIndex(--sel);
                  if (this.windowStart > sel) {
                     this.windowStart--;
                     this.windowEnd--;
                  }

                  this.repaint();
                  this.serviceRepaints();
               }
            } else if (this.mode == 4) {
               if (this.windowStart > 0) {
                  this.windowStart--;
                  this.windowEnd--;
                  this.repaint();
                  this.serviceRepaints();
               }
            }
            break;
         case 6:
            if (this.mode == 3 || this.mode == 5 || this.mode == 6) {
               int sel = this.selectedIndex();
               if (sel < this.itemCount() - 1) {
                  this.setSelectedIndex(++sel);
                  if (this.windowEnd < sel) {
                     this.windowStart++;
                     this.windowEnd++;
                  }

                  this.repaint();
                  this.serviceRepaints();
               }
            } else if (this.mode == 4 && this.windowEnd < this.wrappedLineCount - 1) {
               this.windowStart++;
               this.windowEnd++;
               this.repaint();
               this.serviceRepaints();
            }
      }
   }

   // Returns the currently visible item strings for list-shaped modes
   // (from the List directly for mode 3, from the Form's ChoiceGroup for
   // modes 5/6); null for the message mode (4), which has no item list.
   String[] visibleItemStrings() {
      switch (this.mode) {
         case 3:
            List list = (List)this.backingContainer;
            int n = list.size();
            String[] out = new String[n];

            for (int i = 0; i < n; i++) {
               out[i] = list.getString(i);
            }

            return out;
         case 4:
         default:
            return null;
         case 5:
         case 6:
            Form form = (Form)this.backingContainer;
            ChoiceGroup choices = (ChoiceGroup)form.get(this.choiceGroupIndex);
            int m = choices.size();
            String[] out2 = new String[m];

            for (int i = 0; i < m; i++) {
               out2[i] = choices.getString(i);
            }

            return out2;
      }
   }

   String selectedItemString() {
      int i = this.selectedIndex();
      String[] items = this.visibleItemStrings();
      return items[i];
   }

   int selectedIndex() {
      switch (this.mode) {
         case 3:
            List list = (List)this.backingContainer;
            return list.getSelectedIndex();
         case 4:
         default:
            return -1;
         case 5:
         case 6:
            Form form = (Form)this.backingContainer;
            ChoiceGroup choices = (ChoiceGroup)form.get(this.choiceGroupIndex);
            return choices.getSelectedIndex();
      }
   }

   void setSelectedIndex(int index) {
      switch (this.mode) {
         case 3:
            List list = (List)this.backingContainer;
            list.setSelectedIndex(index, true);
         case 4:
         default:
            break;
         case 5:
         case 6:
            Form form = (Form)this.backingContainer;
            ChoiceGroup choices = (ChoiceGroup)form.get(this.choiceGroupIndex);
            choices.setSelectedIndex(index, true);
      }
   }

   // onEnter-shaped hook (matches dawnstar's Screen.onEnter position):
   // mode 2 starts the splash background thread.
   protected void onEnter() {
      switch (this.mode) {
         case 2:
            this.startSplashThread();
      }
   }

   // onExit-shaped hook: modes 1/2 stop the splash background thread.
   protected void onExit() {
      switch (this.mode) {
         case 1:
         case 2:
            this.stopSplashThread();
      }
   }

   private void startSplashThread() {
      System.out.println("IN START HELPER THREAD IN UICANVAS");
      this.splashThread = new Thread(this);
      System.out.println("Helper thread in UICanvas: " + this.splashThread);
      System.out.println("num active threads = " + Thread.activeCount());
      this.splashThreadRunning = true;
      this.splashThread.start();
   }

   private void stopSplashThread() {
      this.splashThreadRunning = false;
   }

   public void run() {
      if (this.mode == 2) {
         this.runSplash();
      } else if (this.mode == 1) {
         System.out.println("Running a download helper thread to repaint");
      }
   }

   // Splash sequencing: waits for progressPercent to hit 100 (with a
   // minimum 4-second floor), then a 2s+1s two-phase fade (toggling
   // ESGame.ac, the "which splash image" flag paintSplash() reads) before
   // clearing the splash images and handing off to whatever
   // `nextScreen` holds as "next screen".
   private void runSplash() {
      try {
         this.splashElapsedMs = 0L;
         System.out.println("Just before helper thread loop in UICanvas");

         while (this.splashThreadRunning && (this.progressPercent < 100 || this.splashElapsedMs < 4000L)) {
            long start = System.currentTimeMillis();
            this.repaint();
            this.serviceRepaints();
            long elapsed = System.currentTimeMillis() - start;

            try {
               if (elapsed < 500L) {
                  Thread.sleep(500L - elapsed);
               }
            } catch (Exception ignored) {
            }

            elapsed = System.currentTimeMillis() - start;
            this.splashElapsedMs += elapsed;
            System.out.println("Progress pct is " + this.progressPercent);
         }

         ESGame.showCredits = true;
         this.game.splashFadeStage = false;
         this.holdRepainting(2000L);
         this.game.splashFadeStage = true;
         ESGame.showCredits = false;
         this.holdRepainting(1000L);
         this.game.splashFadeStage = false;
         ESGame.vir2lLogoImage = null;
         ESGame.mformaLogoImage = null;
         ESGame.mformaLogoImage2 = null;
         ESGame.vir2lLogoImage2 = null;
         this.splashThread = null;
         ESGame.debugLog("After nuking splash");
         System.out.println("End of splash, changing to next");
         this.game.showScreen(this.nextScreen);
      } catch (Throwable t) {
         t.printStackTrace();
         this.game.showScreen(this.game.errorForm);
      }
   }

   private void holdRepainting(long durationMs) {
      long elapsed = 0L;

      do {
         this.repaint();
         this.serviceRepaints();

         try {
            Thread.sleep(500L);
         } catch (Exception ignored) {
         }

         elapsed += 500L;
      } while (elapsed <= durationMs);
   }

   int itemCount() {
      if (this.mode == 3) {
         List list = (List)this.backingContainer;
         return list.size();
      }

      if (this.mode != 5 && this.mode != 6) {
         return 0;
      }

      Form form = (Form)this.backingContainer;
      ChoiceGroup choices = (ChoiceGroup)form.get(this.choiceGroupIndex);
      return choices.size();
   }

   // Overwrites the text of Form item `index` -- used to patch a
   // header/footer line after setup (e.g. for <TAG> substitution).
   void setItemText(int index, String text) {
      Form form = (Form)this.backingContainer;
      if (this.mode == 6) {
         System.out.println("type is form2, size is " + form.size());
      }

      if (this.mode == 5 || this.mode == 6 || this.mode == 4) {
         StringItem item = (StringItem)form.get(index);
         item.setText(text);
      }
   }

   void setTitle(String title) {
      if (this.backingContainer != null) {
         this.backingContainer.setTitle(title);
      }
   }

   // Overwrites the body text of a mode-4 message screen's single
   // StringItem.
   void setMessageBody(String text) {
      Form form = (Form)this.backingContainer;

      try {
         if (form != null) {
            StringItem item = (StringItem)form.get(0);
            item.setText(text);
         }
      } catch (Throwable ignored) {
      }
   }

   String messageBody() {
      Form form = (Form)this.backingContainer;

      try {
         if (form != null) {
            StringItem item = (StringItem)form.get(0);
            return item.getText();
         }
      } catch (Throwable ignored) {
      }

      return null;
   }

   public void addCommand(Command cmd) {
      this.commands.addElement(cmd);
   }

   // Convenience aliases used by ESGame's screen-setup code -- softkey
   // position is auto-derived from WHICH static Command singleton is
   // present (see leftSoftKeyCommand/rightSoftKeyCommand below), so these
   // are just addCommand(cmd) under a name that documents intent at the
   // call site.
   void setOkCommand(Command cmd) {
      this.addCommand(cmd);
   }

   void setCancelCommand(Command cmd) {
      this.addCommand(cmd);
   }

   public void removeCommand(Command cmd) {
      this.commands.removeElement(cmd);
   }

   public void setCommandListener(CommandListener listener) {
      this.listener = listener;
   }

   // Bottom soft-key label bar, auto-derived from whichever of
   // cmdOk/cmdSelect/cmdCancel/cmdBack were added -- screens never hardcode
   // key positions.
   private void paintCommandBar(Graphics g) {
      if (this.commands.size() != 0) {
         g.setColor(16777215);
         g.fillRect(0, 190, this.width(), 20);
         g.setColor(0);
         g.setFont(commandFont);
         Command left = this.leftSoftKeyCommand();
         if (left != null) {
            g.drawString(left.getLabel(), 10, 192, 20);
         }

         Command right = this.rightSoftKeyCommand();
         if (right != null) {
            g.drawString(right.getLabel(), this.width() - 10, 195, 24);
         }
      }
   }

   private Command rightSoftKeyCommand() {
      int n = this.commands.size();
      Command result = null;
      if (n == 1) {
         result = (Command)this.commands.elementAt(0);
      } else if (n == 2) {
         for (int i = 0; i < 2; i++) {
            Command c = (Command)this.commands.elementAt(i);
            if (c == cmdOk || c == cmdSelect) {
               result = c;
               break;
            }
         }
      }

      return result;
   }

   private Command leftSoftKeyCommand() {
      int n = this.commands.size();
      Command result = null;
      if (n == 2) {
         for (int i = 0; i < 2; i++) {
            Command c = (Command)this.commands.elementAt(i);
            if (c == cmdBack || c == cmdCancel) {
               result = c;
               break;
            }
         }
      }

      return result;
   }

   static ScreenCanvas activeCanvas() {
      return canvasInstances[activeCanvasIndex];
   }

   // Repaint gate: only repaints if this screen is (or nothing is) the
   // game's currently-active screen -- see this file's screenGroup doc
   // comment.
   public void repaint() {
      if (ESGame.activeScreen == null || this.screenGroup == ESGame.activeScreen.screenGroup) {
         canvasInstances[activeCanvasIndex].repaint();
      }
   }

   public void serviceRepaints() {
      canvasInstances[activeCanvasIndex].serviceRepaints();
   }

   public int width() {
      return canvasInstances[activeCanvasIndex].getWidth();
   }

   public int height() {
      return canvasInstances[activeCanvasIndex].getHeight();
   }

   public int gameAction(int key) {
      return canvasInstances[activeCanvasIndex].getGameAction(key);
   }

   static {
      canvasInstances[0] = new ScreenCanvas();
      canvasInstances[1] = new ScreenCanvas();

      try {
         offscreenBuffer = Image.createImage(canvasInstances[0].getWidth(), canvasInstances[0].getHeight());
      } catch (Throwable t) {
         System.out.println("Error allocating bufferImage");
      }

      commandFont = Font.getFont(0, 1, 8);
      titleFont = Font.getFont(0, 1, 0);
      unusedFont = Font.getFont(64, 0, 8);
      itemFont = Font.getFont(0, 1, 8);
      progressFont = Font.getFont(0, 1, 0);
      cmdOk = new Command("Ok", 3, 0);
      cmdSelect = new Command("Select", 3, 0);
      cmdCancel = new Command("Cancel", 4, 0);
      cmdBack = new Command("Back", 4, 0);
      cmdExit = new Command("Exit", 7, 0);
      creditsLines = new String[]{
         "(c) 2003 Vir2L Studios, ",
         "a ZeniMax Media company. ",
         "The Elder Scrolls and Vir2L ",
         "are registered trademarks ",
         "of ZeniMax Media Inc. ",
         "All rights reserved."
      };
   }
}
