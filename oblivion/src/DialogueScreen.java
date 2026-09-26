/**
 * Renamed from decompiled/f.java (see docs/CLASS_MAP.md, docs/rename.map).
 * The shared list-menu screen: NPC dialogue trees, the inventory/equipment
 * tabs ({4,1,2,3,18} sprite groups) and the shop Buy/Sell tabs ({17,15,16}).
 * `roots` holds one DialogueNode tree per tab; the current tab is `tab`.
 * Keys (handleKey): 3 up, 4 down, 5 previous tab, 6 next tab, 7 select
 * (a submenu node is entered, a leaf is marked and reported to
 * Game.menuSelected). Over-long lines scroll horizontally (textScroll,
 * scrollState, scrollDir) after a 1 s pause.
 */
import java.util.Vector;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

public final class DialogueScreen {
   private static byte lineHeight = (byte)Game.fontSmall.getHeight();
   private SpriteFrame ui = null;
   public String caption = null;
   private DialogueNode[] roots = null;
   private Game game = null;
   private Image backdrop = null;
   private byte[] tabSprites = null;
   private byte cursor = 0;
   private byte lastVisible = 0;
   private byte firstVisible = 0;
   private byte tab = 0;
   private byte textScroll = 0;
   private byte scrollDir = 1;
   private byte scrollState = 0;
   private byte scrollPause = 0;
   public byte open = 0;
   private short scrollY = 0;
   private short scrollTimer = 0;
   public static boolean showPauseOverlay = false;

   public DialogueScreen(String var1, Game var2) {
      this.game = var2;
      this.ui = SpriteRenderer.load(var1);
   }

