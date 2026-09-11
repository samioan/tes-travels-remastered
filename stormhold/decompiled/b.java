import java.io.DataInputStream;

public class b {
   String c;
   byte h;
   byte e;
   byte f;
   byte d;
   byte j;
   byte g;
   String a;
   static int i;
   static b[] b;

   b() {
   }

   static int d(int var0) {
      return var0 - 1;
   }

   static b c(int var0) {
      return b[d(var0)];
   }

   static boolean a(int var0) {
      return var0 >= 1 && var0 <= i;
   }

   static boolean b(int var0) {
      int var1 = d(var0);
      b var2 = b[var1];
      return var2.d == 2;
   }

   static void a() {
      try {
         DataInputStream var0 = f.a("/spellsin.dat");
         i = var0.readShort();
         b = new b[i];

         for (int var1 = 0; var1 < i; var1++) {
            b[var1] = new b();
         }

         System.out.println("Number of spells is " + i);

         for (int var2 = 0; var2 < i; var2++) {
            b[var2].c = var0.readUTF();
         }

         for (int var3 = 0; var3 < i; var3++) {
            b[var3].h = var0.readByte();
         }

         for (int var4 = 0; var4 < i; var4++) {
            b[var4].e = var0.readByte();
         }

         for (int var5 = 0; var5 < i; var5++) {
            b[var5].f = var0.readByte();
         }

         for (int var6 = 0; var6 < i; var6++) {
            b[var6].d = var0.readByte();
         }

         for (int var7 = 0; var7 < i; var7++) {
            b[var7].j = var0.readByte();
         }

         for (int var8 = 0; var8 < i; var8++) {
            b[var8].g = var0.readByte();
         }

         for (int var9 = 0; var9 < i; var9++) {
            b[var9].a = var0.readUTF();
         }

         var0.close();
      } catch (Exception var10) {
         System.out.println("ERROR: cannot load spells!");
         System.out.println(var10);
      }
   }
}
