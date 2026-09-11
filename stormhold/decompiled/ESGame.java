import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Random;
import java.util.Vector;
import javax.microedition.lcdui.Alert;
import javax.microedition.lcdui.AlertType;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.StringItem;
import javax.microedition.lcdui.TextField;
import javax.microedition.rms.RecordStore;

public class ESGame extends ngame.midlet.a implements Runnable, CommandListener {
   private static int aT;
   static byte[] at = new byte[2000];
   static String aN = null;
   Form w;
   StringItem ad;
   Form ao;
   StringItem ar;
   private static final Command aC = new Command("Menu", 7, 0);
   private static final Command aS = new Command("Exit", 7, 0);
   static int af;
   static int aO;
   static byte[][] i;
   static int J = 1;
   static String c = "";
   private static Thread b = null;
   private static int E = 0;
   private static byte[][] ai;
   private static int[][] x = new int[][]{
      {2, 3, 4, 11, 12, 13, 20, 21, 22, 29, 30, 31}, {5, 6, 7}, {23, 24, 25}, {14, 15, 16}, {8, 9, 10}, {32, 33, 34}, {26, 27, 28}, {17, 18, 19}, {35, 36, 37}
   };
   private static String[][] T;
   private static final int[][] ah = new int[][]{{0, 7}, {7, 7}, {14, 7}, {21, 7}, {28, 5}};
   static final String[] p = new String[]{
      "/charin.dat", "/droppeditemsin.dat", "/itemsin.dat", "/monstersin.dat", "/npcstrings.dat", "/spellsin.dat", "/geomin.dat", "/dungnamesin.dat"
   };
   private static int[] l = new int[3];
   private static final String[] aM = new String[]{
      "Your fingers look gnarled to you.",
      "The scales on your arms and back itch.",
      "Your ears dissolve back into your skull.",
      "Your jaw hurts as it elongates, and your teeth seem to completely fill your mouth.",
      "Walking hurts, and the camp denizens are sure looking tasty. \n\nYour dreams are filled with the screams of overseers and others as you follow the delicious scent of blood throughout the camp. You awake cold, curled up, with Vander's head tucked under your arm."
   };
   Display d;
   private Form W;
   static Image O = null;
   static Image aU = null;
   private h aR;
   private h aZ;
   private static h al;
   private static h aQ;
   private h ay;
   private h U;
   private h az;
   private h y;
   private h h;
   private h aE;
   private h M;
   private h ba;
   private h f;
   h ab;
   h aq;
   h m;
   h[] R;
   h Q;
   h ap;
   h z;
   h ae;
   h I;
   h X;
   h V;
   h aJ;
   h an;
   h aw;
   h aI;
   h aL;
   h H;
   h K;
   h s;
   h t;
   h B;
   h aY;
   h aB;
   h L;
   h N;
   h v;
   h q;
   h ag;
   h aV;
   h aP;
   h C;
   h D;
   private h F;
   h aj;
   h A;
   private e av;
   private Form aD;
   private static String[] o = new String[12];
   private static String[] as = new String[12];
   private static String n = null;
   public j k;
   public j e;
   static i[] u;
   Thread aH;
   boolean ak;
   boolean aW;
   byte a;
   boolean am;
   static Hashtable[] G;
   static Hashtable[] S;
   static Vector[] au;
   static h ax = null;
   int Y;
   int g;
   static boolean aG;
   static Image aa;
   static Image aA;
   boolean ac;
   static Random P;
   private static boolean j = false;

   public ESGame() {
      super.aF = "The Elder Scrolls";
      this.d = null;
      aN = this.getAppProperty("Pluto-Server-URL");
      if (aN != null) {
         System.out.println("FOUND Pluto-Server-URL in JAR! Adding prefix gives: " + aN);
      } else {
         aN = "http://localhost/essm";
         System.out.println("Did not find Pluto-Server-URL in JAR! Using default of http://localhost/essm");
      }

      String var1 = this.getAppProperty("Mserver-User-Id");
      if (var1 != null) {
         j.X = var1;
         System.out.println("User ID is " + j.X);
      } else {
         System.out.println("User ID is NULL!");
      }

      aT = 1;
   }

   public void m() {
      if (this.d == null) {
         c("Very start of startapp");
         this.d = Display.getDisplay(this);
         this.Y = -1;
         this.g = -1;
         this.aH = new Thread(this);
         this.ak = false;
         this.aW = false;
         this.a = 0;
         this.am = false;
         c("Before error form");
         this.y();
         c("After error form");
         this.H();
         aT = 2;
      }
   }

   private void H() {
      try {
         aa = Image.createImage("/mformaLogo.png");
         aA = Image.createImage("/vir2lLogo.png");
         Thread var1 = new Thread(this);
         O = this.d("/splashtop.png");
         aU = this.d("/splashbot.png");
         this.ay = new h(this, 2, 1);
         this.ay.e();
         this.ay.c = this.w;
         J = 2;
         this.ac = true;
         aG = false;
         this.a((Object)this.ay);
         var1.start();
      } catch (Exception var2) {
         System.out.println("Barfed in initSplash");
         this.d.setCurrent(this.w);
      }
   }

   public void run() {
      if (J == 1) {
         System.out.println("run() Initial download, no longer implemented");
      } else if (J == 2) {
         this.e();
      } else if (J == 4) {
         this.o();
      } else if (J == 5) {
         if (this.l()) {
            this.a(this.av);
         } else {
            this.a((Object)this.A);
         }
      } else if (J == 6) {
         if (this.w()) {
            this.J();
            this.a = this.k.j;
            this.ak = true;
            j = true;
            j = false;
            this.ak = false;
            aQ.m = 100;
            aQ.c();
            aQ.f();
            this.k.w();
            this.av.ax = this.k;
            this.av.v = true;
            this.av.i();
            this.a(this.av);
         } else {
            this.a((Object)this.ba);
         }
      } else {
         this.s();
         this.a(this.av);
      }
   }

   private void e() {
      try {
         this.ac = true;
         aG = false;

         try {
            Thread.sleep(1000L);
         } catch (Exception var4) {
         }

         this.ac = false;
         this.ay.m = 0;
         this.t();
         this.G();
         this.A();
         this.ay.m = 100;
      } catch (Throwable var5) {
         System.out.println("ERROR: CANNOT LOAD APP!!");
         System.out.println(var5);
         this.a("" + var5, true);
         this.a(this.w);

         try {
            Thread.sleep(15000L);
         } catch (Exception var3) {
         }
      }
   }

   private void t() throws Exception {
      this.a("Start of allocateESGame", true);
      this.a("Start of allocateESGame", true);
      I();
      i.g();
      this.a("Right before character load", true);
      j.u();
      this.a("ESPersonality load", true);
      k.e();
      this.ay.m = 5;
      this.a("Item load", true);
      a.e();
      this.a("Spell load", true);
      b.a();
      this.a("Monster load", true);
      d.g();
      this.ay.m = 10;
      this.a("End of allocESGame", true);
   }

   private void A() {
      u = null;
      System.gc();
      System.out.println(" >>>> CREATING CAMP DUNGEON <<<<<<");
      c("    Before dungeon vector");
      u = new i[37];
      c("    After dungeon vector");
      this.ay.m = 62;
      u[0] = new i((byte)1, ai[0], af, aO, i);
      c("    After camp dungeon before GC");
      System.gc();
      c("    After camp dungeon after GC");

      for (int var1 = 1; var1 < 37; var1++) {
         u[var1] = new i((byte)(var1 + 1), ai[var1]);
         c("Before dungeon " + var1);
         u[var1].b();
         c("    After dungeon " + var1 + " before GC");
         System.gc();
         c("    After dungeon " + var1);
         this.ay.m++;
      }

      System.out.println(" After creating dungeons");
      c(" After creating dungeons, before GC");
      System.gc();
      c(" After creating dungeons");
   }

   private void o() {
      System.out.println("Start of createNewGame");
      this.aZ.m = 0;
      int var1 = d(0);

      for (int var2 = 0; var2 <= var1; var2++) {
         this.n(var2);
      }

      this.a((Object)this.az);
   }

