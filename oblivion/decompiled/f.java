import java.util.Vector;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

public final class f {
   private static byte b = (byte)b.a.getHeight();
   private d a = null;
   public String a = null;
   private c[] a = null;
   private b a = null;
   private Image a = null;
   private byte[] a = null;
   private byte c = 0;
   private byte d = 0;
   private byte e = 0;
   private byte f = 0;
   private byte g = 0;
   private byte h = 1;
   private byte i = 0;
   private byte j = 0;
   public byte a = 0;
   private short a = 0;
   private short b = 0;
   public static boolean a = false;

   public f(String var1, b var2) {
      this.a = var2;
      this.a = g.a(var1);
   }

   public final void a(Graphics var1) {
      boolean var2 = false;
      boolean var3 = false;
      Vector var4 = this.a[this.f].a;
      c var5 = null;
      byte var6 = 0;
      int var7 = 0;
      int var8 = this.a;
      boolean var9 = false;
      var1.drawImage(this.a, 0, 0, 0);
      if (this.a != null) {
         g.a(var1, this.a, this.a[0], (b.a >> 1) - (g.a(this.a, this.a[0]) >> 1), b.b - b.a.getHeight() - 4 - g.b(this.a, this.a[0]));
         g.a(var1, this.a, this.a[this.f + 1], (b.a >> 1) - (g.a(this.a, this.a[this.f + 1]) >> 1), b.b - b.a.getHeight() - 4 - g.b(this.a, this.a[this.f + 1]));
      }

      var1.setFont(b.a);
      var1.setColor(0);
      var1.drawString(this.a[this.f].a, (b.a >> 1) - (b.a.stringWidth(this.a[this.f].a) >> 1), 12, 0);
      if (this.a != null) {
         var1.drawString(this.a, (b.a >> 1) - (b.a.stringWidth(this.a) >> 1), b.b - g.b(this.a, 5) - (b << 1), 0);
      }

      var8 += 12 + (b << 1);
      this.e = -1;

      for (var7 = 0; var4 != null && var7 < var4.size(); var7++) {
         var5 = (c)var4.elementAt(var7);
         if (var8 >= 12 + (b << 1)) {
            if (this.e == -1) {
               this.e = (byte)var7;
            }

            if (var7 == this.c) {
               var1.setColor(16448974);
               var1.fillRect(15, var8, b.a - 30, b);
               if (var5.b != null) {
                  var1.setFont(b.a);
                  var1.setColor(0);
                  var1.drawRect(20, b.b - b.a.getHeight() - 4 - g.b(this.a, 5) - (b << 1) - 6, b.a - 40, b + 4);
                  var1.drawString(var5.b, 23, b.b - b.a.getHeight() - 4 - g.b(this.a, 5) - (b << 1) - 3, 0);
               }

               var1.setColor(var5.b ? 10318649 : 16711680);
            } else {
               var1.setColor(var5.b ? 0 : 16711680);
            }

            if (var5.a) {
               g.a(var1, this.a, 14, 15, var8);
               var1.setFont(b.c);
               var6 = 15;
            } else {
               var1.setFont(b.a);
               var6 = 0;
            }

            if (var5.a.size() == 0) {
               String var10 = var5.a;
               String var11 = var5.a;
               boolean var12 = false;
               if (var7 == this.c) {
                  var11 = var10 = var10.substring(this.g);
               }

               while (g.a(this.a, 12) + var6 + 15 > b.a - var1.getFont().stringWidth(var11)) {
                  var12 = true;
                  var10 = var10.substring(0, var10.length() - 1);
                  var11 = var10 + "...";
               }

               if (var7 == this.c) {
                  if (var12) {
                     if (this.h == -1 && this.g == 0) {
                        this.h = 1;
                        this.j = 1;
                     }

                     this.a(1);
                  } else if (this.i == 1 && this.h == 1) {
                     this.h = -1;
                     this.j = 1;
                  }
               }

               var1.drawString(var11, 15 + var6, var8, 0);
            } else {
               var1.drawString("<" + var5.a + ">", 15 + var6, var8, 0);
            }
         } else {
            var2 = true;
            var3 = true;
         }

         if ((var8 += b) + (b << 1) >= b.b - b.a.getHeight() - 4 - g.b(this.a, 5) - (b << 1)) {
            var2 = true;
            var3 = true;
            break;
         }
      }

      if (this.a != null && this.f < this.a.length && this.a[this.f] != null && this.a[this.f].a != null) {
         String[] var19 = this.a[this.f].a;
         var8 = this.a + b * 3;

         for (byte var18 = 0; var18 < var19.length; var18 += 2) {
            if (var8 >= 12 + (b << 1)) {
               if (var19[var18] != null) {
                  var1.setFont(b.c);
                  var1.setColor(0);
                  var1.drawString(var19[var18], 10, var8, 0);
               }

               if (var19[var18 + 1] != null) {
                  var1.setFont(b.a);
                  var1.setColor(16711680);
                  var1.drawString(var19[var18 + 1], 15 + b.c.stringWidth(var19[var18]), var8, 0);
               }
            } else {
               var2 = true;
            }

            if ((var8 += b) + b >= b.b - b.a.getHeight() - 4 - g.b(this.a, 5)) {
               if (var18 < var19.length - 2) {
                  var3 = true;
               }
               break;
            }
         }
      }

      this.d = (byte)var7;
      var1.setFont(b.a);
      var1.setColor(16711680);
      var1.drawString(b.a(449).toUpperCase(), 2, b.b - b.a.getHeight() - 2, 0);
      if (var2) {
         g.a(var1, this.a, 54, b.a - g.a(this.a, 54) - 10, 35);
      }

      if (var3) {
         g.a(var1, this.a, 53, b.a - g.a(this.a, 53) - 10, b.b - b.a.getHeight() - g.b(this.a, 53) - g.b(this.a, 5) - 6);
      }

      if (a) {
         var1.setColor(0);
         var1.fillRect(0, 0, b.a, b.b);
         var1.setColor(16777215);
         var1.setFont(b.d);
         var1.drawString(b.a(571), (b.a >> 1) - (b.d.stringWidth(b.a(571)) >> 1), (b.b >> 1) - (b.d.getHeight() >> 1), 0);
         var1.drawString(b.a(22).toUpperCase(), 2, b.b - b.d.getHeight() - 2, 0);
         var1.drawString(b.a(426).toUpperCase(), b.a - b.d.stringWidth(b.a(426)) - 2, b.b - b.d.getHeight() - 2, 0);
      }
   }

