import blt.Main;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Random;
import java.util.Vector;
import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;
import javax.microedition.rms.RecordStore;
import javax.microedition.rms.RecordStoreNotFoundException;

public final class b extends Canvas implements Runnable {
   private boolean h = false;
   private static byte i = 32;
   private static byte j = 16;
   public static byte a = 22;
   public static byte b = 21;
   public static byte c = 23;
   public static byte d = -104;
   public static byte e = -105;
   public static final Random a = new Random();
   private static byte[] e = new byte[8];
   private static byte[] f = new byte[]{55, 57, 51};
   private static byte[] g;
   private static byte k = -1;
   private static byte l = 0;
   private static int d = 0;
   private static int e = 0;
   private static int f = 0;
   private static int g = 0;
   private static int h = 0;
   private static int i = 0;
   private static int j = 0;
   private static int k = 0;
   private static int l = -1;
   public static int a = -286331154;
   private static boolean i = true;
   public static short a = 0;
   public static short b = 0;
   public static byte f = 0;
   public static byte g = 0;
   public static boolean a = true;
   private static byte m = -1;
   private static byte n = -1;
   public static byte[] a = null;
   public static byte[] b = null;
   public static boolean b = true;
   private static boolean j = false;
   private static boolean k = false;
   public static boolean c = true;
   public static j[] a = new j[25];
   public static b a = null;
   public static e a = null;
   public static Vector a = new Vector();
   private static String[][] a = (String[][])null;
   private static String[] a = null;
   private static String[] b = new String[100];
   private static String b = null;
   private static String c = null;
   private byte[] h = new byte[]{0, 0};
   private byte[] i = new byte[]{0, 0};
   private byte o = -1;
   private byte p = 0;
   private byte q = 0;
   private byte r = 0;
   private int[] i = new int[]{0, 0};
   private int[] j = new int[]{0, 0};
   private int[] k = new int[]{0, 0};
   private int[] l = new int[]{-12345, -12345};
   private short[] a = null;
   private byte[] j = null;
   private byte[] k = null;
   public static byte[] c = null;
   private int[] m = new int[50];
   private byte s = 0;
   private byte t = 0;
   private byte u = 0;
   public static int b = 100;
   private short c = 0;
   private short d = 0;
   private short e = 128;
   private short f = 0;
   private int m = 0;
   private int n = 0;
   private int o = 0;
   private boolean l = true;
   private boolean m = true;
   private boolean n = true;
   private boolean o = false;
   private long a = 0L;
   private long b = 0L;
   private d a = null;
   private d b = null;
   private f a = null;
   private j a = null;
   private Graphics a = null;
   private Image a = null;
   private Main a = null;
   private static String d = null;
   private String e = null;
   public static String a = null;
   private static int[] n = new int[255];
   private static int p = 0;
   private int q = 0;
   private Vector[] a = null;
   private short g = 0;
   private short h = 0;
   private boolean p = false;
   private static int[] o = new int[2];
   public static int c = 0;
   private static short i = 0;
   private static short j = 0;
   public static Font a = Font.getFont(0, 0, 8);
   public static Font b = Font.getFont(0, 0, 0);
   public static Font c = Font.getFont(0, 1, 8);
   public static Font d = Font.getFont(0, 1, 16);
   public boolean d = false;
   private static d c = null;
   private short k = 0;
   public static byte[] d = new byte[75];
   public static byte h = 0;
   public static boolean e = true;
   private static String[][] b = (String[][])null;
   private static String[][] c = (String[][])null;
   private static String[][] d = (String[][])null;
   private static String[][] e = (String[][])null;
   private static String[][] f = (String[][])null;
   private static String[][] g = (String[][])null;
   private static String[][] h = (String[][])null;
   private static byte v = 0;
   private static byte w = 0;
   private static short l = 0;
   private static boolean q = false;
   private static byte x = 0;
   private static c a = null;
   private static c b = null;
   private static Image b = null;
   private static Image c = null;
   private static char[] a = null;
   private static short[] b = null;
   private static char[] b = null;
   private static short[] c = null;
   public static boolean f = false;
   public int[] a = new int[]{0, 0};
   public int[] b = new int[]{0, 0};
   public int[] c = new int[]{0, 0};
   public int[] d = new int[]{0, 0};
   public int[] e = new int[]{0, 0};
   public int[] f = new int[]{0, 0};
   public int[] g = new int[]{0, 0};
   public int[] h = new int[]{0, 0};
   private boolean r = false;
   public boolean g = false;
   private Vector b = new Vector();
   private int r = 0;
   private int s = 12;
   private int t = 7;
   private int u = 0;
   private int v = 0;
   private int w = 0;
   private long c = 0L;
   private static String f = null;

   public b(Main var1, String var2, String var3, String var4) {
      this.setFullScreenMode(true);
      this.a = var1;
      d = var4;
      a = this;
      j();
      a = new e(this);
      this.a(this.getWidth(), this.getHeight() + 25);
      g.a();
      this.g = (short)(b - (c.getHeight() << 3));
      this.h = 0;
      h = 0;
      this.a = new f(var3, this);
      c = g.a("/oh_pc.cml");
      this.a(var2);
      this.b(false);
   }

   private static final void j() {
      boolean var0 = false;
      f[0] = 55;
      f[1] = 57;
      f[2] = 51;
      k = -1;
      d = 0;
      e = 0;
      f = 0;
      l = 0;
      g = -1;
      h = -1;
      i = 0;
      j = 0;
      k = 0;
      l = -1;
      b = 100;
      a = 0;
      b = 0;
      f = 0;
      g = 0;
      a = null;
      b = null;
      j = false;
      k = false;
      a = null;
      a = (String[][])null;
      a = null;
      b = null;
      c = null;

      for (int var1 = 0; var1 < e.length; var1++) {
         e[var1] = 0;
      }

      for (int var2 = 0; var2 < g.length; var2++) {
         g[var2] = 0;
      }

      for (int var3 = 0; var3 < a.length; var3++) {
         a[var3] = null;
      }

      for (int var4 = 0; var4 < b.length; var4++) {
         b[var4] = null;
      }

      a.removeAllElements();
   }

   private void k() {
      b = (String[][])null;
      c = (String[][])null;
      d = (String[][])null;
      e = (String[][])null;
      f = (String[][])null;
      g = (String[][])null;
      h = (String[][])null;
      System.gc();
   }

   private final void l() {
      int var1 = 0;
      boolean var2 = false;
      a = new String[][]{
         null,
         null,
         {"Level 1", "Level 2", "Level 3", "Level 4", "Level 5", "Level 6", "Level 7", "Level 8", "Level 9", "Level 10", "Level 11", "Level 12"},
         {
               "/l01_1.scr",
               "/l02_2_1.scr",
               "/l03_3.scr",
               "/l04_4.scr",
               "/l05_5.scr",
               "/l06_6_cr.scr",
               "/l07_7_cr.scr",
               "/l08_8_cr.scr",
               "/l09_9_cr.scr",
               "/l10_10_cr.scr",
               "/l11_11_cr.scr",
               "/l12_12.scr"
         },
         {a(18), a(19), a(20)},
         null,
         {a(457), a(458), a(573), a(522), a(459), a(460), a(461), a(462)}
      };
      a[0] = new String[this.b() ? 5 : 4];
      a[5] = new String[this.b() ? 6 : 5];
      int var3 = 0;
      String[] var10000 = a[0];
      var3++;
      var10000[0] = a(2);
      if (this.b()) {
         var10000 = a[0];
         var3++;
         var10000[1] = a(3);
      }

      a[0][var3++] = a(456);
      a[0][var3++] = a(6);
      a[0][var3] = a(22);
      var3 = 0;
      var10000 = a[5];
      var3++;
      var10000[0] = a(21);
      var10000 = a[5];
      var3++;
      var10000[1] = a(2);
      if (this.b()) {
         var10000 = a[5];
         var3++;
         var10000[2] = a(3);
      }

      a[5][var3++] = a(456);
      a[5][var3++] = a(6);
      a[5][var3] = a(22);

      for (int var5 = 0; var5 < a.h.length; var5++) {
         if (a.h[var5][0] > 0) {
            var1++;
         }
      }

      a[1] = new String[var1];
      var1 = 0;

      for (int var6 = 0; var6 < a.h.length; var6++) {
         if (a.h[var6][0] > 0) {
            a[1][var1++] = a.a(a.h[var6][1]);
         }
      }

      a = new String[]{a(292), a(293), a(463), a(294)};
      b[1] = a(281);
      b[6] = a(282);
      b[2] = a(283);
      b[5] = a(284);
      b[8] = a(285);
      b[48] = "# 0";
      b[49] = "# 1";
      b[50] = "# 2";
      b[51] = "# 3";
      b[52] = "# 4";
      b[53] = "# 5";
      b[54] = "# 6";
      b[55] = "# 7";
      b[56] = "# 8";
      b[57] = "# 9";
      b[35] = a(286);
      b[42] = a(287);
   }

   public final void a(String var1) {
      boolean var2 = false;
      b();
      a((byte)6);
      c = var1;
      a = null;
      this.j = null;
      this.k = null;
      c = null;
      this.a = null;
      this.l = true;
      this.m = true;
      c = true;
      this.o = 0;
      this.r = -1;
      this.i[0] = 0;
      this.i[1] = 0;
      this.l[0] = -12345;
      this.l[1] = -12345;
      this.j[0] = 0;
      this.j[1] = 0;
      this.k[0] = 0;
      this.k[1] = 0;
      k = -1;
      a = -286331154;
      this.p = 0;
      this.g = (short)(b - (c.getHeight() << 3));
      this.h = 0;
      this.g = false;
      i.a();
      if (this.a != null) {
         this.a.c = null;
      }

      for (int var3 = 0; var3 < 25; var3++) {
         a[var3] = null;
      }

      b();
      a.a(var1);
   }

   public final void a(int var1, int var2) {
      a = (short)var1;
      b = (short)var2;
      this.c = (short)(a >> 1);
      this.d = (short)(b >> 1);
      this.a = Image.createImage(a, b);
      this.a = this.a.getGraphics();
   }

   private final void m() {
      int[] var1 = new int[]{0, 0};
      int[] var2 = new int[]{0, 0};
      int var3 = i >> 1;
      boolean var4 = false;
      boolean var5 = false;
      this.n = true;

      for (int var6 = 0; var6 < f; var6++) {
         for (int var8 = 0; var8 < g; var8++) {
            a(var1, var2);
            this.a[var6 * g + 0 * this.f + var8] = (short)(var2[0] - var3);
            this.a[var6 * g + 1 * this.f + var8] = (short)var2[1];
            var1[1] += this.e;
         }

         var1[0] += this.e;
         var1[1] = 0;
      }

      for (int var7 = 1; var7 < 25; var7++) {
         a[var7] = null;
      }

      if (a[0] != null) {
         h.a(a[0]);
         h.b(a[0]);
      }

      b();
   }

   private final void a(byte[] var1, int var2, int var3, int var4) {
      int var5;
      if ((var5 = var2 * g + var3) < a.length && var5 >= 0) {
         var1[var5] = (byte)var4;
         a[var5] = 0;
      }
   }

