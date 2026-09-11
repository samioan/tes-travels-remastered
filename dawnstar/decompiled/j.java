import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

public class j {
   static boolean R = false;
   private static boolean aa;
   static Vector al = D();
   private static final Integer H = new Integer(0);
   private static final Integer t = new Integer(1);
   private static final Integer a = new Integer(-1);
   static boolean G = false;
   static short am;
   static short F;
   static String[] i;
   static String[] p;
   static String[] ax;
   static short[][] l;
   static String[] ak;
   static String[] u;
   static short[] P;
   static int[][] q = new int[][]{{1, 27}, {7, 27}, {7, 22}, {17, 27}, {12, 22}, {17, 27}, {12, 22}};
   private static final String[] an = new String[]{
      "Frost Limbs", "Snow Mirage", "Blind", "Troll Thirst", "Glacier Curse", "Grievous Harm", "Terrified", "Winter Worn"
   };
   public static int V = -1;
   ESGame K;
   public static String N = null;
   String Y;
   short U;
   short A;
   short[] E;
   byte X;
   int o;
   short[] n;
   short S;
   short[] y;
   short[][] au;
   byte aq;
   byte[] af;
   int[] ae;
   byte[] ag;
   int s;
   byte c;
   short av;
   short at;
   byte r;
   short ar;
   short O;
   short J;
   boolean Z;
   byte ao;
   byte x;
   byte w;
   byte aw;
   byte as;
   byte g;
   byte e;
   byte m;
   byte[] D;
   short z;
   short v;
   boolean C = false;
   boolean ac = false;
   boolean b = false;
   byte k;
   byte j;
   byte W;
   byte h;
   boolean L;
   boolean ab;
   byte[][] ap;
   byte f;
   byte d;
   boolean[] ad = new boolean[96];
   byte B = 0;
   byte ai = 0;
   boolean aj = false;
   boolean M = false;
   boolean ah = false;
   int Q = -1;
   boolean I = false;
   boolean T = false;

   int g(boolean var1) {
      return var1 ? 400 : 200;
   }

   public j(ESGame var1) {
      R = false;
      s();
      this.Y = null;
      this.E = new short[10];
      this.n = new short[16];
      this.y = new short[2];
      this.au = new short[14][3];
      this.aq = 0;
      this.af = new byte[24];
      this.ae = new int[24];
      this.ag = new byte[7];
      this.D = new byte[25];
      this.ap = new byte[9][5];
      this.K = var1;
      this.ab = false;
   }

   void a(short[] var1) {
      var1[2] = var1[3];
      var1[4] = var1[5];
      var1[6] = var1[7];
      var1[8] = 0;
   }

   void c(int var1) {
      this.U = (short)var1;
      this.A = l[this.U][1];
      byte var2 = 8;

      for (int var3 = 0; var3 < var2; var3++) {
         int var4 = 2 * var3;
         this.n[var4] = l[this.U][2 + var3];
         this.n[var4 + 1] = 0;
      }

      this.S = l[this.U][10];
      this.y[0] = l[this.U][11];
      this.y[1] = l[this.U][12];
      this.E[0] = 1;
      this.E[1] = 0;
      this.f();
      this.E[2] = this.E[3];
      this.E[4] = this.E[5];
      this.E[6] = this.E[7];
      this.E[8] = 0;
      this.E[9] = 0;
      this.X = 0;
      this.o = 50;
      this.ai = (byte)(f.a(4) - 1);
      System.out.println("traitor is " + this.ai);
      int var8 = 13;

      for (int var5 = 0; var5 < 14; var5++) {
         this.au[var5][0] = l[this.U][var8++];
         this.au[var5][1] = l[this.U][var8++];
         this.au[var5][2] = 0;
      }

      for (int var6 = 0; var6 < 24; var6++) {
         this.af[var6] = 0;
         this.ae[var6] = 0;
      }

      for (int var7 = 0; var7 < 7; var7++) {
         this.ag[var7] = 0;
      }

      this.s = this.z();
   }

   void f() {
      this.E[3] = (short)((this.n[0] + this.n[10]) / 2);
      this.E[5] = (short)(this.S * this.n[2] / 4);
      this.E[7] = (short)(this.n[0] + this.n[4] + this.n[6] + this.n[10]);
   }

   private int z() {
      int var1 = 0;
      int var2 = 13;
      byte var3 = -1;
      boolean var4 = true;

      for (int var5 = 0; var5 < 14; var5++) {
         short var6 = l[this.U][var2++];
         short var7 = l[this.U][var2++];
         switch (var5) {
            case 1:
               var3 = 0;
               break;
            case 2:
            case 5:
            case 7:
            case 8:
            case 9:
            default:
               var3 = -1;
               break;
            case 3:
               var3 = 5;
               break;
            case 4:
               var3 = 10;
               break;
            case 6:
               var3 = 15;
               break;
            case 10:
               var3 = 20;
         }

         if (var3 != -1 && var6 > 0) {
            var1 |= 1 << var3;
            if (var4) {
               this.c = (byte)(var3 + 1);
               var4 = false;
            }
         }
      }

      return var1;
   }

   public void f(boolean var1) {
      if (!var1) {
         this.av = 0;
         this.at = 0;
      }

      this.r = 0;
      this.ar = 0;
      this.O = 0;
      this.J = 0;
      this.Z = false;
      this.e(var1);
      if (!var1) {
         this.as = 0;
         this.g = 0;
         this.e = 0;
         this.m = 0;
      }

      for (int var2 = 0; var2 < 25; var2++) {
         this.D[var2] = 0;
      }

      this.z = 0;
      this.v = 0;
      this.C = false;
      this.ac = false;
      this.b = false;
      if (!var1) {
         this.A();
      }
   }

   public void e(boolean var1) {
      if (this.M) {
         Hashtable var2 = ESGame.monsters[this.ao - 1];
         if (var2 != null) {
            d var3 = new d();
            Enumeration var4 = var2.elements();

            while (var4.hasMoreElements()) {
               byte[] var5 = (byte[])var4.nextElement();
               d.a(var3, var5);
               if (var3.l == 41) {
                  this.M = false;
                  ESGame.removeMonster(this.ao, var3.o, var3.m);
                  break;
               }
            }
         }

         if (this.M) {
            System.out.println("Remove roaming gehen failed");
         }
      }

      if (!var1) {
         this.ao = this.h = 1;
         this.x = this.k = 9;
         this.w = this.j = 9;
         this.aw = this.W = 1;
      } else {
         this.ao = this.h = 1;
         this.x = this.k = 13;
         this.w = this.j = 6;
         this.aw = this.W = 4;
      }

      if (this.K.gameCanvas != null) {
         this.v();
         this.K.gameCanvas.h();
         this.K.gameCanvas.c();
      }
   }

   String m() {
      StringBuffer var1 = new StringBuffer(300);
      String var2 = " ";
      String var3 = ": ";
      var1.append(p[this.A]);
      var1.append(var2);
      var1.append(i[this.U]);
      var1.append('\n');
      var1.append(ak[0]);
      var1.append(var3);
      var1.append(this.E[0]);
      var1.append('\n');
      var1.append(ak[2]);
      var1.append(var3);
      var1.append(this.l(2));
      var1.append('\n');
      var1.append(ak[4]);
      var1.append(var3);
      var1.append(this.l(4));
      var1.append('\n');
      var1.append(ak[6]);
      var1.append(var3);
      var1.append(this.l(6));
      var1.append('\n');

      for (int var4 = 0; var4 < 8; var4++) {
         int var5 = 2 * var4;
         var1.append(u[var5]);
         var1.append(var3);
         var1.append(this.n[var5]);
         var1.append('\n');
      }

      for (int var7 = 0; var7 < 14; var7++) {
         if (this.au[var7][0] > 0) {
            var1.append(ax[var7]);
            var1.append(var3);
            var1.append(this.au[var7][0]);
            var1.append('\n');
         }
      }

      return var1.toString();
   }

   static void s() {
      if (!G) {
         try {
            u();
            G = true;
         } catch (Exception var1) {
            System.out.println("Error: could not load character data");
            System.out.println("Exception: " + var1);
         }
      }
   }

