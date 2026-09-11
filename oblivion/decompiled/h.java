import java.io.ByteArrayOutputStream;
import javax.microedition.lcdui.Graphics;

public final class h {
   private static byte a = -52;
   private static byte b = -39;
   private static short a = 300;
   private static short b = 200;
   private static final byte[] a = new byte[]{0, 4, 8, 12, 16, 20, 24, 25};
   public static final short[] a = new short[]{
      0, 0, 100, 210, 340, 500, 700, 950, 1260, 1640, 2100, 2650, 3300, 4060, 4940, 5950, 7100, 8400, 9860, 11490, 13300, 15300, 17500, 19910, 22540, 25400
   };
   private static final short[] b = new short[]{
      0, 10, 12, 15, 19, 24, 30, 37, 45, 54, 64, 75, 87, 100, 114, 129, 145, 162, 180, 199, 219, 240, 262, 285, 309, 334
   };
   private static int[] a = new int[2];
   private static int[] b = new int[2];
   private static int[] c = new int[2];
   private static int[] d = new int[2];
   private static int[] e = new int[3];

   public static final j a(String var0, byte var1) {
      j var2 = new j();
      boolean var3 = false;
      var2.b = var0;
      var2.c = var1;
      var2.a[0] = 0;
      var2.a[1] = 0;
      var2.a = g.a(var0);

      for (int var4 = 0; var4 < var2.n.length; var4++) {
         var2.n[var4] = -1;
      }

      var2.a = (byte)g.a(var2.a, 1);
      var2.b = (byte)(var2.a >> 1);
      return var2;
   }

