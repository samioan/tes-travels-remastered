import java.util.Enumeration;
import java.util.Hashtable;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.game.Sprite;

public final class g {
   private static Hashtable a = new Hashtable();
   private static Hashtable b = new Hashtable();
   private static byte a = 0;
   private static byte b = 0;

   public static final void a() {
      a.clear();
   }

   private static final int a(byte[] var0, int var1, int[] var2) {
      boolean var3 = false;
      int var4 = (char)((var0[var1++] & 255) << 8) | (char)((var0[var1++] & 255) << 0);

      for (int var9 = 0; var9 < var2.length; var9++) {
         var2[var9] = 0;
      }

      if ((var4 & 512) > 0) {
         var2[0] = (char)(var0[var1++] & 0xFF);
      }

      if ((var4 & 256) > 0) {
         var2[1] = (char)(var0[var1++] & 0xFF) << '\b' | (char)(var0[var1++] & 0xFF) << 0;
      }

      if ((var4 & 128) > 0) {
         var2[2] = (char)(var0[var1++] & 0xFF) << '\b' | (char)(var0[var1++] & 0xFF) << 0;
      }

      if ((var4 & 64) > 0) {
         var2[3] = (char)var0[var1++] & 255;
      }

      if ((var4 & 32) > 0) {
         var2[4] = (char)var0[var1++] & 255;
      }

      if ((var4 & 16) > 0) {
         var2[5] = var0[var1++];
      }

      if ((var4 & 8) > 0) {
         var2[6] = var0[var1++];
      }

      if ((var4 & 4) > 0) {
         var2[7] = var0[var1++];
      }

      if ((var4 & 2) > 0) {
         var2[8] = var0[var1++];
      }

      if ((var4 & 1) > 0) {
         var2[9] = var0[var1++];
      }

      return var1;
   }

   private static final void a(int[] var0, d var1) {
      var1.a = (byte)var0[0];
      var1.a = (short)var0[1];
      var1.b = (short)var0[2];
      var1.c = (short)var0[3];
      var1.d = (short)var0[4];
      var1.b = (byte)var0[5];
      var1.c = (byte)var0[6];
      var1.d = (byte)var0[7];
      var1.e = (byte)var0[8];
   }

   public static final d a(String var0) {
      String var1 = null;
      String var2 = "";
      d var3 = null;
      d var4 = null;
      String[] var5 = new String[255];
      Object[] var6 = new Object[50];
      int[] var7 = new int[10];
      int[] var8 = new int[10];
      int[] var9 = new int[10];
      Object var10 = null;
      int var11 = b.a(var0);
      boolean var12 = false;
      boolean var13 = false;
      int var14 = 0;
      byte var15 = 0;
      char var16 = '\u0000';
      char var17 = '\u0000';
      char var18 = '\u0000';
      char var19 = '\u0000';
      char var20 = '\u0000';
      int var21 = 0;
      boolean var23 = false;
      byte[] var24 = new byte[var11];
      a = 0;
      b = 0;
      System.arraycopy(b.b, 0, var24, 0, var11);
      if ((var19 = (char)(var24[0] & 0xFF)) > 0) {
         var2 = new String(var24, 1, var19);
      }

      var14 = 1 + var19;

      while (var14 != var11) {
         var16 = (char)(var24[var14++] & 0xFF);
         var18 = (char)(var24[var14++] & 0xFF);
         if (!(var1 = new String(var24, var14, var18)).startsWith("/")) {
            var1 = var2 + var1;
         }

         var14 = a(var24, var14 + var18, var7);
         if (var7[0] == 0) {
            var7[0] = var16;
         }

         var5[var7[0]] = new String(var1);
         var10 = new int[(var20 = (char)(var24[var14++] & 0xFF)) * 2];

         for (int var48 = 0; var48 < var20; var48++) {
            ((Object[])var10)[var21++] = (char)(var24[var14++] & 0xFF) << 16 | (char)(var24[var14++] & 0xFF) << '\b' | (char)(var24[var14++] & 0xFF) << 0;
            ((Object[])var10)[var21++] = (char)(var24[var14++] & 0xFF) << 16 | (char)(var24[var14++] & 0xFF) << '\b' | (char)(var24[var14++] & 0xFF) << 0;
         }

         if (var20 > 0) {
            var6[var7[0]] = var10;
         }

         var17 = (char)(var24[var14++] & 0xFF);
         if (!var1.equals("/4.png")) {
            Image var22 = a(var1, var20 > 0 ? (int[])var6[var7[0]] : null);
            if (var17 == 0) {
               if (var3 == null) {
                  var4 = var3 = new d();
               } else {
                  var4.c = new d();
                  var4 = var4.c;
               }

               a(var7, var4);
               var4.a = var1;
               var4.b = var4;
               var4.c = (short)var22.getWidth();
               var4.d = (short)var22.getHeight();
            } else {
               for (int var28 = 0; var28 < var17; var28++) {
                  var14 = a(var24, var14, var8);
                  if (var3 == null) {
                     var4 = var3 = new d();
                  } else {
                     var4.c = new d();
                     var4 = var4.c;
                  }

                  var4.b = var4;
                  var15 = var24[var14++];
                  d var25 = var4;

                  for (int var29 = 0; var29 < var15; var29++) {
                     var14 = a(var24, var14, var9);
                     a(var9, var25);
                     var25.a = (byte)var8[0];
                     var25.d = (byte)var8[7];
                     var25.a = var1;
                     var25.f = 1;
                     if (var29 < var15 - 1) {
                        var25.a = new d();
                        var25 = var25.a;
                     }
                  }
               }
            }
         }
      }

      b.b = null;
      b.c(100);
      return var3;
   }

