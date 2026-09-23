import java.io.DataInputStream;
import java.io.IOException;

/**
 * Renamed from decompiled/a.java (see docs/CLASS_MAP.md).
 * Loads the plain-text string resources: /lang_N.txt, /copywrite.txt,
 * /start.txt. Not a game-object class -- every instance method here is
 * only ever called on a throwaway instance (`getClass()` is used purely
 * to resolve classpath resources), matching the original.
 */
public final class Strings {
   // Loaded by load(int): the raw concatenated character buffer of a
   // /lang_N.txt file. Whether N is a human language or a UI-text
   // category is still unconfirmed -- see docs/CLASS_MAP.md.
   public static char[] buffer;
   public static boolean firstLangLoaded = false;

   // 13 entries (index 0-12, matching lang_0..lang_12). Values don't
   // match the lang file byte sizes, so these are likely font-metric or
   // buffer-size constants rather than per-language data. UNCONFIRMED.
   public static final int unconfirmedTableA(int var0) {
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

   public static final int unconfirmedTableB(int var0) {
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

   /** Reads /lang_{index}.txt fully into buffer. */
   public final char[] load(int index) {
      String path = "/lang_" + index + ".txt";
      int length = 0;
      DataInputStream in = null;

      try {
         in = new DataInputStream(this.getClass().getResourceAsStream(path));

         while (in.read() != -1) {
            length++;
         }

         in.close();
         in = new DataInputStream(this.getClass().getResourceAsStream(path));
         buffer = new char[length + 1];
         int i = 0;

         int b;
         while ((b = in.read()) != -1) {
            buffer[i++] = (char)b;
         }
      } catch (IOException e) {
         System.out.println("/text/strings.txt is corrupt.");
      } finally {
         try {
            in.close();
         } catch (Exception ignored) {
         }
      }

      System.gc();
      if (index == 0) {
         firstLangLoaded = true;
      }

      return buffer;
   }

   /** Reads /copywrite.txt fully as a single string. */
   public final String loadCopywrite() {
      String path = "/copywrite.txt";
      String result = "";
      DataInputStream in = null;

      try {
         in = new DataInputStream(this.getClass().getResourceAsStream(path));

         int b;
         while ((b = in.read()) != -1) {
            result = result + (char)b;
         }

         in.close();
      } catch (IOException e) {
         System.out.println("/copywrite.txt is corrupt.");
      }

      System.gc();
      return result;
   }

   /** Reads the {@code index}-th '|'-delimited entry out of /start.txt. */
   public final String loadStartupLine(byte index) {
      String path = "/start.txt";
      String result = "";
      byte entry = 0;
      DataInputStream in = null;

      try {
         in = new DataInputStream(this.getClass().getResourceAsStream(path));

         int b;
         while ((b = in.read()) != -1) {
            char c = (char)b;
            if (index == entry) {
               if (c == '|') {
                  break;
               }

               result = result + c;
            } else if (index > entry && c == '|') {
               entry++;
            }
         }

         in.close();
      } catch (IOException e) {
         System.out.println("/copywrite.txt is corrupt.");
      }

      System.gc();
      return result;
   }
}
