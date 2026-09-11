import java.io.InputStream;

public class g {
   int e;
   int a;
   int h;
   boolean c;
   short d;
   short f;
   short[] g;
   static short[] b = new short[256];

   private g() {
      this.e = this.a = 0;
      this.g = null;
      this.f = -1;
   }

   static g b(String var0) throws Exception {
      InputStream var1 = new Object().getClass().getResourceAsStream(a(var0));
      if (var1 == null) {
         throw new Exception("Image " + var0 + " is null!");
      }

      g var2 = new g();
      var2.e = b(var1);
      var2.a = b(var1);
      var2.h = var2.e;
      int var3 = var1.read() & 0xFF;
      if (var3 != 0) {
         var2.c = true;
      } else {
         var2.c = false;
      }

      var2.d = a(var1);
      int var4 = var1.read() & 0xFF;
      if (var4 > 255) {
         throw new Exception("Too many colors in image " + var0);
      }

      for (int var5 = 0; var5 < var4; var5++) {
         short var6 = a(var1);
         b[var5] = var6;
         if (var2.c && var2.f < 0 && var2.d == var6) {
            var2.f = (short)var5;
         }
      }

      int var10 = var2.e * var2.a;
      var2.g = new short[var10];

      for (int var7 = 0; var7 < var10; var7++) {
         int var8 = var1.read() & 0xFF;
         short var9 = b[var8];
         if (var2.c && var8 == var2.f) {
            var9 = (short)(var9 & -61441);
         } else {
            var9 = (short)(var9 | 61440);
         }

         var2.g[var7] = var9;
      }

      return var2;
   }

   int a() {
      return this.e;
   }

   int b() {
      return this.a;
   }

   private static String a(String var0) {
      return var0.startsWith("/") ? var0 : "/" + var0;
   }

   private static int b(InputStream var0) throws Exception {
      int var5 = 0;
      int var1 = var0.read();
      var5 |= var1 << 24;
      int var2 = var0.read();
      var5 |= var2 << 16;
      int var3 = var0.read();
      var5 |= var3 << 8;
      int var4 = var0.read();
      return var5 | var4;
   }

   private static short a(InputStream var0) throws Exception {
      int var5 = 0;
      int var1 = var0.read();
      var5 |= var1 << 8;
      int var2 = var0.read();
      var5 |= var2;
      var5 &= 65535;
      return (short)var5;
   }
}