   private void J() {
      int var1 = d(this.k.W);
      i(var1);
   }

   private void G() throws Exception {
      System.out.println("Starting allocateAllUIs");
      this.a("Start of allocateAllUIs", true);
      System.gc();
      c("Start of allocateAllUIs");
      this.h();
      this.ay.m = 20;
      this.j();
      this.av = new e(this);
      c("Before floors and walls");
      e.h = this.d("floor3.png");
      c("after floors");
      e.Y = this.d("newwallsnok.png");
      c("After walls");
      this.a("After floor and wall images", true);
      e.q = new g[33];

      for (int var1 = 0; var1 < 33; var1++) {
         e.q[var1] = null;
      }

      this.a("After alloc monster images", true);
      this.a = 1;
      this.ak = true;
      this.a("before runImageLoader", true);
      this.ab = new h(this, 11, 304);
      this.ab.o();
      c("Before load camp monster images ");
      this.p();
      this.a("After monster images", true);
      c("After monster images ");
      this.j();
      e.ar = new g[3];
      e.ar[0] = g.b("baglarge.cus");
      e.ar[1] = g.b("bagmid.cus");
      e.ar[2] = g.b("bagsmall.cus");
      c("After bag images ");
      System.gc();
      this.a("After bag images ", true);
      e.u = new g[3];
      e.u[0] = g.b("crystalnear.cus");
      e.u[1] = g.b("crystalmid.cus");
      e.u[2] = g.b("crystalfar.cus");
      c("After crystal images ");
      this.a("After crystal images ", true);
      this.j();
      c("After oracle images ");
      this.a("After oracle images ", true);
      e.aE = new Image[3];

      for (int var2 = 0; var2 < 3; var2++) {
         e.aE[var2] = null;
      }

      e.aE[0] = this.d("blood1.png");
      e.aE[1] = this.d("monsterspell.png");
      e.aE[2] = this.d("selfspell.png");
      System.gc();
      c("After spell images ");
      this.a("After effects images ", true);
      e.ab = new g[3];
      e.ab[0] = g.b("chestnearclosed.cus");
      e.ab[1] = g.b("chestmidclosed.cus");
      e.ab[2] = g.b("chestfarclosed.cus");
      this.a("After chest images ", true);
      this.j();
      e.y = new Image[6];
      e.y[0] = this.d("icon_attack.png");
      e.y[1] = this.d("icon_cast.png");
      e.y[2] = this.d("icon_change.png");
      e.y[3] = this.d("icon_option.png");
      e.y[4] = this.d("icon_action.png");
      e.y[5] = this.d("icon_camp.png");
      this.a("After monster and icon images", true);
      System.gc();
      this.j();
      this.a();
      this.a("After HELP STRINGS", true);
      this.u();
      this.a("After HELP TITLES", true);
      n = this.q();
      this.a("After CREDITS", true);
      this.U = new h(this, 3, 2);
      String[] var3 = new String[]{"New Game", "Continue Game", "Help", "Credits", "Exit"};
      Object var4 = null;
      this.U.a("Main Menu", var3, (Vector)var4, false);
      this.az = new h(this, 5, 3);
      String[] var5 = j.K;
      this.az.a("New Game", "Select a Class:", var5, null);
      this.ay.m = 35;
      this.a("After newGameUI", true);
      this.y = new h(this, 6, 4);
      String[] var6 = new String[]{"See Class Info", "Create Character"};
      Object var7 = null;
      this.y.a("Character", "You selected:", "", var6, (Vector)var7);
      this.h = new h(this, 4, 5);
      this.h.a("Info", "");
      this.aE = new h(this, 4, 6);
      this.aE.a("New Character", "Character Created!\n \nPress 'select' to enter a name");
      this.aE.b(h.I);
      this.aE.a(h.u);
      this.aE.a(h.P);
      this.M = new h(this, 4, 7);
      this.M.a("Welcome", "Welcome to The Elder Scrolls Travels!");
      c("After all the welcome screens");
      this.a("After all the welcome screens", true);
      this.A = new h(this, 4, 499);
      this.A
         .a(
            "Save Error",
            "There was an error in saving your character record. Your previous character record is still saved. Try turning your phone off then on again to clear the memory."
         );
      c("Before NPCHelloUI");
      this.aq = new h(this, 4, 8);
      this.aq.a("NPC name here", "NPC text here", true);
      this.m = new h(this, 4, 360);
      this.m.a("Rumors", "Rumors text here", true);
      c("After NPCHelloUI");
      this.a("After helloUI", true);
      this.R = new h[6];

      for (int var8 = 0; var8 < 4; var8++) {
         this.R[var8] = new h(this, 5, 9 + var8);
         String[] var9 = new String[]{"Train", "Give", "Befriend", "Threaten", "Kill"};
         this.R[var8].a("Name", "Aid: <TAG>", var9, null);
         this.R[var8].s = this.av;
      }

      this.R[4] = new h(this, 5, 13);
      String[] var14 = new String[]{"Give Item", "Take Crystal"};
      this.R[4].a("Beneca", "Aid: <TAG>", var14, null);
      this.R[4].s = this.av;
      this.R[5] = new h(this, 5, 14);
      String[] var10 = new String[]{"Rumors", "Give Crystal", "Enchant", "Bless", "Cure", "Warp", "Recovery"};
      this.R[5].a("Helga", "Aid: <TAG>", var10, null);
      this.R[5].s = this.av;
      this.a("After choicesUI", true);
      this.ap = new h(this, 4, 23);
      this.ap.a("Oracle", "NPC text here", true);
      this.t = new h(this, 3, 31);
      String[] var11 = new String[]{"Stats", "Inventory", "Skills", "Spells", "Save Game", "Load Game", "Help", "Quit Game"};
      this.t.a("Options", var11, null, false);
      this.t.a(h.z);
      this.a("After OptionsUI", true);
      this.ay.m = 42;
      this.aD = new Form("Enter name");
      StringItem var12 = new StringItem(null, "Enter a name for your character");
      this.aD.append(var12);
      TextField var13 = new TextField(null, null, 10, 0);
      this.aD.append(var13);
      this.aD.addCommand(h.I);
      this.aD.addCommand(h.P);
      this.aD.setCommandListener(this);
      this.ba = new h(this, 4, 305);
      this.ba.a("Unavailable", "No game is available for loading. Press OK to return to main menu.");
      this.ba.a(h.I);
      this.ba.a(this);
      this.ba.s = this.U;
      this.a("After NoSavedGameUI", true);
      this.ay.c = this.U;
      this.az.s = this.U;
      this.y.s = this.az;
      this.h.s = this.y;
      this.h.c = this.y;
      this.aE.s = this.y;
      this.f = new h(this, 4, 399);
      this.f.n();
      this.ay.m = 55;
      c("End of loading UI and images");
      this.a("End of allocateAllUIs", true);
   }

   private h a(int var1) {
      System.gc();
      h var2 = new h(this, 5, 22);
      var2.N = var1;
      String[] var3 = new String[this.k.p];

      for (int var4 = 0; var4 < this.k.p; var4++) {
         int var5 = Math.abs(this.k.H[var4]);
         if (this.k.C(var4)) {
            var3[var4] = "E:" + a.d(var5);
         } else {
            var3[var4] = a.d(var5);
         }
      }

      var2.a(k.s[var1], "Give What?", var3, null, true);
      var2.s = this.av;
      return var2;
   }

   private h b(int var1) {
      System.gc();
      c("Start of newTrainWhat");
      h var2 = new h(this, 5, 20);
      var2.N = var1;
      String[] var3 = new String[3];
      int var4 = 0;

      for (int var5 = 0; var5 < 14; var5++) {
         int var6 = this.k.b(var5, false);
         String var7 = j.E[var5] + " (<TAG>)";
         if (k.c(var1, var5)) {
            var3[var4++] = f.a(var7, "<TAG>", var6);
         }
      }

      var2.a(k.s[var1], "Train What?", var3, null, true);
      var2.s = this.av;
      return var2;
   }

