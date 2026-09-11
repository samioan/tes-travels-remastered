import java.io.DataInputStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Random;
import java.util.Vector;

public class i {
   static String[][] c;
   static final int[][] l = new int[][]{
      {1, 2, 1, 3},
      {6, 7, 8, 6},
      {1, 2, 3, 1},
      {6, 7, 8, 7},
      {3, 4, 11, 12},
      {8, 9, 11, 12},
      {3, 4, 12, 13},
      {8, 9, 12, 13},
      {4, 5, 12, 13},
      {9, 10, 12, 13},
      {4, 5, 13, 14},
      {9, 10, 13, 14},
      {12, 13, 4, 5},
      {12, 13, 9, 10},
      {13, 14, 15, 16},
      {14, 15, 16, 17},
      {15, 16, 17, 18},
      {16, 17, 18, 21},
      {17, 18, 19, 26},
      {18, 19, 20, 21},
      {19, 20, 26, 27},
      {21, 22, 26, 27},
      {21, 22, 27, 28},
      {22, 23, 27, 28},
      {22, 23, 28, 29},
      {23, 24, 28, 29},
      {23, 24, 29, 30},
      {24, 25, 29, 30},
      {26, 27, 28, 31},
      {27, 28, 31, 32},
      {28, 29, 32, 33},
      {29, 30, 33, 34},
      {31, 32, 34, 35},
      {32, 33, 36, 37},
      {34, 35, 37, 38},
      {38, 39, 40, 35},
      {38, 39, 40, 35}
   };
   static final byte[] q = new byte[]{
      1, 5, 9, 13, 14, 15, 22, 23, 24, 2, 6, 10, 19, 20, 21, 31, 32, 33, 3, 7, 11, 16, 17, 18, 28, 29, 30, 4, 8, 12, 25, 26, 27, 34, 35, 36
   };
   static final int[] n = new int[]{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 3, 1, 3, 3, 1, 3, 3, 3, 3, 5, 3, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5};
   byte o;
   byte a;
   short g;
   short y;
   byte[][] w;
   short[] f;
   short[] d;
   short p;
   short i;
   short r;
   short b;
   short s;
   short v;
   boolean k = false;
   Vector m;
   Vector j;
   Vector t;
   Random e;
   byte[] x;
   int[] u = new int[2];
   boolean h = false;

   public i() {
   }

   public i(byte var1, byte[] var2) {
      this();
      this.o = var1;
      this.i();
      this.g = 35;
      this.y = 35;
      this.f = new short[2];
      this.d = new short[6];
      this.x = var2;
      this.s = this.x[4];
      this.v = this.x[5];
   }

   public i(byte var1, byte[] var2, int var3, int var4, byte[][] var5) {
      this();
      this.o = var1;
      this.i();
      this.g = (short)var3;
      this.y = (short)var4;
      this.w = var5;
      this.f = new short[2];
      this.d = new short[6];
      this.x = var2;
      this.s = this.x[4];
      this.v = this.x[5];
      int var6 = 6;
      if (k.d) {
         var6++;
      }

      for (int var7 = 0; var7 < var6; var7++) {
         this.w[k.j[var7]][k.i[var7]] = (byte)(this.w[k.j[var7]][k.i[var7]] | 32);
      }

      this.k = true;
   }

   void i() {
      this.a = 1;
      if (this.o >= 2 && this.o <= 37) {
         this.a = q[this.o - 2];
      }
   }

   void c() {
      int var1 = this.t.size();
      short var2 = (short)(this.o << 8);
      d var3 = null;

      for (int var4 = 0; var4 < var1; var4++) {
         short[] var5 = (short[])this.t.elementAt(var4);
         if (this.o == 37 && var4 == var1 - 1) {
            var3 = d.a(this.e, this, 41);
         } else {
            var3 = d.a(this.e, this, -1);
         }

         this.a(var3, var5);
         var3.d();
      }

      System.gc();
   }

   void a(d var1, short[] var2) {
      var1.o = (byte)var2[4];
      var1.m = (byte)var2[5];
      this.w[var1.o][var1.m] = (byte)(this.w[var1.o][var1.m] | 2);
   }

