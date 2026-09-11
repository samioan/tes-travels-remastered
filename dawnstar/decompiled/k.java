import java.io.DataInputStream;

public class k {
   static String[] r = new String[]{
      "Weapon Peddler", "Heavy Armor Peddler", "Light Armor Peddler", "Jakar's", "Eustacia", "Alhavara", "Beatrice", "Chung", "Delacroix"
   };
   static byte[] c = new byte[]{4, 4, 4, 4, 2, 1, 1, 1, 1};
   static byte[] f = new byte[]{12, 6, 7, 12, 12, 1, 1, 1, 1};
   static byte[] e = new byte[]{12, 11, 7, 8, 6, 1, 1, 1, 1};
   static boolean[] p;
   static byte[] b;
   static byte[] k;
   static short[] q;
   static short[] o;
   static short d;
   static boolean h;
   static String[][] g;
   static int[] l = new int[]{3, 3, 3, 3, 14, 16, 16, 16, 16, 77};
   static boolean j = false;
   static byte[][] n = new byte[][]{
      {1, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13, 14, 15, 17, 18, 19, 20},
      {22, 23, 24, 25, 32, 33, 34, 35, 47, 48, 49, 50},
      {27, 28, 29, 30, 37, 38, 39, 40, 42, 43, 44, 45},
      {87, 88, 89, 90, 91, 93, 94, 95, 96}
   };
   static final byte[][] m = new byte[][]{{1, 3, 5, 8, 10, 12}, {1, 2, 4, 7, 9, 12}, {2, 3, 6, 7, 10, 11}, {2, 4, 5, 8, 9, 11}};
   static final byte[] a = new byte[]{13, 19, 25, 31, 14, 20, 26, 32, 15, 21, 27, 33, 17, 23, 29, 35, 16, 22, 28, 34, 18, 24, 30, 36};
   static final byte[] i = new byte[]{37, 43, 49, 55, 38, 44, 50, 56, 39, 45, 51, 57, 41, 47, 53, 59, 40, 46, 52, 58, 42, 48, 54, 60};

   static void c() {
      a("/npcstrings.dat");
   }

   static void a(String var0) {
      try {
         DataInputStream var1 = f.a(var0);
         byte var2 = 10;
         g = new String[var2][];

         for (int var3 = 0; var3 < var2; var3++) {
            a(var3, l[var3], var1);
         }

         j = true;
      } catch (Exception var4) {
         System.out.println("ERROR loading NPC and generic strings!");
         j = false;
      }
   }

   static void a(int var0, int var1, DataInputStream var2) throws Exception {
      int var3 = var2.readInt();
      if (var3 != var1) {
         System.out.println("Unexpected number of messages for whichNPC = " + var0);
         throw new Exception("Error in readNPCMessages: npc is " + var0);
      }

      g[var0] = new String[var3];

      for (int var4 = 0; var4 < var3; var4++) {
         g[var0][var4] = var2.readUTF();
      }
   }

   static void a() {
      p = new boolean[9];

      for (int var0 = 0; var0 < 9; var0++) {
         p[var0] = true;
      }

      q = new short[4];
      o = new short[4];
      b = new byte[4];
      k = new byte[4];

      for (int var1 = 0; var1 < 4; var1++) {
         q[var1] = 0;
         o[var1] = 0;
         b[var1] = 0;
         k[var1] = 0;
      }

      d = 0;
      h = false;
   }

   static void b() {
      for (int var0 = 0; var0 < 4; var0++) {
         b[var0] = 0;
         k[var0] = 0;
      }
   }

   static boolean a(int var0) {
      return c[var0] == 1;
   }

   static boolean b(int var0) {
      return c[var0] == 4;
   }

   static int a(int var0, int var1) {
      for (int var2 = 0; var2 < 5; var2++) {
         if (var0 == f[var2] && var1 == e[var2]) {
            return var2;
         }
      }

      return -1;
   }

   static int d(int var0, int var1) {
      int var2 = a.a(3, var1);
      if (var0 == 5) {
         return var2 >>> 6 & 3;
      } else if (var0 == 6) {
         return var2 >>> 4 & 3;
      } else if (var0 == 7) {
         return var2 >>> 2 & 3;
      } else {
         return var0 == 8 ? var2 & 3 : 0;
      }
   }

