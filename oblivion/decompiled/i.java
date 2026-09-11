import javax.microedition.lcdui.Graphics;

public final class i {
   private static d a = null;
   private static short[] a = new short[99];

   private static final int a() {
      boolean var0 = false;

      for (byte var1 = 0; var1 < a.length - 9; var1 += 9) {
         if (a[var1 + 0] == -1) {
            return var1;
         }
      }

      return -1;
   }

   public static final void a() {
      boolean var0 = false;

      for (byte var1 = 0; var1 < a.length; var1 += 9) {
         a(var1);
      }
   }

   public static final void a(int var0) {
      if (var0 >= 0 && var0 < a.length) {
         a[var0 + 0] = -1;
         a[var0 + 1] = -1;
         a[var0 + 2] = -1;
         a[var0 + 3] = -1;
         a[var0 + 4] = -1;
         a[var0 + 5] = -1;
         a[var0 + 6] = -1;
         a[var0 + 7] = -1;
         a[var0 + 8] = -1;
      }
   }

   public static final void a(int var0, int var1) {
      boolean var2 = false;

      for (byte var3 = 0; var3 < a.length; var3 += 9) {
         if (a[var3 + 0] != -1 && a[var3 + 1] == var0 && a[var3 + 2] == var1) {
            a(var3);
            return;
         }
      }
   }

   public static final int a(int var0, j var1) {
      return a(var0, 0, var1);
   }

   public static final int a(int var0, j var1, int var2) {
      return a(var0, 0, var1, var2);
   }

   public static final int a(int var0, int var1, j var2) {
      return a(var0, var1, var2, 0);
   }

   public static final int a(int var0, int var1, j var2, int var3) {
      int var4;
      if ((var4 = a()) == -1) {
         return var4;
      }

      if (var0 == 0) {
         if (var1 == 2) {
            var0 = 0;
         } else if (var1 == 1) {
            var0 = 2;
         } else if (var1 == 3) {
            var0 = 4;
         } else if (var1 == 4) {
            var0 = 6;
         }
      } else if (var0 == 11) {
         if (var1 == 2) {
            var0 = 11;
         } else if (var1 == 1) {
            var0 = 12;
         } else if (var1 == 3) {
            var0 = 13;
         } else if (var1 == 4) {
            var0 = 14;
         }
      }

      a[var4 + 0] = (short)(-4096 | var2.c << 8 | var0);
      a[var4 + 1] = (short)var2.b[0];
      a[var4 + 2] = (short)var2.b[1];
      a[var4 + 5] = (short)var2.b[0];
      a[var4 + 6] = (short)var2.b[1];
      a[var4 + 3] = 0;
      a[var4 + 4] = 0;
      a[var4 + 7] = (short)var3;
      a[var4 + 8] = 0;
      return var4;
   }

   public static final int a(int var0, int var1, int var2) {
      return a(var0, var1, var2, 0);
   }

   public static final int a(int var0, int var1, int var2, int var3) {
      int var4;
      if ((var4 = a()) == -1) {
         return -1;
      }

      a[var4 + 0] = (short)var0;
      a[var4 + 1] = (short)var1;
      a[var4 + 2] = (short)var2;
      a[var4 + 5] = (short)var1;
      a[var4 + 6] = (short)var2;
      a[var4 + 3] = 0;
      a[var4 + 4] = 0;
      a[var4 + 7] = (short)var3;
      a[var4 + 8] = 0;
      return var4;
   }

   private static final boolean a(int var0) {
      int[] var1 = new int[]{a[var0 + 1], a[var0 + 2]};
      int var2 = -1;
      int var3 = 0;
      int var4 = 16777215;
      int var5 = (a[var0 + 0] & 4095) >> 8;
      boolean var6 = false;
      j var7 = null;
      if (var5 > 0 && var5 < b.a.length) {
         if ((var7 = b.a[var5 - 1]) == null) {
            a(var0);
            return false;
         }

         for (int var9 = 0; var9 < 25; var9++) {
            if (b.a[var9] != null && b.a[var9].q != 1 && var7 != b.a[var9] && var7.r != b.a[var9].r && (var3 = h.a(var1, b.a[var9].b)) < 200 && var3 < var4) {
               var2 = var9;
               var4 = var3;
            }
         }

         if (var2 != -1) {
            h.a(var7, b.a[var2], false);
            return true;
         } else {
            return false;
         }
      } else {
         a(var0);
         return false;
      }
   }

