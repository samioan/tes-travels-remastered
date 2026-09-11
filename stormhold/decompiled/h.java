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
import javax.microedition.lcdui.Screen;
import javax.microedition.lcdui.StringItem;

public class h implements Runnable {
   static c[] K = new c[2];
   static int x = 0;
   static Image r;
   Vector q;
   CommandListener J;
   static final Font O;
   private static final Font H;
   private static final Font p;
   static final Font d;
   static final Font l;
   static final Command I;
   static final Command u;
   static final Command P;
   static final Command z;
   static final Command a;
   static final String[] y;
   int Q;
   int B;
   int N;
   private ESGame L;
   Screen i;
   int f;
   private int h;
   Vector n;
   String M;
   String[] E;
   volatile int m;
   private boolean j;
   private Thread D;
   long C;
   Displayable t;
   Object s;
   Object c;
   int o;
   int e;
   int G;
   Font b;
   int g;
   int F;
   int w;
   int v;
   int k;
   boolean A = false;

   public h(ESGame var1, int var2, int var3) {
      this.L = var1;
      this.Q = var2;
      this.B = var3;
      this.N = 0;
      this.i = null;
      this.f = 0;
      this.s = null;
      this.c = null;
      this.M = null;
      this.E = null;
      this.D = null;
      this.q = new Vector(5);
   }

   void e() {
      this.t = K[x];
   }

   void o() {
      this.t = K[x];
   }

   void a(String var1, String[] var2, Vector var3) {
      this.a(var1, var2, var3, true);
   }

   void a(String var1, String[] var2, Vector var3, boolean var4) {
      List var5 = new List(var1, 3);
      this.i = var5;
      this.f = 1;
      int var6 = var2.length;

      for (int var7 = 0; var7 < var6; var7++) {
         var5.append(var2[var7], null);
      }

      this.n = var3;
      this.w = 0;
      this.v = 0;
      this.k = 0;
      this.t = K[x];
      this.a(u);
      if (var4) {
         this.a(P);
      }

      this.a(this.L);
   }

   void n() {
      String var1 = "";

      for (int var2 = 0; var2 < y.length; var2++) {
         var1 = var1 + y[var2];
      }

      this.a("Exiting", var1);
      this.b(I);
      this.a(a);
   }

   void a(String var1, String var2) {
      this.a(var1, var2, false);
   }

   void a(String var1, String var2, boolean var3) {
      Form var4 = new Form(var1);
      this.i = var4;
      this.f = 2;
      StringItem var5 = new StringItem(null, var2);
      var4.append(var5);
      this.g = 10;
      this.F = 10;
      this.w = 0;
      this.v = 0;
      this.k = 0;
      this.t = K[x];
      this.a(I);
      this.a(this.L);
   }

   void a(String var1, String var2, String[] var3, Vector var4) {
      this.a(var1, var2, var3, var4, false);
   }

   void a(String var1, String var2, String[] var3, Vector var4, boolean var5) {
      Form var6 = new Form(var1);
      this.i = var6;
      this.f = 2;
      StringItem var7 = new StringItem(null, var2);
      var6.append(var7);
      this.g = 15;
      this.F = 15;
      ChoiceGroup var8 = new ChoiceGroup(null, 1);
      int var9 = var3.length;

      for (int var10 = 0; var10 < var9; var10++) {
         var8.append(var3[var10], null);
      }

      this.h = 1;
      var6.append(var8);
      this.n = var4;
      this.w = 0;
      this.v = 0;
      this.k = 0;
      this.t = K[x];
      this.a(u);
      this.a(P);
      this.a(this.L);
      if (var2 != null && var2.indexOf("<TAG>") >= 0) {
         this.M = new String(var2);
      }
   }

