import com.nokia.mid.ui.DirectGraphics;
import com.nokia.mid.ui.DirectUtils;
import com.nokia.mid.ui.FullCanvas;
import java.util.Enumeration;
import java.util.Hashtable;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

public class e extends FullCanvas implements Runnable {
   private static final Font aj = Font.getFont(64, 0, 8);
   private static final Font M = Font.getFont(64, 2, 16);
   private static final Font w = Font.getFont(64, 2, 16);
   private static final Font K = Font.getFont(64, 1, 16);
   static final int[][][] n = new int[][][]{
      {{12, 0, 0, 1}, {11, 0, -1, 1}, {12, 1, -1, 2}, {12, 2, -1, 3}, {11, 2, -2, 3}, {12, 3, -2, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {11, 1, -1, 2}, {12, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {11, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {11, 3, -1, 4}, {11, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {12, 3, 0, 4}, {12, 3, 0, 4}}
   };
   private static final byte[][] ae = new byte[][]{
      {1, 5, 31, 53, 0, 1, 40, -39, 1, 4, 13, -2, 3, 6, 71, 4, 30, 64, 2, 0, 0, 0},
      {6, 10, 31, 53, 7, 1, 27, -35, 8, 4, 1, 69, 11, 27, 66, 9, 33, 10, 10, 0, 0, 0},
      {11, 25, 31, 20, 14, 1, 0, 0, -1, -1, 2, 25, 15, 81, 8, 16, 9, 0, 17, 60, 57, 18},
      {26, 40, 31, 32, 21, 1, 0, 0, -1, -1, 43, 44, 22, 50, 25, 23, -36, 9, 24, -25, 44, 25}
   };
   private static final byte[][] a = new byte[][]{
      {0, 0},
      {0, 0},
      {0, 3},
      {0, 3},
      {0, 3},
      {0, 2},
      {0, 2},
      {0, 3},
      {0, 3},
      {0, 3},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0}
   };
   private static final boolean[][] J = new boolean[][]{
      {false, false, false, false},
      {true, false, false, false},
      {false, false, false, false},
      {false, false, true, false},
      {true, false, true, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, true, false, false},
      {false, false, true, false},
      {false, true, true, false},
      {true, false, false, false},
      {false, true, false, false},
      {true, false, true, false},
      {false, true, true, false},
      {true, true, true, false},
      {true, false, false, false},
      {false, true, false, false},
      {true, false, true, false},
      {false, true, true, false},
      {true, true, true, false},
      {true, true, false, false},
      {true, true, false, false},
      {true, true, false, false},
      {true, true, true, false},
      {true, true, true, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, true, false, true},
      {false, true, false, true},
      {true, false, false, true},
      {true, false, true, false},
      {false, true, true, false},
      {false, false, false, false}
   };
   private static final char[] e = new char[]{'1', '3', '5', '7', '9', '0'};
   private static final char[] O = new char[]{'0', 'N', 'E', 'S', 'W'};
   private static final int[][] o = new int[][]{{0, 0, 0}, {0, 1, 0}, {0, 2, 1}};
   private ESGame H;
   private int ac;
   private int T;
   private Thread au = new Thread(this);
   private boolean ak;
   private boolean z;
   private boolean aB;
   j ax;
   byte aD;
   boolean v;
   byte c;
   long aC;
   long s;
   long B;
   long V;
   boolean aw;
   int p;
   static boolean aa;
   static boolean m;
   static boolean R;
   static boolean W;
   static Image h;
   static Image Y;
   static g[] q;
   static Image[] aE;
   static g[] ab;
   static g[] ar;
   static g[] u;
   static Image[] y;
   private static byte[][] l;
   private static byte[][] F;
   private static int f = 1;
   static boolean S = false;
   static boolean ao = false;
   static boolean am = false;
   static int aq;
   static boolean av = false;
   static boolean ay = false;
   static boolean I = false;
   static boolean Z = false;
   static boolean ap = false;
   static boolean U = false;
   static boolean ad = false;
   static long as = 0L;
   static String[] d = null;
   private static int X = 0;
   static boolean af = false;
   private static boolean al = true;
   private static boolean at = false;
   static boolean E = false;
   static boolean A = false;
   static final String[] r = new String[]{"Warden's", "Camp"};
   static final String[] ag = new String[]{"Outer", "Camp"};
   static final String[] C = new String[]{"Cannot", "Camp!"};
   static final String[] Q = new String[]{"No spells!", ""};
   static final String[] t = new String[]{"Not enough", "magic!"};
   static final String[] g = new String[]{"No monster", "here!"};
   static final String[] N = new String[]{"Rest", "disturbed!"};
   static final String[] b = new String[]{"Rest", "complete!"};
   static final String[] P = new String[]{"Creature", "is dead!"};
   static final String[] ah = new String[]{"Creature", "attacks!"};
   static final String[] x = new String[]{"Chest", ""};
   static final String[] D = new String[]{"Chest", "locked!"};
   static final String[] aA = new String[]{"Inventory", "full!"};
   static final String[] L = new String[]{"Found", "item!"};
   static final String[] an = new String[]{"Several", "items!"};
   static final String[][] i = new String[][]{
      {"Arantamo", ""}, {"Celegil", ""}, {"Favela Dralor", ""}, {"Vander", ""}, {"Beneca", ""}, {"Helga", ""}, {"Varus", ""}
   };
   static d k = new d();
   static d j = null;
   private static long ai = 0L;
   private static boolean az = false;
   private static String G = null;

   public e(ESGame var1) {
      this.H = var1;
      this.T = 0;
      this.ac = 0;
      this.z = false;
      this.ak = false;
      this.aB = false;
      this.ax = null;
      this.aD = 1;
      this.v = false;
      this.c = 0;
      this.s = 0L;
      this.aC = 0L;
      this.p = 0;
      this.aw = false;
      aa = false;
      m = false;
      R = false;
      W = false;
      l = new byte[7][7];
      F = new byte[17][17];
      aq = 0;
      this.B = 0L;
      this.V = 0L;
   }

   public void paint(Graphics var1) {
      if (this.aD == 3) {
         this.f(var1);
      } else if (this.c != 1 && this.c != 2) {
         this.m(var1);
      } else {
         this.c(var1);
      }
   }

   private void f(Graphics var1) {
      var1.setColor(0);
      var1.fillRect(0, 0, this.getWidth(), this.getHeight());
      var1.setColor(16777215);
      var1.setFont(w);
      var1.drawString("You're Dead!", this.getWidth() / 2, this.getHeight() / 2, 33);
   }

   private void c(Graphics var1) {
      var1.setColor(0);
      var1.fillRect(0, 0, this.getWidth(), this.getHeight());
      var1.setColor(16777215);
      var1.setFont(w);
      var1.drawString("CAMPING", this.getWidth() / 2, this.getHeight() / 2, 33);
   }

   private void m(Graphics var1) {
      var1.setColor(0);
      var1.fillRect(0, 0, this.getWidth(), this.getHeight());
      this.j(var1);
      this.b(var1);
      if (W) {
         int var2 = this.ax.r();
         this.b(var1, var2);
      }

      try {
         this.g(var1);
      } catch (Throwable var3) {
         System.out.println("Error in paintMonsters: " + var3);
      }

      this.a(var1);
      this.d(var1);
      this.l(var1);
      this.e(var1);
      if (az) {
         this.i(var1);
      }

      if (f == 1 && !this.ax.k(3)) {
         this.k(var1);
      }

      if (f == 2 && !this.ax.k(3)) {
         this.h(var1);
      }
   }

   private void i(Graphics var1) {
      var1.setColor(16777215);
      var1.drawString(G, 60, 10, 17);
   }

   private void e(Graphics var1) {
      if (S) {
         int var2 = 40 + f.a(30);
         int var3 = 50 + f.a(20);
         var1.drawImage(aE[0], var2, var3, 20);
         S = false;
      }

      if (ao) {
         int var4 = 40 + f.a(30);
         int var6 = 50 + f.a(22);
         var1.drawImage(aE[1], var4, var6, 20);
         ao = false;
      }

      if (am) {
         int var5 = 50 + f.a(2);
         int var7 = 80 + f.a(2);
         var1.drawImage(aE[2], var5, var7, 20);
         am = false;
      }
   }

   private void j(Graphics var1) {
      i var2 = this.ax.b();
      byte[][] var3 = this.ax.ae;
      if (!this.ax.k(3)) {
         if (this.ax.k(4)) {
            var1.setColor(10485760);
            var1.fillRect(0, 0, this.getWidth(), h.getHeight());
         } else {
            for (int var6 = 0; var6 < 5; var6++) {
               var1.drawImage(h, var6 * 36, 0, 20);
            }
         }
      }

      for (int var15 = 0; var15 < 5; var15++) {
         int var5 = var15 * 18;

         for (int var7 = 0; var7 < 6; var7++) {
            int var8 = n[var15][var7][0];
            int var9 = n[var15][var7][1];
            int var10 = n[var15][var7][2];
            int var11 = n[var15][var7][3];
            if (f.a((byte)1, var2.a(var10, var11, var3))) {
               int var4 = this.a(var8, var9, -1);
               this.a(var1, var4, var5);
               break;
            }
         }
      }

      for (int var16 = 5; var16 < 10; var16++) {
         int var14 = var16 * 18;

         for (int var17 = 0; var17 < 6; var17++) {
            int var18 = n[9 - var16][var17][0];
            int var19 = n[9 - var16][var17][1];
            int var20 = -n[9 - var16][var17][2];
            int var12 = n[9 - var16][var17][3];
            if (f.a((byte)1, var2.a(var20, var12, var3))) {
               int var13 = this.a(var18, var19, 1);
               this.a(var1, var13, var14);
               break;
            }
         }
      }
   }

   private void b(Graphics var1) {
      for (int var2 = 8; var2 <= 12; var2++) {
         Object var3 = j.ad.elementAt(var2);
         if (var3 instanceof byte[]) {
            byte[] var4 = (byte[])var3;
            if (var4.length == 8 || var4.length == 7) {
               this.a(var1, var4, var2);
            }
         }
      }

      for (int var6 = 4; var6 <= 6; var6++) {
         Object var7 = j.ad.elementAt(var6);
         if (var7 instanceof byte[]) {
            byte[] var5 = (byte[])var7;
            if (var5.length == 8 || var5.length == 7) {
               this.a(var1, var5, var6);
            }
         }
      }

      Object var8 = j.ad.elementAt(1);
      if (var8 instanceof byte[]) {
         byte[] var9 = (byte[])var8;
         if (var9.length == 8 || var9.length == 7) {
            this.a(var1, var9, 1);
         }
      }
   }

   private void a(Graphics var1, int var2, int var3) {
      var1.setClip(var3, 0, 18, this.getHeight());
      if (var2 > 7) {
         int var4 = var2 - 8;
         DirectGraphics var5 = DirectUtils.getDirectGraphics(var1);
         var5.drawImage(Y, var3 - var4 * 18, 0, 20, 8192);
      } else {
         var1.drawImage(Y, var3 - var2 * 18, 0, 20);
      }

      var1.setClip(0, 0, this.getWidth(), this.getHeight());
   }

   private boolean a(String[] var1, int var2) {
      if (var2 <= X && var2 >= 0) {
         return false;
      }

      d = var1;
      if (var2 < 0) {
         X = 10;
      } else {
         X = var2;
      }

      return true;
   }

   private void c() {
      i var1 = this.ax.b();
      byte[][] var2 = this.ax.ae;
      byte var3 = var1.a(0, 1, var2);
      if (f.a((byte)32, var3)) {
         int var4 = this.ax.r();
         W = true;
         String[] var5 = i[var4];
         if (this.a(var5, 1)) {
            as = System.currentTimeMillis();
            ad = true;
         }
      } else {
         W = false;
         if (!this.d() && k.d && this.ax.m == k.f) {
            k.a();
         }
      }
   }

   private int a(int var1, int var2, int var3) {
      if (var1 == 12) {
         return var2;
      } else {
         return var3 == -1 ? 8 + var2 : 7 - var2;
      }
   }

   private void b(Graphics var1, int var2) {
      boolean var5 = false;
      boolean var6 = false;
      boolean var7 = false;
      boolean var8 = false;
      switch (var2) {
         case 0:
            this.e(var1, 1, 1);
            break;
         case 1:
            this.e(var1, 6, 1);
            break;
         case 2:
            this.e(var1, 7, 1);
            break;
         case 3:
            this.e(var1, 2, 1);
            break;
         case 4:
            this.e(var1, 3, 2);
            break;
         case 5:
            this.e(var1, 8, 0);
            break;
         case 6:
            int var9 = Math.min(k.f, 3) - 1;
            this.a(var1, var9);
      }

      var1.setClip(0, 0, this.getWidth(), this.getHeight());
   }

   private void a(Graphics var1, int var2) {
      byte var3 = 15;
      byte var4 = 32;
      this.a(var1, q[28], o[var2][0], 1, var3, var4);
      int var5 = q[28].a();
      this.a(var1, q[28], o[var2][0], 1, var3 + var5, var4, 8192);
      this.a(var1, q[29], o[var2][1], 3, var3 + 45, var4 + -22);
   }

   private void g(Graphics var1) {
      A = false;

      for (int var2 = 8; var2 <= 12; var2++) {
         Object var3 = j.ad.elementAt(var2);
         if (var3 instanceof byte[]) {
            byte[] var4 = (byte[])var3;
            if (var4.length == 28 && var4[6] != 0) {
               A = true;
               this.c(var1, var4[2], var2);
            }
         } else if (var3 instanceof String) {
            String var7 = (String)var3;
            if (var7.equals("W")) {
               this.b(var1, 32, var2);
            }
         }
      }

      for (int var6 = 4; var6 <= 6; var6++) {
         Object var8 = j.ad.elementAt(var6);
         if (var8 instanceof byte[]) {
            byte[] var5 = (byte[])var8;
            if (var5.length == 28 && var5[6] != 0) {
               A = true;
               this.c(var1, var5[2], var6);
            }
         } else if (var8 instanceof String) {
            String var10 = (String)var8;
            if (var10.equals("W")) {
               this.d(var1, 31, var6);
            }
         }
      }

      Object var9 = j.ad.elementAt(1);
      if (var9 instanceof byte[]) {
         byte[] var11 = (byte[])var9;
         if (var11.length == 28 && var11[6] != 0) {
            A = true;
            this.c(var1, var11[2], 1);
         }
      } else if (var9 instanceof String) {
      }
   }

   private void a(Graphics var1, byte[] var2, int var3) {
      if (var3 == 1) {
         this.a(var1, var2, false);
      } else if (var3 >= 4 && var3 <= 6) {
         this.b(var1, var2, false, var3);
      } else if (var3 >= 8 && var3 <= 12) {
         this.a(var1, var2, false, var3);
      }
   }

   private static boolean a(byte[] var0) {
      return var0.length != 7 ? false : (var0[6] & 4) != 0;
   }

   private void a(Graphics var1, byte[] var2, boolean var3) {
      if (a(var2)) {
         byte var4 = 45;
         byte var5 = 65;
         this.a(var1, u[0], var4, var5);
      } else {
         byte var7 = 60;
         int var8 = 94;
         byte var6 = 0;
         if (var2.length == 8) {
            this.a(var1, ab[var6], var7, var8);
         } else if (var2.length == 7) {
            var8 += 14;
            this.a(var1, ar[var6], var7, var8);
         }
      }
   }

   private void b(Graphics var1, byte[] var2, boolean var3, int var4) {
      byte var5 = 1;
      short var6 = 0;
      int var7 = 0;
      boolean var8 = a(var2);
      switch (var4) {
         case 4:
            var6 = 14;
            var7 = 80;
            if (var8) {
               var6 = 14;
               var7 = 55;
            }
            break;
         case 5:
            var6 = 68;
            var7 = 80;
            if (var8) {
               var6 = 73;
               var7 = 55;
            } else if (var2.length == 7) {
               var6 = 73;
               var7 = 80;
            }
            break;
         case 6:
            var6 = 122;
            var7 = 80;
            if (var8) {
               var6 = 125;
               var7 = 55;
            } else if (var2.length == 7) {
               var6 = 132;
               var7 = 80;
            }
      }

      if (var8) {
         var7 += 13;
         this.a(var1, u[var5], var6, var7);
      } else if (var2.length == 8) {
         var7 += 17;
         this.a(var1, ab[var5], var6, var7);
      } else if (var2.length == 7) {
         var7 += 20;
         this.a(var1, ar[var5], var6, var7);
      }
   }

   private void a(Graphics var1, byte[] var2, boolean var3, int var4) {
      byte var5 = 2;
      short var6 = 0;
      int var7 = 0;
      boolean var8 = a(var2);
      switch (var4) {
         case 8:
            var6 = 10;
            var7 = 59;
            if (var8) {
               var6 = 10;
               var7 = 52;
            }
            break;
         case 9:
            var6 = 44;
            var7 = 59;
            if (var8) {
               var6 = 44;
               var7 = 52;
            }
            break;
         case 10:
            var6 = 79;
            var7 = 59;
            if (var8) {
               var6 = 79;
               var7 = 52;
            }
            break;
         case 11:
            var6 = 112;
            var7 = 59;
            if (var8) {
               var6 = 112;
               var7 = 52;
            }
            break;
         case 12:
            var6 = 146;
            var7 = 59;
            if (var8) {
               var6 = 146;
               var7 = 52;
            }
      }

      if (var8) {
         var7 += 20;
         this.a(var1, u[var5], var6, var7);
      } else if (var2.length == 8) {
         var7 += 28;
         this.a(var1, ab[var5], var6, var7);
      } else if (var2.length == 7) {
         var7 += 28;
         this.a(var1, ar[var5], var6, var7);
      }
   }

   private int c(int var1) {
      if (var1 >= 1 && var1 <= 5) {
         return 5;
      } else if (var1 >= 6 && var1 <= 10) {
         return 12;
      } else if (var1 >= 11 && var1 <= 25) {
         return 19;
      } else if (var1 >= 26 && var1 <= 40) {
         return 26;
      } else {
         return var1 == 41 ? 31 : -1;
      }
   }

   private int a(int var1) {
      if (var1 >= 1 && var1 <= 5) {
         return 6;
      } else if (var1 >= 6 && var1 <= 10) {
         return 13;
      } else if (var1 >= 11 && var1 <= 25) {
         return 20;
      } else if (var1 >= 26 && var1 <= 40) {
         return 27;
      } else {
         return var1 == 41 ? 32 : -1;
      }
   }

   private void c(Graphics var1, int var2, int var3) {
      if (var3 == 1) {
         this.c(var1, var2);
      } else if (var3 >= 4 && var3 <= 6) {
         this.d(var1, this.c(var2), var3);
      } else if (var3 >= 8 && var3 <= 12) {
         this.b(var1, this.a(var2), var3);
      }

      var1.setClip(0, 0, this.getWidth(), this.getHeight());
   }

   private int b(int var1) {
      if (var1 >= 1 && var1 <= 5) {
         return 0;
      } else if (var1 >= 6 && var1 <= 10) {
         return 1;
      } else if (var1 >= 11 && var1 <= 25) {
         return 2;
      } else {
         return var1 >= 26 && var1 <= 40 ? 3 : -1;
      }
   }

   private void c(Graphics var1, int var2) {
      this.e(var1, var2, -1);
   }

   private void e(Graphics var1, int var2, int var3) {
      if (var2 == 41) {
         this.a(var1, 2);
      } else {
         int var4 = this.b(var2);
         if (var4 >= 0) {
            byte var6 = ae[var4][2];
            byte var7 = ae[var4][3];
            byte var8 = ae[var4][4];
            byte var9 = ae[var4][5];
            int var10 = var6 + ae[var4][6];
            int var11 = var7 + ae[var4][7];
            byte var12 = ae[var4][8];
            byte var13 = ae[var4][9];
            boolean var5;
            if (var12 >= 0) {
               var5 = true;
            } else {
               var5 = false;
            }

            byte var14 = a[var2 - 1][0];
            int var15 = a[var2 - 1][1];
            if (var3 >= 0) {
               var15 = var3;
            }

            boolean var16 = J[var2 - 1][0];
            boolean var17 = J[var2 - 1][1];
            boolean var18 = J[var2 - 1][2];
            boolean var19 = J[var2 - 1][3];
            this.a(var1, q[var8], var14, var9, var6, var7);
            if (var5) {
               this.a(var1, q[var12], var15, var13, var10, var11);
            }

            if (var16) {
               int var20 = var6 + ae[var4][10];
               int var21 = var7 + ae[var4][11];
               byte var22 = ae[var4][12];
               this.a(var1, q[var22], 0, 1, var20, var21);
            }

            if (var17) {
               int var23 = var6 + ae[var4][13];
               int var26 = var7 + ae[var4][14];
               byte var29 = ae[var4][15];
               this.a(var1, q[var29], 0, 1, var23, var26);
            }

            if (var18) {
               int var24 = var6 + ae[var4][16];
               int var27 = var7 + ae[var4][17];
               byte var30 = ae[var4][18];
               this.a(var1, q[var30], 0, 1, var24, var27);
            }

            if (var19) {
               int var25 = var6 + ae[var4][19];
               int var28 = var7 + ae[var4][20];
               byte var31 = ae[var4][21];
               this.a(var1, q[var31], 0, 1, var25, var28);
            }
         }
      }
   }

   private void d(Graphics var1, int var2, int var3) {
      this.a(var1, var2, var3, 0, 1);
   }

   private void a(Graphics var1, int var2, int var3, int var4, int var5) {
      byte var6 = 0;
      byte var7 = 0;
      switch (var3) {
         case 4:
            var6 = 10;
            var7 = 38;
            break;
         case 5:
            var6 = 62;
            var7 = 38;
            break;
         case 6:
            var6 = 112;
            var7 = 38;
      }

      this.a(var1, q[var2], var4, var5, var6, var7);
   }

   private void b(Graphics var1, int var2, int var3) {
      short var4 = 0;
      byte var5 = 0;
      switch (var3) {
         case 8:
            var4 = 10;
            var5 = 44;
            break;
         case 9:
            var4 = 44;
            var5 = 44;
            break;
         case 10:
            var4 = 79;
            var5 = 44;
            break;
         case 11:
            var4 = 112;
            var5 = 44;
            break;
         case 12:
            var4 = 146;
            var5 = 44;
      }

      DirectGraphics var6 = DirectUtils.getDirectGraphics(var1);
      var6.drawPixels(q[var2].g, true, 0, q[var2].h, var4, var5, q[var2].e, q[var2].a, 0, 4444);
   }

   private void a(Graphics var1) {
      var1.setColor(16776960);
      var1.fillRect(5, 130, 40, 7);
      var1.fillRect(5, 138, 40, 7);
      var1.fillRect(5, 146, 40, 7);
      var1.setColor(16711680);
      int var2 = this.ax.n(2) * 38 / this.ax.U[3];
      var1.fillRect(6, 131, var2, 5);
      var1.setColor(65280);
      var2 = this.ax.n(4) * 38 / this.ax.U[5];
      var1.fillRect(6, 139, var2, 5);
      var1.setColor(255);
      var2 = this.ax.n(6) * 38 / this.ax.U[7];
      if (var2 > 40) {
         var2 = 40;
      }

      var1.fillRect(6, 147, var2, 5);
   }

   private void l(Graphics var1) {
      if (ad) {
         var1.setColor(13080935);
         var1.fillRoundRect(96, 118, 75, 35, 5, 5);
         var1.setFont(aj);
         var1.setColor(0);
         var1.drawString(d[0], 100, 122, 20);
         if (d.length > 1) {
            var1.drawString(d[1], 100, 134, 20);
         }
      }
   }

   private void d(Graphics var1) {
      var1.setFont(aj);
      var1.setClip(0, 0, this.getWidth(), this.getHeight());
      var1.setColor(0);
      var1.fillRect(0, 156, this.getWidth(), 52);
      var1.setColor(13080935);
      var1.fillRoundRect(2, 158, this.getWidth() - 4, 48, 5, 5);
      var1.setColor(0);
      int var2 = this.j();
      aq = var2;
      if (var2 == 0) {
         var1.drawImage(y[1], 14, 174, 20);
         var1.drawImage(y[2], 62, 174, 20);
         var1.drawImage(y[3], 104, 174, 20);
         var1.drawImage(y[5], 144, 174, 20);
         var1.drawChar(e[1], 5, 180, 20);
         var1.drawChar(e[2], 53, 180, 20);
         var1.drawChar(e[3], 96, 180, 20);
         var1.drawChar(e[5], 135, 180, 20);
      } else if (var2 == 1) {
         var1.drawImage(y[0], 14, 174, 20);
         var1.drawImage(y[1], 62, 174, 20);
         var1.drawImage(y[2], 104, 174, 20);
         var1.drawImage(y[3], 144, 174, 20);
         var1.drawChar(e[0], 5, 180, 20);
         var1.drawChar(e[1], 53, 180, 20);
         var1.drawChar(e[2], 96, 180, 20);
         var1.drawChar(e[3], 135, 180, 20);
      } else if (var2 == 2) {
         var1.drawImage(y[1], 14, 174, 20);
         var1.drawImage(y[2], 62, 174, 20);
         var1.drawImage(y[3], 104, 174, 20);
         var1.drawImage(y[4], 144, 174, 20);
         var1.drawChar(e[1], 5, 180, 20);
         var1.drawChar(e[2], 53, 180, 20);
         var1.drawChar(e[3], 96, 180, 20);
         var1.drawChar(e[4], 135, 180, 20);
      }
   }

   private int j() {
      if (aa) {
         return 1;
      } else if (m || R) {
         return 2;
      } else {
         return W && !this.d() ? 2 : 0;
      }
   }

   private void a(Graphics var1, g var2, int var3, int var4, int var5, int var6) {
      this.a(var1, var2, var3, var4, var5, var6, 0);
   }

   private void a(Graphics var1, g var2, int var3, int var4, int var5, int var6, int var7) {
      int var8 = var2.a() / var4;
      int var9 = var2.b();
      var1.setClip(var5, var6, var8, var9);
      DirectGraphics var10 = DirectUtils.getDirectGraphics(var1);
      var10.drawPixels(var2.g, true, 0, var2.h, var5 - var3 * var8, var6, var2.e, var2.a, var7, 4444);
   }

   private void k(Graphics var1) {
      var1.setFont(aj);
      var1.setColor(16777215);
      var1.drawChar(O[this.ax.ak], 16, 10, 20);
      var1.setColor(0);
      var1.fillRect(10, 20, 23, 23);
      this.a(var1, 10, 20, 7, 3, 1, l);
   }

   private void h(Graphics var1) {
      var1.setFont(K);
      var1.setColor(16777215);
      var1.drawChar(O[this.ax.ak], 58, 10, 20);
      byte var2 = 89;
      var1.fillRect(15, 25, var2, var2);
      this.a(var1, 15, 25, 17, 5, 2, F);
   }

   private void a(Graphics var1, int var2, int var3, int var4, int var5, int var6, byte[][] var7) {
      int var8 = var4 / 2;

      for (int var9 = 0; var9 < var4; var9++) {
         int var10 = var3 + var6 + var9 * var5;

         for (int var11 = 0; var11 < var4; var11++) {
            int var12 = var2 + var6 + var11 * var5;
            if (var7[var11][var9] == 1) {
               var1.setColor(0);
               var1.fillRect(var12, var10, var5, var5);
            } else if (var7[var11][var9] == 0) {
               var1.setColor(16777215);
               var1.fillRect(var12, var10, var5, var5);
            } else if ((var7[var11][var9] & 2) != 0) {
               var1.setColor(16711680);
               var1.fillRect(var12, var10, var5, var5);
            } else if ((var7[var11][var9] & 4) != 0) {
               var1.setColor(255);
               var1.fillRect(var12, var10, var5, var5);
            } else if ((var7[var11][var9] & 8) != 0) {
               var1.setColor(13369599);
               var1.fillRect(var12, var10, var5, var5);
            }

            if (var9 == var8 && var11 == var8) {
               var1.setColor(65280);
               var1.fillRect(var12, var10, var5, var5);
            }
         }
      }
   }

   void q() {
      byte var1 = this.ax.l;
      byte var2 = this.ax.k;
      byte var3 = this.ax.ak;
      this.ax.b().c(var1, var2, var3, l);
   }

   void p() {
      byte var1 = this.ax.l;
      byte var2 = this.ax.k;
      byte var3 = this.ax.ak;
      this.ax.b().a(var1, var2, var3, F);
   }

   public void keyPressed(int var1) {
      this.T = this.ac;
      if (var1 == 49) {
         if (aq == 1) {
            av = true;
         }
      } else if (var1 == 50) {
         this.p = 1;
      } else if (var1 == 51) {
         ap = true;
      } else if (var1 == 52) {
         this.aw = true;
         this.p = 4;
      } else if (var1 == 53) {
         U = true;
      } else if (var1 == 54) {
         this.aw = true;
         this.p = 3;
      } else if (var1 == 55) {
         Z = true;
      } else if (var1 == 56) {
         this.p = 2;
      } else if (var1 == 57) {
         if (aq == 2) {
            ay = true;
         }
      } else if (var1 == 48) {
         if (aq == 0) {
            I = true;
         }
      } else if (var1 == 42) {
         f++;
         if (f > 2) {
            f = 1;
         }
      } else {
         this.aw = false;
         this.ac = this.getGameAction(var1);
         switch (this.ac) {
            case 1:
               this.p = 1;
               break;
            case 2:
               this.p = 4;
            case 3:
            case 4:
            default:
               break;
            case 5:
               this.p = 3;
               break;
            case 6:
               this.p = 2;
         }
      }
   }

   public void keyReleased(int var1) {
      int var2 = this.getGameAction(var1);
      this.T = this.ac;
      this.ac = 0;
   }

   void g() {
      if (this.au != null) {
         this.z = true;
         if (this.au.isAlive()) {
            System.out.println("Killing game thread");
            this.aB = true;

            try {
               this.au.join();
            } catch (Exception var2) {
            }

            System.out.println("Done killing game thread");
         }

         this.au = null;
         System.gc();
      }
   }

   void i() {
      try {
         this.g();
         this.au = new Thread(this);
         this.au.start();
         ESGame.c("after starting game thread");
      } catch (Throwable var2) {
         System.out.println(" start error:");
         var2.printStackTrace();
         this.repaint();
         this.serviceRepaints();
      }
   }

   public void run() {
      long var1 = System.currentTimeMillis();
      long var5 = 0L;
      long var7 = 0L;

      try {
         while (this.z) {
            boolean var9 = false;
            if (this.ak) {
               long var20 = System.currentTimeMillis();

               try {
                  Thread.sleep(250L);
               } catch (Exception var16) {
               }

               if (this.aB) {
                  this.aB = false;
                  return;
               }
            } else {
               boolean var10 = true;
               at = false;
               System.gc();
               if (this.c == 1) {
                  var10 = false;
                  if (var1 - this.aC > 2500L) {
                     if (this.o()) {
                        this.c = 0;
                        this.ax.b().a(this.ax);
                        this.aC = 0L;
                        this.v = true;
                        this.ax.e(false);
                        var10 = true;
                        if (this.a(N, 1)) {
                           as = var1;
                           ad = true;
                        }
                     } else {
                        this.c = 2;
                     }
                  }
               } else if (this.c == 2) {
                  var10 = false;
                  if (var1 - this.aC > 5000L) {
                     this.c = 0;
                     this.aC = 0L;
                     this.v = true;
                     this.ax.e(true);
                     if (this.a(b, 1)) {
                        as = var1;
                        ad = true;
                     }

                     var10 = true;
                  }
               } else if (this.aD != 1) {
                  if (this.aD == 2) {
                     this.aD = 3;
                     ad = false;
                     X = 0;
                  }

                  var10 = false;
                  if (var1 - this.s > 5000L) {
                     System.out.println("Restart after dead");
                     this.ax.a(this.ax.U);

                     for (int var11 = this.ax.p - 1; var11 >= 0; var11--) {
                        if (!this.ax.C(var11)) {
                           this.ax.y(var11);
                        }
                     }

                     this.ax.c(this.ax.ar, true);
                     this.s = 0L;
                     this.aD = 1;
                     k.l = true;
                     this.v = true;
                     E = true;
                     var10 = true;
                     if (E) {
                        String[] var12 = null;
                        if (this.ax.u) {
                           var12 = r;
                        } else if (this.ax.O) {
                           var12 = ag;
                        } else {
                           var12 = this.ax.b().a();
                        }

                        if (this.a(var12, 1)) {
                           as = System.currentTimeMillis();
                           ad = true;
                        }

                        E = false;
                     }
                  }
               }

               if (var10) {
                  if (k.a(this.ax.W)) {
                     k.c();
                  }

                  if (this.d() && k.f > this.ax.m) {
                     String var21 = k.a(this.ax, 6, -1, -1);
                     this.H.aV = this.H.e(var21);
                     var9 = true;
                  }

                  this.b(var1);
                  this.e(var1);
                  this.a(var1, var5);
                  if (this.ax.o()) {
                     this.e();
                     this.H.ag = this.H.j(1);
                     this.H.a(this.H.ag);
                     al = false;
                  }

                  this.v = false;
               }

               this.a(false);
               if (al) {
                  this.repaint();
                  this.serviceRepaints();
               }

               long var22 = System.currentTimeMillis() - var1;
               ai = var22;

               try {
                  if (var22 < 250L) {
                     long var13 = 250L - var22;
                     Thread.sleep(250L - var22);
                  }
               } catch (Exception var17) {
               }

               if (this.aB) {
                  this.aB = false;
                  return;
               }

               long var3 = var1;
               var1 = System.currentTimeMillis();
               var5 = var1 - var3;
               this.c(var5);
               var7 += var5;
               if (var7 > 1000L) {
                  var7 -= 1000L;
                  this.l();
               }

               if (var1 - as > 3000L) {
                  ad = false;
                  X = 0;
               }

               if (var9) {
                  this.e();
                  this.H.a(this.H.aV);
               }
            }
         }
      } catch (OutOfMemoryError var18) {
         this.H.B();
      } catch (Throwable var19) {
         System.out.println("ERROR: An error was thrown in GameCanvas run method!");
         System.out.println(var19);
         var19.printStackTrace();
         az = true;
         G = String.valueOf(var19);
         this.repaint();
         this.serviceRepaints();

         try {
            Thread.sleep(10000L);
         } catch (Throwable var15) {
         }

         this.H.b();
      }
   }

   private void a(boolean var1) {
      this.ax.d(var1);
      this.q();
      if (f == 2) {
         this.p();
      }
   }

   private void b(long var1) {
      Hashtable var3 = ESGame.G[this.ax.j - 1];
      if (var3 != null) {
         Enumeration var4 = var3.elements();

         while (var4.hasMoreElements()) {
            byte[] var5 = (byte[])var4.nextElement();
            d var6 = d.a(var5);
            if (var6.b(this.ax)) {
               if (var6.f == 0) {
                  var6.k = var1;
                  var6.f = 1;
               } else if (var6.f == 1 && var1 - var6.k > 800L) {
                  var6.a(this.ax, var1);
                  if (this.a(ah, 2)) {
                     as = var1;
                     ad = true;
                  }
               } else if (var1 - var6.k > 800L) {
                  var6.a(this.ax, var1);
               }

               var6.d();
            } else {
               var6.a(this.ax);
               var6.d();
            }
         }
      }
   }

   private void e(long var1) {
      if (I) {
         if (A) {
            if (this.a(C, 1)) {
               as = var1;
               ad = true;
            }

            I = false;
         } else {
            this.a(var1);
         }
      } else if (ay) {
         this.f(var1);
      } else if (ap) {
         this.h(var1);
      } else if (U) {
         this.g(var1);
      } else if (av) {
         this.d(var1);
      } else if (Z) {
         this.f();
      } else if ((this.p != 0 || this.v) && !this.v) {
         this.n();
      }

      this.h();
      this.c();
      this.a();
      this.m();
   }

   private void d(long var1) {
      if (var1 - this.B >= 500L && j != null) {
         at = true;
         this.ax.a(j);
         System.out.println("monster health is " + j.g);
         this.B = var1;
         S = true;
      }

      av = false;
   }

   private void m() {
      if (j != null && j.g <= 0) {
         if (j.l == 41) {
            j.a(true);
         } else {
            j.a(false);
         }

         ESGame.a(this.ax.j, j.a);
         if (this.ax.k(4)) {
            this.ax.U[2] = (short)(this.ax.U[2] + 3 * this.ax.U[3] / 10);
            this.ax.U[2] = (short)Math.min(this.ax.U[2], this.ax.U[3]);
         }

         if (this.a(P, 1)) {
            as = System.currentTimeMillis();
            ad = true;
         }

         j = null;
         aa = false;
      }
   }

   private void a() {
      j = this.ax.n();
      if (j != null) {
         aa = true;
         byte[] var1 = j.f();
         j = d.a(k, var1);
      } else {
         aa = false;
      }

      if (aa) {
         String var4 = j.a();
         String[] var2 = new String[2];
         int var3 = var4.indexOf(32);
         if (var3 < 0) {
            var2[0] = var4;
            var2[1] = "";
         } else {
            var2[0] = var4.substring(0, var3);
            var2[1] = var4.substring(var3 + 1);
         }

         if (this.a(var2, 1)) {
            as = System.currentTimeMillis();
            ad = true;
         }
      }
   }

   private void h() {
      byte[] var1 = this.ax.h();
      if (var1 != null) {
         m = true;
      } else {
         m = false;
      }

      if (m && this.a(x, 1)) {
         as = System.currentTimeMillis();
         ad = true;
      }
   }

   private void n() {
      if (this.p != 0) {
         byte var1 = this.ax.p;
         at = true;
         this.ax.a(this.p, this.aw);
         if (j.g) {
            this.H.aP = this.H.F();
            this.H.a(this.H.aP);
            al = false;
         } else {
            if (this.ax.i || this.ax.u || this.ax.O) {
               E = true;
               if (E) {
                  String[] var2 = null;
                  if (this.ax.u) {
                     var2 = r;
                  } else if (this.ax.O) {
                     var2 = ag;
                  } else {
                     var2 = this.ax.b().a();
                  }

                  if (this.a(var2, 1)) {
                     as = System.currentTimeMillis();
                     ad = true;
                  }

                  E = false;
               }
            }

            if (this.aw) {
               this.aw = false;
            }
         }

         this.p = 0;
         int var4 = this.ax.p - var1;
         if (var4 == 1) {
            if (this.a(this.k(), -1)) {
               as = System.currentTimeMillis();
               ad = true;
            }
         } else if (var4 > 1 && this.a(an, -1)) {
            as = System.currentTimeMillis();
            ad = true;
         }
      }
   }

   private void h(long var1) {
      if (ap) {
         byte var3 = this.ax.b;
         if (!b.a(var3)) {
            System.out.println("Invalid spell id,= " + var3);
            ap = false;
            return;
         }

         if (b.c(var3).e > this.ax.n(4)) {
            if (this.a(t, 3)) {
               as = var1;
               ad = true;
            }
         } else if (var1 - this.V >= 500L && b.a(var3)) {
            at = true;
            if (b.b(var3)) {
               if (!aa) {
                  if (this.a(g, 1)) {
                     as = var1;
                     ad = true;
                  }
               } else {
                  this.ax.b(var3, j);
                  System.out.println("monster health is " + j.g);
                  ao = true;
               }
            } else {
               this.ax.p(var3);
               am = true;
            }

            this.V = var1;
         }

         ap = false;
      }
   }

   private void g(long var1) {
      if (U) {
         int var3 = this.ax.l();
         if (var3 == 0) {
            if (this.a(Q, -1)) {
               as = var1;
               ad = true;
            }
         } else {
            this.ax.b = (byte)var3;
            String var4 = b.c(var3).c;
            String[] var5 = f.c(var4);
            String[] var6 = null;
            if (var5.length == 1) {
               var6 = new String[]{var4, ""};
            } else if (var5.length >= 3) {
               var6 = new String[]{var5[0] + " " + var5[1], var5[2]};
            } else {
               var6 = var5;
            }

            if (this.a(var6, -1)) {
               as = var1;
               ad = true;
            }
         }

         U = false;
      }
   }

   private void f(long var1) {
      int var3 = this.ax.r();
      System.out.println("NPC In front is " + var3);
      if (var3 >= 0) {
         this.d(var3);
         System.out.println("done interacting with NPC, must paint as well");
      } else if (m) {
         byte[] var4 = this.ax.h();
         int var5 = this.ax.b(var4);
         if (var5 == -1) {
            if (this.a(D, 4)) {
               as = var1;
               ad = true;
            }
         } else if (var5 == 0) {
            if (this.a(aA, -1)) {
               as = var1;
               ad = true;
            }
         } else if (this.a(this.k(), -1)) {
            as = var1;
            ad = true;
         }
      }

      ay = false;
   }

   private String[] k() {
      int var1 = this.ax.p - 1;
      int var2 = Math.abs(this.ax.H[var1]);
      String[] var3 = f.c(a.d(var2));
      String[] var4 = new String[]{"", ""};
      if (var3.length >= 3) {
         var4[0] = var3[0] + " " + var3[1];
         var4[1] = var3[2];
      } else {
         for (int var5 = 0; var5 < var3.length; var5++) {
            var4[var5] = var3[var5];
         }
      }

      return var4;
   }

   private void f() {
      this.H.a(this.H.t);
      al = false;
      Z = false;
   }

   private void a(long var1) {
      this.c = 1;
      if (this.ax.I) {
         this.c = 2;
      }

      if (this.ax.j == 1) {
         this.c = 2;
      }

      this.aC = var1;
      I = false;
   }

   void d(int var1) {
      String var2 = k.a(this.ax, var1, 1, 0);
      System.out.println("Just after NPC interaction in game canvas!");
      if (var2 != null) {
         this.H.aq.a(k.s[var1]);
         this.H.aq.e(var2);
         this.H.aq.c = this.H.R[var1];
         this.H.aq.N = var1;
         h var3 = (h)this.H.aq.c;
         String var4 = var3.M;
         String var5 = var3.t();
         short var6 = 0;
         if (k.b(var1)) {
            var6 = k.p[var1];
         } else if (var1 == 4) {
            var6 = k.a;
         } else if (var1 == 5) {
            var6 = k.g;
         }

         var5 = f.a(var4, "<TAG>", var6);
         var3.e(var5);
         this.H.a(this.H.aq);
         al = false;
      } else if (var1 == 4) {
         System.out.println("BENECA has nothing more to say!");
         h var7 = this.H.R[4];
         String var9 = var7.M;
         String var12 = var7.t();
         short var16 = 0;
         var16 = k.a;
         var12 = f.a(var9, "<TAG>", var16);
         var7.e(var12);
         this.H.a(var7);
         al = false;
      } else if (var1 == 5) {
         System.out.println("HELGA has nothing more to say!");
         h var8 = this.H.R[5];
         String var10 = var8.M;
         String var14 = var8.t();
         short var18 = 0;
         var18 = k.g;
         var14 = f.a(var10, "<TAG>", var18);
         var8.e(var14);
         this.H.a(var8);
         al = false;
      }
   }

   private void a(long var1, long var3) {
      int var5 = this.ax.n(2);
      if (var5 <= 0) {
         aa = false;
         this.aD = 2;
         this.s = var1;
      }

      if (!at) {
         this.ax.a(var3);
      }
   }

   public void e() {
      this.ak = true;
   }

   public void b() {
      this.ak = false;
   }

   protected void showNotify() {
      this.ax.w();
      this.q();
      if (f == 2) {
         this.p();
      }

      this.c();
      this.a();
      al = true;
      this.b();
      if (E) {
         String[] var1 = null;
         if (this.ax.u) {
            var1 = r;
         } else if (this.ax.O) {
            var1 = ag;
         } else {
            var1 = this.ax.b().a();
         }

         if (this.a(var1, 1)) {
            as = System.currentTimeMillis();
            ad = true;
         }

         E = false;
      }
   }

   private boolean o() {
      int var1 = f.a(10);
      return var1 == 1;
   }

   private void c(long var1) {
      if (this.ax.k(4)) {
         this.ax.ah = (short)(this.ax.ah - var1);
         if (this.ax.ah < 0) {
            this.ax.ah = 0;
            byte var3 = 3;
            this.ax.A = (byte)f.c((int)var3, (int)this.ax.A);
         }
      }

      if (this.ax.k(5)) {
         this.ax.F = (short)(this.ax.F - var1);
         if (this.ax.F < 0) {
            this.ax.F = 0;
            byte var4 = 4;
            this.ax.A = (byte)f.c((int)var4, (int)this.ax.A);
         }
      }

      if (this.ax.k(7) && A) {
         this.ax.ap = (short)(this.ax.ap - var1);
         if (this.ax.ap < 0) {
            this.ax.ap = 0;
            byte var5 = 6;
            this.ax.A = (byte)f.c((int)var5, (int)this.ax.A);
         }
      }
   }

   private void l() {
      if (this.ax.k(4)) {
         int var1 = 2 * this.ax.U[3] / 100;
         var1 = Math.max(var1, 0);
         this.ax.U[2] = (short)(this.ax.U[2] - var1);
      }

      if (this.ax.k(5)) {
         int var7 = this.ax.U[5] / 10;
         this.ax.U[4] = (short)(this.ax.U[4] + var7);
         if (this.ax.U[4] >= this.ax.U[5]) {
            this.ax.U[4] = 0;
            int var2 = this.ax.U[5] / 10;
            this.ax.U[2] = (short)(this.ax.U[2] - var2);
         }
      }

      for (int var8 = 0; var8 < 25; var8++) {
         if (this.ax.G[var8] > 0) {
            this.ax.G[var8]--;
            if (this.ax.G[var8] <= 0) {
               this.ax.G[var8] = 0;
               if (var8 == 5) {
                  System.out.println("Removing daedric weapon!");
                  int var9 = this.ax.o(109);
                  System.out.println("Removing daedric weapon!: index is " + var9);
                  this.ax.y(var9);
               }
            }
         }
      }

      Hashtable var10 = ESGame.G[this.ax.j - 1];
      if (var10 != null) {
         Enumeration var3 = var10.elements();

         while (var3.hasMoreElements()) {
            byte[] var4 = (byte[])var3.nextElement();
            d var5 = d.a(var4);
            if (var5.c[6] != 0) {
               var5.c[7]--;
               if (var5.c[7] < 0) {
                  var5.c[7] = 0;
                  var5.c[6] = 0;
               }
            }
         }
      }
   }

   private boolean d() {
      if (k.a(this.ax)) {
         return true;
      } else if (j == null) {
         return false;
      } else if (this.ax.j == 37 && j.l == 41) {
         int var1 = Math.abs(this.ax.l - j.o);
         int var2 = Math.abs(this.ax.k - j.m);
         return var1 + var2 == 1;
      } else {
         return false;
      }
   }

   private void a(Graphics var1, g var2, int var3, int var4) {
      DirectGraphics var5 = DirectUtils.getDirectGraphics(var1);
      var5.drawPixels(var2.g, true, 0, var2.h, var3, var4, var2.e, var2.a, 0, 4444);
   }
}