   private final void a(byte[] var1, int[] var2, int[] var3, int var4, int var5, int[] var6) {
      boolean var7 = false;
      boolean var8 = false;
      int[] var9 = new int[]{var2[0], var2[1]};
      int var10 = var5;
      boolean var11 = false;
      int var12 = 0;
      int var13 = 0;

      while (!var8) {
         if (a.nextInt() % var6[16] == 0 && ++this.s < var6[15]) {
            while ((var12 = Math.abs(a.nextInt()) % 4 + 1) == var10) {
            }

            var13 = Math.abs(a.nextInt() % Math.min(f, g));
            if (var12 == 2) {
               this.a(var1, var9, new int[]{var13, 3}, var4, var12, var6);
            } else if (var12 == 1) {
               this.a(var1, var9, new int[]{var13, g - 3}, var4, var12, var6);
            } else if (var12 == 3) {
               this.a(var1, var9, new int[]{f - 3, var13}, var4, var12, var6);
            } else if (var12 == 4) {
               this.a(var1, var9, new int[]{3, var13}, var4, var12, var6);
            }
         }

         if (var10 != 1 && var10 != 2) {
            if (var10 == 3 || var10 == 4) {
               var7 = a.nextInt() % var6[19] == 0;
               this.u++;

               for (int var17 = 0; var17 < var4; var17++) {
                  this.a(var1, var9[0], var9[1] + var17, var6[4]);
               }

               if (var7 && this.t < 50 && this.u > 10) {
                  this.m[this.t++] = var9[0];
                  this.m[this.t++] = var9[1] + 1;
               }
            }
         } else {
            var7 = a.nextInt() % var6[19] == 0;
            this.u++;

            for (int var16 = 0; var16 < var4; var16++) {
               this.a(var1, var9[0] + var16, var9[1], var6[4]);
            }

            if (var7 && this.t < 50 && this.u > 10) {
               this.m[this.t++] = var9[0] + 1;
               this.m[this.t++] = var9[1];
            }
         }

         while (true) {
            var12 = Math.abs(a.nextInt() % 4);
            if (var3[0] < var9[0]) {
               if (var12 == 1) {
                  var10 = 2;
               }

               if (var12 == 2) {
                  var10 = 1;
               }

               if (var12 == 3) {
                  var10 = 4;
               }
            } else if (var3[0] > var9[0]) {
               if (var12 == 1) {
                  var10 = 2;
               }

               if (var12 == 2) {
                  var10 = 1;
               }

               if (var12 == 3) {
                  var10 = 3;
               }
            } else if (var12 < 2) {
               var10 = 2;
            } else {
               var10 = 1;
            }

            if (var10 == 1 && var9[1] < var3[1]) {
               var9[1]++;
               break;
            }

            if (var10 == 3 && var9[0] < var3[0]) {
               var9[0]++;
               break;
            }

            if (var10 == 2 && var9[1] > var3[1]) {
               var9[1]--;
               break;
            }

            if (var10 == 4 && var9[0] > var3[0]) {
               var9[0]--;
               break;
            }
         }

         var8 = var9[0] == var3[0] && var9[1] == var3[1];
      }

      if (var10 != 1 && var10 != 2) {
         if (var10 == 3 || var10 == 4) {
            this.u++;

            for (int var19 = 0; var19 < var4; var19++) {
               this.a(var1, var9[0], var9[1] + var19, var6[4]);
            }
         }
      } else {
         this.u++;

         for (int var18 = 0; var18 < var4; var18++) {
            this.a(var1, var9[0] + var18, var9[1], var6[4]);
         }
      }
   }

   public final void a(byte[] var1, byte[] var2, int[] var3) {
      boolean var4 = false;
      boolean var5 = false;

      for (int var6 = 0; var6 < f; var6++) {
         for (int var7 = 0; var7 < g; var7++) {
            if (var6 * g + var7 <= var1.length - 1
               && var6 * g + var7 >= 0
               && var6 * g + (var7 - 1) <= var1.length - 1
               && var6 * g + (var7 - 1) >= 0
               && var6 * g + var7 + 1 <= var1.length - 1
               && var6 * g + var7 + 1 >= 0
               && (var6 - 1) * g + var7 <= var1.length - 1
               && (var6 - 1) * g + var7 >= 0
               && (var6 + 1) * g + var7 <= var1.length - 1
               && (var6 + 1) * g + var7 >= 0
               && var1[var6 * g + var7] == var3[4]) {
               if (var1[var6 * g + (var7 - 1)] == var3[3]) {
                  if (var1[(var6 - 1) * g + var7] == var3[3]) {
                     var2[var6 * g + var7] = (byte)var3[9];
                  } else if (var1[(var6 + 1) * g + var7] == var3[3]) {
                     var2[var6 * g + var7] = (byte)var3[10];
                  } else {
                     var2[var6 * g + var7] = (byte)var3[5];
                  }
               } else if (var1[var6 * g + var7 + 1] == var3[3]) {
                  if (var1[(var6 - 1) * g + var7] == var3[3]) {
                     var2[var6 * g + var7] = (byte)var3[11];
                  } else if (var1[(var6 + 1) * g + var7] == var3[3]) {
                     var2[var6 * g + var7] = (byte)var3[12];
                  } else {
                     var2[var6 * g + var7] = (byte)var3[6];
                  }
               } else if (var1[(var6 + 1) * g + var7] == var3[3]) {
                  var2[var6 * g + var7] = (byte)var3[7];
               } else if (var1[(var6 - 1) * g + var7] == var3[3]) {
                  var2[var6 * g + var7] = (byte)var3[8];
               }
            }
         }
      }
   }

   private final void a(byte[] var1, int[] var2, int[] var3, int[] var4) {
      int var5 = var1.length;
      byte var6 = g;
      boolean var7 = false;
      boolean var8 = false;
      boolean var9 = false;
      byte[][] var10;
      int var11 = (var10 = new byte[][]{{-1, 1}, {-1, 0}, {-1, -1}, {0, 1}, {0, 0}, {0, -1}, {1, 1}, {1, 0}, {1, -1}}).length;

      for (int var14 = 0; var14 < var11; var14++) {
         int var12 = var2[0] + var10[var14][0];
         int var13 = var2[1] + var10[var14][1];
         if (var12 * var6 + var13 < var5 && var12 * var6 + var13 >= 0) {
            var1[var12 * var6 + var13] = (byte)var4[4];
            a[var12 * var6 + var13] = 0;
         }
      }

      for (int var15 = 0; var15 < var11; var15++) {
         int var16 = var3[0] + var10[var15][0];
         int var17 = var3[1] + var10[var15][1];
         if (var16 * var6 + var17 < var5 && var16 * var6 + var17 >= 0) {
            var1[var16 * var6 + var17] = (byte)var4[4];
         }
      }
   }

   public final void a(int[] var1, int[] var2, int var3, int var4) {
      byte[] var5 = null;
      int var6 = 0;
      boolean var7 = false;
      boolean var8 = false;
      byte var9 = 0;
      int[] var10 = null;
      int[] var11 = null;
      a = null;
      this.j = null;
      this.k = null;
      c = null;
      this.a = null;
      b();

      for (int var16 = 1; var16 < a.length; var16++) {
         a[var16] = null;
      }

      if (this.a != null) {
         this.a.c = null;
      }

      this.s = 0;
      this.t = 0;
      this.u = 0;
      h = 0;
      a.removeAllElements();
      f = (byte)var1[1];
      g = (byte)var1[2];
      this.f = (short)(f * g);
      a = new byte[this.f];
      this.j = new byte[this.f];
      this.k = new byte[this.f];
      c = new byte[this.f];
      this.a = new short[this.f * 2];

      for (int var17 = 0; var17 < f; var17++) {
         for (int var20 = 0; var20 < g; var20++) {
            this.j[var17 * g + var20] = -1;
            this.k[var17 * g + var20] = -1;
            c[var17 * g + var20] = -1;
         }
      }

      var5 = new byte[this.f];
      a.addElement(var5);

      for (int var18 = 0; var18 < f; var18++) {
         for (int var21 = 0; var21 < g; var21++) {
            var5[var18 * g + var21] = (byte)var1[3];
            a[var18 * g + var21] = 1;
         }
      }

      var10 = new int[]{2, 2};
      var11 = new int[]{f - 2, g - 2};
      this.a(var5, var10, var11, var1[14], 1, var1);
      this.a(var5, var10, var11, var1);
      byte[] var12 = var5;
      var5 = new byte[this.f];
      a.addElement(var5);
      this.a(var12, var5, var1);
      var5 = new byte[this.f];
      a.addElement(var5);
      var5[var10[0] * g + var10[1]] = (byte)var1[13];
      var5[var11[0] * g + var11[1]] = (byte)var1[13];
      this.j[var11[0] * g + 0 * this.f + var11[1]] = (byte)var4;
      this.k[var10[0] * g + var10[1]] = -2;
      this.j[var10[0] * g + var10[1]] = -2;
      c[var10[0] * g + var10[1]] = (byte)var3;
      this.m();

      for (byte var19 = 0; var19 < this.t && var19 < var1[18]; var19 += 2) {
         if (var2[var6] != 0) {
            this.a(var2[var6], false, this.m[var19], this.m[var19 + 1]);
            var6++;
         }

         var9 = 2;

         while (a[var9] != null) {
            var9++;
         }

         this.a(null, a.a(a.a[var1[17]][1]), var9, this.m[var19] << 7, this.m[var19 + 1] << 7, a.a[var1[17]]);
      }
   }

   public final void a() {
      a.removeAllElements();
      this.n = true;
      if (a[0] != null) {
         h.a(a[0]);
         h.b(a[0]);
      }
   }

   public final void b(String var1) throws Exception {
      a = null;
      this.j = null;
      this.k = null;
      c = null;
      this.a = null;
      b();
      Object var2 = null;
      char var3 = '\u0000';
      boolean var4 = false;
      boolean var5 = false;
      int var6 = 0;
      int var7 = 0;
      int var8 = 0;
      int var9 = 0;
      int var10 = a(var1);
      if (this.a != null) {
         this.a.c = null;
      }

      a.removeAllElements();
      i.a();
      var7++;
      f = b[0];
      var7++;
      g = b[1];
      this.f = (short)(f * g);
      h = 0;
      a = new byte[this.f];
      this.j = new byte[this.f];
      this.k = new byte[this.f];
      c = new byte[this.f];
      this.a = new short[this.f * 2];

      for (int var14 = 0; var14 < f; var14++) {
         for (int var17 = 0; var17 < g; var17++) {
            this.j[var14 * g + var17] = -1;
            this.k[var14 * g + var17] = -1;
            c[var14 * g + var17] = -1;
         }
      }

      var9 = 0;
      var8 = -1;
      var6 = -1;
      var3 = '\u0000';

      for (int var18 = 0; var18 < g; var18++) {
         for (int var15 = 0; var15 < f; var15++) {
            if (var8 == -1) {
               var8 = (char)(b[var7++] & 0xFF);
            }

            if (var8 == 255) {
               if (var9 == 0) {
                  var3 = (char)(b[var7++] & 0xFF);
                  var6 = (char)(b[var7++] & 0xFF);
               }

               if (++var9 < var3) {
                  a[var15 * g + var18] = (byte)var6;
               } else {
                  a[var15 * g + var18] = (byte)var6;
                  var8 = -1;
                  var9 = 0;
               }
            } else {
               a[var15 * g + var18] = (byte)var8;
               var8 = -1;
               var9 = 0;
            }
         }
      }

      while (var7 < var10) {
         var2 = new byte[this.f];
         a.addElement(var2);
         var9 = 0;
         var8 = -1;
         var6 = -1;
         var3 = '\u0000';

         for (int var19 = 0; var19 < g; var19++) {
            for (int var16 = 0; var16 < f; var16++) {
               if (var8 == -1) {
                  var8 = (char)(b[var7++] & 0xFF);
               }

               if (var8 == 255) {
                  if (var9 == 0) {
                     var3 = (char)(b[var7++] & 0xFF);
                     var6 = (char)(b[var7++] & 0xFF);
                  }

                  if (++var9 < var3) {
                     ((Object[])var2)[var16 * g + var19] = (byte)var6;
                  } else {
                     ((Object[])var2)[var16 * g + var19] = (byte)var6;
                     var8 = -1;
                     var9 = 0;
                  }
               } else {
                  ((Object[])var2)[var16 * g + var19] = (byte)var8;
                  var8 = -1;
                  var9 = 0;
               }
            }
         }
      }

      b = null;
      this.m();
   }

   private final void b(Graphics var1) {
      Object var2 = null;
      int var3 = 0;
      int var4 = 0;
      int var5 = 0;
      boolean var6 = false;
      boolean var7 = false;
      boolean var8 = false;
      if (this.n) {
         this.n = false;
         this.a.setColor(c);
         this.a.fillRect(0, 0, a, b);

         for (int var14 = 0; var14 < a.size() - 1; var14++) {
            var2 = (byte[])a.elementAt(var14);

            for (int var12 = this.h[0]; var12 <= this.i[0] && var12 < f; var12++) {
               for (int var13 = this.h[1]; var13 <= this.i[1] && var13 < g; var13++) {
                  if (var12 >= 0 && var13 >= 0) {
                     var4 = this.a[var12 * g + 0 * this.f + var13] + this.i[0];
                     var5 = this.a[var12 * g + 1 * this.f + var13] + this.i[1];
                     if (((Object[])var2)[var12 * g + var13] != 0) {
                        var3 = g.b(this.a, (int)((Object[])var2)[var12 * g + var13]);
                     }

                     if (var4 > -i && var4 < a && var5 > -j && var5 < b + var3 && ((Object[])var2)[var12 * g + var13] != 0) {
                        g.a(this.a, this.a, (int)((Object[])var2)[var12 * g + var13], var4, var5);
                     }
                  }
               }
            }
         }

         this.b = System.currentTimeMillis();
      }
   }

