import java.util.Vector;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;

public class g {
   Vector u;
   CommandListener b;
   static final Font E = Font.getFont(0, 1, 8);
   private static final Font r = Font.getFont(0, 1, 0);
   private static final Font B = Font.getFont(64, 0, 8);
   static final Font g = Font.getFont(0, 1, 8);
   static final Font D = Font.getFont(0, 1, 0);
   int h;
   int s;
   int i;
   ESGame w;
   String p;
   e C;
   Object v;
   Object t;
   int a;
   int n;
   Font e;
   int o;
   int j;
   int A;
   int c;
   int F;
   String[] m;
   String[] k;
   String[] z;
   int[] y = null;
   String q;
   int x;

   public g(ESGame var1, int var2, int var3) {
      this.w = var1;
      this.C = this.w.gameCanvas;
      this.h = var2;
      this.s = var3;
      this.i = 0;
      this.v = null;
      this.t = null;
      this.p = null;
      this.u = new Vector(2);
      this.m = null;
      this.k = null;
      this.z = null;
      if (var2 == 4) {
         this.a(ESGame.okCommand);
         this.a(this.w);
      }
   }

   void a(int var1) {
      this.s = var1;
   }

   void a(String var1, String[] var2, boolean var3) {
      this.q = var1;
      this.z = var2;
      this.o = 15;
      this.j = 15;
      this.e = g;
      this.A = 0;
      this.F = this.z.length;
      int var4 = Math.min(this.F, 10);
      this.c = this.A + var4 - 1;
      this.x = 0;
      this.a(ESGame.selectCommand);
      if (var3) {
         this.a(ESGame.cancelCommand);
      }

      this.a(this.w);
   }

   void a(String var1, String var2) {
      this.m = null;
      this.k = null;
      this.z = null;
      System.gc();
      this.q = var1;
      this.o = 5;
      this.j = 5;
      this.A = 0;
      this.e = g;
      this.z = this.b(var2);
      this.F = this.z.length;
      int var3 = Math.min(this.F, 11);
      this.c = this.A + var3 - 1;
   }

   void a(String var1, String var2, String[] var3) {
      this.q = var1;
      this.o = 10;
      this.j = 10;
      this.e = g;
      this.m = this.b(var2);
      this.z = var3;
      this.A = 0;
      this.F = this.z.length;
      this.y = new int[this.F];
      short var4 = 0;

      while (var4 < this.F) {
         this.y[var4] = var4++;
      }

      Object var5 = null;
      int var6 = 0;

      for (short var7 = 0; var7 < this.F; var7++) {
         var6++;
         if (this.e.stringWidth(this.z[var7]) > this.C.getWidth() - this.o - this.j) {
            String[] var8 = this.b(this.z[var7]);
            int var9 = var8.length;
            var5 = new String[this.F + var9 - 1];
            System.arraycopy(this.z, 0, var5, 0, var7);
            System.arraycopy(var8, 0, var5, var7, var9);
            System.arraycopy(this.z, var7 + 1, var5, var7 + 1 + --var9, this.F - var7 - 1);

            for (int var10 = this.y.length - 1; var10 >= var6; var10--) {
               this.y[var10] = this.y[var10] + var9;
            }

            this.F += var9;
            var7 = (short)(var7 + var9);
            this.z = new String[this.F];
            System.arraycopy(var5, 0, this.z, 0, this.F);
         }
      }

      int var12 = Math.min(this.F, 9);
      this.c = this.A + var12 - 1;
      this.a(ESGame.selectCommand);
      this.a(ESGame.cancelCommand);
      this.a(this.w);
      if (var2 != null && var2.indexOf("<TAG>") >= 0) {
         this.p = new String(var2);
      }
   }

   void a(String var1, String var2, String var3, String[] var4) {
      this.a(var1, var2, var4);
      this.k = this.b(var3);
   }

   protected void l() {
   }

   protected void e() {
   }

   public void b(Graphics var1) throws Exception {
      switch (this.h) {
         case 3:
            this.f(var1);
            break;
         case 4:
            this.g(var1);
            break;
         case 5:
            this.a(var1, 1);
            break;
         case 6:
            this.a(var1, 2);
      }

      this.c(var1);
   }

   private void a(Graphics var1) {
      Font var2 = var1.getFont();
      var1.setFont(r);
      int var3 = var1.getColor();
      var1.setColor(0);
      var1.fillRect(0, 0, this.b(), 14);
      var1.setColor(16777215);
      var1.drawString(this.q, this.b() / 2, 0, 17);
      var1.setColor(var3);
      var1.setFont(var2);
   }

