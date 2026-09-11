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
   static short j;

   static short b() {
      j++;
      return j;
   }

   byte[] e() {
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

   static void a(d var0, byte[] var1) {
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
   }

   void c() {
      ESGame.monsters[this.n - 1].put(f.b((int)this.o, (int)this.m), this.e());
   }

   String a() {
      return h[this.l - 1];
   }

   int c(int var1) {
      return e[this.l - 1][var1] & 0xFF;
   }

   boolean d() {
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
      i var5 = ESGame.dungeons[this.n - 1];
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

      if (!var5.b(var3, var4)) {
         return false;
      }

      if (this.b(var3, var4)) {
         return false;
      }

      ESGame.monsters[this.n - 1].remove(f.b((int)this.o, (int)this.m));
      var5.d[this.o][this.m] = f.c((byte)2, var5.d[this.o][this.m]);
      var5.d[var3][var4] = f.b((byte)2, var5.d[var3][var4]);
      this.o = var3;
      this.m = var4;
      this.c();
      return true;
   }

   boolean a(int var1, int var2) {
      boolean var3 = false;
      if (this.b == 0) {
         this.c(var1, var2);
         this.b++;
         var3 = true;
      } else if (this.b >= 4) {
         this.b = 0;
      } else {
         this.b++;
      }

      this.f = 0;
      this.c();
      return var3;
   }

   private void c(int var1, int var2) {
      int var7 = Math.abs(var1 - this.o);
      int var8 = Math.abs(var2 - this.m);
      byte var3;
      if (this.o < var1) {
         var3 = 2;
      } else if (this.o > var1) {
         var3 = 4;
      } else {
         var3 = -1;
      }

      byte var4;
      if (this.m < var2) {
         var4 = 3;
      } else if (this.m > var2) {
         var4 = 1;
      } else {
         var4 = -1;
      }

      byte var5;
      byte var6;
      if (var7 > var8) {
         var5 = var3;
         var6 = var4;
      } else if (var7 < var8) {
         var5 = var4;
         var6 = var3;
      } else {
         int var9 = ESGame.nextInt(2);
         if (var9 == 0) {
            var5 = var3;
            var6 = var4;
         } else {
            var5 = var4;
            var6 = var3;
         }
      }

      if (!this.a(var5)) {
         if (!this.a(var6)) {
            ;
         }
      }
   }

   private boolean b(int var1, int var2) {
      i var3 = ESGame.dungeons[this.n - 1];
      if (var3.j != 1 && var3.g != 1) {
         if (var3.j != 3 && var3.g != 3) {
            if (var3.j != 4 && var3.g != 4) {
               if ((var3.j == 2 || var3.g == 2) && var1 == 30 && var2 == 17) {
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

   boolean a(j var1, long var2) {
      boolean var4 = false;
      if (this.f == 0) {
         this.k = var2;
         this.f = 1;
      } else if (this.f == 1 && var2 - this.k > 800L) {
         var4 = true;
      }

      if (!var4) {
         return false;
      }

      this.f = 2;
      this.k = var2;
      byte var5 = e[this.l - 1][4];
      int var6 = var1.b(true);
      int var7 = var6 - var5;
      var7 = Math.min(var7, e[this.l - 1][2]);
      int var8 = e[this.l - 1][3] - var7 * 5;
      int var9 = var1.F() + var7 * 5;
      var8 = Math.min(Math.max(var8, 10), 95);
      var9 = Math.min(Math.max(var9, 10), 95);
      int var10 = f.a(100);
      int var11 = f.a(100);
      boolean var12 = var10 <= var8;
      boolean var13 = var11 <= var9;
      byte var14 = 0;
      if (var12 && !var13) {
         var14 = 3;
      } else if (var12 && var13) {
         if (var10 >= var11) {
            var14 = 2;
         } else {
            var14 = 1;
         }
      } else if (var12 || var13) {
         var14 = 0;
      } else if (var10 >= var11) {
         var14 = 2;
      } else {
         var14 = 1;
      }

      if (var14 == 0) {
         this.f = 1;
         return false;
      }

      byte var15 = e[this.l - 1][5];
      int var16 = var1.t();
      if (var14 == 1) {
         var16 = 2 * var16;
      }

      int var17 = var15 - var16;
      var17 = Math.max(var17, 4);
      int var18 = var17 * var1.E[3] / 100;
      var1.E[2] = (short)(var1.E[2] - var18);
      var1.E[2] = (short)Math.max(var1.E[2], 0);
      if (var13) {
         var1.b(var1.x(), 1);
      }

      if (var14 < 3) {
         this.f = 1;
         return true;
      }

      if (f.a(100) <= 30) {
         byte var19 = e[this.l - 1][11];
         if (var19 > 0) {
            int var20 = var19 - 1;
            var1.r = (byte)(var1.r | 1 << var20);
            if (var19 != 1) {
               if (var19 == 2) {
                  i var21 = ESGame.dungeons[this.n - 1];
                  var21.a(3);
               } else if (var19 != 3) {
                  if (var19 == 4) {
                     var1.ar = 30000;
                  } else if (var19 == 5) {
                     var1.O = 30000;
                  } else if (var19 != 6 && var19 != 7 && var19 == 8) {
                  }
               }
            }
         }
      }

      this.f = 1;
      return true;
   }

   static void f() throws Exception {
      DataInputStream var0 = ESGame.getResource("monstersin.dat");
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

      var0.close();
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

   void a(boolean var1) {
      byte var2 = e[this.l - 1][15];
      if (var1) {
         var2 = 100;
      }

      byte var3 = e[this.l - 1][16];
      int var4 = f.a(100);
      boolean var5 = var4 <= var2;
      if (var5 || var1) {
         i var6 = ESGame.dungeons[this.n - 1];
         byte var7 = var6.c;
         int var8 = a.a(ESGame.r, var7, var3);
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

         var6.b(var11);
      }
   }

   static d a(Random var0, int var1, int var2, int var3) {
      short var4 = b();
      int var5 = var3;
      if (var5 < 0) {
         int var6 = var1 - 1;
         if (var6 < 0) {
            var6 = 0;
         }

         if (var6 > 36) {
            var6 = 36;
         }

         int var7 = ESGame.lingoRandomInt(var0, 10);
         byte var8 = 0;
         if (var7 <= 4) {
            var8 = 0;
         } else if (var7 <= 7) {
            var8 = 1;
         } else if (var7 <= 9) {
            var8 = 2;
         } else {
            var8 = 3;
         }

         var5 = i.m[var6][var8];
      }

      return new d(var4, var5, var2);
   }
}