   void c(int var1) {
      int var2 = this.t.size();
      boolean var3 = false;

      for (int var4 = 0; var4 < var1; var4++) {
         d var6;
         short var10;
         short var11;
         do {
            int var5 = Math.abs(this.e.nextInt() % var2);
            var6 = d.a(this);
            short[] var7 = (short[])this.t.elementAt(var5);
            short var8 = (short)(var7[2] - var7[0] + 1);
            short var9 = (short)(var7[3] - var7[1] + 1);
            var10 = (short)(var7[0] + Math.abs(this.e.nextInt() % var8));
            var11 = (short)(var7[1] + Math.abs(this.e.nextInt() % var9));
         } while (!this.d(var10, var11));

         var6.o = (byte)var10;
         var6.m = (byte)var11;
         this.w[var6.o][var6.m] = f.b((byte)2, this.w[var6.o][var6.m]);
         var6.d();
      }
   }

   void a(j var1) {
      if (this.o != 1) {
         d var2 = d.a(this);
         byte var3 = var1.l;
         byte var4 = var1.k;
         boolean var5 = true;

         for (int var6 = 0; var6 <= 4; var6++) {
            int var7 = var3;
            int var8 = var4;
            if (var6 < 2) {
               var7 += 2 * var6 - 1;
            } else {
               var8 += 2 * var6 - 5;
            }

            if (this.d(var7, var8)) {
               var2.o = (byte)var7;
               var2.m = (byte)var8;
               var2.d();
               this.w[var2.o][var2.m] = f.b((byte)2, this.w[var2.o][var2.m]);
               break;
            }
         }
      }
   }

   static int a(int var0, int var1, boolean var2) {
      if (var2) {
         if (var0 < var1) {
            return -1;
         } else {
            return var0 > var1 ? 1 : 0;
         }
      } else if (var0 > var1) {
         return -1;
      } else {
         return var0 < var1 ? 1 : 0;
      }
   }

   int[] b(int var1) {
      int var2 = this.t.size();
      int[] var3 = new int[var2];
      int[] var4 = new int[var2];

      for (int var5 = 0; var5 < var2; var5++) {
         var3[var5] = var5;
         int var6 = f.a(this.e, 1000);
         var4[var5] = var6;
      }

      for (int var10 = 1; var10 < var2; var10++) {
         int var7 = var4[var10];
         int var8 = var3[var10];

         int var9;
         for (var9 = var10 - 1; var9 >= 0 && a(var4[var9], var7, false) > 0; var9--) {
            var4[var9 + 1] = var4[var9];
            var3[var9 + 1] = var3[var9];
         }

         var4[var9 + 1] = var7;
         var3[var9 + 1] = var8;
      }

      int[] var11 = new int[var1];

      for (int var12 = 0; var12 < var1; var12++) {
         var11[var12] = var3[var12];
      }

      return var11;
   }

   void j() {
      int var1 = 0;
      int[] var2 = this.b(5);
      int var3 = n[this.a - 1];
      boolean var4 = true;

      for (int var5 = 0; var5 < 5; var5++) {
         int var6 = var2[var5];
         short[] var7 = (short[])this.t.elementAt(var6);
         boolean var8 = false;
         if (var4) {
            var1 = a.a(this.e, var3);
            var4 = false;
         } else {
            var1 = a.a(this.e, this.a, 2);
         }

         short var9 = (short)(var7[2] - var7[0] + 1);
         short var10 = (short)(var7[3] - var7[1] + 1);
         short var11 = (short)(var7[0] + Math.abs(this.e.nextInt() % var9));

         short var12;
         for (var12 = (short)(var7[1] + Math.abs(this.e.nextInt() % var10));
            (this.w[var11][var12] & 8) != 0;
            var12 = (short)(var7[1] + Math.abs(this.e.nextInt() % var10))
         ) {
            var11 = (short)(var7[0] + Math.abs(this.e.nextInt() % var9));
         }

         byte[] var13 = new byte[8];
         var13[0] = (byte)var11;
         var13[1] = (byte)var12;
         var13[2] = (byte)(var4 ? 1 : 0);
         byte var14 = (byte)(Math.abs(this.e.nextInt() % 3) << 6);
         var13[3] = (byte)(var14 | this.a);
         byte var15 = (byte)(var1 & 0xFF);
         byte var16 = 0;
         if (var15 == 86) {
            var16 = (byte)(var1 >>> 8 & 0xFF);
         }

         var13[4] = var15;
         var13[7] = var16;
         short var17 = a.a();
         var15 = (byte)(var17 >>> 8 & 0xFF);
         var16 = (byte)(var17 & 0xFF);
         var13[5] = var15;
         var13[6] = var16;
         this.a(var13);
      }
   }