   private void f(Graphics var1) {
      var1.setColor(2510210);
      var1.fillRect(0, 0, this.b(), 20 + this.h());
      this.a(var1);
      var1.setFont(this.e);
      this.a = this.e.getHeight();
      this.n = 20;
      this.d(var1);
      if (this.A > 0) {
         this.a(var1, 155, 180, 1);
      }

      if (this.c + 1 < this.F) {
         this.a(var1, 165, 180, 2);
      }
   }

   private void a(Graphics var1, int var2) {
      var1.setColor(2510210);
      var1.fillRect(0, 0, this.b(), 20 + this.h());
      this.a(var1);
      var1.setFont(this.e);
      this.a = this.e.getHeight();
      this.n = 20;
      var1.setColor(16776960);
      if (this.m == null) {
         System.out.println("empty labels");
      }

      for (int var3 = 0; var3 < this.m.length; var3++) {
         var1.drawString(this.m[var3], this.o, this.n, 20);
         this.n = this.n + this.a;
      }

      if (var2 == 2 && this.k != null) {
         var1.setColor(16777215);

         for (int var4 = 0; var4 < this.k.length; var4++) {
            var1.drawString(this.k[var4], this.o, this.n, 20);
            this.n = this.n + this.a;
         }
      }

      this.n += 5;
      this.d(var1);
      if (this.A > 0) {
         this.a(var1, 155, 180, 1);
      }

      if (this.c + 1 < this.F) {
         this.a(var1, 165, 180, 2);
      }
   }

   private void d(Graphics var1) {
      for (int var2 = this.A; var2 <= this.c; var2++) {
         if (this.y != null && var2 == this.y[this.x] || this.y == null && var2 == this.x) {
            var1.setColor(6710886);
            int var3 = this.b() - 2 * (this.o - 10);
            int var4 = 1;
            if (this.y != null) {
               if (this.x + 1 != this.y.length) {
                  var4 = this.y[this.x + 1] - this.y[this.x];
               } else {
                  var4 = this.F - this.y[this.x];
               }
            }

            var4 = var4 * this.a + 2;
            var1.fillRect(this.o - 10, this.n - 1, var3, var4);
         }

         var1.setColor(16776960);
         var1.drawString(this.z[var2], this.o, this.n, 20);
         this.n = this.n + this.a;
         this.n++;
      }
   }

   private void g(Graphics var1) {
      var1.setColor(2510210);
      var1.fillRect(0, 0, this.b(), 20 + this.h());
      this.a(var1);
      var1.setFont(this.e);
      this.a = this.e.getHeight();
      this.n = 20;
      var1.setColor(16776960);

      for (int var2 = this.A; var2 <= this.c; var2++) {
         var1.drawString(this.z[var2], this.o, this.n, 20);
         this.n = this.n + this.a;
      }

      if (this.A > 0) {
         this.a(var1, 155, 180, 1);
      }

      if (this.c + 1 < this.F) {
         this.a(var1, 165, 180, 2);
      }
   }

   private void a(Graphics var1, int var2, int var3, int var4) {
      int var5 = var1.getColor();
      var1.setColor(0);
      byte var6 = 5;
      if (var4 == 1) {
         for (int var7 = 0; var7 < var6; var7++) {
            var1.drawLine(var2 - var7, var3 + var7, var2 + var7, var3 + var7);
         }
      } else {
         for (int var8 = 0; var8 < var6; var8++) {
            var1.drawLine(var2 - (var6 - var8), var3 + var8, var2 + (var6 - var8), var3 + var8);
         }
      }
   }

   String[] b(String var1) {
      int var2 = this.C.getWidth() - this.o - this.j;
      return this.C.a(var1, var2, this.e);
   }