   private h p(int var1) {
      System.gc();
      c("Start of newTakeWhat");
      h var2 = new h(this, 5, 27);
      var2.N = var1;
      String[] var3 = a.b();
      var2.a(k.s[var1], "Take What?", var3, null, true);
      var2.s = null;
      return var2;
   }

   private h c(int var1) {
      System.gc();
      c("Start of newEnchantWhat");
      h var2 = new h(this, 5, 350);
      var2.N = var1;
      String[] var3 = new String[this.k.p];

      for (int var4 = 0; var4 < this.k.p; var4++) {
         int var5 = Math.abs(this.k.H[var4]);
         var3[var4] = a.d(var5);
      }

      var2.a(k.s[var1], "Enchant What?", var3, null, true);
      var2.s = this.av;
      return var2;
   }

   private h D() {
      h var1 = new h(this, 4, 32);
      var1.a("Stats", this.av.ax.j());
      return var1;
   }

   private h c() {
      System.gc();
      h var1 = new h(this, 5, 33);
      String[] var2 = new String[this.k.p];

      for (int var3 = 0; var3 < this.k.p; var3++) {
         byte var4 = this.k.H[var3];
         System.out.println("itemid is " + var4);
         if (var4 < 0) {
            var2[var3] = "E: " + a.d(Math.abs(var4));
            System.out.println("item is " + var2[var3]);
         } else {
            var2[var3] = a.d(var4);
            System.out.println("item is " + var2[var3]);
         }
      }

      var1.a("Inventory", "Items:", var2, null, true);
      var1.s = this.t;
      return var1;
   }

   private h c(h var1) {
      System.gc();
      h var2 = new h(this, 5, 202);
      String[] var3 = new String[]{"Yes", "No"};
      var2.a("Quit?", "Are you sure?", var3, null, true);
      var2.s = var1;
      return var2;
   }

   private h b(h var1) {
      System.gc();
      h var2 = new h(this, 3, 203);
      var2.a("Help", as, null);
      var2.s = var1;
      return var2;
   }

   private h g(int var1) {
      System.gc();
      h var2 = new h(this, 4, 206);
      var2.a(as[var1], o[var1], true);
      var2.s = this.D;
      return var2;
   }

   private h a(h var1) {
      System.gc();
      h var2 = new h(this, 4, 204);
      var2.a("Credits", n, true);
      var2.s = var1;
      return var2;
   }

   public void pauseApp() {
      this.av.e();
      aT = 3;
   }

   public void destroyApp(boolean var1) {
      aT = 4;
   }