   void a(byte[] var1) {
      byte var2 = var1[0];
      byte var3 = var1[1];
      ESGame.S[this.o - 1].put(f.b((int)var2, (int)var3), var1);
      this.w[var2][var3] = (byte)(this.w[var2][var3] | 16);
   }

   void c(byte[] var1) {
      byte var2 = var1[0];
      byte var3 = var1[1];
      ESGame.au[this.o - 1].addElement(var1);
      this.w[var2][var3] = (byte)(this.w[var2][var3] | 4);
   }

   public void b() {
      long var1 = this.o * 5000;
      this.w = new byte[this.g][this.y];
      short var3 = this.x[4];
      short var4 = this.x[5];
      this.t = new Vector();
      this.a(var1, var3, var4);
   }

   void e() {
      this.c();
      this.j();
   }

   void h() {
      boolean var1 = false;
      Hashtable var2 = ESGame.G[this.o - 1];
      if (var2 != null) {
         Enumeration var3 = var2.elements();

         while (var3.hasMoreElements()) {
            byte[] var4 = (byte[])var3.nextElement();
            d var5 = d.a(var4);
            this.w[var5.o][var5.m] = (byte)(this.w[var5.o][var5.m] | 2);
         }

         System.gc();
      }

      var1 = false;
      var2 = ESGame.S[this.o - 1];
      if (var2 != null) {
         Enumeration var9 = var2.elements();

         while (var9.hasMoreElements()) {
            byte[] var11 = (byte[])var9.nextElement();
            this.w[var11[0]][var11[1]] = (byte)(this.w[var11[0]][var11[1]] | 16);
         }
      }

      var1 = false;
      Enumeration var10 = ESGame.au[this.o - 1].elements();

      while (var10.hasMoreElements()) {
         byte[] var12 = (byte[])var10.nextElement();
         this.w[var12[0]][var12[1]] = (byte)(this.w[var12[0]][var12[1]] | 4);
      }

      if (this.o == 1 && k.d) {
         byte var13 = k.j[6];
         byte var14 = k.i[6];
         this.w[var13][var14] = (byte)(this.w[var13][var14] | 32);
      }
   }

   boolean b(short var1) {
      switch (var1) {
         case 1:
         case 2:
         case 3:
         case 4:
            return true;
         default:
            return false;
      }
   }

   public void a(long var1, short var3, short var4) {
      this.s = var3;
      this.v = var4;
      int var5 = -1;
      this.e = new Random(var1);

      for (int var6 = 0; var6 < 35; var6++) {
         for (int var7 = 0; var7 < 35; var7++) {
            this.w[var6][var7] = 1;
         }
      }

      this.m = new Vector();
      this.j = new Vector();
      this.p = 3;
      this.b = 3;
      this.i = 31;
      this.r = 31;
      if (this.b(var3)) {
         var5 = this.a(var3);
         if (var5 >= 0) {
            this.d(var5);
         }
      }

      if (this.b(var4)) {
         var5 = this.a(var4);
         if (var5 >= 0) {
            this.d(var5);
         }
      }

      boolean var18 = false;
      int var8 = 0;

      while (var8 < 15) {
         this.d = this.f();
         if (this.a(this.d)) {
            var8++;
            int var9 = this.a(this.d[4], this.d[5]);
            this.d(var9);
            if (var8 >= 2 && !var18) {
               short var10 = this.d[4];
               short var11 = this.d[5];
               short var12 = (short)(this.d[2] - this.d[0] + 1);
               short var13 = (short)(this.d[3] - this.d[1] + 1);
               if (var12 >= 3 && var13 >= 3) {
                  short var14 = (short)(this.d[0] + var12 / 2);
                  short var15 = (short)(this.d[1] + var13 / 2);
                  if (var14 != var10 || var15 != var11) {
                     this.w[var14][var15] = (byte)(this.w[var14][var15] | 8);
                     var18 = true;
                  }
               }
            }
         }
      }

      this.d();
   }

