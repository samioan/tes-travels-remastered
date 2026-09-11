import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.InputStream;
import java.util.Random;

public class f {
   private static Object b = new Object();
   static int a = 0;

   private f() {
   }

   static byte[] a(int var0, InputStream var1) throws Exception {
      if (var0 > 0) {
         byte[] var5 = new byte[var0];
         int var6 = var1.read(var5);
         return var5;
      }

      ByteArrayOutputStream var2 = new ByteArrayOutputStream();

      int var3;
      while ((var3 = var1.read()) != -1) {
         var2.write(var3);
      }

      return var2.toByteArray();
   }

   static String b(String var0) {
      return var0.startsWith("/") ? var0 : "/" + var0;
   }

   static DataInputStream a(String var0) throws Exception {
      return ESGame.getDataInputStream(var0);
   }

   static int a(int var0) {
      return ESGame.lingoRandomInt(var0);
   }

   static int a(Random var0, int var1) {
      return ESGame.lingoRandomInt(var0, var1);
   }

   static String b(int var0, int var1) {
      return var0 + "," + var1;
   }

   static String a(String var0, String var1, int var2) {
      String var3 = "" + var2;
      return a(var0, var1, var3);
   }

   static String a(String var0, String var1, String[] var2) {
      int var3 = var2.length;
      String var4 = var0;

      for (int var5 = 0; var5 < var3; var5++) {
         String var6 = var2[var5];
         var4 = a(var4, var1, var6);
      }

      return var4;
   }

   static String a(String var0, String var1, String var2) {
      if (var0 != null && var1 != null) {
         int var3 = var0.indexOf(var1);
         if (var3 < 0) {
            return var0;
         }

         String var4 = var0.substring(0, var3);
         if (var4 == null) {
            var4 = "";
         }

         String var5 = var0.substring(var3 + var1.length());
         if (var5 == null) {
            var5 = "";
         }

         return var4 + var2 + var5;
      } else {
         return var0;
      }
   }

   static int a(int var0, int var1) {
      return var1 | 1 << var0;
   }

   static int c(int var0, int var1) {
      return var1 & ~(1 << var0);
   }

   static byte b(byte var0, byte var1) {
      return (byte)(var1 | var0);
   }

   static byte c(byte var0, byte var1) {
      return (byte)(var1 & ~var0);
   }

   static boolean a(byte var0, byte var1) {
      return (var1 & var0) != 0;
   }

   static long a(byte[] var0, int var1) {
      long var2 = 0L;

      for (int var4 = 0; var4 < 8; var4++) {
         long var5 = var0[var4 + var1] & 0xFF;
         int var7 = (7 - var4) * 8;
         var2 |= var5 << var7;
      }

      return var2;
   }
}
