import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.Random;

public class d {
   private static int d;
   private static String[] h;
   private static byte[][] e;
   short a;
   byte l;
   byte g;
   byte o;
   byte m;
   boolean i;
   byte n;
   byte[] c;
   byte b;
   byte f;
   long k;
   static d p = new d();
   static short j;

   static short b() {
      j++;
      return j;
   }

   byte[] f() {
      byte[] var1 = new byte[28];
      var1[0] = (byte)(this.a >>> 8 & 0xFF);
      var1[1] = (byte)(this.a & 0xFF);
      var1[2] = this.l;
      var1[3] = this.g;
      var1[4] = this.o;
      var1[5] = this.m;
      var1[6] = (byte)(this.i ? 1 : 0);
      var1[7] = this.n;
      var1[8] = this.b;
      var1[9] = this.f;
      var1[10] = (byte)(this.k >>> 56 & 255L);
      var1[11] = (byte)(this.k >>> 48 & 255L);
      var1[12] = (byte)(this.k >>> 40 & 255L);
      var1[13] = (byte)(this.k >>> 32 & 255L);
      var1[14] = (byte)(this.k >>> 24 & 255L);
      var1[15] = (byte)(this.k >>> 16 & 255L);
      var1[16] = (byte)(this.k >>> 8 & 255L);
      var1[17] = (byte)(this.k & 255L);

      for (int var2 = 0; var2 < 10; var2++) {
         var1[18 + var2] = this.c[var2];
      }

      return var1;
   }

   public d() {
      this.c = new byte[10];
      this.i = false;
   }

   public d(int var1, int var2, int var3) {
      this.a = (short)var1;
      this.l = (byte)var2;
      this.g = e[this.l - 1][14];
      this.c = new byte[10];
      this.i = false;
      this.n = (byte)var3;
      this.f = 0;
   }

   static d a(byte[] var0) {
      short var1 = (short)(var0[0] & 0xFF);
      short var2 = (short)(var0[1] & 0xFF);
      p.a = (short)(var1 << 8 | var2);
      p.l = var0[2];
      p.g = var0[3];
      p.o = var0[4];
      p.m = var0[5];
      p.i = var0[6] != 0;
      p.n = var0[7];
      p.b = var0[8];
      p.f = var0[9];
      p.k = f.a(var0, 10);

      for (int var3 = 0; var3 < 10; var3++) {
         p.c[var3] = var0[18 + var3];
      }

      return p;
   }

   static d a(d var0, byte[] var1) {
      short var2 = (short)(var1[0] & 0xFF);
      short var3 = (short)(var1[1] & 0xFF);
      var0.a = (short)(var2 << 8 | var3);
      var0.l = var1[2];
      var0.g = var1[3];
      var0.o = var1[4];
      var0.m = var1[5];
      var0.i = var1[6] != 0;
      var0.n = var1[7];
      var0.b = var1[8];
      var0.f = var1[9];
      var0.k = f.a(var1, 10);

      for (int var4 = 0; var4 < 10; var4++) {
         var0.c[var4] = var1[18 + var4];
      }

      return var0;
   }

   void d() {
      ESGame.G[this.n - 1].put(String.valueOf(this.a), this.f());
   }

   String a() {
      return h[this.l - 1];
   }

   int c(int var1) {
      return e[this.l - 1][var1] & 0xFF;
   }

   boolean e() {
      return this.l >= 6 && this.l <= 8;
   }

   void b(int var1) {
      int var2 = this.g & 255;
      if (var1 > var2) {
         var1 = var2;
      }

      var2 -= var1;
      this.g = (byte)var2;
   }

   boolean a(int var1) {
      byte var2 = 1;
      byte var3 = this.o;
      byte var4 = this.m;
      i var5 = ESGame.u[this.n - 1];
      switch (var1) {
         case 1:
            var2 = -1;
         case 3:
            var3 = this.o;
            var4 = (byte)(this.m + var2);
            break;
         case 4:
            var2 = -1;
         case 2:
            var4 = this.m;
            var3 = (byte)(this.o + var2);
            break;
         default:
            return false;
      }

      if (var3 < 0 || var4 < 0) {
         return false;
      }

      if (var3 >= var5.g || var4 >= var5.y) {
         return false;
      }

      if (this.a(var3, var4)) {
         return false;
      }

      if (!var5.d(var3, var4)) {
         return false;
      }

      var5.w[this.o][this.m] = f.c((byte)2, var5.w[this.o][this.m]);
      var5.w[var3][var4] = f.b((byte)2, var5.w[var3][var4]);
      this.o = var3;
      this.m = var4;
      return true;
   }