   public void commandAction(Command var1, Displayable var2) {
      if (ax != null) {
         if (var1 == h.P && ax.s != null) {
            this.a(ax.s);
            return;
         }

         if (ax.B == 2) {
            if (var1 == h.u) {
               int var3 = ax.a();
               String[] var4 = ax.r();
               switch (var3) {
                  case 0:
                     System.gc();
                     this.aZ = new h(this, 8, 301);
                     this.aZ.o();
                     Thread var5 = new Thread(this);
                     J = 4;
                     var5.start();
                     this.a((Object)this.aZ);
                     break;
                  case 1:
                     System.gc();
                     this.av.g();
                     aQ = new h(this, 9, 302);
                     aQ.o();
                     Thread var6 = new Thread(this);
                     J = 6;
                     var6.start();
                     this.a((Object)aQ);
                     break;
                  case 2:
                     this.D = this.b(ax);
                     this.a((Object)this.D);
                     break;
                  case 3:
                     this.aj = this.a(ax);
                     this.a((Object)this.aj);
                     break;
                  case 4:
                     this.C = this.c(ax);
                     this.a((Object)this.C);
               }
            }
         } else if (ax.B == 3) {
            if (var1 == h.u) {
               int var7 = ax.a();
               String[] var30 = ax.r();
               this.e = null;
               System.gc();
               this.e = new j(this);
               this.e.c(var7);
               this.e.d(var7);
               this.y.a(1, var30[var7]);
               this.a((Object)this.y);
            }
         } else if (ax.B == 4) {
            if (var1 == h.u) {
               int var8 = ax.a();
               String[] var31 = ax.r();
               if (var8 == 0) {
                  String var40 = this.e.m();
                  this.h.a(0, var40);
                  this.h.w = 0;
                  this.a((Object)this.h);
               } else {
                  this.k = this.e;
                  this.a((Object)this.aE);
               }
            }
         } else if (ax.B == 5) {
            if (var1 == h.I) {
               this.a(ax.c);
            }
         } else if (ax.B == 6) {
            if (var1 == h.u) {
               this.a(this.aD);
            }
         } else if (ax.B == 7) {
            this.aR = new h(this, 4, 101);
            this.aR.a("Introduction", k.k[7][3], true);
            this.a((Object)this.aR);
         } else if (ax.B == 101) {
            if (var1 == h.I) {
               this.av.ax = this.k;
               this.av.i();
               this.a(this.av);
            }
         } else if (ax.B != 8 && ax.B != 360) {
            if (ax.B >= 9 && ax.B <= 14) {
               if (var1 == h.P) {
                  this.a(ax.s);
               } else {
                  this.d(ax);
               }
            } else if (ax.B == 20) {
               if (var1 == h.u) {
                  int var10 = ax.N;
                  int var32 = ax.a();
                  int var41 = k.b(var10, var32);
                  this.ae = this.a(ax, var10, 21, 5, var41);
                  this.a((Object)this.ae);
               } else if (var1 == h.P) {
                  int var11 = ax.N;
                  this.k(var11);
                  this.a((Object)this.R[var11]);
               }
            } else if (ax.B == 22) {
               if (var1 == h.u) {
                  System.out.println("Found give what select");
                  int var12 = ax.N;
                  int var33 = ax.a();
                  if (var33 >= 0) {
                     this.ap = this.a(ax, var12, 23, 4, var33);
                     this.a((Object)this.ap);
                  }
               } else if (var1 == h.P) {
                  int var13 = ax.N;
                  this.k(var13);
                  this.a((Object)this.R[var13]);
               }
            } else if (ax.B == 27) {
               if (var1 == h.u) {
                  int var14 = ax.N;
                  int var34 = ax.a() + 87;
                  this.aJ = this.a(ax, var14, 28, 7, var34);
                  this.a((Object)this.aJ);
               } else if (var1 == h.P) {
                  int var15 = ax.N;
                  this.k(var15);
                  this.a((Object)this.R[var15]);
               }
            } else if (ax.B == 350) {
               if (var1 == h.u) {
                  int var16 = ax.N;
                  int var35 = ax.a();
                  if (var35 >= 0) {
                     this.aI = this.a(ax, var16, 351, 8, var35);
                     this.a((Object)this.aI);
                  }
               } else if (var1 == h.P) {
                  int var17 = ax.N;
                  this.k(var17);
                  this.a((Object)this.R[var17]);
               }
            } else if (ax.B != 23 && ax.B != 21 && ax.B != 24 && ax.B != 25 && ax.B != 28) {
               if (ax.B == 26) {
                  this.a(this.av);
               } else if (ax.B != 351 && ax.B != 352 && ax.B != 353 && ax.B != 355) {
                  if (ax.B == 30) {
                     if (var1 == h.I) {
                        this.a(this.av);
                     }
                  } else if (ax.B == 41) {
                     if (var1 == h.I) {
                        this.k.Q = false;
                        this.a(this.av);
                     }
                  } else if (ax.B == 31) {
                     if (var1 == h.u) {
                        int var20 = ax.a();
                        String[] var36 = ax.r();
                        switch (var20) {
                           case 0:
                              this.B = this.D();
                              this.a((Object)this.B);
                              break;
                           case 1:
                              this.aY = this.c();
                              this.a((Object)this.aY);
                              break;
                           case 2:
                              this.L = this.d();
                              this.a((Object)this.L);
                              break;
                           case 3:
                              this.v = this.r();
                              this.a((Object)this.v);
                              break;
                           case 4:
                              al = new h(this, 10, 303);
                              al.o();
                              Thread var42 = new Thread(this);
                              J = 5;
                              var42.start();
                              this.a((Object)al);
                              break;
                           case 5:
                              System.gc();
                              this.av.g();
                              aQ = new h(this, 9, 302);
                              aQ.o();
                              Thread var46 = new Thread(this);
                              J = 6;
                              var46.start();
                              this.a((Object)aQ);
                              break;
                           case 6:
                              c("Help");
                              this.D = this.b(ax);
                              this.a((Object)this.D);
                              break;
                           case 7:
                              this.C = this.c(ax);
                              this.a((Object)this.C);
                              break;
                           case 8:
                              this.g();
                              this.a(this.ao);
                        }
                     } else if (var1 == h.z) {
                        this.a(this.av);
                     }
                  } else if (ax.B == 32) {
                     if (var1 == h.I) {
                        this.a((Object)this.t);
                     }
                  } else if (ax.B == 33) {
                     if (var1 == h.u) {
                        int var21 = ax.a();
                        if (var21 >= 0) {
                           this.aB = this.l(var21);
                           this.Y = var21;
                           this.a((Object)this.aB);
                        }
                     }
                  } else if (ax.B == 34) {
                     if (var1 == h.u) {
                        int var22 = ax.a();
                        Integer var37 = (Integer)ax.n.elementAt(var22);
                        int var43 = var37;
                        if (var43 == 0) {
                           this.k.i(this.Y);
                           this.aY = this.c();
                           this.a((Object)this.aY);
                        } else if (var43 == 1) {
                           if (!this.k.C(this.Y)) {
                              this.k.d(this.Y, true);
                           } else {
                              this.k.A(this.Y);
                           }

                           this.aY = this.c();
                           this.a((Object)this.aY);
                        } else if (var43 == 2) {
                           this.k.r(this.Y);
                           this.aY = this.c();
                           this.a((Object)this.aY);
                        } else if (var43 == 3) {
                           this.k.a(this.Y);
                           if (this.k.Q) {
                              this.k.Q = false;
                              this.a(this.av);
                           } else {
                              this.aY = this.c();
                              this.a((Object)this.aY);
                           }
                        }

                        this.Y = -1;
                     }
                  } else if (ax.B == 35) {
                     if (var1 == h.u) {
                        int var23 = ax.a();
                        this.N = this.o(var23);
                        this.a((Object)this.N);
                     }
                  } else if (ax.B == 36) {
                     if (var1 == h.I) {
                        this.a((Object)this.L);
                     }
                  } else if (ax.B == 37) {
                     if (var1 == h.u) {
                        int var24 = ax.a();
                        if (var24 >= 0) {
                           this.q = this.e(var24);
                           this.g = var24;
                           this.a((Object)this.q);
                        }
                     }
                  } else if (ax.B == 38) {
                     if (var1 == h.u) {
                        int var25 = this.k.B(this.g);
                        this.k.b = (byte)(var25 + 1);
                        this.v = this.r();
                        this.a((Object)this.v);
                        this.g = -1;
                     }
                  } else if (ax.B == 39) {
                     if (var1 == h.u) {
                        String var26 = ax.p();
                        l[ax.N] = -1;

                        for (int var38 = 0; var38 < j.y.length; var38++) {
                           if (var26.equals(j.y[var38])) {
                              l[ax.N] = var38;
                              break;
                           }
                        }

                        if (ax.N < 2) {
                           int var44 = ax.N + 1;
                           this.ag = this.j(var44 + 1);
                           this.a((Object)this.ag);
                        } else {
                           this.k.J[l[0]] = (short)(this.k.J[l[0]] + 3);
                           this.k.J[l[1]] = (short)(this.k.J[l[1]] + 2);
                           this.k.J[l[2]]++;
                           this.k.g();
                           this.k.d();
                           this.a(this.av);
                           this.av.b();
                        }
                     }
                  } else if (ax.B == 202) {
                     if (var1 == h.u) {
                        int var27 = ax.a();
                        if (var27 == 0) {
                           this.a((Object)this.f);
                        } else {
                           this.a(ax.s);
                        }
                     }
                  } else if (ax.B == 202) {
                     this.b();
                  } else if (ax.B == 40) {
                     this.a(this.av);
                     this.av.b();
                  } else if (ax.B == 102) {
                     this.a(this.av);
                     this.av.b();
                  } else if (ax.B == 203) {
                     if (var1 == h.u) {
                        int var28 = ax.a();
                        this.F = this.g(var28);
                        this.a((Object)this.F);
                     } else {
                        this.a(ax.s);
                     }
                  } else if (ax.B == 206) {
                     this.a(ax.s);
                  } else if (ax.B == 204) {
                     this.a(ax.s);
                  } else if (ax.B == 305) {
                     this.a(ax.s);
                  } else if (ax.B == 205) {
                     this.a(ax.s);
                  } else if (ax.B == 200 || ax.B == 201) {
                     this.a(ax.c);
                  } else if (ax.B == 399) {
                     this.b();
                  } else if (ax.B == 499) {
                     this.b();
                  }
               } else if (var1 == h.I) {
                  int var19 = ax.N;
                  this.k(var19);
                  this.a((Object)this.R[var19]);
               }
            } else if (var1 == h.I) {
               int var18 = ax.N;
               this.k(var18);
               this.a((Object)this.R[var18]);
            }
         } else if (var1 == h.I) {
            if (ax.c == null) {
               System.out.println("ERROR: next is null!");
            } else {
               h var9 = (h)ax.c;
               if (var9 == null) {
                  System.out.println("uic.next is null!");
               }
            }

            this.a(ax.c);
         }
      } else if (var2 == this.w) {
         this.b();
      } else if (var2 == this.ao) {
         this.a(this.av);
      } else if (var2 == this.W) {
         if (var1 == aS) {
            this.b();
         }
      } else if (var2 == this.aD) {
         if (var1 == h.I) {
            TextField var29 = (TextField)this.aD.get(1);
            String var39 = var29.getString();
            if (var39.length() < 3) {
               Alert var45 = new Alert("Error", f.a("Your character name must be at least <TAG> letters", "<TAG>", 3), null, AlertType.ERROR);
               var45.setTimeout(-2);
               this.a(var45);
            } else {
               this.k.v = var39;
               this.a((Object)this.M);
            }
         } else if (var1 == h.P) {
            this.a((Object)this.aE);
         }
      }
   }

   private void d(h var1) {
      int var3 = var1.a();
      int var4 = var1.B - 9;
      switch (var4) {
         case 0:
         case 1:
         case 2:
         case 3:
            if (var3 == 0) {
               this.z = this.b(var4);
               this.a((Object)this.z);
            } else if (var3 == 1) {
               if (this.k.p <= 0) {
                  this.ap.a(k.s[var4]);
                  this.ap.e("You have nothing to give me!");
                  this.a((Object)this.ap);
               } else {
                  this.Q = this.a(var4);
                  this.a((Object)this.Q);
               }
            } else if (var3 == 2) {
               this.X = this.a(var1, var4, 24, 2, 0);
               this.a((Object)this.X);
            } else if (var3 == 3) {
               this.V = this.a(var1, var4, 25, 3, 0);
               this.a((Object)this.V);
            } else if (var3 == 4) {
               this.an = this.a(var1, var4, 26, 6, 0);
               this.a((Object)this.an);
            }
            break;
         case 4:
            if (var3 == 0) {
               if (this.k.p <= 0) {
                  this.ap.a(k.s[var4]);
                  this.ap.e("You have nothing to give me!");
                  this.a((Object)this.ap);
               } else {
                  this.Q = this.a(var4);
                  this.a((Object)this.Q);
               }
            } else if (var3 == 1) {
               this.I = this.p(var4);
               this.a((Object)this.I);
            }
            break;
         case 5:
            if (var3 == 0) {
               this.x();
            } else if (var3 == 1) {
               if (this.k.p <= 0) {
                  this.ap.a(k.s[var4]);
                  this.ap.e("You have nothing to give me!");
                  this.a((Object)this.ap);
               } else {
                  this.Q = this.a(var4);
                  this.a((Object)this.Q);
               }
            } else if (var3 == 2) {
               this.aw = this.c(var4);
               this.a((Object)this.aw);
            } else if (var3 == 3) {
               this.aL = this.a(var1, var4, 352, 9, 0);
               this.a((Object)this.aL);
            } else if (var3 == 4) {
               this.H = this.a(var1, var4, 353, 10, 0);
               this.a((Object)this.H);
            } else if (var3 == 5) {
               this.K = this.a(var1, var4, 41, 11, 0);
               this.a((Object)this.K);
            } else if (var3 == 6) {
               this.s = this.a(var1, var4, 355, 12, 0);
               this.a((Object)this.s);
            }
      }
   }