   private short[] f() {
      byte var1 = 4;
      int var2 = 2 + Math.abs(this.e.nextInt()) % var1;
      int var3 = 2 + Math.abs(this.e.nextInt()) % var1;
      int var4 = this.i - this.b + 1 - (var2 - 1);
      int var5 = this.r - this.p + 1 - (var3 - 1);
      this.d[0] = (short)(this.b + Math.abs(this.e.nextInt()) % var4);
      this.d[1] = (short)(this.p + Math.abs(this.e.nextInt()) % var5);
      this.d[2] = (short)(this.d[0] + (var2 - 1));
      this.d[3] = (short)(this.d[1] + (var3 - 1));
      this.d[4] = (short)(this.d[0] + Math.abs(this.e.nextInt()) % var2);
      this.d[5] = (short)(this.d[1] + Math.abs(this.e.nextInt()) % var3);
      return this.d;
   }

   private boolean a(short[] var1) {
      int var2 = var1[0] - 1 >= 0 ? var1[0] - 1 : 0;
      int var3 = var1[2] + 1 <= 34 ? var1[2] + 1 : 34;
      int var4 = var1[1] - 1 >= 0 ? var1[1] - 1 : 0;
      int var5 = var1[3] + 1 <= 34 ? var1[3] + 1 : 34;

      for (int var6 = var2; var6 <= var3; var6++) {
         for (int var7 = var4; var7 <= var5; var7++) {
            if (this.w[var6][var7] == 0) {
               return false;
            }
         }
      }

      this.a(var1[0], (short)(var1[2] - var1[0] + 1), var1[1], (short)(var1[3] - var1[1] + 1));
      if (var1[2] != var1[0] && var1[3] != var1[1]) {
         short[] var9 = new short[6];

         for (int var8 = 0; var8 < 6; var8++) {
            var9[var8] = var1[var8];
         }

         this.t.addElement(var9);
      }

      return true;
   }

   private void d() {
      Object var1 = null;
      int var2 = this.m.size();

      for (int var3 = 0; var3 < var2; var3++) {
         Integer var4 = (Integer)this.m.elementAt(var3);
         int var5 = this.j.size();
         int var6 = Integer.MAX_VALUE;
         Integer var7 = null;
         int var8 = -1;

         for (int var9 = 0; var9 < var5; var9++) {
            Integer var10 = (Integer)this.j.elementAt(var9);
            if (!var10.equals(var4)) {
               int var11 = this.g(var4, var10);
               if (var11 < var6) {
                  var6 = var11;
                  var7 = var10;
               }
            } else {
               var8 = var9;
            }
         }

         if (var7 != null) {
            this.h(var4, var7);
         }

         if (var8 != -1) {
            this.j.removeElementAt(var8);
         }
      }
   }

   private int a(short var1) {
      int var2 = -1;
      if (var1 == 1) {
         this.a(17, 1, 0, 5);
         var2 = this.a((short)17, (short)4);
      } else if (var1 == 3) {
         this.a(17, 1, 30, 5);
         var2 = this.a((short)17, (short)30);
      } else if (var1 == 4) {
         this.a(0, 5, 17, 1);
         var2 = this.a((short)4, (short)17);
      } else if (var1 == 2) {
         this.a(30, 5, 17, 1);
         var2 = this.a((short)30, (short)17);
      }

      return var2;
   }

   private void h(int var1, int var2) {
      short[] var3 = this.a(var1);
      short var4 = var3[0];
      short var5 = var3[1];
      var3 = this.a(var2);
      short var6 = var3[0];
      short var7 = var3[1];
      int var8 = Math.abs(this.e.nextInt() % 2);
      if (var8 == 0) {
         if (var6 > var4) {
            this.a(var4, var6 - var4 + 1, var5, 1);
         } else {
            this.a(var6, var4 - var6 + 1, var5, 1);
         }

         if (var7 > var5) {
            this.a(var6, 1, var5, var7 - var5 + 1);
         } else {
            this.a(var6, 1, var7, var5 - var7 + 1);
         }
      } else {
         if (var7 > var5) {
            this.a(var4, 1, var5, var7 - var5 + 1);
         } else {
            this.a(var4, 1, var7, var5 - var7 + 1);
         }

         if (var6 > var4) {
            this.a(var4, var6 - var4 + 1, var7, 1);
         } else {
            this.a(var6, var4 - var6 + 1, var7, 1);
         }
      }
   }