   private static void u() throws Exception {
      DataInputStream var0 = ESGame.getResource("charin.dat");
      int var1 = var0.available();
      ak = a(var0);
      u = a(var0);
      i = a(var0);
      am = (short)i.length;
      p = a(var0);
      F = (short)i.length;
      ax = a(var0);
      short var2 = (short)ax.length;
      if (var2 != 14) {
         throw new Exception("Error: mismatch between input number of skill types and that specified in code");
      }

      P = new short[var2];

      for (int var3 = 0; var3 < var2; var3++) {
         P[var3] = var0.readShort();
      }

      int var4 = 13 + 2 * var2;
      l = new short[am][var4];

      for (int var5 = 0; var5 < am; var5++) {
         for (int var6 = 0; var6 < var4; var6++) {
            l[var5][var6] = var0.readShort();
         }
      }

      var0.close();
   }

   private static String[] a(DataInputStream var0) throws Exception {
      short var1 = var0.readShort();
      String[] var2 = new String[var1];

      for (int var3 = 0; var3 < var1; var3++) {
         var2[var3] = var0.readUTF();
      }

      return var2;
   }

   static j a(byte[] var0, boolean var1) throws Exception {
      j var2 = null;
      ByteArrayInputStream var3 = new ByteArrayInputStream(var0, 0, var0.length);
      DataInputStream var4 = new DataInputStream(var3);
      var2 = new j(null);
      var2.Y = var4.readUTF();
      var2.U = var4.readShort();
      if (!var1) {
         var2.c(var2.U);
         var2.f(false);
      }

      var2.A = var4.readShort();

      for (int var5 = 0; var5 < 10; var5++) {
         var2.E[var5] = var4.readShort();
      }

      if (var1) {
         var2.X = var4.readByte();
      }

      var2.o = var4.readInt();

      for (int var6 = 0; var6 < 16; var6++) {
         var2.n[var6] = var4.readShort();
      }

      var2.S = var4.readShort();
      var2.y[0] = var4.readShort();
      var2.y[1] = var4.readShort();

      for (int var7 = 0; var7 < 14; var7++) {
         for (int var8 = 0; var8 < 3; var8++) {
            var2.au[var7][var8] = var4.readShort();
         }
      }

      if (var1) {
         var2.aq = var4.readByte();

         for (int var15 = 0; var15 < 24; var15++) {
            var2.af[var15] = var4.readByte();
         }

         for (int var9 = 0; var9 < 24; var9++) {
            var2.ae[var9] = var4.readInt();
         }

         for (int var10 = 0; var10 < 7; var10++) {
            var2.ag[var10] = var4.readByte();
         }

         var2.s = var4.readInt();
         var2.c = var4.readByte();
      } else {
         var2.s = var4.readInt();
      }

      if (var1) {
         var2.av = var4.readShort();
         var2.at = var4.readShort();
         var2.r = var4.readByte();
         var2.ar = var4.readShort();
         var2.O = var4.readShort();
         var2.J = var4.readShort();
         var2.Z = var4.readBoolean();
         var2.ao = var4.readByte();
         var2.x = var4.readByte();
         var2.w = var4.readByte();
         var2.aw = var4.readByte();
         var2.as = var4.readByte();
         var2.g = var4.readByte();
         var2.e = var4.readByte();
         var2.m = var4.readByte();

         for (int var16 = 0; var16 < 25; var16++) {
            var2.D[var16] = var4.readByte();
         }

         var2.z = var4.readShort();
         var2.v = var4.readShort();
         var2.C = var4.readBoolean();
         var2.ac = var4.readBoolean();
         var2.b = var4.readBoolean();
         byte var17 = 0;
         var17 = var4.readByte();
         var2.M = (var17 & 32) == 32;
         var2.aj = (var17 & 16) == 16;
         var2.B = (byte)(var17 % 4);
         var2.ai = (byte)((var17 >> 2) % 4);
         System.out.println("traitor is " + var2.ai);
         int var20 = 0;

         while (var20 < 96) {
            var17 = var4.readByte();
            var2.ad[var20++] = (var17 & 128) != 0;
            var2.ad[var20++] = (var17 & 64) != 0;
            var2.ad[var20++] = (var17 & 32) != 0;
            var2.ad[var20++] = (var17 & 16) != 0;
            var2.ad[var20++] = (var17 & 8) != 0;
            var2.ad[var20++] = (var17 & 4) != 0;
            var2.ad[var20++] = (var17 & 2) != 0;
            var2.ad[var20++] = (var17 & 1) != 0;
         }
      }

      try {
         var4.close();
      } catch (Exception var11) {
      }

      Object var14 = null;
      Object var13 = null;
      return var2;
   }

   byte[] j(boolean var1) throws Exception {
      int var2 = this.g(var1);
      ByteArrayOutputStream var3 = new ByteArrayOutputStream(var2);
      DataOutputStream var4 = new DataOutputStream(var3);
      var4.writeUTF(this.Y);
      var4.writeShort(this.U);
      var4.writeShort(this.A);
      if (var1) {
         for (int var5 = 0; var5 < 10; var5++) {
            var4.writeShort(this.E[var5]);
         }

         var4.writeByte(this.X);
      } else {
         short[] var13 = new short[10];

         for (int var6 = 0; var6 < 10; var6++) {
            var13[var6] = this.E[var6];
         }

         this.a(var13);

         for (int var7 = 0; var7 < 10; var7++) {
            var4.writeShort(var13[var7]);
         }
      }

      var4.writeInt(this.o);

      for (int var14 = 0; var14 < 16; var14++) {
         var4.writeShort(this.n[var14]);
      }

      var4.writeShort(this.S);
      var4.writeShort(this.y[0]);
      var4.writeShort(this.y[1]);

      for (int var15 = 0; var15 < 14; var15++) {
         for (int var16 = 0; var16 < 3; var16++) {
            var4.writeShort(this.au[var15][var16]);
         }
      }

      if (var1) {
         var4.writeByte(this.aq);

         for (int var17 = 0; var17 < 24; var17++) {
            var4.writeByte(this.af[var17]);
         }

         for (int var8 = 0; var8 < 24; var8++) {
            var4.writeInt(this.ae[var8]);
         }

         for (int var9 = 0; var9 < 7; var9++) {
            var4.writeByte(this.ag[var9]);
         }

         var4.writeInt(this.s);
         var4.writeByte(this.c);
      } else {
         int var18 = this.z();
         var4.writeInt(var18);
      }

      if (var1) {
         var4.writeShort(this.av);
         var4.writeShort(this.at);
         var4.writeByte(this.r);
         var4.writeShort(this.ar);
         var4.writeShort(this.O);
         var4.writeShort(this.J);
         var4.writeBoolean(this.Z);
         var4.writeByte(this.ao);
         var4.writeByte(this.x);
         var4.writeByte(this.w);
         var4.writeByte(this.aw);
         var4.writeByte(this.as);
         var4.writeByte(this.g);
         var4.writeByte(this.e);
         var4.writeByte(this.m);

         for (int var19 = 0; var19 < 25; var19++) {
            var4.writeByte(this.D[var19]);
         }

         var4.writeShort(this.z);
         var4.writeShort(this.v);
         var4.writeBoolean(this.C);
         var4.writeBoolean(this.ac);
         var4.writeBoolean(this.b);
         int var21 = 0;
         var21 = (byte)(this.ai << 2 + this.B);
         if (this.aj) {
            var21 = (byte)(var21 + 16);
         }

         if (this.M) {
            var21 = (byte)(var21 + 32);
         }

         var4.writeByte(var21);
         int var31 = 0;

         while (var31 < 96) {
            var21 = this.ad[var31++] ? -128 : 0;
            byte var24 = (byte)(var21 + (this.ad[var31++] ? 64 : 0));
            var24 = (byte)(var24 + (this.ad[var31++] ? 32 : 0));
            var24 = (byte)(var24 + (this.ad[var31++] ? 16 : 0));
            var24 = (byte)(var24 + (this.ad[var31++] ? 8 : 0));
            var24 = (byte)(var24 + (this.ad[var31++] ? 4 : 0));
            var24 = (byte)(var24 + (this.ad[var31++] ? 2 : 0));
            var24 = (byte)(var24 + (this.ad[var31++] ? 1 : 0));
            var4.writeByte(var24);
         }
      }

      var3.flush();
      byte[] var20 = var3.toByteArray();

      try {
         var4.close();
      } catch (Exception var10) {
      }

      Object var12 = null;
      Object var11 = null;
      return var20;
   }