   void a(String var1, String var2, String var3, String[] var4, Vector var5) {
      Form var6 = new Form(var1);
      this.i = var6;
      this.f = 2;
      StringItem var7 = new StringItem(null, var2);
      var6.append(var7);
      StringItem var8 = new StringItem(null, var3);
      var6.append(var8);
      this.g = 15;
      this.F = 15;
      ChoiceGroup var9 = new ChoiceGroup(null, 1);
      int var10 = var4.length;

      for (int var11 = 0; var11 < var10; var11++) {
         var9.append(var4[var11], null);
      }

      this.h = 2;
      var6.append(var9);
      this.n = var5;
      this.t = K[x];
      this.a(u);
      this.a(P);
      this.a(this.L);
   }

   public void e(Graphics var1) {
      Graphics var2 = r.getGraphics();
      switch (this.Q) {
         case 1:
            System.out.println("        IN CANVAS DOWNLOAD PAINT!");
            break;
         case 2:
            this.f(var2);
            break;
         case 3:
            this.c(var2);
            break;
         case 4:
            this.d(var2);
            break;
         case 5:
            this.a(var2, 1);
            break;
         case 6:
            this.a(var2, 2);
         case 7:
         default:
            break;
         case 8:
         case 9:
         case 10:
         case 11:
            this.b(var2);
      }

      this.a(var2);
      var1.drawImage(r, 0, 0, 20);
   }

   private void f(Graphics var1) {
      var1.setColor(0);
      var1.fillRect(0, 0, this.b(), 20 + this.k());
      if (ESGame.aG) {
         var1.setColor(16777215);
         var1.fillRect(0, 0, this.b(), 20 + this.k());
         var1.drawImage(ESGame.aA, this.b() / 2, 10, 17);
         int var2 = 10 + ESGame.aA.getHeight() + 3;
         var1.setColor(0);

         for (int var3 = 0; var3 < y.length; var3++) {
            var1.drawString(y[var3], this.b() / 2, var2, 17);
            var2 += 14;
         }

         var1.drawString("Distributed by:", this.b() / 2, 143, 17);
         var1.drawImage(ESGame.aa, this.b() / 2, 158, 17);
      } else if (this.L.ac) {
         var1.setColor(0);
         var1.fillRect(0, 0, this.b(), 20 + this.k());
         var1.drawImage(ESGame.O, this.b() / 2, 20, 17);
         var1.drawImage(ESGame.aU, this.b() / 2, 100, 17);
      } else {
         var1.setColor(0);
         var1.fillRect(0, 0, this.b(), 20 + this.k());
         var1.drawImage(ESGame.O, this.b() / 2, 20, 17);
         var1.drawImage(ESGame.aU, this.b() / 2, 100, 17);
         var1.setColor(16777215);
         var1.fillRect(12, 165, 152, 22);
         var1.setColor(10485760);
         var1.fillRect(13, 166, 3 * this.m / 2, 20);
      }
   }

   private void b(Graphics var1) {
      var1.setColor(11429934);
      var1.fillRect(0, 0, this.b(), 20 + this.k());
      var1.setFont(l);
      var1.setColor(16777215);
      int var2 = this.b() / 2;
      if (this.Q == 8) {
         var1.drawString("Creating New Game", var2, 30, 17);
      } else if (this.Q == 9) {
         var1.drawString("Loading Game", var2, 30, 17);
      } else if (this.Q == 10) {
         var1.drawString("Saving Game", var2, 30, 17);
      } else if (this.Q == 11) {
         var1.drawString("Loading Dungeon", var2, 30, 17);
      }

      var1.drawString("Please Wait", var2, 45, 17);
      var1.setColor(16777215);
      var1.fillRect((this.b() - 90) / 2, 60, 90, 20);
      int var3 = this.m * 88 / 100;
      var1.setColor(255);
      var1.fillRect((this.b() - 88) / 2, 61, var3, 18);
   }

   private void a(Graphics var1, String var2) {
      Font var3 = var1.getFont();
      var1.setFont(H);
      int var4 = var1.getColor();
      var1.setColor(0);
      var1.fillRect(0, 0, this.b(), 14);
      var1.setColor(16777215);
      var1.drawString(var2, this.b() / 2, 0, 17);
      var1.setColor(var4);
      var1.setFont(var3);
   }

