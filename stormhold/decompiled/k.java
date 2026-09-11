import java.io.DataInputStream;

public class k {
   static String[] s = new String[]{"Arantamo", "Celegil", "Favela Dralor", "Vander", "Beneca", "Helga", "Varus"};
   static byte[] e = new byte[]{1, 1, 1, 1, 2, 2, 3};
   static byte[] j = new byte[]{12, 3, 15, 6, 7, 12, 9};
   static byte[] i = new byte[]{3, 7, 7, 13, 2, 13, 9};
   static boolean[] b;
   static boolean[] q;
   static byte f = 0;
   static boolean d = false;
   static byte[] c;
   static byte[] n;
   static short[] r;
   static short[] p;
   static short[] h;
   static short a;
   static short g;
   static boolean l;
   static String[][] k;
   static int[] o = new int[]{20, 20, 20, 20, 5, 22, 5, 41};
   static boolean m = false;

   static void e() {
      a("/npcstrings.dat");
   }

   static void a(String var0) {
      try {
         DataInputStream var1 = f.a(var0);
         byte var2 = 8;
         k = new String[var2][];

         for (int var3 = 0; var3 < var2; var3++) {
            a(var3, o[var3], var1);
         }

         m = true;
      } catch (Exception var4) {
         System.out.println("ERROR loading NPC and generic strings!");
         m = false;
      }
   }

   static boolean a(int var0) {
      if (f == 0 && !d && var0 >= 13) {
         return true;
      } else {
         return f == 1 && !d && var0 >= 26 ? true : f == 2 && !d && var0 >= 39;
      }
   }

   static void c() {
      System.out.println("WARDEN VISITS!!");
      d = true;
      f++;
      byte var0 = j[6];
      byte var1 = i[6];
      ESGame.u[0].w[var0][var1] = (byte)(ESGame.u[0].w[var0][var1] | 32);
   }

   static void a() {
      System.out.println("WARDEN LEAVES!!");
      byte var0 = j[6];
      byte var1 = i[6];
      d = false;
      byte var2 = ESGame.u[1].w[var0][var1];
      ESGame.u[0].w[var0][var1] = f.c((byte)32, var2);
   }

   static void a(int var0, int var1, DataInputStream var2) throws Exception {
      int var3 = var2.readInt();
      if (var3 != var1) {
         System.out.println("Unexpected number of messages for whichNPC = " + var0);
         throw new Exception("Error in readNPCMessages: npc is " + var0);
      }

      k[var0] = new String[var3];

      for (int var4 = 0; var4 < var3; var4++) {
         k[var0][var4] = var2.readUTF();
      }
   }

   static void b() {
      b = new boolean[7];
      q = new boolean[7];

      for (int var0 = 0; var0 < 7; var0++) {
         b[var0] = true;
         q[var0] = true;
      }

      r = new short[4];
      p = new short[4];
      h = new short[4];
      c = new byte[4];
      n = new byte[4];

      for (int var1 = 0; var1 < 4; var1++) {
         r[var1] = 0;
         p[var1] = 0;
         h[var1] = 0;
         c[var1] = 0;
         n[var1] = 0;
      }

      a = 0;
      g = 0;
      l = false;
   }

   static void d() {
      for (int var0 = 0; var0 < 4; var0++) {
         c[var0] = 0;
         n[var0] = 0;
      }
   }

   static boolean b(int var0) {
      return e[var0] == 1;
   }

   static int a(int var0, int var1) {
      for (int var2 = 0; var2 < 7; var2++) {
         if (var0 == j[var2] && var1 == i[var2] && b[var2]) {
            return var2;
         }
      }

      return -1;
   }

   static int d(int var0, int var1) {
      int var2 = a.a(3, var1);
      if (var0 == 0) {
         return var2 >>> 6 & 3;
      } else if (var0 == 1) {
         return var2 >>> 4 & 3;
      } else if (var0 == 2) {
         return var2 >>> 2 & 3;
      } else {
         return var0 == 3 ? var2 & 3 : 0;
      }
   }

   static String a(j var0, int var1) {
      short var2 = var0.R[var1][0];
      if (var2 == 0) {
         var0.R[var1][0] = 1;
         String var5 = k[7][1];
         return f.a(var5, "<TAG>", j.E[var1]);
      } else {
         var0.R[var1][0] = (short)(var2 + 1);
         String var3 = k[7][2];
         String[] var4 = new String[]{j.E[var1], String.valueOf(var2), String.valueOf(var2 + 1)};
         return f.a(var3, "<TAG>", var4);
      }
   }