   void g(int var1) {
      byte var2 = -1;
      switch (var1) {
         case 1:
            var2 = 1;
         case 2:
            this.W = this.aw;
            if (this.aw == 1) {
               this.k = this.x;
               this.j = (byte)(this.w - var2);
            } else if (this.aw == 3) {
               this.k = this.x;
               this.j = (byte)(this.w + var2);
            } else if (this.aw == 2) {
               this.k = (byte)(this.x + var2);
               this.j = this.w;
            } else if (this.aw == 4) {
               this.k = (byte)(this.x - var2);
               this.j = this.w;
            }

            i var3 = ESGame.dungeons[this.ao - 1];
            if (this.k < 0) {
               this.L = true;
               this.h = var3.a[3];
               i var10 = ESGame.dungeons[this.h - 1];
               if (this.h != 1 && this.ao != 1) {
                  this.k = (byte)(var10.h - 1);
               } else {
                  this.k = (byte)(var10.h - 1);
                  this.j = (byte)(this.j + (var10.l - var3.l) / 2);
               }
            } else if (this.k >= var3.h) {
               this.L = true;
               this.h = var3.a[1];
               i var9 = ESGame.dungeons[this.h - 1];
               if (this.h != 1 && this.ao != 1) {
                  this.k = 0;
               } else {
                  this.k = 0;
                  this.j = (byte)(this.j + (var9.l - var3.l) / 2);
               }
            } else if (this.j < 0) {
               this.L = true;
               this.h = var3.a[0];
               i var8 = ESGame.dungeons[this.h - 1];
               if (this.h != 1 && this.ao != 1) {
                  this.j = (byte)(var8.l - 1);
               } else {
                  this.k = (byte)(this.k + (var8.h - var3.h) / 2);
                  this.j = (byte)(var8.l - 1);
               }
            } else if (this.j >= var3.l) {
               this.L = true;
               this.h = ESGame.dungeons[this.ao - 1].a[2];
               i var4 = ESGame.dungeons[this.h - 1];
               if (this.h != 1 && this.ao != 1) {
                  this.j = 0;
               } else {
                  this.k = (byte)(this.k + (var4.h - var3.h) / 2);
                  this.j = 0;
               }
            } else {
               this.L = false;
               this.h = this.ao;
            }

            if (this.L && this.M) {
               Hashtable var11 = ESGame.monsters[this.ao - 1];
               if (var11 != null) {
                  d var5 = new d();
                  Enumeration var6 = var11.elements();

                  while (var6.hasMoreElements()) {
                     byte[] var7 = (byte[])var6.nextElement();
                     d.a(var5, var7);
                     if (var5.l == 41) {
                        this.M = false;
                        ESGame.removeMonster(this.ao, var5.o, var5.m);
                        break;
                     }
                  }
               }

               if (this.M) {
                  System.out.println("Remove roaming gehen failed");
               }
            }
            break;
         case 3:
            this.h = this.ao;
            this.L = false;
            this.W = (byte)(this.aw + 1);
            if (this.W > 4) {
               this.W = 1;
            }

            this.k = this.x;
            this.j = this.w;
            break;
         case 4:
            this.h = this.ao;
            this.L = false;
            this.W = (byte)(this.aw - 1);
            if (this.W < 1) {
               this.W = 4;
            }

            this.k = this.x;
            this.j = this.w;
      }
   }

   boolean a(int var1, boolean var2) {
      i var3 = this.a();
      if (this.E[6] <= 0) {
         return false;
      }

      boolean var4 = false;
      boolean var5 = false;
      if (var2 && var1 == 4) {
         this.o(4);
         var5 = this.o(1);
         if (!this.ab) {
            var4 = this.L;
            var5 = this.o(3);
            this.L = var4;
         }
      } else if (var2 && var1 == 3) {
         this.o(3);
         var5 = this.o(1);
         if (!this.ab) {
            var4 = this.L;
            var5 = this.o(4);
            this.L = var4;
         }
      } else {
         var5 = this.o(var1);
      }

      this.ab = false;
      return var5;
   }

   boolean o(int var1) {
      if (this.E[6] <= 0) {
         return false;
      }

      if (var1 == 0) {
         return false;
      }

      this.g(var1);
      if (this.h <= 0) {
         return false;
      }

      i var2 = ESGame.dungeons[this.h - 1];
      if (!var2.b) {
         return false;
      }

      byte var3 = var2.d[this.k][this.j];
      if (!this.a(var3)) {
         return false;
      }

      this.ao = this.h;
      this.f = this.x;
      this.d = this.w;
      this.x = this.k;
      this.w = this.j;
      this.aw = this.W;
      var2.f = true;
      if (var1 == 1 || var1 == 2) {
         if (k.h) {
            k.h = false;
         }

         this.E[6] = (short)(this.E[6] - 1 * this.b());
         this.E[6] = (short)Math.max(this.E[6], 0);
      }

      boolean var4 = (var3 & 4) != 0;
      if (var4 && (var1 == 1 || var1 == 2)) {
         boolean var5 = false;
         boolean var6 = true;
         Vector var7 = var2.d(this.x, this.w);
         Enumeration var8 = var7.elements();

         while (var8.hasMoreElements()) {
            byte[] var9 = (byte[])var8.nextElement();
            boolean var10 = this.b(var9);
            if (var10) {
               var2.c(var9);
               if ((var9[6] & 2) == 0) {
                  int var11 = var9[2] - 1;
                  if (a.j[var11] == 11) {
                     this.av = (short)(this.av + a.c[var11]);
                  }
               }
            } else {
               var6 = false;
            }
         }

         if (var6) {
            var2.c(this.x, this.w);
         }
      }

      if ((var3 & 8) == 0 || var1 != 1 && var1 != 2) {
         this.v();
      } else {
         this.a(false);
      }

      return true;
   }

   boolean a(byte var1) {
      if ((var1 & 1) != 0) {
         return false;
      } else {
         return (var1 & 32) != 0 ? false : (var1 & 2) == 0;
      }
   }

   void a(d var1) {
      this.z = var1.a;
      byte var2 = var1.l;
      int var3 = this.d(true);
      int var4 = var1.c(7);
      int var5 = var3 - var4;
      var5 = Math.min(var5, var1.c(2));
      if (this.r(10)) {
         if (var1.c[8] == 0) {
            this.v(10);
         } else {
            var5 += var1.c[8];
         }
      }

      int var6 = var1.c(6) - var5 * 5;
      int var7 = this.q() + var5 * 5;
      var6 = Math.min(Math.max(var6, 10), 95);
      var7 = Math.min(Math.max(var7, 10), 95);
      int var8 = d(var7, var6);
      if (var8 != 0) {
         int var9 = this.y();
         int var10 = var1.c(8);
         if (this.r(13)) {
            if (var1.c[5] == 0) {
               this.D[12] = 0;
            } else {
               var10 -= var1.c[5];
            }
         }

         if (var8 == 1) {
            var10 = 2 * var10;
         } else if (var8 == 3) {
            var9 = 2 * var9;
         }

         int var11 = var9 - var10;
         var11 = Math.max(var11, 4);
         int var12 = var11 * var1.c(14) / 100;
         var1.b(var12);
         var1.c();
         if (this.r(7)) {
            if (var1.c[1] == 0) {
               this.v(7);
            } else {
               int var18 = var1.c[1];
               var18 = Math.max(var18, 4);
               var12 = var18 * var1.c(14) / 100;
               var1.b(var12);
            }
         }

         if (var8 >= 2) {
            this.b(this.C(), 1);
         }

         if (!this.r(7)) {
            this.E[6] = (short)(this.E[6] - 7 * this.b());
            this.E[6] = (short)Math.max(this.E[6], 0);
         }

         if (this.i(6)) {
            int var13 = 2 * this.E[3] / 100;
            if (var13 < 1) {
               var13 = 1;
            }

            this.E[2] = (short)(this.E[2] - (short)var13);
         }
      }
   }

   void b(int var1, Object var2) {
      byte[] var3 = null;
      Object var4 = null;
      switch (var1) {
         case 1:
            var3 = (byte[])var2;
            var3[6] = 1;
            d var5 = new d();
            d.a(var5, var3);
            var4 = String.valueOf(var5.a);
            var5.i = true;
            var5.c();
            break;
         case 2:
            var3 = (byte[])var2;
            var3[6] = (byte)(var3[6] | 1);
      }
   }