   private h a(h var1, int var2, int var3, int var4, int var5) {
      h var6 = new h(this, 4, var3);
      var6.a("NPC name here", "NPC text here", true);
      String var7 = k.a(this.k, var2, var4, var5);
      var6.a(k.s[var2]);
      var6.e(var7);
      var6.N = var2;
      return var6;
   }

   private Image d(String var1) throws Exception {
      return !var1.startsWith("/") ? Image.createImage("/" + var1) : Image.createImage(var1);
   }

   private boolean w() {
      boolean var1 = true;
      boolean var2 = false;
      RecordStore var3 = null;
      aQ.m = 0;
      String var4 = this.M();

      try {
         if (var4 == null) {
            throw new Exception("No valid record store!");
         }

         var3 = RecordStore.openRecordStore(var4, false);
         int var5 = var3.getNumRecords();
         byte[] var6 = var3.getRecord(1);
         this.k = j.a(var6, true);
         this.k.ai = this;
         aQ.m = 20;
         aQ.c();
         aQ.f();
         int var7 = a(var3, 2);
         System.out.println("Read the master lists from RecordStore");
         var6 = var3.getRecord(var7);
         b(var6);
      } catch (Exception var17) {
         System.out.println("Exception in loadGameState");
         System.out.println(var17);
         var1 = false;
      } finally {
         if (var3 != null) {
            try {
               var3.closeRecordStore();
            } catch (Exception var16) {
            }
         }
      }

      return var1;
   }

   private boolean l() {
      boolean var1 = true;
      boolean var2 = false;
      RecordStore var3 = null;
      al.m = 0;
      String var4 = this.f();

      try {
         var3 = RecordStore.openRecordStore(var4, true);
         byte[] var5 = this.k.g(true);
         al.m = 20;
         al.c();
         al.f();
         var3.addRecord(var5, 0, var5.length);
         System.gc();
         a(var3);
         var5 = k();
         var3.addRecord(var5, 0, var5.length);
         Object var21 = null;
         System.gc();
         var3.closeRecordStore();
         var3 = null;
         this.L();
         al.m = 100;
         al.c();
         al.f();
      } catch (Throwable var18) {
         System.out.println("Exception in saveGameState");
         System.out.println(var18);

         try {
            RecordStore.deleteRecordStore(var4);
         } catch (Exception var17) {
         }

         var1 = false;
      } finally {
         if (var3 != null) {
            try {
               var3.closeRecordStore();
            } catch (Exception var16) {
            }
         }
      }

      return var1;
   }

   private void y() {
      this.w = new Form("Error");
      this.ad = new StringItem("Error: ", "Cannot load game");
      this.w.append(this.ad);
      Command var1 = new Command("Ok", 4, 1);
      this.w.addCommand(var1);
      this.w.setCommandListener(this);
   }

   private void a(String var1, boolean var2) {
      this.b(var1);
      boolean var3 = false;
      if (var3) {
         String var4 = this.ad.getText();
         var4 = var4 + "\n" + f(var1);
         this.ad.setText(var4);
      } else {
         this.ad.setText(f(var1));
      }
   }

   void B() {
      this.a("Out of memory", true);
      this.d.setCurrent(this.w);
   }

   static int d(int var0) {
      System.out.println("In getGameAdvancementLevel, giftPoints = " + var0);
      if (var0 < 9) {
         return 0;
      } else if (var0 < 13) {
         return 1;
      } else if (var0 < 17) {
         return 2;
      } else if (var0 < 23) {
         return 3;
      } else if (var0 < 28) {
         return 4;
      } else if (var0 < 34) {
         return 5;
      } else if (var0 < 40) {
         return 6;
      } else {
         return var0 < 48 ? 7 : 8;
      }
   }

   void n(int var1) {
      System.out.println("In checkOpenAndPopulateDungeons, gameAdvLevel = " + var1);
      int[] var2 = x[var1];
      int var3 = var2.length;

      for (int var4 = 0; var4 < var3; var4++) {
         int var5 = x[var1][var4];
         int var6 = var5 - 1;
         if (!u[var6].k) {
            u[var6].k = true;
            u[var6].e();
         }

         if (this.aZ != null) {
            this.aZ.m = 100 * (var4 + 1) / var3;
            if (this.aZ.m > 100) {
               this.aZ.m = 100;
            }

            this.aZ.c();
            this.aZ.f();
         }
      }
   }

   static void i(int var0) {
      int var1 = 0;
      int var2 = 0;

      for (int var3 = 0; var3 <= var0; var3++) {
         int[] var4 = x[var3];
         int var5 = var4.length;
         var2 += var5;
      }

      for (int var10 = 0; var10 <= var0; var10++) {
         int[] var11 = x[var10];
         int var6 = var11.length;

         for (int var7 = 0; var7 < var6; var7++) {
            int var8 = x[var10][var7];
            int var9 = var8 - 1;
            u[var9].k = true;
            u[var9].h();
            var1++;
            aQ.m = 100 * var1 / var2;
            if (aQ.m > 100) {
               aQ.m = 100;
            }
         }
      }
   }

   static void i() throws Exception {
      E();
   }

   private static void E() {
      af = 19;
      aO = 19;
      i = new byte[aO][af];

      for (int var0 = 0; var0 < aO; var0++) {
         for (int var1 = 0; var1 < af; var1++) {
            i[var0][var1] = 1;
         }
      }

      for (int var5 = 0; var5 < af; var5++) {
         i[var5][9] = 0;
      }

      for (int var2 = 0; var2 < aO; var2++) {
         i[9][var2] = 0;
      }

      for (int var3 = 0; var3 < 3; var3++) {
         for (int var4 = 0; var4 < 3; var4++) {
            i[8 + var3][8 + var4] = 0;
         }
      }

      i[4][8] = 0;
      i[4][7] = 0;
      i[3][7] = 0;
      i[16][8] = 0;
      i[16][7] = 0;
      i[15][7] = 0;
      i[8][3] = 0;
      i[7][3] = 0;
      i[7][2] = 0;
      i[10][4] = 0;
      i[11][4] = 0;
      i[12][4] = 0;
      i[12][3] = 0;
      i[6][14] = 0;
      i[7][14] = 0;
      i[8][14] = 0;
      i[9][14] = 0;
      i[10][14] = 0;
      i[11][14] = 0;
      i[12][14] = 0;
      i[6][13] = 0;
      i[12][13] = 0;
   }

   private static void K() {
      G = new Hashtable[37];

      for (int var0 = 1; var0 < 37; var0++) {
         G[var0] = new Hashtable();
      }

      S = new Hashtable[37];

      for (int var1 = 1; var1 < 37; var1++) {
         S[var1] = new Hashtable();
      }

      au = new Vector[37];

      for (int var2 = 0; var2 < 37; var2++) {
         au[var2] = new Vector();
      }
   }

