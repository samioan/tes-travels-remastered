public final class e {
   private static final byte[][] a = new byte[][]{{3, 1, 1, 2, 0, 1, 1}, {2, 2, 2, 1, 0, 1, 1}, {1, 3, 3, 1, 0, 2, 3}};
   private int[] a = new int[10];
   private int[] b = new int[10];
   private int[] c = new int[8];
   private int[] d = new int[256];
   private int[] e = null;
   private int[] f = new int[100];
   private int[] g = new int[10];
   private int[] h = null;
   public int[][] a = new int[25][21];
   public int[][] b = new int[25][21];
   public int[][] c = new int[37][8];
   public int[][] d = new int[42][10];
   public int[][] e = new int[11][14];
   public int[][] f = new int[10][21];
   public int[][] g = new int[25][7];
   public int[][] h = new int[9][15];
   public int[][] i = new int[9][15];
   public int[][] j = new int[9][15];
   public int[][] k = new int[10][15];
   public int[][] l = new int[30][4];
   private int a = 0;
   private int b = 0;
   private int c = -1;
   private int d = 0;
   private int e = 0;
   private b a = null;
   private String[] a = new String[255];
   private byte[] a = null;
   private static e a = null;
   private byte a = 0;
   private byte b = 0;
   private byte c = 0;
   private byte d = -1;
   public boolean a = false;
   private boolean b = true;

   public e(b var1) {
      this.a = var1;
      a = this;
      this.a();
   }

   private final void a() {
      boolean var1 = false;
      boolean var2 = false;
      this.c[3] = -1;
      this.c[4] = -1;
      this.c[5] = -1;
      this.c[6] = -1;
      this.c[7] = -1;
      this.f[0] = -1;
      this.a = 0;
      this.b = 0;
      this.c = -1;
      this.d = 0;
      this.e = null;
      this.a = null;
      this.e = 0;
      this.h = null;
      this.d = -1;
      this.a = false;

      for (int var3 = 0; var3 < this.g.length; var3++) {
         this.g[var3] = 0;
      }

      for (int var4 = 0; var4 < 10; var4++) {
         this.a[var4] = 0;
         this.b[var4] = 0;
      }

      for (int var5 = 0; var5 < this.d.length; var5++) {
         this.d[var5] = 0;
      }

      for (int var6 = 0; var6 < 30; var6++) {
         for (int var7 = 0; var7 < 4; var7++) {
            this.l[var6][var7] = 0;
         }
      }
   }

   public final void a(String var1) {
      int var2 = 0;
      boolean var3 = false;
      if (var1 != null) {
         this.a();
         int var4 = b.a(var1);

         for (var2 = 1; var2 < b.b[0] * 3; var2 += 3) {
            this.d[b.b[var2]] = ((char)b.b[var2 + 1] & 255) << 8 | ((char)b.b[var2 + 2] & 255) << 0;
         }

         while (b.b[var2++] == 30) {
            if (b.b[var2] == 0) {
               var2 = this.a(++var2);
            } else if (b.b[var2] == 1) {
               var2 = this.b(++var2);
            } else if (b.b[var2] == 2) {
               var2 = this.c(++var2);
            } else if (b.b[var2] == 4) {
               var2 = this.d(++var2);
            } else if (b.b[var2] == 5) {
               var2 = this.e(++var2);
            } else if (b.b[var2] == 6) {
               var2 = this.f(++var2);
            } else if (b.b[var2] == 7) {
               var2 = this.g(++var2);
            } else if (b.b[var2] == 8) {
               var2 = this.h(++var2);
            } else if (b.b[var2] == 9) {
               var2 = this.i(++var2);
            } else if (b.b[var2] == 10) {
               var2 = this.j(++var2);
            }
         }

         if (this.b) {
            for (var2 = 0; var2 < this.a.length; var2++) {
               System.arraycopy(this.a[var2], 0, this.b[var2], 0, this.b[var2].length);
            }

            this.b = false;
         }

         var2 += 2;

         for (int var17 = 0; var17 < this.d.length; var17++) {
            if (this.d[var17] != 0) {
               this.d[var17] = this.d[var17] - var2;
            }
         }

         this.a = new byte[var4 - var2];
         System.arraycopy(b.b, var2, this.a, 0, this.a.length);
         a(1);
      }
   }

   private final int a(int var1) {
      byte var2 = 0;
      int[] var3 = new int[21];

      while (b.b[var1] != 31) {
         if (b.b[var1] == 1) {
            this.a[this.d] = new String(b.b, var1 + 2, b.b[var1 + 1]);
            var3[b.b[var1]] = this.d++;
            var1 += b.b[var1 + 1] + 1;
         } else if (b.b[var1] != 7 && b.b[var1] != 14 && b.b[var1] != 15) {
            if (b.b[var1] == 0) {
               var2 = b.b[var1 + 1];
            }

            var3[b.b[var1++]] = (char)(b.b[var1] & 0xFF);
         } else {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
            var1 += 2;
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.a[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int b(int var1) {
      byte var2 = 0;
      int[] var3 = new int[10];

      while (b.b[var1] != 31) {
         if (b.b[var1] == 1) {
            byte var4;
            if (((var4 = b.b[var1 + 1]) & 240) == 240) {
               var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.a[this.d] = new String(b.b, var1 + 2, b.b[var1 + 1]);
               var3[b.b[var1]] = this.d++;
               var1 += b.b[var1 + 1] + 1;
            }
         } else if (b.b[var1] == 5) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 16 | ((char)b.b[var1 + 2] & 255) << 8 | ((char)b.b[var1 + 3] & 255) << 0;
            var1 += 3;
         } else if (b.b[var1] == 9) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.b[var1] == 6) {
            var3[b.b[var1]] = 1;
         } else {
            if (b.b[var1] == 0) {
               var2 = b.b[var1 + 1];
            }

            var3[b.b[var1++]] = b.b[var1];
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.d[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int c(int var1) {
      byte var2 = 0;
      int[] var3 = new int[14];

      while (b.b[var1] != 31) {
         if (b.b[var1] == 1) {
            byte var4;
            if (((var4 = b.b[var1 + 1]) & 240) == 240) {
               var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.a[this.d] = new String(b.b, var1 + 2, b.b[var1 + 1]);
               var3[b.b[var1]] = this.d++;
               var1 += b.b[var1 + 1] + 1;
            }
         } else if (b.b[var1] == 5) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 16 | ((char)b.b[var1 + 2] & 255) << 8 | ((char)b.b[var1 + 3] & 255) << 0;
            var1 += 3;
         } else if (b.b[var1] == 13) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.b[var1] == 4) {
            var3[b.b[var1]] = 1;
         } else {
            if (b.b[var1] == 0) {
               var2 = b.b[var1 + 1];
            }

            var3[b.b[var1++]] = (char)b.b[var1] & 255;
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.e[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int d(int var1) {
      byte var2 = 0;
      int[] var3 = new int[8];

      while (b.b[var1] != 31) {
         if (b.b[var1] == 1) {
            byte var4;
            if (((var4 = b.b[var1 + 1]) & 240) == 240) {
               var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.a[this.d] = new String(b.b, var1 + 2, b.b[var1 + 1]);
               var3[b.b[var1]] = this.d++;
               var1 += b.b[var1 + 1] + 1;
            }
         } else if (b.b[var1] == 7) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
            var1 += 2;
         } else {
            if (b.b[var1] == 0) {
               var2 = b.b[var1 + 1];
            }

            var3[b.b[var1++]] = b.b[var1];
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.c[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int e(int var1) {
      byte var2 = 0;
      int[] var3 = new int[15];
      int[] var4 = new int[15];
      int[] var5 = new int[15];
      int var6 = 0;
      int var7 = 0;

      while (b.b[var1] != 31) {
         if (b.b[var1] == 1) {
            byte var8;
            if (((var8 = b.b[var1 + 1]) & 240) == 240) {
               var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.a[this.d] = new String(b.b, var1 + 2, b.b[var1 + 1]);
               var3[b.b[var1]] = this.d++;
               var1 += b.b[var1 + 1] + 1;
            }
         } else if (b.b[var1] == 6 || b.b[var1] == 13 || b.b[var1] == 14) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.b[var1] == 2) {
            var4[var6++] = b.b[++var1];
         } else if (b.b[var1] == 3) {
            var5[var7++] = b.b[++var1];
         } else {
            if (b.b[var1] == 0) {
               var2 = b.b[var1 + 1];
            }

            var3[b.b[var1++]] = b.b[var1];
         }

         var1++;
      }

      var4[var6] = -1;
      var5[var7] = -1;
      System.arraycopy(var3, 0, this.h[var2], 0, var3.length);
      System.arraycopy(var5, 0, this.j[var2], 0, var5.length);
      System.arraycopy(var4, 0, this.i[var2], 0, var4.length);
      return var1 + 1;
   }

   private final int f(int var1) {
      byte var2 = 0;
      int[] var3 = new int[7];

      while (b.b[var1] != 31) {
         if (b.b[var1] == 2) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
            var1 += 2;
         } else {
            if (b.b[var1] == 0) {
               var2 = b.b[var1 + 1];
            }

            var3[b.b[var1++]] = (char)b.b[var1] & 255;
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.g[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int g(int var1) {
      int var2 = 0;

      while (b.b[var1] != 31) {
         this.f[var2++] = b.b[var1++];
         this.f[var2++] = b.b[var1++];
      }

      this.f[var2] = -1;
      return var1 + 1;
   }

   private final int h(int var1) {
      byte var2 = 0;
      int[] var3 = new int[15];

      while (b.b[var1] != 31) {
         if (b.b[var1] == 1) {
            byte var4;
            if (((var4 = b.b[var1 + 1]) & 240) == 240) {
               var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
               var1 += 2;
            } else {
               this.a[this.d] = new String(b.b, var1 + 2, b.b[var1 + 1]);
               var3[b.b[var1]] = this.d++;
               var1 += b.b[var1 + 1] + 1;
            }
         } else if (b.b[var1] == 14) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.b[var1] == 6) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 16 | ((char)b.b[var1 + 2] & 255) << 8 | ((char)b.b[var1 + 3] & 255) << 0;
            var1 += 3;
         } else {
            if (b.b[var1] == 0) {
               var2 = b.b[var1 + 1];
            }

            var3[b.b[var1++]] = b.b[var1];
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.k[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int i(int var1) {
      byte var2 = 0;
      int[] var3 = new int[21];

      while (b.b[var1] != 31) {
         if (b.b[var1] == 1 || b.b[var1] == 2) {
            var3[b.b[var1]] = ((char)b.b[var1 + 1] & 255) << 8 | ((char)b.b[var1 + 2] & 255) << 0;
            var1 += 2;
         } else if (b.b[var1] == 20) {
            this.g[this.e++] = (char)b.b[++var1] & 255;
         } else {
            if (b.b[var1] == 0) {
               var2 = b.b[var1 + 1];
            }

            var3[b.b[var1++]] = (char)b.b[var1] & 255;
         }

         var1++;
      }

      System.arraycopy(var3, 0, this.f[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int j(int var1) {
      byte var2 = 0;
      int[] var3 = new int[4];

      while (b.b[var1] != 31) {
         if (b.b[var1] == 0) {
            var2 = b.b[var1 + 1];
         }

         var3[b.b[var1++]] = (char)b.b[var1] & 255;
         var1++;
      }

      System.arraycopy(var3, 0, this.l[var2], 0, var3.length);
      return var1 + 1;
   }

   private final int b() {
      int var1 = (char)this.a[this.a[this.a - 1]] & 255;
      this.a[this.a - 1]++;
      return var1;
   }

   private final int c() {
      int var1 = ((char)this.a[this.a[this.a - 1] + 0] & 255) << 16
         | ((char)this.a[this.a[this.a - 1] + 1] & 255) << 8
         | ((char)this.a[this.a[this.a - 1] + 2] & 255) << 0;
      this.a[this.a - 1] = this.a[this.a - 1] + 3;
      return var1;
   }

   private final int d() {
      int var1 = ((char)this.a[this.a[this.a - 1] + 0] & 255) << 8 | ((char)this.a[this.a[this.a - 1] + 1] & 255) << 0;
      this.a[this.a - 1] = this.a[this.a - 1] + 2;
      return var1;
   }

   private final String c(int var1) {
      String var2 = new String(this.a, this.a[this.a - 1], var1);
      this.a[this.a - 1] = this.a[this.a - 1] + var1;
      return var2;
   }

   private final void b(long var1) {
      String var3 = null;
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      int var7 = this.a[this.a - 1];
      byte var8 = this.a[var7];
      int var9 = 0;
      int var10 = 0;
      int var11 = 0;
      int var12 = 0;
      int var13 = 0;
      int var14 = 0;
      int var15 = 0;
      int var16 = 0;
      int var17 = 0;
      int var18 = 0;
      int var19 = 0;
      int var20 = 0;
      int var21 = 0;
      int var22 = 0;
      int var23 = 0;
      int var24 = 0;
      int var25 = 0;
      int var26 = 0;
      int var27 = 0;
      int var28 = 0;
      int var29 = 0;
      int var30 = 0;
      int var31 = 0;
      if (!this.a) {
         if (!this.a.g) {
            if (this.d >= 0) {
               h.b(b.a[this.d], (byte)0);
               this.d = -1;
            }

            if (!this.a.a()) {
               if (this.c >= 0) {
                  this.b = (int)(this.b + var1);
                  if (this.b < this.c) {
                     return;
                  }

                  this.c = -1;
               }

               if (this.e != null) {
                  for (int var51 = 0; var51 < this.e.length; var51++) {
                     if (b.a[this.e[var51]] != null && b.a[this.e[var51]].j[0] != -1) {
                        return;
                     }
                  }

                  this.e = null;
               }

               if (this.h != null) {
                  switch (this.c) {
                     case 0:
                        this.a.a(false);
                        this.a.b(this.b);
                        if (this.a != 0 && this.a != 1) {
                           h.b(b.a[this.b], this.h[0], b.a[this.b].b[1]);
                        } else {
                           h.b(b.a[this.b], b.a[this.b].b[0], this.h[1]);
                        }

                        h.a(b.a[this.b], (byte)2);
                        h.a(b.a[this.b], 7, 900, this);
                        this.c = 1;
                        return;
                     case 1:
                        if (b.a[this.b].j[0] == -1) {
                           h.a(b.a[this.b], (byte)3);
                           h.a(b.a[this.b], 7, 400, this);
                           h.b(b.a[this.b], this.h[0], this.h[1]);
                           this.c = 2;
                           return;
                        }
                        break;
                     case 2:
                        if (b.a[this.b].j[0] == -1) {
                           this.a.a(true);
                           this.h = null;
                        }
                  }
               } else {
                  this.a[this.a - 1]++;
                  switch (var8) {
                     case 0:
                        System.err.println("1) Never should have gotten here!!!");
                        return;
                     case 1:
                        System.err.println("2) Never should have gotten here!!!");
                        return;
                     case 2:
                        this.b();
                        return;
                     case 3:
                        var20 = this.d();
                        b.g(null);
                        if ((var20 & 61440) == 61440) {
                           var3 = b.a(var20 & 4095);
                        } else {
                           var3 = this.c(var20);
                        }

                        this.a.f(var3);
                        this.a.i();
                        return;
                     case 4:
                        this.a.a(this.b(), this.b());
                        return;
                     case 5:
                        this.b();
                        return;
                     case 6:
                        this.b();
                        return;
                     case 7:
                        b.c = this.b() == 1;
                        return;
                     case 8:
                        try {
                           this.a.b(this.c(this.b()));
                           this.a.c(this.c(this.b()));
                           return;
                        } catch (Exception var36) {
                           var36.printStackTrace();
                           return;
                        }
                     case 9:
                        this.c(this.b());
                        return;
                     case 10:
                        this.a.b(this.b(), this.c());
                        return;
                     case 11:
                        this.c = this.d();
                        this.b = 0;
                        return;
                     case 12:
                        b.a((byte)0);
                        return;
                     case 13:
                        this.b();
                        return;
                     case 14:
                        switch (this.b()) {
                           case 0:
                              this.c[3] = this.b();
                              break;
                           case 1:
                              this.c[4] = this.b();
                              break;
                           case 2:
                              this.c[5] = this.b();
                              break;
                           case 3:
                              this.c[6] = this.b();
                              break;
                           case 4:
                              this.c[7] = this.b();
                        }

                        return;
                     case 15:
                        if ((var20 = this.d()) != 0) {
                           if ((var20 & 61440) == 61440) {
                              var3 = b.a(var20 & 4095);
                           } else {
                              var3 = this.c(var20);
                           }
                        }

                        var14 = this.b();
                        var11 = this.b();
                        var12 = this.d();
                        var13 = this.d();
                        this.a.a(var3, this.a[this.a[var11][1]], (byte)var14, var12, var13, this.a[var11]);
                        return;
                     case 16:
                        this.a.a(this.b(), this.b(), this.b(), this.b(), this.b());
                        return;
                     case 17:
                        var17 = this.b();
                        var15 = this.d();
                        var16 = this.d();
                        if (b.a[var17] != null) {
                           h.b(b.a[var17], var15, var16);
                           return;
                        }
                        break;
                     case 18:
                        var15 = this.b();
                        var16 = this.b();
                        var10 = this.b();
                        var9 = this.b();
                        this.a.a(var15, var16, var10, var9);
                        return;
                     case 19:
                        this.a.a(this.b() == 1);
                        return;
                     case 20:
                        b.a(this.b());
                        return;
                     case 21:
                        this.e = new int[this.b()];

                        for (int var60 = 0; var60 < this.e.length; var60++) {
                           this.e[var60] = this.b();
                        }
                        break;
                     case 22:
                        this.a.a(this.b(), this.b(), this.b() == 1);
                        return;
                     case 23:
                        a(this.b());
                        return;
                     case 24:
                        h.a(b.a[this.b()], (byte)this.b());
                        return;
                     case 25:
                        this.a.c(this.d(), this.d());
                        return;
                     case 26:
                        this.a.b(this.b());
                        return;
                     case 27:
                        this.a.a(this.b(), this.b(), 255, 255, 255);
                        return;
                     case 28:
                        switch (this.b()) {
                           case 0:
                              this.c[3] = -1;
                              break;
                           case 1:
                              this.c[4] = -1;
                              break;
                           case 2:
                              this.c[5] = -1;
                              break;
                           case 3:
                              this.c[6] = -1;
                              break;
                           case 4:
                              this.c[7] = -1;
                        }

                        return;
                     case 29:
                        this.a.a(this.c(this.b()));
                        return;
                     case 30:
                     case 31:
                     default:
                        break;
                     case 32:
                        h.c(b.a[this.b()], this.b(), this.b());
                        return;
                     case 33:
                        h.a(b.a[this.b()], (int)this.b());
                        return;
                     case 34:
                        var17 = this.b();
                        var21 = this.b();
                        var22 = 0;
                        switch (var21) {
                           case 2:
                           case 3:
                           case 4:
                           case 5:
                           case 6:
                           case 8:
                           case 9:
                           case 10:
                           case 11:
                           case 12:
                           case 13:
                           case 18:
                           case 19:
                           case 20:
                              var22 = this.b();
                              break;
                           case 7:
                           case 14:
                           case 15:
                              var22 = this.d();
                           case 16:
                           case 17:
                        }

                        if (b.a[var17] != null) {
                           h.a(b.a[var17], var21, var22, this);
                           return;
                        }
                        break;
                     case 35:
                        this.b();
                        return;
                     case 36:
                        var17 = this.b();
                        var15 = this.d();
                        var16 = this.d();
                        if (b.a[var17] != null) {
                           h.a(b.a[var17], var15, var16);
                           return;
                        }
                        break;
                     case 37:
                        var17 = this.b();
                        var18 = this.b();
                        var19 = this.b();
                        if (b.a[var17] != null) {
                           switch (var18) {
                              case 0:
                                 h.a(b.a[var17], var18, this.c[var19]);
                                 break;
                              case 1:
                                 h.a(b.a[var17], var18, this.d[var19]);
                                 break;
                              case 2:
                                 h.a(b.a[var17], var18, this.e[var19]);
                           }

                           return;
                        }
                        break;
                     case 38:
                        var17 = this.b();
                        var18 = this.b();
                        var19 = this.b();
                        if (b.a[var17] != null) {
                           switch (var18) {
                              case 0:
                                 h.b(b.a[var17], var18, this.c[var19]);
                                 break;
                              case 1:
                                 h.b(b.a[var17], var18, this.d[var19]);
                                 break;
                              case 2:
                                 h.b(b.a[var17], var18, this.e[var19]);
                           }

                           return;
                        }
                        break;
                     case 39:
                        if (((var20 = this.d()) & 61440) == 61440) {
                           var3 = b.a(var20 & 4095);
                        } else {
                           var3 = this.c(var20);
                        }

                        var4 = this.b();
                        var5 = this.b();
                        var6 = this.b();
                        b.a(var3, var4, var5, var6);
                        return;
                     case 40:
                        b.a(null, 0, 0, 0);
                        return;
                     case 41:
                        var17 = this.b();
                        var15 = this.d();
                        if (b.a[var17] != null) {
                           h.b(b.a[var17], var15, b.a[var17].b[1]);
                           return;
                        }
                        break;
                     case 42:
                        var17 = this.b();
                        var16 = this.d();
                        if (b.a[var17] != null) {
                           h.b(b.a[var17], b.a[var17].b[0], var16);
                           return;
                        }
                        break;
                     case 43:
                        this.a.d(this.c(this.b()));
                        return;
                     case 44:
                        this.a.e();
                        return;
                     case 45:
                        this.a.f();
                        return;
                     case 46:
                        var17 = this.b();
                        var23 = this.b();
                        if (b.a[var17] != null) {
                           h.b(b.a[var17], (byte)var23);
                           return;
                        }
                        break;
                     case 47:
                        this.a.a(this.a(9, this.b()), this.g, this.b(), this.b());
                        return;
                     case 48:
                        this.a.a();
                        return;
                     case 49:
                        this.a.a(this.b(), true, this.b(), this.b());
                        return;
                     case 50:
                        var24 = this.b();
                        var25 = this.b();
                        var26 = this.b();
                        var27 = this.b();
                        var28 = this.b();
                        var29 = this.b();
                        var30 = this.b();

                        for (int var57 = var24; var57 <= var26; var57++) {
                           for (int var69 = var25; var69 <= var27; var69++) {
                              this.a.a(var57, var69, var28, var29, var30);
                           }
                        }
                        break;
                     case 51:
                        var24 = this.b();
                        var25 = this.b();
                        var26 = this.b();
                        var27 = this.b();

                        for (int var56 = var24; var56 <= var26; var56++) {
                           for (int var68 = var25; var68 <= var27; var68++) {
                              this.a.a(var56, var68, 255, 255, 255);
                           }
                        }
                        break;
                     case 52:
                        this.b = (byte)this.b();
                        this.a = (byte)this.b();
                        this.h = new int[]{this.d(), this.d()};
                        this.c = 0;
                        return;
                     case 53:
                        this.d = (byte)this.b();
                        var23 = this.b();
                        var20 = this.d();
                        this.a.b(this.d);
                        h.b(b.a[this.d], (byte)var23);
                        if ((var20 & 61440) == 61440) {
                           this.a.f(b.a(var20 & 4095));
                        } else {
                           this.a.f(this.c(var20));
                        }

                        this.a.i();
                        return;
                     case 54:
                        return;
                     case 55:
                        return;
                     case 56:
                        System.err.println("load lang command");
                        String var32 = this.c(this.b());
                        int var33;
                        if ((var33 = this.b()) == 0) {
                           var33 = 65535;
                        }

                        b.a(var32, var33);
                        return;
                     case 57:
                        return;
                     case 58:
                        var14 = this.b();
                        int var112 = this.b();
                        this.a[var14][2] = var112;

                        for (int var67 = 3; var67 <= 9; var67++) {
                           this.a[var14][var67] = this.a[var14][var67] + var112 * a[this.a[var14][17]][var67 - 3];
                        }
                        break;
                     case 59:
                        var17 = this.b();
                        int var35 = this.b();
                        if (b.a[var17] != null) {
                           h.b(b.a[var17], var35 == 1);
                           return;
                        }
                        break;
                     case 60:
                        this.a = true;
                        return;
                     case 61:
                        b.a((byte)9);
                        return;
                     case 62:
                        return;
                     case 63:
                        return;
                     case 64:
                        b.c = this.c();
                        return;
                     case 65:
                        var17 = this.b();
                        int var34 = this.b();
                        if (b.a[var17] != null) {
                           h.b(b.a[var17], var34);
                           return;
                        }
                        break;
                     case 66:
                        if (((var20 = this.d()) & 61440) == 61440) {
                           b.e(b.a(var20 & 4095));
                           return;
                        }

                        b.e(this.c(var20));
                        return;
                     case 67:
                        var14 = this.b();
                        System.arraycopy(this.b[var14], 0, this.a[var14], 0, this.a[var14].length);
                        return;
                     case 68:
                        var31 = this.b();
                        var15 = this.d();
                        var16 = this.d();
                        if (var31 == 0) {
                           var31 = 8;
                        } else if (var31 == 2) {
                           var31 = 10;
                        } else if (var31 == 1) {
                           var31 = 9;
                        }

                        i.a(var31, var15, var16);
                        return;
                     case 69:
                        var31 = this.b();
                        var15 = this.d();
                        var16 = this.d();
                        var4 = this.b();
                        if (var31 == 0) {
                           var31 = 8;
                        } else if (var31 == 2) {
                           var31 = 10;
                        } else if (var31 == 1) {
                           var31 = 9;
                        }

                        i.a(var31, var15, var16, var4 * 1000);
                        return;
                     case 70:
                        var15 = this.d();
                        var16 = this.d();
                        i.a(var15, var16);
                        return;
                     case 71:
                        var15 = this.d();
                        var16 = this.d();
                        b.d(var15, var16);
                        return;
                     case 72:
                        g.a(this.c(this.d()));
                        return;
                     case 73:
                        b.a((byte)15);
                        b.a = false;
                        return;
                     case 74:
                        b.a = true;
                        return;
                     case 75:
                        var17 = this.b();
                        if (b.a[var17] != null) {
                           b.a[var17].u = (byte)(b.a[var17].u == 1 ? 0 : 1);
                           return;
                        }
                        break;
                     case 76:
                        b.e = this.b() == 1;
                        return;
                     case 77:
                        b.a((byte)4);
                        b.f = false;
                        return;
                     case 78:
                        var17 = this.b();
                        if (b.a[var17] != null) {
                           b.a[var17].z = (byte)this.b();
                        }
                  }
               }
            }
         }
      }
   }

   public static final void a(int var0) {
      if (a.a < a.b.length - 2 && a.a < a.a.length - 2) {
         a.b[a.a] = var0;
         a.a[a.a++] = a.d[var0];
      }
   }

   private final void b() {
      this.a--;
   }

   public final void a(long var1) {
      if (this.a > 0) {
         this.b(var1);
      }
   }

   public final int[] a(int var1, int var2) {
      if (var2 < 0) {
         return null;
      }

      switch (var1) {
         case 0:
            if (var2 < this.a.length) {
               return this.a[var2];
            }
         case 1:
            if (var2 < this.d.length) {
               return this.d[var2];
            }
         case 2:
            if (var2 < this.e.length) {
               return this.e[var2];
            }
         case 4:
            if (var2 < this.c.length) {
               return this.c[var2];
            }
         case 7:
            return this.f;
         case 9:
            if (var2 < this.f.length) {
               return this.f[var2];
            }
         case 6:
            if (var2 < this.g.length) {
               return this.g[var2];
            }
         case 5:
            if (var2 < this.h.length) {
               return this.h[var2];
            }
         case 8:
            if (var2 < this.k.length) {
               return this.k[var2];
            }
         case 10:
            if (var2 < this.l.length) {
               return this.l[var2];
            }
         case 3:
         default:
            return null;
      }
   }

   public final int[] b(int var1, int var2) {
      switch (var1) {
         case 0:
            return this.c[var2];
         case 1:
            return this.d[var2];
         case 2:
            return this.e[var2];
         default:
            return null;
      }
   }

   public final void b(int var1) {
      if (this.a) {
         this.a = false;
         b.a = -286331154;
      } else if (var1 == 3 && this.c[3] >= 0) {
         a(this.c[3]);
         this.c[3] = -1;
      } else if (var1 == 4 && this.c[4] >= 0) {
         a(this.c[4]);
         this.c[4] = -1;
      } else if (var1 == 5 && this.c[5] >= 0) {
         a(this.c[5]);
         this.c[5] = -1;
      } else if (var1 == 6 && this.c[6] >= 0) {
         a(this.c[6]);
         this.c[6] = -1;
      } else {
         if (var1 == 7 && this.c[7] >= 0) {
            a(this.c[7]);
            this.c[7] = -1;
         }
      }
   }

   public final String a(int var1) {
      return (var1 & 61440) == 61440 ? b.a(var1 & 4095) : this.a[var1];
   }

   public final int a(String var1) {
      int var2 = 0;
      boolean var3 = false;

      for (int var5 = 0; var5 < this.d; var5++) {
         if (this.a[var5].equals(var1)) {
            return var5;
         }
      }

      return (var2 = b.b(var1)) == -1 ? var2 : 61440 | var2;
   }

   public final int[] a(String var1) {
      int var2 = this.a(var1);
      boolean var3 = false;
      if (var2 != -1) {
         for (int var4 = 0; var4 < this.c.length; var4++) {
            if (this.c[var4] != null && this.c[var4][1] == var2) {
               return this.c[var4];
            }
         }

         for (int var5 = 0; var5 < this.e.length; var5++) {
            if (this.e[var5] != null && this.e[var5][1] == var2) {
               return this.e[var5];
            }
         }

         for (int var6 = 0; var6 < this.d.length; var6++) {
            if (this.d[var6] != null && this.d[var6][1] == var2) {
               return this.d[var6];
            }
         }

         for (int var7 = 0; var7 < this.h.length; var7++) {
            if (this.h[var7] != null && this.h[var7][1] == var2) {
               return this.h[var7];
            }
         }

         for (int var8 = 0; var8 < this.k.length; var8++) {
            if (this.k[var8] != null && this.k[var8][1] == var2) {
               return this.k[var8];
            }
         }
      }

      return null;
   }

   public final int b(String var1) {
      int var2 = this.a(var1);
      boolean var3 = false;
      if (var2 != -1) {
         for (int var4 = 0; var4 < this.c.length; var4++) {
            if (this.c[var4] != null && this.c[var4][1] == var2) {
               return 0;
            }
         }

         for (int var5 = 0; var5 < this.e.length; var5++) {
            if (this.e[var5] != null && this.e[var5][1] == var2) {
               return 2;
            }
         }

         for (int var6 = 0; var6 < this.d.length; var6++) {
            if (this.d[var6] != null && this.d[var6][1] == var2) {
               return 1;
            }
         }
      }

      return -1;
   }

   public final int a() {
      int var1 = b.a.nextInt();

      for (int var2 = 1; this.l[var2][1] != 0; var2++) {
         if (var1 % this.l[var2][2] == 0 && this.l[var2][3] > 0) {
            this.l[var2][3]--;
            return this.l[var2][1];
         }
      }

      return 0;
   }

   public final boolean a(int var1, int var2) {
      boolean var3 = false;

      for (int var4 = 0; var4 < this.i[var1].length; var4++) {
         if (this.i[var1][var4] == var2) {
            return true;
         }
      }

      return false;
   }

   public static final String b(int var0) {
      switch (var0) {
         case 0:
            return b.a(523);
         case 1:
            return b.a(524);
         case 2:
            return b.a(525);
         case 3:
            return b.a(526);
         case 4:
            return b.a(527);
         case 5:
            return b.a(528);
         case 6:
            return b.a(529);
         case 7:
            return b.a(530);
         case 8:
            return b.a(531);
         case 9:
            return b.a(532);
         case 10:
            return b.a(533);
         case 11:
            return b.a(534);
         case 12:
            return b.a(535);
         case 13:
            return b.a(536);
         case 14:
            return b.a(537);
         default:
            return null;
      }
   }
}