   void c(boolean var1) {
      this.h();
      Hashtable var2 = ESGame.monsters[this.ao - 1];
      if (var2 != null) {
         Enumeration var3 = var2.elements();

         while (var3.hasMoreElements()) {
            byte[] var4 = (byte[])var3.nextElement();
            if (this.a(1, var4)) {
               this.b(1, var4);
            }
         }
      }

      Hashtable var7 = ESGame.chests[this.ao - 1];
      if (var7 != null) {
         Enumeration var8 = var7.elements();

         while (var8.hasMoreElements()) {
            byte[] var5 = (byte[])var8.nextElement();
            this.a(4, var5);
         }
      }

      Vector var9 = ESGame.droppedItems[this.ao - 1];
      if (var9 != null) {
         Enumeration var10 = var9.elements();

         while (var10.hasMoreElements()) {
            byte[] var6 = (byte[])var10.nextElement();
            if (this.a(2, var6)) {
               this.b(2, var6);
            }
         }
      }

      if (this.ao == 1) {
         for (int var11 = 0; var11 < 5; var11++) {
            this.a(6, k.r[var11]);
         }
      } else if (this.ao == 3) {
         this.a(6, "A");
      } else if (this.ao == 12) {
         this.a(6, "B");
      } else if (this.ao == 21) {
         this.a(6, "C");
      } else if (this.ao == 30) {
         this.a(6, "D");
      }
   }

   byte a(int var1, int var2) {
      return var2 < 4 ? this.ap[var1 + var2 + 1][var2] : this.ap[var1 + var2][var2];
   }