   void a(j var1) {
      if (this.d(var1)) {
         if (this.b == 0) {
            this.e(var1);
            this.b++;
         } else if (this.b >= 4) {
            this.b = 0;
         } else {
            this.b++;
         }
      }
   }

   private void e(j var1) {
      int var6 = Math.abs(var1.l - this.o);
      int var7 = Math.abs(var1.k - this.m);
      byte var2;
      if (this.o < var1.l) {
         var2 = 2;
      } else if (this.o > var1.l) {
         var2 = 4;
      } else {
         var2 = -1;
      }

      byte var3;
      if (this.m < var1.k) {
         var3 = 3;
      } else if (this.m > var1.k) {
         var3 = 1;
      } else {
         var3 = -1;
      }

      byte var4;
      byte var5;
      if (var6 > var7) {
         var4 = var2;
         var5 = var3;
      } else if (var6 < var7) {
         var4 = var3;
         var5 = var2;
      } else {
         int var8 = ESGame.h(2);
         if (var8 == 0) {
            var4 = var2;
            var5 = var3;
         } else {
            var4 = var3;
            var5 = var2;
         }
      }

      if (!this.a(var4)) {
         if (!this.a(var5)) {
            ;
         }
      }
   }

   private boolean a(int var1, int var2) {
      i var3 = ESGame.u[this.n - 1];
      if (var3.s != 1 && var3.v != 1) {
         if (var3.s != 3 && var3.v != 3) {
            if (var3.s != 4 && var3.v != 4) {
               if ((var3.s == 2 || var3.v == 2) && var1 == 30 && var2 == 17) {
                  return true;
               }
            } else if (var1 == 5 && var2 == 17) {
               return true;
            }
         } else if (var1 == 17 && var2 == 30) {
            return true;
         }
      } else if (var1 == 17 && var2 == 5) {
         return true;
      }

      return false;
   }

   boolean d(j var1) {
      return this.c(var1) <= 3;
   }

   int c(j var1) {
      int var2 = Math.abs(var1.l - this.o);
      int var3 = Math.abs(var1.k - this.m);
      return var2 + var3;
   }

   boolean b(j var1) {
      if (this.c(var1) == 1) {
         return true;
      }

      this.f = 0;
      return false;
   }

   void a(j var1, long var2) {
      this.f = 2;
      this.k = var2;
      byte var4 = e[this.l - 1][4];
      int var5 = var1.f(true);
      int var6 = var5 - var4;
      var6 = Math.min(var6, e[this.l - 1][2]);
      int var7 = e[this.l - 1][3] - var6 * 5;
      int var8 = var1.I() + var6 * 5;
      var7 = Math.min(Math.max(var7, 10), 95);
      var8 = Math.min(Math.max(var8, 10), 95);
      int var9 = f.a(100);
      int var10 = f.a(100);
      boolean var11 = var9 <= var7;
      boolean var12 = var10 <= var8;
      byte var13 = 0;
      if (var11 && !var12) {
         var13 = 3;
      } else if (var11 && var12) {
         if (var9 >= var10) {
            var13 = 2;
         } else {
            var13 = 1;
         }
      } else if (var11 || var12) {
         var13 = 0;
      } else if (var9 >= var10) {
         var13 = 2;
      } else {
         var13 = 1;
      }

      if (var13 == 0) {
         this.f = 1;
      } else {
         byte var14 = e[this.l - 1][5];
         int var15 = var1.v();
         if (var13 == 1) {
            var15 = 2 * var15;
         }

         int var16 = var14 - var15;
         var16 = Math.max(var16, 4);
         int var17 = var16 * var1.U[3] / 100;
         var1.U[2] = (short)(var1.U[2] - var17);
         var1.U[2] = (short)Math.max(var1.U[2], 0);
         if (var12) {
            var1.a(var1.y(), 1);
         }

         if (var13 < 3) {
            this.f = 1;
         } else {
            if (f.a(100) <= 30) {
               byte var18 = e[this.l - 1][11];
               if (var18 > 0) {
                  int var19 = var18 - 1;
                  var1.A = (byte)(var1.A | 1 << var19);
                  if (var18 != 1) {
                     if (var18 == 2) {
                        i var20 = ESGame.u[this.n - 1];
                        var20.c(3);
                     } else if (var18 != 3) {
                        if (var18 == 4) {
                           var1.ah = 30000;
                        } else if (var18 == 5) {
                           var1.F = 30000;
                        } else if (var18 != 6 && var18 != 7 && var18 == 8) {
                        }
                     }
                  }
               }
            }

            this.f = 1;
         }
      }
   }

