import java.io.DataInputStream;
import java.util.Random;

public class a {
   static int n;
   static String[] g;
   static int k;
   static String[] b;
   static byte[] j;
   static byte[] c;
   static byte[] m;
   static short[] f;
   static short[] a;
   static byte[] e;
   static String[] l = new String[]{
      "Warp to camp",
      "Cures ailment",
      "Restores Health",
      "Restores Magicka",
      " ",
      "Grants level experience",
      "Health & Magicka",
      "Increase harm",
      "Increase armor",
      "Safe camping",
      "Kills weak monster",
      "Kills normal monster",
      "Kills strong monster"
   };
   static byte[][] h;
   static byte d;
   static short i;

   a() {
   }

   static short a() {
      i++;
      return i;
   }

   static int d(int var0) {
      return var0 - 1;
   }

   static boolean b(int var0) {
      int var1 = d(var0);
      return e[var1] != -1;
   }

   static int a(int var0) {
      int var1 = d(var0);
      return e[var1];
   }

   static String c(int var0) {
      int var1 = d(var0);
      return b[var1];
   }

   static int a(int var0, int var1) {
      int var2 = d(var1);
      short var3;
      switch (var0) {
         case 1:
            var3 = j[var2];
            break;
         case 2:
            var3 = c[var2];
            break;
         case 3:
            var3 = m[var2];
            break;
         case 4:
            var3 = f[var2];
            break;
         case 5:
            var3 = a[var2];
            break;
         case 6:
            var3 = e[var2];
            break;
         default:
            var3 = -1;
      }

      return var3;
   }

   static void d() throws Exception {
      i = 0;
      b();
      c();
   }

   static void b() throws Exception {
      DataInputStream var0 = ESGame.getResource("itemsin.dat");
      n = var0.readShort();
      g = new String[n];

      for (int var1 = 0; var1 < n; var1++) {
         g[var1] = var0.readUTF();
      }

      k = var0.readShort();
      b = new String[k];
      j = new byte[k];
      c = new byte[k];
      m = new byte[k];
      f = new short[k];
      a = new short[k];
      e = new byte[k];

      for (int var2 = 0; var2 < k; var2++) {
         b[var2] = var0.readUTF();
      }

      for (int var3 = 0; var3 < k; var3++) {
         j[var3] = var0.readByte();
      }

      for (int var4 = 0; var4 < k; var4++) {
         c[var4] = var0.readByte();
      }

      for (int var5 = 0; var5 < k; var5++) {
         m[var5] = var0.readByte();
      }

      for (int var6 = 0; var6 < k; var6++) {
         f[var6] = var0.readShort();
      }

      for (int var7 = 0; var7 < k; var7++) {
         a[var7] = var0.readShort();
      }

      for (int var8 = 0; var8 < k; var8++) {
         e[var8] = var0.readByte();
      }

      var0.close();
   }

   static void c() throws Exception {
      DataInputStream var0 = ESGame.getResource("droppeditemsin.dat");
      short var1 = var0.readShort();
      d = (byte)var1;
      short var2 = var0.readShort();
      h = new byte[var1][var2];

      for (int var3 = 0; var3 < var1; var3++) {
         for (int var4 = 0; var4 < var2; var4++) {
            h[var3][var4] = var0.readByte();
         }
      }

      var0.close();
   }

   static int a(Random var0, int var1) {
      int var2 = -1;
      int var3 = -1;

      for (int var4 = 0; var4 < k; var4++) {
         if (j[var4] == 11 && c[var4] == (byte)var1) {
            if (var2 == -1) {
               var2 = var4;
            }

            var3 = var4;
         }
      }

      int var5 = var3 - var2 + 1;
      int var6 = var2 + Math.abs(var0.nextInt() % var5);
      return 1 + var6;
   }

   static int a(Random var0, int var1, int var2) {
      int var3 = f.a(var0, 100);
      int var4 = var3;

      for (int var5 = 1; var5 < var2; var5++) {
         var3 = f.a(var0, 100);
         if (var3 > var4) {
            var4 = var3;
         }
      }

      var3 = var4;
      byte var6;
      if (var3 <= 64) {
         var6 = 0;
      } else if (var3 <= 75) {
         var6 = 1;
      } else if (var3 <= 90) {
         var6 = 3;
      } else {
         var6 = 4;
      }

      int var7 = f.a(var0, 10);
      var7 += var1 - 2;
      if (var7 > d - 1) {
         var7 = d - 1;
      }

      if (var7 < 0) {
         var7 = 0;
      }

      byte var8 = h[var7][var6];
      int var9 = var8;
      if (var6 == 1) {
         byte var10 = h[var7][2];
         var9 |= var10 << 8;
      }

      return var9;
   }
}