   private static void b(byte[] var0) throws Exception {
      DataInputStream var1 = new DataInputStream(new ByteArrayInputStream(var0, 0, var0.length));
      a.i = var1.readShort();
      d.j = var1.readShort();

      for (int var2 = 0; var2 < 7; var2++) {
         k.b[var2] = var1.readBoolean();
      }

      for (int var3 = 0; var3 < 7; var3++) {
         k.q[var3] = var1.readBoolean();
      }

      for (int var4 = 0; var4 < 4; var4++) {
         k.r[var4] = var1.readShort();
      }

      for (int var5 = 0; var5 < 4; var5++) {
         k.p[var5] = var1.readShort();
      }

      for (int var6 = 0; var6 < 4; var6++) {
         k.h[var6] = var1.readShort();
      }

      for (int var7 = 0; var7 < 4; var7++) {
         k.c[var7] = var1.readByte();
      }

      for (int var8 = 0; var8 < 4; var8++) {
         k.n[var8] = var1.readByte();
      }

      k.f = var1.readByte();
      k.d = var1.readBoolean();
      k.a = var1.readShort();
      k.g = var1.readShort();
      k.l = var1.readBoolean();

      for (int var9 = 0; var9 < 7; var9++) {
         if (!k.b[var9]) {
            i var10 = u[0];
            var10.w[k.j[var9]][k.i[var9]] = f.c((byte)32, var10.w[k.j[var9]][k.i[var9]]);
         }
      }
   }

   private static byte[] k() throws Exception {
      ByteArrayOutputStream var0 = new ByteArrayOutputStream(60);
      DataOutputStream var1 = new DataOutputStream(var0);
      var1.writeShort(a.i);
      var1.writeShort(d.j);

      for (int var2 = 0; var2 < 7; var2++) {
         var1.writeBoolean(k.b[var2]);
      }

      for (int var3 = 0; var3 < 7; var3++) {
         var1.writeBoolean(k.q[var3]);
      }

      for (int var4 = 0; var4 < 4; var4++) {
         var1.writeShort(k.r[var4]);
      }

      for (int var5 = 0; var5 < 4; var5++) {
         var1.writeShort(k.p[var5]);
      }

      for (int var6 = 0; var6 < 4; var6++) {
         var1.writeShort(k.h[var6]);
      }

      for (int var7 = 0; var7 < 4; var7++) {
         var1.writeByte(k.c[var7]);
      }

      for (int var8 = 0; var8 < 4; var8++) {
         var1.writeByte(k.n[var8]);
      }

      var1.writeByte(k.f);
      var1.writeBoolean(k.d);
      var1.writeShort(k.a);
      var1.writeShort(k.g);
      var1.writeBoolean(k.l);
      return var0.toByteArray();
   }

   private static void a(RecordStore var0) throws Exception {
      for (int var2 = 1; var2 < 37; var2++) {
         int var3 = G[var2].size();
         int var1 = 4 + var3 * 28;
         ByteArrayOutputStream var4 = new ByteArrayOutputStream(var1);
         DataOutputStream var5 = new DataOutputStream(var4);
         var5.writeInt(var3);
         Enumeration var6 = G[var2].elements();

         while (var6.hasMoreElements()) {
            byte[] var7 = (byte[])var6.nextElement();
            d var8 = d.a(var7);
            var8.a(var5);
         }

         byte[] var25 = var4.toByteArray();
         var0.addRecord(var25, 0, var25.length);

         try {
            var5.close();
         } catch (Exception var13) {
         }

         Object var19 = null;
         Object var26 = null;
         System.gc();
         al.m = 20 + 30 * (var2 + 1) / 37;
         al.c();
         al.f();
      }

      for (int var16 = 1; var16 < 37; var16++) {
         int var17 = S[var16].size();
         int var14 = 4 + var17 * 8;
         ByteArrayOutputStream var20 = new ByteArrayOutputStream(var14);
         DataOutputStream var22 = new DataOutputStream(var20);
         var22.writeInt(var17);
         Enumeration var27 = S[var16].elements();

         while (var27.hasMoreElements()) {
            byte[] var30 = (byte[])var27.nextElement();
            a(var22, var30, 8);
         }

         byte[] var31 = var20.toByteArray();
         var0.addRecord(var31, 0, var31.length);

         try {
            var22.close();
         } catch (Exception var12) {
         }

         Object var23 = null;
         Object var32 = null;
         System.gc();
         al.m = 50 + 30 * (var16 + 1) / 37;
         al.c();
         al.f();
      }

      for (int var18 = 0; var18 < 37; var18++) {
         int var21 = au[var18].size();
         int var15 = 4 + var21 * 7;
         ByteArrayOutputStream var24 = new ByteArrayOutputStream(var15);
         DataOutputStream var28 = new DataOutputStream(var24);
         var28.writeInt(var21);
         Enumeration var33 = au[var18].elements();

         while (var33.hasMoreElements()) {
            byte[] var9 = (byte[])var33.nextElement();
            a(var28, var9, 7);
         }

         byte[] var34 = var24.toByteArray();
         var0.addRecord(var34, 0, var34.length);

         try {
            var28.close();
         } catch (Exception var11) {
         }

         Object var29 = null;
         Object var35 = null;
         System.gc();
         al.m = 80 + 19 * (var18 + 1) / 37;
         al.c();
         al.f();
      }
   }

   private static int a(RecordStore var0, int var1) throws Exception {
      int var2 = var1;

      for (int var3 = 1; var3 < 37; var3++) {
         byte[] var4 = var0.getRecord(var2++);
         DataInputStream var5 = new DataInputStream(new ByteArrayInputStream(var4, 0, var4.length));
         G[var3].clear();
         int var6 = var5.readInt();

         for (int var7 = 0; var7 < var6; var7++) {
            d var8 = d.a(var5);
            String var9 = String.valueOf(var8.a);
            G[var3].put(var9, var8.f());
         }

         try {
            var5.close();
         } catch (Exception var13) {
         }

         Object var16 = null;
         Object var14 = null;
         System.gc();
         aQ.m = 20 + 30 * (var3 + 1) / 37;
         aQ.c();
         aQ.f();
      }

      for (int var15 = 1; var15 < 37; var15++) {
         byte[] var17 = var0.getRecord(var2++);
         DataInputStream var20 = new DataInputStream(new ByteArrayInputStream(var17, 0, var17.length));
         S[var15].clear();
         int var24 = var20.readInt();

         for (int var27 = 0; var27 < var24; var27++) {
            byte[] var29 = a(var20, 8);
            String var10 = f.b((int)var29[0], (int)var29[1]);
            S[var15].put(var10, var29);
         }

         try {
            var20.close();
         } catch (Exception var12) {
         }

         Object var21 = null;
         Object var18 = null;
         System.gc();
         aQ.m = 50 + 30 * (var15 + 1) / 37;
         aQ.c();
         aQ.f();
      }

      for (int var19 = 0; var19 < 37; var19++) {
         byte[] var22 = var0.getRecord(var2++);
         DataInputStream var25 = new DataInputStream(new ByteArrayInputStream(var22, 0, var22.length));
         au[var19].removeAllElements();
         int var28 = var25.readInt();

         for (int var30 = 0; var30 < var28; var30++) {
            byte[] var31 = a(var25, 7);
            au[var19].addElement(var31);
         }

         try {
            var25.close();
         } catch (Exception var11) {
         }

         Object var26 = null;
         Object var23 = null;
         System.gc();
         aQ.m = 80 + 19 * (var19 + 1) / 37;
         aQ.c();
         aQ.f();
      }

      return var2;
   }

   private static byte[] a(DataInputStream var0, int var1) throws Exception {
      byte[] var2 = new byte[var1];

      for (int var3 = 0; var3 < var1; var3++) {
         var2[var3] = var0.readByte();
      }

      return var2;
   }

   private static void a(DataOutputStream var0, byte[] var1, int var2) throws Exception {
      for (int var3 = 0; var3 < var2; var3++) {
         var0.writeByte(var1[var3]);
      }
   }

   public static void c(String var0) {
   }

   static String f(String var0) {
      if (var0 == null) {
         var0 = "";
      }

      Runtime var1 = Runtime.getRuntime();
      long var2 = var1.freeMemory();
      long var4 = var1.totalMemory();
      return ">>> MEMORY: " + var0 + ": Free memory is " + var2 + ", Total memory is " + var4;
   }