   public final void paint(Graphics var1) {
      boolean var2 = false;
      boolean var3 = false;
      Vector var4 = this.roots[this.tab].children;
      DialogueNode var5 = null;
      byte var6 = 0;
      int var7 = 0;
      int var8 = this.scrollY;
      boolean var9 = false;
      var1.drawImage(this.backdrop, 0, 0, 0);
      if (this.tabSprites != null) {
         SpriteRenderer.draw(
            var1,
            this.ui,
            this.tabSprites[0],
            (Game.screenWidth >> 1) - (SpriteRenderer.getWidth(this.ui, this.tabSprites[0]) >> 1),
            Game.screenHeight - Game.fontSmall.getHeight() - 4 - SpriteRenderer.getHeight(this.ui, this.tabSprites[0])
         );
         SpriteRenderer.draw(
            var1,
            this.ui,
            this.tabSprites[this.tab + 1],
            (Game.screenWidth >> 1) - (SpriteRenderer.getWidth(this.ui, this.tabSprites[this.tab + 1]) >> 1),
            Game.screenHeight - Game.fontSmall.getHeight() - 4 - SpriteRenderer.getHeight(this.ui, this.tabSprites[this.tab + 1])
         );
      }

      var1.setFont(Game.fontSmall);
      var1.setColor(0);
      var1.drawString(this.roots[this.tab].text, (Game.screenWidth >> 1) - (Game.fontSmall.stringWidth(this.roots[this.tab].text) >> 1), 12, 0);
      if (this.caption != null) {
         var1.drawString(
            this.caption,
            (Game.screenWidth >> 1) - (Game.fontSmall.stringWidth(this.caption) >> 1),
            Game.screenHeight - SpriteRenderer.getHeight(this.ui, 5) - (lineHeight << 1),
            0
         );
      }

      var8 += 12 + (lineHeight << 1);
      this.firstVisible = -1;

      for (var7 = 0; var4 != null && var7 < var4.size(); var7++) {
         var5 = (DialogueNode)var4.elementAt(var7);
         if (var8 >= 12 + (lineHeight << 1)) {
            if (this.firstVisible == -1) {
               this.firstVisible = (byte)var7;
            }

            if (var7 == this.cursor) {
               var1.setColor(16448974);
               var1.fillRect(15, var8, Game.screenWidth - 30, lineHeight);
               if (var5.tooltip != null) {
                  var1.setFont(Game.fontSmall);
                  var1.setColor(0);
                  var1.drawRect(
                     20,
                     Game.screenHeight - Game.fontSmall.getHeight() - 4 - SpriteRenderer.getHeight(this.ui, 5) - (lineHeight << 1) - 6,
                     Game.screenWidth - 40,
                     lineHeight + 4
                  );
                  var1.drawString(
                     var5.tooltip, 23, Game.screenHeight - Game.fontSmall.getHeight() - 4 - SpriteRenderer.getHeight(this.ui, 5) - (lineHeight << 1) - 3, 0
                  );
               }

               var1.setColor(var5.available ? 10318649 : 16711680);
            } else {
               var1.setColor(var5.available ? 0 : 16711680);
            }

            if (var5.marked) {
               SpriteRenderer.draw(var1, this.ui, 14, 15, var8);
               var1.setFont(Game.fontSmallBold);
               var6 = 15;
            } else {
               var1.setFont(Game.fontSmall);
               var6 = 0;
            }

            if (var5.children.size() == 0) {
               String var10 = var5.text;
               String var11 = var5.text;
               boolean var12 = false;
               if (var7 == this.cursor) {
                  var11 = var10 = var10.substring(this.textScroll);
               }

               while (SpriteRenderer.getWidth(this.ui, 12) + var6 + 15 > Game.screenWidth - var1.getFont().stringWidth(var11)) {
                  var12 = true;
                  var10 = var10.substring(0, var10.length() - 1);
                  var11 = var10 + "...";
               }

               if (var7 == this.cursor) {
                  if (var12) {
                     if (this.scrollDir == -1 && this.textScroll == 0) {
                        this.scrollDir = 1;
                        this.scrollPause = 1;
                     }

                     this.setScrollState(1);
                  } else if (this.scrollState == 1 && this.scrollDir == 1) {
                     this.scrollDir = -1;
                     this.scrollPause = 1;
                  }
               }

               var1.drawString(var11, 15 + var6, var8, 0);
            } else {
               var1.drawString("<" + var5.text + ">", 15 + var6, var8, 0);
            }
         } else {
            var2 = true;
            var3 = true;
         }

         if ((var8 += lineHeight) + (lineHeight << 1)
            >= Game.screenHeight - Game.fontSmall.getHeight() - 4 - SpriteRenderer.getHeight(this.ui, 5) - (lineHeight << 1)) {
            var2 = true;
            var3 = true;
            break;
         }
      }

      if (this.roots != null && this.tab < this.roots.length && this.roots[this.tab] != null && this.roots[this.tab].answerLines != null) {
         String[] var19 = this.roots[this.tab].answerLines;
         var8 = this.scrollY + lineHeight * 3;

         for (byte var18 = 0; var18 < var19.length; var18 += 2) {
            if (var8 >= 12 + (lineHeight << 1)) {
               if (var19[var18] != null) {
                  var1.setFont(Game.fontSmallBold);
                  var1.setColor(0);
                  var1.drawString(var19[var18], 10, var8, 0);
               }

               if (var19[var18 + 1] != null) {
                  var1.setFont(Game.fontSmall);
                  var1.setColor(16711680);
                  var1.drawString(var19[var18 + 1], 15 + Game.fontSmallBold.stringWidth(var19[var18]), var8, 0);
               }
            } else {
               var2 = true;
            }

            if ((var8 += lineHeight) + lineHeight >= Game.screenHeight - Game.fontSmall.getHeight() - 4 - SpriteRenderer.getHeight(this.ui, 5)) {
               if (var18 < var19.length - 2) {
                  var3 = true;
               }
               break;
            }
         }
      }

      this.lastVisible = (byte)var7;
      var1.setFont(Game.fontSmall);
      var1.setColor(16711680);
      var1.drawString(Game.getString(449).toUpperCase(), 2, Game.screenHeight - Game.fontSmall.getHeight() - 2, 0);
      if (var2) {
         SpriteRenderer.draw(var1, this.ui, 54, Game.screenWidth - SpriteRenderer.getWidth(this.ui, 54) - 10, 35);
      }

      if (var3) {
         SpriteRenderer.draw(
            var1,
            this.ui,
            53,
            Game.screenWidth - SpriteRenderer.getWidth(this.ui, 53) - 10,
            Game.screenHeight - Game.fontSmall.getHeight() - SpriteRenderer.getHeight(this.ui, 53) - SpriteRenderer.getHeight(this.ui, 5) - 6
         );
      }

      if (showPauseOverlay) {
         var1.setColor(0);
         var1.fillRect(0, 0, Game.screenWidth, Game.screenHeight);
         var1.setColor(16777215);
         var1.setFont(Game.fontLargeBold);
         var1.drawString(
            Game.getString(571),
            (Game.screenWidth >> 1) - (Game.fontLargeBold.stringWidth(Game.getString(571)) >> 1),
            (Game.screenHeight >> 1) - (Game.fontLargeBold.getHeight() >> 1),
            0
         );
         var1.drawString(Game.getString(22).toUpperCase(), 2, Game.screenHeight - Game.fontLargeBold.getHeight() - 2, 0);
         var1.drawString(
            Game.getString(426).toUpperCase(),
            Game.screenWidth - Game.fontLargeBold.stringWidth(Game.getString(426)) - 2,
            Game.screenHeight - Game.fontLargeBold.getHeight() - 2,
            0
         );
      }
   }

