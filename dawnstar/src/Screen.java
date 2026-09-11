// Renamed from decompiled/g.java. See ../docs/CLASS_MAP.md.
//
// A single class implements every non-3D-view UI screen via a `mode`
// int set once at construction: 3/4 are both "scrollable text", 5/6 are
// "prompt + selectable list" (6 adds a second/footer text block). Modes
// 3 and 5/6 render through the shared highlighted-row renderer
// (renderItemRows); mode 4 has its own non-highlighted list renderer.
// Which `setup*` method a caller uses is a separate, orthogonal choice
// from `mode` -- by convention only certain (mode, setup method)
// combinations are actually used (e.g. ESGame always builds its
// GenericInfoUI-style popups with mode 4 + setupMessage).
import java.util.Vector;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;

public class Screen {
   Vector commands;
   CommandListener listener;
   static final Font SOFT_KEY_FONT = Font.getFont(0, 1, 8);
   private static final Font TITLE_FONT = Font.getFont(0, 1, 0);
   // Declared, never referenced elsewhere in this class -- vestigial.
   private static final Font UNUSED_FONT = Font.getFont(64, 0, 8);
   static final Font DEFAULT_TEXT_FONT = Font.getFont(0, 1, 8);
   static final Font LARGE_TEXT_FONT = Font.getFont(0, 1, 0);
   int mode;
   // Set from the constructor's 3rd arg; every call site so far passes a
   // distinct-looking numeric id (e.g. 410, 27, 304) whose purpose isn't
   // confirmed -- possibly a debug/support reference code, not consumed
   // by any rendering or input logic traced so far.
   int secondaryParam;
   // Set to 0 in the constructor, never reassigned in what's been
   // traced -- unconfirmed purpose.
   int unused1;
   ESGame game;
   // Raw text containing a literal "<TAG>" placeholder, stashed by
   // setupPromptList for later re-substitution (e.g. dynamic NPC
   // dialogue topics). Null otherwise.
   String rawTaggedText;
   // Type `e` (GameCanvas) is not mechanically renamed yet -- see
   // CLASS_MAP.md. Member names below are e's ORIGINAL (unrenamed) ones:
   // ay=width, l=height, an=dirty/needs-repaint flag, e()=isRunning,
   // a(text,maxWidth,font)=wrapText.
   e canvas;
   Object unused2;
   // Displayable to return to -- set externally (e.g. ESGame wires
   // LoadingScreen.returnDisplay = errorForm).
   Object returnDisplay;
   int lineHeight;
   int cursorY;
   Font textFont;
   int marginX;
   int marginRight;
   int scrollTop;
   int scrollBottom;
   int itemCount;
   String[] promptLines;
   String[] footerLines;
   String[] items;
   // Per-item -> first-visual-line index, when a long item got
   // word-wrapped into multiple visual lines (setupPromptList only) --
   // lets up/down navigation move by logical item, not visual line.
   // Null for setupList/setupMessage's un-grouped item arrays.
   int[] itemGroupStart = null;
   String title;
   int selectedIndex;

   public Screen(ESGame game, int mode, int secondaryParam) {
      this.game = game;
      this.canvas = this.game.gameCanvas;
      this.mode = mode;
      this.secondaryParam = secondaryParam;
      this.unused1 = 0;
      this.unused2 = null;
      this.returnDisplay = null;
      this.rawTaggedText = null;
      this.commands = new Vector(2);
      this.promptLines = null;
      this.footerLines = null;
      this.items = null;
      if (mode == 4) {
         this.addCommand(ESGame.okCommand);
         this.setCommandListener(this.game);
      }
   }

   // Purpose of `secondaryParam` beyond storage isn't confirmed -- see
   // its field doc comment.
   void setSecondaryParam(int value) {
      this.secondaryParam = value;
   }