   public static final void a(long var0) {
      int[] var2 = new int[]{0, 0};
      int[] var3 = new int[]{0, 0};
      int var4 = 0;
      int var5 = 0;
      boolean var6 = false;

      for (byte var9 = 0; var9 < a.length; var9 += 9) {
         if (a[var9 + 0] != -1) {
            a[var9 + 3] = (short)(a[var9 + 3] + var0);
            a[var9 + 8] = (short)(a[var9 + 8] + var0);
            if (a[var9 + 7] > 0 && a[var9 + 8] >= a[var9 + 7]) {
               a[var9 + 4] = 0;
               a[var9 + 8] = 0;
            }

            if ((a[var9 + 4] & '\uff00') != 65280 && a[var9 + 3] > 100) {
               a[var9 + 3] = 0;
               a[var9 + 4]++;
               if ((var4 = a[var9 + 0] & 255) >= 0 && var4 <= 6 || var4 >= 11 && var4 <= 14) {
                  if (var4 == 0) {
                     a[var9 + 2] = (short)(a[var9 + 2] - 60);
                  } else if (var4 == 2) {
                     a[var9 + 2] = (short)(a[var9 + 2] + 60);
                  } else if (var4 == 4) {
                     a[var9 + 1] = (short)(a[var9 + 1] + 60);
                  } else if (var4 == 6) {
                     a[var9 + 1] = (short)(a[var9 + 1] - 60);
                  }

                  if (var4 == 11) {
                     a[var9 + 2] = (short)(a[var9 + 2] - 150);
                  } else if (var4 == 12) {
                     a[var9 + 2] = (short)(a[var9 + 2] + 150);
                  } else if (var4 == 13) {
                     a[var9 + 1] = (short)(a[var9 + 1] + 150);
                  } else if (var4 == 14) {
                     a[var9 + 1] = (short)(a[var9 + 1] - 150);
                  }

                  if (g.a(a, var4, a[var9 + 4])) {
                     if (var4 != 1 && var4 != 3 && var4 != 5 && var4 != 7) {
                        a[var9 + 4] = 0;
                        g.a(a, var4, a[var9 + 4]);
                     } else {
                        a(var9);
                     }
                  }

                  var2[0] = a[var9 + 1];
                  var2[1] = a[var9 + 2];
                  var3[0] = a[var9 + 5];
                  var3[1] = a[var9 + 6];
                  if (h.a(var2, var3) > 750 || a(var9)) {
                     if (var4 == 11 || var4 == 12 || var4 == 13 || var4 == 14) {
                        a(var9);
                     } else if (var4 == 0 || var4 == 2 || var4 == 4 || var4 == 6) {
                        a[var9 + 0]++;
                     }
                  }
               } else {
                  if ((a[var9 + 0] & -4096) == -4096) {
                     if ((var5 = ((a[var9 + 0] & 4095) >> 8) - 1) < 0 || var5 > b.a.length || b.a[var5] == null) {
                        a(var9);
                        continue;
                     }

                     a[var9 + 1] = (short)b.a[var5].b[0];
                     a[var9 + 2] = (short)b.a[var5].b[1];
                  }

                  if (g.a(a, var4, a[var9 + 4])) {
                     if (a[var9 + 7] <= 0) {
                        a(var9);
                     } else {
                        a[var9 + 4] = (short)(a[var9 + 4] | 0xFF00);
                     }
                  }
               }
            }
         }
      }
   }

   public static final void a(Graphics var0, int[] var1) {
      int[] var2 = new int[]{0, 0};
      int[] var3 = new int[]{0, 0};
      boolean var4 = false;

      for (byte var5 = 0; var5 < a.length; var5 += 9) {
         if (a[var5 + 0] != -1 && (a[var5 + 4] & '\uff00') != 65280) {
            var2[0] = a[var5 + 1];
            var2[1] = a[var5 + 2];
            b.a(var2, var3);
            if (var3[0] + var1[0] >= 0 && var3[0] + var1[0] <= b.a && var3[1] + var1[1] >= 0 && var3[1] + var1[1] <= b.b) {
               g.a(a, a[var5 + 0] & 255, a[var5 + 4]);
               g.a(var0, a, a[var5 + 0] & 255, var3[0] + var1[0], var3[1] + var1[1]);
            }
         }
      }
   }

   static {
      boolean var0 = false;
      a = g.a("/oh_magic.cml");

      for (int var1 = 0; var1 < a.length; var1++) {
         a[var1] = -1;
      }
   }
}
