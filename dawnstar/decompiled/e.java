import com.nokia.mid.ui.FullCanvas;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

public class e extends FullCanvas implements Runnable {
   private static final Font ah = Font.getFont(64, 0, 8);
   private static final Font J = Font.getFont(64, 2, 16);
   private static final Font s = Font.getFont(64, 2, 16);
   private static final Font H = Font.getFont(64, 1, 16);
   static final int[][][] k = new int[][][]{
      {{12, 0, 0, 1}, {11, 0, -1, 1}, {12, 1, -1, 2}, {12, 2, -1, 3}, {11, 2, -2, 3}, {12, 3, -2, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {11, 1, -1, 2}, {12, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {11, 2, -1, 3}, {12, 3, -1, 4}, {12, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {11, 3, -1, 4}, {11, 3, -1, 4}},
      {{12, 0, 0, 1}, {12, 1, 0, 2}, {12, 2, 0, 3}, {12, 3, 0, 4}, {12, 3, 0, 4}, {12, 3, 0, 4}}
   };
   private static final byte[][] ad = new byte[][]{
      {1, 5, 43, 48, 0, 2, 23, 7, 1, 3, 0, 64, 2, 18, 27, 3, 24, 19, 4, 0, 0, 0},
      {6, 10, 43, 49, 7, 2, 18, 8, 8, 3, 3, 84, 11, 17, 24, 10, 0, 80, 9, 0, 0, 0},
      {11, 25, 40, 50, 14, 3, 0, 0, -1, -1, 9, 29, 16, 11, 0, 15, 41, 41, 17},
      {26, 40, 37, 50, 20, 3, 0, 0, -1, -1}
   };
   private static final byte[][] a = new byte[][]{
      {0, 0},
      {1, 0},
      {0, 1},
      {1, 2},
      {0, 2},
      {0, 1},
      {1, 1},
      {0, 2},
      {1, 2},
      {0, 2},
      {0, 0},
      {1, 0},
      {0, 0},
      {1, 0},
      {0, 0},
      {2, 0},
      {0, 0},
      {2, 0},
      {0, 0},
      {2, 0},
      {1, 0},
      {2, 0},
      {1, 0},
      {2, 0},
      {2, 0},
      {0, 0},
      {1, 0},
      {0, 0},
      {1, 0},
      {0, 0},
      {2, 0},
      {0, 0},
      {2, 0},
      {0, 0},
      {2, 0},
      {1, 0},
      {2, 0},
      {1, 0},
      {2, 0},
      {1, 0},
      {0, 0}
   };
   private static final boolean[][] G = new boolean[][]{
      {false, false, false, false},
      {true, true, false, false},
      {false, false, true, false},
      {true, false, false, false},
      {true, true, true, false},
      {false, false, false, false},
      {true, false, false, false},
      {false, false, true, false},
      {false, true, false, false},
      {true, true, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {false, false, false, false},
      {true, false, true, false},
      {true, false, true, false},
      {true, false, true, false},
      {true, false, true, false},
      {true, false, true, false},
      {true, false, true, false},
      {true, false, true, false},
      {true, false, true, false},
      {true, true, false, false},
      {true, true, false, false},
      {true, true, false, false},
      {true, true, false, false},
      {true, true, false, false},
      {true, true, false, false},
      {true, true, false, false},
      {false, false, false, false}
   };
   private static final char[] e = new char[]{'1', '3', '5', '7', '9', '0'};
   private static final char[] L = new char[]{'0', 'N', 'E', 'S', 'W'};
   private static final int[][] m = new int[][]{{0, 0, 0}, {0, 1, 0}, {0, 2, 1}};
   private ESGame E;
   private int ab;
   private int Q;
   private Thread az;
   private boolean ai;
   private boolean v;
   private boolean aK;
   j aD;
   byte aM;
   boolean r;
   byte c;
   long aL;
   long p;
   long y;
   long T;
   boolean aC;
   int n;
   static boolean Z;
   static boolean j;
   static int U = -1;
   static Image h;
   static Image aE;
   static Image W;
   static Image ax;
   static Image w;
   static Image[] o;
   static Image[] aa;
   static Image[] au;
   static Image al;
   static Image R;
   private static byte[][] C;
   private static boolean f = false;
   static boolean O = false;
   static boolean ap = false;
   static boolean am = false;
   static int as;
   static boolean aA = false;
   static boolean aF = false;
   static boolean aG = false;
   static int P = 0;
   static int aI = 0;
   static boolean F = false;
   static boolean X = false;
   static boolean ar = false;
   static boolean S = false;
   static boolean ac = false;
   static long av = 0L;
   static String[] d = null;
   private static int V = 0;
   static boolean ae = false;
   private static boolean ak = true;
   private static boolean aw = false;
   static boolean B = false;
   static boolean x = false;
   static final String[] z = new String[]{"Cannot", "Camp!"};
   static final String[] N = new String[]{"No spells!", ""};
   static final String[] q = new String[]{"Not enough", "magicka!"};
   static final String[] g = new String[]{"No monster", "here!"};
   static final String[] K = new String[]{"Rest", "disturbed!"};
   static final String[] b = new String[]{"Rest", "complete!"};
   static final String[] M = new String[]{"Creature", "is dead!"};
   static final String[] af = new String[]{"Creature", "attacks!"};
   static final String[] t = new String[]{"Chest", ""};
   static final String[] A = new String[]{"Chest", "locked!"};
   static final String[] aJ = new String[]{"Inventory", "full!"};
   static final String[] I = new String[]{"Found", "item!"};
   static final String[] ao = new String[]{"Several", "items!"};
   static final String[] aB = new String[]{"Enemy", "arrived!"};
   static d i = null;
   private static long ag = 0L;
   private static boolean aH = false;
   private static String D = null;
   private static final int[] aj = new int[]{0, 0, 36, 72, 90, 108, 126, 144, 158, 176, 194, 212};
   private boolean aq = false;
   private boolean u = false;
   g Y;
   boolean an;
   int l;
   int ay;
   public boolean aN = false;
   Image at;

   public e(ESGame var1) {
      this.az = new Thread(this);
      this.E = var1;
      this.l = this.getHeight();
      this.ay = this.getWidth();
      this.Q = 0;
      this.ab = 0;
      this.v = false;
      this.ai = false;
      this.aK = false;
      this.aD = null;
      this.aM = 1;
      this.r = false;
      this.c = 0;
      this.p = 0L;
      this.aL = 0L;
      this.n = 0;
      this.aC = false;
      Z = false;
      j = false;
      U = -1;
      C = new byte[17][17];
      as = 0;
      this.y = 0L;
      this.T = 0L;
      this.Y = null;
      this.an = false;
      this.at = Image.createImage(89, 89);
   }

   public void paint(Graphics var1) {
      if (this.Y != null) {
         try {
            this.Y.b(var1);
         } catch (Exception var3) {
            this.E.displayDebug();
            System.out.println("paint " + var3.toString());
         }
      } else if (this.aM == 3) {
         this.e(var1);
      } else if (this.c != 1 && this.c != 3 && this.c != 2) {
         this.j(var1);
      } else {
         this.b(var1);
      }
   }

   private void e(Graphics var1) {
      var1.setColor(0);
      var1.fillRect(0, 0, this.ay, this.l);
      var1.setColor(16777215);
      var1.setFont(s);
      var1.drawString("You're Dead!", this.ay >> 1, this.l >> 1, 33);
   }

   private void b(Graphics var1) {
      var1.setColor(0);
      var1.fillRect(0, 0, this.ay, this.l);
      var1.setColor(16777215);
      var1.setFont(s);
      var1.drawString("CAMPING", this.ay >> 1, this.l >> 1, 33);
   }

   private void j(Graphics var1) {
      var1.setColor(0);
      var1.fillRect(0, 0, this.ay, this.l);
      this.h(var1);

      try {
         this.f(var1);
      } catch (Throwable var3) {
         System.out.println("Error in paintMonsters: " + var3);
      }

      if (U >= 0) {
         this.a(var1, U);
      }

      this.a(var1);
      this.c(var1);
      this.i(var1);
      this.d(var1);
      if (aH) {
         this.g(var1);
      }

      if (!this.aD.i(3)) {
         var1.setColor(16777215);
         if (!f) {
            var1.setFont(ah);
            var1.drawChar(L[this.aD.aw], 16, 10, 20);
            var1.setClip(10, 20, 23, 23);
            var1.drawImage(this.at, 10, 20, 20);
         } else {
            var1.setFont(H);
            var1.drawChar(L[this.aD.aw], 58, 10, 20);
            var1.drawImage(this.at, 15, 25, 20);
         }
      }
   }

   private void g(Graphics var1) {
      var1.setColor(16777215);
      var1.drawString(D, 60, 10, 17);
   }

   private void d(Graphics var1) {
      if (O) {
         int var2 = 40 + f.a(30);
         int var3 = 50 + f.a(20);
         this.b(var1, 6, var2, var3);
         O = false;
      }

      if (ap) {
         int var4 = 40 + f.a(30);
         int var6 = 50 + f.a(22);
         this.b(var1, 8, var4, var6);
         ap = false;
      }

      if (am) {
         int var5 = 50 + f.a(2);
         int var7 = 80 + f.a(2);
         this.b(var1, 7, var5, var7);
         am = false;
      }

      var1.setClip(0, 0, this.ay, this.l);
   }

   private void h(Graphics var1) {
      i var2 = this.aD.a();
      if (!this.aD.i(3)) {
         if (this.aD.i(4)) {
            var1.setColor(10485760);
            var1.fillRect(0, 0, this.ay, h.getHeight());
         } else {
            for (int var5 = 0; var5 < 5; var5++) {
               if (var2.e != 1) {
                  var1.drawImage(aE, var5 * 36, 0, 20);
               } else {
                  var1.drawImage(h, var5 * 36, 0, 20);
               }
            }
         }
      }

      this.aq = false;
      this.u = false;

      for (int var16 = 0; var16 < 5; var16++) {
         int var4 = var16 * 18;

         for (int var6 = 0; var6 < 6; var6++) {
            int var7 = k[var16][var6][0];
            int var8 = k[var16][var6][1];
            int var9 = k[var16][var6][2];
            int var10 = k[var16][var6][3];
            if (f.a((byte)1, this.aD.a(var9, var10))) {
               int var12 = this.a(var7, var8, -1);
               this.a(var1, var12, var4, var2.e);
               break;
            }

            if (f.a((byte)64, this.aD.a(var9, var10))) {
               int var3 = this.a(var7, var8, -1);
               this.a(var1, var3, var4, -1);
               break;
            }
         }
      }

      for (int var17 = 5; var17 < 10; var17++) {
         int var15 = var17 * 18;

         for (int var18 = 0; var18 < 6; var18++) {
            int var19 = k[9 - var17][var18][0];
            int var20 = k[9 - var17][var18][1];
            int var21 = -k[9 - var17][var18][2];
            int var11 = k[9 - var17][var18][3];
            if (f.a((byte)1, this.aD.a(var21, var11))) {
               int var14 = this.a(var19, var20, 1);
               this.a(var1, var14, var15, var2.e);
               break;
            }

            if (f.a((byte)64, this.aD.a(var21, var11))) {
               int var13 = this.a(var19, var20, 1);
               this.a(var1, var13, var15, -1);
               break;
            }
         }
      }

      var1.setClip(0, 0, this.ay, this.l);
   }

   private void a(Graphics var1, int var2, int var3, int var4) {
      var1.setClip(var3, 0, 18, this.l);
      if (var2 != 0 && var2 != 1) {
         this.aq = false;
         if (var2 == 2) {
            if (this.u) {
               this.u = false;
               if (var4 == -1) {
                  var1.drawImage(w, var3 - aj[var2] - 18, 8, 20);
               } else if (var4 != 1) {
                  var1.drawImage(ax, var3 - aj[var2] - 18, 0, 20);
               } else {
                  var1.drawImage(W, var3 - aj[var2] - 18, 0, 20);
               }

               return;
            }

            this.u = true;
         }
      } else {
         this.u = false;
         if (this.aq) {
            this.aq = false;
            if (var4 == -1) {
               var1.drawImage(w, var3 - aj[var2] - 18, 8, 20);
            } else if (var4 != 1) {
               var1.drawImage(ax, var3 - aj[var2] - 18, 0, 20);
            } else {
               var1.drawImage(W, var3 - aj[var2] - 18, 0, 20);
            }

            return;
         }

         this.aq = true;
      }

      if (var4 == -1) {
         var1.drawImage(w, var3 - aj[var2], 8, 20);
      } else if (var4 != 1) {
         var1.drawImage(ax, var3 - aj[var2], 0, 20);
      } else {
         var1.drawImage(W, var3 - aj[var2], 0, 20);
      }
   }

   public String[] a(String var1, int var2, Font var3) {
      int var4 = 0;
      String var5 = null;
      if ((var4 = var1.indexOf(10, 0)) >= 0) {
         if (var4 != var1.length() - 1) {
            String[] var15;
            if (var4 == 0) {
               var15 = new String[]{" "};
            } else {
               var15 = this.a(var1.substring(0, var4), var2, var3);
            }

            String[] var16 = this.a(var1.substring(var4 + 1), var2, var3);
            var5 = new String[var15.length + var16.length];
            int var18 = var15.length;
            System.arraycopy(var15, 0, var5, 0, var18);
            System.arraycopy(var16, 0, var5, var18, var16.length);
            return var5;
         }

         var1 = var1.substring(0, var1.length() - 1);
      }

      if (var3.stringWidth(var1) < var2) {
         return new String[]{var1};
      }

      var1 = var1 + " ";
      Vector var6 = new Vector();
      int var7 = 0;
      var2 -= 8;

      while ((var4 = var1.indexOf(" ", var7 + 1)) > 0) {
         if (var3.substringWidth(var1, 0, var4) < var2) {
            var7 = var4;
         } else {
            if (var7 == 0) {
               for (int var8 = 0; var8 < var2; var7++) {
                  var8 += var3.charWidth(var1.charAt(var7));
               }

               var6.addElement(var1.substring(0, var7));
               var7--;
            } else {
               var6.addElement(var1.substring(0, var7));
            }

            var1 = var1.substring(var7 + 1);
            var7 = 0;
         }
      }

      if (var1.length() > 0 && !var1.equals(" ")) {
         var6.addElement(var1);
      }

      String[] var13 = new String[var6.size()];

      for (int var17 = var13.length - 1; var17 >= 0; var17--) {
         var13[var17] = (String)var6.elementAt(var17);
      }

      return var13;
   }

   private String[] a(String var1) {
      if (var1 == null) {
         var1 = "";
      }

      String[] var2 = new String[2];
      Object var3 = this.a(var1, 69, ah);
      if (((Object[])var3).length == 1) {
         var2[0] = new String((String)((Object[])var3)[0]);
         var2[1] = "";
      } else {
         System.arraycopy(var3, 0, var2, 0, 2);
      }

      var3 = null;
      return var2;
   }

   private boolean a(String[] var1, int var2) {
      if (var2 <= V && var2 >= 0) {
         return false;
      }

      d = var1;
      if (var2 < 0) {
         V = 10;
      } else {
         V = var2;
      }

      return true;
   }

   public void c() {
      byte var1 = this.aD.a(0, 1);
      if (f.a((byte)32, var1)) {
         int var2 = this.aD.p();
         if (var2 == -1) {
            U = -1;
            System.out.println("NPC infront is not defined!!!");
            return;
         }

         U = var2;
         if (this.a(this.a(k.r[var2]), 1)) {
            av = System.currentTimeMillis();
            ac = true;
         }
      } else {
         U = -1;
      }
   }

   private int a(int var1, int var2, int var3) {
      if (var1 == 12) {
         return var2;
      } else {
         return var3 == -1 ? 8 + var2 : 7 - var2;
      }
   }

   private void a(Graphics var1, int var2) {
      switch (var2) {
         case 0:
            this.c(var1, 1, 2);
            break;
         case 1:
            this.c(var1, 4, 1);
            break;
         case 2:
            this.c(var1, 7, 0);
            break;
         case 3:
            this.c(var1, 6, 2);
            break;
         case 4:
            this.c(var1, 8, 1);
            break;
         case 5:
            this.c(var1, 9, 0);
            break;
         case 6:
            this.c(var1, 10, 2);
            break;
         case 7:
            this.c(var1, 2, 0);
            break;
         case 8:
            this.c(var1, 3, 2);
      }
   }

   private void a(Graphics var1, boolean var2) {
      byte var3 = 33;
      byte var4 = 48;
      this.a(var1, o[23], var2 ? 0 : 1, 2, var3, var4);
      var1.setClip(0, 0, this.ay, this.l);
   }

   private void f(Graphics var1) {
      x = false;

      for (int var2 = 8; var2 <= 12; var2++) {
         Object var3 = j.al.elementAt(var2);
         if (var3 instanceof byte[]) {
            byte[] var4 = (byte[])var3;
            if (var4.length == 28) {
               if (var4[6] != 0) {
                  x = true;
                  this.a(var1, this.a(var4[2]), var2);
               }
            } else if (var4.length == 8 || var4.length == 7) {
               this.a(var1, var4.length == 8, var2);
            }
         } else if (var3 instanceof String) {
            String var7 = (String)var3;
            if (var7.equals("W")) {
               this.a(var1, 25, var2);
            } else if (!var7.equals("C") && !var7.equals("D") && !var7.equals(k.r[0]) && !var7.equals(k.r[1])) {
               this.a(var1, 13, var2);
            } else {
               this.a(var1, 6, var2);
            }
         }
      }

      for (int var6 = 4; var6 <= 6; var6++) {
         Object var8 = j.al.elementAt(var6);
         if (var8 instanceof byte[]) {
            byte[] var5 = (byte[])var8;
            if (var5.length == 28) {
               if (var5[6] != 0) {
                  x = true;
                  this.b(var1, this.c(var5[2]), var6);
               }
            } else if (var5.length == 8 || var5.length == 7) {
               this.b(var1, var5.length == 8, var6);
            }
         } else if (var8 instanceof String) {
            String var10 = (String)var8;
            if (var10.equals("W")) {
               this.b(var1, 24, var6);
            } else if (!var10.equals("C") && !var10.equals("D") && !var10.equals(k.r[0]) && !var10.equals(k.r[1])) {
               this.b(var1, 12, var6);
            } else {
               this.b(var1, 5, var6);
            }
         }
      }

      Object var9 = j.al.elementAt(1);
      if (var9 instanceof byte[]) {
         byte[] var11 = (byte[])var9;
         if (var11.length == 28) {
            if (var11[6] != 0) {
               x = true;
               this.c(var1, var11[2], -1);
            }
         } else if (var11.length == 8 || var11.length == 7) {
            this.b(var1, var11.length == 8);
         }
      }
   }

   private void b(Graphics var1, boolean var2) {
      byte var3 = 60;
      int var4 = 110;
      if (var2) {
         var1.drawImage(aa[0], var3, var4, 20);
      } else {
         var4 += 14;
         var1.drawImage(au[0], var3, var4, 20);
      }
   }

   private void b(Graphics var1, boolean var2, int var3) {
      short var4 = 0;
      int var5 = 97;
      switch (var3) {
         case 4:
            var4 = 14;
            break;
         case 5:
            if (var2) {
               var4 = 68;
            } else {
               var4 = 73;
            }
            break;
         case 6:
            if (var2) {
               var4 = 125;
            } else {
               var4 = 142;
            }
      }

      if (var2) {
         var1.drawImage(aa[1], var4, var5, 20);
      } else {
         var5 += 8;
         var1.drawImage(au[1], var4, var5, 20);
      }
   }

   private void a(Graphics var1, boolean var2, int var3) {
      short var4 = 0;
      byte var5 = 87;
      switch (var3) {
         case 8:
            var4 = 10;
            break;
         case 9:
            var4 = 46;
            break;
         case 10:
            var4 = 84;
            break;
         case 11:
            var4 = 120;
            break;
         case 12:
            var4 = 156;
      }

      if (var2) {
         var1.drawImage(aa[2], var4, var5, 20);
      } else {
         var1.drawImage(au[2], var4, var5, 20);
      }
   }

   private int c(int var1) {
      if (var1 >= 1 && var1 <= 5) {
         return 5;
      } else if (var1 >= 6 && var1 <= 10) {
         return 12;
      } else if (var1 >= 11 && var1 <= 25) {
         return 21;
      } else if (var1 >= 26 && var1 <= 40) {
         return 18;
      } else {
         return var1 != 41 && var1 != 42 ? -1 : 24;
      }
   }

   private int a(int var1) {
      if (var1 >= 1 && var1 <= 5) {
         return 6;
      } else if (var1 >= 6 && var1 <= 10) {
         return 13;
      } else if (var1 >= 11 && var1 <= 25) {
         return 22;
      } else if (var1 >= 26 && var1 <= 40) {
         return 19;
      } else {
         return var1 != 41 && var1 != 42 ? -1 : 25;
      }
   }

   private int b(int var1) {
      if (var1 >= 1 && var1 <= 5) {
         return 0;
      } else if (var1 >= 6 && var1 <= 10) {
         return 1;
      } else if (var1 >= 11 && var1 <= 25) {
         return 3;
      } else if (var1 >= 26 && var1 <= 40) {
         return 2;
      } else {
         return var1 != 41 && var1 != 42 ? -1 : 4;
      }
   }

   private void c(Graphics var1, int var2, int var3) {
      int var4 = this.b(var2);
      if (var4 == 4) {
         this.a(var1, var2 == 41);
      } else {
         if (var4 >= 0) {
            byte var6 = ad[var4][2];
            byte var7 = ad[var4][3];
            byte var8 = ad[var4][4];
            byte var9 = ad[var4][5];
            int var10 = var6 + ad[var4][6];
            int var11 = var7 + ad[var4][7];
            byte var12 = ad[var4][8];
            byte var13 = ad[var4][9];
            boolean var5;
            if (var12 >= 0) {
               var5 = true;
            } else {
               var5 = false;
            }

            byte var14 = a[var2 - 1][0];
            int var15 = a[var2 - 1][1];
            if (var3 >= 0) {
               var15 = var3;
            }

            boolean var16 = G[var2 - 1][0];
            boolean var17 = G[var2 - 1][1];
            boolean var18 = G[var2 - 1][2];
            boolean var19 = G[var2 - 1][3];
            this.a(var1, o[var8], var14, var9, var6, var7);
            if (var5) {
               this.a(var1, o[var12], var15, var13, var10, var11);
            }

            if (var16) {
               int var20 = var6 + ad[var4][10];
               int var21 = var7 + ad[var4][11];
               byte var22 = ad[var4][12];
               this.a(var1, o[var22], 0, 1, var20, var21);
            }

            if (var17) {
               int var23 = var6 + ad[var4][13];
               int var26 = var7 + ad[var4][14];
               byte var29 = ad[var4][15];
               this.a(var1, o[var29], 0, 1, var23, var26);
            }

            if (var18) {
               int var24 = var6 + ad[var4][16];
               int var27 = var7 + ad[var4][17];
               byte var30 = ad[var4][18];
               this.a(var1, o[var30], 0, 1, var24, var27);
            }

            if (var19) {
               int var25 = var6 + ad[var4][19];
               int var28 = var7 + ad[var4][20];
               byte var31 = ad[var4][21];
               this.a(var1, o[var31], 0, 1, var25, var28);
            }
         }

         var1.setClip(0, 0, this.ay, this.l);
      }
   }

   private void b(Graphics var1, int var2, int var3) {
      byte var4 = 0;
      byte var5 = 38;
      switch (var3) {
         case 4:
            var4 = 10;
            break;
         case 5:
            var4 = 62;
            break;
         case 6:
            var4 = 112;
      }

      var1.drawImage(o[var2], var4, var5, 20);
   }

   private void a(Graphics var1, int var2, int var3) {
      short var4 = 0;
      byte var5 = 44;
      switch (var3) {
         case 8:
            var4 = 10;
            break;
         case 9:
            var4 = 44;
            break;
         case 10:
            var4 = 79;
            break;
         case 11:
            var4 = 112;
            break;
         case 12:
            var4 = 146;
      }

      var1.drawImage(o[var2], var4, var5, 20);
   }

   private void a(Graphics var1) {
      var1.setColor(16776960);
      var1.fillRect(5, 130, 40, 7);
      var1.fillRect(5, 138, 40, 7);
      var1.fillRect(5, 146, 40, 7);
      var1.setColor(16711680);
      int var2 = this.aD.l(2) * 38 / this.aD.E[3];
      var1.fillRect(6, 131, var2, 5);
      var1.setColor(65280);
      var2 = this.aD.l(4) * 38 / this.aD.E[5];
      var1.fillRect(6, 139, var2, 5);
      var1.setColor(255);
      var2 = this.aD.l(6) * 38 / this.aD.E[7];
      if (var2 > 40) {
         var2 = 40;
      }

      var1.fillRect(6, 147, var2, 5);
   }

   private void i(Graphics var1) {
      if (ac) {
         var1.setColor(13080935);
         var1.fillRoundRect(96, 118, 75, 35, 5, 5);
         var1.setFont(ah);
         var1.setColor(0);
         var1.drawString(d[0], 100, 122, 20);
         if (d.length > 1) {
            var1.drawString(d[1], 100, 134, 20);
         }
      }
   }

   private void c(Graphics var1) {
      var1.setFont(ah);
      var1.setClip(0, 0, this.ay, this.l);
      var1.drawImage(R, 0, 156, 20);
      var1.setColor(0);
      int var2 = this.j();
      as = var2;
      if (var2 == 0) {
         var1.drawChar(e[1], 26, 191, 20);
         var1.drawChar(e[2], 66, 191, 20);
         var1.drawChar(e[3], 106, 191, 20);
         var1.drawChar(e[5], 146, 191, 20);
         var1.setColor(16777215);
         var1.drawChar(e[1], 25, 190, 20);
         var1.drawChar(e[2], 65, 190, 20);
         var1.drawChar(e[3], 105, 190, 20);
         var1.drawChar(e[5], 145, 190, 20);
         this.b(var1, 1, 13, 164);
         this.b(var1, 2, 53, 164);
         this.b(var1, 3, 93, 164);
         this.b(var1, 5, 133, 164);
      } else if (var2 == 1) {
         var1.drawChar(e[0], 26, 191, 20);
         var1.drawChar(e[1], 66, 191, 20);
         var1.drawChar(e[2], 106, 191, 20);
         var1.drawChar(e[3], 146, 191, 20);
         var1.setColor(16777215);
         var1.drawChar(e[0], 25, 190, 20);
         var1.drawChar(e[1], 65, 190, 20);
         var1.drawChar(e[2], 105, 190, 20);
         var1.drawChar(e[3], 145, 190, 20);
         this.b(var1, 0, 13, 164);
         this.b(var1, 1, 53, 164);
         this.b(var1, 2, 93, 164);
         this.b(var1, 3, 133, 164);
      } else if (var2 == 2) {
         var1.drawChar(e[1], 26, 191, 20);
         var1.drawChar(e[2], 66, 191, 20);
         var1.drawChar(e[3], 106, 191, 20);
         var1.drawChar(e[4], 146, 191, 20);
         var1.setColor(16777215);
         var1.drawChar(e[1], 25, 190, 20);
         var1.drawChar(e[2], 65, 190, 20);
         var1.drawChar(e[3], 105, 190, 20);
         var1.drawChar(e[4], 145, 190, 20);
         this.b(var1, 1, 13, 164);
         this.b(var1, 2, 53, 164);
         this.b(var1, 3, 93, 164);
         this.b(var1, 4, 133, 164);
      }

      var1.setClip(0, 0, this.ay, this.l);
   }

   private void b(Graphics var1, int var2, int var3, int var4) {
      var1.setClip(var3, var4, 30, 24);
      var1.drawImage(al, var3 - 30 * var2, var4, 20);
   }

   private int j() {
      if (Z) {
         return 1;
      } else {
         return !j && U < 0 ? 0 : 2;
      }
   }

   private void a(Graphics var1, Image var2, int var3, int var4, int var5, int var6) {
      int var7 = var2.getWidth() / var4;
      int var8 = var2.getHeight();
      var1.setClip(var5, var6, var7, var8);
      var1.drawImage(var2, var5 - var3 * var7, var6, 20);
   }

   private void a(Graphics var1, int var2, int var3, int var4, int var5) {
      var1.setColor(0);
      var1.fillRect(0, 0, var4 * var5 + var2 << 1, var4 * var5 + var2 << 1);
      var1.setColor(16777215);
      var1.drawRect(0, 0, var4 * var5 + var2 << 0, var4 * var5 + var2 << 0);
      if (var2 == 2) {
         var1.drawRect(1, 1, var4 * var5 + var2 << -1, var4 * var5 + var2 << -1);
      }

      int var6 = var4 / 2;

      for (int var8 = 0; var8 < var4; var8++) {
         int var9 = var3 + var8 * var5;

         for (int var10 = 0; var10 < var4; var10++) {
            int var11 = var2 + var10 * var5;
            if (var8 == var6 && var10 == var6) {
               var1.setColor(65280);
               var1.fillRect(var11, var9, var5, var5);
            } else {
               byte var7 = C[var10][var8];
               if (var7 != 1) {
                  if (var7 == 0) {
                     var1.setColor(16777215);
                     var1.fillRect(var11, var9, var5, var5);
                  } else if ((var7 & 2) != 0) {
                     var1.setColor(16711680);
                     var1.fillRect(var11, var9, var5, var5);
                  } else if ((var7 & 4) != 0) {
                     var1.setColor(255);
                     var1.fillRect(var11, var9, var5, var5);
                  } else if ((var7 & 8) != 0) {
                     var1.setColor(13369599);
                     var1.fillRect(var11, var9, var5, var5);
                  }
               }
            }
         }
      }
   }

   void p() {
      this.aN = false;
      byte var1 = this.aD.x;
      byte var2 = this.aD.w;
      byte var3 = this.aD.aw;
      if (!f) {
         this.aD.a().a(var1, var2, var3, 7, C);
         this.a(this.at.getGraphics(), 1, 1, 7, 3);
      } else {
         this.aD.a().a(var1, var2, var3, 17, C);
         this.a(this.at.getGraphics(), 2, 2, 17, 5);
      }
   }

   public void keyPressed(int var1) {
      if (this.Y != null) {
         this.Y.c(var1);
      } else {
         this.Q = this.ab;
         if (var1 == 49) {
            if (as == 1) {
               aA = true;
            }
         } else if (var1 == 50) {
            this.n = 1;
         } else if (var1 == 51) {
            ar = true;
         } else if (var1 == 52) {
            this.aC = true;
            this.n = 4;
         } else if (var1 == 53) {
            S = true;
         } else if (var1 == 54) {
            this.aC = true;
            this.n = 3;
         } else if (var1 == 55) {
            X = true;
         } else if (var1 == 56) {
            this.n = 2;
         } else if (var1 == 57) {
            if (as == 2) {
               aF = true;
            } else {
               aG = true;
            }
         } else if (var1 == 48) {
            if (as == 0) {
               F = true;
            }
         } else if (var1 == 42) {
            f = !f;
            this.aN = true;
         } else {
            this.aC = false;
            this.ab = this.getGameAction(var1);
            switch (this.ab) {
               case 1:
                  this.n = 1;
                  break;
               case 2:
                  this.n = 4;
               case 3:
               case 4:
               default:
                  break;
               case 5:
                  this.n = 3;
                  break;
               case 6:
                  this.n = 2;
            }
         }
      }
   }

   public void keyReleased(int var1) {
      int var2 = this.getGameAction(var1);
      this.Q = this.ab;
      this.ab = 0;
   }

   void g() {
      if (this.az != null) {
         this.v = true;
         if (this.az.isAlive()) {
            System.out.println("Killing game thread");
            this.aK = true;

            try {
               this.az.join();
            } catch (Exception var2) {
            }

            System.out.println("Done killing game thread");
         }

         this.az = null;
         this.v = false;
         System.gc();
      }
   }

   void i() {
      try {
         this.g();
         this.az = new Thread(this);
         this.v = true;
         this.az.start();
      } catch (Throwable var2) {
         System.out.println(" start error:");
         var2.printStackTrace();
         this.repaint();
         this.serviceRepaints();
      }
   }

   public void run() {
      long var1 = System.currentTimeMillis();
      long var5 = 0L;
      long var7 = 0L;

      try {
         while (this.v) {
            if (this.Y != null && this.an) {
               this.an = false;
               this.repaint();
               this.serviceRepaints();

               try {
                  Thread.sleep(100L);
               } catch (Exception var17) {
               }
            } else if (this.ai) {
               long var20 = System.currentTimeMillis();

               try {
                  Thread.sleep(250L);
               } catch (Exception var15) {
               }

               if (this.aK) {
                  this.aK = false;
                  return;
               }
            } else {
               boolean var9 = true;
               aw = false;
               System.gc();
               if (this.c == 1 || this.c == 3) {
                  var9 = false;
                  if (var1 - this.aL > 2500L) {
                     if (this.c != 3 && !this.o()) {
                        this.c = 2;
                     } else {
                        this.aL = 0L;
                        this.r = true;
                        this.aD.h(false);
                        if (this.c == 3) {
                           this.aD.M = true;
                           if (!this.aD.a().a(this.aD.x, this.aD.w, 41)) {
                           }
                        } else {
                           this.aD.a().a(this.aD.x, this.aD.w, -1);
                        }

                        this.c = 0;
                        var9 = true;
                        if (this.a(K, 1)) {
                           av = var1;
                           ac = true;
                        }
                     }
                  }
               } else if (this.c == 2) {
                  var9 = false;
                  if (var1 - this.aL > 5000L) {
                     this.c = 0;
                     this.aL = 0L;
                     this.r = true;
                     this.aD.h(true);
                     if (this.a(b, 1)) {
                        av = var1;
                        ac = true;
                     }

                     var9 = true;
                  }
               } else if (this.aM != 1) {
                  if (this.aM == 2) {
                     this.aM = 3;
                     ac = false;
                     V = 0;
                  }

                  var9 = false;
                  if (var1 - this.p > 5000L) {
                     System.out.println("Restart after dead");
                     this.aD.a(this.aD.E);

                     for (int var10 = this.aD.aq - 1; var10 >= 0; var10--) {
                        if (!this.aD.A(var10)) {
                           this.aD.w(var10);
                        }
                     }

                     this.aD.T = false;
                     this.aD.f(true);
                     this.p = 0L;
                     this.aM = 1;
                     k.h = true;
                     this.r = true;
                     this.aN = true;
                     B = true;
                     var9 = true;
                     if (B) {
                        if (this.a(this.a(this.aD.a().a()), 1)) {
                           av = System.currentTimeMillis();
                           ac = true;
                        }

                        B = false;
                     }
                  }
               }

               if (var9) {
                  byte var21 = this.aD.a().a(var1, this.aD);
                  if ((var21 & 1) != 0) {
                     this.aN = true;
                  }

                  if ((var21 & 2) != 0 && this.a(af, 2)) {
                     av = var1;
                     ac = true;
                  }

                  this.d(var1);
                  this.a(var1, var5);
                  if (this.aD.I) {
                     this.aD.I = false;
                     this.d();
                     this.E.LevelUpUI = this.E.newLevelUpUI(1);
                     this.E.setCurrentDisplay(this.E.LevelUpUI);
                     ak = false;
                  }

                  this.aD.c(false);
                  if (this.aN) {
                     this.p();
                  }

                  this.r = false;
               }

               if (ak) {
                  this.repaint();
                  this.serviceRepaints();
               }

               long var22 = System.currentTimeMillis() - var1;
               ag = var22;

               try {
                  if (var22 < 250L) {
                     long var12 = 250L - var22;
                     Thread.sleep(250L - var22);
                  }
               } catch (Exception var16) {
               }

               if (this.aK) {
                  this.aK = false;
                  return;
               }

               long var3 = var1;
               var1 = System.currentTimeMillis();
               var5 = var1 - var3;
               this.e(var5);
               var7 += var5;
               if (var7 > 1000L) {
                  var7 -= 1000L;
                  this.l();
               }

               if (var1 - av > 3000L) {
                  ac = false;
                  V = 0;
               }
            }
         }
      } catch (OutOfMemoryError var18) {
         System.out.println("Out of memory");
      } catch (Throwable var19) {
         System.out.println("ERROR: An error was thrown in GameCanvas run method!");
         System.out.println(var19);
         var19.printStackTrace();
         aH = true;
         D = String.valueOf(var19);
         this.repaint();
         this.serviceRepaints();

         try {
            Thread.sleep(10000L);
         } catch (Throwable var14) {
         }

         this.E.exit();
      }
   }

   private void d(long var1) {
      if (F) {
         if (x) {
            if (this.a(z, 1)) {
               av = var1;
               ac = true;
            }

            F = false;
         } else {
            this.a(var1);
         }
      } else if (aF) {
         this.c(var1);
      } else if (ar) {
         this.g(var1);
      } else if (S) {
         this.f(var1);
      } else if (aA) {
         this.b(var1);
      } else if (X) {
         this.f();
      } else if (aG) {
         aG = false;
      } else if ((this.n != 0 || this.r) && !this.r) {
         this.n();
      }

      this.a();
      this.m();
   }

   private void b(long var1) {
      if (var1 - this.y >= 500L && i != null) {
         aw = true;
         byte var3 = i.g;
         this.aD.a(i);
         this.y = var1;
         if (var3 > i.g) {
            O = true;
         }
      }

      aA = false;
   }

   private void m() {
      if (i != null && i.g <= 0) {
         if (i.l == 41) {
            this.aD.aj = true;
            this.aD.M = false;
         }

         if (i.l == 42) {
            this.E.endOfGameUI = this.E.newEndOfGameUI();
            this.E.setCurrentDisplay(this.E.endOfGameUI);
         } else {
            i.a(false);
         }

         ESGame.removeMonster(this.aD.ao, i.o, i.m);
         if (this.aD.i(4)) {
            this.aD.E[2] = (short)(this.aD.E[2] + 3 * this.aD.E[3] / 10);
            this.aD.E[2] = (short)Math.min(this.aD.E[2], this.aD.E[3]);
         }

         if (this.a(M, 1)) {
            av = System.currentTimeMillis();
            ac = true;
         }

         i = null;
         Z = false;
         this.aN = true;
      }
   }

   private void a() {
      i = this.aD.n();
      if (i != null) {
         Z = true;
         if (this.a(this.a(i.a()), 1)) {
            av = System.currentTimeMillis();
            ac = true;
         }
      } else {
         Z = false;
      }
   }

   public void h() {
      byte[] var1 = this.aD.g();
      if (var1 != null) {
         j = true;
      } else {
         j = false;
      }

      if (j && this.a(t, 1)) {
         av = System.currentTimeMillis();
         ac = true;
      }
   }

   private void n() {
      if (this.n != 0) {
         byte var1 = this.aD.aq;
         aw = true;
         this.aD.a(this.n, this.aC);
         if (j.R) {
            this.E.endOfGameUI = this.E.newEndOfGameUI();
            this.E.setCurrentDisplay(this.E.endOfGameUI);
            ak = false;
         } else {
            if (this.aD.L) {
               B = true;
               if (B) {
                  if (this.a(this.a(this.aD.a().a()), 1)) {
                     av = System.currentTimeMillis();
                     ac = true;
                  }

                  B = false;
               }
            }

            if (this.aC) {
               this.aC = false;
            }
         }

         this.n = 0;
         int var2 = this.aD.aq - var1;
         if (var2 == 1) {
            if (this.a(this.k(), -1)) {
               av = System.currentTimeMillis();
               ac = true;
            }
         } else if (var2 > 1 && this.a(ao, -1)) {
            av = System.currentTimeMillis();
            ac = true;
         }

         this.h();
         this.c();
         this.aN = true;
      }
   }

   private void g(long var1) {
      if (ar) {
         byte var3 = this.aD.c;
         if (!b.a(var3)) {
            ar = false;
            return;
         }

         if (b.c(var3).e > this.aD.l(4)) {
            if (this.a(q, 3)) {
               av = var1;
               ac = true;
            }
         } else if (var1 - this.T >= 500L) {
            aw = true;
            if (b.b(var3)) {
               if (!Z) {
                  if (this.a(g, 1)) {
                     av = var1;
                     ac = true;
                  }
               } else {
                  this.aD.b(var3, i);
                  ap = true;
               }
            } else {
               this.aD.m(var3);
               am = true;
            }

            this.T = var1;
         }

         ar = false;
      }
   }

   private void f(long var1) {
      if (S) {
         int var3 = this.aD.k();
         if (var3 == 0) {
            if (this.a(N, -1)) {
               av = var1;
               ac = true;
            }
         } else {
            this.aD.c = (byte)var3;
            if (this.a(this.a(b.c(var3).c), -1)) {
               av = var1;
               ac = true;
            }
         }

         S = false;
      }
   }

   private void c(long var1) {
      if (U >= 0) {
         this.d(U);
      } else if (j) {
         byte[] var3 = this.aD.g();
         int var4 = this.aD.a(var3);
         if (var4 == -1) {
            if (this.a(A, 4)) {
               av = var1;
               ac = true;
            }
         } else if (var4 == 0) {
            if (this.a(aJ, -1)) {
               av = var1;
               ac = true;
            }
         } else {
            j = false;
            this.aN = true;
            if (this.a(this.k(), -1)) {
               av = var1;
               ac = true;
            }
         }
      }

      aF = false;
   }

   private String[] k() {
      int var1 = this.aD.aq - 1;
      int var2 = Math.abs(this.aD.af[var1]);
      return this.a(a.c(var2));
   }

   private void f() {
      this.E.setCurrentDisplay(this.E.OptionsUI);
      ak = false;
      X = false;
   }

   private void a(long var1) {
      this.c = 1;
      if (this.aD.b) {
         this.c = 2;
      }

      if (!this.aD.aj && this.aD.E[0] > 3 && f.a(10) == 1) {
         this.c = 3;
      }

      if (this.aD.ao == 1) {
         this.c = 2;
      }

      this.aL = var1;
      F = false;
   }

   void d(int var1) {
      String var2 = k.a(this.aD, var1, 1, 0);
      if (var2 != null) {
         this.E.GenericInfoUI.a(8);
         this.E.GenericInfoUI.a(k.r[var1], var2);
         this.E.GenericInfoUI.i = var1;
         this.E.setAidPointsForNPC(var1);
         this.E.setCurrentDisplay(this.E.GenericInfoUI);
         ak = false;
      } else if (var1 == 4) {
         this.E.setCurrentDisplay(this.E.NPCChoicesUI[4]);
         ak = false;
      }
   }

   private void a(long var1, long var3) {
      int var5 = this.aD.l(2);
      if (var5 <= 0) {
         Z = false;
         this.aM = 2;
         this.p = var1;
      }

      if (!aw) {
         this.aD.a(var3);
      }
   }

   public void d() {
      this.ai = true;
   }

   public void b() {
      this.ai = false;
   }

   protected void showNotify() {
      if (this.Y == null) {
         this.aD.v();
         this.aN = true;
         this.h();
         this.c();
         this.a();
         ak = true;
         this.b();
         if (B) {
            if (this.a(this.a(this.aD.a().a()), 1)) {
               av = System.currentTimeMillis();
               ac = true;
            }

            B = false;
         }
      }
   }

   private boolean o() {
      int var1 = f.a(10);
      return var1 == 1;
   }

   private void e(long var1) {
      if (this.aD.i(4)) {
         this.aD.ar = (short)(this.aD.ar - var1);
         if (this.aD.ar < 0) {
            this.aD.ar = 0;
            byte var3 = 3;
            this.aD.r = (byte)f.c((int)var3, (int)this.aD.r);
         }
      }

      if (this.aD.i(5)) {
         this.aD.O = (short)(this.aD.O - var1);
         if (this.aD.O < 0) {
            this.aD.O = 0;
            byte var4 = 4;
            this.aD.r = (byte)f.c((int)var4, (int)this.aD.r);
         }
      }

      if (this.aD.i(7) && x) {
         this.aD.J = (short)(this.aD.J - var1);
         if (this.aD.J < 0) {
            this.aD.J = 0;
            byte var5 = 6;
            this.aD.r = (byte)f.c((int)var5, (int)this.aD.r);
         }
      }
   }

   private void l() {
      if (this.aD.i(4)) {
         int var1 = 2 * this.aD.E[3] / 100;
         var1 = Math.max(var1, 0);
         this.aD.E[2] = (short)(this.aD.E[2] - var1);
      }

      if (this.aD.i(5)) {
         int var8 = this.aD.E[5] / 10;
         this.aD.E[4] = (short)(this.aD.E[4] + var8);
         if (this.aD.E[4] >= this.aD.E[5]) {
            this.aD.E[4] = 0;
            int var2 = this.aD.E[5] / 10;
            this.aD.E[2] = (short)(this.aD.E[2] - var2);
         }
      }

      for (int var9 = 0; var9 < 25; var9++) {
         if (this.aD.D[var9] > 0) {
            this.aD.D[var9]--;
            if (this.aD.D[var9] <= 0) {
               this.aD.D[var9] = 0;
               if (var9 == 5) {
                  int var10 = this.aD.n(101);
                  if (var10 != -1) {
                     this.aD.w(var10);
                  }
               }
            }
         }
      }

      Hashtable var11 = ESGame.monsters[this.aD.ao - 1];
      if (var11 != null) {
         Enumeration var3 = var11.elements();
         d var4 = new d();

         while (var3.hasMoreElements()) {
            byte[] var5 = (byte[])var3.nextElement();
            d.a(var4, var5);
            if (var4.c[6] != 0) {
               var4.c[7]--;
               if (var4.c[7] < 0) {
                  var4.c[7] = 0;
                  var4.c[6] = 0;
               }
            }
         }
      }

      if (this.aD.Q >= 0) {
         int var12 = ++this.aD.Q;
         byte var13 = -1;
         if (this.aD.ah) {
            switch (var12) {
               case 3:
                  var13 = 4;
                  break;
               case 20:
                  var13 = 16;
                  break;
               case 35:
                  var13 = 7;
                  break;
               case 38:
                  var13 = 18;
                  break;
               case 53:
                  var13 = 12;
                  break;
               case 68:
                  var13 = 20;
                  break;
               case 70:
                  var13 = 22;
                  break;
               case 85:
                  var13 = 24;
                  break;
               case 100:
                  var13 = 26;
                  break;
               case 115:
                  var13 = 28;
                  break;
               case 117:
                  var13 = 30;
                  break;
               case 127:
                  var13 = 31;
                  break;
               case 132:
                  var13 = 32;
            }
         } else {
            switch (var12) {
               case 5:
                  var13 = 4;
                  break;
               case 16:
                  var13 = 16;
                  break;
               case 28:
                  var13 = 8;
                  break;
               case 38:
                  var13 = 20;
                  break;
               case 42:
                  var13 = 21;
                  break;
               case 55:
                  var13 = 22;
                  break;
               case 66:
                  var13 = 23;
                  break;
               case 77:
                  var13 = 24;
                  break;
               case 88:
                  var13 = 25;
                  break;
               case 99:
                  var13 = 26;
                  break;
               case 105:
                  var13 = 27;
                  break;
               case 115:
                  var13 = 28;
                  break;
               case 118:
                  var13 = 29;
                  break;
               case 127:
                  var13 = 30;
                  break;
               case 132:
                  var13 = 31;
            }
         }

         if (var12 == 140) {
            var13 = 42;
         }

         if (var13 > 0) {
            int var14 = 1 + f.a(17);

            for (int var6 = 1 + f.a(17); !this.aD.a().a(var14, var6, var13); var6 = 1 + f.a(17)) {
               var14 = 1 + f.a(17);
            }

            if (ESGame.monsters[this.aD.ao - 1].size() > 5) {
               this.E.endOfGameUI = this.E.newGameOverUI();
               this.E.setCurrentDisplay(this.E.endOfGameUI);
            } else if (this.a(aB, 3)) {
               av = System.currentTimeMillis();
               ac = true;
            }
         }
      }
   }

   public void addCommand(Command var1) {
      if (this.Y != null) {
         this.Y.a(var1);
      }
   }

   public void removeCommand(Command var1) {
      if (this.Y != null) {
         this.Y.b(var1);
      }
   }

   public void setCommandListener(CommandListener var1) {
      if (this.Y != null) {
         this.Y.a(var1);
      }
   }

   boolean e() {
      return this.v;
   }
}
