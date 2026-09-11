import java.io.DataInputStream;
import java.util.Random;
import java.util.Vector;

class c {
   private byte[][] a;
   static final int[] d = new int[]{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 3, 1, 3, 3, 1, 3, 3, 3, 3, 5, 3, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5};
   Vector c;
   Vector f;
   Vector h;
   Random b;
   static int e;
   static int i;
   static byte[][] g;

   public c(i[] var1, h var2) {
      this.c();

      try {
         this.a();
      } catch (Exception var4) {
      }

      var1[0] = new i((byte)1, this.a[0], e, i, g);

      for (int var3 = 1; var3 < 37; var3++) {
         var1[var3] = new i((byte)(var3 + 1), this.a[var3]);
         this.a(var1[var3]);
         var2.G = 60 + var3;
         System.gc();
      }
   }

   private void c() {
      e = 19;
      i = 19;
      g = new byte[i][e];

      for (int var1 = 0; var1 < i; var1++) {
         for (int var2 = 0; var2 < e; var2++) {
            g[var1][var2] = 1;
         }
      }

      for (int var6 = 0; var6 < e; var6++) {
         g[var6][9] = 0;
      }

      for (int var3 = 0; var3 < i; var3++) {
         g[9][var3] = 0;
      }

      for (int var4 = 4; var4 < 15; var4++) {
         g[4][var4] = 0;
         g[14][var4] = 0;
      }

      for (int var5 = 4; var5 < 15; var5++) {
         g[var5][4] = 0;
         g[var5][14] = 0;
      }

      g[5][6] = 0;
      g[6][6] = 0;
      g[7][6] = 0;
      g[7][5] = 0;
      g[7][7] = 0;
      g[12][6] = 0;
      g[13][6] = 0;
      g[12][8] = 0;
      g[8][8] = 0;
      g[10][8] = 0;
      g[8][10] = 0;
      g[10][10] = 0;
      g[11][12] = 0;
      g[11][13] = 0;
      g[12][12] = 0;
      g[5][11] = 0;
      g[6][11] = 0;
   }

   private void a() throws Exception {
      DataInputStream var1 = ESGame.getResource("geomin.dat");
      this.a = new byte[37][6];

      for (int var2 = 0; var2 < 37; var2++) {
         for (int var3 = 0; var3 < 6; var3++) {
            this.a[var2][var3] = var1.readByte();
         }
      }

      var1.close();
   }

   public void a(i var1) {
      long var2 = var1.e * 8000;
      var1.d = new byte[var1.h][var1.l];
      var1.j = var1.a[4];
      var1.g = var1.a[5];
      this.h = new Vector();
      int var4 = -1;
      this.b = new Random(var2);

      for (int var5 = 0; var5 < 35; var5++) {
         for (int var6 = 0; var6 < 35; var6++) {
            var1.d[var5][var6] = 1;
         }
      }

      this.c = new Vector();
      this.f = new Vector();
      if (this.a(var1.j)) {
         var4 = this.a(var1, var1.j);
         if (var4 >= 0) {
            this.b(var4);
         }
      }

      if (this.a(var1.g)) {
         var4 = this.a(var1, var1.g);
         if (var4 >= 0) {
            this.b(var4);
         }
      }

      boolean var21 = false;
      int var7 = 0;

      while (var7 < 15) {
         short[] var8 = this.b();
         if (this.a(var8, var1)) {
            var7++;
            int var9 = this.a(var8[4], var8[5]);
            this.b(var9);
            if (var7 >= 2 && !var21) {
               short var10 = var8[4];
               short var11 = var8[5];
               short var12 = (short)(var8[2] - var8[0] + 1);
               short var13 = (short)(var8[3] - var8[1] + 1);
               if (var12 >= 3 && var13 >= 3) {
                  short var14 = (short)(var8[0] + var12 / 2);
                  short var15 = (short)(var8[1] + var13 / 2);
                  if (var14 != var10 || var15 != var11) {
                     if (var1.e == 3) {
                        var1.d[var14][var15] = (byte)(var1.d[var14][var15] | 32);
                        k.f[5] = (byte)var14;
                        k.e[5] = (byte)var15;
                     } else if (var1.e == 12) {
                        var1.d[var14][var15] = (byte)(var1.d[var14][var15] | 32);
                        k.f[6] = (byte)var14;
                        k.e[6] = (byte)var15;
                     } else if (var1.e == 21) {
                        var1.d[var14][var15] = (byte)(var1.d[var14][var15] | 32);
                        k.f[7] = (byte)var14;
                        k.e[7] = (byte)var15;
                     } else if (var1.e == 30) {
                        var1.d[var14][var15] = (byte)(var1.d[var14][var15] | 32);
                        k.f[8] = (byte)var14;
                        k.e[8] = (byte)var15;
                     } else {
                        var1.d[var14][var15] = (byte)(var1.d[var14][var15] | 8);
                     }

                     var21 = true;
                  }
               }
            }
         }
      }

      this.b(var1);
      int var22 = this.h.size();
      short var24 = (short)(var1.e << 8);
      d var25 = null;

      for (int var27 = 0; var27 < var22; var27++) {
         short[] var28 = (short[])this.h.elementAt(var27);
         var25 = d.a(this.b, var1.c, var1.e, -1);
         var25.o = (byte)var28[4];
         var25.m = (byte)var28[5];
         var1.d[var25.o][var25.m] = (byte)(var1.d[var25.o][var25.m] | 2);
         var25.c();
      }

      System.gc();
      var22 = this.h.size();
      int[] var29 = new int[var22];
      int[] var30 = new int[var22];

      for (int var31 = 0; var31 < var22; var31++) {
         var29[var31] = var31;
         int var32 = f.a(this.b, 1000);
         var30[var31] = var32;
      }

      for (int var33 = 1; var33 < var22; var33++) {
         int var16 = var30[var33];
         int var17 = var29[var33];

         int var18;
         for (var18 = var33 - 1; var18 >= 0 && var30[var18] < var16; var18--) {
            var30[var18 + 1] = var30[var18];
            var29[var18 + 1] = var29[var18];
         }

         var30[var18 + 1] = var16;
         var29[var18 + 1] = var17;
      }

      int[] var34 = new int[5];

      for (int var35 = 0; var35 < 5; var35++) {
         var34[var35] = var29[var35];
      }

      this.a(var34, var1);
      this.h = null;
      this.c = null;
      this.f = null;
      System.gc();
   }