   public final void a(char var1) {
      boolean var2 = false;
      boolean var3 = false;
      if (var1 == 4) {
         if (++this.c >= this.a[this.f].a.size()) {
            this.c = 0;
         }

         if (this.a != null && this.f < this.a.length && this.a[this.f] != null && this.a[this.f].a != null) {
            this.a = (short)(this.a - b);
            if ((this.a[this.f].a.length >> 1) * b + this.a + (b << 2) < b.b - b.a.getHeight() - 4 - g.b(this.a, 5)) {
               this.a = (short)(this.a + b);
            }
         }
      } else if (var1 == 3) {
         if (--this.c < 0) {
            this.c = (byte)(this.a[this.f].a.size() - 1);
         }

         if (this.a != null && this.f < this.a.length && this.a[this.f] != null && this.a[this.f].a != null) {
            this.a = (short)(this.a + b);
            if (this.a > 0) {
               this.a = 0;
            }
         }
      } else if (var1 == 5) {
         this.a();
         if (--this.f < 0) {
            this.f = (byte)(this.a.length - 2);
         }

         this.c = 0;
         this.d = 0;
         this.e = 0;
         this.a = 0;
      } else if (var1 == 6) {
         this.a();
         if (++this.f == this.a.length - 1) {
            this.f = 0;
         }

         this.c = 0;
         this.d = 0;
         this.e = 0;
         this.a = 0;
      } else if (var1 == 7 && this.c < this.a[this.f].a.size() && this.c >= 0) {
         c var4 = (c)this.a[this.f].a.elementAt(this.c);
         if (!this.a[this.f].a.equals(b.a(36)) && !this.a[this.f].a.equals(b.a(37)) && !var4.b) {
            return;
         }

         c var5 = null;
         if (var4.a.size() != 0) {
            this.a[this.f] = var4;
            this.c = 0;
         } else {
            for (int var6 = 0; var6 < this.a[this.f].a.size(); var6++) {
               var5 = (c)this.a[this.f].a.elementAt(var6);
               if (var4.a.a.equals(b.a(27))) {
                  if (this.a(var5, var4)) {
                     var5.a = false;
                  }
               } else {
                  var5.a = false;
               }
            }

            var4.a = true;
         }

         if (this.a != null) {
            this.a.a(var4);
         }
      }

      if (this.c > this.d) {
         this.a = (short)(-b * (this.c - (this.d - this.e)));
      } else if (this.c < this.e) {
         this.a = (short)(-b * this.c);
      }

      this.a(0);
   }