   private static final Image a(String var0, int[] var1) {
      Image var2;
      if ((var2 = (Image)a.get(var0)) == null) {
         try {
            var2 = Image.createImage(var0);
            a.put(var0, var2);
         } catch (Exception var4) {
            var4.printStackTrace();
            var2 = null;
         }
      }

      b++;
      b.c(++a * 100 / b);
      return var2;
   }

   private static final d a(d var0, int var1) {
      for (d var2 = var0; var2 != null; var2 = var2.c) {
         if (var2.a == var1) {
            return var2;
         }
      }

      return null;
   }

   public static final int a(Graphics var0, d var1, int var2, int var3, int var4) {
      d var5 = a(var1, (byte)var2);
      int var6 = 0;
      int var7 = 0;
      short var8 = 0;
      short var9 = 0;
      String var10 = var5.a;
      Image var11 = (Image)a.get(var10);
      if (var5 != null && var5.b != null) {
         if ((var5 = var5.b).f == 0) {
            var0.drawImage(var11, var3 + var5.b, var4 + var5.c, 0);
         } else {
            var6 = var3 + var5.b;
            var7 = var4 + var5.c;
            if (var6 < b.a && var7 < b.b) {
               var8 = b.a < var5.c ? b.a : var5.c;
               var9 = b.b < var5.d ? b.b : var5.d;
               var0.setClip(var6, var7, var8, var9);
               var0.clipRect(var6, var7, var8, var9);
               if (var5.e == 1) {
                  Sprite var12;
                  if ((var12 = (Sprite)b.get(var10)) == null) {
                     try {
                        (var12 = new Sprite(var11)).setTransform(2);
                        b.put(var10, var12);
                     } catch (Exception var14) {
                        var14.printStackTrace();
                     }
                  }

                  var12.setPosition(var3 + var5.c - var11.getWidth() + var5.a + var5.b, var4 - var5.b + var5.c);
                  var12.paint(var0);
               } else {
                  var0.drawImage(var11, var3 - var5.a + var5.b, var4 - var5.b + var5.c, 0);
               }

               var0.setClip(0, 0, b.a, b.b);
            }
         }

         return var5.c;
      } else {
         return 0;
      }
   }

   public static final int a(d var0, int var1) {
      try {
         return a(var0, (byte)var1).c;
      } catch (Exception var3) {
         return 0;
      }
   }

   public static final int b(d var0, int var1) {
      try {
         return a(var0, (byte)var1).d;
      } catch (RuntimeException var3) {
         return 0;
      }
   }

   public static final boolean a(d var0, int var1) {
      d var2 = a(var0, var1);
      Object var3 = null;
      if (var2 == null) {
         return true;
      }

      var3 = var2.b;
      var2.b = var2.b.a;
      if (var2.b == null) {
         if (var2.d != 1) {
            var2.b = (d)var3;
            return true;
         }

         var2.b = a(var0, var1);
      }

      return false;
   }

   public static final boolean a(d var0, int var1, int var2) {
      d var3;
      d var4 = var3 = a(var0, var1);
      boolean var5 = false;
      if (var3 == null) {
         return true;
      }

      for (int var6 = 0; var6 < var2; var6++) {
         if (var4.a == null) {
            return true;
         }

         var4 = var4.a;
      }

      var3.b = var4;
      return false;
   }

   public static final void a(d var0, int var1) {
      a(var0, var1).b = a(var0, var1);
   }

   public static final void a(String var0) {
      Object var1 = null;
      Enumeration var2 = a.keys();

      while (var2.hasMoreElements()) {
         String var3;
         if ((var3 = (String)var2.nextElement()).startsWith(var0)) {
            a.remove(var3);
            b.clear();
         }
      }

      b.b();
   }

   private static int[] a() {
      int[] var0 = new int[256];
      boolean var1 = false;
      int var2 = 0;

      for (int var4 = 0; var4 < 256; var4++) {
         int var3 = var4;
         var2 = 8;

         while (--var2 >= 0) {
            if ((var3 & 1) != 0) {
               var3 = -306674912 ^ var3 >>> 1;
            } else {
               var3 >>>= 1;
            }
         }

         var0[var4] = var3;
      }

      return var0;
   }

   static {
      a();
   }
}