   void a(int[] var1, i var2) {
      int var3 = 0;
      int var4 = d[var2.c - 1];
      boolean var5 = true;

      for (int var6 = 0; var6 < 5; var6++) {
         int var7 = var1[var6];
         short[] var8 = (short[])this.h.elementAt(var7);
         boolean var9 = false;
         if (var5) {
            var3 = a.a(this.b, var4);
            var5 = false;
         } else {
            var3 = a.a(this.b, var2.c, 2);
         }

         short var10 = (short)(var8[2] - var8[0] + 1);
         short var11 = (short)(var8[3] - var8[1] + 1);
         short var12 = (short)(var8[0] + Math.abs(this.b.nextInt() % var10));

         short var13;
         for (var13 = (short)(var8[1] + Math.abs(this.b.nextInt() % var11));
            (var2.d[var12][var13] & 8) != 0 && (var2.d[var12][var13] & 32) != 0;
            var13 = (short)(var8[1] + Math.abs(this.b.nextInt() % var11))
         ) {
            var12 = (short)(var8[0] + Math.abs(this.b.nextInt() % var10));
         }

         byte[] var14 = new byte[8];
         var14[0] = (byte)var12;
         var14[1] = (byte)var13;
         var14[2] = (byte)(var5 ? 1 : 0);
         byte var15 = (byte)(Math.abs(this.b.nextInt() % 3) << 6);
         var14[3] = (byte)(var15 | var2.c);
         byte var16 = (byte)(var3 & 0xFF);
         byte var17 = 0;
         if (var16 == 86) {
            var17 = (byte)(var3 >>> 8 & 0xFF);
         }

         var14[4] = var16;
         var14[7] = var17;
         short var18 = a.a();
         var16 = (byte)(var18 >>> 8 & 0xFF);
         var17 = (byte)(var18 & 0xFF);
         var14[5] = var16;
         var14[6] = var17;
         byte var19 = var14[0];
         byte var20 = var14[1];
         ESGame.chests[var2.e - 1].put(f.b((int)var19, (int)var20), var14);
         var2.d[var19][var20] = (byte)(var2.d[var19][var20] | 16);
      }
   }

   private short[] b() {
      short[] var1 = new short[6];
      byte var2 = 3;
      byte var3 = 3;
      byte var4 = 31;
      byte var5 = 31;
      byte var6 = 4;
      int var7 = 2 + Math.abs(this.b.nextInt()) % var6;
      int var8 = 2 + Math.abs(this.b.nextInt()) % var6;
      int var9 = var4 - var3 + 1 - (var7 - 1);
      int var10 = var5 - var2 + 1 - (var8 - 1);
      var1[0] = (short)(var3 + Math.abs(this.b.nextInt()) % var9);
      var1[1] = (short)(var2 + Math.abs(this.b.nextInt()) % var10);
      var1[2] = (short)(var1[0] + (var7 - 1));
      var1[3] = (short)(var1[1] + (var8 - 1));
      var1[4] = (short)(var1[0] + Math.abs(this.b.nextInt()) % var7);
      var1[5] = (short)(var1[1] + Math.abs(this.b.nextInt()) % var8);
      return var1;
   }