   protected void c(int var1) {
      if (var1 == -6) {
         Command var2 = this.m();
         if (var2 != null) {
            this.b.commandAction(var2, this.C);
            return;
         }
      } else if (var1 == -7) {
         Command var4 = this.g();
         if (var4 != null) {
            this.b.commandAction(var4, this.C);
            return;
         }
      }

      ESGame.debugCode = 21;
      int var5 = this.C.getGameAction(var1);
      switch (var5) {
         case 1:
            if (this.h != 3 && this.h != 5 && this.h != 6) {
               if (this.h == 4) {
                  int var6 = this.a();
                  if (this.A > 0) {
                     this.A--;
                     this.c--;
                     if (this.C.e()) {
                        this.C.an = true;
                     } else {
                        this.C.repaint();
                        this.C.serviceRepaints();
                     }
                  }
               }
            } else if (this.y != null) {
               if (this.x + 1 > 1) {
                  this.x--;
                  if (this.y[this.x] < this.A) {
                     this.c = this.c - (this.A - this.y[this.x]);
                     this.A = this.y[this.x];
                  }

                  if (this.C.e()) {
                     this.C.an = true;
                  } else {
                     this.C.repaint();
                     this.C.serviceRepaints();
                  }
               }
            } else if (this.x > 0) {
               this.x--;
               if (this.A > this.x) {
                  this.A--;
                  this.c--;
               }

               if (this.C.e()) {
                  this.C.an = true;
               } else {
                  this.C.repaint();
                  this.C.serviceRepaints();
               }
            }
            break;
         case 6:
            if (this.h != 3 && this.h != 5 && this.h != 6) {
               if (this.h == 4 && this.c < this.F - 1) {
                  this.A++;
                  this.c++;
                  if (this.C.e()) {
                     this.C.an = true;
                  } else {
                     this.C.repaint();
                     this.C.serviceRepaints();
                  }
               }
            } else if (this.y != null) {
               if (this.x + 1 < this.y.length) {
                  this.x++;
                  int var3 = this.c - this.A;
                  if (this.x + 1 == this.y.length) {
                     this.A = this.F - var3 - 1;
                     this.c = this.F - 1;
                  } else if (this.y[this.x + 1] > this.c) {
                     this.c = this.y[this.x + 1];
                     this.A = this.c - var3;
                  }

                  if (this.C.e()) {
                     this.C.an = true;
                  } else {
                     this.C.repaint();
                     this.C.serviceRepaints();
                  }
               }
            } else if (this.x < this.z.length - 1) {
               this.x++;
               if (this.c < this.x) {
                  this.A++;
                  this.c++;
               }

               if (this.C.e()) {
                  this.C.an = true;
               } else {
                  this.C.repaint();
                  this.C.serviceRepaints();
               }
            }
      }
   }

   String k() {
      return this.z[this.x];
   }

   int a() {
      switch (this.h) {
         case 3:
         case 5:
         case 6:
            return this.x;
         case 4:
         default:
            return -1;
      }
   }

   void b(int var1) {
      switch (this.h) {
         case 3:
         case 5:
         case 6:
            this.x = var1;
            if (this.x >= this.z.length) {
               this.x = this.z.length - 1;
            }

            if (this.x > this.c) {
               int var2 = this.x - this.c;
               this.c += var2;
               this.A += var2;
            } else if (this.x < this.A) {
               int var3 = this.A - this.x;
               this.c -= var3;
               this.A -= var3;
            }
         case 4:
      }
   }

   void a(int var1, String var2) {
      if (var1 != 0 || this.h != 5 && this.h != 6 && this.h != 4) {
         if (var1 == 1 && this.h == 6) {
            this.k = this.b(var2);
         }
      } else {
         this.m = this.b(var2);
      }
   }

   void a(String var1) {
      this.q = var1;
   }

   void c(String var1) {
      if (this.h == 5 || this.h == 6) {
         this.m = this.b(var1);
      } else if (this.h == 4) {
         this.z = this.b(var1);
      }

      this.A = 0;
   }

   String n() {
      if (this.h == 5 || this.h == 6) {
         return this.m[0];
      } else {
         return this.h == 4 ? this.z[0] : null;
      }
   }

   public void a(Command var1) {
      this.u.addElement(var1);
   }

   public void b(Command var1) {
      this.u.removeElement(var1);
   }

   public void a(CommandListener var1) {
      this.b = var1;
   }

   private void c(Graphics var1) {
      if (this.u.size() != 0) {
         var1.setColor(16777215);
         var1.fillRect(0, 190, this.b(), 20);
         int var2 = this.u.size();
         var1.setColor(0);
         var1.setFont(E);
         Command var3 = this.m();
         if (var3 != null) {
            var1.drawString(var3.getLabel(), 10, 194, 20);
         }

         Command var4 = this.g();
         if (var4 != null) {
            var1.drawString(var4.getLabel(), this.b() - 10, 194, 24);
         }
      }
   }

   private Command g() {
      int var1 = this.u.size();
      Command var2 = null;
      if (var1 == 1) {
         var2 = (Command)this.u.elementAt(0);
      } else if (var1 == 2) {
         for (int var3 = 0; var3 < 2; var3++) {
            Command var4 = (Command)this.u.elementAt(var3);
            if (var4 == ESGame.okCommand || var4 == ESGame.selectCommand) {
               var2 = var4;
               break;
            }
         }
      }

      return var2;
   }

   private Command m() {
      int var1 = this.u.size();
      Command var2 = null;
      if (var1 == 2) {
         for (int var3 = 0; var3 < 2; var3++) {
            Command var4 = (Command)this.u.elementAt(var3);
            if (var4 == ESGame.backCommand || var4 == ESGame.cancelCommand) {
               var2 = var4;
               break;
            }
         }
      }

      return var2;
   }

   public int b() {
      return this.C.ay;
   }

   public int h() {
      return this.C.l;
   }
}
