// Renamed from decompiled/f.java. See ../docs/CLASS_MAP.md.
//
// Grab-bag of stateless helpers: string find/replace for NPC dialogue
// <TAG> substitution, bitset helpers on byte/int, an 8-byte big-endian
// long decode, and thin wrappers around ESGame's resource/RNG statics.
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
      return ESGame.getDataInputStream(path);
   }

   static int randomInt(int bound) {
      return ESGame.lingoRandomInt(bound);
   }

   static int randomInt(Random rng, int bound) {
      return ESGame.lingoRandomInt(rng, bound);
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
}