   // Mode-4-style plain scrollable list (menus): up to 10 items visible
   // at once.
   void setupList(String title, String[] items, boolean cancelable) {
      this.title = title;
      this.items = items;
      this.marginX = 15;
      this.marginRight = 15;
      this.textFont = DEFAULT_TEXT_FONT;
      this.scrollTop = 0;
      this.itemCount = this.items.length;
      int visible = Math.min(this.itemCount, 10);
      this.scrollBottom = this.scrollTop + visible - 1;
      this.selectedIndex = 0;
      this.addCommand(ESGame.selectCommand);
      if (cancelable) {
         this.addCommand(ESGame.cancelCommand);
      }

      this.setCommandListener(this.game);
   }

   // Word-wrapped scrollable text (up to 11 lines visible).
   void setupMessage(String title, String body) {
      this.promptLines = null;
      this.footerLines = null;
      this.items = null;
      System.gc();
      this.title = title;
      this.marginX = 5;
      this.marginRight = 5;
      this.scrollTop = 0;
      this.textFont = DEFAULT_TEXT_FONT;
      this.items = this.wrapText(body);
      this.itemCount = this.items.length;
      int visible = Math.min(this.itemCount, 11);
      this.scrollBottom = this.scrollTop + visible - 1;
   }

   // Prompt text (word-wrapped into promptLines) above a selectable
   // list (items), with per-item word-wrap that groups wrapped
   // continuation lines under their logical item for navigation
   // (itemGroupStart).
   void setupPromptList(String title, String prompt, String[] items) {
      this.title = title;
      this.marginX = 10;
      this.marginRight = 10;
      this.textFont = DEFAULT_TEXT_FONT;
      this.promptLines = this.wrapText(prompt);
      this.items = items;
      this.scrollTop = 0;
      this.itemCount = this.items.length;
      this.itemGroupStart = new int[this.itemCount];
      short i = 0;

      while (i < this.itemCount) {
         this.itemGroupStart[i] = i++;
      }

      int visibleLines = 0;

      for (short item = 0; item < this.itemCount; item++) {
         visibleLines++;
         if (this.textFont.stringWidth(this.items[item]) > this.canvas.getWidth() - this.marginX - this.marginRight) {
            String[] wrapped = this.wrapText(this.items[item]);
            int extraLines = wrapped.length;
            String[] merged = new String[this.itemCount + extraLines - 1];
            System.arraycopy(this.items, 0, merged, 0, item);
            System.arraycopy(wrapped, 0, merged, item, extraLines);
            System.arraycopy(this.items, item + 1, merged, item + 1 + --extraLines, this.itemCount - item - 1);

            for (int g = this.itemGroupStart.length - 1; g >= visibleLines; g--) {
               this.itemGroupStart[g] = this.itemGroupStart[g] + extraLines;
            }

            this.itemCount += extraLines;
            item = (short)(item + extraLines);
            this.items = new String[this.itemCount];
            System.arraycopy(merged, 0, this.items, 0, this.itemCount);
         }
      }

      int visible = Math.min(this.itemCount, 9);
      this.scrollBottom = this.scrollTop + visible - 1;
      this.addCommand(ESGame.selectCommand);
      this.addCommand(ESGame.cancelCommand);
      this.setCommandListener(this.game);
      if (prompt != null && prompt.indexOf("<TAG>") >= 0) {
         this.rawTaggedText = new String(prompt);
      }
   }

   // Same as the 3-arg overload, plus a second word-wrapped text block
   // (footerLines) rendered between the prompt and the list (mode 6).
   void setupPromptList(String title, String prompt, String footer, String[] items) {
      this.setupPromptList(title, prompt, items);
      this.footerLines = this.wrapText(footer);
   }

   // originally g.java's `l()` -- empty here, overridden by LoadingScreen
   // to stop its background thread. Name/call-order inferred from that
   // override's behavior (stop-on-leave), not from a traced caller.
   protected void onExit() {
   }

   // originally g.java's `e()` -- empty here, overridden by LoadingScreen
   // to start its background thread. Name/call-order inferred the same
   // way as onExit.
   protected void onEnter() {
   }