   public final void paint(Graphics var1) {
      Object var2 = null;
      int var3 = 0;
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      boolean var7 = false;
      boolean var8 = false;
      String var9 = null;
      if (this.e != null) {
         var1.setColor(16777215);
         var1.fillRect(0, 0, a, b);
         var1.setColor(0);
         var1.drawString(this.e, 5, 10, 0);
         var1.drawString(a, 5, 25, 0);
         var1.drawString("Press any key 3x to quit...", 5, 40, 0);
      } else if (b != null) {
         var1.setColor(0);
         var1.fillRect(0, 0, a, b);
         var1.drawImage(b, this.c - (b.getWidth() >> 1), this.d - (b.getHeight() >> 1), 0);
      } else {
         switch (m) {
            case 0:
               this.r();
               this.q();
               this.b(var1);
               var1.drawImage(this.a, 0, 0, 0);
               if (a.size() != 0) {
                  var2 = (byte[])a.elementAt(a.size() - 1);

                  for (int var33 = this.h[0]; var33 <= this.i[0] && var33 < f; var33++) {
                     for (int var35 = this.h[1]; var35 <= this.i[1] && var35 < g; var35++) {
                        if (var33 >= 0 && var35 >= 0) {
                           var4 = this.a[var33 * g + 0 * this.f + var35] + this.i[0];
                           var5 = this.a[var33 * g + 1 * this.f + var35] + this.i[1];
                           if (((Object[])var2)[var33 * g + var35] != 0) {
                              var3 = g.b(this.a, (int)((Object[])var2)[var33 * g + var35]);
                           }

                           if (var4 > -i && var4 < a && var5 > -j && var5 < b + var3) {
                              if (((Object[])var2)[var33 * g + var35] != 0) {
                                 g.a(var1, this.a, (int)((Object[])var2)[var33 * g + var35], var4, var5);
                              }

                              for (int var36 = 0; var36 < a.length; var36++) {
                                 if (a[var36] != null && a[var36].a[0] == var33 && a[var36].a[1] == var35) {
                                    h.a(a[var36], var1, this.i);
                                 }
                              }
                           }
                        }
                     }
                  }

                  if (e && this.a != null) {
                     if (this.a.q == 0) {
                        int var41 = Math.min(70, 70 * this.a.q / this.a.o);
                        int var11 = Math.min(70, 70 * this.a.r / this.a.p);
                        var1.setColor(16711680);
                        var1.fillRect(18, 10, var41, 7);
                        var1.setColor(255);
                        var1.fillRect(18, 18, var11, 7);
                     }

                     g.a(var1, this.a, -56, 0, 0);
                     if (this.a.v != -1) {
                        g.a(var1, this.b, this.a.v, a - g.a(this.b, this.a.v) - 2, 2);
                     }

                     if (this.a.w != -1) {
                        g.a(var1, this.b, this.a.w, a - (g.a(this.b, this.a.v) << 1) - 4, 2);
                     }
                  }

                  var1.setFont(c);
                  var1.setColor(16777215);
                  var1.drawString(a(422).toUpperCase(), 2, b - c.getHeight() - 2, 0);
                  if (e) {
                     var1.drawString(a(421).toUpperCase(), a - c.stringWidth(a(421)) - 2, b - c.getHeight() - 2, 0);
                  }

                  var1.setFont(a);
                  if (b != null) {
                     h = b - b.getHeight() - 5;
                     var1.setFont(b);
                     var1.setColor(0);
                     var1.fillRect(0, h - 5, a, b);
                     if (!k) {
                        if (g == -1) {
                           if (l == 0 || l == 1) {
                              g = (a >> 1) - (b.stringWidth(b) >> 1);
                           } else if (l == 2) {
                              g = -b.stringWidth(b);
                           } else if (l == 3) {
                              g = a;
                           }
                        }

                        var1.setColor(f);
                        var1.drawString(b, g, h, 0);
                     }
                  }

                  i.a(var1, this.i);
                  if (this.g && this.a.a == 0) {
                     this.a(var1);
                  }
               }
            case 1:
            case 2:
            case 12:
            default:
               break;
            case 3:
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(0);
               var1.setFont(d);
               if (c != null) {
                  var1.drawImage(c, this.c - (c.getWidth() >> 1), 0, 0);
                  if (k == 1) {
                     var1.setColor(1044480);
                     var1.drawString(a(423), (a >> 1) - (d.stringWidth(a(423)) >> 1), c.getHeight() + 1, 0);
                  }
               }

               var1.setColor(16711680);
               var1.drawString("<<", 0, (c != null ? c.getHeight() : 0) + 25, 0);
               var1.drawString(">>", a - d.stringWidth(">>"), (c != null ? c.getHeight() : 0) + 25, 0);
               var1.setColor(16777215);
               int var10 = a - d.stringWidth("<<  >>");
               if (d.stringWidth(a[k][e[k]]) >= var10 && a[k][e[k]].indexOf(32) != -1) {
                  String var42 = a[k][e[k]].substring(0, a[k][e[k]].indexOf(32));
                  String var43 = a[k][e[k]].substring(a[k][e[k]].indexOf(32) + 1);
                  var1.drawString(var42, (a >> 1) - (d.stringWidth(var42) >> 1), (c != null ? c.getHeight() : 0) + 25 - (d.getHeight() >> 1), 0);
                  var1.drawString(var43, (a >> 1) - (d.stringWidth(var43) >> 1), (c != null ? c.getHeight() : 0) + 25 + (d.getHeight() >> 1), 0);
               } else {
                  var1.drawString(a[k][e[k]], (a >> 1) - (d.stringWidth(a[k][e[k]]) >> 1), (c != null ? c.getHeight() : 0) + 25, 0);
               }

               if (k != 0 && k != 5 && k != 4) {
                  var1.setFont(c);
                  var1.drawString(a(449).toUpperCase(), 2, b - c.getHeight() - 2, 0);
                  var1.setFont(d);
               }
               break;
            case 4:
            case 9:
            case 10:
            case 17:
            case 21:
            case 23:
               var1.setFont(c);
               var1.setColor(m != 4 && m != 21 && m != 23 && m != 17 ? 15327683 : 0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(m != 4 && m != 21 && m != 23 && m != 17 ? 0 : 16777215);
               boolean var14 = false;
               if (m == 21) {
                  var14 = true;
                  var1.setFont(a);
               }

               if ((m == 23 || m == 17) && this.g > 20) {
                  this.g = 20;
               }

               if (m == 21) {
                  this.g--;
               }

               var5 = 3 + this.g;

               for (int var31 = 0; var31 < this.a.length; var31++) {
                  for (int var34 = 0; var34 < this.a[var31].size(); var34++) {
                     int var15 = m != 4 && m != 21 && m != 23 && m != 17 ? 0 : 16777215;
                     var4 = 2;
                     if (!var14) {
                        var5 += c.getHeight() + 1;
                     } else {
                        var14 = false;
                     }

                     if ((var9 = (String)this.a[var31].elementAt(var34)).charAt(1) == '~') {
                        if (var9.charAt(0) == '1') {
                           var1.setFont(c);
                           var15 = 11184640;
                        } else if (var9.charAt(0) == '3') {
                           var15 = 11141120;
                           var4 = (a >> 1) - (c.stringWidth(var9.substring(var9.indexOf(126) + 1)) >> 1);
                        }

                        var9 = var9.substring(var9.indexOf(126) + 1);
                     }

                     var1.setColor(var15);
                     var1.drawString(var9, var4, var5, 0);
                  }

                  if (this.a[var31].size() == 0) {
                     var5 += c.getHeight() + 1;
                  }
               }

               if (m == 23 || m == 17) {
                  var1.setColor(0);
                  var1.setFont(c);
                  var1.fillRect(0, 0, a, c.getHeight() + 20);
                  var1.setColor(16777215);
                  var1.fillRect(5, 5, a - 10, c.getHeight() + 10);
                  var1.setColor(14483456);
                  var1.drawString(a(l), (a >> 1) - (c.stringWidth(a(l)) >> 1), 10, 0);
                  var1.setColor(16777215);
               }

               int var44 = b - c.getHeight();
               if (m == 10 || m == 23 || m == 4 || m == 17) {
                  var44 -= c.getHeight() * 3;
               }

               if (m == 4 || m == 23 || m == 17) {
                  var1.setColor(0);
                  var1.fillRect(0, b - c.getHeight() - 8, a, c.getHeight() + 8);
                  var1.setColor(16777215);
                  var1.setFont(c);
                  if (m != 4 && m != 23 && m != 17) {
                     var1.drawString(a(21).toUpperCase(), 2, b - c.getHeight() - 2, 0);
                  } else {
                     var1.drawString(a(449).toUpperCase(), 2, b - c.getHeight() - 2, 0);
                  }

                  var1.setFont(d);
                  int var46 = 0;
                  if (m == 4) {
                     var46 = b - (c.getHeight() << 2);
                  } else if (m == 23 || m == 17) {
                     var46 = 10;
                  }

                  if (this.g < var46) {
                     g.a(var1, this.b, 54, this.c - (g.a(this.b, 54) >> 1), b - c.getHeight() - 7);
                  }

                  if (var5 > var44) {
                     g.a(var1, this.b, 53, this.c - (g.a(this.b, 53) >> 1), b - c.getHeight() - 5 + g.b(this.b, 54));
                  }
               }

               if (m == 10) {
                  var1.setColor(15327683);
                  var1.fillRect(0, b - c.getHeight() - 8, a, c.getHeight() + 8);
                  var1.setColor(0);
                  if (this.g < b - (c.getHeight() << 2)) {
                     g.a(var1, this.b, 54, this.c - (g.a(this.b, 54) >> 1), b - c.getHeight() - 7);
                  }

                  if (var5 > var44) {
                     g.a(var1, this.b, 53, this.c - (g.a(this.b, 53) >> 1), b - c.getHeight() - 5 + g.b(this.b, 54));
                  }
               }

               if (var5 < var44) {
                  if (b) {
                     b = false;
                  } else {
                     b = true;
                     if (m != 21 && m != 23 && m != 17) {
                        try {
                           Thread.sleep(3000L);
                        } catch (Exception var19) {
                        }
                     }

                     if (m == 23 || m == 17) {
                        this.p = true;
                     }

                     if (m == 9) {
                        this.b = System.currentTimeMillis();
                        this.g = (short)(b - (c.getHeight() << 3));
                        this.h = 0;
                        e = new byte[7];
                        if (f) {
                           k = 5;
                        } else {
                           k = 0;
                        }

                        for (int var32 = 0; var32 < a.length; var32++) {
                           a[var32] = null;
                        }

                        a((byte)4);
                     } else if (m == 10) {
                        a((byte)0);
                     } else if (m == 4) {
                        a((byte)3);
                        if (f) {
                           k = 5;
                        } else {
                           k = 0;
                        }
                     }
                  }
               } else if (m == 23 || m == 17) {
                  this.p = false;
               }
               break;
            case 5:
               var1.setColor(0);
               var1.setFont(c);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.fillRect(5, 5, a - 10, 20);
               var1.setColor(14483456);
               if (j) {
                  var9 = a(424);
               } else {
                  var9 = a(425);
               }

               var1.drawString(var9, (a >> 1) - (c.stringWidth(var9) >> 1), 7, 0);
               var1.setColor(16777215);
               var5 = 30;

               for (int var30 = 0; var30 < a.length; var30++) {
                  var4 = 5;
                  if (var30 != k) {
                     var4 = 5 + c.stringWidth("> ");
                  }

                  if (a[var30].equals(a(294))) {
                     var5 += c.getHeight();
                  }

                  var1.drawString((var30 == k ? "> " : "") + a[var30], var4, var5, 0);
                  var5 += c.getHeight();
               }

               if (!a[k].equals(a(294))) {
                  if (j) {
                     var1.setColor(16711680);
                  }

                  var1.drawString(b[g[k]], (a >> 1) - (c.stringWidth(b[g[k]]) >> 1), b - c.getHeight() - 2, 0);
               }

               var1.drawString(a(449).toUpperCase(), 2, b - c.getHeight() - 2, 0);
               break;
            case 6:
            case 7:
               int var12 = a - 20;
               int var13 = this.r * var12 / 100;
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16711680);
               var1.fillRect(10, 30, var13, 10);
               var1.setColor(16777215);
               var1.drawRect(10, 30, var12, 10);
               if ((var9 = a(39)).length() == 0) {
                  var9 = new a().a((byte)0);
               }

               var1.drawString(var9 + "...", 10, 10, 0);
               break;
            case 8:
               var1.setColor(this.n);
               var1.fillRect(0, 0, a, b);
               g.a(var1, this.b, this.m, this.c - (g.a(this.b, this.m) >> 1), this.d - (g.b(this.b, this.m) >> 1));
               if (a.a) {
                  if (i) {
                     var9 = new a().a((byte)1);
                     var1.setFont(c);
                     var1.setColor(0);
                     var1.drawString(var9, (a >> 1) - (c.stringWidth(var9) >> 1), this.d + (g.b(this.b, this.m) >> 1) + 12, 0);
                  }

                  if (l == -1) {
                     l = 500;
                  }
               }
               break;
            case 11:
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.setFont(d);
               var1.drawString(a(428), (a >> 1) - (d.stringWidth(a(428)) >> 1), (b >> 1) - (d.getHeight() >> 1), 0);
               var1.drawString(a(427).toUpperCase(), 2, b - d.getHeight() - 2, 0);
               var1.drawString(a(426).toUpperCase(), a - d.stringWidth(a(426)) - 2, b - d.getHeight() - 2, 0);
               break;
            case 13:
               var5 = (b >> 1) - (d.getHeight() >> 1);
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.setFont(d);
               var1.drawString(a(450), (a >> 1) - (d.stringWidth(a(450)) >> 1), var5, 0);
               var1.setFont(c);
               var1.drawString(a(401), (a >> 1) - (c.stringWidth(a(401)) >> 1), var5 + (d.getHeight() << 1), 0);
               break;
            case 14:
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.setFont(d);
               var1.drawString(a(451), (a >> 1) - (d.stringWidth(a(451)) >> 1), (b >> 1) - (d.getHeight() >> 1), 0);
               var1.drawString(a(427).toUpperCase(), 2, b - d.getHeight() - 2, 0);
               var1.drawString(a(426).toUpperCase(), a - d.stringWidth(a(426)) - 2, b - d.getHeight() - 2, 0);
               break;
            case 15:
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.setFont(d);
               var1.drawString("Please Wait...", (a >> 1) - (d.stringWidth("Please Wait...") >> 1), (b >> 1) - (d.getHeight() >> 1), 0);
               g.a(var1, c, 5, (a >> 1) - (g.a(c, 5) >> 1), (b >> 1) + d.getHeight());
               break;
            case 16:
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.setFont(d);
               var1.drawString(a(455), (a >> 1) - (d.stringWidth(a(455)) >> 1), (b >> 1) - (d.getHeight() >> 1), 0);
               var1.drawString(a(464), (a >> 1) - (d.stringWidth(a(464)) >> 1), (b >> 1) - (d.getHeight() >> 1) + d.getHeight(), 0);
               var1.drawString(a(427).toUpperCase(), 2, b - d.getHeight() - 2, 0);
               var1.drawString(a(426).toUpperCase(), a - d.stringWidth(a(426)) - 2, b - d.getHeight() - 2, 0);
               break;
            case 18:
               var1.setColor(0);
               var1.setFont(c);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.fillRect(5, 5, a - 10, c.getHeight() + 10);
               var1.setColor(14483456);
               var1.drawString(a(l), (a >> 1) - (c.stringWidth(a(l)) >> 1), 10, 0);
               var1.setColor(16777215);
               var5 = 35;

               for (var6 = w; var6 < b[v].length && b[v][var6] != null; var6++) {
                  String var45;
                  String var49 = var45 = b[v][var6];

                  int var18;
                  for (var18 = var45.length(); c.stringWidth(var45) > a - 20; var45 = var45.substring(0, var18)) {
                     var18 = var45.lastIndexOf(32, var18 - 1);
                  }

                  var1.drawString(var45, 10, var5, 0);
                  var5 += c.getHeight();
                  if (var18 < var49.length()) {
                     if (var5 + (c.getHeight() << 1) >= b) {
                        break;
                     }

                     var1.drawString(var49.substring(var18), 15, var5, 0);
                     var5 += c.getHeight();
                  }

                  if (var5 + (c.getHeight() << 1) >= b) {
                     break;
                  }
               }

               var1.drawString(a(449).toUpperCase(), 2, b - c.getHeight() - 2, 0);
               if (w != 0) {
                  g.a(var1, this.b, 54, a - g.a(this.b, 54) - 2, 35);
               }

               if (var6 < b[v].length && b[v][var6] != null) {
                  g.a(var1, this.b, 53, a - g.a(this.b, 53) - 2, b - c.getHeight() - g.b(this.b, 53) - g.b(this.b, 55) - 4);
                  q = true;
               } else {
                  q = false;
               }

               if (l != 573) {
                  g.a(var1, this.b, 56, 2, b - c.getHeight() - g.b(this.b, 53) - 4);
                  g.a(var1, this.b, 55, a - g.a(this.b, 55) - 2, b - c.getHeight() - g.b(this.b, 53) - 4);
               }
               break;
            case 19:
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.setFont(d);
               var1.drawString(a(475), (a >> 1) - (d.stringWidth(a(451)) >> 1), (b >> 1) - (d.getHeight() >> 1), 0);
               var1.drawString(a(427).toUpperCase(), 2, b - d.getHeight() - 2, 0);
               var1.drawString(a(426).toUpperCase(), a - d.stringWidth(a(426)) - 2, b - d.getHeight() - 2, 0);
               break;
            case 20:
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.setFont(c);
               var1.drawString(a(566), (a >> 1) - (c.stringWidth(a(566)) >> 1), (b >> 1) - (c.getHeight() >> 1), 0);
               var1.drawString(a(567).toUpperCase(), 2, b - c.getHeight() - 2, 0);
               break;
            case 22:
               var1.setColor(0);
               var1.fillRect(0, 0, a, b);
               var1.setColor(16777215);
               var1.setFont(d);
               if (a(571).length() == 0) {
                  a var16;
                  String var17 = (var16 = new a()).a((byte)2);
                  var1.drawString(var17, (a >> 1) - (d.stringWidth(var17) >> 1), (b >> 1) - (d.getHeight() >> 1), 0);
                  var17 = var16.a((byte)4);
                  var1.drawString(var17.toUpperCase(), 2, b - d.getHeight() - 2, 0);
                  var17 = var16.a((byte)3);
                  var1.drawString(var17.toUpperCase(), a - d.stringWidth(var17) - 2, b - d.getHeight() - 2, 0);
               } else {
                  var1.drawString(a(571), (a >> 1) - (d.stringWidth(a(571)) >> 1), (b >> 1) - (d.getHeight() >> 1), 0);
                  var1.drawString(a(22).toUpperCase(), 2, b - d.getHeight() - 2, 0);
                  var1.drawString(a(426).toUpperCase(), a - d.stringWidth(a(426)) - 2, b - d.getHeight() - 2, 0);
               }
         }

         if (this.a.a == 1) {
            this.a.a(var1);
         }
      }
   }