   public static final j a(byte[] var0, int var1) {
      j var2 = new j();
      byte var3 = 0;
      byte var4 = 0;
      boolean var5 = false;
      var2.c = var0[var1++];
      var2.f = var0[var1++];
      var2.b = ((char)var0[var1++] & 255) << 16 | ((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0;
      var2.o = (byte)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.s = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.t = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.v = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.w = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.x = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.u = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.j = (byte)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.E = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.F = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.r = var0[var1++];
      b.b = ((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0;
      var3 = var0[var1++];
      var2.b = new String(var0, var1, var3);
      var2.c = "Champion";
      var1 += var3;
      var4 = var0[var1++];
      a(var2, var2.f, true);

      for (int var43 = 0; var43 < var2.k.length; var43++) {
         var2.k[var43] = 0;
      }

      for (int var44 = 0; var44 < var4; var44++) {
         int var6 = (char)var0[var1++];
         char var7 = (char)var0[var1++];
         boolean var8;
         if (var8 = (var6 & 128) == 128) {
            int var45;
            var6 = (var45 = var6 & -129) & 0xFF;
         }

         a(var2, var6, b.a.b(var6, var7), var8);
      }

      f(var2);
      var2.a[0] = 0;
      var2.a[1] = 0;
      var2.a = g.a(var2.b);
      var2.a = (byte)g.a(var2.a, 1);
      var2.b = (byte)(var2.a >> 1);
      f(var2);
      return var2;
   }

   public static final void a(j var0) {
      boolean var1 = false;

      for (int var2 = 0; var2 < 25; var2++) {
         if (b.a[var2] != null) {
            b.a[var2].a = null;
         }
      }

      var0.a[0] = 0;
      var0.a[1] = 0;
      var0.l = -1;
      var0.m = -1;
      var0.n = -1;
      var0.q = 0;
      var0.g = -1;
      var0.a = null;
      var0.i = 0;
      var0.a = null;
      var0.Q = 0;
      var0.c = 16711680;
      var0.e = 0;
      var0.j[0] = -1;
      var0.j[1] = -1;
      var0.k = 0;
      var0.l = 0;
      var0.b = null;
      var0.q = var0.o = (short)(var0.o * 4 + (var0.s + var0.O) * 2 + var0.x * 2 + var0.I);
      var0.d = (short)('鱀' / var0.o);
      var0.r = var0.p = (short)(var0.o * 4 + var0.t * 2 + var0.J);
      var0.f = (short)('鱀' / var0.p);
      f(var0);
      e(var0);
   }

   private static final void d(j var0) {
      var0.i[0] = var0.b[0] - var0.b[1] >> 3;
      var0.i[1] = var0.b[0] + var0.b[1] >> 4;
   }

   public static final void b(j var0) {
      var0.b[0] = (byte)(var0.b[0] >> 7);
      var0.b[1] = (byte)(var0.b[1] >> 7);
      var0.c[0] = (byte)(var0.c[0] >> 7);
      var0.c[1] = (byte)(var0.c[1] >> 7);
      var0.d[0] = (byte)(var0.d[0] >> 7);
      var0.d[1] = (byte)(var0.d[1] >> 7);
      e(var0);
   }

   public static final boolean a(j var0) {
      if (var0 == null) {
         return false;
      } else if (var0.p == 0) {
         return false;
      } else if (var0.b[0] < 0) {
         return true;
      } else if (var0.b[1] >= b.g) {
         return true;
      } else if (var0.d[0] >= b.f) {
         return true;
      } else if (var0.d[1] < 0) {
         return true;
      } else if (a(var0, (byte)1)) {
         return true;
      } else {
         return a(var0, (byte)2) ? true : a(var0, (byte)3);
      }
   }

   private static final boolean a(j var0, byte var1) {
      byte var2 = 0;
      int var3 = 0;
      int var4 = 0;
      if (b.a == null) {
         return false;
      }

      if (var0 == null) {
         return false;
      }

      if (var0.b[0] * b.g + var0.b[1] > b.a.length) {
         return true;
      }

      if (var0.c[0] * b.g + var0.c[1] > b.a.length) {
         return true;
      }

      if (var0.d[0] * b.g + var0.d[1] > b.a.length) {
         return true;
      }

      switch (var1) {
         case 1:
            var2 = b.a[var0.b[0] * b.g + var0.b[1]];
            var3 = var0.b[0] % 128;
            var4 = var0.b[1] % 128;
            break;
         case 2:
            var2 = b.a[var0.c[0] * b.g + var0.c[1]];
            var3 = var0.c[0] % 128;
            var4 = var0.c[1] % 128;
            break;
         case 3:
            var2 = b.a[var0.d[0] * b.g + var0.d[1]];
            var3 = var0.d[0] % 128;
            var4 = var0.d[1] % 128;
      }

      if (var2 == 0) {
         return false;
      }

      switch (var2) {
         case 1:
            return true;
         case 2:
            if (var3 <= var4) {
               return true;
            }

            return false;
         case 3:
            if (var4 >= var3) {
               return true;
            }

            return false;
         case 4:
            if (var4 <= var3) {
               return true;
            }

            return false;
         case 5:
            if (var3 >= var4) {
               return true;
            }

            return false;
         default:
            return false;
      }
   }

   public static final void a(j var0, int var1, long var2) {
      System.out.println("moveInWorld()  ");
      if (var0 != null) {
         var0.g = (short)(var0.g + var2);
         if (var0.g > 50) {
            if (var0.g > 400) {
               var0.g = 50;
            }

            int var4 = var0.w / (1000 / var0.g);
            switch (var1) {
               case 1:
                  d(var0, 0, var4);
                  break;
               case 2:
                  d(var0, 0, -var4);
                  break;
               case 3:
                  d(var0, var4, 0);
                  break;
               case 4:
                  d(var0, -var4, 0);
            }

            if (a(var0)) {
               c(var0);
            }

            var0.g = 0;
         }
      }
   }

   private static final void d(j var0, int var1, int var2) {
      var0.e[0] = var0.b[0];
      var0.e[1] = var0.b[1];
      var0.b[0] = var0.b[0] + var1;
      var0.b[1] = var0.b[1] + var2;
      var0.c[0] = var0.c[0] + var1;
      var0.c[1] = var0.c[1] + var2;
      var0.d[0] = var0.d[0] + var1;
      var0.d[1] = var0.d[1] + var2;
      d(var0);
      b(var0);
      if (var1 > 0) {
         var0.d = 3;
      } else if (var1 < 0) {
         var0.d = 4;
      } else if (var2 > 0) {
         var0.d = 1;
      } else if (var2 < 0) {
         var0.d = 2;
      }

      var0.a = 500;
   }

   public static final void a(j var0, int var1, int var2) {
      a[0] = var0.b;
      a[1] = 0;
      b[0] = var0.a;
      b[1] = 0;
      c[0] = 0;
      c[1] = 0;
      d[0] = 0;
      d[1] = 0;
      b.b(c, a);
      b.b(d, b);
      var0.b[0] = var1;
      var0.b[1] = var2;
      var0.c[0] = var1 + c[0];
      var0.c[1] = var2 + c[1];
      var0.d[0] = var1 + d[0];
      var0.d[1] = var2 + d[1];
      d(var0);
      b(var0);
   }

   private static final void e(j var0) {
      if (var0.d[0] > var0.c[0]) {
         var0.a[0] = var0.d[0];
         var0.a[1] = var0.d[1];
      } else if (var0.c[1] <= var0.d[1] && var0.c[0] <= var0.b[0]) {
         var0.a[0] = var0.b[0];
         var0.a[1] = var0.b[1];
      } else {
         var0.a[0] = var0.c[0];
         var0.a[1] = var0.c[1];
      }
   }

   public static final void c(j var0) {
      a(var0, var0.e[0], var0.e[1]);
   }

   public static final byte a(j var0, byte[] var1, byte[] var2) {
      boolean var3 = false;
      var0.l = -1;
      var0.m = -1;
      if (var1 != null && var2 != null) {
         e[0] = var0.b[0] * b.g + var0.b[1];
         e[1] = var0.c[0] * b.g + var0.c[1];
         e[2] = var0.d[0] * b.g + var0.d[1];

         for (int var4 = 0; var4 < e.length; var4++) {
            if (e[var4] < 0 || e[var4] >= var1.length) {
               return -1;
            }

            if (var1[e[var4]] != 0 && var1[e[var4]] != -1) {
               var0.l = var1[e[var4]];
               var0.m = var2[e[var4]];
               return var1[e[var4]];
            }
         }

         return -1;
      } else {
         return -1;
      }
   }

   public static final void b(j var0, int var1, int var2) {
      var0.j[0] = var1;
      var0.j[1] = var2;
      var0.e = 1;
   }

   public static final void a(j var0, long var1, boolean var3) {
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      if (var0 != null) {
         var0.b = (short)(var0.b + var1);
         var0.a = (int)(var0.a + var1);
         var0.e = (int)(var0.e + var1);
         if (var0.b > 125 && var0.q == 0) {
            g.a(var0.a, var0.d + a[var0.e]);
            var0.b = 0;
         }

         if (var0.q == 0) {
            if (var0.j[0] != -1) {
               var0.g = (short)(var0.g + var1);
               if (var0.g >= 50) {
                  if (var0.g > 100) {
                     var0.g = 100;
                  }

                  var6 = var0.w / (1000 / var0.g);
                  if (var0.b[0] < var0.j[0]) {
                     var4 = Math.min(var6, var0.j[0] - var0.b[0]);
                  } else if (var0.b[0] > var0.j[0]) {
                     var4 = Math.max(-var6, var0.j[0] - var0.b[0]);
                  } else if (var0.b[1] < var0.j[1]) {
                     var5 = Math.min(var6, var0.j[1] - var0.b[1]);
                  } else if (var0.b[1] > var0.j[1]) {
                     var5 = Math.max(-var6, var0.j[1] - var0.b[1]);
                  } else {
                     var0.j[0] = -1;
                     if (var0.e != 2) {
                        var0.e = 0;
                     }
                  }

                  d(var0, var4, var5);
                  var0.g = 0;
               }
            } else if (var0.c == 1 && var0.a > 0) {
               var0.a = (short)(var0.a - var1);
               if (var0.a <= 0) {
                  var0.e = 0;
               }
            }

            if (var0.k > 0) {
               var0.k = (short)(var0.k - var1);
               var0.l = (short)(var0.l - var1);
               if (var0.l <= 0) {
                  i.a(8, var0);
                  var0.l = 1000;
                  a(var0.x, var0, var0.b, false, true);
               }
            } else if (var0.w == -47) {
               var0.w = -1;
            }

            if (var0.y == 2) {
               var0.n = (short)(var0.n - var1);
            }

            if (var0.c == 1) {
               if (var0.q < var0.o) {
                  var0.c = (short)(var0.c + var1);
                  if (var0.c >= var0.d) {
                     if (var0.q < var0.o) {
                        var0.q++;
                     }

                     var0.c = 0;
                     f(var0);
                  }
               }

               if (var0.r < var0.p) {
                  var0.e = (short)(var0.e + var1);
                  if (var0.e >= var0.f) {
                     if (var0.r < var0.p) {
                        var0.r++;
                     }

                     var0.e = 0;
                     f(var0);
                  }
               }

               if (var0.P > 0) {
                  if (var0.j >= var0.P) {
                     var0.j = 0;
                     var0.P = 0;
                     var0.J = 0;
                     var0.L = 0;
                     var0.K = 0;
                     var0.M = 0;
                     var0.N = 0;
                     var0.H = 0;
                     var0.w = -1;
                     i.a(var0.h);
                     if (var0.J != 0) {
                        var0.r = var0.p = (short)(var0.o * 4 + var0.t * 2 + var0.J);
                        var0.f = (short)('鱀' / var0.p);
                     }

                     if (var0.O != 0) {
                        var0.O = 0;
                        var0.o = (short)(var0.o * 4 + (var0.s + var0.O) * 2 + var0.x * 2 + var0.I);
                        var0.d = (short)('鱀' / var0.o);
                     }

                     f(var0);
                  }

                  var0.j = (short)(var0.j + var1);
               }
            } else if (var0.z == 1 && b(var0) && var0.a != null && var0.e >= var0.m) {
               if ((var3 || var0.a.c != 1) && a(var0, var0.a, true)) {
                  var0.a = null;
                  var0.b = null;
                  var0.e = 0;
               }

               var0.e = 0;
            }

            if (var0.a != null) {
               var0.h = (short)(var0.h + var1);
               if (var0.h > 50) {
                  var0.Q = (short)(var0.Q - 2);
                  var0.c = var0.c - var0.d;
                  if (var0.c <= 0 || Math.abs(var0.R - var0.Q) > 20) {
                     var0.c = 0;
                     var0.Q = 0;
                     var0.R = 0;
                     var0.a = null;
                  }

                  var0.h = 0;
               }
            }

            if (var0.G > 0) {
               var0.G = (short)(var0.G - var1);
               if (var0.G <= 0) {
                  var0.j = 0;
                  var0.P = 0;
                  var0.J = 0;
                  var0.L = 0;
                  var0.K = 0;
                  var0.M = 0;
                  var0.N = 0;
                  var0.H = 0;
                  var0.w = -1;
                  i.a(var0.h);
                  if (var0.J != 0) {
                     var0.r = var0.p = (short)(var0.o * 4 + var0.t * 2 + var0.J);
                     var0.f = (short)('鱀' / var0.p);
                  }

                  if (var0.O != 0) {
                     var0.O = 0;
                     var0.o = (short)(var0.o * 4 + (var0.s + var0.O) * 2 + var0.x * 2 + var0.I);
                     var0.d = (short)('鱀' / var0.o);
                  }

                  f(var0);
                  return;
               }
            }
         } else {
            if (var0.i >= 250) {
               b.a(var0.c - 1);
            }

            var0.i = (short)(var0.i + var1);
         }
      }
   }

   public static final void a(j var0, Graphics var1, int[] var2) {
      if (var0.q != 1 && var0.e != 6) {
         g.a(
            var1,
            var0.a,
            -56,
            var0.i[0] + var2[0] + (g.a(var0.a, var0.d + a[0]) >> 1) - (g.a(var0.a, -56) >> 1),
            var0.i[1] + var2[1] - g.b(var0.a, -56) + 3 + (var0.e != 2 && var0.e != 3 ? 0 : 3)
         );
         g.a(var1, var0.a, var0.d + a[var0.e], var0.i[0] + var2[0], var0.i[1] + var2[1] - g.b(var0.a, var0.d + a[var0.e]));
         if (var0.q == 0 && var0.c != 1 && var0.r == 0) {
            int var3 = var0.i[0] + var2[0] + (g.a(var0.a, var0.d) >> 1) - 10;
            int var4 = var0.i[1] + var2[1] - g.b(var0.a, var0.d) - 6;
            var1.setColor(16777215);
            var1.drawRect(var3, var4, 20, 3);
            var1.setColor(16711680);
            var1.fillRect(var3 + 1, var4 + 1, 19 * var0.q / var0.o, 2);
         }

         if (var0.q == 0 && var0.a != null) {
            if (var0.Q == 0) {
               var0.R = var0.Q = (short)(var0.i[1] - g.b(var0.a, var0.d) - (var0.c == 1 ? 6 : 10));
               if (var0.a.equals(b.a(471))) {
                  var0.c = 65280;
                  var0.d = 8704;
               } else if (var0.a.equals(b.a(470))) {
                  var0.c = 255;
                  var0.d = 34;
               } else {
                  var0.c = 16711680;
                  var0.d = 2228224;
               }
            }

            var1.setColor(var0.c);
            var1.drawString(var0.a, var0.i[0] + var2[0] + (g.a(var0.a, var0.d) >> 1) - 10, var0.Q + var2[1], 0);
         }

         if (var0.g != -1) {
            int var5 = var0.i[0] + var2[0] + g.a(var0.a, var0.d) - 4;
            int var6 = var0.i[1] + var2[1] - g.b(var0.a, var0.d + a[var0.e]) - g.a(var0.a, -54) - 4;
            if (var0.c != 1) {
               var6 -= 8;
            }

            g.a(var1, var0.a, -54, var5, var6);
            if (var0.g != -2) {
               g.a(var1, var0.a, var0.g, var5, var6);
            }
         }
      } else {
         g.a(var1, var0.a, -55, var0.i[0] + var2[0] + (g.a(var0.a, var0.d + a[0]) >> 1) - (g.a(var0.a, -55) >> 1), var0.i[1] + var2[1] - g.b(var0.a, -55) + 3);
      }
   }

   public static final void a(j var0, byte var1) {
      if (var1 == 6) {
         var0.q = 1;
      } else if (var0.e != var1) {
         g.a(var0.a, var0.d + a[var1]);
      }

      var0.e = var1;
   }

   private static final void f(j var0) {
      boolean var1 = false;
      int[] var2 = b.a.a(4, var0.j);
      var0.i = (byte)var2[3];
      var0.z = 0;

      for (int var3 = 0; var3 < var0.n.length; var3++) {
         if (var0.n[var3] != -1) {
            var0.z = (short)(var0.z + b.a.a(1, var0.n[var3])[4]);
         }
      }

      switch (var0.f) {
         case 1:
            if (var0.o == 1) {
               var0.A = 5;
            }

            if (var0.o == 7) {
               var0.A = 10;
            }

            if (var0.o == 15) {
               var0.A = 15;
            }

            if (var2[2] == 4) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 7) {
                  var0.D = 110;
               }

               if (var0.o == 16) {
                  var0.D = 125;
                  return;
               }
            } else if (var0.j == 0) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 5) {
                  var0.D = 110;
               }

               if (var0.o == 15) {
                  var0.D = 125;
               }

               if (var0.o == 1) {
                  var0.C = 110;
               }

               if (var0.o == 5) {
                  var0.C = 125;
               }

               if (var0.o == 15) {
                  var0.C = 140;
                  return;
               }
            }
            break;
         case 2:
            if (var0.o == 1) {
               var0.A = 5;
            }

            if (var0.o == 7) {
               var0.A = 10;
            }

            if (var0.o == 15) {
               var0.A = 15;
            }

            if (var0.o == 1) {
               var0.C = 100;
            }

            if (var0.o == 10) {
               var0.C = 115;
            }

            if (var0.o == 20) {
               var0.C = 130;
            }

            if (var2[2] == 2) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 3) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 6) {
                  var0.D = 110;
               }