   void h() {
      byte var1 = 0;

      for (int var2 = 0; var2 < 13; var2++) {
         al.setElementAt(H, var2);
      }

      var1 = this.a(-1, 1);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 0);
      }

      var1 = this.a(0, 1);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 1);
      }

      var1 = this.a(1, 1);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 2);
      }

      var1 = this.a(-2, 2);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 3);
      }

      var1 = this.a(-1, 2);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 4);
      }

      var1 = this.a(0, 2);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 5);
      }

      var1 = this.a(1, 2);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 6);
      }

      var1 = this.a(2, 2);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 7);
      }

      var1 = this.a(-2, 3);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 8);
      }

      var1 = this.a(-1, 3);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 9);
      }

      var1 = this.a(0, 3);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 10);
      }

      var1 = this.a(1, 3);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 11);
      }

      var1 = this.a(2, 3);
      if (f.a((byte)1, var1)) {
         al.setElementAt(t, 12);
      }

      if (a(al.elementAt(0))) {
         al.setElementAt(a, 4);
         al.setElementAt(a, 8);
         al.setElementAt(a, 9);
      }

      if (a(al.elementAt(1))) {
         for (int var3 = 0; var3 < 13; var3++) {
            if (var3 != 1) {
               al.setElementAt(a, var3);
            }
         }
      }

      if (a(al.elementAt(2))) {
         al.setElementAt(a, 6);
         al.setElementAt(a, 11);
         al.setElementAt(a, 12);
      }

      if (a(al.elementAt(3))) {
         al.setElementAt(a, 8);
      }

      if (a(al.elementAt(4))) {
         al.setElementAt(a, 8);
         al.setElementAt(a, 9);
      }

      if (a(al.elementAt(5))) {
         al.setElementAt(a, 9);
         al.setElementAt(a, 10);
         al.setElementAt(a, 11);
         al.setElementAt(a, 4);
         al.setElementAt(a, 6);
      }

      if (a(al.elementAt(6))) {
         al.setElementAt(a, 11);
         al.setElementAt(a, 12);
      }

      if (a(al.elementAt(7))) {
         al.setElementAt(a, 12);
      }

      if (a(al.elementAt(9))) {
         al.setElementAt(a, 8);
      }

      if (a(al.elementAt(10))) {
         al.setElementAt(a, 9);
         al.setElementAt(a, 11);
      }

      if (a(al.elementAt(11))) {
         al.setElementAt(a, 12);
      }
   }

   boolean a(int var1, Object var2) {
      i var3 = this.a();
      Object var4 = null;
      byte var5 = 0;
      byte var6 = 0;
      if (var1 == 1) {
         var4 = (byte[])var2;
         var5 = (byte)((Object[])var4)[4];
         var6 = (byte)((Object[])var4)[5];
      } else if (var1 == 4) {
         var4 = (byte[])var2;
         var5 = (byte)((Object[])var4)[0];
         var6 = (byte)((Object[])var4)[1];
      } else if (var1 == 6) {
         if (this.ao == 1) {
            String var7 = (String)var2;

            for (int var8 = 0; var8 < 5; var8++) {
               if (var7.equals(k.r[var8])) {
                  var5 = k.f[var8];
                  var6 = k.e[var8];
                  var8 = 5;
               }
            }
         } else if (this.ao == 3) {
            var5 = k.f[5];
            var6 = k.e[5];
         } else if (this.ao == 12) {
            var5 = k.f[6];
            var6 = k.e[6];
         } else if (this.ao == 21) {
            var5 = k.f[7];
            var6 = k.e[7];
         } else if (this.ao == 30) {
            var5 = k.f[8];
            var6 = k.e[8];
         }
      } else {
         var4 = (byte[])var2;
         var5 = (byte)((Object[])var4)[0];
         var6 = (byte)((Object[])var4)[1];
      }

      int var13 = 0;
      int var14 = 0;
      if (this.aw == 1 || this.aw == 3) {
         byte var15 = -1;
         if (this.aw == 1) {
            var15 = 1;
         }

         var13 = var15 * (var5 - this.x) + 3;
         var14 = var15 * (var6 - this.w) + 3;
      } else if (this.aw == 2 || this.aw == 4) {
         byte var9 = -1;
         if (this.aw == 2) {
            var9 = 1;
         }

         var13 = var9 * (var6 - this.w) + 3;
         var14 = 3 - var9 * (var5 - this.x);
      }

      boolean var16 = false;
      if (var13 == 3 && var14 == 2) {
         if (al.elementAt(1) == H) {
            var16 = true;
            al.setElementAt(var2, 1);
         }
      } else if (var13 == 2 && var14 == 1) {
         if (al.elementAt(4) == H) {
            var16 = true;
            al.setElementAt(var2, 4);
         }
      } else if (var13 == 3 && var14 == 1) {
         if (al.elementAt(5) == H) {
            var16 = true;
            al.setElementAt(var2, 5);
         }
      } else if (var13 == 4 && var14 == 1) {
         if (al.elementAt(6) == H) {
            var16 = true;
            al.setElementAt(var2, 6);
         }
      } else if (var13 == 1 && var14 == 0) {
         if (al.elementAt(8) == H) {
            var16 = true;
            al.setElementAt(var2, 8);
         }
      } else if (var13 == 2 && var14 == 0) {
         if (al.elementAt(9) == H) {
            var16 = true;
            al.setElementAt(var2, 9);
         }
      } else if (var13 == 3 && var14 == 0) {
         if (al.elementAt(10) == H) {
            var16 = true;
            al.setElementAt(var2, 10);
         }
      } else if (var13 == 4 && var14 == 0) {
         if (al.elementAt(11) == H) {
            var16 = true;
            al.setElementAt(var2, 11);
         }
      } else if (var13 == 5 && var14 == 0 && al.elementAt(12) == H) {
         var16 = true;
         al.setElementAt(var2, 12);
      }

      return var16;
   }

   private static boolean a(Object var0) {
      if (var0 instanceof Integer) {
         Integer var1 = (Integer)var0;
         if (var1 == 1) {
            return true;
         }

         if (var1 == -1) {
            return true;
         }
      }

      return false;
   }

   static int d(int var0, int var1) {
      int var2 = f.a(100);
      int var3 = f.a(100);
      boolean var4 = var2 <= var1;
      aa = var3 <= var0;
      byte var5 = 0;
      if (aa && !var4) {
         var5 = 3;
      } else if (aa && var4) {
         if (var3 >= var2) {
            var5 = 2;
         } else {
            var5 = 1;
         }
      } else if (aa || var4) {
         var5 = 0;
      } else if (var3 >= var2) {
         var5 = 2;
      } else {
         var5 = 1;
      }

      return var5;
   }

   int b(int var1, boolean var2) {
      int var3 = this.au[var1][0];
      if (var2) {
         int var4 = 1 + P[var1];
         var3 += this.n[var4] / 3;
      }

      if (var1 == 11 && this.r(3)) {
         var3 += this.au[1][0];
      }

      if (this.E[6] < 7) {
         var3--;
      }

      if (this.T) {
         var3 += 4;
      }

      return var3;
   }

   int x(int var1) {
      return this.au[var1][1];
   }

   int b(boolean var1) {
      if (this.ag[1] != 0) {
         int var2 = a.a(1, this.ag[1]);
         var2 = Math.abs(var2);
         return var2 == 5 ? this.b(5, var1) : this.b(7, var1);
      } else {
         return 0;
      }
   }

   int F() {
      if (this.ag[1] != 0) {
         int var1 = a.a(1, this.ag[1]);
         var1 = Math.abs(var1);
         return var1 == 5 ? this.x(5) : this.x(7);
      } else {
         return 20;
      }
   }

   int r() {
      byte var1 = 0;
      int var2 = this.b(0, false);
      int var3 = this.b(2, false);
      if (var3 > var2) {
         var2 = var3;
         var1 = 2;
      }

      var3 = this.b(8, false);
      if (var3 > var2) {
         var2 = var3;
         var1 = 8;
      }

      var3 = this.b(12, false);
      if (var3 > var2) {
         var1 = 12;
      }

      return var1;
   }

   int C() {
      if (this.r(6)) {
         return this.r();
      }

      if (this.ag[0] != 0) {
         int var1 = a.a(1, this.ag[0]);
         var1 = Math.abs(var1);
         if (var1 == 1) {
            return 0;
         } else if (var1 == 2) {
            return 2;
         } else {
            return var1 == 3 ? 8 : 12;
         }
      } else {
         return -1;
      }
   }

   int d(boolean var1) {
      if (this.r(14)) {
         return 5 + this.b(4, false);
      }

      if (this.r(6)) {
         int var5 = this.r();
         return this.b(var5, var1);
      }

      int var2 = 0;
      if (this.ag[0] != 0) {
         int var3 = a.a(1, this.ag[0]);
         var3 = Math.abs(var3);
         if (var3 == 1) {
            var2 = this.b(0, var1);
         } else if (var3 == 2) {
            var2 = this.b(2, var1);
         } else if (var3 == 3) {
            var2 = this.b(8, var1);
         } else {
            var2 = this.b(12, var1);
         }
      } else {
         var2 = 0;
      }

      if (this.r(5)) {
         var2 += this.b(1, false);
      }

      return var2;
   }

   int q() {
      if (this.r(6) || this.r(14)) {
         int var3 = this.r();
         return this.x(var3);
      }

      if (this.ag[0] != 0) {
         int var1 = a.a(1, this.ag[0]);
         var1 = Math.abs(var1);
         if (var1 == 1) {
            return this.x(0);
         } else if (var1 == 2) {
            return this.x(2);
         } else {
            return var1 == 3 ? this.x(8) : this.x(12);
         }
      } else {
         return 20;
      }
   }

   int y() {
      int var1 = 0;
      if (this.r(14)) {
         var1 = 5 + this.b(4, false);
      } else if (this.r(6)) {
         var1 = 20 + this.b(3, false);
      } else if (this.ag[0] != 0) {
         var1 = a.a(3, this.ag[0]);
      } else {
         var1 = 0;
      }

      if (this.r(1)) {
         var1 += 10 + this.b(1, false);
      }

      if (this.C) {
         var1 += 25;
      }

      return var1;
   }

   int x() {
      if (this.ag[1] != 0) {
         int var1 = a.a(1, this.ag[1]);
         var1 = Math.abs(var1);
         return var1 == 5 ? 5 : 7;
      } else {
         return -1;
      }
   }

   int t() {
      int var1 = 0;
      int var2 = 0;
      if (this.ag[1] != 0) {
         var2 = a.a(3, this.ag[1]);
         var1 += 4 * var2;
      }

      if (this.ag[2] != 0) {
         var2 = a.a(3, this.ag[2]);
         var1 += 2 * var2;
      }

      if (this.ag[3] != 0) {
         var2 = a.a(3, this.ag[3]);
         var1 += 2 * var2;
      }

      if (this.ag[4] != 0) {
         var2 = a.a(3, this.ag[4]);
         var1 += var2;
      }

      if (this.ag[5] != 0) {
         var2 = a.a(3, this.ag[5]);
         var1 += var2;
      }

      var1 /= 10;
      if (this.r(2)) {
         var1 += 10 + this.b(1, false);
      }

      if (this.r(17)) {
         var1 += this.v;
      }

      if (this.ac) {
         var1 += 15;
      }

      return var1;
   }

   int a(byte[] var1) {
      var1[2] = 2;
      if (this.j()) {
         byte var6 = var1[4];
         int var3 = (var1[5] << 8) + var1[6];
         byte var4 = var1[7];
         this.a(var6, var3, var4);
         ESGame.dungeons[this.ao - 1].a(var1);
         int var5 = var6 - 1;
         if (a.j[var5] == 11) {
            this.av = (short)(this.av + a.c[var5]);
         }

         return 1;
      } else {
         byte[] var2 = new byte[]{var1[0], var1[1], var1[4], var1[5], var1[6], var1[7], 1};
         this.a().b(var2);
         ESGame.dungeons[this.ao - 1].a(var1);
         return 0;
      }
   }

   private boolean r(int var1) {
      if (this.D[var1 - 1] == -1) {
         return true;
      } else {
         return this.D[var1 - 1] == -2 ? this.z != 0 : this.D[var1 - 1] > 0;
      }
   }

   void v(int var1) {
      this.D[var1 - 1] = 0;
   }

   d n() {
      this.g(1);
      if (this.h <= 0) {
         return null;
      } else {
         Object var1 = ESGame.monsters[this.h - 1].get(f.b((int)this.k, (int)this.j));
         if (var1 != null) {
            d var2 = new d();
            d.a(var2, (byte[])var1);
            return var2;
         } else {
            return null;
         }
      }
   }

   byte[] g() {
      this.g(1);
      if (this.h <= 0) {
         return null;
      }

      byte var1 = this.h;
      Hashtable var2 = ESGame.chests[var1 - 1];
      if (var2 == null) {
         return null;
      }

      Object var3 = var2.get(f.b((int)this.k, (int)this.j));
      return var3 == null ? null : (byte[])var3;
   }

   int p() {
      this.g(1);
      if (this.h <= 0) {
         return -1;
      } else {
         byte var1 = this.h;
         if (var1 == 3 && this.k == k.f[5] && this.j == k.e[5]) {
            return 5;
         } else if (var1 == 12 && this.k == k.f[6] && this.j == k.e[6]) {
            return 6;
         } else if (var1 == 21 && this.k == k.f[7] && this.j == k.e[7]) {
            return 7;
         } else if (var1 == 30 && this.k == k.f[8] && this.j == k.e[8]) {
            return 8;
         } else {
            return var1 != 1 ? -1 : k.a(this.k, this.j);
         }
      }
   }

   int s(int var1) {
      if (var1 <= 5) {
         return 1;
      } else if (var1 <= 10) {
         return 3;
      } else if (var1 <= 15) {
         return 4;
      } else {
         return var1 <= 20 ? 6 : 10;
      }
   }

   void m(int var1) {
      int var2 = this.s(var1);
      int var3 = this.b(var2, true);
      int var4 = this.x(var2);
      byte var5 = b.c(var1).g;
      byte var6 = b.c(var1).j;
      byte var7 = b.c(var1).e;
      byte var8 = b.c(var1).f;
      int var9 = var3 - var5;
      int var10 = var4 + var9 * 5;
      int var11 = var6 - var9 * 5;
      var10 = Math.min(Math.max(var10, 10), 95);
      var11 = Math.min(Math.max(var11, 10), 95);
      int var12 = d(var10, var11);
      byte var13 = 1;
      if (var12 == 0) {
         this.E[4] = (short)(this.E[4] - 3 * var7);
      } else if (var12 == 1) {
         this.E[4] = (short)(this.E[4] - 3 * var7 / 2);
      } else if (var12 == 2) {
         this.E[4] = (short)(this.E[4] - var7);
      } else if (var12 == 3) {
         this.E[4] = (short)(this.E[4] - var7);
         var13 = 2;
      }

      this.E[4] = (short)Math.max(this.E[4], 0);
      if (var12 >= 2) {
         this.b(var2, 1);
      }

      switch (var1) {
         case 1:
         case 2:
         case 3:
         case 5:
            this.D[var1 - 1] = (byte)(var8 * var13);
         case 4:
         case 7:
         case 8:
         case 9:
         case 10:
         case 11:
         case 12:
         case 13:
         case 14:
         case 15:
         case 16:
         case 17:
         case 18:
         case 19:
         case 20:
         case 22:
         default:
            break;
         case 6:
            byte var14 = 0;
            if (this.a(101, var14, 0) && this.i(true)) {
               this.D[var1 - 1] = (byte)(var8 * var13);
            }
            break;
         case 21:
            int var15 = 6 + this.b(10, false);
            this.E[2] = (short)(this.E[2] + var13 * var15);
            this.E[2] = (short)Math.min(this.E[2], this.E[3]);
            break;
         case 23:
            this.D[var1 - 1] = -2;
            break;
         case 24:
            this.D[var1 - 1] = -4;
            break;
         case 25:
            for (int var16 = 1; var16 <= var13; var16++) {
               this.E();
            }
      }

      this.E[6] = (short)(this.E[6] - 5 * this.b());
      this.E[6] = (short)Math.max(this.E[6], 0);
      if (this.i(6)) {
         int var17 = 2 * this.E[3] / 100;
         if (var17 < 1) {
            var17 = 1;
         }

         this.E[2] = (short)(this.E[2] - (short)var17);
      }
   }

   void b(int var1, d var2) {
      int var3 = this.s(var1);
      int var4 = this.b(var3, true);
      int var5 = this.x(var3);
      int var6 = var2.c(10);
      int var7 = var2.c(9);
      byte var8 = b.c(var1).e;
      byte var9 = b.c(var1).f;
      int var10 = var4 - var6;
      int var11 = var2.c(2);
      var10 = Math.min(var10, var11);
      int var12 = var5 + var10 * 5;
      int var13 = var7 - var10 * 5;
      var12 = Math.min(Math.max(var12, 10), 95);
      var13 = Math.min(Math.max(var13, 10), 95);
      int var14 = d(var12, var13);
      byte var15 = 1;
      if (var14 == 0) {
         this.E[4] = (short)(this.E[4] - 3 * var8);
      } else if (var14 == 1) {
         this.E[4] = (short)(this.E[4] - 3 * var8 / 2);
      } else if (var14 == 2) {
         this.E[4] = (short)(this.E[4] - var8);
      } else if (var14 == 3) {
         this.E[4] = (short)(this.E[4] - var8);
         var15 = 2;
      }

      this.E[4] = (short)Math.max(this.E[4], 0);
      if (var14 >= 2) {
         this.b(var3, 1);
      }

      switch (var1) {
         case 4:
            var2.c[9] = -2;
            var2.c();
         case 5:
         case 6:
         default:
            break;
         case 7:
            int var34 = 10 + this.b(3, false);
            var2.c[1] = (byte)var34;
            break;
         case 8:
            int var33 = this.b(3, false);
            int var39 = 12 + 2 * var33;
            var2.b(var39);
            this.E[6] = (short)(this.E[6] + var33);
            this.E[6] = (short)Math.min(this.E[6], this.E[7]);
            this.E[2] = (short)(this.E[2] + var33);
            this.E[2] = (short)Math.min(this.E[2], this.E[3]);
            this.E[4] = (short)(this.E[4] + 12);
            this.E[4] = (short)Math.min(this.E[4], this.E[5]);
            break;
         case 9:
            if (var2.d()) {
               int var32 = 60 * var15;
               int var37 = var32 - var2.c(8);
               var37 = Math.max(var37, 4);
               int var42 = var37 * var2.c(14) / 100;
               var2.b(var42);
            }
            break;
         case 10:
            this.D[var1 - 1] = -2;
            var2.c[8] = (byte)(2 * var15);
            break;
         case 11:
            int var31 = 25 + this.b(4, false);
            int var36 = var31 * var15;
            int var40 = var36 - var2.c(8);
            var40 = Math.max(var40, 4);
            int var43 = var40 * var2.c(14) / 100;
            var2.b(var43);
            var2.c();
            break;
         case 12:
            this.D[var1 - 1] = -2;
            int var29 = var15 * (10 + this.b(4, false));
            var29 = Math.min(var29, 255);
            var2.c[4] = (byte)var29;
            var2.c();
            break;
         case 13:
            this.D[var1 - 1] = -2;
            int var27 = var15 * (10 + this.b(4, false));
            var27 = Math.min(var27, 255);
            var2.c[5] = (byte)var27;
            var2.c();
            break;
         case 14:
            this.D[var1 - 1] = -1;
            this.a(var2);
            this.D[var1 - 1] = 0;
            break;
         case 15:
            this.D[var1 - 1] = -2;
            var2.c[2] = 1;
            var2.c();
            break;
         case 16:
            int var16 = 10 - var6;
            if (var16 > 0) {
               var16 = var15 * var16;
               this.D[var1 - 1] = (byte)var16;
               var2.c[6] = 1;
            }
            break;
         case 17:
            this.D[var1 - 1] = -2;
            this.v = (short)(10 + this.b(6, false));
            break;
         case 18:
            this.D[var1 - 1] = -2;
            var2.c[0] = (byte)(3 * var15);
            break;
         case 19:
            this.D[var1 - 1] = -2;
            int var17 = var15 * (60 - 5 * var6);
            var17 = Math.min(Math.max(var17, 0), 100);
            var2.c[3] = (byte)var17;
            break;
         case 20:
            int var18 = 80 - 5 * var6;
            int var19 = var18 * var15;
            int var20 = var19 - var2.c(8);
            var20 = Math.max(var20, 4);
            int var21 = var20 * var2.c(14) / 100;
            var2.b(var21);
      }

      this.E[6] = (short)(this.E[6] - 5 * this.b());
      this.E[6] = (short)Math.max(this.E[6], 0);
      if (this.i(6)) {
         int var22 = 2 * this.E[3] / 100;
         if (var22 < 1) {
            var22 = 1;
         }

         this.E[2] = (short)(this.E[2] - (short)var22);
      }
   }

   void b(int var1, int var2) {
      if (var1 >= 0 && var1 < 14) {
         for (this.au[var1][2] = (short)(this.au[var1][2] + var2); this.au[var1][2] > 10; this.E[1]++) {
            this.au[var1][2] = (short)(this.au[var1][2] - 10);
            this.au[var1][0]++;
            short var3 = P[var1];
            int var4 = var3 / 2;
            this.X = (byte)(this.X | 1 << var4);
         }

         if (this.E[1] >= 10) {
            this.E[0]++;
            this.I = true;
         }
      }
   }

   boolean b(byte[] var1) {
      byte var2 = var1[2];
      int var3 = (var1[3] << 8) + var1[4];
      byte var4 = var1[5];
      return this.a(var2, var3, var4);
   }

   void l() {
      this.T = true;
      short var1 = a.a();
      boolean var2 = this.a(100, var1, 0);
      if (!var2) {
         int var3 = 10000;
         int var4 = -1;
         int var5 = -1;

         for (int var7 = 0; var7 < this.aq; var7++) {
            int var6 = Math.abs(this.af[var7]);
            if (var6 == 87) {
               var5 = var7;
               break;
            }

            int var8 = a.a(5, var6);
            if (!this.A(var7) && var8 < var3 && var8 > 0) {
               var4 = var7;
               var3 = var8;
            }
         }

         if (var5 > -1) {
            this.w(var5);
         } else {
            this.w(var4);
         }

         var2 = this.a(100, var1, 0);
         if (!var2) {
            System.out.println("Still can't add StarFrost");
         }
      }
   }

   boolean a(int var1, int var2, int var3) {
      if (this.aq < 24) {
         this.af[this.aq] = (byte)var1;
         int var4 = (var2 << 16) + (byte)var3;
         this.ae[this.aq] = var4;
         this.aq++;
         return true;
      } else {
         return false;
      }
   }

   boolean w(int var1) {
      if (var1 >= this.aq) {
         return false;
      }

      this.y(var1);
      this.af[var1] = 0;

      for (int var2 = var1; var2 < this.aq - 1; var2++) {
         this.af[var2] = this.af[var2 + 1];
         this.ae[var2] = this.ae[var2 + 1];
      }

      this.aq--;
      return true;
   }

   void h(int var1) {
      int var2 = Math.abs(this.af[var1]);
      if (var2 != 101) {
         byte[] var3 = new byte[]{this.x, this.w, (byte)var2, 0, 0, (byte)(this.ae[var1] & 0xFF), 0};
         int var4 = this.ae[var1] >>> 16 & 65535;
         var3[3] = (byte)(var4 >> 8 & 0xFF);
         var3[4] = (byte)(var4 & 0xFF);
         var3[6] = 3;
         this.a().b(var3);
         this.w(var1);
      } else {
         this.w(var1);
      }
   }

   boolean A(int var1) {
      byte var2 = this.af[var1];
      return !a.b(Math.abs(var2)) ? false : var2 < 0;
   }

   boolean c(int var1, boolean var2) {
      byte var3 = this.af[var1];
      if (var3 < 0) {
         return false;
      }

      if (!a.b(var3)) {
         return false;
      }

      int var4 = a.a(var3);
      if (this.ag[var4] != 0) {
         if (!var2) {
            return false;
         }

         this.f(var4);
      }

      this.ag[var4] = var3;
      this.af[var1] = (byte)(-Math.abs(this.af[var1]));
      return true;
   }

   private void f(int var1) {
      for (int var2 = 0; var2 < this.aq; var2++) {
         byte var3 = this.af[var2];
         var3 = (byte)Math.abs(var3);
         int var4 = a.a(var3);
         if (var4 == var1) {
            this.y(var2);
         }
      }
   }

   void d(int var1) {
      this.o += var1;
   }

   boolean i(boolean var1) {
      int var2 = this.aq - 1;
      return this.c(var2, var1);
   }

   void y(int var1) {
      if (this.A(var1)) {
         if (var1 >= 0 && var1 <= 23) {
            byte var2 = this.af[var1];
            var2 = (byte)Math.abs(var2);
            this.af[var1] = var2;

            for (int var3 = 0; var3 < 7; var3++) {
               if (this.ag[var3] == var2) {
                  this.ag[var3] = 0;
                  break;
               }
            }
         }
      }
   }

   int n(int var1) {
      int var2 = -1;
      int var3 = -Math.abs(var1);

      for (int var4 = 0; var4 < this.aq; var4++) {
         if (var3 == this.af[var4]) {
            var2 = var4;
            break;
         }
      }

      return var2;
   }

   boolean j() {
      return this.aq < 24;
   }

   String b(int var1) {
      int var2 = Math.abs(this.af[var1]);
      byte var3 = a.j[var2 - 1];
      String var4 = null;
      switch (var3) {
         case 1:
         case 2:
         case 3:
         case 4:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            int var5 = a.m[var2 - 1] + (this.ae[var1] & 0xFF);
            var4 = var4 + "\nWeapon value: " + var5;
            break;
         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            int var6 = a.m[var2 - 1] + (this.ae[var1] & 0xFF);
            var4 = var4 + "\nArmor value: " + var6;
            break;
         case 11:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            break;
         case 12:
            var4 = a.b[var2 - 1] + '\n' + "Spell: ";
            int var7 = this.ae[var1] & 0xFF;
            var4 = var4 + b.b[var7 - 1].c;
            if ((this.s & 1 << var7 - 1) != 0) {
               var4 = var4 + " (known)";
            }
            break;
         case 13:
            int var8 = var2 - 87;
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1] + '\n' + a.l[var8];
            break;
         case 14:
         default:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            break;
         case 15:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            int var9 = this.b(3, false);
            int var10 = 20 + var9;
            var4 = var4 + "\nWeapon value: " + var10;
      }

      return var4;
   }

   boolean p(int var1) {
      int var2 = Math.abs(this.af[var1]);
      byte var3 = a.j[var2 - 1];
      int var4 = this.ae[var1] & 0xFF;
      int var5 = var4 - 1;
      this.s = f.a(var5, this.s);
      this.w(var1);
      return true;
   }

   void a(int var1) {
      this.a(var1, e.i);
   }

   boolean u(int var1) {
      int var2 = Math.abs(this.af[var1]);
      byte var3 = a.j[var2 - 1];
      switch (var3) {
         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
         case 15:
            return true;
         case 11:
         case 12:
         case 13:
         case 14:
         default:
            return false;
      }
   }

   boolean t(int var1) {
      int var2 = Math.abs(this.af[var1]);
      byte var3 = a.j[var2 - 1];
      switch (var3) {
         case 13:
            return true;
         default:
            return false;
      }
   }

   boolean e(int var1) {
      int var2 = Math.abs(this.af[var1]);
      byte var3 = a.j[var2 - 1];
      switch (var3) {
         case 12:
            int var4 = this.ae[var1] & 0xFF;
            byte var5 = b.b[var4 - 1].h;
            if ((this.s & 1 << var4 - 1) != 0) {
               return false;
            } else {
               if (this.au[var5][0] > 0) {
                  return true;
               }

               return false;
            }
         default:
            return false;
      }
   }

   int l(int var1) {
      int var2 = this.E[var1];
      if (this.r(23)) {
         if (var1 == 2) {
            var2 += this.b(10, false);
            if (var2 > this.E[3]) {
               var2 = this.E[3];
            }
         } else if (var1 == 6) {
            var2 += this.b(10, false);
            if (var2 > this.E[7]) {
               var2 = this.E[7];
            }
         } else if (var1 == 4) {
            var2 += this.b(10, false);
            if (var2 > this.E[5]) {
               var2 = this.E[5];
            }
         }
      }

      return var2;
   }

   boolean w() {
      return this.as > 0;
   }

   void a(boolean var1) {
      if (!var1) {
         this.as = this.ao;
         this.g = this.x;
         this.e = this.w;
         this.m = this.aw;
      }

      this.e(true);
      this.ab = true;
   }

   void d() {
      this.ao = this.h = this.as;
      this.x = this.k = this.g;
      this.w = this.j = this.e;
      this.aw = this.m;
      this.v();
      this.ab = true;
      this.K.gameCanvas.h();
      this.K.gameCanvas.c();
   }

   int B() {
      int var1 = 0;

      for (int var2 = 0; var2 < 8; var2++) {
         int var3 = this.r >> var2 & 1;
         if (var3 != 0) {
            var1++;
         }
      }

      return var1;
   }

   void E() {
      int var1 = this.B();
      if (var1 > 0) {
         int var2 = 0;
         if (var1 == 1) {
            var2 = 1;
         } else {
            var2 = f.a(var1);
         }

         int var3 = 0;

         for (int var4 = 0; var4 < 8; var4++) {
            int var5 = this.r >> var4 & 1;
            if (var5 == 1) {
               if (++var3 == var2) {
                  this.r = (byte)f.c(var4, this.r);
                  break;
               }
            }
         }
      }
   }

   int b() {
      return (this.r & 1) == 1 ? 3 : 1;
   }

   i a() {
      return ESGame.dungeons[this.ao - 1];
   }

   void v() {
      this.a().a(this.x, this.w, this.aw, this.ap);
   }

   String i() {
      StringBuffer var1 = new StringBuffer(900);
      String var2 = " ";
      String var3 = ": ";
      var1.append(this.Y);
      var1.append('\n');
      var1.append(i[this.U]);
      var1.append('\n');
      var1.append("Level ");
      var1.append(this.E[0]);
      var1.append(" (");
      var1.append(this.E[1]);
      var1.append("/10)");
      var1.append('\n');
      var1.append("Health: ");
      var1.append(this.l(2));
      var1.append('/');
      var1.append(this.E[3]);
      var1.append('\n');
      var1.append("Magicka: ");
      var1.append(this.l(4));
      var1.append('/');
      var1.append(this.E[5]);
      var1.append('\n');
      var1.append("Fatigue: ");
      var1.append(this.l(6));
      var1.append('/');
      var1.append(this.E[7]);
      var1.append('\n');
      var1.append("  ");
      var1.append('\n');
      var1.append("Status ailments: ");
      int var4 = 0;

      for (int var5 = 1; var5 <= 8; var5++) {
         if (this.i(var5)) {
            var1.append('\n');
            var1.append(an[var5 - 1]);
            var4++;
         }
      }

      if (var4 == 0) {
         var1.append('\n');
         var1.append("None");
      }

      var1.append('\n');
      var1.append("  ");
      var1.append('\n');
      var1.append("Gift points found: ");
      var1.append(this.av);
      var1.append('\n');
      var1.append("  ");
      var1.append('\n');
      var1.append("Attributes:");
      var1.append('\n');

      for (int var6 = 0; var6 < 8; var6++) {
         int var7 = 2 * var6;
         var1.append(u[var7]);
         var1.append(var3);
         var1.append(this.n[var7]);
         var1.append('\n');
      }

      return var1.toString();
   }

   static Vector D() {
      Vector var0 = new Vector();

      for (int var1 = 0; var1 < 13; var1++) {
         var0.addElement(new Object());
      }

      return var0;
   }

   String[] e() {
      Vector var1 = new Vector();

      for (int var2 = 0; var2 < 14; var2++) {
         if (this.au[var2][0] > 0) {
            String var3 = ax[var2] + ": " + this.au[var2][0];
            var1.addElement(var3);
         }
      }

      int var6 = var1.size();
      String[] var4 = new String[var6];

      for (int var5 = 0; var5 < var6; var5++) {
         var4[var5] = (String)var1.elementAt(var5);
      }

      return var4;
   }

   int j(int var1) {
      int var2 = 0;

      for (int var3 = 0; var3 < 14; var3++) {
         if (this.au[var3][0] > 0) {
            if (var2 == var1) {
               return var3;
            }

            var2++;
         }
      }

      return -1;
   }

   String k(int var1) {
      return ax[var1] + '\n' + "Rank: " + this.au[var1][0] + '\n' + "Exp: " + this.au[var1][2] + "/10";
   }

   Vector G() {
      Vector var1 = new Vector();

      for (int var2 = 0; var2 < b.i; var2++) {
         if ((this.s & 1 << var2) != 0) {
            int var3 = var2 + 1;
            String var4 = b.b[var2].c;
            if (var3 == this.c) {
               var4 = "R: " + var4;
            }

            var1.addElement(var4);
         }
      }

      return var1;
   }

   int z(int var1) {
      int var2 = 0;

      for (int var3 = 0; var3 < b.i; var3++) {
         if ((this.s & 1 << var3) != 0) {
            int var4 = var3 + 1;
            if (var2 == var1) {
               return var3;
            }

            var2++;
         }
      }

      return -1;
   }

   int k() {
      if (!b.a(this.c)) {
         int var3 = this.z(0);
         return var3 < 0 ? 0 : var3 + 1;
      }

      int var1 = this.c - 1;
      int var2 = var1 + 1;
      if (var2 == b.i) {
         var2 = 0;
      }

      while (var2 != var1) {
         if ((this.s & 1 << var2) != 0) {
            return var2 + 1;
         }

         if (++var2 == b.i) {
            var2 = 0;
         }
      }

      return this.c;
   }

   String q(int var1) {
      int var2 = var1;
      String var3 = b.b[var2].c + '\n';
      var3 = var3 + ax[b.b[var2].h] + '\n';
      var3 = var3 + "Cost: " + b.b[var2].e + '\n';
      return var3 + b.b[var2].a;
   }

   void h(boolean var1) {
      if (this.M) {
         Hashtable var2 = ESGame.monsters[this.ao - 1];
         if (var2 != null) {
            d var3 = new d();
            Enumeration var4 = var2.elements();

            while (var4.hasMoreElements()) {
               byte[] var5 = (byte[])var4.nextElement();
               d.a(var3, var5);
               if (var3.l == 41) {
                  this.M = false;
                  ESGame.removeMonster(this.ao, var3.o, var3.m);
                  break;
               }
            }
         }

         if (this.M) {
            System.out.println("Remove roaming gehen failed");
         }
      }

      short var8 = (short)(this.E[3] - this.E[2]);
      short var9 = (short)(this.E[5] - this.E[4]);
      short var10 = (short)(this.E[7] - this.E[6]);
      if (!var1) {
         var8 = (short)(2 * var8 / 3);
         var9 = (short)(2 * var9 / 3);
         var10 = (short)(2 * var10 / 3);
      }

      this.E[9] = 0;
      this.E[8] = 0;
      if (this.i(8)) {
         var8 = (short)(3 * var8 / 4);
         var9 = (short)(3 * var9 / 4);
         var10 = (short)(3 * var10 / 4);
      }

      this.E[2] = (short)(this.E[2] + var8);
      this.E[4] = (short)(this.E[4] + var9);
      this.E[6] = (short)(this.E[6] + var10);
      this.C = false;
      this.ac = false;
      this.b = false;
      int var11 = f.a(100);
      if (var11 <= 10) {
         for (int var6 = 0; var6 < this.aq; var6++) {
            int var7 = Math.abs(this.af[var6]);
            if (var7 == 96) {
               this.w(var6);
               break;
            }
         }
      }

      for (int var13 = 0; var13 < 8; var13++) {
         int var14 = var13 + 1;
         if (var14 != 4 && var14 != 5) {
            var11 = f.a(100);
            if (var11 <= 25) {
               this.r = (byte)f.c(var13, this.r);
            }
         }
      }
   }

   boolean i(int var1) {
      int var2 = var1 - 1;
      return (this.r & 1 << var2) != 0;
   }

   void a(int var1, d var2) {
      int var3 = Math.abs(this.af[var1]);
      byte var4 = a.j[var3 - 1];
      if (var4 == 13) {
         boolean var5 = true;
         switch (var3) {
            case 87:
               if (this.ao == 1 && this.w()) {
                  this.d();
                  break;
               }

               this.a(false);
               break;
            case 88:
               this.E();
               break;
            case 89:
               this.E[2] = this.E[3];
               break;
            case 90:
               this.E[4] = this.E[5];
               break;
            case 91:
               this.E[6] = (short)(this.E[6] + 3 * this.E[5]);
               break;
            case 92:
               this.E[1]++;
               break;
            case 93:
               this.E[2] = this.E[3];
               this.E[4] = this.E[5];
               break;
            case 94:
               this.C = true;
               break;
            case 95:
               this.ac = true;
               break;
            case 96:
               this.b = true;
               var5 = false;
               break;
            case 97:
               if (var2 != null) {
                  int var9 = var2.c(4);
                  int var11 = var2.c(10);
                  if (var9 <= 13 && var11 <= 13) {
                     var2.g = 0;
                     var2.c();
                  }
               }
               break;
            case 98:
               if (var2 != null) {
                  int var8 = var2.c(4);
                  int var10 = var2.c(10);
                  if (var8 <= 22 && var10 <= 22) {
                     var2.g = 0;
                     var2.c();
                  }
               }
               break;
            case 99:
               if (var2 != null) {
                  int var6 = var2.c(4);
                  int var7 = var2.c(10);
                  if (var6 <= 29 && var7 <= 29) {
                     var2.g = 0;
                     var2.c();
                  }
               }
         }

         if (var5) {
            this.w(var1);
         }
      }
   }

   void c() {
      this.X = 0;
      k.b();
      this.E[1] = (short)(this.E[1] - 10);
   }

   String[] o() {
      Vector var1 = new Vector();

      for (int var2 = 0; var2 < 8; var2++) {
         if ((this.X & 1 << var2) != 0) {
            int var3 = var2 * 2;
            var1.addElement(u[var3]);
         }
      }

      int var6 = var1.size();
      if (var6 == 0) {
         return null;
      }

      String[] var4 = new String[var6];

      for (int var5 = 0; var5 < var6; var5++) {
         var4[var5] = (String)var1.elementAt(var5);
      }

      return var4;
   }

   private void A() {
      short var1 = a.a();
      int[] var2 = q[this.U];

      for (int var3 = 0; var3 < var2.length; var3++) {
         this.a(var2[var3], var1, 0);
         int var4 = this.aq - 1;
         this.c(var4, true);
      }
   }

   void a(long var1) {
      int var3 = (int)(var1 * (this.n[10] + this.n[11]) / 2000L);
      this.E[6] = (short)(this.E[6] + var3);
      if (this.E[6] > this.E[7]) {
         this.E[6] = this.E[7];
      }
   }

   int c(int var1, int var2) {
      int var3 = this.b(13, true);
      if (var2 == 3) {
         var3 += 3;
      }

      short var4 = k.q[var1 - 5];
      int var5 = var3 - var4;
      int var6 = 20 - var5 * 5;
      int var7 = 20 + this.n[12] / 2 + var5 * 5;
      var6 = Math.min(Math.max(var6, 10), 95);
      var7 = Math.min(Math.max(var7, 10), 95);
      return d(var7, var6);
   }
}