   private static final void c(int[] var0, int[] var1) {
      b(o, var0);
      var1[0] = o[0] >> 7;
      var1[1] = o[1] >> 7;
   }

   public static final void a(int[] var0, int[] var1) {
      var1[0] = var0[0] - var0[1] >> 3;
      var1[1] = var0[0] + var0[1] >> 4;
   }

   public static final void b(int[] var0, int[] var1) {
      var0[0] = (var1[0] << 2) + (var1[1] << 3);
      var0[1] = (var1[1] << 3) - (var1[0] << 2);
   }

   public final void run() {
      long var1 = 0L;
      boolean var3 = false;
      boolean var4 = false;
      int[] var5 = new int[2];
      boolean var6 = false;
      this.b = System.currentTimeMillis();

      try {
         while (this.m) {
            this.a = System.currentTimeMillis();
            var1 = this.a - this.b;
            this.b = this.a;
            if (this.d) {
               Thread.sleep(1000L);
            } else {
               if (m == 12) {
                  break;
               }

               if (this.a.a == 1) {
                  this.a.a(var1);
               } else if (m != 3 && m != 10 && m != 9 && m != 13) {
                  i.a(var1);
                  a.a(var1);
               }

               this.b(var1);
               if (l >= 0) {
                  l = (int)(l - var1);
                  if (l <= 0) {
                     l = 500;
                     i = !i;
                  }
               }

               this.a(var1);

               for (int var14 = 0; var14 <= this.o && m == 0; var14++) {
                  if (a[var14] != null) {
                     byte var7 = 0;
                     byte var8 = 0;
                     byte var9 = 0;
                     if (var14 == 0) {
                        var7 = a[var14].l;
                        var8 = a[var14].m;
                     }

                     h.a(a[var14], var1, !this.g && this.l);
                     if (a[var14] != null && var14 == 0) {
                        if ((var9 = h.a(a[var14], this.j, this.k)) != var7) {
                           if (var8 != 0 && var8 != -1 && var8 != -2) {
                              e.a((char)var8);
                           }

                           if (var9 != 0 && var9 != -1 && var9 != -2) {
                              e.a((char)var9);
                           } else if (var9 == -2) {
                              a(a(24), 60, 4, 0);
                           }
                        }

                        if (b == null || b.equals(a(363))) {
                           var6 = false;
                        }

                        for (byte var15 = 0; !var6 && var15 < h; var15 += 3) {
                           var5[0] = d[var15 + 0] << 7;
                           var5[1] = d[var15 + 1] << 7;
                           if (Math.abs(h.a(var5, this.a.b)) < 350) {
                              a(a(363), 60, 4, 0);
                              var6 = true;
                           }
                        }

                        if ((b == null || b.equals(a(363))) && !var6) {
                           a((String)null, 0, 0, 0);
                        }
                     }
                  }
               }

               if (m == 9 || m == 10 || m == 4 || m == 21) {
                  if (this.h > 100) {
                     this.g--;
                     this.h = 0;
                  }

                  this.h = (short)(this.h + var1);
               }

               if (m == 15) {
                  this.k = (short)(this.k + var1);
                  if (this.k >= 200) {
                     g.a(c, 5);
                     this.k = 0;
                  }
               }

               this.repaint();
               this.serviceRepaints();
               System.gc();
            }
         }
      } catch (Exception var11) {
         this.e = "Code: " + a;

         for (int var13 = 0; var13 < p - 1; var13++) {
            this.e = this.e + (var13 != 0 ? "." : "") + n[var13];
         }

         a = var11.toString();
         this.repaint();
         this.serviceRepaints();
         var11.printStackTrace();
      }
   }

   private final void a(long var1) {
      if (b != null) {
         if (l == 1) {
            if (i >= 500) {
               k = !k;
               i = 0;
            }

            i = (int)(i + var1);
         } else if (l == 2) {
            if (j >= 50) {
               if ((g += 2) == -1) {
                  g++;
               }

               j = 0;
            }

            j = (int)(j + var1);
         } else if (l == 3) {
            if (j >= 50) {
               if ((g -= 2) == -1) {
                  g--;
               }

               j = 0;
            }

            j = (int)(j + var1);
         }

         if (e > d) {
            b = null;
            e = 0;
            d = 0;
            f = 0;
            g = -1;
            h = -1;
            return;
         }

         e = (int)(e + var1);
      }
   }

   private final void n() {
      int[] var1 = null;
      int[] var2 = null;
      boolean var3 = false;
      int var4 = 0;
      char var5 = '\u0000';
      char var6 = '\u0000';
      boolean var7 = false;
      c[] var8 = new c[]{new c(a(25), null, false), new c(a(26), null, false), new c(a(27), null, false), new c(a(394), null, false)};
      c[] var9 = new c[]{
         new c(a(28), null, false),
         new c(a(29), null, false),
         new c(a(30), null, false),
         new c(a(31), null, false),
         new c(a(32), null, false),
         new c(a(33), null, false),
         new c(a(34), null, false),
         new c(a(35), null, false)
      };

      for (int var24 = 0; var24 < var9.length; var24++) {
         c var10 = var9[var24];
         c var11;
         (var11 = var8[1]).a.addElement(var10);
         var10.a = var11;
      }

      String var28 = null;
      if (this.a.f == 4) {
         var28 = a(12);
      } else if (this.a.f == 3) {
         var28 = a(11);
      } else if (this.a.f == 8) {
         var28 = a(16);
      } else if (this.a.f == 5) {
         var28 = a(13);
      } else if (this.a.f == 1) {
         var28 = a(9);
      } else if (this.a.f == 2) {
         var28 = a(10);
      } else if (this.a.f == 7) {
         var28 = a(15);
      } else if (this.a.f == 6) {
         var28 = a(14);
      }

      var8[3].a = new String[]{
         a(443) + ": ",
         var28,
         a(17) + ": ",
         Integer.toString(this.a.o),
         a(441) + ": ",
         Integer.toString(this.a.b),
         a(442) + ": ",
         this.a.o < h.a.length - 1 ? Integer.toString(h.a[this.a.o + 1] - this.a.b) : "0",
         a(415) + ": ",
         Integer.toString(this.a.s * 3),
         a(416) + ": ",
         Integer.toString(this.a.t * 3),
         a(417) + ": ",
         Integer.toString(this.a.u * 3),
         a(418) + ": ",
         Integer.toString(this.a.v * 3),
         a(419) + ": ",
         Integer.toString(this.a.x * 3),
         a(420) + ": ",
         Integer.toString(this.a.y * 3),
         a(431) + ": ",
         Integer.toString(this.a.C * 3),
         a(432) + ": ",
         Integer.toString(this.a.D * 3),
         a(563) + ": ",
         "42",
         a(562) + ": ",
         "40",
         a(38) + ": ",
         Integer.toString(b)
      };
      boolean[] var29 = new boolean[]{false, false, false, false, false, false, false, false, false};
      boolean var12 = false;
      c var13 = null;
      boolean var14 = false;
      boolean var15 = false;
      if (this.a != null) {
         for (; this.a.k[var4] != 0; var4++) {
            var5 = (char)(this.a.k[var4] >> 8 & 0xFF);
            var6 = (char)(this.a.k[var4] >> 0 & 0xFF);
            switch (var5) {
               case '\u0000':
                  if ((var2 = a.a(4, var6))[2] == 4) {
                     (var13 = new c(a(400) + a.a(var2[1]), a(432) + ": " + var2[3], h.a(this.a, var2, false) && !var12)).b = h.a(this.a, 0, var2);
                     c var35;
                     (var35 = var8[0]).a.addElement(var13);
                     var13.a = var35;
                  } else {
                     var13 = new c(a(305) + a.a(var2[1]), a(432) + ": " + var2[3], h.a(this.a, var2, false) && !var12);
                     if (h.a(this.a, var2, false)) {
                        a = var13;
                     }

                     var13.b = h.a(this.a, 0, var2);
                     c var34;
                     (var34 = var8[0]).a.addElement(var13);
                     var13.a = var34;
                  }

                  var12 |= h.a(this.a, var2, false);
                  break;
               case '\u0001':
                  var2 = a.a(1, var6);
                  boolean var16 = h.a(this.a, (int)var2[0]);
                  (var13 = new c(a.a(var2[1]), a(444) + ": " + var2[4], var16 & !var29[var2[3]])).b = h.a(this.a, 1, var2);
                  c var17;
                  (var17 = var9[var2[3]]).a.addElement(var13);
                  var13.a = var17;
                  var29[var2[3]] = var29[var2[3]] | var16;
                  break;
               case '\u0002':
                  var2 = a.a(2, var6);
                  c var18 = new c(a.a((int)((Object[])var2)[1]), null, false);
                  if (var2 == this.a.f && !var14) {
                     var18.a = true;
                     var14 = true;
                  }

                  if (var2 == this.a.g && !var15) {
                     var18.a = true;
                     var15 = true;
                  }

                  c var19;
                  (var19 = var8[2]).a.addElement(var18);
                  var18.a = var19;
            }
         }

         for (int var27 = 0; var27 < this.a.h.length && this.a.h[var27] != -1; var27++) {
            var1 = a.a(8, this.a.h[var27]);
            var13 = new c(a(304) + a.a(var1[1]), null, h.a(this.a, var1, true));
            c var36;
            (var36 = var8[0]).a.addElement(var13);
            var13.a = var36;
            if (h.a(this.a, var1, true)) {
               b = var13;
            }
         }
      }

      this.a.a(new byte[]{4, 1, 2, 3, 18}, var8, null, this.a, this.a);
      this.n = true;
   }