               if (var0.o == 15) {
                  var0.D = 125;
                  return;
               }
            }
            break;
         case 3:
            if (var0.o == 1) {
               var0.B = 0;
            }

            if (var0.o == 5) {
               var0.B = 3;
            }

            if (var0.o == 17) {
               var0.B = 10;
            }

            if (var0.o == 1) {
               var0.C = 100;
            }

            if (var0.o == 10) {
               var0.C = 115;
            }

            if (var0.o == 20) {
               var0.C = 130;
            }

            if (var2[2] == 1) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 2) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 3) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 6) {
                  var0.D = 110;
               }

               if (var0.o == 15) {
                  var0.D = 125;
                  return;
               }
            } else if (var0.j == 0) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 5) {
                  var0.D = 110;
               }

               if (var0.o == 15) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 0) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            }
            break;
         case 4:
            if (var0.o == 1) {
               var0.C = 100;
            }

            if (var0.o == 10) {
               var0.C = 115;
            }

            if (var0.o == 20) {
               var0.C = 130;
            }

            if (var2[2] == 1) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 4) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 7) {
                  var0.D = 110;
               }

               if (var0.o == 16) {
                  var0.D = 125;
                  return;
               }
            }
            break;
         case 5:
            if (var0.o == 1) {
               var0.B = 0;
            }

            if (var0.o == 5) {
               var0.B = 3;
            }

            if (var0.o == 17) {
               var0.B = 10;
            }

            if (var0.o == 1) {
               var0.C = 100;
            }

            if (var0.o == 10) {
               var0.C = 115;
            }

            if (var0.o == 20) {
               var0.C = 130;
            }

            if (var0.o == 1) {
               var0.C = 100;
            }

            if (var0.o == 7) {
               var0.C = 115;
            }

            if (var0.o == 17) {
               var0.C = 130;
            }

            if (var2[2] == 1) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 2) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 0) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            }
            break;
         case 6:
            if (var0.o == 1) {
               var0.B = 0;
            }

            if (var0.o == 5) {
               var0.B = 3;
            }

            if (var0.o == 17) {
               var0.B = 10;
            }

            if (var0.o == 1) {
               var0.C = 100;
            }

            if (var0.o == 10) {
               var0.C = 115;
            }

            if (var0.o == 20) {
               var0.C = 130;
            }

            if (var2[2] == 2) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 4) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 7) {
                  var0.D = 110;
               }

               if (var0.o == 16) {
                  var0.D = 125;
               }
            }
            break;
         case 7:
            if (var0.o == 1) {
               var0.A = 5;
            }

            if (var0.o == 7) {
               var0.A = 10;
            }

            if (var0.o == 15) {
               var0.A = 15;
            }

            if (var0.o == 1) {
               var0.C = 100;
            }

            if (var0.o == 10) {
               var0.C = 115;
            }

            if (var0.o == 20) {
               var0.C = 130;
               return;
            }
            break;
         case 8:
            if (var0.o == 1) {
               var0.C = 100;
            }

            if (var0.o == 10) {
               var0.C = 115;
            }

            if (var0.o == 20) {
               var0.C = 130;
            }

            if (var0.o == 1) {
               var0.C = 100;
            }

            if (var0.o == 7) {
               var0.C = 115;
            }

            if (var0.o == 17) {
               var0.C = 130;
            }

            if (var2[2] == 1) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 2) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            } else if (var2[2] == 0) {
               if (var0.o == 1) {
                  var0.D = 100;
               }

               if (var0.o == 8) {
                  var0.D = 110;
               }

               if (var0.o == 18) {
                  var0.D = 125;
                  return;
               }
            }
      }
   }

   public static final int a(int[] var0, int[] var1) {
      boolean var2 = false;
      int var3 = 0;
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      int var7 = 0;
      var6 = var0[0] - var1[0];
      var7 = var0[1] - var1[1];
      var6 = var6 < 0 ? -var6 : var6;
      var7 = var7 < 0 ? -var7 : var7;
      if (var6 < var7) {
         var4 = var6;
         var5 = var7;
      } else {
         var4 = var7;
         var5 = var6;
      }

      var3 = var5 * 1007 + var4 * 441;
      if (var5 < var4 << 4) {
         var3 -= var5 * 40;
      }

      return Math.abs(var3 + 512 >> 10);
   }

   private static final j a(j var0) {
      j var1 = null;
      int var2 = 16777215;
      int var3 = 0;
      boolean var4 = false;

      for (int var6 = 0; var6 < b.a.length; var6++) {
         if (b.a[var6] != null && b.a[var6].q != 1 && b.a[var6].r != var0.r && b.a[var6].c != var0.c && (var3 = a(var0.b, b.a[var6].b)) < var2) {
            var2 = var3;
            var1 = b.a[var6];
         }
      }

      return var1;
   }

   private static final void a(j var0, j var1) {
      int var2 = var0.b[0] - var1.b[0];
      int var3 = var0.b[1] - var1.b[1];
      if (Math.abs(var2) > Math.abs(var3)) {
         if (var2 > 0) {
            b(var0, var0.b[0] - 20, var0.b[1]);
         } else {
            b(var0, var0.b[0] + 20, var0.b[1]);
         }
      } else if (var3 > 0) {
         b(var0, var0.b[0], var0.b[1] - 20);
      } else {
         b(var0, var0.b[0], var0.b[1] + 20);
      }
   }

   private static final void b(j var0, j var1) {
      if (var0.i[0] < var1.i[0] && var0.i[1] > var1.i[1]) {
         var0.d = 2;
      } else if (var0.i[0] > var1.i[0] && var0.i[1] < var1.i[1]) {
         var0.d = 1;
      } else if (var0.i[0] < var1.i[0] && var0.i[1] < var1.i[1]) {
         var0.d = 3;
      } else {
         if (var0.i[0] > var1.i[0] && var0.i[1] > var1.i[1]) {
            var0.d = 4;
         }
      }
   }

   private static final boolean b(j var0) {
      j var1 = a(var0);
      int var2 = 0;
      if (var1 != null) {
         if ((var2 = a(var0.b, var1.b)) <= var0.E) {
            if (var2 >= var0.F) {
               if (var0.c != 1) {
                  a(var0, var1);
                  return false;
               }
            } else {
               var0.j[0] = -1;
               var0.a = var1;
               var0.e = 4;
               b(var0, var1);
            }
         } else if (var0.a != null && var0.y != 2) {
            var0.j[0] = -1;
            var0.a = null;
            var0.e = 0;
         }
      } else if (var0.a != null) {
         var0.a = null;
         var0.e = 0;
      }

      return true;
   }

   public static final void a(j var0, int[] var1) {
      int var2 = 0;
      if (var0 != null && var1 != null) {
         var0.o = var1;
         var0.o = (byte)var1[2];
         if (var0.c != 1) {
            var0.s = (short)var1[3];
            var0.t = (short)var1[4];
            var0.u = (short)var1[5];
            var0.v = (short)var1[6];
            var0.w = (short)var1[7];
            var0.x = (short)var1[8];
            var0.y = (short)var1[9];
            var0.E = (short)var1[14];
            var0.F = (short)var1[15];
            var0.j = (byte)var1[10];
            var0.y = (byte)var1[18];
            var2 = var1[11];
            var0.l = b.a.k[var1[19]];
            var0.t = (byte)(var0.y == 4 ? 1 : 0);
            if (var1[20] > 0) {
               var0.m = (short)(var1[20] * 1000);
            }

            if (var0.t == 1 || var0.y == 0) {
               var0.l = null;
            }

            if (var0.j > 0) {
               a(var0, 0, b.a.a(4, var0.j));
            }

            if (var2 > 0) {
               a(var0, 1, b.a.a(1, var2));
            }
         }

         var0.r = (byte)var1[13];
         var0.q = var0.o = (short)(var0.o * 4 + (var0.s + var0.O) * 2 + var0.x * 2 + var0.I);
         var0.d = (short)('鱀' / var0.o);
         var0.r = var0.p = (short)(var0.o * 4 + var0.t * 2 + var0.J);
         var0.f = (short)('鱀' / var0.p);
         if (var0.E == 0) {
            var0.E = a;
         }

         if (var0.F == 0) {
            var0.F = b;
         }

         f(var0);
      }
   }

   private static final boolean a(int var0, j var1, j var2, boolean var3, boolean var4) {
      if (var1 != null && (var2 != null || var4) && var1.u != 1) {
         int var5 = var1.A + (var1.A >> 1);
         int var6 = var1.B + (var1.B >> 1);
         int var7 = (var1.v + var1.z + var1.L >> 3) + var1.M;
         if (var4) {
            var7 = 0;
            var6 = -1000;
            var5 = -1000;
         }

         int var8 = var0 - var7;
         int var9 = b.a.nextInt() % 100;
         int var10 = b.a.nextInt() % 100;
         int var12;
         var5 = (var12 = var5 * var1.H) / 100;
         var9 = var9 < 0 ? -var9 : var9;
         var10 = var10 < 0 ? -var10 : var10;
         if (var1.a == null) {
            var1.a = var2;
         }

         if (var9 <= var5) {
            var1.a = b.a(471);
            var1.Q = 0;
         } else if (var10 <= var6) {
            var1.a = b.a(470);
            var1.Q = 0;
         } else if (var8 > 0) {
            if (var1.c != 1 && var1.a != null && !var4) {
               var1.E = (short)Math.max(a(var1.b, var1.a.b), var1.E);
            }

            if (var2 != null && var2.t == 0) {
               b.a.nextInt();
            }

            var1.q = (short)(var1.q - var8);
            var1.a = (var3 ? b.a(472) : "") + Integer.toString(var8);
            var1.Q = 0;
            var1.q = (byte)(var1.q <= 0 ? 1 : 0);
            if (var1.q == 1) {
               if (var2 != null) {
                  var2.a = 0;
                  c(var2, var1.o);
               }

               b.a.nextInt();
               var1.e = 6;
               e(var1);
               if (var1.k >= 0) {
                  e.a((char)var1.k);
               }

               int var11;
               if (var1.s == 1 && (var11 = b.a.a()) != 0) {
                  b.a.a(var11, false, var1.c[0], var1.c[1]);
               }
            }
         }

         return var1.q == 1;
      } else {
         return false;
      }
   }

   private static final boolean c(j var0) {
      int var1 = 0;
      int var2 = 0;
      boolean var3 = false;
      boolean var4 = false;
      int var5 = 0;
      int var6 = 0;
      int var7 = 0;
      Object var8 = null;
      byte[][] var9 = new byte[][]{{-1, 0}, {0, -1}, {0, 0}, {0, 1}, {1, 0}};
      if (var0.A == 1) {
         if (var0.n <= -1000 && b.a[0].q == 0) {
            for (byte[] var14 = (byte[])b.a.elementAt(0); var14 != null && !var4 && var5 < 100; var5++) {
               var4 = true;
               var1 = Math.abs(b.a[0].b[0] + b.a.nextInt() % 500);
               var2 = Math.abs(b.a[0].b[1] + b.a.nextInt() % 500);
               var6 = var1 >> 7;
               var7 = var2 >> 7;

               for (int var11 = 0; var11 < var9.length; var11++) {
                  int var10;
                  if ((var10 = (var6 + var9[var11][0]) * b.g + var7 + var9[var11][1]) >= 0 && var10 < var14.length) {
                     if (b.a[var10] != 0 || var14[var10] == 0) {
                        var4 = false;
                        break;
                     }
                  } else {
                     var4 = false;
                  }
               }
            }

            a(var0, var1, var2);
            i.a(8, var0.b[0], var0.b[1]);
            if (var4) {
               var0.A = 0;
               return true;
            }
         }
      } else if (b.a[0].q == 0) {
         i.a(8, var0.b[0], var0.b[1]);
         var0.j[0] = -1;
         var0.j[1] = -1;
         a(var0, -10000, -10000);
         var0.A = 1;
      }

      return false;
   }

   public static final boolean a(j var0, j var1, boolean var2) {
      if (var0 != null && var1 != null) {
         if (var0.c != 1 && (var0.l != null || var0.t == 1) && var2) {
            c(var0, false);
            if (var0.y == 3) {
               var0.l = null;
               var0.F = (short)(var0.F >> 1);
            } else if (var0.y == 2 && var0.n <= 0 && c(var0)) {
               var0.n = (short)(Math.abs(b.a.nextInt()) % 2000 + 2000);
            }

            return false;
         } else {
            int var3 = (var0.s + var0.O + var0.i >> 1) + var0.K + var0.N;
            int var4 = b.a.nextInt() % 16;
            boolean var5 = false;
            int var6;
            var3 = (var6 = var3 * var0.D) / 100;
            if (var0.l != null) {
               if (var0.o >= var0.l[10]) {
                  var3 = var0.l[5];
               } else if (var0.o >= var0.l[9]) {
                  var3 = var0.l[4];
               } else {
                  var3 = var0.l[3];
               }

               if (var0.c != 1 && var0.l[2] != 4) {
                  var3 >>= 1;
               }
            }

            if ((var4 < 0 ? -var4 : var4) == 1) {
               var3 = Math.max(var1.q >> 2, var3 << 1);
               var5 = true;
            }

            return a(var3, var1, var0, var5, false);
         }
      } else {
         return false;
      }
   }

   private static final void c(j var0, int var1) {
      if (var0.d != null) {
         var0 = var0.d;
      }

      if (var0.c == 1) {
         var0.b = var0.b + b[var1];
         if (var0.o < 25 && var0.b >= a[var0.o + 1]) {
            short var2 = var0.s;
            short var3 = var0.t;
            short var4 = var0.u;
            short var5 = var0.v;
            short var6 = var0.x;
            short var7 = var0.y;
            StringBuffer var8 = new StringBuffer(b.a(41));
            var0.o++;
            var0.s++;
            var0.t++;
            var0.u++;
            var0.v++;
            var0.w++;
            var0.x++;
            var0.y++;
            g(var0);
            var8.append(" ");
            var8.append(var0.o);
            var8.append(": +");
            var8.append(var0.s - var2);
            var8.append(" ");
            var8.append(b.a(415));
            var8.append(", +");
            var8.append(var0.t - var3);
            var8.append(" ");
            var8.append(b.a(416));
            var8.append(", +");
            var8.append(var0.u - var4);
            var8.append(" ");
            var8.append(b.a(417));
            var8.append(", +");
            var8.append(var0.v - var5);
            var8.append(" ");
            var8.append(b.a(418));
            var8.append(", +");
            var8.append(var0.x - var6);
            var8.append(" ");
            var8.append(b.a(419));
            var8.append(", +");
            var8.append(var0.y - var7);
            var8.append(" ");
            var8.append(b.a(420));
            var0.o = (short)(var0.o * 4 + (var0.s + var0.O) * 2 + var0.x * 2 + var0.I);
            var0.d = (short)('鱀' / var0.o);
            var0.p = (short)(var0.o * 4 + var0.t * 2 + var0.J);
            var0.f = (short)('鱀' / var0.p);
            f(var0);
            b.a(var8.toString(), 30, 4, 3);
            return;
         }

         b.a(b[var1] + " " + b.a(42) + "!!!", 3, 4, 1);
      }
   }

   private static final void g(j var0) {
      switch (var0.f) {
         case 1:
            if (var0.o == 5) {
               var0.w = (short)(var0.w + 25);
               return;
            }

            if (var0.o == 10) {
               var0.v++;
               return;
            }

            if (var0.o == 15) {
               var0.x = (short)(var0.x + 2);
               return;
            }

            if (var0.o == 20) {
               var0.s = (short)(var0.s + 2);
               return;
            }
            break;
         case 2:
            if (var0.o == 5) {
               var0.v++;
               return;
            }

            if (var0.o == 10) {
               var0.u++;
               return;
            }

            if (var0.o == 15) {
               var0.t = (short)(var0.t + 2);
               return;
            }

            if (var0.o == 20) {
               var0.v = (short)(var0.v + 2);
               return;
            }
            break;
         case 3:
            if (var0.o == 5) {
               var0.s++;
               return;
            }

            if (var0.o == 10) {
               var0.x++;
               return;
            }

            if (var0.o == 15) {
               var0.x = (short)(var0.x + 2);
               return;
            }

            if (var0.o == 20) {
               var0.s = (short)(var0.s + 2);
               return;
            }
            break;
         case 4:
            if (var0.o == 5) {
               var0.w = (short)(var0.w + 25);
               return;
            }

            if (var0.o == 10) {
               var0.v = (short)(var0.v + 2);
               return;
            }

            if (var0.o == 15) {
               var0.s++;
               return;
            }

            if (var0.o == 20) {
               var0.s = (short)(var0.s + 2);
               return;
            }
            break;
         case 5:
            if (var0.o == 5) {
               var0.s++;
               return;
            }

            if (var0.o == 10) {
               var0.x++;
               return;
            }

            if (var0.o == 15) {
               var0.s = (short)(var0.s + 2);
               return;
            }

            if (var0.o == 20) {
               var0.x = (short)(var0.x + 2);
               return;
            }
            break;
         case 6:
            if (var0.o == 5) {
               var0.v++;
               return;
            }

            if (var0.o == 10) {
               var0.u++;
               return;
            }

            if (var0.o == 15) {
               var0.t = (short)(var0.t + 2);
               return;
            }

            if (var0.o == 20) {
               var0.u = (short)(var0.u + 2);
               return;
            }
            break;
         case 7:
            if (var0.o == 5) {
               var0.t++;
               return;
            }

            if (var0.o == 10) {
               var0.u++;
               return;
            }

            if (var0.o == 15) {
               var0.u = (short)(var0.u + 2);
               return;
            }

            if (var0.o == 20) {
               var0.t = (short)(var0.t + 2);
               return;
            }
            break;
         case 8:
            if (var0.o == 5) {
               var0.u++;
               return;
            }

            if (var0.o == 10) {
               var0.s++;
               return;
            }

            if (var0.o == 15) {
               var0.t = (short)(var0.t + 2);
               return;
            }

            if (var0.o == 20) {
               var0.u = (short)(var0.u + 2);
            }
      }
   }

   public static final byte a(j var0, byte[] var1) {
      boolean var2 = false;
      var0.n = -1;
      if (var1 == null) {
         return var0.n;
      }

      int[] var3 = new int[]{var0.b[0] * b.g + var0.b[1], var0.c[0] * b.g + var0.c[1], var0.d[0] * b.g + var0.d[1]};

      for (int var4 = 0; var4 < var3.length; var4++) {
         if (var3[var4] < 0 || var3[var4] >= var1.length) {
            return -1;
         }

         if (var1[var3[var4]] >= 0 && var1[var3[var4]] < 255) {
            var0.n = var1[var3[var4]];
            return var1[var3[var4]];
         }
      }

      if (var0.t == 0 && var0.l == null) {
         var0.a = 500;
         var0.e = 4;
      }

      if (var0.e >= var0.m) {
         var0.e = 0;
         if (var0.l == null && var0.t != 1) {
            b(var0);
            if (var0.a != null) {
               b(var0, var0.a);
               if (a(var0, var0.a, true)) {
                  var0.a = null;
                  var0.b = null;
                  var0.e = 0;
               }
            }
         } else {
            c(var0, true);
         }
      }

      return -1;
   }

   private static final void c(j var0, boolean var1) {
      int var2 = 0;
      boolean var3 = false;
      if (var0.t == 1) {
         if (var0.c == 1) {
            var0.a = 500;
            var0.e = 7;
         }

         i.a(11, var0.d, var0);
      } else {
         if (var0.o >= var0.l[10]) {
            if (var0.r < var0.l[13] && var1) {
               return;
            }

            var2 = var0.l[5];
            var0.r = (short)(var0.r - var0.l[13]);
         } else if (var0.o >= var0.l[9]) {
            if (var0.r < var0.l[12] && var1) {
               return;
            }

            var2 = var0.l[4];
            var0.r = (short)(var0.r - var0.l[12]);
         } else {
            if (var0.r < var0.l[11] && var1) {
               return;
            }

            var2 = var0.l[3];
            var0.r = (short)(var0.r - var0.l[11]);
         }

         switch (var0.l[2]) {
            case 0:
               var0.G = (short)var0.l[6];
               var0.L = (short)var2;
               var0.w = -48;
               i.a(var0.h);
               var0.h = (byte)i.a(9, var0, 5000);
               break;
            case 1:
               var0.G = (short)var0.l[6];
               var0.N = (short)var2;
               var0.w = -50;
               i.a(var0.h);
               var0.h = (byte)i.a(9, var0, 5000);
               break;
            case 2:
               if (var0.c != null) {
                  b.a(var0.c.c - 1);
               }

               var0.c = b.a.a("/oh_scamp.cml", var0.b[0], var0.b[1], var0.o);
               var0.c.d = var0;
               b(var0.c, false);
            case 4:
               for (int var6 = 0; var6 < b.a.length; var6++) {
                  if (b.a[var6] != null && b.a[var6] != var0 && b.a[var6].r != var0.r && a(var0.b, b.a[var6].b) <= var0.l[14]) {
                     a(var0, b.a[var6], var2, var0.l[6]);
                  }
               }
               break;
            case 3:
               if (var0.l[1] == 61618) {
                  for (int var5 = 0; var5 < b.a.length; var5++) {
                     if (b.a[var5] != null && b.a[var5] != var0 && b.a[var5].r != var0.r && a(var0.b, b.a[var5].b) <= var0.l[14]) {
                        a(var0, b.a[var5], var2);
                     }
                  }
               } else if (var0.l[1] == 61619) {
                  i.a(8, var0);
                  var0.q = (short)Math.min(var0.o, var0.q + Math.abs(var2));
               } else {
                  i.a(0, var0.d, var0);
               }
               break;
            case 5:
               var0.G = (short)var0.l[6];
               var0.H = (short)(var2 + 100);
               var0.w = -48;
               i.a(var0.h);
               var0.h = (byte)i.a(9, var0, 5000);
               break;
            case 6:
               i.a(8, var0);
               var0.k = 0;
               var0.l = 0;
               var0.w = -1;
         }

         f(var0);
      }
   }

   public static final void c(j var0, int var1, int var2) {
      var0.k = (byte)var2;
   }

   public static final void a(j var0, int var1) {
      var0.k = -1;
   }

   public static final void a(j var0, int var1, int var2, e var3) {
      switch (var1) {
         case 2:
            var0.o = (byte)var2;
            break;
         case 3:
            var0.s = (short)var2;
            break;
         case 4:
            var0.t = (short)var2;
            break;
         case 5:
            var0.u = (short)var2;
            break;
         case 6:
            var0.v = (short)var2;
            break;
         case 7:
            var0.w = (short)var2;
            break;
         case 8:
            var0.x = (short)var2;
            break;
         case 9:
            var0.y = (short)var2;
            break;
         case 10:
            var0.j = (byte)var2;
         case 11:
         case 12:
         case 16:
         case 17:
         default:
            break;
         case 13:
            var0.r = (byte)var2;
            break;
         case 14:
            var0.E = (short)var2;
            break;
         case 15:
            var0.F = (short)var2;
            break;
         case 18:
            var0.y = (byte)var2;
            var0.t = (byte)(var0.y == 4 ? 1 : 0);
            if (var0.t == 1 || var0.y == 0) {
               var0.l = null;
            }
            break;
         case 19:
            var0.l = b.a.k[var2];
            break;
         case 20:
            var0.m = (short)(var2 * 1000);
      }

      var0.o = (short)(var0.o * 4 + (var0.s + var0.O) * 2 + var0.x * 2 + var0.I);
      var0.q = (short)Math.min(var0.q, var0.o);
      var0.d = (short)('鱀' / var0.o);
      var0.p = (short)(var0.o * 4 + var0.t * 2 + var0.J);
      var0.r = (short)Math.min(var0.r, var0.p);
      var0.f = (short)('鱀' / var0.p);
      if (var0.j > 0) {
         var0.i = (byte)var3.a(4, var0.j)[3];
      }

      if (var0.E == 0) {
         var0.E = a;
      }

      if (var0.F == 0) {
         var0.F = b;
      }

      f(var0);
   }

   public static final void a(j var0, int var1, int[] var2) {
      a(var0, var1, var2, false);
   }

   public static final void a(j var0, int var1, int[] var2, boolean var3) {
      int var4 = 0;

      while (var4 < var0.k.length && var0.k[var4] != 0) {
         var4++;
      }

      if (var4 < var0.k.length) {
         switch (var1) {
            case 0:
               var0.k[var4] = 0 | var2[0];
               if (var0.j == 0 && var0.l == null && a(var0, 0, var2) || var3) {
                  var0.j = (byte)var2[0];
               }
               break;
            case 1:
               if (var0.n[var2[3]] == -1 || var3) {
                  c(var0, var2);
               }

               var0.k[var4] = 256 | var2[0];
               return;
            case 2:
               var0.k[var4] = 512 | var2[0];
               if (var2[5] == 0) {
                  if (var0.f == null && var2[2] > 0) {
                     var0.f = var2;
                     return;
                  }

                  if (var0.g == null && var2[3] > 0) {
                     var0.g = var2;
                     return;
                  }
               }
         }
      }
   }

   private static final void h(j var0) {
      boolean var1 = false;
      int[] var2 = null;
      int[] var3 = null;

      for (int var4 = 0; var4 < var0.k.length && var0.k[var4] != 0; var4++) {
         if (var0.k[var4] <= 255) {
            var3 = b.a.a(4, var0.k[var4] & 0xFF);
            if (a(var0, 0, var3) && (var2 == null || var3[3] > var2[3])) {
               var2 = var3;
            }
         }
      }

      if (var2 != null) {
         var0.j = (byte)var2[0];
         var0.t = (byte)(var2[2] == 4 ? 1 : 0);
      }
   }

   public static final void b(j var0, int var1, int[] var2) {
      int var3 = 0;
      boolean var4 = false;
      int var5 = 0;
      boolean var6 = false;
      switch (var1) {
         case 0:
            var5 = 0 | var2[0];
            if (var0.j == var2[0]) {
               var0.j = 0;
               var0.t = 0;
               var6 = true;
            }
            break;
         case 1:
            var5 = 256 | var2[0];
            break;
         case 2:
            var5 = 512 | var2[0];
      }

      while (var3 < var0.k.length && var0.k[var3] != 0) {
         if (var0.k[var3] == var5) {
            for (int var7 = var3; var7 < var0.k.length - 1; var7++) {
               var0.k[var7] = var0.k[var7 + 1];
            }
            break;
         }

         var3++;
      }

      if (var6) {
         h(var0);
      }

      f(var0);
   }

   public static final int a(j var0) {
      return var0.e == 6 ? 0 : g.b(var0.a, var0.d + a[var0.e]);
   }

   public static final int b(j var0) {
      return var0.e == 6 ? 0 : g.a(var0.a, var0.d + a[var0.e]);
   }

   public static final void a(j var0, byte var1, boolean var2) {
      var0.f = var1;
      if (var0.f == 4) {
         var0.t = 1;
      }

      var0.a = b.a.a(5, var1);
      var0.h = b.a.j[var1];
      if (!var2) {
         a(var0, 0, b.a.a(4, var0.a[4]));
         a(var0, 1, b.a.a(1, var0.a[5]));
         var0.s = (short)b.a.h[var1][7];
         var0.t = (short)b.a.h[var1][8];
         var0.u = (short)b.a.h[var1][9];
         var0.v = (short)b.a.h[var1][10];
         var0.w = (short)b.a.h[var1][6];
         var0.x = (short)b.a.h[var1][11];
         var0.y = (short)b.a.h[var1][12];
         var0.F = (short)b.a.h[var1][13];
         var0.E = (short)b.a.h[var1][14];
      }

      f(var0);
   }

   public static final void b(j var0, byte var1) {
      switch (var1) {
         case 0:
            var0.g = -1;
         default:
            return;
         case 1:
            var0.g = -53;
            return;
         case 2:
            var0.g = -52;
            return;
         case 3:
            var0.g = -51;
            return;
         case 4:
            var0.g = -2;
      }
   }

   public static final void a(j var0, ByteArrayOutputStream var1) throws Exception {
      int var2 = 0;
      boolean var3 = false;
      boolean var4 = false;
      var1.write(var0.c);
      var1.write(var0.f);
      var1.write((byte)(var0.b >> 16));
      var1.write((byte)(var0.b >> 8));
      var1.write((byte)(var0.b >> 0));
      var1.write((byte)(var0.o >> 8));
      var1.write((byte)(var0.o >> 0));
      var1.write((byte)(var0.s >> 8));
      var1.write((byte)(var0.s >> 0));
      var1.write((byte)(var0.t >> 8));
      var1.write((byte)(var0.t >> 0));
      var1.write((byte)(var0.v >> 8));
      var1.write((byte)(var0.v >> 0));
      var1.write((byte)(var0.w >> 8));
      var1.write((byte)(var0.w >> 0));
      var1.write((byte)(var0.x >> 8));
      var1.write((byte)(var0.x >> 0));
      var1.write((byte)(var0.u >> 8));
      var1.write((byte)(var0.u >> 0));
      var1.write((byte)(var0.j >> 8));
      var1.write((byte)(var0.j >> 0));
      var1.write((byte)(var0.E >> 8));
      var1.write((byte)(var0.E >> 0));
      var1.write((byte)(var0.F >> 8));
      var1.write((byte)(var0.F >> 0));
      var1.write(var0.r);
      var1.write((byte)(b.b >> 8));
      var1.write((byte)(b.b >> 0));
      var1.write(var0.b.length());
      var1.write(var0.b.getBytes());

      while (var0.k[var2] != 0) {
         var2++;
      }

      var1.write(var2);

      for (int var5 = 0; var5 < var0.k.length && var0.k[var5] != 0; var5++) {
         if ((var0.k[var5] >> 8 & 0xFF) == 1) {
            var4 = a(var0, (int)(var0.k[var5] & 0xFF));
         } else if ((var0.k[var5] >> 8 & 0xFF) == 0) {
            var4 = a(var0, var0.k[var5] & 0xFF, false);
         } else {
            var4 = false;
         }

         var1.write((byte)((var4 ? 128 : 0) | var0.k[var5] >> 8));
         var1.write((byte)(var0.k[var5] >> 0));
      }
   }

   public static final void b(j var0, int[] var1) {
      if (var1[5] == 0) {
         if (b.a.a(var1[1]).equals(b.a(158))) {
            var0.q = (short)Math.min(var0.o, var0.q + var1[2]);
            var0.r = (short)Math.min(var0.p, var0.r + var1[3]);
            b(var0, 2, var1);
            f(var0);
         }

         if (var1[2] > 0) {
            var0.f = var1;
            return;
         }

         if (var1[3] > 0) {
            var0.g = var1;
            return;
         }

         if (var1[4] > 0) {
            var0.k = 0;
            var0.l = 0;
            var0.w = -1;
            b(var0, 2, var1);
            f(var0);
            return;
         }
      } else {
         if (var1[3] > 0) {
            var0.J = (short)var1[3];
            var0.r = var0.p = (short)(var0.o * 4 + var0.t * 2 + var0.J);
            var0.f = (short)('鱀' / var0.p);
         }

         if (var1[6] > 0) {
            var0.K = (short)var1[6];
         }

         if (var1[7] > 0) {
            var0.L = (short)var1[7];
         }

         if (var1[8] > 0) {
            var0.M = (short)var1[8];
         }

         if (var1[10] > 0) {
            var0.N = (short)var1[10];
         }

         if (var1[11] > 0) {
            var0.O = (short)var1[11];
            var0.o = (short)(var0.o * 4 + (var0.s + var0.O) * 2 + var0.x * 2 + var0.I);
            var0.d = (short)('鱀' / var0.o);
         }

         if (var1[4] > 0) {
            var0.k = 0;
            var0.l = 0;
            var0.w = -1;
         }

         var0.j = 0;
         var0.P = (short)var1[5];
         b(var0, 2, var1);
         f(var0);
      }
   }

   public static final void a(j var0, boolean var1) {
      boolean var2 = false;
      if (var1) {
         if (var0.f != null && var0.q < var0.o) {
            b(var0, 2, var0.f);
            var0.q = (short)Math.min(var0.o, var0.q + var0.f[2]);
            f(var0);
            var0.f = null;

            for (int var6 = 0; var6 < var0.k.length; var6++) {
               int var3 = var0.k[var6] >> 8 & 0xFF;
               int var4 = var0.k[var6] >> 0 & 0xFF;
               int[] var5;
               if (var3 == 2 && (var5 = b.a.a(2, var4))[2] > 0) {
                  var0.f = var5;
               }
            }
         }
      } else if (var0.g != null && var0.r < var0.p) {
         b(var0, 2, var0.g);
         var0.r = (short)Math.min(var0.p, var0.r + var0.g[3]);
         f(var0);
         var0.g = null;

         for (int var7 = 0; var7 < var0.k.length; var7++) {
            int var8 = var0.k[var7] >> 8 & 0xFF;
            int var9 = var0.k[var7] >> 0 & 0xFF;
            int[] var10;
            if (var8 == 2 && (var10 = b.a.a(2, var9))[3] > 0) {
               var0.g = var10;
            }
         }
      }
   }

   public static final boolean a(j var0, String var1) {
      if (var1.startsWith(b.a(304))) {
         var1 = var1.substring(b.a(304).length());
         var0.m = b.a.a(var1);
         var0.t = 0;
         if (var0.l != null) {
            var0.l = var0.m;
            i(var0);
         }

         return true;
      } else {
         if (var1.startsWith(b.a(400))) {
            var1 = var1.substring(b.a(400).length());
            var0.t = 1;
         } else {
            var1 = var1.substring(b.a(305).length());
            var0.t = 0;
         }

         int[] var2 = b.a.a(var1);
         var0.j = (byte)var2[0];
         return false;
      }
   }

   public static final void a(j var0, j var1, int var2, int var3) {
      var1.x = (byte)var2;
      var1.k = (short)var3;
      var1.b = var0;
      var1.w = -47;
      i.a(8, var1);
      a(var2, var1, var0, false, true);
   }

   public static final void a(j var0, j var1, int var2) {
      i.a(10, var1);
      a(var2, var1, var0, false, false);
   }

   public static final void c(j var0, int[] var1) {
      if (var1 != null && a(var0, 1, var1)) {
         var0.n[var1[3]] = var1[0];
         f(var0);
      }
   }

   public static final boolean a(j var0, int var1) {
      boolean var2 = false;

      for (int var3 = 0; var3 < var0.n.length; var3++) {
         if (var0.n[var3] == var1) {
            return true;
         }
      }

      return false;
   }

   public static final boolean a(j var0, int var1, boolean var2) {
      return a(var0, b.a.a(4, var1), var2);
   }

   public static final boolean a(j var0, int[] var1, boolean var2) {
      if (var0.m != null && var2) {
         return var1[0] == var0.m[0];
      } else {
         return !var2 ? var0.j == var1[0] : false;
      }
   }

   public static final void b(j var0, boolean var1) {
      var0.s = (byte)(var1 ? 1 : 0);
   }

   public static final void b(j var0, int var1) {
      while (var0.o < var1) {
         var0.o++;
         var0.s++;
         var0.t++;
         var0.u++;
         var0.v++;
         var0.w++;
         var0.x++;
         var0.y++;
         g(var0);
         var0.o = (short)(var0.o * 4 + (var0.s + var0.O) * 2 + var0.x * 2 + var0.I);
         var0.d = (short)('鱀' / var0.o);
         var0.p = (short)(var0.o * 4 + var0.t * 2 + var0.J);
         var0.f = (short)('鱀' / var0.p);
         f(var0);
      }
   }

   public static final boolean a(j var0, int var1, long var2) {
      switch (var1) {
         case 2:
            if (var0.l == null && var0.m != null) {
               var0.l = var0.m;
            } else {
               var0.l = null;
            }

            i(var0);
            return true;
         case 3:
            a(var0, (byte)1);
            a(var0, 2, (long)var2);
            break;
         case 4:
            a(var0, (byte)1);
            a(var0, 1, (long)var2);
            break;
         case 5:
            a(var0, (byte)1);
            a(var0, 4, (long)var2);
            break;
         case 6:
            a(var0, (byte)1);
            a(var0, 3, (long)var2);
            break;
         case 7:
            a(var0, b.c);
      }

      return false;
   }

   private static final void i(j var0) {
      if (var0.l != null) {
         switch (var0.l[2]) {
            case 0:
               var0.v = -48;
               break;
            case 1:
               var0.v = -50;
               break;
            case 2:
               var0.v = -46;
               break;
            case 3:
               if (var0.l[1] == 61618) {
                  var0.v = -44;
               } else if (var0.l[1] == 61619) {
                  var0.v = -43;
               } else {
                  var0.v = -50;
               }
               break;
            case 4:
               var0.v = -47;
               break;
            case 5:
               var0.v = -48;
               break;
            case 6:
               var0.v = -43;
         }
      } else {
         var0.v = -45;
      }
   }

   public static final boolean a(j var0, int var1, int[] var2) {
      if (var0 != null && var2 != null && var0.f != -1) {
         if (var1 == 0) {
            if (var2[2] == 1) {
               return b.a.a(var0.f, 5);
            }

            if (var2[2] == 2) {
               return b.a.a(var0.f, 6);
            }

            if (var2[2] == 3) {
               return b.a.a(var0.f, 7);
            }

            if (var2[2] == 4) {
               return b.a.a(var0.f, 8);
            }

            if (var2[2] == 0) {
               return b.a.a(var0.f, 14);
            }
         } else if (var1 == 1) {
            if (var2[2] == 2) {
               return b.a.a(var0.f, 4);
            }

            if (var2[2] == 1) {
               return b.a.a(var0.f, 3);
            }

            if (var2[2] == 0) {
               return b.a.a(var0.f, 1);
            }
         }

         return true;
      } else {
         return false;
      }
   }
}