   private void h() {
      this.W = new Form("Done");
      StringItem var1 = new StringItem(null, "I am done");
      this.W.append(var1);
      this.W.addCommand(aS);
      this.W.setCommandListener(this);
   }

   private void p() {
      byte[] var1 = new byte[5];
      System.out.println("LOADING WARDEN IMAGES");
      var1[0] = 1;
      var1[1] = 1;
      var1[2] = 1;
      var1[3] = 1;
      var1[4] = 1;
      this.a = 1;
      this.a(var1);
   }

   void s() {
      System.out.println("Running image loader thread for new dungeon ID: " + this.a);
      this.am = false;
      if (j) {
         aQ.m = 80;
      } else {
         this.ab.m = 0;
      }

      byte[] var1 = new byte[5];

      for (int var2 = 0; var2 < 5; var2++) {
         var1[var2] = 0;
      }

      if (this.a == 1) {
         this.z();
         this.p();
      } else {
         Hashtable var3 = G[this.a - 1];
         if (var3 == null) {
            return;
         }

         Enumeration var4 = var3.elements();

         while (var4.hasMoreElements()) {
            byte[] var5 = (byte[])var4.nextElement();
            d var6 = d.a(var5);
            if (var6.l >= 1 && var6.l <= 5) {
               var1[0]++;
            } else if (var6.l >= 6 && var6.l <= 10) {
               var1[1]++;
            } else if (var6.l >= 11 && var6.l <= 25) {
               var1[2]++;
            } else if (var6.l >= 26 && var6.l <= 40) {
               var1[3]++;
            } else {
               var1[4]++;
            }
         }

         this.z();
         this.a(var1);
      }

      e.E = true;
   }

   private boolean m(int var1) {
      switch (var1) {
         case 4:
         case 11:
         case 18:
         case 23:
         case 30:
            return true;
         default:
            return false;
      }
   }

   void a(byte[] var1) {
      this.am = false;
      this.a("Inside runImageLoader", true);

      try {
         for (int var2 = 0; var2 < 5; var2++) {
            if (var1[var2] > 0) {
               this.a("Handling ichunk = " + var2, true);
               int var3 = ah[var2][0];
               int var4 = ah[var2][1];

               for (int var5 = 0; var5 < var4; var5++) {
                  int var6 = var3 + var5;
                  if (!this.m(var6)) {
                     e.q[var6] = g.b(T[var2][var5]);
                  }

                  if (!this.ak) {
                     return;
                  }

                  if (this.aW) {
                     this.aW = false;
                     return;
                  }
               }
            }

            if (j) {
               aQ.m = 80 + (var2 + 1) * 20 / 5;
               aQ.c();
               aQ.f();
            } else {
               this.ab.m = (var2 + 1) * 100 / 5;
               this.ab.c();
               this.ab.f();
            }
         }

         this.am = true;
         System.out.println("SUCCESSFULLY LOADED MONSTER IMAGES!!");
         this.ak = false;
      } catch (Throwable var7) {
         System.out.println("ERROR in image loader: " + var7);
         this.d.setCurrent(this.w);
      }
   }

   void z() {
      int var1 = e.q.length;

      for (int var2 = 0; var2 < var1; var2++) {
         if (e.q[var2] != null) {
            e.q[var2].g = null;
            e.q[var2] = null;
         }
      }

      System.gc();
      c("After unloading all monster images");
   }

   static void a(int var0, int var1) {
      System.out.println("In killMonster! dungid is " + var0);
      byte[] var2 = (byte[])G[var0 - 1].remove(String.valueOf(var1));
      byte var3 = var2[4];
      byte var4 = var2[5];
      i var5 = u[var0 - 1];
      if (var2 != null) {
         var5.w[var3][var4] = f.c((byte)2, var5.w[var3][var4]);
      }

      System.out.println("End of killMonster, size of HT is " + G[var0 - 1].size());
   }

   private h l(int var1) {
      h var2 = new h(this, 5, 34);
      System.out.println("In newInventoryItemUI: getting item " + var1);
      String var3 = this.k.b(var1);
      Vector var4 = new Vector();
      Vector var5 = new Vector();
      var4.addElement("Drop");
      var5.addElement(new Integer(0));
      if (this.k.w(var1)) {
         if (!this.k.C(var1)) {
            var4.addElement("Equip");
         } else {
            var4.addElement("Unequip");
         }

         var5.addElement(new Integer(1));
      }

      if (this.k.e(var1)) {
         var4.addElement("Learn");
         var5.addElement(new Integer(2));
      }

      if (this.k.v(var1)) {
         var4.addElement("Use");
         var5.addElement(new Integer(3));
      }

      String[] var6 = new String[var4.size()];

      for (int var7 = 0; var7 < var4.size(); var7++) {
         var6[var7] = (String)var4.elementAt(var7);
      }

      var2.a("Item", var3, var6, var5);
      var2.A = true;
      var2.s = this.aY;
      return var2;
   }

   private h d() {
      System.gc();
      c("Start of newSkillsListUI");
      h var1 = new h(this, 5, 35);
      Vector var2 = this.k.f();
      int var3 = var2.size();
      String[] var4 = new String[var3];

      for (int var5 = 0; var5 < var3; var5++) {
         var4[var5] = (String)var2.elementAt(var5);
      }

      var1.a("Skills", "Your Skills:", var4, null, true);
      var1.s = this.t;
      return var1;
   }

   private h o(int var1) {
      System.gc();
      c("Start of newSkillInfoUI");
      h var2 = new h(this, 4, 36);
      int var3 = this.k.l(var1);
      String var4 = this.k.m(var3);
      var2.a("Skill Info", var4);
      var2.s = this.L;
      return var2;
   }

   private h r() {
      System.gc();
      c("Start of newSpellsListUI");
      h var1 = new h(this, 5, 37);
      Vector var2 = this.k.J();
      int var3 = var2.size();
      String[] var4 = new String[var3];

      for (int var5 = 0; var5 < var3; var5++) {
         var4[var5] = (String)var2.elementAt(var5);
      }

      var1.a("Spells", "Your Spells:", var4, null, true);
      var1.s = this.t;
      return var1;
   }

   private h e(int var1) {
      System.gc();
      c("Start of newSpellInfoUI");
      h var2 = new h(this, 5, 38);
      int var3 = this.k.B(var1);
      String var4 = this.k.s(var3);
      String[] var5 = new String[]{"Ready Spell"};
      var2.a("Spell Info", var4, var5, null);
      var2.A = true;
      var2.s = this.v;
      return var2;
   }

   h j(int var1) {
      System.gc();
      c("Start of newLevelUpUI: index= " + var1);
      h var2 = new h(this, 5, 39);
      String[] var3 = this.k.q();
      String var4 = null;
      if (var1 == 1) {
         var2.N = 0;
         var4 = "Select an attribute to \nincrease 3 points:";
      } else if (var1 == 2) {
         var4 = "Select an attribute to \nincrease 2 points:";
         var2.N = 1;
      } else if (var1 == 3) {
         var2.N = 2;
         var4 = "Select an attribute to \nincrease 1 point:";
      }

      var2.a("Level Up", var4, var3, null);
      var2.t.removeCommand(h.P);
      var2.A = true;
      var2.s = var2;
      return var2;
   }

   h e(String var1) {
      System.gc();
      c("Start of newWardenSpeaksUI");
      h var2 = new h(this, 4, 102);
      var2.a("Varus", var1);
      return var2;
   }

   h F() {
      System.gc();
      c("Start of newEndOfGameUI");
      String var1 = k.k[7][4];
      h var2 = new h(this, 4, 200);
      var2.a("Victory!", var1);
      var2.c = this.v();
      return var2;
   }

   private h v() {
      System.gc();
      c("Start of newGameOverUI");
      String var1 = k.k[7][5];
      h var2 = new h(this, 4, 201);
      var2.a("Game Over", var1);
      var2.c = this.U;
      return var2;
   }

