// Renamed from decompiled/f.java. See ../docs/CLASS_MAP.md.
//
// Grab-bag of stateless helpers: string find/replace for NPC dialogue
// <TAG> substitution, a space-tokenizer (used for word-wrapping, not
// present in dawnstar's Util.java -- a real addition here), bitset
// helpers on byte/int, an 8-byte big-endian long decode, and thin
// wrappers around ESGame's resource/RNG statics. Near-identical to
// dawnstar's Util.java (../../dawnstar/src/Util.java) apart from that
// extra tokenizer.
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.InputStream;
import java.util.Random;

public class Util {
   private static Object lock = new Object();
   static int unused = 0;

   private Util() {
   }

   static byte[] readAll(int knownLength, InputStream in) throws Exception {
      if (knownLength > 0) {
         byte[] buf = new byte[knownLength];
         int read = in.read(buf);
         return buf;
      }

      ByteArrayOutputStream out = new ByteArrayOutputStream();

      int b;
      while ((b = in.read()) != -1) {
         out.write(b);
      }

      return out.toByteArray();
   }

   static String ensureLeadingSlash(String path) {
      return path.startsWith("/") ? path : "/" + path;
   }

   static DataInputStream openResource(String path) throws Exception {
      return ESGame.getResource(path);
   }

   // 1-based: 1 + abs(rng.nextInt() % bound). Matches ESGame.nextInt, NOT
   // ESGame.lingoRandomInt (0-based) -- confirmed from the original's own
   // `static int a(int var0) { return ESGame.f(var0); }` (f = nextInt).
   static int randomInt(int bound) {
      return ESGame.nextInt(bound);
   }

   static int randomInt(Random rng, int bound) {
      return ESGame.randomInt(rng, bound);
   }

   static String posKey(int x, int y) {
      return x + "," + y;
   }

   static String replace(String source, String tag, int value) {
      return replace(source, tag, "" + value);
   }

   static String replace(String source, String tag, String[] values) {
      String result = source;
      for (int i = 0; i < values.length; i++) {
         result = replace(result, tag, values[i]);
      }
      return result;
   }

   // Replaces only the FIRST occurrence of `tag` in `source` -- callers
   // that need to substitute several distinct <TAG> placeholders call
   // this once per placeholder in order, relying on that.
   static String replace(String source, String tag, String value) {
      if (source == null || tag == null) {
         return source;
      }

      int at = source.indexOf(tag);
      if (at < 0) {
         return source;
      }

      String head = source.substring(0, at);
      if (head == null) {
         head = "";
      }

      String tail = source.substring(at + tag.length());
      if (tail == null) {
         tail = "";
      }

      return head + value + tail;
   }

   static int setBit(int bitIndex, int value) {
      return value | 1 << bitIndex;
   }

   static int clearBit(int bitIndex, int value) {
      return value & ~(1 << bitIndex);
   }

   static byte setBit(byte mask, byte value) {
      return (byte)(value | mask);
   }

   static byte clearBit(byte mask, byte value) {
      return (byte)(value & ~mask);
   }

   static boolean testBit(byte mask, byte value) {
      return (value & mask) != 0;
   }

   // 8-byte big-endian long starting at `offset` in `bytes`.
   static long readLongAt(byte[] bytes, int offset) {
      long result = 0L;

      for (int i = 0; i < 8; i++) {
         long b = bytes[i + offset] & 0xFF;
         int shift = (7 - i) * 8;
         result |= b << shift;
      }

      return result;
   }

   // Splits `text` on runs of spaces (trimmed first). Used by the word-wrap
   // path for menu/dialogue text -- no dawnstar Util.java equivalent found.
   public static String[] splitWords(String text) {
      text = text.trim();
      int wordCount = 1;
      int len = text.length();
      boolean inRun = false;

      for (int i = 0; i < len; i++) {
         if (text.charAt(i) == ' ') {
            if (!inRun) {
               wordCount++;
               inRun = true;
            }
         } else {
            inRun = false;
         }
      }

      String[] words = new String[wordCount];
      int start = 0;
      int out = 0;

      for (int i = 0; i < len; i++) {
         if (text.charAt(i) == ' ') {
            if (!inRun) {
               words[out++] = text.substring(start, i);
               start = i + 1;
               inRun = true;
            } else {
               start++;
            }
         } else {
            inRun = false;
         }
      }

      words[out] = text.substring(start, len);
      return words;
   }
}