   private final void o() {
      c[] var1 = new c[]{new c(a(36), null, false), new c(a(37), null, false)};
      int[] var2 = a.a(7, 0);
      int[] var3 = null;
      int var4 = 0;
      c var5 = null;

      while (var2[var4] != -1) {
         switch (var2[var4++]) {
            case 0:
               var3 = a.a(4, var2[var4++]);
               (var5 = new c(a.a(var3[1]) + " : " + var3[7] + " " + a(38), a(432) + ": " + var3[3], false)).b = h.a(this.a, 0, var3);
               c var6;
               (var6 = var1[0]).a.addElement(var5);
               var5.a = var6;
               break;
            case 1:
               var3 = a.a(1, var2[var4++]);
               (var5 = new c(a.a(var3[1]) + " : " + var3[9] + " " + a(38), a(444) + ": " + var3[4], false)).b = h.a(this.a, 1, var3);
               c var7;
               (var7 = var1[0]).a.addElement(var5);
               var5.a = var7;
               break;
            case 2:
               var3 = a.a(2, var2[var4++]);
               c var8 = new c(a.a(var3[1]) + " : " + var3[13] + " " + a(38), null, false);
               c var9;
               (var9 = var1[0]).a.addElement(var8);
               var8.a = var9;
               break;
            case 3:
               var3 = a.a(3, var2[var4++]);
               c var10 = new c(a.a(var3[1]) + " : " + var3[2] + " " + a(38), null, false);
               c var11;
               (var11 = var1[0]).a.addElement(var10);
               var10.a = var11;
         }
      }

      if (this.a != null) {
         for (int var19 = 0; this.a.k[var19] != 0; var19++) {
            char var24 = (char)(this.a.k[var19] >> 8 & 0xFF);
            char var25 = (char)(this.a.k[var19] >> 0 & 0xFF);
            switch (var24) {
               case '\u0000':
                  var3 = a.a(4, var25);
                  (var5 = new c(a.a(var3[1]) + " : " + (var3[7] >> 2) + " " + a(38), a(432) + ": " + var3[3], false)).b = h.a(this.a, 0, var3);
                  c var26;
                  (var26 = var1[1]).a.addElement(var5);
                  var5.a = var26;
                  break;
               case '\u0001':
                  var3 = a.a(1, var25);
                  (var5 = new c(a.a(var3[1]) + " : " + (var3[9] >> 2) + " " + a(38), a(444) + ": " + var3[4], false)).b = h.a(this.a, 1, var3);
                  c var27;
                  (var27 = var1[1]).a.addElement(var5);
                  var5.a = var27;
                  break;
               case '\u0002':
                  var3 = a.a(2, var25);
                  c var28 = new c(a.a(var3[1]) + " : " + (var3[13] >> 2) + " " + a(38), null, false);
                  c var29;
                  (var29 = var1[1]).a.addElement(var28);
                  var28.a = var29;
            }
         }
      }

      this.a.a(new byte[]{17, 15, 16}, var1, a(38) + " : " + b, this.a, this.a);
      this.n = true;
   }

   public final void keyReleased(int var1) {
      this.p = 1;
   }

   public static final void a(byte var0) {
      if (a) {
         if (m != 12) {
            if (m == 0) {
               f = true;
            }

            byte var1 = m;
            m = var0;
            if (m == 3 && c == null) {
               if (!f && k != 4) {
                  try {
                     c = Image.createImage("/main.png");
                  } catch (IOException var3) {
                     var3.printStackTrace();
                  }
               }
            } else {
               if (var1 != 22) {
                  c = null;
               }

               b();
            }

            if (m == 9) {
               f = false;
               h(a(547));
            } else if (m == 4) {
               h(a(548));
               a.g = (short)(b - (c.getHeight() << 2));
               a.h = 0;
            } else if (m == 21) {
               h(new a().a());
               a.g = 0;
            } else if (m == 23) {
               h(a(574));
               a.g = 15;
               a.p = false;
            } else {
               if (m == 17) {
                  h(a(465));
                  a.g = 15;
                  a.p = false;
               }
            }
         }
      }
   }

   private static final void h(String var0) {
      boolean var1 = false;
      int var2 = 0;
      int var3 = 0;
      Vector var4 = new Vector();
      if ((var2 = var0.indexOf("VERSION")) != -1) {
         var0 = var0.substring(0, var2) + d + var0.substring(var2 + 7);
      }

      while ((var2 = var0.indexOf("\\n", var3)) != -1) {
         var4.addElement(var0.substring(var3, var2));
         var3 = var2 + 2;
      }

      var4.addElement(var0.substring(var3));
      a.a = new Vector[var4.size()];
      g(null);

      for (int var5 = 0; var5 < var4.size(); var5++) {
         a.a[var5] = new Vector();
         a((String)var4.elementAt(var5), a.a[var5], a - 10);
      }
   }

   public static final void b() {
      System.gc();
      Runtime.getRuntime().gc();

      try {
         Thread.sleep(100L);
      } catch (Exception var1) {
      }

      System.gc();
      Runtime.getRuntime().gc();
   }

   private final int a(int var1) {
      int var2 = -1122868;
      if (var1 == 1 || var1 == 50) {
         var2 = 3;
      } else if (var1 == 6 || var1 == 56) {
         var2 = 4;
      } else if (var1 == 2 || var1 == 52) {
         var2 = 5;
      } else if (var1 == 5 || var1 == 54) {
         var2 = 6;
      } else if (var1 == 8 || var1 == 20 || var1 == 53) {
         var2 = 7;
      } else if (var1 == f[0]) {
         var2 = 0;
      } else if (var1 == f[1]) {
         var2 = 1;
      } else if (var1 == f[2]) {
         var2 = 2;
      }

      return var2;
   }

   public final void keyPressed(int var1) {
      a = var1;
      if (this.e != null && ++this.q == 3) {
         this.c();
      }
   }

   private void p() {
      boolean var1 = false;

      for (int var2 = 0; var2 < e.length; var2++) {
         e[var2] = 0;
      }

      if (f) {
         k = 5;
      } else {
         k = 0;
      }
   }