   public void paint(Graphics g) throws Exception {
      switch (this.mode) {
         case 3:
            this.renderHighlightedText(g);
            break;
         case 4:
            this.renderPlainList(g);
            break;
         case 5:
            this.renderPromptList(g, 1);
            break;
         case 6:
            this.renderPromptList(g, 2);
      }

      this.renderSoftKeyBar(g);
   }

   private void renderTitleBar(Graphics g) {
      Font savedFont = g.getFont();
      g.setFont(TITLE_FONT);
      int savedColor = g.getColor();
      g.setColor(0);
      g.fillRect(0, 0, this.width(), 14);
      g.setColor(16777215);
      g.drawString(this.title, this.width() / 2, 0, 17);
      g.setColor(savedColor);
      g.setFont(savedFont);
   }

   private void renderHighlightedText(Graphics g) {
      g.setColor(2510210);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      this.renderTitleBar(g);
      g.setFont(this.textFont);
      this.lineHeight = this.textFont.getHeight();
      this.cursorY = 20;
      this.renderItemRows(g);
      if (this.scrollTop > 0) {
         this.drawScrollArrow(g, 155, 180, 1);
      }

      if (this.scrollBottom + 1 < this.itemCount) {
         this.drawScrollArrow(g, 165, 180, 2);
      }
   }

   private void renderPromptList(Graphics g, int variant) {
      g.setColor(2510210);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      this.renderTitleBar(g);
      g.setFont(this.textFont);
      this.lineHeight = this.textFont.getHeight();
      this.cursorY = 20;
      g.setColor(16776960);
      if (this.promptLines == null) {
         System.out.println("empty labels");
      }

      for (int i = 0; i < this.promptLines.length; i++) {
         g.drawString(this.promptLines[i], this.marginX, this.cursorY, 20);
         this.cursorY = this.cursorY + this.lineHeight;
      }

      if (variant == 2 && this.footerLines != null) {
         g.setColor(16777215);

         for (int i = 0; i < this.footerLines.length; i++) {
            g.drawString(this.footerLines[i], this.marginX, this.cursorY, 20);
            this.cursorY = this.cursorY + this.lineHeight;
         }
      }

      this.cursorY += 5;
      this.renderItemRows(g);
      if (this.scrollTop > 0) {
         this.drawScrollArrow(g, 155, 180, 1);
      }

      if (this.scrollBottom + 1 < this.itemCount) {
         this.drawScrollArrow(g, 165, 180, 2);
      }
   }

   // Shared "scrollable rows with a highlight box behind the selected
   // (group of) row(s)" renderer -- used by modes 3, 5, 6.
   private void renderItemRows(Graphics g) {
      for (int i = this.scrollTop; i <= this.scrollBottom; i++) {
         if (this.itemGroupStart != null && i == this.itemGroupStart[this.selectedIndex]
            || this.itemGroupStart == null && i == this.selectedIndex) {
            g.setColor(6710886);
            int boxWidth = this.width() - 2 * (this.marginX - 10);
            int boxHeight = 1;
            if (this.itemGroupStart != null) {
               if (this.selectedIndex + 1 != this.itemGroupStart.length) {
                  boxHeight = this.itemGroupStart[this.selectedIndex + 1] - this.itemGroupStart[this.selectedIndex];
               } else {
                  boxHeight = this.itemCount - this.itemGroupStart[this.selectedIndex];
               }
            }

            boxHeight = boxHeight * this.lineHeight + 2;
            g.fillRect(this.marginX - 10, this.cursorY - 1, boxWidth, boxHeight);
         }

         g.setColor(16776960);
         g.drawString(this.items[i], this.marginX, this.cursorY, 20);
         this.cursorY = this.cursorY + this.lineHeight;
         this.cursorY++;
      }
   }