   static void g() throws Exception {
      DataInputStream var0 = f.a("/monstersin.dat");
      j = 0;
      d = var0.readInt();
      h = new String[d];
      e = new byte[d][17];

      for (int var1 = 0; var1 < d; var1++) {
         h[var1] = var0.readUTF();
      }

      for (int var2 = 0; var2 < d; var2++) {
         for (int var3 = 0; var3 < 17; var3++) {
            e[var2][var3] = var0.readByte();
         }
      }
   }

   static d a(DataInputStream var0) throws Exception {
      d var1 = new d();
      var1.a = var0.readShort();
      var1.l = var0.readByte();
      var1.g = var0.readByte();
      var1.o = var0.readByte();
      var1.m = var0.readByte();
      var1.i = var0.readBoolean();
      var1.n = var0.readByte();
      var1.b = var0.readByte();
      var1.f = var0.readByte();
      var1.k = var0.readLong();

      for (int var2 = 0; var2 < 10; var2++) {
         var1.c[var2] = var0.readByte();
      }

      return var1;
   }

   void a(DataOutputStream var1) throws Exception {
      var1.writeShort(this.a);
      var1.writeByte(this.l);
      var1.writeByte(this.g);
      var1.writeByte(this.o);
      var1.writeByte(this.m);
      var1.writeBoolean(this.i);
      var1.writeByte(this.n);
      var1.writeByte(this.b);
      var1.writeByte(this.f);
      var1.writeLong(this.k);

      for (int var2 = 0; var2 < 10; var2++) {
         var1.writeByte(this.c[var2]);
      }
   }

   void c() {
   }

   void a(boolean var1) {
      byte var2 = e[this.l - 1][15];
      if (var1) {
         var2 = 100;
      }

      byte var3 = e[this.l - 1][16];
      int var4 = f.a(100);
      boolean var5 = var4 <= var2;
      if (var5 || var1) {
         i var6 = ESGame.u[this.n - 1];
         byte var7 = var6.a;
         int var8 = a.a(ESGame.P, var7, var3);
         byte var9 = (byte)(var8 & 0xFF);
         byte var10 = 0;
         if (var9 == 86) {
            var10 = (byte)(var8 >>> 8 & 0xFF);
         }

         byte[] var11 = new byte[]{this.o, this.m, var9, 0, 0, var10, 0};
         short var12 = a.a();
         var9 = (byte)(var12 >>> 8 & 0xFF);
         var10 = (byte)(var12 & 0xFF);
         var11[3] = var9;
         var11[4] = var10;
         var11[6] = 1;
         if (var1) {
            var11[6] = (byte)(var11[6] | 4);
         }

         var6.c(var11);
      }
   }

   static d a(i var0) {
      return a(ESGame.P, var0, -1);
   }

   static d a(Random var0, i var1, int var2) {
      short var3 = b();
      int var4 = var2;
      if (var4 < 0) {
         int var5 = var1.a - 1;
         if (var5 < 0) {
            var5 = 0;
         }

         if (var5 > 36) {
            var5 = 36;
         }

         int var6 = ESGame.a(var0, 10);
         byte var7 = 0;
         if (var6 <= 4) {
            var7 = 0;
         } else if (var6 <= 7) {
            var7 = 1;
         } else if (var6 <= 9) {
            var7 = 2;
         } else {
            var7 = 3;
         }

         var4 = i.l[var5][var7];
      }

      return new d(var3, var4, var1.o);
   }
}