   private final void b(long var1) {
      if (a == -c || a == -a || a == -b) {
         a = -a;
      }

      int var3 = a;
      int var4 = 0;

      try {
         var4 = this.getGameAction(var3);
      } catch (Exception var15) {
      }

      if (a == c || a == a || a == b || a == d || a == e || a >= 48 && a <= 57 || var4 == 8 || var4 == 1 || var4 == 6 || var4 == 2 || var4 == 5) {
         int var5;
         if (a != d && a != e) {
            byte var6 = 0;
            byte var7 = 0;
            byte var8 = 0;
            boolean var9 = false;
            boolean var10 = false;
            if (a == -286331154 || a == c) {
               this.p = 0;
               return;
            }

            var5 = a >= 0 ? a : this.getGameAction(a);
            switch (m) {
               case 0:
                  if (a == a) {
                     if (this.a != null && e) {
                        this.n();
                        a((byte)2);
                        a = -286331154;
                     }
                  } else if (a == b) {
                     this.l();
                     k = 5;

                     for (int var24 = 0; var24 < e.length; var24++) {
                        e[var24] = 0;
                     }

                     a((byte)3);
                     a = -286331154;
                  } else {
                     var5 = this.a(var5);
                     if (this.g || this.a.a != 0 || !this.l || this.a == null || this.a.q != 0) {
                        break;
                     }

                     var6 = this.a.l;
                     var7 = this.a.m;
                     var8 = this.a.n;
                     if (h.a(this.a, var5, (long)var1)) {
                        a = -286331154;
                        this.p = 0;
                     }

                     if (var8 != this.a.n && this.a.n != 0 && this.a.n != -1 && this.a.n != -2) {
                        e.a((char)this.a.n);
                     }

                     if (var5 == 0) {
                        h.a(this.a, true);
                        a = -286331154;
                        this.p = 0;
                     } else if (var5 == 1) {
                        h.a(this.a, false);
                        a = -286331154;
                        this.p = 0;
                     } else if (var5 == 7 && this.a.a > 1000) {
                        int[] var26 = new int[2];

                        for (byte var23 = 0; var23 < h; var23 += 3) {
                           var26[0] = d[var23 + 0] << 7;
                           var26[1] = d[var23 + 1] << 7;
                           int[] var13;
                           if (h.a(var26, this.a.b) < 350 && (var13 = a.a(6, d[var23 + 2])) != null) {
                              i.a(8, var26[0], var26[1] + 128);
                              if (((byte[])a.elementAt(a.size() - 1))[d[var23 + 0] * g + d[var23 + 1]] == -45) {
                                 ((byte[])a.elementAt(a.size() - 1))[d[var23 + 0] * g + d[var23 + 1]] = -44;
                              } else {
                                 ((byte[])a.elementAt(a.size() - 1))[d[var23 + 0] * g + d[var23 + 1]] = 0;
                              }

                              for (byte var25 = var23; var25 < h && var25 < d.length - 3; var25 += 3) {
                                 d[var25 + 0] = d[var25 + 3];
                                 d[var25 + 1] = d[var25 + 4];
                                 d[var25 + 2] = d[var25 + 5];
                              }

                              h = (byte)(h - 3);
                              if (var13[2] > 0) {
                                 a(var13[2] + " " + a(38), 3, 4, 0);
                                 b = b + var13[2];
                              } else if (var13[4] > 0) {
                                 int[] var14 = a.a(1, var13[4]);
                                 a(a.a(var14[1]), 3, 4, 0);
                                 h.a(this.a, 1, var14);
                              } else if (var13[3] > 0) {
                                 int[] var27 = a.a(4, var13[3]);
                                 a(a.a(var27[1]), 3, 4, 0);
                                 h.a(this.a, 0, var27);
                              } else if (var13[5] > 0) {
                                 int[] var28 = a.a(2, var13[5]);
                                 a(a.a(var28[1]), 3, 4, 0);
                                 h.a(this.a, 2, var28);
                              }
                              break;
                           }
                        }
                     }

                     h.a(this.a, this.j, this.k);
                     if (this.a.l == var6) {
                        break;
                     }

                     if (var7 != 0 && var7 != -1 && var7 != -2) {
                        e.a((char)var7);
                     } else if (var7 == -2) {
                        a((String)null, 0, 0, 0);
                     }

                     if (this.a.l != 0 && this.a.l != -1 && this.a.l != -2) {
                        e.a((char)this.a.l);
                     } else if (this.a.l == -2) {
                        a(a(24), 60, 4, 0);
                     }
                  }
                  break;
               case 1:
                  var5 = this.a(var5);
                  if (a == b) {
                     this.a.a = 0;
                     a((byte)3);
                  } else if (a != a && this.a.a == 1) {
                     this.a.a((char)var5);
                  }

                  this.p = 1;
                  break;
               case 2:
                  var5 = this.a(var5);
                  if (a == b) {
                     if (!this.a.a()) {
                        this.a.a = 0;
                        a((byte)0);
                     } else {
                        a = -286331154;
                     }
                  } else if (a != a && this.a.a == 1) {
                     this.a.a((char)var5);
                  }

                  this.p = 1;
                  break;
               case 3:
                  if ((var5 = this.a(var5)) == 5) {
                     if (--e[k] == -1) {
                        e[k] = (byte)(a[k].length - 1);
                     }
                  } else if (var5 == 6) {
                     if (++e[k] == a[k].length) {
                        e[k] = 0;
                     }
                  } else if (a == b && m == 3) {
                     if (k == 6) {
                        k = x;
                     } else if (k == 1) {
                        if (f) {
                           k = 5;
                        } else {
                           k = 0;
                        }
                     }
                  } else if (var5 == 7) {
                     if (k == 2) {
                        this.r = 0;
                        a((byte)6);
                        this.repaint();
                        this.serviceRepaints();

                        for (int var20 = 0; var20 < a.length; var20++) {
                           this.a = a[var20] = null;
                        }

                        this.a(a[3][e[k]]);
                     } else if (a[k][e[k]].startsWith(a(4))) {
                        this.o = !this.o;
                        this.l();
                        this.g();
                     } else if (a[k][e[k]].equals(a(19))) {
                        this.g();
                        e[k] = 2;
                        a((byte)13);
                        a = -286331154;
                        this.p = 0;
                     } else if (a[k][e[k]].equals(a(3))) {
                        this.k();
                        a((byte)14);
                        a = -286331154;
                        this.p = 0;
                     } else if (a[k][e[k]].equals(a(21))) {
                        a((byte)0);
                        this.d = false;
                     } else if (a[k][e[k]].equals(a(2))) {
                        if (this.b()) {
                           a((byte)16);
                        } else {
                           k = 1;
                        }
                     } else if (a[k][e[k]].equals(a(6))) {
                        a((byte)4);
                     } else if (a[k][e[k]].equals(a(456))) {
                        x = k;
                        k = 6;
                     } else if (a[k][e[k]].equals(a(457))) {
                        l = 457;
                        w = 0;
                        v = 0;
                        a((byte)17);
                     } else if (a[k][e[k]].equals(a(458))) {
                        for (int var21 = 0; var21 < g.length; var21++) {
                           g[var21] = f[var21];
                        }

                        a = -286331154;
                        this.p = 0;
                        a((byte)5);
                     } else if (a[k][e[k]].equals(a(573))) {
                        l = 573;
                        w = 0;
                        v = 0;
                        b = a();
                        a((byte)23);
                     } else if (a[k][e[k]].equals(a(522))) {
                        l = 522;
                        w = 0;
                        v = 0;
                        b = b();
                        a((byte)18);
                     } else if (a[k][e[k]].equals(a(459))) {
                        l = 459;
                        w = 0;
                        v = 0;
                        b = f();
                        a((byte)18);
                     } else if (a[k][e[k]].equals(a(460))) {
                        l = 460;
                        w = 0;
                        v = 0;
                        b = e();
                        a((byte)18);
                     } else if (a[k][e[k]].equals(a(461))) {
                        l = 461;
                        w = 0;
                        v = 0;
                        b = d();
                        a((byte)18);
                     } else if (a[k][e[k]].equals(a(462))) {
                        l = 462;
                        w = 0;
                        v = 0;
                        b = c();
                        a((byte)18);
                     } else if (a[k][e[k]].equals(a(18))) {
                        this.o();
                        a((byte)1);
                     } else if (a[k][e[k]].equals(a(20))) {
                        a((byte)0);
                     } else if (k == 1) {
                        this.k();
                        if (this.h) {
                           k++;
                        } else {
                           this.r = 0;
                           a((byte)6);
                           this.repaint();
                           this.serviceRepaints();

                           for (int var22 = 0; var22 < a.length; var22++) {
                              this.a = a[var22] = null;
                           }

                           this.a("/l01_1.scr");
                        }

                        b = 100;
                     } else if (a[k][e[k]].equals(a(22))) {
                        a((byte)19);
                     }
                  }

                  this.p = 1;
                  break;
               case 4:
               case 9:
               case 10:
               case 17:
               case 23:
                  var5 = this.a(var5);
                  long var11 = var1 / 10L;
                  if (var5 == 3) {
                     this.h = 0;
                     this.g = (short)Math.min(b - (c.getHeight() << 2), this.g + var11);
                  } else if (var5 == 4 && (m != 23 || !this.p) && (m != 17 || !this.p)) {
                     this.h = 0;
                     this.g = (short)(this.g - var11);
                  }

                  if (m != 4 && m != 23 && m != 17 || a != b) {
                     break;
                  }

                  if (m == 23 || m == 17) {
                     k = 6;
                     a = -286331154;
                     this.p = 0;
                  } else if (f) {
                     k = 5;
                  } else {
                     k = 0;
                  }

                  a((byte)3);
                  break;
               case 5:
                  var5 = this.a(var5);
                  if (j) {
                     if (a != b && a != a) {
                        if (this.a(a, var5)) {
                           if (a >= 0) {
                              g[k] = (byte)a;
                           } else {
                              g[k] = (byte)this.getGameAction(a);
                           }
                        } else {
                           a((byte)20);
                        }

                        j = false;
                     }
                  } else if (a == b) {
                     a((byte)3);
                  } else if (var5 == 3) {
                     k = Math.max(0, k - 1);
                  } else if (var5 == 4) {
                     k = Math.min(a.length - 1, k + 1);
                  } else if (var5 == 7) {
                     if (a[k].equals(a(294))) {
                        for (int var19 = 0; var19 < g.length; var19++) {
                           f[var19] = g[var19];
                        }

                        j = false;
                        a((byte)3);
                        this.g();
                     } else {
                        j = true;
                     }
                  }

                  this.p = 1;
               case 6:
               case 7:
               case 8:
               case 12:
               case 15:
               case 21:
               default:
                  break;
               case 11:
                  if (a == a) {
                     a((byte)0);
                  } else if (a == b) {
                     this.p();
                     a((byte)3);
                  }

                  this.p = 1;
                  break;
               case 13:
                  a((byte)3);
                  this.p = 1;
                  break;
               case 14:
                  if (a == a) {
                     this.h();
                  } else if (a == b) {
                     this.p();
                     a((byte)3);
                     e[0] = 1;
                  }

                  this.p = 1;
                  break;
               case 16:
                  if (a == a) {
                     k = 1;
                     a((byte)3);
                     a = -286331154;
                     this.p = 0;
                  } else if (a == b) {
                     a((byte)3);
                     a = -286331154;
                     this.p = 0;
                  }
                  break;
               case 18:
                  var5 = this.a(var5);
                  if (a == b) {
                     a((byte)3);
                  } else if (var5 == 5) {
                     w = 0;
                     if (--v < 0) {
                        v = (byte)(b.length - 1);
                     }
                  } else if (var5 == 6) {
                     w = 0;
                     if (++v > b.length - 1) {
                        v = 0;
                     }
                  } else if (var5 == 3) {
                     if (--w < 0) {
                        w = 0;
                     }
                  } else if (var5 == 4 && q) {
                     w++;
                  }

                  a = -286331154;
                  this.p = 0;
                  break;
               case 19:
                  if (a == a) {
                     this.c();
                  } else if (a == b) {
                     a((byte)3);
                     a = -286331154;
                     this.p = 0;
                  }
                  break;
               case 20:
                  if (a == b) {
                     a((byte)5);
                     a = -286331154;
                     this.p = 0;
                  }
                  break;
               case 22:
                  if (a == a) {
                     a(n);
                     n = -1;
                     f.a = false;
                     a = -286331154;
                     this.p = 0;
                  } else if (a == b) {
                     this.c();
                  }
            }

            if (a == -286331154) {
               return;
            }
         } else {
            if (m != 8) {
               return;
            }

            var5 = 0;
         }

         if (this.a.a == 0) {
            a.b((char)var5);
            this.d((char)var5);
         }

         if (this.p != 0) {
            a = -286331154;
            this.p = 0;
         }
      }
   }

   private final boolean a(int var1, int var2) {
      boolean var3 = false;
      var1 = var1 >= 0 ? var1 : this.getGameAction(var1);

      for (int var5 = 0; var5 < g.length; var5++) {
         if (var5 != k && var1 == g[var5]) {
            return false;
         }
      }

      if (var2 == 3) {
         return false;
      } else if (var2 == 4) {
         return false;
      } else if (var2 == 5) {
         return false;
      } else if (var2 == 6) {
         return false;
      } else if (var2 == 7) {
         return false;
      } else {
         return var1 == a ? false : var1 != b;
      }
   }

   private static final String[][] a() {
      if (h != null) {
         return h;
      }

      h = new String[1][1];
      h[0][0] = a(574);
      return h;
   }

   private static final String[][] b() {
      int var0 = 0;
      boolean var1 = false;
      boolean var2 = false;
      boolean var3 = false;
      if (g != null) {
         return g;
      }

      g = new String[a.h.length - 1][];

      for (int var16 = 1; var16 < a.h.length; var16++) {
         var0 = 0;
         g[var16 - 1] = new String[30];
         String[] var10000 = g[var16 - 1];
         var0++;
         var10000[0] = a(481) + a.a(a.h[var16][1]);
         var10000 = g[var16 - 1];
         var0++;
         var10000[1] = a(415) + ": " + a.h[var16][7] * 3;
         var10000 = g[var16 - 1];
         var0++;
         var10000[2] = a(416) + ": " + a.h[var16][8] * 3;
         var10000 = g[var16 - 1];
         var0++;
         var10000[3] = a(417) + ": " + a.h[var16][9] * 3;
         var10000 = g[var16 - 1];
         var0++;
         var10000[4] = a(418) + ": " + a.h[var16][10] * 3;
         var10000 = g[var16 - 1];
         var0++;
         var10000[5] = a(419) + ": " + a.h[var16][11] * 3;
         var10000 = g[var16 - 1];
         var0++;
         var10000[6] = a(420) + ": " + a.h[var16][12] * 3;
         var10000 = g[var16 - 1];
         var0++;
         var10000[7] = a(305) + a.a(a.c[a.h[var16][4]][1]);
         var10000 = g[var16 - 1];
         var0++;
         var10000[8] = a(499) + a.a(a.d[a.h[var16][5]][1]);
         var10000 = g[var16 - 1];
         var0++;
         var10000[9] = a(538);

         for (int var17 = 0; var17 < 15 && a.i[var16][var17] != -1; var17++) {
            g[var16 - 1][var0++] = "   " + e.b(a.i[var16][var17]);
         }

         g[var16 - 1][var0++] = a(539);

         for (int var18 = 0; var18 < a.j.length; var18++) {
            if (a.j[var16][var18] == -1) {
               if (var18 == 0) {
                  g[var16 - 1][var0] = "   " + a(572);
               }
               break;
            }

            g[var16 - 1][var0++] = "   " + a.a(a.k[a.j[var16][var18]][1]);
         }
      }

      return g;
   }

   private static final String[][] c() {
      int var0 = 0;
      boolean var1 = false;
      if (f != null) {
         return f;
      }

      f = new String[a.e.length - 1][];

      for (int var6 = 1; var6 < a.e.length; var6++) {
         var0 = 0;
         f[var6 - 1] = new String[10];
         String[] var10000 = f[var6 - 1];
         var0++;
         var10000[0] = a(481) + a.a(a.e[var6][1]);
         var10000 = f[var6 - 1];
         var0++;
         var10000[1] = a(484) + a.e[var6][13];
         var10000 = f[var6 - 1];
         var0++;
         var10000[2] = a(485) + (a.e[var6][13] >> 2);
         if (a.e[var6][2] > 0) {
            var10000 = f[var6 - 1];
            var0++;
            var10000[3] = a(496) + a.e[var6][2];
         }

         if (a.e[var6][3] > 0) {
            f[var6 - 1][var0++] = a(497) + a.e[var6][3];
         }

         if (a.e[var6][6] > 0) {
            f[var6 - 1][var0++] = a(498) + a.e[var6][6];
         }

         if (a.e[var6][7] > 0) {
            f[var6 - 1][var0++] = a(499) + a.e[var6][7];
         }

         if (a.e[var6][8] > 0) {
            f[var6 - 1][var0++] = a(500) + a.e[var6][8];
         }

         if (a.e[var6][10] > 0) {
            f[var6 - 1][var0++] = a(501) + a.e[var6][10];
         }

         if (a.e[var6][5] > 0) {
            f[var6 - 1][var0] = a(502) + a.e[var6][5] / 1000 + a(521);
         }
      }

      return f;
   }

