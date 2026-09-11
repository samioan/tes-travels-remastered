import java.io.DataInputStream;
import java.io.IOException;

public final class a {
   public static char[] a;
   public static boolean a = false;

   public static final int a(int var0) {
      switch (var0) {
         case 0:
            return 574;
         case 1:
            return 550;
         case 2:
            return 399;
         case 3:
            return 345;
         case 4:
            return 362;
         case 5:
            return 565;
         case 6:
            return 494;
         case 7:
            return 546;
         case 8:
            return 495;
         case 9:
            return 495;
         case 10:
            return 495;
         case 11:
            return 409;
         case 12:
            return 547;
         default:
            return 0;
      }
   }

   public static final int b(int var0) {
      switch (var0) {
         case 0:
            return 305;
         case 1:
            return 34;
         case 2:
            return 46;
         case 3:
            return 9;
         case 4:
            return 16;
         case 5:
            return 6;
         case 6:
            return 42;
         case 7:
            return 20;
         case 8:
            return 16;
         case 9:
            return 30;
         case 10:
            return 9;
         case 11:
            return 7;
         case 12:
            return 6;
         default:
            return 0;
      }
   }

   public final char[] a(int var1) {
      String var2 = "/lang_" + var1 + ".txt";
      int var4 = 0;
      DataInputStream var5 = null;

      try {
         var5 = new DataInputStream(this.getClass().getResourceAsStream(var2));

         while (var5.read() != -1) {
            var4++;
         }

         var5.close();
         var5 = new DataInputStream(this.getClass().getResourceAsStream(var2));
         a = new char[var4 + 1];
         int var6 = 0;

         int var3;
         while ((var3 = var5.read()) != -1) {
            a[var6++] = (char)var3;
         }
      } catch (IOException var15) {
         System.out.println("/text/strings.txt is corrupt.");
      } finally {
         try {
            var5.close();
         } catch (Exception var14) {
         }
      }

      System.gc();
      if (var1 == 0) {
         a = true;
      }

      return a;
   }

   public final String a() {
      String var1 = "/copywrite.txt";
      String var2 = "";
      int var3 = 0;
      DataInputStream var4 = null;

      try {
         var4 = new DataInputStream(this.getClass().getResourceAsStream(var1));

         while ((var3 = var4.read()) != -1) {
            var2 = var2 + (char)var3;
         }

         var4.close();
      } catch (IOException var6) {
         System.out.println("/copywrite.txt is corrupt.");
      }

      System.gc();
      return var2;
   }

   public final String a(byte var1) {
      String var2 = "/start.txt";
      String var3 = "";
      byte var4 = 0;
      char var5 = '\u0000';
      int var6 = 0;
      DataInputStream var7 = null;

      try {
         var7 = new DataInputStream(this.getClass().getResourceAsStream(var2));

         while ((var6 = var7.read()) != -1) {
            var5 = (char)var6;
            if (var1 == var4) {
               if (var5 == '|') {
                  break;
               }

               var3 = var3 + var5;
            } else if (var1 > var4 && var5 == '|') {
               var4++;
            }
         }

         var7.close();
      } catch (IOException var9) {
         System.out.println("/copywrite.txt is corrupt.");
      }

      System.gc();
      return var3;
   }
}