   static String a(j var0, int var1) {
      short var2 = var0.au[var1][0];
      if (var2 == 0) {
         var0.au[var1][0] = 1;
         String var5 = g[9][1];
         return f.a(var5, "<TAG>", j.ax[var1]);
      } else {
         var0.au[var1][0] = (short)(var2 + 1);
         String var3 = g[9][2];
         String[] var4 = new String[]{j.ax[var1], String.valueOf(var2), String.valueOf(var2 + 1)};
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
               return g[var1][0];
            } else if (var2 == 14) {
               int var16 = var3;
               byte var20 = n[var1][var16];
               int var25 = a.a(4, var20);
               if (var25 > var0.o) {
                  return g[var1][1];
               } else {
                  short var26 = a.a();
                  boolean var27 = var0.a(var20, var26, 0);
                  if (var27) {
                     var0.d(-var25);
                     return g[var1][2];
                  }

                  return "Sorry, but your pack is too full.";
               }
            } else {
               if (var2 == 15) {
                  int var15 = var3;
                  int var19 = Math.abs(var0.af[var15]);
                  if (a.a(1, var19) == 11) {
                     return "Sorry, you may not sell a gift item.  It should be given to one of the champions.";
                  }

                  int var24 = a.a(5, var19);
                  var0.d(var24);
                  var0.w(var15);
                  return "For that you can have " + var24 + " gold.";
               }

               return "quack";
            }
         case 5:
         case 6:
         case 7:
         case 8:
            if (var2 == 1) {
               if (p[var1]) {
                  p[var1] = false;
                  return g[var1][0];
               }

               int var14 = ESGame.nextInt(3);
               return g[var1][1 + var14];
            } else if (var2 == 2) {
               if (b[var1 - 5] != 0) {
                  return g[var1][4];
               }

               int var13 = var0.c(var1, var2);
               if (var13 == 0) {
                  b[var1 - 5] = 1;
               } else if (var13 == 1) {
                  var0.b(13, 2);
               } else if (var13 == 2) {
                  var0.b(13, 5);
                  o[var1 - 5]++;
                  b[var1 - 5] = 1;
               } else if (var13 == 3) {
                  var0.b(13, 8);
                  o[var1 - 5]++;
                  b[var1 - 5] = 1;
               }

               q[var1 - 5]++;
               return g[var1][5 + var13];
            } else if (var2 == 3) {
               if (k[var1 - 5] != 0) {
                  return g[var1][4];
               }

               int var12 = var0.c(var1, var2);
               int var18 = var3 <= 1 ? 0 : 1;
               if (var12 == 0) {
                  k[var1 - 5] = 2;
               } else if (var12 == 1) {
                  var0.b(13, 2);
                  k[var1 - 5] = 2;
               } else if (var12 == 2) {
                  var0.b(13, 5);
                  o[var1 - 5]++;
                  k[var1 - 5] = 1;
               } else if (var12 == 3) {
                  var0.b(13, 8);
                  o[var1 - 5]++;
                  k[var1 - 5] = 1;
               }

               q[var1 - 5]++;
               return g[var1][9 + var18];
            } else if (var2 == 4) {
               if (b[var1 - 5] != 2 && k[var1 - 5] != 2) {
                  int var11 = var3;
                  int var17 = Math.abs(var0.af[var11]);
                  if (a.a(1, var17) == 11) {
                     int var23 = d(var1, var17);
                     if (var23 > 0) {
                        var0.w(var11);
                        o[var1 - 5] = (short)(o[var1 - 5] + var23);
                        b[var1 - 5] = 0;
                        k[var1 - 5] = 0;
                     }

                     return g[var1][11 + var23];
                  }

                  return g[var1][11];
               }

               return g[var1][11];
            } else if (var2 == 5) {
               if (o[var1 - 5] == 0) {
                  return g[var1][15];
               }

               o[var1 - 5]--;
               int var10 = var3;
               return a(var0, var10);
            } else if (var2 != 8) {
               return "quack";
            }
         case 4:
            if (var2 == 1) {
               if (p[var1]) {
                  p[var1] = false;
                  var0.at = 0;
                  if (h) {
                     h = false;
                     return g[4][13] + "\n \n" + g[4][0] + "\n \n" + g[4][1] + "\n \n" + g[4][2];
                  }

                  return g[4][0] + "\n \n" + g[4][1] + "\n \n" + g[4][2];
               } else {
                  int var9 = ESGame.getGameAdvancementLevel(var0.av);
                  if (var9 > var0.at) {
                     var0.at++;
                  }

                  if (h) {
                     h = false;
                     return g[4][13];
                  }

                  return null;
               }
            } else {
               if (var2 != 13) {
                  if (var2 == 10) {
                     var0.r = 0;
                     return g[var1][9];
                  }

                  if (var2 == 11) {
                     if (!var0.w()) {
                        return g[var1][10];
                     }

                     var0.d();
                     return g[var1][11];
                  }

                  if (var2 == 12) {
                     var0.E[2] = var0.E[3];
                     var0.E[4] = var0.E[5];
                     return g[var1][12];
                  }

                  return null;
               }

               int var4 = 0;
               int var5 = 0;

               for (; var5 < 6; var5++) {
                  if (var0.ad[90 + var5]) {
                     var4++;
                  }
               }

               String var6 = "";
               if (var4 <= var0.at) {
                  var6 = g[var1][3 + var0.at];
                  int var7 = 0;
                  if (var4 < 6) {
                     var7 = f.a(6 - var4) - 1;
                  }

                  for (int var8 = 0; var8 < 6; var8++) {
                     if (!var0.ad[90 + var8]) {
                        if (var7 == 0) {
                           var7 = var8;
                           break;
                        }

                        var7--;
                     }
                  }

                  var0.ad[90 + var7] = true;
                  var6 = f.a(var6, "<TAG>", g[9][5 + m[var0.ai][var7]]);
               } else {
                  var6 = "I have no new rumors.";
               }

               return var6;
            }
         default:
            return "quack2";
      }
   }

   static boolean c(int var0, int var1) {
      switch (var0) {
         case 5:
            switch (var1) {
               case 7:
               case 8:
               case 10:
                  return true;
               case 9:
               default:
                  return false;
            }
         case 6:
            switch (var1) {
               case 1:
               case 3:
               case 4:
                  return true;
               case 2:
               default:
                  return false;
            }
         case 7:
            switch (var1) {
               case 0:
               case 2:
               case 5:
                  return true;
               default:
                  return false;
            }
         case 8:
            switch (var1) {
               case 6:
               case 12:
               case 13:
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
         case 5:
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
         case 6:
            switch (var1) {
               case 0:
                  return 1;
               case 1:
                  return 3;
               case 2:
                  return 4;
               default:
                  return -1;
            }
         case 7:
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
         case 8:
            switch (var1) {
               case 0:
                  return 6;
               case 1:
                  return 12;
               case 2:
                  return 13;
               default:
                  return -1;
            }
         default:
            return -1;
      }
   }

   static {
      a();
   }
}
