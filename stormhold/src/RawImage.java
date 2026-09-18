// Renamed from decompiled/g.java. See ../docs/CLASS_MAP.md.
//
// A from-scratch indexed-color raw image format, completely unrelated to
// dawnstar's own `g` (Screen) -- no dawnstar analog exists for this class.
// Confirmed by direct read: big-endian int32 width/height, a transparency
// flag byte, a big-endian int16 "transparent color" value, a byte count of
// palette entries (<=255), that many big-endian int16 palette colors, then
// 1 byte per pixel as a palette index. Each palette entry is converted to
// an ARGB4444-ish `short` as it's read: bit 0xF000 (the alpha nibble) is
// cleared for the one palette index matching the transparent color (once
// found) and set for every other pixel -- i.e. binary (on/off) transparency
// baked into the palette-to-pixel conversion itself, not a separate mask.
import java.io.InputStream;

public class RawImage {
   int width;
   int height;
   int widthAgain;
   boolean hasTransparency;
   short transparentColorValue;
   // Decoded per-pixel ARGB4444-ish data, width*height entries -- NOT the
   // palette (that's the transient `paletteScratch` below, reused across
   // loads and never kept on the instance).
   short[] pixels;
   // Set to the palette index that matches transparentColorValue, once
   // found during decode; -1 (the private constructor's default) if this
   // image has no transparency or the color was never matched.
   short transparentPaletteIndex;
   static short[] paletteScratch = new short[256];

   private RawImage() {
      this.width = this.height = 0;
      this.pixels = null;
      this.transparentPaletteIndex = -1;
   }

   static RawImage load(String path) throws Exception {
      InputStream in = new Object().getClass().getResourceAsStream(ensureLeadingSlash(path));
      if (in == null) {
         throw new Exception("Image " + path + " is null!");
      }

      RawImage img = new RawImage();
      img.width = readInt32(in);
      img.height = readInt32(in);
      img.widthAgain = img.width;
      int transparencyFlag = in.read() & 0xFF;
      img.hasTransparency = transparencyFlag != 0;
      img.transparentColorValue = readInt16(in);
      int colorCount = in.read() & 0xFF;
      if (colorCount > 255) {
         throw new Exception("Too many colors in image " + path);
      }

      for (int i = 0; i < colorCount; i++) {
         short color = readInt16(in);
         paletteScratch[i] = color;
         if (img.hasTransparency && img.transparentPaletteIndex < 0 && img.transparentColorValue == color) {
            img.transparentPaletteIndex = (short)i;
         }
      }

      int pixelCount = img.width * img.height;
      img.pixels = new short[pixelCount];

      for (int i = 0; i < pixelCount; i++) {
         int index = in.read() & 0xFF;
         short pixel = paletteScratch[index];
         if (img.hasTransparency && index == img.transparentPaletteIndex) {
            pixel = (short)(pixel & -61441);
         } else {
            pixel = (short)(pixel | 61440);
         }

         img.pixels[i] = pixel;
      }

      return img;
   }

   int width() {
      return this.width;
   }

   int height() {
      return this.height;
   }

   private static String ensureLeadingSlash(String path) {
      return path.startsWith("/") ? path : "/" + path;
   }

   private static int readInt32(InputStream in) throws Exception {
      int result = 0;
      result |= in.read() << 24;
      result |= in.read() << 16;
      result |= in.read() << 8;
      result |= in.read();
      return result;
   }

   private static short readInt16(InputStream in) throws Exception {
      int result = 0;
      result |= in.read() << 8;
      result |= in.read();
      result &= 65535;
      return (short)result;
   }
}
