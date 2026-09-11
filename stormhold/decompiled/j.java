import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

public class j {
   static boolean g = false;
   private static boolean D;
   static Vector ad = G();
   private static final Integer aj = new Integer(0);
   private static final Integer am = new Integer(1);
   private static final Integer o = new Integer(-1);
   static boolean S = false;
   static short B;
   static short al;
   static String[] K;
   static String[] r;
   static String[] E;
   static short[][] M;
   static String[] h;
   static String[] y;
   static short[] c;
   static int[][] ac = new int[][]{{1, 27}, {7, 27}, {7, 22}, {17, 27}, {12, 22}, {17, 27}, {12, 22}};
   private static final String[] af = new String[]{"Stone Blood", "Delusions", "Blind", "Vampirism", "Mana Burn", "Grievous Harm", "Terrified", "Haunted"};
   public static int ag = -1;
   ESGame ai;
   public static String X = null;
   String v;
   short ar;
   short q;
   short[] U;
   byte N;
   int n;
   short[] J;
   short V;
   short[] aq;
   short[][] R;
   byte p;
   byte[] H;
   int[] P;
   byte[] T;
   int x;
   byte b;
   short W;
   short Y;
   short m;
   byte A;
   short ah;
   short F;
   short ap;
   boolean f;
   byte j;
   byte l;
   byte k;
   byte ak;
   byte C;
   byte ao;
   byte an;
   byte Z;
   byte[] G;
   short t;
   short aa;
   boolean s = false;
   boolean L = false;
   boolean I = false;
   byte z;
   byte w;
   byte e;
   byte ab;
   boolean i;
   boolean Q;
   boolean u;
   boolean O;
   byte[][] ae;
   byte d;
   byte a;

   int b(boolean var1) {
      return var1 ? 400 : 200;
   }

   public j(ESGame var1) {
      g = false;
      u();
      this.v = null;
      this.U = new short[10];
      this.J = new short[16];
      this.aq = new short[2];
      this.R = new short[14][3];
      this.p = 0;
      this.H = new byte[24];
      this.P = new int[24];
      this.T = new byte[7];
      this.G = new byte[25];
      this.ae = new byte[9][5];
      this.ai = var1;
      this.Q = false;
   }

   void a(short[] var1) {
      var1[2] = var1[3];
      var1[4] = var1[5];
      var1[6] = var1[7];
      var1[8] = 0;
   }

   void c(int var1) {
      this.ar = (short)var1;
      this.q = M[this.ar][1];
      byte var2 = 8;

      for (int var3 = 0; var3 < var2; var3++) {
         int var4 = 2 * var3;
         this.J[var4] = M[this.ar][2 + var3];
         this.J[var4 + 1] = 0;
      }

      this.V = M[this.ar][10];
      this.aq[0] = M[this.ar][11];
      this.aq[1] = M[this.ar][12];
      this.U[0] = 1;
      this.U[1] = 0;
      this.g();
      this.U[2] = this.U[3];
      this.U[4] = this.U[5];
      this.U[6] = this.U[7];
      this.U[8] = 0;
      this.U[9] = 0;
      this.N = 0;
      this.n = 0;
      int var8 = 13;

      for (int var5 = 0; var5 < 14; var5++) {
         this.R[var5][0] = M[this.ar][var8++];
         this.R[var5][1] = M[this.ar][var8++];
         this.R[var5][2] = 0;
      }

      for (int var6 = 0; var6 < 24; var6++) {
         this.H[var6] = 0;
         this.P[var6] = 0;
      }

      for (int var7 = 0; var7 < 7; var7++) {
         this.T[var7] = 0;
      }

      this.x = this.A();
   }

   void g() {
      this.U[3] = (short)((this.J[0] + this.J[10]) / 2);
      this.U[5] = (short)(this.V * this.J[2] / 4);
      this.U[7] = (short)(this.J[0] + this.J[4] + this.J[6] + this.J[10]);
   }