   public final void handleKey(char var1) {
      boolean var2 = false;
      boolean var3 = false;
      if (var1 == 4) {
         if (++this.cursor >= this.roots[this.tab].children.size()) {
            this.cursor = 0;
         }

         if (this.roots != null && this.tab < this.roots.length && this.roots[this.tab] != null && this.roots[this.tab].answerLines != null) {
            this.scrollY = (short)(this.scrollY - lineHeight);
            if ((this.roots[this.tab].answerLines.length >> 1) * lineHeight + this.scrollY + (lineHeight << 2)
               < Game.screenHeight - Game.fontSmall.getHeight() - 4 - SpriteRenderer.getHeight(this.ui, 5)) {
               this.scrollY = (short)(this.scrollY + lineHeight);
            }
         }
      } else if (var1 == 3) {
         if (--this.cursor < 0) {
            this.cursor = (byte)(this.roots[this.tab].children.size() - 1);
         }

         if (this.roots != null && this.tab < this.roots.length && this.roots[this.tab] != null && this.roots[this.tab].answerLines != null) {
            this.scrollY = (short)(this.scrollY + lineHeight);
            if (this.scrollY > 0) {
               this.scrollY = 0;
            }
         }
      } else if (var1 == 5) {
         this.goBack();
         if (--this.tab < 0) {
            this.tab = (byte)(this.tabSprites.length - 2);
         }

         this.cursor = 0;
         this.lastVisible = 0;
         this.firstVisible = 0;
         this.scrollY = 0;
      } else if (var1 == 6) {
         this.goBack();
         if (++this.tab == this.tabSprites.length - 1) {
            this.tab = 0;
         }

         this.cursor = 0;
         this.lastVisible = 0;
         this.firstVisible = 0;
         this.scrollY = 0;
      } else if (var1 == 7 && this.cursor < this.roots[this.tab].children.size() && this.cursor >= 0) {
         DialogueNode var4 = (DialogueNode)this.roots[this.tab].children.elementAt(this.cursor);
         if (!this.roots[this.tab].text.equals(Game.getString(36)) && !this.roots[this.tab].text.equals(Game.getString(37)) && !var4.available) {
            return;
         }

         DialogueNode var5 = null;
         if (var4.children.size() != 0) {
            this.roots[this.tab] = var4;
            this.cursor = 0;
         } else {
            for (int var6 = 0; var6 < this.roots[this.tab].children.size(); var6++) {
               var5 = (DialogueNode)this.roots[this.tab].children.elementAt(var6);
               if (var4.parent.text.equals(Game.getString(27))) {
                  if (this.sameBuySellGroup(var5, var4)) {
                     var5.marked = false;
                  }
               } else {
                  var5.marked = false;
               }
            }

            var4.marked = true;
         }

         if (this.game != null) {
            this.game.menuSelected(var4);
         }
      }

      if (this.cursor > this.lastVisible) {
         this.scrollY = (short)(-lineHeight * (this.cursor - (this.lastVisible - this.firstVisible)));
      } else if (this.cursor < this.firstVisible) {
         this.scrollY = (short)(-lineHeight * this.cursor);
      }

      this.setScrollState(0);
   }

   private boolean sameBuySellGroup(DialogueNode var1, DialogueNode var2) {
      return !var1.text.equals(Game.getString(149)) && !var1.text.equals(Game.getString(151))
            || !var2.text.equals(Game.getString(149)) && !var2.text.equals(Game.getString(151))
         ? (var1.text.equals(Game.getString(150)) || var1.text.equals(Game.getString(152)))
            && (var2.text.equals(Game.getString(150)) || var2.text.equals(Game.getString(152)))
         : true;
   }