   private boolean a(short[] var1, i var2) {
      int var3 = var1[0] - 1 >= 0 ? var1[0] - 1 : 0;
      int var4 = var1[2] + 1 <= 34 ? var1[2] + 1 : 34;
      int var5 = var1[1] - 1 >= 0 ? var1[1] - 1 : 0;
      int var6 = var1[3] + 1 <= 34 ? var1[3] + 1 : 34;

      for (int var7 = var3; var7 <= var4; var7++) {
         for (int var8 = var5; var8 <= var6; var8++) {
            if (var2.d[var7][var8] == 0) {
               return false;
            }
         }
      }

      this.a(var2, var1[0], (short)(var1[2] - var1[0] + 1), var1[1], (short)(var1[3] - var1[1] + 1));
      if (var1[2] != var1[0] && var1[3] != var1[1]) {
         short[] var10 = new short[6];

         for (int var9 = 0; var9 < 6; var9++) {
            var10[var9] = var1[var9];
         }

         this.h.addElement(var10);
      }

      return true;
   }

   private void b(i var1) {
      Object var2 = null;
      int var3 = this.c.size();

      for (int var4 = 0; var4 < var3; var4++) {
         Integer var5 = (Integer)this.c.elementAt(var4);
         int var6 = this.f.size();
         int var7 = Integer.MAX_VALUE;
         Integer var8 = null;
         int var9 = -1;

         for (int var10 = 0; var10 < var6; var10++) {
            Integer var11 = (Integer)this.f.elementAt(var10);
            if (!var11.equals(var5)) {
               int var12 = this.a(var5, var11);
               if (var12 < var7) {
                  var7 = var12;
                  var8 = var11;
               }
            } else {
               var9 = var10;
            }
         }

         if (var8 != null) {
            this.a(var1, var5, var8);
         }

         if (var9 != -1) {
            this.f.removeElementAt(var9);
         }
      }
   }

   private int a(i var1, short var2) {
      int var3 = -1;
      if (var2 == 1) {
         this.a(var1, 17, 1, 0, 5);
         var3 = this.a((short)17, (short)4);
      } else if (var2 == 3) {
         this.a(var1, 17, 1, 30, 5);
         var3 = this.a((short)17, (short)30);
      } else if (var2 == 4) {
         this.a(var1, 0, 5, 17, 1);
         var3 = this.a((short)4, (short)17);
      } else if (var2 == 2) {
         this.a(var1, 30, 5, 17, 1);
         var3 = this.a((short)30, (short)17);
      }

      return var3;
   }

   private void a(i var1, int var2, int var3) {
      short[] var4 = this.a(var2);
      short var5 = var4[0];
      short var6 = var4[1];
      var4 = this.a(var3);
      short var7 = var4[0];
      short var8 = var4[1];
      int var9 = Math.abs(this.b.nextInt() % 2);
      if (var9 == 0) {
         if (var7 > var5) {
            this.a(var1, var5, var7 - var5 + 1, var6, 1);
         } else {
            this.a(var1, var7, var5 - var7 + 1, var6, 1);
         }

         if (var8 > var6) {
            this.a(var1, var7, 1, var6, var8 - var6 + 1);
         } else {
            this.a(var1, var7, 1, var8, var6 - var8 + 1);
         }
      } else {
         if (var8 > var6) {
            this.a(var1, var5, 1, var6, var8 - var6 + 1);
         } else {
            this.a(var1, var5, 1, var8, var6 - var8 + 1);
         }

         if (var7 > var5) {
            this.a(var1, var5, var7 - var5 + 1, var8, 1);
         } else {
            this.a(var1, var7, var5 - var7 + 1, var8, 1);
         }
      }
   }

   private void a(i var1, int var2, int var3, int var4, int var5) {
      for (int var6 = var2; var6 < var2 + var3; var6++) {
         for (int var7 = var4; var7 < var4 + var5; var7++) {
            if (var1.d[var6][var7] != 8 && var1.d[var6][var7] != 32) {
               var1.d[var6][var7] = 0;
            }
         }
      }
   }

   private int a(int var1, int var2) {
      short[] var3 = this.a(var1);
      short var4 = var3[0];
      short var5 = var3[1];
      short[] var6 = this.a(var2);
      short var7 = var6[0];
      short var8 = var6[1];
      return (var7 - var4) * (var7 - var4) + (var8 - var5) * (var8 - var5);
   }

   private int a(short var1, short var2) {
      return var1 << 16 | var2;
   }

   private short[] a(int var1) {
      return new short[]{(short)((-65536 & var1) >>> 16), (short)(65535 & var1)};
   }

   private void b(int var1) {
      Integer var2 = new Integer(var1);
      this.c.addElement(var2);
      this.f.addElement(var2);
   }

   boolean a(short var1) {
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
}