   private void a(int var1, int var2, int var3, int var4) {
      for (int var5 = var1; var5 < var1 + var2; var5++) {
         for (int var6 = var3; var6 < var3 + var4; var6++) {
            if (this.w[var5][var6] != 8) {
               this.w[var5][var6] = 0;
            }
         }
      }
   }

   private int a(short var1, short var2) {
      return var1 << 16 | var2;
   }

   private short[] a(int var1) {
      this.f[0] = (short)((-65536 & var1) >>> 16);
      this.f[1] = (short)(65535 & var1);
      return this.f;
   }

   private void d(int var1) {
      Integer var2 = new Integer(var1);
      this.m.addElement(var2);
      this.j.addElement(var2);
   }

   private int g(int var1, int var2) {
      short[] var3 = this.a(var1);
      short var4 = var3[0];
      short var5 = var3[1];
      short[] var6 = this.a(var2);
      short var7 = var6[0];
      short var8 = var6[1];
      return (var7 - var4) * (var7 - var4) + (var8 - var5) * (var8 - var5);
   }

   boolean d(int var1, int var2) {
      byte var3 = this.w[var1][var2];
      if (f.a((byte)1, var3)) {
         return false;
      } else if (f.a((byte)2, var3)) {
         return false;
      } else {
         return f.a((byte)8, var3) ? false : !f.a((byte)32, var3);
      }
   }

   d c(int var1, int var2) {
      byte var3 = this.w[var1][var2];
      if (f.a((byte)1, var3)) {
         return null;
      }

      if (!f.a((byte)2, var3)) {
         return null;
      }

      Enumeration var4 = ESGame.G[this.o - 1].elements();

      while (var4.hasMoreElements()) {
         byte[] var5 = (byte[])var4.nextElement();
         d var6 = d.a(var5);
         if (var6.o == var1 && var6.m == var2) {
            return var6;
         }
      }

      return null;
   }

   void b(byte[] var1) {
      byte var2 = var1[0];
      byte var3 = var1[1];
      byte var4 = this.w[var2][var3];
      if (!f.a((byte)1, var4)) {
         if (f.a((byte)16, var4)) {
            ESGame.S[this.o - 1].remove(f.b((int)var2, (int)var3));
            this.w[var2][var3] = f.c((byte)16, this.w[var2][var3]);
         }
      }
   }

   void d(byte[] var1) {
      byte var2 = var1[0];
      byte var3 = var1[1];
      byte var4 = this.w[var2][var3];
      if (!f.a((byte)1, var4)) {
         if (f.a((byte)4, var4)) {
            ESGame.au[this.o - 1].removeElement(var1);
            if (this.f(var2, var3) == 0) {
               this.w[var2][var3] = f.c((byte)4, this.w[var2][var3]);
            }
         }
      }
   }

   int f(int var1, int var2) {
      int var3 = 0;
      Enumeration var4 = ESGame.au[this.o - 1].elements();

      while (var4.hasMoreElements()) {
         byte[] var5 = (byte[])var4.nextElement();
         if (var5[0] == var1 && var5[1] == var2) {
            var3++;
         }
      }

      return var3;
   }

   byte[] e(int var1, int var2) {
      Enumeration var3 = ESGame.au[this.o - 1].elements();

      while (var3.hasMoreElements()) {
         byte[] var4 = (byte[])var3.nextElement();
         if (var4[0] == var1 && var4[1] == var2) {
            return var4;
         }
      }

      return null;
   }

   Vector b(int var1, int var2) {
      Vector var3 = new Vector(5);
      Enumeration var4 = ESGame.au[this.o - 1].elements();

      while (var4.hasMoreElements()) {
         byte[] var5 = (byte[])var4.nextElement();
         if (var5[0] == var1 && var5[1] == var2) {
            var3.addElement(var5);
         }
      }

      return var3;
   }