   public final void openTabs(byte[] var1, DialogueNode[] var2, String var3, Image var4, Graphics var5) {
      this.tabSprites = var1;
      this.roots = var2;
      this.open = 1;
      this.tab = 0;
      this.caption = var3;
      this.cursor = 0;
      this.lastVisible = 0;
      this.firstVisible = 0;
      this.scrollY = 0;
      this.backdrop = var4;
      this.textScroll = 0;
      this.scrollState = 0;
      this.drawFrame(var5);
   }

   private final void drawFrame(Graphics var1) {
      int var2 = 0;
      int var3 = 0;
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      int var7 = 0;
      int var8 = 0;
      int var9 = 0;
      boolean var10 = false;
      boolean var11 = false;
      var1.setColor(0);
      var1.fillRect(0, 0, Game.screenWidth, Game.screenHeight);
      var2 = SpriteRenderer.getWidth(this.ui, 13);
      var3 = SpriteRenderer.getHeight(this.ui, 13);
      var4 = SpriteRenderer.getHeight(this.ui, 11);
      var5 = SpriteRenderer.getHeight(this.ui, 12);
      var6 = SpriteRenderer.getWidth(this.ui, 12);
      var7 = SpriteRenderer.getWidth(this.ui, 8);
      var8 = SpriteRenderer.getHeight(this.ui, 5);
      var9 = SpriteRenderer.getWidth(this.ui, 5);

      for (int var20 = 0; var20 < Game.screenWidth; var20 += var2) {
         for (int var23 = 0; var23 < Game.screenHeight - Game.fontSmall.getHeight() - 4 - var3; var23 += var3) {
            SpriteRenderer.draw(var1, this.ui, 13, var20, var23);
         }
      }

      for (int var24 = 0; var24 < Game.screenHeight - Game.fontSmall.getHeight() - 4 - var4; var24 += var4) {
         SpriteRenderer.draw(var1, this.ui, 11, 0, var24);
      }

      for (int var25 = 0; var25 < Game.screenHeight - Game.fontSmall.getHeight() - 4 - var5; var25 += var5) {
         SpriteRenderer.draw(var1, this.ui, 12, Game.screenWidth - var6, var25);
      }

      for (int var21 = 0; var21 < Game.screenWidth; var21 += var7) {
         SpriteRenderer.draw(var1, this.ui, 8, var21, 0);
      }

      for (int var22 = 0; var22 < Game.screenWidth; var22 += var9) {
         SpriteRenderer.draw(var1, this.ui, 5, var22, Game.screenHeight - Game.fontSmall.getHeight() - 4 - var8);
      }

      SpriteRenderer.draw(var1, this.ui, 9, 0, 0);
      SpriteRenderer.draw(var1, this.ui, 10, Game.screenWidth - SpriteRenderer.getWidth(this.ui, 10), 0);
      SpriteRenderer.draw(var1, this.ui, 6, 0, Game.screenHeight - Game.fontSmall.getHeight() - 4 - SpriteRenderer.getHeight(this.ui, 6));
      SpriteRenderer.draw(
         var1,
         this.ui,
         7,
         Game.screenWidth - SpriteRenderer.getWidth(this.ui, 7),
         Game.screenHeight - Game.fontSmall.getHeight() - 4 - SpriteRenderer.getHeight(this.ui, 7)
      );
   }

   public final boolean goBack() {
      if (this.roots == null || this.roots[this.tab] == null) {
         return false;
      } else if (this.roots[this.tab].parent != null) {
         this.roots[this.tab] = this.roots[this.tab].parent;
         this.cursor = 0;
         this.lastVisible = 0;
         this.firstVisible = 0;
         this.scrollY = 0;
         return true;
      } else {
         return false;
      }
   }

   public final void tick(long var1) {
      if (this.scrollState == 1) {
         this.scrollTimer = (short)(this.scrollTimer + var1);
         if (this.scrollPause == 1) {
            if (this.scrollTimer >= 1000) {
               this.scrollTimer = 0;
               this.scrollPause = 0;
               return;
            }
         } else if (this.scrollTimer >= 500) {
            this.textScroll = (byte)(this.textScroll + this.scrollDir);
            this.scrollTimer = 0;
         }
      }
   }

   private final void setScrollState(int var1) {
      if (var1 != this.scrollState) {
         this.scrollTimer = 0;
         this.textScroll = 0;
         this.scrollDir = 1;
         this.scrollPause = 0;
         this.scrollState = (byte)var1;
      }
   }
}