   static String a(j var0, int var1, int var2, int var3) {
      switch (var1) {
         case 0:
         case 1:
         case 2:
         case 3:
            if (var2 == 1) {
               if (q[var1]) {
                  q[var1] = false;
                  return k[var1][0];
               } else if (h[var1] > 50) {
                  return k[var1][1];
               } else {
                  if (var0.U[8] > 50) {
                     return k[var1][2];
                  }

                  int var16 = ESGame.h(3);
                  return k[var1][3 + var16];
               }
            } else if (var2 == 2) {
               if (c[var1] != 0) {
                  return k[var1][6];
               }

               int var15 = var0.b(var1, var2);
               if (var15 == 0) {
                  c[var1] = 1;
               } else if (var15 == 1) {
                  var0.R[13][2] = (short)(var0.R[13][2] + 2);
               } else if (var15 == 2) {
                  var0.R[13][2] = (short)(var0.R[13][2] + 5);
                  p[var1]++;
                  c[var1] = 1;
               } else if (var15 == 3) {
                  var0.R[13][2] = (short)(var0.R[13][2] + 8);
                  p[var1]++;
                  c[var1] = 1;
               }

               r[var1]++;
               return k[var1][7 + var15];
            } else if (var2 == 3) {
               if (n[var1] != 0) {
                  return k[var1][6];
               }

               int var14 = var0.b(var1, var2);
               int var21 = var3 <= 1 ? 0 : 1;
               if (var14 == 0) {
                  n[var1] = 2;
               } else if (var14 == 1) {
                  var0.R[13][2] = (short)(var0.R[13][2] + 2);
                  n[var1] = 2;
               } else if (var14 == 2) {
                  var0.R[13][2] = (short)(var0.R[13][2] + 5);
                  p[var1]++;
                  n[var1] = 1;
               } else if (var14 == 3) {
                  var0.R[13][2] = (short)(var0.R[13][2] + 8);
                  p[var1]++;
                  n[var1] = 1;
               }

               r[var1]++;
               return k[var1][11 + var21];
            } else if (var2 == 4) {
               if (c[var1] != 2 && n[var1] != 2) {
                  int var13 = var3;
                  int var20 = Math.abs(var0.H[var13]);
                  if (a.a(1, var20) == 15) {
                     var0.y(var13);
                     int var25 = a.a(3, var20);
                     h[var1] = (short)(h[var1] - var25);
                     h[var1] = (short)Math.max(h[var1], 0);
                     return k[var1][17];
                  }

                  if (a.a(1, var20) == 11) {
                     int var24 = d(var1, var20);
                     if (var24 > 0) {
                        var0.y(var13);
                        p[var1] = (short)(p[var1] + var24);
                        c[var1] = 0;
                        n[var1] = 0;
                     }

                     return k[var1][13 + var24];
                  }

                  return k[var1][13];
               }

               return k[var1][13];
            } else if (var2 == 5) {
               if (p[var1] == 0) {
                  return k[var1][18];
               } else if (h[var1] > 50) {
                  return k[var1][1];
               } else {
                  if (var0.U[8] > 50) {
                     return k[var1][2];
                  }

                  p[var1]--;
                  int var12 = var3;
                  return a(var0, var12);
               }
            } else {
               if (var2 == 6) {
                  b[var1] = false;
                  i var11 = ESGame.u[0];
                  var11.w[j[var1]][i[var1]] = f.c((byte)32, var11.w[j[var1]][i[var1]]);
                  return k[var1][19];
               }

               return null;
            }
         case 4:
            if (var2 == 1) {
               if (q[var1]) {
                  q[var1] = false;
                  return k[var1][0];
               }

               return null;
            } else if (var2 == 4) {
               int var10 = var3;
               int var19 = Math.abs(var0.H[var10]);
               int var23 = a.a(1, var19);
               if (var23 != 13 && var23 != 15 && var23 != 17) {
                  a++;
                  var0.y(var10);
                  return k[var1][2];
               }

               return k[var1][1];
            } else {
               if (var2 == 7) {
                  if (a / 3 > 0) {
                     int var9 = var3;
                     short var18 = a.a();
                     boolean var22 = var0.b(var9, var18, 0);
                     if (!var22) {
                        return k[7][0];
                     }

                     a = (short)(a - 3);
                     return k[var1][3];
                  }

                  return k[var1][4];
               }

               return null;
            }
         case 5:
            if (var2 == 1) {
               System.out.println("Greeting Helga");
               if (q[var1]) {
                  System.out.println("first meeting");
                  q[var1] = false;
                  var0.Y = 0;
                  System.out.println("message[iNPC] length is " + k[var1].length);
                  System.out.println(k[var1][0]);
                  System.out.println(k[var1][2]);
                  if (l) {
                     l = false;
                     return k[5][21] + "\n" + k[var1][0] + "\n" + k[var1][2];
                  }

                  return k[var1][0] + "\n" + k[var1][2];
               } else {
                  int var8 = ESGame.d(var0.W);
                  if (var8 > var0.Y) {
                     var0.Y++;
                     if (l) {
                        l = false;
                        return k[5][21] + "\n" + k[var1][2 + var0.Y];
                     }

                     return k[var1][2 + var0.Y];
                  } else {
                     if (l) {
                        l = false;
                        return k[5][21];
                     }

                     return null;
                  }
               }
            } else if (var2 == 13) {
               return k[var1][2 + var0.Y];
            } else if (var2 == 4) {
               int var7 = var3;
               int var17 = Math.abs(var0.H[var7]);
               if (a.a(1, var17) == 13) {
                  int var6 = var0.D(var7);
                  if (var6 > 3) {
                     g = (short)(g + 5);
                  } else {
                     g = (short)(g + 3);
                  }

                  var0.y(var7);
                  return k[var1][11];
               }

               return k[var1][12];
            } else if (var2 == 8) {
               if (g < 7) {
                  return k[var1][1];
               } else {
                  int var4 = var3;
                  int var5 = Math.abs(var0.H[var4]);
                  if (a.b(var5) && !var0.j(var4)) {
                     g = (short)(g - 7);
                     var0.h(var4);
                     return k[var1][13];
                  }

                  return k[var1][14];
               }
            } else if (var2 == 9) {
               if (g < 2) {
                  return k[var1][1];
               } else {
                  if (var0.f) {
                     return k[var1][15];
                  }

                  var0.f = true;
                  g = (short)(g - 2);
                  return k[var1][16];
               }
            } else if (var2 == 10) {
               if (g < 1) {
                  return k[var1][1];
               }

               var0.A = 0;
               g--;
               return k[var1][17];
            } else if (var2 == 11) {
               if (g < 1) {
                  return k[var1][1];
               } else {
                  if (!var0.x()) {
                     return k[var1][18];
                  }

                  g--;
                  var0.e();
                  return k[var1][19];
               }
            } else {
               if (var2 == 12) {
                  var0.U[2] = var0.U[3];
                  var0.U[4] = var0.U[5];
                  return k[var1][20];
               }

               return null;
            }
         case 6:
            if (f == 0) {
               return null;
            } else if (f == 1 && var0.m == 0) {
               var0.m = 1;
               return k[var1][0];
            } else if (f == 2 && var0.m <= 1) {
               var0.m = 2;
               return k[var1][1];
            } else if (f == 3 && var0.m <= 2) {
               var0.m = 3;
               return k[var1][2];
            } else {
               if (f == 4 && var0.m <= 3) {
                  var0.m = 4;
                  return k[var1][3] + "\n" + k[var1][4];
               }

               return null;
            }
         default:
            return null;
      }
   }