   private int A() {
      int var1 = 0;
      int var2 = 13;
      byte var3 = -1;
      boolean var4 = true;

      for (int var5 = 0; var5 < 14; var5++) {
         short var6 = M[this.ar][var2++];
         short var7 = M[this.ar][var2++];
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
               this.b = (byte)(var3 + 1);
               var4 = false;
            }
         }
      }

      return var1;
   }

   public void d(int var1) {
      this.c(var1, false);
   }

   public void c(int var1, boolean var2) {
      if (!var2) {
         this.W = 0;
         this.Y = 0;
         this.m = 0;
      }

      this.A = 0;
      this.ah = 0;
      this.F = 0;
      this.ap = 0;
      this.f = false;
      this.c(var2);
      this.w();
      if (!var2) {
         this.C = 0;
         this.ao = 0;
         this.an = 0;
         this.Z = 0;
      }

      for (int var3 = 0; var3 < 25; var3++) {
         this.G[var3] = 0;
      }

      this.t = 0;
      this.aa = 0;
      this.s = false;
      this.L = false;
      this.I = false;
      if (!var2) {
         this.C();
      }
   }

   private void c(boolean var1) {
      if (!var1) {
         this.j = this.ab = 1;
         this.l = this.z = 9;
         this.k = this.w = 10;
         this.ak = this.e = 1;
      } else {
         this.j = this.ab = 1;
         this.l = this.z = 12;
         this.k = this.w = 14;
         this.ak = this.e = 1;
      }
   }

   String m() {
      StringBuffer var1 = new StringBuffer(300);
      String var2 = " ";
      String var3 = ": ";
      var1.append(r[this.q]);
      var1.append(var2);
      var1.append(K[this.ar]);
      var1.append('\n');
      var1.append(h[0]);
      var1.append(var3);
      var1.append(this.U[0]);
      var1.append('\n');
      var1.append(h[2]);
      var1.append(var3);
      var1.append(this.n(2));
      var1.append('\n');
      var1.append(h[4]);
      var1.append(var3);
      var1.append(this.n(4));
      var1.append('\n');
      var1.append(h[6]);
      var1.append(var3);
      var1.append(this.n(6));
      var1.append('\n');

      for (int var4 = 0; var4 < 8; var4++) {
         int var5 = 2 * var4;
         var1.append(y[var5]);
         var1.append(var3);
         var1.append(this.J[var5]);
         var1.append('\n');
      }

      for (int var7 = 0; var7 < 14; var7++) {
         if (this.R[var7][0] > 0) {
            var1.append(E[var7]);
            var1.append(var3);
            var1.append(this.R[var7][0]);
            var1.append('\n');
         }
      }

      return var1.toString();
   }

   static void u() {
      if (!S) {
         try {
            a("/charin.dat");
            S = true;
         } catch (Exception var1) {
            System.out.println("Error: could not load character data");
            System.out.println("Exception: " + var1);
         }
      }
   }

   private static void a(String var0) throws Exception {
      DataInputStream var1 = ESGame.a(var0);
      int var2 = var1.available();
      h = a(var1);
      y = a(var1);
      K = a(var1);
      B = (short)K.length;
      r = a(var1);
      al = (short)K.length;
      E = a(var1);
      short var3 = (short)E.length;
      if (var3 != 14) {
         throw new Exception("Error: mismatch between input number of skill types and that specified in code");
      }

      c = new short[var3];

      for (int var4 = 0; var4 < var3; var4++) {
         c[var4] = var1.readShort();
      }

      int var5 = 13 + 2 * var3;
      M = new short[B][var5];

      for (int var6 = 0; var6 < B; var6++) {
         for (int var7 = 0; var7 < var5; var7++) {
            M[var6][var7] = var1.readShort();
         }
      }
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
      DataInputStream var3 = new DataInputStream(new ByteArrayInputStream(var0, 0, var0.length));
      var2 = new j(null);
      var2.v = var3.readUTF();
      var2.ar = var3.readShort();
      if (!var1) {
         var2.c(var2.ar);
         var2.d(var2.ar);
      }

      var2.q = var3.readShort();

      for (int var4 = 0; var4 < 10; var4++) {
         var2.U[var4] = var3.readShort();
      }

      if (var1) {
         var2.N = var3.readByte();
      }

      var2.n = var3.readInt();

      for (int var5 = 0; var5 < 16; var5++) {
         var2.J[var5] = var3.readShort();
      }

      var2.V = var3.readShort();
      var2.aq[0] = var3.readShort();
      var2.aq[1] = var3.readShort();

      for (int var6 = 0; var6 < 14; var6++) {
         for (int var7 = 0; var7 < 3; var7++) {
            var2.R[var6][var7] = var3.readShort();
         }
      }

      if (var1) {
         var2.p = var3.readByte();

         for (int var11 = 0; var11 < 24; var11++) {
            var2.H[var11] = var3.readByte();
         }

         for (int var8 = 0; var8 < 24; var8++) {
            var2.P[var8] = var3.readInt();
         }

         for (int var9 = 0; var9 < 7; var9++) {
            var2.T[var9] = var3.readByte();
         }

         var2.x = var3.readInt();
         var2.b = var3.readByte();
      } else {
         var2.x = var3.readInt();
      }

      if (var1) {
         var2.W = var3.readShort();
         var2.Y = var3.readShort();
         var2.m = var3.readShort();
         var2.A = var3.readByte();
         var2.ah = var3.readShort();
         var2.F = var3.readShort();
         var2.ap = var3.readShort();
         var2.f = var3.readBoolean();
         var2.j = var3.readByte();
         var2.l = var3.readByte();
         var2.k = var3.readByte();
         var2.ak = var3.readByte();
         var2.C = var3.readByte();
         var2.ao = var3.readByte();
         var2.an = var3.readByte();
         var2.Z = var3.readByte();

         for (int var12 = 0; var12 < 25; var12++) {
            var2.G[var12] = var3.readByte();
         }

         var2.t = var3.readShort();
         var2.aa = var3.readShort();
         var2.s = var3.readBoolean();
         var2.L = var3.readBoolean();
         var2.I = var3.readBoolean();
      }

      return var2;
   }

   byte[] g(boolean var1) throws Exception {
      int var2 = this.b(var1);
      ByteArrayOutputStream var3 = new ByteArrayOutputStream(var2);
      DataOutputStream var4 = new DataOutputStream(var3);
      var4.writeUTF(this.v);
      var4.writeShort(this.ar);
      var4.writeShort(this.q);
      if (var1) {
         for (int var5 = 0; var5 < 10; var5++) {
            var4.writeShort(this.U[var5]);
         }

         var4.writeByte(this.N);
      } else {
         short[] var10 = new short[10];

         for (int var6 = 0; var6 < 10; var6++) {
            var10[var6] = this.U[var6];
         }

         this.a(var10);

         for (int var7 = 0; var7 < 10; var7++) {
            var4.writeShort(var10[var7]);
         }
      }

      var4.writeInt(this.n);

      for (int var11 = 0; var11 < 16; var11++) {
         var4.writeShort(this.J[var11]);
      }

      var4.writeShort(this.V);
      var4.writeShort(this.aq[0]);
      var4.writeShort(this.aq[1]);

      for (int var12 = 0; var12 < 14; var12++) {
         for (int var13 = 0; var13 < 3; var13++) {
            var4.writeShort(this.R[var12][var13]);
         }
      }

      if (var1) {
         var4.writeByte(this.p);

         for (int var14 = 0; var14 < 24; var14++) {
            var4.writeByte(this.H[var14]);
         }

         for (int var8 = 0; var8 < 24; var8++) {
            var4.writeInt(this.P[var8]);
         }

         for (int var9 = 0; var9 < 7; var9++) {
            var4.writeByte(this.T[var9]);
         }

         var4.writeInt(this.x);
         var4.writeByte(this.b);
      } else {
         int var15 = this.A();
         var4.writeInt(var15);
      }

      if (var1) {
         var4.writeShort(this.W);
         var4.writeShort(this.Y);
         var4.writeShort(this.m);
         var4.writeByte(this.A);
         var4.writeShort(this.ah);
         var4.writeShort(this.F);
         var4.writeShort(this.ap);
         var4.writeBoolean(this.f);
         var4.writeByte(this.j);
         var4.writeByte(this.l);
         var4.writeByte(this.k);
         var4.writeByte(this.ak);
         var4.writeByte(this.C);
         var4.writeByte(this.ao);
         var4.writeByte(this.an);
         var4.writeByte(this.Z);

         for (int var16 = 0; var16 < 25; var16++) {
            var4.writeByte(this.G[var16]);
         }

         var4.writeShort(this.t);
         var4.writeShort(this.aa);
         var4.writeBoolean(this.s);
         var4.writeBoolean(this.L);
         var4.writeBoolean(this.I);
      }

      return var3.toByteArray();
   }

   void g(int var1) {
      byte var2 = -1;
      switch (var1) {
         case 1:
            var2 = 1;
         case 2:
            this.e = this.ak;
            if (this.ak == 1) {
               this.z = this.l;
               this.w = (byte)(this.k - var2);
            } else if (this.ak == 3) {
               this.z = this.l;
               this.w = (byte)(this.k + var2);
            } else if (this.ak == 2) {
               this.z = (byte)(this.l + var2);
               this.w = this.k;
            } else if (this.ak == 4) {
               this.z = (byte)(this.l - var2);
               this.w = this.k;
            }

            i var3 = ESGame.u[this.j - 1];
            if (this.z < 0) {
               this.i = true;
               this.ab = var3.x[3];
               i var4 = ESGame.u[this.ab - 1];
               if (this.ab != 1 && this.j != 1) {
                  this.z = (byte)(var4.g - 1);
               } else {
                  this.z = (byte)(var4.g - 1);
                  this.w = (byte)(this.w + (var4.y - var3.y) / 2);
               }
            } else if (this.z >= var3.g) {
               this.i = true;
               this.ab = var3.x[1];
               i var5 = ESGame.u[this.ab - 1];
               if (this.ab != 1 && this.j != 1) {
                  this.z = 0;
               } else {
                  this.z = 0;
                  this.w = (byte)(this.w + (var5.y - var3.y) / 2);
               }
            } else if (this.w < 0) {
               this.i = true;
               this.ab = var3.x[0];
               i var6 = ESGame.u[this.ab - 1];
               if (this.ab != 1 && this.j != 1) {
                  this.w = (byte)(var6.y - 1);
               } else {
                  this.z = (byte)(this.z + (var6.g - var3.g) / 2);
                  this.w = (byte)(var6.y - 1);
               }
            } else if (this.w >= var3.y) {
               this.i = true;
               this.ab = ESGame.u[this.j - 1].x[2];
               i var7 = ESGame.u[this.ab - 1];
               if (this.ab != 1 && this.j != 1) {
                  this.w = 0;
               } else {
                  this.z = (byte)(this.z + (var7.g - var3.g) / 2);
                  this.w = 0;
               }
            } else {
               this.i = false;
               this.ab = this.j;
            }
            break;
         case 3:
            this.ab = this.j;
            this.i = false;
            this.e = (byte)(this.ak + 1);
            if (this.e > 4) {
               this.e = 1;
            }

            this.z = this.l;
            this.w = this.k;
            break;
         case 4:
            this.ab = this.j;
            this.i = false;
            this.e = (byte)(this.ak - 1);
            if (this.e < 1) {
               this.e = 4;
            }

            this.z = this.l;
            this.w = this.k;
      }
   }

   boolean a(int var1, boolean var2) {
      i var3 = this.b();
      var3.a(this.l, this.k);
      if (this.U[6] <= 0) {
         return false;
      } else {
         boolean var4 = false;
         if (var2 && var1 == 4) {
            this.q(4);
            boolean var9 = this.q(1);
            var4 = this.i;
            var9 = this.q(3);
            this.i = var4;
            return var9;
         } else if (var2 && var1 == 3) {
            this.q(3);
            boolean var5 = this.q(1);
            var4 = this.i;
            var5 = this.q(4);
            this.i = var4;
            return var5;
         } else {
            return this.q(var1);
         }
      }
   }

   boolean q(int var1) {
      if (this.U[6] <= 0) {
         return false;
      }

      if (var1 == 0) {
         return false;
      }

      this.g(var1);
      boolean var2 = false;
      if (var1 == 1 || var1 == 2) {
         var2 = true;
      }

      if (this.ab <= 0) {
         return false;
      }

      i var3 = ESGame.u[this.ab - 1];
      if (!var3.k) {
         return false;
      }

      byte var4 = var3.w[this.z][this.w];
      if (!this.a(var4)) {
         return false;
      }

      if (this.ab == 37 && this.j != 37) {
         Enumeration var5 = ESGame.G[this.ab - 1].elements();

         while (var5.hasMoreElements()) {
            byte[] var6 = (byte[])var5.nextElement();
            d var7 = d.a(var6);
            if (var7.l == 41) {
               var7.g = (byte)var7.c(14);
               var7.d();
            }
         }

         Object var12 = null;
      }

      if (!this.a(this.j, this.l, this.k) && this.a(this.ab, this.z, this.w)) {
         this.u = true;
      } else {
         this.u = false;
      }

      if (this.a(this.j, this.l, this.k) && !this.a(this.ab, this.z, this.w)) {
         this.O = true;
      } else {
         this.O = false;
      }

      this.j = this.ab;
      this.d = this.l;
      this.a = this.k;
      this.l = this.z;
      this.k = this.w;
      this.ak = this.e;
      var3.h = true;
      if (var1 == 1 || var1 == 2) {
         if (k.l) {
            k.l = false;
         }

         this.U[6] = (short)(this.U[6] - 1 * this.c());
         this.U[6] = (short)Math.max(this.U[6], 0);
      }

      boolean var13 = (var4 & 4) != 0;
      if (var13) {
         int var14 = var3.f(this.l, this.k);
         if (var14 == 1) {
            byte[] var15 = var3.e(this.l, this.k);
            if ((var15[6] & 4) != 0) {
               g = true;
               this.w();
               return true;
            }

            System.out.println(">>>>>>Found a dropped item item: " + d(var15));
            boolean var8 = this.c(var15);
            if (var8) {
               var3.d(var15);
               if ((var15[6] & 2) == 0) {
                  System.out.println("Dropped item not possessed before");
                  int var9 = var15[2] - 1;
                  System.out.println("item index=" + var9);
                  if (a.j[var9] == 11) {
                     this.W = (short)(this.W + a.c[var9]);
                     int var10 = ESGame.d(this.W);
                     this.ai.n(var10);
                  }
               }
            }
         } else if (var14 > 1) {
            System.out.println("Found several items in square");
            Vector var16 = var3.b(this.l, this.k);
            Enumeration var17 = var16.elements();

            while (var17.hasMoreElements()) {
               byte[] var18 = (byte[])var17.nextElement();
               if ((var18[6] & 4) != 0) {
                  g = true;
                  this.w();
                  return true;
               }

               boolean var19 = this.c(var18);
               if (var19) {
                  var3.d(var18);
                  if ((var18[6] & 2) != 0) {
                     int var11 = var18[2] - 1;
                     System.out.println("item index=" + var11);
                     if (a.j[var11] == 11) {
                        this.W = (short)(this.W + a.c[var11]);
                        this.ai.n(ESGame.d(this.W));
                     }
                  }
               }
            }
         }
      }

      this.w();
      if (var2 && (var4 & 8) != 0) {
         this.p();
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
      this.t = var1.a;
      byte var2 = var1.l;
      int var3 = this.h(true);
      int var4 = var1.c(7);
      int var5 = var3 - var4;
      var5 = Math.min(var5, var1.c(2));
      if (this.t(10)) {
         if (var1.c[8] == 0) {
            this.x(10);
         } else {
            var5 += var1.c[8];
         }
      }

      int var6 = var1.c(6) - var5 * 5;
      int var7 = this.s() + var5 * 5;
      var6 = Math.min(Math.max(var6, 10), 95);
      var7 = Math.min(Math.max(var7, 10), 95);
      int var8 = d(var7, var6);
      if (var8 != 0) {
         int var9 = this.z();
         int var10 = var1.c(8);
         if (this.t(13)) {
            if (var1.c[5] == 0) {
               this.G[12] = 0;
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
         var1.d();
         if (this.t(7)) {
            if (var1.c[1] == 0) {
               this.x(7);
            } else {
               int var18 = var1.c[1];
               var18 = Math.max(var18, 4);
               var12 = var18 * var1.c(14) / 100;
               var1.b(var12);
            }
         }

         if (var8 >= 2) {
            this.a(this.F(), 1);
         }

         if (!this.t(7)) {
            this.U[6] = (short)(this.U[6] - 7 * this.c());
            this.U[6] = (short)Math.max(this.U[6], 0);
         }

         if (this.k(6)) {
            int var13 = 2 * this.U[3] / 100;
            if (var13 < 1) {
               var13 = 1;
            }

            this.U[2] = (short)(this.U[2] - (short)var13);
         }
      }
   }

   int b(Object var1) {
      byte var2 = 0;
      byte var3 = 0;
      byte[] var4 = null;
      int var5 = Integer.MAX_VALUE;
      var4 = (byte[])var1;
      if (var4.length == 28) {
         d var6 = d.a(var4);
         var2 = var6.o;
         var3 = var6.m;
      } else {
         var2 = var4[0];
         var3 = var4[1];
      }

      if (this.ak == 1) {
         var5 = this.k - var3;
      } else if (this.ak == 3) {
         var5 = var3 - this.k;
      } else if (this.ak == 2) {
         var5 = var2 - this.l;
      } else {
         var5 = this.l - var2;
      }

      if (var5 < 0) {
         var5 = Integer.MAX_VALUE;
      }

      return var5;
   }

   void b(int var1, Object var2) {
      byte[] var3 = null;
      Object var4 = null;
      switch (var1) {
         case 1:
            var3 = (byte[])var2;
            var3[6] = 1;
            d var5 = d.a(var3);
            var4 = String.valueOf(var5.a);
            var5.i = true;
            var5.d();
            break;
         case 2:
            var3 = (byte[])var2;
            var3[6] = (byte)(var3[6] | 1);
      }
   }

   void d(boolean var1) {
      this.i();
      Vector var2 = ESGame.au[this.j - 1];
      if (var2 != null) {
         Enumeration var3 = var2.elements();

         while (var3.hasMoreElements()) {
            byte[] var4 = (byte[])var3.nextElement();
            this.a(2, var4, var1);
         }
      }

      Hashtable var7 = ESGame.S[this.j - 1];
      if (var7 != null) {
         Enumeration var8 = var7.elements();

         while (var8.hasMoreElements()) {
            byte[] var5 = (byte[])var8.nextElement();
            this.a(4, var5, var1);
         }
      }

      Hashtable var9 = ESGame.G[this.j - 1];
      if (var9 != null) {
         Enumeration var10 = var9.elements();

         while (var10.hasMoreElements()) {
            byte[] var6 = (byte[])var10.nextElement();
            this.a(1, var6, var1);
         }
      }

      if (this.j == 1 && k.d) {
         this.a(5, "W");
      }
   }

   boolean a(int var1, Object var2, boolean var3) {
      Object var4 = null;
      Object var5 = null;
      boolean var6 = false;
      boolean var7 = false;
      boolean var8 = false;
      int var9 = this.b(var2);
      boolean var10 = this.a(var1, var2);
      if (!var10) {
         return false;
      } else if (var1 != 4 && var9 != 1) {
         this.b(var1, var2);
         return true;
      } else {
         this.b(var1, var2);
         return true;
      }
   }

   void i() {
      i var1 = this.b();
      byte var2 = 0;

      for (int var3 = 0; var3 < 13; var3++) {
         ad.setElementAt(aj, var3);
      }

      var2 = var1.a(-1, 1, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 0);
      }

      var2 = var1.a(0, 1, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 1);
      }

      var2 = var1.a(1, 1, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 2);
      }

      var2 = var1.a(-2, 2, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 3);
      }

      var2 = var1.a(-1, 2, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 4);
      }

      var2 = var1.a(0, 2, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 5);
      }

      var2 = var1.a(1, 2, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 6);
      }

      var2 = var1.a(2, 2, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 7);
      }

      var2 = var1.a(-2, 3, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 8);
      }

      var2 = var1.a(-1, 3, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 9);
      }

      var2 = var1.a(0, 3, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 10);
      }

      var2 = var1.a(1, 3, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 11);
      }

      var2 = var1.a(2, 3, this.ae);
      if (f.a((byte)1, var2)) {
         ad.setElementAt(am, 12);
      }

      if (a(ad.elementAt(0))) {
         ad.setElementAt(o, 4);
         ad.setElementAt(o, 8);
         ad.setElementAt(o, 9);
      }

      if (a(ad.elementAt(1))) {
         for (int var4 = 0; var4 < 13; var4++) {
            if (var4 != 1) {
               ad.setElementAt(o, var4);
            }
         }
      }

      if (a(ad.elementAt(2))) {
         ad.setElementAt(o, 6);
         ad.setElementAt(o, 11);
         ad.setElementAt(o, 12);
      }

      if (a(ad.elementAt(3))) {
         ad.setElementAt(o, 8);
      }

      if (a(ad.elementAt(4))) {
         ad.setElementAt(o, 8);
         ad.setElementAt(o, 9);
      }

      if (a(ad.elementAt(5))) {
         ad.setElementAt(o, 9);
         ad.setElementAt(o, 10);
         ad.setElementAt(o, 11);
         ad.setElementAt(o, 4);
         ad.setElementAt(o, 6);
      }

      if (a(ad.elementAt(6))) {
         ad.setElementAt(o, 11);
         ad.setElementAt(o, 12);
      }

      if (a(ad.elementAt(7))) {
         ad.setElementAt(o, 12);
      }

      if (a(ad.elementAt(9))) {
         ad.setElementAt(o, 8);
      }

      if (a(ad.elementAt(10))) {
         ad.setElementAt(o, 9);
         ad.setElementAt(o, 11);
      }

      if (a(ad.elementAt(11))) {
         ad.setElementAt(o, 12);
      }
   }

   boolean a(int var1, Object var2) {
      i var3 = this.b();
      Object var4 = null;
      Object var5 = null;
      Object var6 = null;
      Object var7 = null;
      byte var8 = 0;
      byte var9 = 0;
      if (var1 == 1) {
         var5 = (byte[])var2;
         var8 = (byte)((Object[])var5)[4];
         var9 = (byte)((Object[])var5)[5];
      } else if (var1 == 4) {
         var7 = (byte[])var2;
         var8 = (byte)((Object[])var7)[0];
         var9 = (byte)((Object[])var7)[1];
      } else if (var1 == 5) {
         var8 = k.j[6];
         var9 = k.i[6];
      } else {
         var6 = (byte[])var2;
         var8 = (byte)((Object[])var6)[0];
         var9 = (byte)((Object[])var6)[1];
      }

      int[] var10 = var3.a(this.l, this.k, this.ak, var8, var9);
      boolean var11 = false;
      if (a(var10[0], var10[1], 3, 2)) {
         var11 = true;
         ad.setElementAt(var2, 1);
      } else if (a(var10[0], var10[1], 2, 1)) {
         if (!a(ad.elementAt(0)) && !a(ad.elementAt(1)) && !a(ad.elementAt(5))) {
            var11 = true;
            ad.setElementAt(var2, 4);
         }
      } else if (a(var10[0], var10[1], 3, 1)) {
         if (!a(ad.elementAt(1))) {
            var11 = true;
            ad.setElementAt(var2, 5);
         }
      } else if (a(var10[0], var10[1], 4, 1)) {
         if (!a(ad.elementAt(1)) && !a(ad.elementAt(2)) && !a(ad.elementAt(5))) {
            var11 = true;
            ad.setElementAt(var2, 6);
         }
      } else if (a(var10[0], var10[1], 1, 0)) {
         if (!a(ad.elementAt(0)) && !a(ad.elementAt(1)) && !a(ad.elementAt(3)) && !a(ad.elementAt(4)) && !a(ad.elementAt(9))) {
            var11 = true;
            ad.setElementAt(var2, 8);
         }
      } else if (a(var10[0], var10[1], 2, 0)) {
         if (!a(ad.elementAt(0)) && !a(ad.elementAt(1)) && !a(ad.elementAt(4)) && !a(ad.elementAt(5)) && !a(ad.elementAt(10))) {
            var11 = true;
            ad.setElementAt(var2, 9);
         }
      } else if (a(var10[0], var10[1], 3, 0)) {
         if (!a(ad.elementAt(1)) && !a(ad.elementAt(5))) {
            var11 = true;
            ad.setElementAt(var2, 10);
         }
      } else if (a(var10[0], var10[1], 4, 0)) {
         if (!a(ad.elementAt(1)) && !a(ad.elementAt(2)) && !a(ad.elementAt(5)) && !a(ad.elementAt(6)) && !a(ad.elementAt(10))) {
            var11 = true;
            ad.setElementAt(var2, 11);
         }
      } else if (a(var10[0], var10[1], 5, 0)
         && !a(ad.elementAt(1))
         && !a(ad.elementAt(2))
         && !a(ad.elementAt(6))
         && !a(ad.elementAt(7))
         && !a(ad.elementAt(11))) {
         var11 = true;
         ad.setElementAt(var2, 12);
      }

      return var11;
   }

   private static boolean a(int var0, int var1, int var2, int var3) {
      return var0 == var2 && var1 == var3;
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

   void a(byte[] var1) {
      i var2 = ESGame.u[this.j - 1];
      var2.c(var1);
   }

   static int d(int var0, int var1) {
      int var2 = f.a(100);
      int var3 = f.a(100);
      boolean var4 = var2 <= var1;
      D = var3 <= var0;
      byte var5 = 0;
      if (D && !var4) {
         var5 = 3;
      } else if (D && var4) {
         if (var3 >= var2) {
            var5 = 2;
         } else {
            var5 = 1;
         }
      } else if (D || var4) {
         var5 = 0;
      } else if (var3 >= var2) {
         var5 = 2;
      } else {
         var5 = 1;
      }

      return var5;
   }

   int b(int var1, boolean var2) {
      int var3 = this.R[var1][0];
      if (var2) {
         int var4 = 1 + c[var1];
         var3 += this.J[var4] / 3;
      }

      if (var1 == 11 && this.t(3)) {
         var3 += this.R[1][0];
      }

      if (this.U[6] < 7) {
         var3--;
      }

      return var3;
   }

   int z(int var1) {
      return this.R[var1][1];
   }

   int f(boolean var1) {
      if (this.T[1] != 0) {
         int var2 = a.a(1, this.T[1]);
         var2 = Math.abs(var2);
         return var2 == 5 ? this.b(5, var1) : this.b(7, var1);
      } else {
         return 0;
      }
   }

   int I() {
      if (this.T[1] != 0) {
         int var1 = a.a(1, this.T[1]);
         var1 = Math.abs(var1);
         return var1 == 5 ? this.z(5) : this.z(7);
      } else {
         return 20;
      }
   }

   int t() {
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

   int F() {
      if (this.t(6)) {
         return this.t();
      }

      if (this.T[0] != 0) {
         int var1 = a.a(1, this.T[0]);
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

   int h(boolean var1) {
      if (this.t(14)) {
         return 5 + this.b(4, false);
      }

      if (this.t(6)) {
         int var5 = this.t();
         return this.b(var5, var1);
      }

      int var2 = 0;
      if (this.T[0] != 0) {
         int var3 = a.a(1, this.T[0]);
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

      if (this.t(5)) {
         var2 += this.b(1, false);
      }

      return var2;
   }

   int s() {
      if (this.t(6) || this.t(14)) {
         int var3 = this.t();
         return this.z(var3);
      }

      if (this.T[0] != 0) {
         int var1 = a.a(1, this.T[0]);
         var1 = Math.abs(var1);
         if (var1 == 1) {
            return this.z(0);
         } else if (var1 == 2) {
            return this.z(2);
         } else {
            return var1 == 3 ? this.z(8) : this.z(12);
         }
      } else {
         return 20;
      }
   }

   int z() {
      int var1 = 0;
      if (this.t(14)) {
         var1 = 5 + this.b(4, false);
      } else if (this.t(6)) {
         var1 = 20 + this.b(3, false);
      } else if (this.T[0] != 0) {
         var1 = a.a(3, this.T[0]);
      } else {
         var1 = 0;
      }

      if (this.t(1)) {
         var1 += 10 + this.b(1, false);
      }

      if (this.s) {
         var1 += 25;
      }

      return var1;
   }

   int y() {
      if (this.T[1] != 0) {
         int var1 = a.a(1, this.T[1]);
         var1 = Math.abs(var1);
         return var1 == 5 ? 5 : 7;
      } else {
         return -1;
      }
   }

   int v() {
      int var1 = 0;
      int var2 = 0;
      if (this.T[1] != 0) {
         var2 = a.a(3, this.T[1]);
         var1 += 4 * var2;
      }

      if (this.T[2] != 0) {
         var2 = a.a(3, this.T[2]);
         var1 += 2 * var2;
      }

      if (this.T[3] != 0) {
         var2 = a.a(3, this.T[3]);
         var1 += 2 * var2;
      }

      if (this.T[4] != 0) {
         var2 = a.a(3, this.T[4]);
         var1 += var2;
      }

      if (this.T[5] != 0) {
         var2 = a.a(3, this.T[5]);
         var1 += var2;
      }

      var1 /= 10;
      if (this.t(2)) {
         var1 += 10 + this.b(1, false);
      }

      if (this.t(17)) {
         var1 += this.aa;
      }

      if (this.L) {
         var1 += 15;
      }

      return var1;
   }

   int b(byte[] var1) {
      var1[2] = 2;
      if (this.k()) {
         byte var7 = var1[4];
         int var3 = (var1[5] << 8) + var1[6];
         byte var4 = var1[7];
         this.b(var7, var3, var4);
         ESGame.u[this.j - 1].b(var1);
         System.out.println("Chest item not possessed before");
         int var5 = var7 - 1;
         System.out.println("item index=" + var5);
         if (a.j[var5] == 11) {
            this.W = (short)(this.W + a.c[var5]);
            int var6 = ESGame.d(this.W);
            this.ai.n(var6);
         }

         return 1;
      } else {
         byte[] var2 = new byte[]{var1[0], var1[1], var1[4], var1[5], var1[6], var1[7], 1};
         this.a(var2);
         ESGame.u[this.j - 1].b(var1);
         return 0;
      }
   }

   private boolean t(int var1) {
      if (this.G[var1 - 1] == -1) {
         return true;
      } else {
         return this.G[var1 - 1] == -2 ? this.t != 0 : this.G[var1 - 1] > 0;
      }
   }

   void x(int var1) {
      this.G[var1 - 1] = 0;
   }

   d n() {
      this.g(1);
      if (this.ab <= 0) {
         return null;
      }

      i var1 = ESGame.u[this.ab - 1];
      d var2 = var1.c(this.z, this.w);
      if (var2 != null) {
         var2.c();
      }

      return var2;
   }

   byte[] h() {
      this.g(1);
      if (this.ab <= 0) {
         return null;
      }

      byte var1 = this.ab;
      Hashtable var2 = ESGame.S[var1 - 1];
      if (var2 == null) {
         return null;
      }

      Object var3 = var2.get(f.b((int)this.z, (int)this.w));
      return var3 == null ? null : (byte[])var3;
   }

   int r() {
      this.g(1);
      if (this.ab <= 0) {
         return -1;
      }

      byte var1 = this.ab;
      return var1 != 1 ? -1 : k.a(this.z, this.w);
   }

   int u(int var1) {
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

   void p(int var1) {
      System.out.println("start of castSpell");
      int var2 = this.u(var1);
      int var3 = this.b(var2, true);
      int var4 = this.z(var2);
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
         this.U[4] = (short)(this.U[4] - 3 * var7);
      } else if (var12 == 1) {
         this.U[4] = (short)(this.U[4] - 3 * var7 / 2);
      } else if (var12 == 2) {
         this.U[4] = (short)(this.U[4] - var7);
      } else if (var12 == 3) {
         this.U[4] = (short)(this.U[4] - var7);
         var13 = 2;
      }

      this.U[4] = (short)Math.max(this.U[4], 0);
      if (var12 >= 2) {
         this.a(var2, 1);
      }

      switch (var1) {
         case 1:
         case 2:
         case 3:
         case 5:
            this.G[var1 - 1] = (byte)(var8 * var13);
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
            if (this.c(109, var14) && this.a(true)) {
               this.G[var1 - 1] = (byte)(var8 * var13);
            }
            break;
         case 21:
            int var15 = 6 + this.b(10, false);
            this.U[2] = (short)(this.U[2] + var13 * var15);
            this.U[2] = (short)Math.min(this.U[2], this.U[3]);
            break;
         case 23:
            this.G[var1 - 1] = -2;
            break;
         case 24:
            this.G[var1 - 1] = -4;
            break;
         case 25:
            for (int var16 = 1; var16 <= var13; var16++) {
               this.H();
            }
      }

      this.U[6] = (short)(this.U[6] - 5 * this.c());
      this.U[6] = (short)Math.max(this.U[6], 0);
      if (this.k(6)) {
         int var17 = 2 * this.U[3] / 100;
         if (var17 < 1) {
            var17 = 1;
         }

         this.U[2] = (short)(this.U[2] - (short)var17);
      }
   }

   void b(int var1, d var2) {
      int var3 = this.u(var1);
      int var4 = this.b(var3, true);
      int var5 = this.z(var3);
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
         this.U[4] = (short)(this.U[4] - 3 * var8);
      } else if (var14 == 1) {
         this.U[4] = (short)(this.U[4] - 3 * var8 / 2);
      } else if (var14 == 2) {
         this.U[4] = (short)(this.U[4] - var8);
      } else if (var14 == 3) {
         this.U[4] = (short)(this.U[4] - var8);
         var15 = 2;
      }

      this.U[4] = (short)Math.max(this.U[4], 0);
      if (var14 >= 2) {
         this.a(var3, 1);
      }

      switch (var1) {
         case 4:
            var2.c[9] = -2;
            var2.d();
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
            this.U[6] = (short)(this.U[6] + var33);
            this.U[6] = (short)Math.min(this.U[6], this.U[7]);
            this.U[2] = (short)(this.U[2] + var33);
            this.U[2] = (short)Math.min(this.U[2], this.U[3]);
            this.U[4] = (short)(this.U[4] + 12);
            this.U[4] = (short)Math.min(this.U[4], this.U[5]);
            break;
         case 9:
            if (var2.e()) {
               int var32 = 60 * var15;
               int var37 = var32 - var2.c(8);
               var37 = Math.max(var37, 4);
               int var42 = var37 * var2.c(14) / 100;
               var2.b(var42);
            }
            break;
         case 10:
            this.G[var1 - 1] = -2;
            var2.c[8] = (byte)(2 * var15);
            break;
         case 11:
            int var31 = 25 + this.b(4, false);
            int var36 = var31 * var15;
            int var40 = var36 - var2.c(8);
            var40 = Math.max(var40, 4);
            int var43 = var40 * var2.c(14) / 100;
            var2.b(var43);
            var2.d();
            break;
         case 12:
            this.G[var1 - 1] = -2;
            int var29 = var15 * (10 + this.b(4, false));
            var29 = Math.min(var29, 255);
            var2.c[4] = (byte)var29;
            var2.d();
            break;
         case 13:
            this.G[var1 - 1] = -2;
            int var27 = var15 * (10 + this.b(4, false));
            var27 = Math.min(var27, 255);
            var2.c[5] = (byte)var27;
            var2.d();
            break;
         case 14:
            this.G[var1 - 1] = -1;
            this.a(var2);
            this.G[var1 - 1] = 0;
            break;
         case 15:
            this.G[var1 - 1] = -2;
            var2.c[2] = 1;
            var2.d();
            break;
         case 16:
            int var16 = 10 - var6;
            if (var16 > 0) {
               var16 = var15 * var16;
               this.G[var1 - 1] = (byte)var16;
               var2.c[6] = 1;
            }
            break;
         case 17:
            this.G[var1 - 1] = -2;
            this.aa = (short)(10 + this.b(6, false));
            break;
         case 18:
            this.G[var1 - 1] = -2;
            var2.c[0] = (byte)(3 * var15);
            break;
         case 19:
            this.G[var1 - 1] = -2;
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

      this.U[6] = (short)(this.U[6] - 5 * this.c());
      this.U[6] = (short)Math.max(this.U[6], 0);
      if (this.k(6)) {
         int var22 = 2 * this.U[3] / 100;
         if (var22 < 1) {
            var22 = 1;
         }

         this.U[2] = (short)(this.U[2] - (short)var22);
      }
   }

   void a(int var1, int var2) {
      if (var1 >= 0) {
         this.R[var1][2] = (short)(this.R[var1][2] + var2);
      }
   }

   boolean c(byte[] var1) {
      byte var2 = var1[2];
      int var3 = (var1[3] << 8) + var1[4];
      byte var4 = var1[5];
      return this.b(var2, var3, var4);
   }

   boolean c(int var1, int var2) {
      return this.b(var1, var2, 0);
   }

   boolean b(int var1, int var2, int var3) {
      if (this.p < 24) {
         this.H[this.p] = (byte)var1;
         int var4 = (var2 << 16) + (byte)var3;
         this.P[this.p] = var4;
         this.p++;
         return true;
      } else {
         return false;
      }
   }

   boolean y(int var1) {
      if (var1 >= this.p) {
         return false;
      }

      this.A(var1);
      this.H[var1] = 0;

      for (int var2 = var1; var2 < this.p - 1; var2++) {
         this.H[var2] = this.H[var2 + 1];
         this.P[var2] = this.P[var2 + 1];
      }

      this.p--;
      return true;
   }

   void i(int var1) {
      int var2 = Math.abs(this.H[var1]);
      if (var2 != 109) {
         byte[] var3 = new byte[]{this.l, this.k, (byte)var2, 0, 0, (byte)(this.P[var1] & 0xFF), 0};
         int var4 = this.P[var1] >>> 16 & 65535;
         var3[3] = (byte)(var4 >> 8 & 0xFF);
         var3[4] = (byte)(var4 & 0xFF);
         var3[6] = 3;
         this.b().c(var3);
         this.y(var1);
      } else {
         this.y(var1);
      }
   }

   boolean C(int var1) {
      byte var2 = this.H[var1];
      return !a.c(Math.abs(var2)) ? false : var2 < 0;
   }

   boolean d(int var1, boolean var2) {
      byte var3 = this.H[var1];
      if (var3 < 0) {
         return false;
      }

      if (!a.c(var3)) {
         return false;
      }

      int var4 = a.a(var3);
      if (this.T[var4] != 0) {
         if (!var2) {
            return false;
         }

         this.f(var4);
      }

      this.T[var4] = var3;
      this.H[var1] = (byte)(-Math.abs(this.H[var1]));
      return true;
   }

   private void f(int var1) {
      for (int var2 = 0; var2 < this.p; var2++) {
         byte var3 = this.H[var2];
         var3 = (byte)Math.abs(var3);
         int var4 = a.a(var3);
         if (var4 == var1) {
            this.A(var2);
         }
      }
   }

   boolean a(boolean var1) {
      int var2 = this.p - 1;
      return this.d(var2, var1);
   }

   void A(int var1) {
      if (this.C(var1)) {
         if (var1 >= 0 && var1 <= 23) {
            byte var2 = this.H[var1];
            var2 = (byte)Math.abs(var2);
            this.H[var1] = var2;

            for (int var3 = 0; var3 < 7; var3++) {
               if (this.T[var3] == var2) {
                  this.T[var3] = 0;
                  break;
               }
            }
         }
      }
   }

   boolean h(int var1) {
      byte var2 = this.H[var1];
      var2 = (byte)Math.abs(var2);
      if (a.b(var2)) {
         byte var3 = (byte)(this.P[var1] & 0xFF);
         var3 = 3;
         this.P[var1] = this.P[var1] & -256;
         this.P[var1] = this.P[var1] | var3;
         return true;
      } else {
         return false;
      }
   }

   boolean j(int var1) {
      byte var2 = this.H[var1];
      var2 = (byte)Math.abs(var2);
      byte var3 = (byte)(this.P[var1] & 0xFF);
      return var3 == 3;
   }

   int D(int var1) {
      byte var2 = this.H[var1];
      var2 = (byte)Math.abs(var2);
      return a.c[var2 - 1];
   }

   int o(int var1) {
      int var2 = -1;
      int var3 = -Math.abs(var1);

      for (int var4 = 0; var4 < this.p; var4++) {
         if (var3 == this.H[var4]) {
            var2 = var4;
            break;
         }
      }

      return var2;
   }

   boolean k() {
      return this.p < 24;
   }

   String b(int var1) {
      int var2 = Math.abs(this.H[var1]);
      byte var3 = a.j[var2 - 1];
      String var4 = null;
      switch (var3) {
         case 1:
         case 2:
         case 3:
         case 4:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            int var5 = a.m[var2 - 1] + (this.P[var1] & 0xFF);
            var4 = var4 + "\nWeapon value: " + var5;
            break;
         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            int var6 = a.m[var2 - 1] + (this.P[var1] & 0xFF);
            var4 = var4 + "\nArmor value: " + var6;
            break;
         case 11:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            break;
         case 12:
            var4 = a.b[var2 - 1] + '\n' + "Spell: ";
            int var7 = this.P[var1] & 0xFF;
            var4 = var4 + b.b[var7 - 1].c;
            break;
         case 13:
            int var8 = var2 - 87;
            String[] var9 = a.l[var8];
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1] + '\n' + var9[0];
            if (var9[1].length() > 0) {
               var4 = var4 + '\n' + var9[1];
            }
            break;
         case 14:
         case 15:
         case 16:
         default:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            break;
         case 17:
            var4 = a.b[var2 - 1] + '\n' + a.g[var3 - 1];
            int var10 = this.b(3, false);
            int var11 = 20 + var10;
            var4 = var4 + "\nWeapon value: " + var11;
      }

      return var4;
   }

   boolean r(int var1) {
      int var2 = Math.abs(this.H[var1]);
      byte var3 = a.j[var2 - 1];
      int var4 = this.P[var1] & 0xFF;
      int var5 = var4 - 1;
      this.x = f.a(var5, this.x);
      this.y(var1);
      return true;
   }

   void a(int var1) {
      this.a(var1, e.j);
   }

   boolean w(int var1) {
      int var2 = Math.abs(this.H[var1]);
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
         case 17:
            return true;
         case 11:
         case 12:
         case 13:
         case 14:
         case 15:
         case 16:
         default:
            return false;
      }
   }

   boolean v(int var1) {
      int var2 = Math.abs(this.H[var1]);
      byte var3 = a.j[var2 - 1];
      switch (var3) {
         case 13:
         case 15:
            return true;
         default:
            return false;
      }
   }

   boolean e(int var1) {
      int var2 = Math.abs(this.H[var1]);
      byte var3 = a.j[var2 - 1];
      switch (var3) {
         case 12:
            int var4 = this.P[var1] & 0xFF;
            byte var5 = b.b[var4 - 1].h;
            if (this.R[var5][0] > 0) {
               return true;
            }

            return false;
         default:
            return false;
      }
   }

   int n(int var1) {
      int var2 = this.U[var1];
      if (this.t(23)) {
         if (var1 == 2) {
            var2 += this.b(10, false);
            if (var2 > this.U[3]) {
               var2 = this.U[3];
            }
         } else if (var1 == 6) {
            var2 += this.b(10, false);
            if (var2 > this.U[7]) {
               var2 = this.U[7];
            }
         } else if (var1 == 4) {
            var2 += this.b(10, false);
            if (var2 > this.U[5]) {
               var2 = this.U[5];
            }
         }
      }

      return var2;
   }

   boolean x() {
      return this.C > 0;
   }

   void D() {
      this.C = this.j;
      this.ao = this.l;
      this.an = this.k;
      this.Z = this.ak;
      this.c(true);
      this.w();
      this.Q = true;
   }

   void e() {
      this.j = this.ab = this.C;
      this.l = this.z = this.ao;
      this.k = this.w = this.an;
      this.w();
      this.Q = true;
   }

   int E() {
      int var1 = 0;

      for (int var2 = 0; var2 < 8; var2++) {
         int var3 = this.A >> var2 & 1;
         if (var3 != 0) {
            var1++;
         }
      }

      return var1;
   }

   void H() {
      int var1 = this.E();
      if (var1 > 0) {
         int var2 = 0;
         if (var1 == 1) {
            var2 = 1;
         } else {
            var2 = f.a(var1);
         }

         int var3 = 0;

         for (int var4 = 0; var4 < 8; var4++) {
            int var5 = this.A >> var4 & 1;
            if (var5 == 1) {
               if (++var3 == var2) {
                  this.A = (byte)f.c(var4, this.A);
                  break;
               }
            }
         }
      }
   }

   int c() {
      return (this.A & 1) == 1 ? 3 : 1;
   }

   i b() {
      return ESGame.u[this.j - 1];
   }

   void w() {
      this.b().b(this.l, this.k, this.ak, this.ae);
   }

   String j() {
      StringBuffer var1 = new StringBuffer(450);
      String var2 = " ";
      String var3 = ": ";
      var1.append(this.v);
      var1.append('\n');
      var1.append(K[this.ar]);
      var1.append('\n');
      var1.append("Level ");
      var1.append(this.U[0]);
      var1.append(" (");
      var1.append(this.U[1]);
      var1.append("/10)");
      var1.append('\n');
      var1.append("Health: ");
      var1.append(this.n(2));
      var1.append('/');
      var1.append(this.U[3]);
      var1.append('\n');
      var1.append("Magic: ");
      var1.append(this.n(4));
      var1.append('/');
      var1.append(this.U[5]);
      var1.append('\n');
      var1.append("Fatigue: ");
      var1.append(this.n(6));
      var1.append('/');
      var1.append(this.U[7]);
      var1.append('\n');
      var1.append('\n');
      var1.append("Status ailments: ");
      int var4 = 0;

      for (int var5 = 1; var5 <= 8; var5++) {
         if (this.k(var5)) {
            var1.append('\n');
            var1.append(af[var5 - 1]);
            var4++;
         }
      }

      if (var4 == 0) {
         var1.append('\n');
         var1.append("None");
      }

      var1.append('\n');
      var1.append('\n');
      var1.append("Gift points found: ");
      var1.append(this.W);
      var1.append('\n');
      var1.append('\n');
      var1.append("Attributes:");
      var1.append('\n');

      for (int var6 = 0; var6 < 8; var6++) {
         int var7 = 2 * var6;
         var1.append(y[var7]);
         var1.append(var3);
         var1.append(this.J[var7]);
         var1.append('\n');
      }

      String var8 = var1.toString();
      System.out.println("Length of stats string is " + var8.length());
      return var8;
   }

   static Vector G() {
      Vector var0 = new Vector();

      for (int var1 = 0; var1 < 13; var1++) {
         var0.addElement(new Object());
      }

      return var0;
   }

   static String d(byte[] var0) {
      String var1 = "X = " + var0[0] + '\n';
      var1 = var1 + "Y = " + var0[1] + '\n';
      var1 = var1 + "Type = " + var0[2] + '\n';
      var1 = var1 + "ID(MSB) = " + var0[3] + '\n';
      var1 = var1 + "ID(LSB) = " + var0[4] + '\n';
      var1 = var1 + "value = " + var0[5] + '\n';
      return var1 + "flags = " + var0[6] + '\n';
   }

   Vector f() {
      Vector var1 = new Vector();

      for (int var2 = 0; var2 < 14; var2++) {
         if (this.R[var2][0] > 0) {
            String var3 = E[var2] + ": " + this.R[var2][0];
            var1.addElement(var3);
         }
      }

      return var1;
   }

   int l(int var1) {
      int var2 = 0;

      for (int var3 = 0; var3 < 14; var3++) {
         if (this.R[var3][0] > 0) {
            if (var2 == var1) {
               return var3;
            }

            var2++;
         }
      }

      return -1;
   }

   String m(int var1) {
      return E[var1] + '\n' + "Rank: " + this.R[var1][0] + '\n' + "Exp: " + this.R[var1][2] + "/10";
   }

   Vector J() {
      Vector var1 = new Vector();

      for (int var2 = 0; var2 < b.i; var2++) {
         if ((this.x & 1 << var2) != 0) {
            int var3 = var2 + 1;
            String var4 = b.b[var2].c;
            if (var3 == this.b) {
               var4 = "R: " + var4;
            }

            var1.addElement(var4);
         }
      }

      return var1;
   }

   int B(int var1) {
      int var2 = 0;

      for (int var3 = 0; var3 < b.i; var3++) {
         if ((this.x & 1 << var3) != 0) {
            int var4 = var3 + 1;
            if (var2 == var1) {
               return var3;
            }

            var2++;
         }
      }

      return -1;
   }

   int l() {
      if (!b.a(this.b)) {
         int var3 = this.B(0);
         return var3 < 0 ? 0 : var3 + 1;
      }

      int var1 = this.b - 1;
      int var2 = var1 + 1;
      if (var2 == b.i) {
         var2 = 0;
      }

      while (var2 != var1) {
         if ((this.x & 1 << var2) != 0) {
            return var2 + 1;
         }

         if (++var2 == b.i) {
            var2 = 0;
         }
      }

      return this.b;
   }

   String s(int var1) {
      int var2 = var1;
      String var3 = b.b[var2].c + '\n';
      var3 = var3 + E[b.b[var2].h] + '\n';
      var3 = var3 + "Cost: " + b.b[var2].e + '\n';
      return var3 + b.b[var2].a;
   }

   void e(boolean var1) {
      short var2 = (short)(this.U[3] - this.U[2]);
      short var3 = (short)(this.U[5] - this.U[4]);
      short var4 = (short)(this.U[7] - this.U[6]);
      if (!var1) {
         var2 = (short)(2 * var2 / 3);
         var3 = (short)(2 * var3 / 3);
         var4 = (short)(2 * var4 / 3);
      }

      if (this.k(8)) {
         var2 = (short)(3 * var2 / 4);
         var3 = (short)(3 * var3 / 4);
         var4 = (short)(3 * var4 / 4);
      }

      this.U[2] = (short)(this.U[2] + var2);
      this.U[4] = (short)(this.U[4] + var3);
      this.U[6] = (short)(this.U[6] + var4);
      this.s = false;
      this.L = false;
      this.I = false;
      int var5 = f.a(100);
      if (var5 <= 10) {
         for (int var6 = 0; var6 < this.p; var6++) {
            int var7 = Math.abs(this.H[var6]);
            if (var7 == 96) {
               this.y(var6);
               break;
            }
         }
      }

      for (int var9 = 0; var9 < 8; var9++) {
         int var10 = var9 + 1;
         if (var10 != 4 && var10 != 5) {
            var5 = f.a(100);
            if (var5 <= 25) {
               this.A = (byte)f.c(var9, this.A);
            }
         }
      }
   }

   boolean k(int var1) {
      int var2 = var1 - 1;
      return (this.A & 1 << var2) != 0;
   }

   void a(int var1, d var2) {
      int var3 = Math.abs(this.H[var1]);
      byte var4 = a.j[var3 - 1];
      if (var4 == 13 || var4 == 15) {
         boolean var5 = true;
         switch (var3) {
            case 87:
               if (this.j == 1 && this.x()) {
                  this.e();
                  break;
               }

               this.D();
               break;
            case 88:
               this.H();
               break;
            case 89:
               this.U[2] = this.U[3];
               break;
            case 90:
               this.U[4] = this.U[5];
               break;
            case 91:
               this.U[6] = (short)(this.U[6] + 3 * this.U[5]);
               break;
            case 92:
               this.U[1]++;
               break;
            case 93:
               this.U[2] = this.U[3];
               this.U[4] = this.U[5];
               break;
            case 94:
               this.s = true;
               break;
            case 95:
               this.L = true;
               break;
            case 96:
               this.I = true;
               var5 = false;
               break;
            case 97:
               if (var2 != null) {
                  int var9 = var2.c(4);
                  int var11 = var2.c(10);
                  if (var9 <= 13 && var11 <= 13) {
                     var2.g = 0;
                     var2.d();
                  }
               }
               break;
            case 98:
               if (var2 != null) {
                  int var8 = var2.c(4);
                  int var10 = var2.c(10);
                  if (var8 <= 22 && var10 <= 22) {
                     var2.g = 0;
                     var2.d();
                  }
               }
               break;
            case 99:
               if (var2 != null) {
                  int var6 = var2.c(4);
                  int var7 = var2.c(10);
                  if (var6 <= 29 && var7 <= 29) {
                     var2.g = 0;
                     var2.d();
                  }
               }
         }

         if (var5) {
            this.y(var1);
         }
      }
   }

   boolean o() {
      for (int var1 = 0; var1 < 14; var1++) {
         if (this.R[var1][2] >= 10) {
            this.R[var1][2] = (short)(this.R[var1][2] - 10);
            this.R[var1][0]++;
            short var2 = c[var1];
            int var3 = var2 / 2;
            this.N = (byte)(this.N | 1 << var3);
            this.U[1]++;
         }
      }

      if (this.U[1] >= 10) {
         this.U[0]++;
         return true;
      } else {
         return false;
      }
   }

   void d() {
      k.d();
      this.U[1] = (short)(this.U[1] - 10);
   }

   String[] q() {
      Vector var1 = new Vector();

      for (int var2 = 0; var2 < 8; var2++) {
         if ((this.N & 1 << var2) != 0) {
            int var3 = var2 * 2;
            var1.addElement(y[var3]);
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

   private void C() {
      short var1 = a.a();
      int[] var2 = ac[this.ar];

      for (int var3 = 0; var3 < var2.length; var3++) {
         this.c(var2[var3], var1);
         int var4 = this.p - 1;
         this.d(var4, true);
      }
   }

   void a(long var1) {
      int var3 = (int)(var1 * (this.J[10] + this.J[11]) / 2000L);
      this.U[6] = (short)(this.U[6] + var3);
      if (this.U[6] > this.U[7]) {
         this.U[6] = this.U[7];
      }
   }

   private boolean a(int var1, int var2, int var3) {
      if (var1 != 1) {
         return false;
      } else {
         return var2 < 6 || var2 > 12 ? false : var3 >= 6 && var3 <= 12;
      }
   }

   private void p() {
      this.D();
      if (this.Q) {
         this.Q = false;
      }
   }

   int b(int var1, int var2) {
      int var3 = this.b(13, true);
      if (var2 == 3) {
         var3 += 3;
      }

      short var4 = k.r[var1];
      int var5 = var3 - var4;
      int var6 = 20 - var5 * 5;
      int var7 = 20 + this.J[12] / 2 + var5 * 5;
      var6 = Math.min(Math.max(var6, 10), 95);
      var7 = Math.min(Math.max(var7, 10), 95);
      return d(var7, var6);
   }

   private void B() {
      this.ak--;
      if (this.ak <= 0) {
         this.ak = 4;
      }
   }

   private void a() {
      this.ak++;
      if (this.ak >= 5) {
         this.ak = 1;
      }
   }

   String K() {
      byte var1 = this.ak;
      StringBuffer var2 = new StringBuffer(1000);
      System.out.println("here 1");
      String[] var3 = this.b().a();
      var2.append("Current dungeon is " + var3[0] + " " + var3[1] + "\n");

      for (int var4 = 1; var4 <= 4; var4++) {
         if (var4 <= 2) {
            this.g(var4);
         } else if (var4 == 3) {
            this.a();
            this.g(1);
         } else if (var4 == 4) {
            this.B();
            this.g(1);
         }

         System.out.println("here 2, i =" + var4);
         if (var4 == 1) {
            var2.append("FORWARD SQUARE: \n");
         } else if (var4 == 2) {
            var2.append("BACKWARD SQUARE: \n");
         } else if (var4 == 3) {
            var2.append("RIGHT SIDE SQUARE: \n");
         } else if (var4 == 4) {
            var2.append("LEFT SIDE SQUARE: \n");
         }

         var2.append("x,y = " + this.z + ", " + this.w + "\n");
         System.out.println("New a and y are " + this.z + ", " + this.w);
         var2.append("map value = " + this.b().w[this.z][this.w] + "\n");
         System.out.println("New dungeon id is " + this.ab);
         Hashtable var5 = ESGame.G[this.ab - 1];
         if (var5 != null) {
            Enumeration var6 = var5.elements();

            while (var6.hasMoreElements()) {
               byte[] var7 = (byte[])var6.nextElement();
               System.out.println("Found a monster");
               if (var7[4] == this.z && var7[5] == this.w) {
                  var2.append("Found monster in square \n");
                  var2.append("type=" + var7[2] + ", health=" + var7[3] + ", dungeon id = " + var7[7] + "\n");
               }
            }
         }

         System.out.println("here 3, i =" + var4);
         var5 = ESGame.S[this.ab - 1];
         if (var5 != null) {
            Enumeration var9 = var5.elements();

            while (var9.hasMoreElements()) {
               byte[] var11 = (byte[])var9.nextElement();
               if (var11[0] == this.z && var11[1] == this.w) {
                  var2.append("Found chest in square \n");
                  var2.append("item type=" + var11[4] + ", value=" + var11[7] + "\n");
               }
            }
         }

         System.out.println("here 4, i =" + var4);
         Enumeration var10 = ESGame.au[this.ab - 1].elements();

         while (var10.hasMoreElements()) {
            byte[] var12 = (byte[])var10.nextElement();
            if (var12[0] == this.z && var12[1] == this.w) {
               var2.append("Found dropped item in square \n");
               var2.append("item type=" + var12[2] + ", value=" + var12[5] + " flags = " + var12[6] + "\n");
            }
         }

         this.ak = var1;
      }

      if (k.d) {
         var2.append("Warden IS visiting now\n");
      } else {
         var2.append("Warden IS NOT visiting now\n");
      }

      var2.append("Player inventory: nitems=" + this.p);
      return var2.toString();
   }
}
