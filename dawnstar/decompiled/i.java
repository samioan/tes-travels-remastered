import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

public class i {
   private static final String[] n = new String[]{
      "Dawnstar",
      "North Creek",
      "North Creek 2",
      "North Creek 3",
      "Ice Spike",
      "Ice Spike 2",
      "Ice Spike 3",
      "Blind Fjord",
      "Blind Fjord 2",
      "Blind Fjord 3",
      "Slipneck Fjord",
      "Slipneck Fjord 2",
      "Slipneck Fjord 3",
      "Troll Pace",
      "Troll Pace 2",
      "Troll Pace 3",
      "Ice Tribe Haven",
      "Ice Tribe Haven 2",
      "Ice Tribe Haven 3",
      "Dawnstar Run",
      "Dawnstar Run 2",
      "Dawnstar Run 3",
      "Massacre Caves",
      "Massacre Caves 2",
      "Massacre Caves 3",
      "Frostheim",
      "Frostheim 2",
      "Frostheim 3",
      "Glacier Run",
      "Glacier Run 2",
      "Glacier Run 3",
      "Troll Hole",
      "Troll Hole 2",
      "Troll Hole 3",
      "Ice Council",
      "Ice Council 2",
      "Ice Council 3"
   };
   static final int[][] m = new int[][]{
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
   static final byte[] i = new byte[]{
      1, 5, 9, 13, 14, 15, 22, 23, 24, 2, 6, 10, 19, 20, 21, 31, 32, 33, 3, 7, 11, 16, 17, 18, 28, 29, 30, 4, 8, 12, 25, 26, 27, 34, 35, 36
   };
   byte e;
   byte c;
   short h;
   short l;
   byte[][] d;
   short j;
   short g;
   boolean b = true;
   byte[] a;
   int[] k = new int[2];
   boolean f = false;

   public i() {
   }

   public i(byte var1, byte[] var2) {
      this();
      this.e = var1;
      this.c();
      this.h = 35;
      this.l = 35;
      this.a = var2;
      this.j = this.a[4];
      this.g = this.a[5];
   }

   public i(byte var1, byte[] var2, int var3, int var4, byte[][] var5) {
      this();
      this.e = var1;
      this.c();
      this.h = (short)var3;
      this.l = (short)var4;
      this.d = var5;
      this.a = var2;
      this.j = this.a[4];
      this.g = this.a[5];

      for (int var6 = 0; var6 < 5; var6++) {
         this.d[k.f[var6]][k.e[var6]] = (byte)(this.d[k.f[var6]][k.e[var6]] | 32);
      }

      this.b = true;
   }

   void c() {
      this.c = 1;
      if (this.e >= 2 && this.e <= 37) {
         this.c = i[this.e - 2];
      }
   }

   void a(int var1) {
      for (int var2 = 0; var2 < var1; var2++) {
         while (!this.a(ESGame.nextInt(this.h), ESGame.nextInt(this.l), -1)) {
         }
      }
   }

   boolean a(int var1, int var2, int var3) {
      int var4 = -1;
      if (var3 == 42 || var3 == 41) {
         var4 = var3;
         var3 = this.c;
      }

      if (var3 < 0) {
         var3 = this.c;
      }

      boolean var5 = false;

      for (int var6 = 0; var6 <= 4; var6++) {
         int var7 = var1;
         int var8 = var2;
         if (var6 < 2) {
            var7 += 2 * var6 - 1;
         } else {
            var8 += 2 * var6 - 5;
         }

         if (this.b(var7, var8)) {
            d var9 = d.a(ESGame.r, var3, this.e, var4);
            var5 = true;
            var9.o = (byte)var7;
            var9.m = (byte)var8;
            var9.c();
            this.d[var9.o][var9.m] = f.b((byte)2, this.d[var9.o][var9.m]);
            break;
         }
      }

      return var5;
   }

   void b(byte[] var1) {
      byte var2 = var1[0];
      byte var3 = var1[1];
      ESGame.droppedItems[this.e - 1].addElement(var1);
      this.d[var2][var3] = (byte)(this.d[var2][var3] | 4);
   }

   void b() {
      for (int var1 = 0; var1 < this.h; var1++) {
         for (int var2 = 0; var2 < this.l; var2++) {
            this.d[var1][var2] = f.c((byte)2, this.d[var1][var2]);
            this.d[var1][var2] = f.c((byte)16, this.d[var1][var2]);
            this.d[var1][var2] = f.c((byte)4, this.d[var1][var2]);
         }
      }

      boolean var8 = false;
      Enumeration var3 = ESGame.monsters[this.e - 1].keys();

      while (var3.hasMoreElements()) {
         String var4 = (String)var3.nextElement();
         int var5 = var4.indexOf(44, 0);
         int var6 = Integer.parseInt(var4.substring(0, var5));
         int var7 = Integer.parseInt(var4.substring(var5 + 1));
         this.d[var6][var7] = (byte)(this.d[var6][var7] | 2);
      }

      var8 = false;
      Hashtable var11 = ESGame.chests[this.e - 1];
      if (var11 != null) {
         Enumeration var12 = var11.elements();

         while (var12.hasMoreElements()) {
            byte[] var14 = (byte[])var12.nextElement();
            this.d[var14[0]][var14[1]] = (byte)(this.d[var14[0]][var14[1]] | 16);
         }
      }

      var8 = false;
      Enumeration var13 = ESGame.droppedItems[this.e - 1].elements();

      while (var13.hasMoreElements()) {
         byte[] var15 = (byte[])var13.nextElement();
         this.d[var15[0]][var15[1]] = (byte)(this.d[var15[0]][var15[1]] | 4);
      }
   }

   boolean b(int var1, int var2) {
      if (var1 >= 0 && var2 >= 0 && var1 < this.h && var2 < this.l) {
         byte var3 = this.d[var1][var2];
         if (f.a((byte)1, var3)) {
            return false;
         } else if (f.a((byte)2, var3)) {
            return false;
         } else {
            return f.a((byte)8, var3) ? false : !f.a((byte)32, var3);
         }
      } else {
         return false;
      }
   }

   byte a(long var1, j var3) {
      byte var4 = var3.x;
      byte var5 = var3.w;
      boolean var8 = false;
      boolean var9 = false;
      d var10 = new d();
      Object var11 = null;

      for (int var12 = -3; var12 < 4; var12++) {
         if (var4 + var12 >= 0 && var4 + var12 < this.h) {
            for (int var13 = -3; var13 < 4; var13++) {
               if (var5 + var13 >= 0 && var5 + var13 < this.l) {
                  int var6 = (var12 >= 0 ? var12 : -var12) + (var13 >= 0 ? var13 : -var13);
                  if (var6 != 0 && var6 <= 3) {
                     byte var7 = this.d[var4 + var12][var5 + var13];
                     if (f.a((byte)2, var7)) {
                        var11 = ESGame.monsters[this.e - 1].get(f.b(var4 + var12, var5 + var13));
                        if (var11 != null) {
                           d.a(var10, (byte[])var11);
                           if (var6 == 1) {
                              if (var10.a(var3, var1)) {
                                 var8 = true;
                              }

                              var10.c();
                           } else if (var10.a(var4, var5)) {
                              var9 = true;
                           }
                        }
                     }
                  }
               }
            }
         }
      }

      byte var14 = 0;
      if (var8) {
         var14 = (byte)(var14 + 2);
      }

      if (var9) {
         var14++;
      }

      return var14;
   }

   void a(byte[] var1) {
      byte var2 = var1[0];
      byte var3 = var1[1];
      byte var4 = this.d[var2][var3];
      if (!f.a((byte)1, var4)) {
         if (f.a((byte)16, var4)) {
            ESGame.chests[this.e - 1].remove(f.b((int)var2, (int)var3));
            this.d[var2][var3] = f.c((byte)16, this.d[var2][var3]);
         }
      }
   }

   void c(byte[] var1) {
      byte var2 = var1[0];
      byte var3 = var1[1];
      byte var4 = this.d[var2][var3];
      if (!f.a((byte)1, var4)) {
         if (f.a((byte)4, var4)) {
            ESGame.droppedItems[this.e - 1].removeElement(var1);
         }
      }
   }

   void c(int var1, int var2) {
      this.d[var1][var2] = f.c((byte)4, this.d[var1][var2]);
   }

   Vector d(int var1, int var2) {
      Vector var3 = new Vector(5);
      Enumeration var4 = ESGame.droppedItems[this.e - 1].elements();

      while (var4.hasMoreElements()) {
         byte[] var5 = (byte[])var4.nextElement();
         if (var5[0] == var1 && var5[1] == var2) {
            var3.addElement(var5);
         }
      }

      return var3;
   }

   void a(int var1, int var2, int var3, byte[][] var4) {
      if (var3 != 1 && var3 != 3) {
         if (var3 == 2 || var3 == 4) {
            byte var11 = -1;
            if (var3 == 2) {
               var11 = 1;
            }

            var4[0][0] = this.a(var1, var2 - var11);
            var4[1][0] = this.a(var1, var2);
            var4[2][0] = this.a(var1, var2 + var11);
            int var15 = var1 + var11;

            for (int var19 = 0; var19 < 5; var19++) {
               var4[var19][1] = this.a(var15, var2 + (var19 - 2) * var11);
            }

            var15 = var1 + 2 * var11;

            for (int var20 = 0; var20 < 7; var20++) {
               var4[var20][2] = this.a(var15, var2 + (var20 - 3) * var11);
            }

            var15 = var1 + 3 * var11;

            for (int var21 = 0; var21 < 9; var21++) {
               var4[var21][3] = this.a(var15, var2 + (var21 - 4) * var11);
            }

            var15 = var1 + 4 * var11;

            for (int var22 = 0; var22 < 9; var22++) {
               var4[var22][4] = this.a(var15, var2 + (var22 - 4) * var11);
            }
         }
      } else {
         byte var5 = -1;
         if (var3 == 1) {
            var5 = 1;
         }

         var4[0][0] = this.a(var1 - var5, var2);
         var4[1][0] = this.a(var1, var2);
         var4[2][0] = this.a(var1 + var5, var2);
         int var6 = var2 - var5;

         for (int var7 = 0; var7 < 5; var7++) {
            var4[var7][1] = this.a(var1 + (var7 - 2) * var5, var6);
         }

         var6 = var2 - 2 * var5;

         for (int var8 = 0; var8 < 7; var8++) {
            var4[var8][2] = this.a(var1 + (var8 - 3) * var5, var6);
         }

         var6 = var2 - 3 * var5;

         for (int var9 = 0; var9 < 9; var9++) {
            var4[var9][3] = this.a(var1 + (var9 - 4) * var5, var6);
         }

         var6 = var2 - 4 * var5;

         for (int var10 = 0; var10 < 9; var10++) {
            var4[var10][4] = this.a(var1 + (var10 - 4) * var5, var6);
         }
      }
   }

   void a(int var1, int var2, int var3, int var4, byte[][] var5) {
      int var6 = var4 / 2;
      d var7 = new d();
      if (var3 != 1 && var3 != 3) {
         if (var3 == 2 || var3 == 4) {
            byte var15 = 0;
            if (var3 == 2) {
               var15 = 1;
            } else {
               var15 = -1;
            }

            for (int var17 = 0; var17 < var4; var17++) {
               for (int var18 = 0; var18 < var4; var18++) {
                  byte var13 = this.a(var1 - (var17 - var6) * var15, var2 + (var18 - var6) * var15);
                  var5[var18][var17] = (byte)(var13 & 1);
                  if ((var5[var18][var17] & 1) == 0) {
                     if ((var13 & 4) == 0 && (var13 & 16) == 0 && (var13 & 32) == 0) {
                        var5[var18][var17] = (byte)(var13 & 8);
                     } else {
                        var5[var18][var17] = (byte)(var5[var18][var17] | 4);
                     }

                     if ((var13 & 2) != 0) {
                        byte[] var19 = (byte[])ESGame.monsters[this.e - 1].get(f.b(var1 - (var17 - var6) * var15, var2 + (var18 - var6) * var15));
                        if (var19 != null) {
                           d.a(var7, var19);
                           if (var7.i) {
                              var5[var18][var17] = (byte)(var5[var18][var17] | 2);
                           }
                        }
                     }
                  }
               }
            }
         }
      } else {
         byte var9 = 0;
         if (var3 == 1) {
            var9 = 1;
         } else {
            var9 = -1;
         }

         for (int var10 = 0; var10 < var4; var10++) {
            for (int var11 = 0; var11 < var4; var11++) {
               byte var8 = this.a(var1 + (var11 - var6) * var9, var2 + (var10 - var6) * var9);
               var5[var11][var10] = (byte)(var8 & 1);
               if ((var5[var11][var10] & 1) == 0) {
                  if ((var8 & 4) == 0 && (var8 & 16) == 0 && (var8 & 32) == 0) {
                     var5[var11][var10] = (byte)(var8 & 8);
                  } else {
                     var5[var11][var10] = (byte)(var5[var11][var10] | 4);
                  }

                  if ((var8 & 2) != 0) {
                     byte[] var12 = (byte[])ESGame.monsters[this.e - 1].get(f.b(var1 + (var11 - var6) * var9, var2 + (var10 - var6) * var9));
                     if (var12 != null) {
                        d.a(var7, var12);
                        if (var7.i) {
                           var5[var11][var10] = (byte)(var5[var11][var10] | 2);
                        }
                     }
                  }
               }
            }
         }
      }
   }

   byte a(int var1, int var2) {
      byte var3 = this.e;
      boolean var4 = false;
      i var5 = null;
      if (var1 < 0) {
         var3 = this.a[3];
         if (var3 <= 0) {
            return 1;
         }

         var5 = ESGame.dungeons[var3 - 1];
         var1 = (byte)(var5.h + var1);
         if (var3 == 1 || this.e == 1) {
            var2 = (byte)(var2 + (var5.l - this.l) / 2);
            if (var1 == var5.h - 2) {
               var4 = true;
            }
         }
      } else if (var1 >= this.h) {
         var3 = this.a[1];
         if (var3 <= 0) {
            return 1;
         }

         var5 = ESGame.dungeons[var3 - 1];
         var1 -= this.h;
         if (var3 == 1 || this.e == 1) {
            var2 = (byte)(var2 + (var5.l - this.l) / 2);
            if (var1 == 1) {
               var4 = true;
            }
         }
      } else if (var2 < 0) {
         var3 = this.a[0];
         if (var3 <= 0) {
            return 1;
         }

         var5 = ESGame.dungeons[var3 - 1];
         var2 = (byte)(var5.l + var2);
         if (var3 == 1 || this.e == 1) {
            var1 = (byte)(var1 + (var5.h - this.h) / 2);
            if (var2 == var5.l - 2) {
               var4 = true;
            }
         }
      } else if (var2 >= this.l) {
         var3 = this.a[2];
         if (var3 <= 0) {
            return 1;
         }

         var5 = ESGame.dungeons[var3 - 1];
         var2 -= this.l;
         if (var3 == 1 || this.e == 1) {
            var1 = (byte)(var1 + (var5.h - this.h) / 2);
            if (var2 == 1) {
               var4 = true;
            }
         }
      }

      if (var3 != this.e) {
         if (var1 < 0 || var1 >= var5.h) {
            return 1;
         }

         if (var2 >= 0 && var2 < var5.l) {
            if (var4 && var5.d[var1][var2] == 0) {
               return 64;
            } else {
               return var5.b ? var5.d[var1][var2] : 1;
            }
         } else {
            return 1;
         }
      } else {
         return this.d[var1][var2];
      }
   }

   String a() {
      return n[this.e - 1];
   }
}