   private static final String[][] d() {
      String var0 = null;
      String var1 = null;
      boolean var2 = false;
      if (e != null) {
         return e;
      }

      e = new String[a.k.length - 1][];

      for (int var3 = 1; var3 < a.k.length; var3++) {
         if (a.k[var3][2] == 0) {
            var0 = a(437);
         } else if (a.k[var3][2] == 3) {
            var0 = a(439);
         } else if (a.k[var3][2] == 6) {
            var0 = a(503);
         } else if (a.k[var3][2] == 5) {
            var0 = a(433);
         } else if (a.k[var3][2] == 4) {
            var0 = a(504);
         } else if (a.k[var3][2] == 1) {
            var0 = a(437);
         } else if (a.k[var3][2] == 2) {
            var0 = a(505);
         }

         if (a.k[var3][7] == 0) {
            var1 = a(506);
         } else if (a.k[var3][7] == 1) {
            var1 = a(507);
         } else if (a.k[var3][7] == 2) {
            var1 = a(508);
         } else if (a.k[var3][7] == 3) {
            var1 = a(509);
         } else if (a.k[var3][7] == 4) {
            var1 = a(510);
         }

         e[var3 - 1] = new String[]{
            a(481) + a.a(a.k[var3][1]),
            a(511) + var0,
            a(512),
            a(513) + a.k[var3][8],
            a(514) + a.k[var3][9],
            a(515) + a.k[var3][10],
            a(516),
            a(513) + Math.abs(a.k[var3][3]),
            a(514) + Math.abs(a.k[var3][4]),
            a(515) + Math.abs(a.k[var3][5]),
            a(517),
            a(513) + a.k[var3][11],
            a(514) + a.k[var3][12],
            a(515) + a.k[var3][13],
            a(518) + a.k[var3][6] / 1000 + a(521),
            a(519) + var1,
            a(520) + a.k[var3][14]
         };
      }

      return e;
   }

   private static final String[][] e() {
      String var0 = null;
      String var1 = null;
      int var2 = 0;
      byte var3 = 0;
      boolean var4 = false;
      boolean var5 = false;
      if (d != null) {
         return d;
      }

      d = new String[a.d.length - 1][];

      for (int var7 = 1; var7 < a.d.length; var7++) {
         var2 = 7;
         if (a.d[var7][3] == 0) {
            var0 = a(28);
         } else if (a.d[var7][3] == 1) {
            var0 = a(29);
         } else if (a.d[var7][3] == 2) {
            var0 = a(30);
         } else if (a.d[var7][3] == 7) {
            var0 = a(35);
         } else if (a.d[var7][3] == 3) {
            var0 = a(31);
         } else if (a.d[var7][3] == 4) {
            var0 = a(32);
         } else if (a.d[var7][3] == 6) {
            var0 = a(34);
         } else if (a.d[var7][3] == 5) {
            var0 = a(33);
         }

         if (a.d[var7][2] == 2) {
            var1 = a(487);
         } else if (a.d[var7][2] == 1) {
            var1 = a(488);
         } else if (a.d[var7][2] == 0) {
            var1 = a(489);
         }

         d[var7 - 1] = new String[]{
            a(481) + a.a(a.d[var7][1]),
            a(482) + var1,
            a(490) + var0,
            a(483) + a.d[var7][4],
            a(484) + a.d[var7][9],
            a(485) + (a.d[var7][9] >> 2),
            a(486),
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null
         };

         for (int var8 = 0; var8 < a.h.length; var8++) {
            if (a.d[var7][2] == 2) {
               var3 = 4;
            } else if (a.d[var7][2] == 1) {
               var3 = 3;
            } else if (a.d[var7][2] == 0) {
               var3 = 1;
            }

            if (a.a(a.h[var8][0], var3)) {
               d[var7 - 1][var2++] = "   " + a.a(a.h[var8][1]);
            }
         }
      }

      return d;
   }

   private static final String[][] f() {
      String var0 = null;
      int var1 = 0;
      byte var2 = 0;
      boolean var3 = false;
      boolean var4 = false;
      if (c != null) {
         return c;
      }

      c = new String[a.c.length - 1][];

      for (int var6 = 1; var6 < a.c.length; var6++) {
         var1 = 6;
         if (a.c[var6][2] == 0) {
            var0 = a(476);
         } else if (a.c[var6][2] == 1) {
            var0 = a(477);
         } else if (a.c[var6][2] == 4) {
            var0 = a(478);
         } else if (a.c[var6][2] == 2) {
            var0 = a(479);
         } else if (a.c[var6][2] == 3) {
            var0 = a(480);
         }

         c[var6 - 1] = new String[]{
            a(481) + a.a(a.c[var6][1]),
            a(482) + var0,
            a(483) + a.c[var6][3],
            a(484) + a.c[var6][7],
            a(485) + (a.c[var6][7] >> 2),
            a(486),
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null
         };

         for (int var7 = 0; var7 < a.h.length; var7++) {
            if (a.c[var6][2] == 0) {
               var2 = 14;
            } else if (a.c[var6][2] == 1) {
               var2 = 5;
            } else if (a.c[var6][2] == 4) {
               var2 = 8;
            } else if (a.c[var6][2] == 2) {
               var2 = 6;
            } else if (a.c[var6][2] == 3) {
               var2 = 7;
            }

            if (a.a(a.h[var7][0], var2)) {
               c[var6 - 1][var1++] = "   " + a.a(a.h[var7][1]);
            }
         }
      }

      return c;
   }

   private final void q() {
      this.a[0] = -this.i[0];
      this.a[1] = -this.i[1];
      this.b[0] = -this.i[0] + a;
      this.b[1] = -this.i[1];
      this.c[0] = -this.i[0];
      this.c[1] = -this.i[1] + b;
      this.d[0] = -this.i[0] + a;
      this.d[1] = -this.i[1] + b;
      c(this.a, this.e);
      c(this.b, this.f);
      c(this.c, this.g);
      c(this.d, this.h);
      this.h[0] = (byte)this.e[0];
      this.h[1] = (byte)this.f[1];
      this.i[0] = (byte)(this.h[0] + 3);
      this.i[1] = (byte)(this.g[1] + 3);
   }

   public final void c() {
      a((byte)12);
      this.repaint();
      this.serviceRepaints();

      try {
         Thread.sleep(2000L);
      } catch (Exception var2) {
      }

      this.m = false;
      this.a.notifyDestroyed();
   }

   public final void d() {
      this.m = true;
      new Thread(this).start();
   }

   public final j a(String var1, int var2, int var3, int[] var4) {
      byte var5 = 0;
      var5 = (byte)(a.length - 1);

      while (var5 >= 0 && a[var5] != null) {
         var5--;
      }

      return this.a(null, var1, var5, var2, var3, var4);
   }

   public final j a(String var1, String var2, byte var3, int var4, int var5, int[] var6) {
      b();
      if (var3 >= 0 && var3 < 25) {
         if (var3 == 0 && this.a != null) {
            a[var3] = this.a;
            h.a(this.a);
         } else {
            a[var3] = h.a(var2, (byte)(var3 + 1));
            if (var3 == 0) {
               h.a(a[var3], (byte)(e[1] + 1), false);
            }

            h.a(a[var3], var6);
            a[var3].c = var1;
         }

         h.a(a[var3], var4, var5);
         if (var3 == 0) {
            this.a = a[var3];
            this.a.p = (byte)(c ? 1 : 0);
            this.a.s = 0;
            this.a.k = -1;
            this.n = true;
            this.q = var3;
            this.i[0] = this.c - a[this.q].i[0];
            this.i[1] = this.d - a[this.q].i[1];
         }

         this.o = Math.max(this.o, var3);
         this.b = System.currentTimeMillis();
         return a[var3];
      } else {
         return null;
      }
   }

   public static final void a(int var0) {
      if (a[var0] != null) {
         if (var0 == 0) {
            i.a();
            a((byte)11);
            h.a(a.a);
            h.a(a.a, i, j);
            a((String)null, 0, 0, 0);
         } else {
            if (var0 == a.q) {
               g(null);
            }

            a[var0] = null;
            if (var0 == a.o) {
               while (var0 > 0 && a[var0] == null) {
                  var0--;
               }

               a.o = var0;
            }

            b();
         }
      }
   }

   public final void a(boolean var1) {
      this.l = var1;
      if (var1) {
         a = -286331154;
      }
   }

   public final void c(String var1) {
      this.a = g.a(var1);
   }

   public final void b(int var1, int var2) {
      this.m = var1;
      this.n = var2;
      if (var1 == 4) {
         a((byte)21);
      } else {
         a((byte)8);
      }
   }

   public final void a(int var1, int var2, boolean var3) {
      a[var1 * g + var2] = (byte)(var3 ? 1 : 0);
   }

   public final void a(int var1, int var2, int var3, int var4) {
      ((byte[])a.elementAt(var3))[var1 * g + var2] = (byte)var4;
      this.n = true;
   }

   public final void c(int var1, int var2) {
      int[] var3 = new int[]{0, 0};
      a(new int[]{var1, var2}, var3);
      this.i[0] = this.c - var3[0];
      this.i[1] = this.d - var3[1];
      this.n = true;
      this.q = -1;
      g(null);
   }

   public final void b(int var1) {
      if (a[var1] != null) {
         this.q = (byte)var1;
         this.n = true;
         this.i[0] = this.c - a[var1].i[0];
         this.i[1] = this.d - a[var1].i[1];
         g(a[var1].c);
      }
   }

   private final void r() {
      if (this.q >= 0 && a[this.q] != null && a[this.q].q == 0) {
         if (a[this.q].i[1] - h.a(a[this.q]) + this.i[1] < 0) {
            this.n = true;
         } else if (a[this.q].i[1] + this.i[1] > b) {
            this.n = true;
         } else if (a[this.q].i[0] + this.i[0] < 0) {
            this.n = true;
         } else if (a[this.q].i[0] + h.b(a[this.q]) + this.i[0] > a) {
            this.n = true;
         }

         if (this.n) {
            this.i[0] = this.c - a[this.q].i[0];
            this.i[1] = this.d - a[this.q].i[1] + h.a(a[this.q]);
         }
      }
   }

   public final void a(int var1, int var2, int var3, int var4, int var5) {
      if (this.j != null && this.k != null && c != null) {
         this.j[var1 * g + var2] = (byte)var3;
         this.k[var1 * g + var2] = (byte)var4;
         c[var1 * g + var2] = (byte)var5;
      }
   }

   public static final int a(String var0) {
      InputStream var1 = var0.getClass().getResourceAsStream(var0);
      int var2 = 0;
      int var3 = 0;
      if (b == null) {
         b = new byte[6144];
      }

      try {
         while ((var2 = var1.read(b, var3, b.length - var3)) > 0) {
            var3 += var2;
         }

         var1.close();
      } catch (Exception var5) {
         System.err.println(var0);
         var5.printStackTrace();
      }

      b();
      return var3;
   }

   public static final void c(int var0) {
      if (m != 0) {
         a.r = (byte)var0;
         if (a.o == -1) {
            a.o = m;
         }

         a((byte)7);
         a.repaint();
         a.serviceRepaints();
         if (var0 == 100) {
            a(a.o);
            a.r = -1;
            a.o = -1;
         }
      }
   }

   public final void e() {
      this.l();
      a((byte)3);
      k = 0;
   }

   public static final void a(String var0, int var1, int var2, int var3) {
      b = var0;
      d = var1 * 1000;
      l = (byte)var3;
      g = -1;
      h = -1;
      e = 0;
      f = 0;
      k = false;
      i = 0;
      j = 0;
      if (var2 == 0) {
         f = 0;
      }

      if (var2 == 3) {
         f = 255;
      }

      if (var2 == 5) {
         f = 65280;
      }

      if (var2 == 2) {
         f = 16711680;
      }

      if (var2 == 1) {
         f = 16777215;
      }

      if (var2 == 4) {
         f = 16776960;
      }
   }

   public final void d(String var1) {
      this.b = g.a(var1);
   }

   public final void f() {
      k = 4;
      a((byte)3);
   }

   public final boolean a() {
      return m == 1;
   }