   void b(int var1, int var2, int var3, byte[][] var4) {
      if (var3 != 1 && var3 != 3) {
         if (var3 == 2 || var3 == 4) {
            byte var12 = 0;
            if (var3 == 2) {
               var12 = 1;
            } else {
               var12 = -1;
            }

            var4[0][0] = this.i(var1, var2 - var12);
            var4[1][0] = 0;
            var4[2][0] = this.i(var1, var2 + var12);
            int var17 = var1 + var12;

            for (int var21 = 0; var21 < 5; var21++) {
               var4[var21][1] = this.i(var17, var2 + (var21 - 2) * var12);
            }

            var17 = var1 + 2 * var12;

            for (int var22 = 0; var22 < 7; var22++) {
               var4[var22][2] = this.i(var17, var2 + (var22 - 3) * var12);
            }

            var17 = var1 + 3 * var12;

            for (int var23 = 0; var23 < 9; var23++) {
               var4[var23][3] = this.i(var17, var2 + (var23 - 4) * var12);
            }

            var17 = var1 + 4 * var12;

            for (int var24 = 0; var24 < 9; var24++) {
               var4[var24][4] = this.i(var17, var2 + (var24 - 4) * var12);
            }
         }
      } else {
         byte var5 = 0;
         if (var3 == 1) {
            var5 = 1;
         } else {
            var5 = -1;
         }

         var4[0][0] = this.i(var1 - var5, var2);
         var4[1][0] = this.i(var1, var2);
         var4[2][0] = this.i(var1 + var5, var2);
         int var6 = var2 - var5;

         for (int var7 = 0; var7 < 5; var7++) {
            var4[var7][1] = this.i(var1 + (var7 - 2) * var5, var6);
         }

         var6 = var2 - 2 * var5;

         for (int var8 = 0; var8 < 7; var8++) {
            var4[var8][2] = this.i(var1 + (var8 - 3) * var5, var6);
         }

         var6 = var2 - 3 * var5;

         for (int var9 = 0; var9 < 9; var9++) {
            var4[var9][3] = this.i(var1 + (var9 - 4) * var5, var6);
         }

         var6 = var2 - 4 * var5;

         for (int var10 = 0; var10 < 9; var10++) {
            var4[var10][4] = this.i(var1 + (var10 - 4) * var5, var6);
         }
      }
   }

   void c(int var1, int var2, int var3, byte[][] var4) {
      this.a(var1, var2, var3, 7, var4);
   }

   void a(int var1, int var2, int var3, byte[][] var4) {
      this.a(var1, var2, var3, 17, var4);
   }