   static boolean a(j var0) {
      if (var0.j != 1) {
         return false;
      }

      int var1 = Math.abs(var0.l - j[6]);
      int var2 = Math.abs(var0.k - i[6]);
      return var1 + var2 == 1;
   }

   static boolean c(int var0, int var1) {
      switch (var0) {
         case 0:
            switch (var1) {
               case 3:
               case 4:
               case 13:
                  return true;
               default:
                  return false;
            }
         case 1:
            switch (var1) {
               case 7:
               case 8:
               case 10:
                  return true;
               case 9:
               default:
                  return false;
            }
         case 2:
            switch (var1) {
               case 1:
               case 6:
               case 12:
                  return true;
               default:
                  return false;
            }
         case 3:
            switch (var1) {
               case 0:
               case 2:
               case 5:
                  return true;
               default:
                  return false;
            }
         default:
            return false;
      }
   }

   static int b(int var0, int var1) {
      switch (var0) {
         case 0:
            switch (var1) {
               case 0:
                  return 3;
               case 1:
                  return 4;
               case 2:
                  return 13;
               default:
                  return -1;
            }
         case 1:
            switch (var1) {
               case 0:
                  return 7;
               case 1:
                  return 8;
               case 2:
                  return 10;
               default:
                  return -1;
            }
         case 2:
            switch (var1) {
               case 0:
                  return 1;
               case 1:
                  return 6;
               case 2:
                  return 12;
               default:
                  return -1;
            }
         case 3:
            switch (var1) {
               case 0:
                  return 0;
               case 1:
                  return 2;
               case 2:
                  return 5;
               default:
                  return -1;
            }
         default:
            return -1;
      }
   }

   static {
      b();
   }
}