   public final void a(c var1) {
      String var2 = var1.a;
      if (var1.a.a.equals(a(36))) {
         int var10 = var2.indexOf(58) + 2;
         int var11 = var2.indexOf(32, var10);
         int var12 = 0;

         try {
            var12 = Integer.parseInt(var2.substring(var10, var11));
         } catch (NumberFormatException var8) {
            System.out.println("IsoMap::menuSelected() buy");
            System.out.println("string= " + var2.substring(var10, var11));
            System.exit(1);
         }

         if (b >= var12) {
            b -= var12;
            int var13 = a.b(var2.substring(0, var10 - 3));
            int[] var14;
            if ((var14 = a.a(var2.substring(0, var10 - 3))) != null && this.a != null) {
               h.a(this.a, var13, var14);
               this.o();
            }
         }

         var1.a = false;
         this.a.a = a(38) + " : " + b;
      } else if (var1.a.a.equals(a(37))) {
         int var3 = var2.indexOf(58) + 2;
         int var4 = var2.indexOf(32, var3);
         int var5 = 0;

         try {
            var5 = Integer.parseInt(var2.substring(var3, var4));
         } catch (NumberFormatException var9) {
            System.out.println("IsoMap::menuSelected() //sell");
            System.out.println("string= " + var2.substring(var3, var4));
            System.exit(1);
         }

         if (this.a != null) {
            int var6 = a.b(var2.substring(0, var3 - 3));
            int[] var7;
            if ((var7 = a.a(var2.substring(0, var3 - 3))) != null) {
               h.b(this.a, var6, var7);
            }

            if (var1.a.a.indexOf(var1) == var1.a.a.size() - 1) {
               this.a.a('\u0003');
            }

            var1.a.a.removeElement(var1);
         }

         b += var5;
         var1.a = false;
         this.a.a = a(38) + " : " + b;
      } else if (var1.a.a.equals(a(26))) {
         var1.a = false;
      } else if (var1.a.a.equals(a(25))) {
         if (h.a(this.a, var2)) {
            if (a != null) {
               a.a = true;
            }

            b = var1;
         } else {
            if (b != null) {
               b.a = true;
            }

            a = var1;
         }
      } else if (var1.a.a.equals(a(27))) {
         h.b(this.a, a.a(var2));
      } else {
         h.c(this.a, a.a(var2));
      }
   }

   public final void g() {
      boolean var1 = false;

      try {
         ByteArrayOutputStream var2 = new ByteArrayOutputStream();
         RecordStore var3 = RecordStore.openRecordStore("ESO", true);

         for (int var5 = 0; var5 < f.length; var5++) {
            var2.write(f[var5]);
         }

         var2.write(this.o ? 1 : 0);
         if (this.a == null) {
            var2.write(0);
         } else {
            var2.write(1);
            var2.write(c.length());
            var2.write(c.getBytes());
            h.a(this.a, var2);
         }

         if (var3.getNextRecordID() == 1) {
            var3.addRecord(var2.toByteArray(), 0, var2.size());
         } else {
            var3.setRecord(1, var2.toByteArray(), 0, var2.size());
         }

         var3.closeRecordStore();
      } catch (Exception var4) {
         var4.printStackTrace();
      }
   }

   public final boolean b() {
      boolean var1 = false;

      try {
         RecordStore var2;
         if ((var2 = RecordStore.openRecordStore("ESO", false)) != null) {
            byte[] var3 = var2.getRecord(1);
            int var4 = 0;
            var4 = 0 + f.length;
            var4++;
            var1 = var3[var4] == 1;
         }

         var2.closeRecordStore();
      } catch (RecordStoreNotFoundException var5) {
      } catch (Exception var6) {
         var6.printStackTrace();
      }

      return var1;
   }

   public final void h() {
      this.b(true);
   }

   public final void b(boolean var1) {
      boolean var2 = false;

      try {
         RecordStore var3;
         if ((var3 = RecordStore.openRecordStore("ESO", false)) != null) {
            byte[] var4 = var3.getRecord(1);
            int var5 = 0;

            for (int var10 = 0; var10 < f.length; var10++) {
               f[var10] = var4[var5++];
            }

            this.o = var4[var5++] == 1;
            this.r = 0;
            a((byte)6);
            this.repaint();
            this.serviceRepaints();
            if (var4[var5++] == 1) {
               byte var6 = var4[var5++];
               String var7 = new String(var4, var5, var6);
               var5 += var6;
               if (var1) {
                  this.a(var7);
               } else {
                  c = var7;
               }

               this.a = a[0] = h.a(var4, var5);
            }

            var3.closeRecordStore();
         }
      } catch (RecordStoreNotFoundException var8) {
      } catch (Exception var9) {
         var9.printStackTrace();
      }
   }

   public final void a(int var1, boolean var2, int var3, int var4) {
      int var5 = var3 * g + var4;
      if (h < d.length - 4 && var5 < this.f && var5 >= 0) {
         if (var2) {
            ((byte[])a.elementAt(a.size() - 1))[var5] = -45;
         } else {
            ((byte[])a.elementAt(a.size() - 1))[var5] = 22;
         }

         d[h++] = (byte)var3;
         d[h++] = (byte)var4;
         d[h++] = (byte)var1;
      }
   }

   public static final void e(String var0) {
      h(var0);
      a((byte)10);
   }

   public static final void d(int var0, int var1) {
      i = (short)var0;
      j = (short)var1;
   }

   public static final void a(String var0, int var1) {
      if (var1 != 65535 && var1 != 0 || !a.a) {
         short var2 = 0;
         boolean var3 = false;
         boolean var4 = false;
         boolean var5 = false;
         boolean var6 = false;
         boolean var7 = false;
         char[] var8 = null;
         Object var9 = null;
         if (var1 == 65535) {
            var1 = 0;
         }

         b = null;
         c = null;
         b();
         var8 = new a().a(var1);
         if (var1 == 0) {
            a = var8;
            var9 = b = new short[a.a(var1) + 1];
         } else {
            b = var8;
            var9 = c = new short[a.a(var1) + 1];
         }

         for (int var16 = 1; var16 < ((Object[])var9).length; var16++) {
            ((Object[])var9)[var16] = -1;
         }

         int var11 = 0;
         int var12 = 0;
         String var13 = null;

         for (int var17 = 1; var17 <= a.b(var1); var17++) {
            while (var8[var12] != '|') {
               var12++;
            }

            if (var11 != 0) {
               var11 += 2;
            }

            int var14 = (var13 = new String(var8, var11, var12 - var11)).indexOf(32);
            var2 = Integer.valueOf(var13.substring(0, var14)).shortValue();
            ((Object[])var9)[var2] = (short)(var11 + var14 + 1);
            var11 = var12++ + 1;
         }

         if (var1 == 0) {
            b = (short[])var9;
         } else {
            c = (short[])var9;
         }

         b();
      }
   }

   public static final String a(int var0) {
      int var1 = 0;
      if (b != null && var0 >= 0 && var0 < b.length && b[var0] != -1) {
         for (int var3 = b[var0]; var3 < a.length && a[var3] != '|'; var3++) {
            var1++;
         }

         return new String(a, b[var0], var1);
      } else if (c != null && var0 >= 0 && var0 < c.length && c[var0] != -1) {
         for (int var2 = c[var0]; var2 < b.length && b[var2] != '|'; var2++) {
            var1++;
         }

         return new String(b, c[var0], var1);
      } else {
         return "";
      }
   }

   public static final int b(String var0) {
      char[] var1 = var0.toCharArray();
      int var2 = 0;
      boolean var3 = false;
      int var4 = 0;
      int var5 = 0;

      for (int var8 = 1; var8 < b.length; var8++) {
         if (b[var8] != -1) {
            var2 = 0;

            for (int var6 = b[var8]; var6 < a.length && a[var6] != '|'; var6++) {
               var2++;
            }

            if (var1.length == var2) {
               var4 = b[var8];

               for (var5 = 0; var4 < b[var8] + var2 && var1[var5] == a[var4]; var5++) {
                  var4++;
               }

               if (var5 == var2) {
                  return var8;
               }
            }
         }
      }

      return -1;
   }

   public final void f(String var1) {
      this.r = a - 10;
      this.v = this.r - g.a(this.b, 54) - 13;
      this.w = Math.min(b >> 1, g.b(this.b, 51)) - 4;
      a(var1, this.b, this.v);
   }

   public static final void a(String var0, Vector var1, int var2) {
      int var3 = 0;
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      int var7 = 0;
      char var8 = '\u0000';
      if ((var7 = var0.indexOf("ACTION_KEY")) != -1) {
         var0 = var0.substring(0, var7) + b[8] + var0.substring(var7 + "ACTION_KEY".length());
      }

      if ((var7 = var0.indexOf("TOGGLE_WEAPON_KEY")) != -1) {
         var0 = var0.substring(0, var7) + b[f[2]] + var0.substring(var7 + "TOGGLE_WEAPON_KEY".length());
      }

      if ((var7 = var0.indexOf("QUICK_HEALTH_KEY")) != -1) {
         var0 = var0.substring(0, var7) + b[f[0]] + var0.substring(var7 + "QUICK_HEALTH_KEY".length());
      }

      if ((var7 = var0.indexOf("QUICK_MAGIKA_KEY")) != -1) {
         var0 = var0.substring(0, var7) + b[f[1]] + var0.substring(var7 + "QUICK_MAGIKA_KEY".length());
      }

      if (f != null) {
         var0 = f + ": " + var0;
      } else if (var0.indexOf(":") != -1) {
         f = var0.substring(0, var0.indexOf(":"));
      }

      var1.removeAllElements();

      for (var3 = 0; var3 < var0.length() - 1; var3++) {
         var8 = var0.charAt(var3);
         var4 = c.substringWidth(var0, var6, var3 - var6 + 1);
         if (var8 == ' ') {
            var5 = var3;
         }

         if (var4 >= var2 && var5 > 0) {
            var1.addElement(var0.substring(var6, var5));
            var6 = var3 = var5 + 1;
            var5 = 0;
         }
      }

      if (var3 > var6) {
         var1.addElement(var0.substring(var6, var3 + 1));
      }
   }

   public final void i() {
      this.u = -1;
      this.r = true;
      this.g = true;
      this.c = System.currentTimeMillis();
   }

   public final void a(Graphics var1) {
      boolean var2 = false;
      boolean var3 = true;
      int var4 = this.s;
      int var5 = this.t;
      int var6 = 0;
      int var7 = 0;
      g.a(var1, this.b, 51, 5, 5);

      for (var6 = 0; var6 <= (this.r - 5 - g.a(this.b, 51) - g.a(this.b, 50)) / g.a(this.b, 52); var6++) {
         g.a(var1, this.b, 52, 5 + g.a(this.b, 51) + var6 * g.a(this.b, 52), 5);
      }

      g.a(var1, this.b, 50, 5 + g.a(this.b, 51) + var6 * g.a(this.b, 52), 5);
      var7 = var6;
      var1.setColor(16777215);
      var1.setFont(c);

      for (var6 = 0; var6 < this.b.size() && var3; var6++) {
         String var8 = (String)this.b.elementAt(var6);
         if (var5 - this.u >= this.t && var5 - this.u <= this.t + this.w - c.getHeight()) {
            var2 |= var6 == 0;
            if (f != null && var8.startsWith(f) && var6 == 0) {
               var8 = var8.substring(f.length() + 2);
               var1.setColor(6684672);
               var1.drawString(f + ": ", var4, var5 - this.u, 0);
               var1.setColor(16777215);
               var1.drawString(var8, var4 + c.stringWidth(f + ": "), var5 - this.u, 0);
            } else {
               var1.drawString(var8, var4, var5 - this.u, 0);
            }
         }

         var3 = (var5 += c.getHeight() + 1) - this.u < this.t + this.w - c.getHeight() - 1;
      }

      this.r = var6 == this.b.size() && var3;
      if (!var2) {
         g.a(var1, this.b, 54, 5 + g.a(this.b, 51) + var7 * g.a(this.b, 52) - g.a(this.b, 54) + 3, 8);
      }

      if (!this.r) {
         g.a(var1, this.b, 53, 5 + g.a(this.b, 51) + var7 * g.a(this.b, 52) - g.a(this.b, 53) + 3, 5 + g.b(this.b, 51) - g.b(this.b, 53) - 3);
      }
   }

   public final void d(int var1) {
      if (this.g) {
         if (var1 == 3 && this.u > -1) {
            this.u -= 4;
         } else if (var1 == 4 && !this.r) {
            this.u += 4;
         } else {
            if (var1 == 7 && System.currentTimeMillis() - this.c >= 1000L) {
               this.g = false;
               this.a.n = 0;
               this.p = 1;
            }
         }
      }
   }

   public static final void g(String var0) {
      f = var0;
   }

   public final void hideNotify() {
      if (!this.c()) {
         super.hideNotify();
         this.d = true;
         if (m != 8 && m != 21 && m != 15 && m != 10) {
            if (m != 22) {
               n = m;
            }

            m = 22;
            f.a = true;
         }
      }
   }

   public final void showNotify() {
      if (!this.c()) {
         super.showNotify();
         this.d = false;
      }
   }

   public final boolean c() {
      return m == 6 || m == 7 || m == 15;
   }

   static {
      g = new byte[f.length];
   }
}