   private void c(Graphics var1) {
      var1.setColor(11429934);
      var1.fillRect(0, 0, this.b(), 20 + this.k());
      this.a(var1, this.i.getTitle());
      this.b = d;
      var1.setFont(this.b);
      this.o = this.b.getHeight();
      this.e = 20;
      this.G = 15;
      String[] var2 = this.r();
      int var3 = var2.length;
      byte var4 = 10;
      int var5 = Math.min(var3, var4);
      this.v = this.w + var5 - 1;

      for (int var6 = this.w; var6 <= this.v; var6++) {
         this.a(var1, var2[var6], var6 == this.a());
      }

      if (this.w > 0) {
         this.a(var1, 155, 180, 1);
      }

      if (this.v + 1 < var3) {
         this.a(var1, 165, 180, 2);
      }
   }

   private void a(Graphics var1, int var2) {
      Form var3 = (Form)this.i;
      var1.setColor(11429934);
      var1.fillRect(0, 0, this.b(), 20 + this.k());
      this.a(var1, var3.getTitle());
      this.b = d;
      var1.setFont(this.b);
      this.o = this.b.getHeight();
      this.e = 20;
      this.G = 15;

      for (int var4 = 0; var4 < var2; var4++) {
         StringItem var5 = (StringItem)var3.get(var4);
         String var6 = var5.getText();
         if (this.A) {
            String[] var7 = this.c(var6);

            for (int var8 = 0; var8 < var7.length; var8++) {
               var1.setColor(16776960);
               var1.drawString(var7[var8], this.G, this.e, 20);
               this.e = this.e + this.o;
            }
         } else {
            if (var4 == 0) {
               var1.setColor(16776960);
            } else {
               var1.setColor(16777215);
            }

            var1.drawString(var6, this.G, this.e, 20);
            this.e = this.e + this.o;
         }
      }

      this.e += 5;
      String[] var10 = this.r();
      int var11 = var10.length;
      byte var12 = 9;
      int var13 = Math.min(var11, var12);
      this.v = this.w + var13 - 1;

      for (int var9 = this.w; var9 <= this.v; var9++) {
         this.a(var1, var10[var9], var9 == this.a());
      }

      if (this.w > 0) {
         this.a(var1, 155, 180, 1);
      }

      if (this.v + 1 < var11) {
         this.a(var1, 165, 180, 2);
      }
   }

   private void a(Graphics var1, String var2, boolean var3) {
      if (var3) {
         var1.setColor(6710886);
         int var4 = this.b() - 2 * (this.G - 10);
         int var5 = this.o + 2;
         var1.fillRect(this.G - 10, this.e - 1, var4, var5);
      }

      var1.setColor(16776960);
      var1.drawString(var2, this.G, this.e, 20);
      this.e = this.e + this.o;
      this.e++;
   }