   private boolean a(c var1, c var2) {
      return !var1.a.equals(b.a(149)) && !var1.a.equals(b.a(151)) || !var2.a.equals(b.a(149)) && !var2.a.equals(b.a(151))
         ? (var1.a.equals(b.a(150)) || var1.a.equals(b.a(152))) && (var2.a.equals(b.a(150)) || var2.a.equals(b.a(152)))
         : true;
   }

   public final void a(byte[] var1, c[] var2, String var3, Image var4, Graphics var5) {
      this.a = var1;
      this.a = var2;
      this.a = 1;
      this.f = 0;
      this.a = var3;
      this.c = 0;
      this.d = 0;
      this.e = 0;
      this.a = 0;
      this.a = var4;
      this.g = 0;
      this.i = 0;
      this.b(var5);
   }

   private final void b(Graphics var1) {
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
      var1.fillRect(0, 0, b.a, b.b);
      var2 = g.a(this.a, 13);
      var3 = g.b(this.a, 13);
      var4 = g.b(this.a, 11);
      var5 = g.b(this.a, 12);
      var6 = g.a(this.a, 12);
      var7 = g.a(this.a, 8);
      var8 = g.b(this.a, 5);
      var9 = g.a(this.a, 5);

      for (int var20 = 0; var20 < b.a; var20 += var2) {
         for (int var23 = 0; var23 < b.b - b.a.getHeight() - 4 - var3; var23 += var3) {
            g.a(var1, this.a, 13, var20, var23);
         }
      }

      for (int var24 = 0; var24 < b.b - b.a.getHeight() - 4 - var4; var24 += var4) {
         g.a(var1, this.a, 11, 0, var24);
      }

      for (int var25 = 0; var25 < b.b - b.a.getHeight() - 4 - var5; var25 += var5) {
         g.a(var1, this.a, 12, b.a - var6, var25);
      }

      for (int var21 = 0; var21 < b.a; var21 += var7) {
         g.a(var1, this.a, 8, var21, 0);
      }

      for (int var22 = 0; var22 < b.a; var22 += var9) {
         g.a(var1, this.a, 5, var22, b.b - b.a.getHeight() - 4 - var8);
      }

      g.a(var1, this.a, 9, 0, 0);
      g.a(var1, this.a, 10, b.a - g.a(this.a, 10), 0);
      g.a(var1, this.a, 6, 0, b.b - b.a.getHeight() - 4 - g.b(this.a, 6));
      g.a(var1, this.a, 7, b.a - g.a(this.a, 7), b.b - b.a.getHeight() - 4 - g.b(this.a, 7));
   }

   public final boolean a() {
      if (this.a == null || this.a[this.f] == null) {
         return false;
      } else if (this.a[this.f].a != null) {
         this.a[this.f] = this.a[this.f].a;
         this.c = 0;
         this.d = 0;
         this.e = 0;
         this.a = 0;
         return true;
      } else {
         return false;
      }
   }

   public final void a(long var1) {
      if (this.i == 1) {
         this.b = (short)(this.b + var1);
         if (this.j == 1) {
            if (this.b >= 1000) {
               this.b = 0;
               this.j = 0;
               return;
            }
         } else if (this.b >= 500) {
            this.g = (byte)(this.g + this.h);
            this.b = 0;
         }
      }
   }

   private final void a(int var1) {
      if (var1 != this.i) {
         this.b = 0;
         this.g = 0;
         this.h = 1;
         this.j = 0;
         this.i = (byte)var1;
      }
   }
}