   static DataInputStream a(String var0) throws Exception {
      InputStream var1 = new Object().getClass().getResourceAsStream(f.b(var0));
      if (var1 == null) {
         return null;
      }

      byte[] var2 = f.a(var1.available(), var1);
      return new DataInputStream(new ByteArrayInputStream(var2));
   }

   static int h(int var0) {
      return Math.abs(P.nextInt() % var0);
   }

   static int f(int var0) {
      return 1 + Math.abs(P.nextInt() % var0);
   }

   static int a(Random var0, int var1) {
      return 1 + Math.abs(var0.nextInt() % var1);
   }

   void a(Object var1) {
      if (ax != null) {
         if (var1 instanceof h) {
            h var2 = (h)var1;
            if (ax != var2) {
               ax.q();
            }
         } else {
            ax.q();
         }
      }

      if (this.av != null) {
         this.av.e();
      }

      if (this.d == null) {
         this.d = Display.getDisplay(this);
      }

      if (var1 instanceof h) {
         ax = (h)var1;
         c var3 = h.j();
         var3.a = ax;
         ax.t = var3;
         this.d.setCurrent(ax.t);
         ax.h();
         ax.c();
         ax.f();
      } else if (var1 instanceof Displayable) {
         Displayable var4 = (Displayable)var1;
         ax = null;
         this.d.setCurrent(var4);
      }
   }

   private static void I() throws Exception {
      DataInputStream var0 = f.a("/geomin.dat");
      ai = new byte[37][6];

      for (int var1 = 0; var1 < 37; var1++) {
         for (int var2 = 0; var2 < 6; var2++) {
            ai[var1][var2] = var0.readByte();
         }
      }
   }

   private static void C() throws Exception {
      InputStream var0 = new Object().getClass().getResourceAsStream("/monsterfilenamesin.dat");
      byte[] var1 = f.a(var0.available(), var0);
      DataInputStream var2 = new DataInputStream(new ByteArrayInputStream(var1));
      T = new String[5][7];

      for (int var3 = 0; var3 < 5; var3++) {
         for (int var4 = 0; var4 < 7; var4++) {
            T[var3][var4] = var2.readUTF();
         }
      }
   }

   private void g() {
      this.ao = new Form("Debug");
      String var1 = this.av.ax.K();
      this.ar = new StringItem("Debug: ", var1);
      this.ao.append(this.ar);
      Command var2 = new Command("Ok", 4, 1);
      this.ao.addCommand(var2);
      this.ao.setCommandListener(this);
   }

   synchronized void b(String var1) {
      c = var1;
   }

   private void u() {
      as[0] = k.k[7][6];
      as[1] = k.k[7][8];
      as[2] = k.k[7][11];
      as[3] = k.k[7][13];
      as[4] = k.k[7][19];
      as[5] = k.k[7][21];
      as[6] = k.k[7][24];
      as[7] = k.k[7][29];
      as[8] = k.k[7][31];
      as[9] = k.k[7][34];
      as[10] = k.k[7][37];
      as[11] = k.k[7][39];
   }

   private void a() {
      StringBuffer var1 = new StringBuffer(1200);
      var1.append(k.k[7][7]);
      o[0] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][9]);
      var1.append(k.k[7][10]);
      o[1] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][12]);
      o[2] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][14]);
      var1.append(k.k[7][15]);
      var1.append(k.k[7][16]);
      var1.append(k.k[7][17]);
      var1.append(k.k[7][18]);
      o[3] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][20]);
      o[4] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][22]);
      var1.append(k.k[7][23]);
      o[5] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][25]);
      var1.append(k.k[7][26]);
      var1.append(k.k[7][27]);
      var1.append(k.k[7][28]);
      o[6] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][30]);
      o[7] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][32]);
      var1.append(k.k[7][33]);
      o[8] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][35]);
      var1.append(k.k[7][36]);
      o[9] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][38]);
      o[10] = var1.toString();
      var1.delete(0, 1200);
      var1.append(k.k[7][40]);
      o[11] = var1.toString();
      var1.delete(0, 1200);
   }

   private String q() {
      StringBuffer var1 = new StringBuffer(400);
      var1.append("Game Design: Anthony Gill and Greg Gorden");
      var1.append('\n');
      var1.append("Art: Mark Jones");
      var1.append('\n');
      var1.append("Programming: Marc Ilgen");
      var1.append('\n');
      var1.append("Technical Director: Andrew Friedman");
      var1.append('\n');
      var1.append("(C) 2003 Vir2L Studos, a ZeniMax Media company. The Elder Scrolls and Vir2L are ");
      var1.append("registered trademarks of ZeniMax Media Inc. All rights reserved.");
      var1.append('\n');
      return var1.toString();
   }

   private void k(int var1) {
      String var2 = this.R[var1].M;
      String var3 = this.R[var1].t();
      short var4 = 0;
      if (k.b(var1)) {
         var4 = k.p[var1];
      } else if (var1 == 4) {
         var4 = k.a;
      } else if (var1 == 5) {
         var4 = k.g;
      }

      var3 = f.a(var2, "<TAG>", var4);
      this.R[var1].e(var3);
   }

   private String f() {
      String[] var1 = RecordStore.listRecordStores();
      int var2 = 0;
      if (var1 == null) {
         var2 = 0;
      } else {
         var2 = var1.length;
      }

      int var3 = f.a(10000);
      String var4 = "es_gamestate" + var3;

      while (true) {
         boolean var5 = false;

         for (int var6 = 0; var6 < var2; var6++) {
            if (var4.equals(var1[var6])) {
               var5 = true;
            }
         }

         if (!var5) {
            return var4;
         }

         var3 = f.a(10000);
         var4 = "es_gamestate" + var3;
      }
   }

   private String M() {
      String[] var1 = RecordStore.listRecordStores();
      int var2 = 0;
      if (var1 == null) {
         var2 = 0;
      } else {
         var2 = var1.length;
      }

      String var3 = null;
      if (var2 == 0) {
         return null;
      }

      long var4 = 0L;
      RecordStore var6 = null;

      for (int var7 = 0; var7 < var2; var7++) {
         try {
            var6 = RecordStore.openRecordStore(var1[var7], false);
            int var8 = var6.getNumRecords();
            long var9 = var6.getLastModified();
            if (var9 > var4) {
               var3 = var1[var7];
               var4 = var9;
            }
         } catch (Throwable var20) {
         } finally {
            try {
               if (var6 != null) {
                  var6.closeRecordStore();
               }

               Object var23 = null;
            } catch (Exception var19) {
            }
         }
      }

      return var3;
   }

   private void L() {
      String var1 = this.M();
      String[] var2 = RecordStore.listRecordStores();
      int var3 = 0;
      if (var2 == null) {
         var3 = 0;
      } else {
         var3 = var2.length;
      }

      if (var3 != 0) {
         for (int var4 = 0; var4 < var3; var4++) {
            if (var1 == null || !var1.equals(var2[var4])) {
               try {
                  RecordStore.deleteRecordStore(var2[var4]);
               } catch (Exception var6) {
               }
            }
         }
      }
   }

   void x() {
      String var1 = k.a(this.k, 5, 13, 0);
      if (var1 == null) {
         var1 = "No rumors!";
      }

      this.m.a(k.s[5]);
      this.m.e(var1);
      this.m.c = this.R[5];
      this.m.N = 5;
      h var2 = (h)this.m.c;
      String var3 = var2.M;
      String var4 = var2.t();
      short var5 = 0;
      var5 = k.g;
      var4 = f.a(var3, "<TAG>", var5);
      var2.e(var4);
      this.a((Object)this.m);
   }

   private void j() {
      if (aT == 4) {
         this.b();
      }
   }

   static {
      try {
         P = new Random(System.currentTimeMillis());
         i();
         C();
         K();
      } catch (Exception var1) {
         System.out.println("ERROR: problem with loading camp or image record HT");
      }
   }
}