   private void d(Graphics var1) {
      Form var2 = (Form)this.i;
      var1.setColor(11429934);
      var1.fillRect(0, 0, this.b(), 20 + this.k());
      this.a(var1, var2.getTitle());
      this.b = d;
      var1.setFont(this.b);
      this.o = this.b.getHeight();
      this.e = 20;
      this.G = 5;
      StringItem var3 = (StringItem)var2.get(0);
      String var4 = var3.getText();
      String[] var5 = this.c(var4);
      this.k = var5.length;
      byte var6 = 11;
      int var7 = Math.min(this.k, var6);
      this.v = this.w + var7 - 1;
      var1.setColor(16776960);

      for (int var8 = this.w; var8 <= this.v; var8++) {
         String var9 = var5[var8];
         var1.drawString(var9, this.G, this.e, 20);
         this.e = this.e + this.o;
      }

      if (this.w > 0) {
         this.a(var1, 155, 180, 1);
      }

      if (this.v + 1 < this.k) {
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

   String[] c(String var1) {
      Vector var2 = new Vector();

      for (String var6 : this.b(var1)) {
         this.a(var2, var6);
      }

      return a(var2);
   }

   private String[] b(String var1) {
      Vector var2 = new Vector();
      int var3 = 0;
      int var4 = 0;

      do {
         var4 = var1.indexOf(10, var3);
         if (var4 < 0) {
            var4 = var1.length();
            var2.addElement(var1.substring(var3, var4));
            break;
         }

         var2.addElement(var1.substring(var3, var4));
         var3 = var4 + 1;
      } while (var3 < var1.length());

      return a(var2);
   }

   private void a(Vector var1, String var2) {
      int var3 = this.b() - this.g - this.F;
      String[] var4 = f.c(var2);
      int var5 = var4.length;
      String var6 = "";

      for (int var7 = 0; var7 < var5; var7++) {
         String var8 = var4[var7];
         String var9 = var6 + var8;
         if (this.b.stringWidth(var9) > var3) {
            if (this.b.stringWidth(var8) <= var3) {
               var1.addElement(new String(var6));
               var9 = var8;
            } else {
               if (var6.length() > 0) {
                  var1.addElement(new String(var6));
               }

               String[] var10 = this.d(var8);

               for (int var11 = 0; var11 < var10.length - 1; var11++) {
                  var1.addElement(new String(var10[var11]));
               }

               var9 = var10[var10.length - 1];
            }
         }

         var6 = var9;
         if (var7 < var5 - 1) {
            var9 = var6 + " ";
            if (this.b.stringWidth(var9) > var3) {
               var1.addElement(new String(var6));
               var6 = "";
            } else {
               var6 = var9;
            }
         }
      }

      var1.addElement(new String(var6));
   }

   private String[] d(String var1) {
      int var2 = this.b() - this.g - this.F;
      Vector var3 = new Vector();
      int var4 = var1.length();
      String var5 = "";

      for (int var6 = 0; var6 < var4; var6++) {
         char var7 = var1.charAt(var6);
         String var8 = var5 + var7;
         if (this.b.stringWidth(var8) > var2) {
            var3.addElement(new String(var5));
            var5 = var7 + "";
         } else {
            var5 = var8;
         }
      }

      var3.addElement(new String(var5));
      return a(var3);
   }

   private static String[] a(Vector var0) {
      int var1 = var0.size();
      String[] var2 = new String[var1];

      for (int var3 = 0; var3 < var1; var3++) {
         var2[var3] = (String)var0.elementAt(var3);
      }

      return var2;
   }

   protected void b(int var1) {
      if (var1 == -6) {
         Command var2 = this.s();
         if (var2 != null) {
            this.J.commandAction(var2, this.t);
            return;
         }
      } else if (var1 == -7) {
         Command var4 = this.l();
         if (var4 != null) {
            this.J.commandAction(var4, this.t);
            return;
         }
      }

      int var5 = this.c(var1);
      switch (var5) {
         case 1:
            if (this.Q == 3 || this.Q == 5 || this.Q == 6) {
               int var8 = this.a();
               if (var8 > 0) {
                  this.a(--var8);
                  if (this.w > var8) {
                     this.w--;
                     this.v--;
                  }

                  this.c();
                  this.f();
               }
            } else if (this.Q == 4) {
               int var7 = this.a();
               if (this.w > 0) {
                  this.w--;
                  this.v--;
                  this.c();
                  this.f();
               }
            }
            break;
         case 6:
            if (this.Q == 3 || this.Q == 5 || this.Q == 6) {
               int var3 = this.a();
               if (var3 < this.d() - 1) {
                  this.a(++var3);
                  if (this.v < var3) {
                     this.w++;
                     this.v++;
                  }

                  this.c();
                  this.f();
               }
            } else if (this.Q == 4 && this.v < this.k - 1) {
               this.w++;
               this.v++;
               this.c();
               this.f();
            }
      }
   }

   String[] r() {
      int var1 = 0;
      switch (this.Q) {
         case 3:
            List var8 = (List)this.i;
            var1 = var8.size();
            String[] var9 = new String[var1];

            for (int var10 = 0; var10 < var1; var10++) {
               var9[var10] = var8.getString(var10);
            }

            return var9;
         case 4:
         default:
            return null;
         case 5:
         case 6:
            Form var2 = (Form)this.i;
            ChoiceGroup var3 = (ChoiceGroup)var2.get(this.h);
            var1 = var3.size();
            String[] var4 = new String[var1];

            for (int var5 = 0; var5 < var1; var5++) {
               var4[var5] = var3.getString(var5);
            }

            return var4;
      }
   }

   String p() {
      int var1 = this.a();
      String[] var2 = this.r();
      return var2[var1];
   }

   int a() {
      byte var1 = -1;
      switch (this.Q) {
         case 3:
            List var2 = (List)this.i;
            return var2.getSelectedIndex();
         case 4:
         default:
            return var1;
         case 5:
         case 6:
            Form var3 = (Form)this.i;
            ChoiceGroup var4 = (ChoiceGroup)var3.get(this.h);
            return var4.getSelectedIndex();
      }
   }

   void a(int var1) {
      switch (this.Q) {
         case 3:
            List var2 = (List)this.i;
            var2.setSelectedIndex(var1, true);
         case 4:
         default:
            break;
         case 5:
         case 6:
            Form var3 = (Form)this.i;
            ChoiceGroup var4 = (ChoiceGroup)var3.get(this.h);
            var4.setSelectedIndex(var1, true);
      }
   }

   protected void h() {
      switch (this.Q) {
         case 2:
            this.g();
      }
   }

   protected void q() {
      switch (this.Q) {
         case 1:
         case 2:
            this.i();
      }
   }

   private void g() {
      System.out.println("IN START HELPER THREAD IN UICANVAS");
      this.D = new Thread(this);
      System.out.println("Helper thread in UICanvas: " + this.D);
      System.out.println("num active threads = " + Thread.activeCount());
      this.j = true;
      this.D.start();
   }

   private void i() {
      this.j = false;
   }

   public void run() {
      if (this.Q == 2) {
         this.m();
      } else if (this.Q == 1) {
         System.out.println("Running a download helper thread to repaint");
      }
   }

   private void m() {
      try {
         this.C = 0L;
         System.out.println("Just before helper thread loop in UICanvas");

         while (this.j && (this.m < 100 || this.C < 4000L)) {
            long var1 = System.currentTimeMillis();
            this.c();
            this.f();
            long var3 = System.currentTimeMillis() - var1;

            try {
               if (var3 < 500L) {
                  long var5 = 500L - var3;
                  Thread.sleep(500L - var3);
               }
            } catch (Exception var7) {
            }

            var3 = System.currentTimeMillis() - var1;
            this.C += var3;
            System.out.println("Progress pct is " + this.m);
         }

         ESGame.aG = true;
         this.L.ac = false;
         this.a(2000L);
         this.L.ac = true;
         ESGame.aG = false;
         this.a(1000L);
         this.L.ac = false;
         ESGame.aa = null;
         ESGame.aA = null;
         ESGame.O = null;
         ESGame.aU = null;
         this.D = null;
         ESGame.c("After nuking splash");
         System.out.println("End of splash, changing to next");
         this.L.a(this.c);
      } catch (Throwable var8) {
         var8.printStackTrace();
         this.L.a(this.L.w);
      }
   }

   private void a(long var1) {
      long var3 = 0L;

      do {
         this.c();
         this.f();

         try {
            Thread.sleep(500L);
         } catch (Exception var6) {
         }

         var3 += 500L;
      } while (var3 <= var1);
   }

   int d() {
      if (this.Q == 3) {
         List var3 = (List)this.i;
         return var3.size();
      }

      if (this.Q != 5 && this.Q != 6) {
         return 0;
      }

      Form var1 = (Form)this.i;
      ChoiceGroup var2 = (ChoiceGroup)var1.get(this.h);
      return var2.size();
   }

   void a(int var1, String var2) {
      Form var3 = (Form)this.i;
      if (this.Q == 6) {
         System.out.println("type is form2, size is " + var3.size());
      }

      if (this.Q == 5 || this.Q == 6 || this.Q == 4) {
         StringItem var4 = (StringItem)var3.get(var1);
         var4.setText(var2);
      }
   }

   void a(String var1) {
      if (this.i != null) {
         this.i.setTitle(var1);
      }
   }

   void e(String var1) {
      Form var2 = (Form)this.i;

      try {
         if (var2 != null) {
            StringItem var3 = (StringItem)var2.get(0);
            var3.setText(var1);
         }
      } catch (Throwable var4) {
      }
   }

   String t() {
      Form var1 = (Form)this.i;

      try {
         if (var1 != null) {
            StringItem var2 = (StringItem)var1.get(0);
            return var2.getText();
         }
      } catch (Throwable var3) {
      }

      return null;
   }

   public void a(Command var1) {
      this.q.addElement(var1);
   }

   public void b(Command var1) {
      this.q.removeElement(var1);
   }

   public void a(CommandListener var1) {
      this.J = var1;
   }

   private void a(Graphics var1) {
      if (this.q.size() != 0) {
         var1.setColor(16777215);
         var1.fillRect(0, 190, this.b(), 20);
         int var2 = this.q.size();
         var1.setColor(0);
         var1.setFont(O);
         Command var3 = this.s();
         if (var3 != null) {
            var1.drawString(var3.getLabel(), 10, 192, 20);
         }

         Command var4 = this.l();
         if (var4 != null) {
            var1.drawString(var4.getLabel(), this.b() - 10, 195, 24);
         }
      }
   }

   private Command l() {
      int var1 = this.q.size();
      Command var2 = null;
      if (var1 == 1) {
         var2 = (Command)this.q.elementAt(0);
      } else if (var1 == 2) {
         for (int var3 = 0; var3 < 2; var3++) {
            Command var4 = (Command)this.q.elementAt(var3);
            if (var4 == I || var4 == u) {
               var2 = var4;
               break;
            }
         }
      }

      return var2;
   }

   private Command s() {
      int var1 = this.q.size();
      Command var2 = null;
      if (var1 == 2) {
         for (int var3 = 0; var3 < 2; var3++) {
            Command var4 = (Command)this.q.elementAt(var3);
            if (var4 == z || var4 == P) {
               var2 = var4;
               break;
            }
         }
      }

      return var2;
   }

   static c j() {
      return K[x];
   }

   public void c() {
      if (ESGame.ax == null || this.B == ESGame.ax.B) {
         K[x].repaint();
      }
   }

   public void f() {
      K[x].serviceRepaints();
   }

   public int b() {
      return K[x].getWidth();
   }

   public int k() {
      return K[x].getHeight();
   }

   public int c(int var1) {
      return K[x].getGameAction(var1);
   }

   static {
      K[0] = new c();
      K[1] = new c();

      try {
         r = Image.createImage(K[0].getWidth(), K[0].getHeight());
      } catch (Throwable var1) {
         System.out.println("Error allocating bufferImage");
      }

      O = Font.getFont(0, 1, 8);
      H = Font.getFont(0, 1, 0);
      p = Font.getFont(64, 0, 8);
      d = Font.getFont(0, 1, 8);
      l = Font.getFont(0, 1, 0);
      I = new Command("Ok", 3, 0);
      u = new Command("Select", 3, 0);
      P = new Command("Cancel", 4, 0);
      z = new Command("Back", 4, 0);
      a = new Command("Exit", 7, 0);
      y = new String[]{
         "(c) 2003 Vir2L Studios, ",
         "a ZeniMax Media company. ",
         "The Elder Scrolls and Vir2L ",
         "are registered trademarks ",
         "of ZeniMax Media Inc. ",
         "All rights reserved."
      };
   }
}