   // Mode 4's own non-highlighted list renderer.
   private void renderPlainList(Graphics g) {
      g.setColor(2510210);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      this.renderTitleBar(g);
      g.setFont(this.textFont);
      this.lineHeight = this.textFont.getHeight();
      this.cursorY = 20;
      g.setColor(16776960);

      for (int i = this.scrollTop; i <= this.scrollBottom; i++) {
         g.drawString(this.items[i], this.marginX, this.cursorY, 20);
         this.cursorY = this.cursorY + this.lineHeight;
      }

      if (this.scrollTop > 0) {
         this.drawScrollArrow(g, 155, 180, 1);
      }

      if (this.scrollBottom + 1 < this.itemCount) {
         this.drawScrollArrow(g, 165, 180, 2);
      }
   }

   private void drawScrollArrow(Graphics g, int x, int y, int direction) {
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

   String[] wrapText(String text) {
      int maxWidth = this.canvas.getWidth() - this.marginX - this.marginRight;
      return this.canvas.a(text, maxWidth, this.textFont);
   }

   // Routes soft-key presses (game-action -6/-7) to the matching
   // Command, then dispatches the game-action-mapped scroll navigation
   // (up/down move the selection or the scroll window depending on
   // mode).
   protected void handleKey(int key) {
      if (key == -6) {
         Command leftCmd = this.leftSoftKeyCommand();
         if (leftCmd != null) {
            this.listener.commandAction(leftCmd, this.canvas);
            return;
         }
      } else if (key == -7) {
         Command rightCmd = this.rightSoftKeyCommand();
         if (rightCmd != null) {
            this.listener.commandAction(rightCmd, this.canvas);
            return;
         }
      }

      ESGame.debugCode = 21;
      int action = this.canvas.getGameAction(key);
      switch (action) {
         case 1:
            if (this.mode != 3 && this.mode != 5 && this.mode != 6) {
               if (this.mode == 4) {
                  int sel = this.selectedIndexOrMinusOne();
                  if (this.scrollTop > 0) {
                     this.scrollTop--;
                     this.scrollBottom--;
                     this.requestRepaint();
                  }
               }
            } else if (this.itemGroupStart != null) {
               if (this.selectedIndex + 1 > 1) {
                  this.selectedIndex--;
                  if (this.itemGroupStart[this.selectedIndex] < this.scrollTop) {
                     this.scrollBottom = this.scrollBottom - (this.scrollTop - this.itemGroupStart[this.selectedIndex]);
                     this.scrollTop = this.itemGroupStart[this.selectedIndex];
                  }

                  this.requestRepaint();
               }
            } else if (this.selectedIndex > 0) {
               this.selectedIndex--;
               if (this.scrollTop > this.selectedIndex) {
                  this.scrollTop--;
                  this.scrollBottom--;
               }

               this.requestRepaint();
            }
            break;
         case 6:
            if (this.mode != 3 && this.mode != 5 && this.mode != 6) {
               if (this.mode == 4 && this.scrollBottom < this.itemCount - 1) {
                  this.scrollTop++;
                  this.scrollBottom++;
                  this.requestRepaint();
               }
            } else if (this.itemGroupStart != null) {
               if (this.selectedIndex + 1 < this.itemGroupStart.length) {
                  this.selectedIndex++;
                  int windowSize = this.scrollBottom - this.scrollTop;
                  if (this.selectedIndex + 1 == this.itemGroupStart.length) {
                     this.scrollTop = this.itemCount - windowSize - 1;
                     this.scrollBottom = this.itemCount - 1;
                  } else if (this.itemGroupStart[this.selectedIndex + 1] > this.scrollBottom) {
                     this.scrollBottom = this.itemGroupStart[this.selectedIndex + 1];
                     this.scrollTop = this.scrollBottom - windowSize;
                  }

                  this.requestRepaint();
               }
            } else if (this.selectedIndex < this.items.length - 1) {
               this.selectedIndex++;
               if (this.scrollBottom < this.selectedIndex) {
                  this.scrollTop++;
                  this.scrollBottom++;
               }

               this.requestRepaint();
            }
      }
   }

   private void requestRepaint() {
      if (this.canvas.e()) {
         this.canvas.an = true;
      } else {
         this.canvas.repaint();
         this.canvas.serviceRepaints();
      }
   }

   String selectedItemText() {
      return this.items[this.selectedIndex];
   }

   int selectedIndexOrMinusOne() {
      switch (this.mode) {
         case 3:
         case 5:
         case 6:
            return this.selectedIndex;
         case 4:
         default:
            return -1;
      }
   }

   void setSelectedIndex(int index) {
      switch (this.mode) {
         case 3:
         case 5:
         case 6:
            this.selectedIndex = index;
            if (this.selectedIndex >= this.items.length) {
               this.selectedIndex = this.items.length - 1;
            }

            if (this.selectedIndex > this.scrollBottom) {
               int delta = this.selectedIndex - this.scrollBottom;
               this.scrollBottom += delta;
               this.scrollTop += delta;
            } else if (this.selectedIndex < this.scrollTop) {
               int delta = this.scrollTop - this.selectedIndex;
               this.scrollBottom -= delta;
               this.scrollTop -= delta;
            }
         case 4:
      }
   }

   // column: 0 = promptLines (or items when mode==4), 1 = footerLines
   // (mode 6 only).
   void setTextColumn(int column, String text) {
      if (column != 0 || this.mode != 5 && this.mode != 6 && this.mode != 4) {
         if (column == 1 && this.mode == 6) {
            this.footerLines = this.wrapText(text);
         }
      } else {
         this.promptLines = this.wrapText(text);
      }
   }

   void setTitle(String title) {
      this.title = title;
   }

   void setItems(String text) {
      if (this.mode == 5 || this.mode == 6) {
         this.promptLines = this.wrapText(text);
      } else if (this.mode == 4) {
         this.items = this.wrapText(text);
      }

      this.scrollTop = 0;
   }

   String firstLine() {
      if (this.mode == 5 || this.mode == 6) {
         return this.promptLines[0];
      } else {
         return this.mode == 4 ? this.items[0] : null;
      }
   }

   public void addCommand(Command c) {
      this.commands.addElement(c);
   }

   public void removeCommand(Command c) {
      this.commands.removeElement(c);
   }

   public void setCommandListener(CommandListener listener) {
      this.listener = listener;
   }

   private void renderSoftKeyBar(Graphics g) {
      if (this.commands.size() != 0) {
         g.setColor(16777215);
         g.fillRect(0, 190, this.width(), 20);
         int count = this.commands.size();
         g.setColor(0);
         g.setFont(SOFT_KEY_FONT);
         Command left = this.leftSoftKeyCommand();
         if (left != null) {
            g.drawString(left.getLabel(), 10, 194, 20);
         }

         Command right = this.rightSoftKeyCommand();
         if (right != null) {
            g.drawString(right.getLabel(), this.width() - 10, 194, 24);
         }
      }
   }

   private Command rightSoftKeyCommand() {
      int count = this.commands.size();
      Command result = null;
      if (count == 1) {
         result = (Command)this.commands.elementAt(0);
      } else if (count == 2) {
         for (int i = 0; i < 2; i++) {
            Command c = (Command)this.commands.elementAt(i);
            if (c == ESGame.okCommand || c == ESGame.selectCommand) {
               result = c;
               break;
            }
         }
      }

      return result;
   }

   private Command leftSoftKeyCommand() {
      int count = this.commands.size();
      Command result = null;
      if (count == 2) {
         for (int i = 0; i < 2; i++) {
            Command c = (Command)this.commands.elementAt(i);
            if (c == ESGame.backCommand || c == ESGame.cancelCommand) {
               result = c;
               break;
            }
         }
      }

      return result;
   }

   public int width() {
      return this.canvas.ay;
   }

   public int height() {
      return this.canvas.l;
   }
}