   void a(int var1, int var2, int var3, int var4, byte[][] var5) {
      int var6 = var4 / 2;
      if (var3 == 1 || var3 == 3) {
         byte var17 = 0;
         if (var3 == 1) {
            var17 = 1;
         } else {
            var17 = -1;
         }

         for (int var19 = 0; var19 < var4; var19++) {
            for (int var22 = 0; var22 < var4; var22++) {
               var5[var22][var19] = (byte)(this.i(var1 + (var22 - var6) * var17, var2 + (var19 - var6) * var17) & 1);
               if ((var5[var22][var19] & 1) == 0) {
                  var5[var22][var19] = (byte)(this.i(var1 + (var22 - var6) * var17, var2 + (var19 - var6) * var17) & 8);
               }
            }
         }

         if (this.o > 1) {
            Enumeration var23 = ESGame.G[this.o - 1].elements();

            while (var23.hasMoreElements()) {
               byte[] var27 = (byte[])var23.nextElement();
               d var33 = d.a(var27);
               int var40 = var17 * (var33.o - var1) + var6;
               int var46 = var17 * (var33.m - var2) + var6;
               if (var40 >= 0 && var40 < var4 && var46 >= 0 && var46 < var4 && var33.i) {
                  var5[var40][var46] = (byte)(var5[var40][var46] | 2);
               }
            }

            Enumeration var28 = ESGame.S[this.o - 1].elements();

            while (var28.hasMoreElements()) {
               byte[] var34 = (byte[])var28.nextElement();
               int var41 = var17 * (var34[0] - var1) + var6;
               int var47 = var17 * (var34[1] - var2) + var6;
               boolean var50 = true;
               if (var41 >= 0 && var41 < var4 && var47 >= 0 && var47 < var4 && var50) {
                  var5[var41][var47] = (byte)(var5[var41][var47] | 4);
               }
            }

            Enumeration var35 = ESGame.au[this.o - 1].elements();

            while (var35.hasMoreElements()) {
               byte[] var42 = (byte[])var35.nextElement();
               int var48 = var17 * (var42[0] - var1) + var6;
               int var51 = var17 * (var42[1] - var2) + var6;
               boolean var52 = (var42[6] & 1) != 0;
               if (var48 >= 0 && var48 < var4 && var51 >= 0 && var51 < var4 && var52) {
                  var5[var48][var51] = (byte)(var5[var48][var51] | 4);
               }
            }
         } else {
            for (int var24 = 0; var24 < 7 && (var24 != 6 || k.d); var24++) {
               if (k.b[var24]) {
                  int var29 = var17 * (k.j[var24] - var1) + var6;
                  int var36 = var17 * (k.i[var24] - var2) + var6;
                  boolean var43 = true;
                  if (var29 >= 0 && var29 < var4 && var36 >= 0 && var36 < var4 && var43) {
                     var5[var29][var36] = (byte)(var5[var29][var36] | 4);
                  }
               }
            }
         }
      } else if (var3 == 2 || var3 == 4) {
         byte var7 = 0;
         if (var3 == 2) {
            var7 = 1;
         } else {
            var7 = -1;
         }

         for (int var8 = 0; var8 < var4; var8++) {
            for (int var9 = 0; var9 < var4; var9++) {
               var5[var9][var8] = (byte)(this.i(var1 - (var8 - var6) * var7, var2 + (var9 - var6) * var7) & 1);
               if ((var5[var9][var8] & 1) == 0) {
                  var5[var9][var8] = (byte)(this.i(var1 - (var8 - var6) * var7, var2 + (var9 - var6) * var7) & 8);
               }
            }
         }

         if (this.o > 1) {
            Enumeration var20 = ESGame.G[this.o - 1].elements();

            while (var20.hasMoreElements()) {
               byte[] var10 = (byte[])var20.nextElement();
               d var11 = d.a(var10);
               int var12 = var7 * (var11.m - var2) + var6;
               int var13 = var6 - var7 * (var11.o - var1);
               if (var12 >= 0 && var12 < var4 && var13 >= 0 && var13 < var4 && var11.i) {
                  var5[var12][var13] = (byte)(var5[var12][var13] | 2);
               }
            }

            Enumeration var25 = ESGame.S[this.o - 1].elements();

            while (var25.hasMoreElements()) {
               byte[] var30 = (byte[])var25.nextElement();
               int var37 = var7 * (var30[1] - var2) + var6;
               int var44 = var6 - var7 * (var30[0] - var1);
               boolean var14 = true;
               if (var37 >= 0 && var37 < var4 && var44 >= 0 && var44 < var4 && var14) {
                  var5[var37][var44] = (byte)(var5[var37][var44] | 4);
               }
            }

            Enumeration var31 = ESGame.au[this.o - 1].elements();

            while (var31.hasMoreElements()) {
               byte[] var38 = (byte[])var31.nextElement();
               int var45 = var7 * (var38[1] - var2) + var6;
               int var49 = var6 - var7 * (var38[0] - var1);
               boolean var15 = (var38[6] & 1) != 0;
               if (var45 >= 0 && var45 < var4 && var49 >= 0 && var49 < var4 && var15) {
                  var5[var45][var49] = (byte)(var5[var45][var49] | 4);
               }
            }
         } else {
            for (int var21 = 0; var21 < 7 && (var21 != 6 || k.d); var21++) {
               if (k.b[var21]) {
                  int var26 = var7 * (k.i[var21] - var2) + var6;
                  int var32 = var6 - var7 * (k.j[var21] - var1);
                  boolean var39 = true;
                  if (var26 >= 0 && var26 < var4 && var32 >= 0 && var32 < var4 && var39) {
                     var5[var26][var32] = (byte)(var5[var26][var32] | 4);
                  }
               }
            }
         }
      }
   }

   int[] a(int var1, int var2, int var3, int var4, int var5) {
      int var6 = 0;
      int var7 = 0;
      if (var3 == 1 || var3 == 3) {
         byte var10 = 0;
         if (var3 == 1) {
            var10 = 1;
         } else {
            var10 = -1;
         }

         var6 = var10 * (var4 - var1) + 3;
         var7 = var10 * (var5 - var2) + 3;
      } else if (var3 == 2 || var3 == 4) {
         byte var8 = 0;
         if (var3 == 2) {
            var8 = 1;
         } else {
            var8 = -1;
         }

         var6 = var8 * (var5 - var2) + 3;
         var7 = 3 - var8 * (var4 - var1);
      }

      this.u[0] = var6;
      this.u[1] = var7;
      return this.u;
   }

   byte i(int var1, int var2) {
      int var3 = var1;
      int var4 = var2;
      byte var5 = this.o;
      i var6 = null;
      if (var1 < 0) {
         var5 = this.x[3];
         if (var5 <= 0) {
            return 1;
         }

         var6 = ESGame.u[var5 - 1];
         if (var5 != 1 && this.o != 1) {
            var3 = (byte)(var6.g - 1);
         } else {
            var3 = (byte)(var6.g - 1);
            var4 = (byte)(var4 + (var6.y - this.y) / 2);
         }
      } else if (var1 >= this.g) {
         var5 = this.x[1];
         if (var5 <= 0) {
            return 1;
         }

         var6 = ESGame.u[var5 - 1];
         if (var5 != 1 && this.o != 1) {
            var3 = 0;
         } else {
            var3 = 0;
            var4 = (byte)(var4 + (var6.y - this.y) / 2);
         }
      } else if (var2 < 0) {
         var5 = this.x[0];
         if (var5 <= 0) {
            return 1;
         }

         var6 = ESGame.u[var5 - 1];
         if (var5 != 1 && this.o != 1) {
            var4 = (byte)(var6.y - 1);
         } else {
            var3 = (byte)(var3 + (var6.g - this.g) / 2);
            var4 = (byte)(var6.y - 1);
         }
      } else if (var2 >= this.y) {
         var5 = this.x[2];
         if (var5 <= 0) {
            return 1;
         }

         var6 = ESGame.u[var5 - 1];
         if (var5 != 1 && this.o != 1) {
            var4 = 0;
         } else {
            var3 = (byte)(var3 + (var6.g - this.g) / 2);
            var4 = 0;
         }
      }

      if (var5 != this.o) {
         if (var3 < 0 || var3 >= var6.g) {
            return 1;
         } else if (var4 < 0 || var4 >= var6.y) {
            return 1;
         } else {
            return var6.k ? var6.w[var3][var4] : 1;
         }
      } else {
         return this.w[var1][var2];
      }
   }

   byte a(int var1, int var2, byte[][] var3) {
      return var2 < 4 ? var3[var1 + var2 + 1][var2] : var3[var1 + var2][var2];
   }

   String[] a() {
      return c[this.o - 1];
   }

   static void g() throws Exception {
      DataInputStream var0 = f.a("/dungnamesin.dat");
      c = new String[37][2];

      for (int var1 = 0; var1 < 37; var1++) {
         for (int var2 = 0; var2 < 2; var2++) {
            c[var1][var2] = var0.readUTF();
         }
      }
   }

   void a(int var1, int var2) {
      int var3 = Math.max(var1 - 4, 0);
      int var4 = Math.min(var1 + 4, this.g - 1);
      int var5 = Math.max(var2 - 4, 0);
      int var6 = Math.min(var2 + 4, this.y - 1);

      for (int var7 = var3; var7 <= var4; var7++) {
         for (int var8 = var5; var8 <= var6; var8++) {
            byte var9 = this.w[var7][var8];
            if (!f.a((byte)1, var9)) {
               this.w[var7][var8] = f.c((byte)2, var9);
            }
         }
      }

      Hashtable var13 = ESGame.G[this.o - 1];
      if (var13 != null) {
         Enumeration var14 = var13.elements();

         while (var14.hasMoreElements()) {
            byte[] var10 = (byte[])var14.nextElement();
            byte var11 = var10[4];
            byte var12 = var10[5];
            if (var11 >= var3 && var11 <= var4 && var12 >= var5 && var12 <= var6) {
               this.w[var11][var12] = (byte)(this.w[var11][var12] | 2);
            }
         }
      }
   }
}
