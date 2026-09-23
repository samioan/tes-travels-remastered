import java.util.Enumeration;
import java.util.Hashtable;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.game.Sprite;

/**
 * Renamed from decompiled/g.java (see docs/CLASS_MAP.md).
 * Static .cml parser + Image/Sprite cache + blitter. Binary layout of
 * the .cml records parsed here is written up in docs/ASSET_FORMATS.md.
 */
public final class SpriteRenderer {
   private static Hashtable images = new Hashtable();
   private static Hashtable sprites = new Hashtable();
   // Progress-bar counters fed to Game's loading-screen callback
   // (b.c(percent)) while images are lazily decoded.
   private static byte loadedCount = 0;
   private static byte totalCount = 0;

   public static final void clearImageCache() {
      images.clear();
   }

   /**
    * Reads one bit-flagged record: a leading 16-bit presence mask (high
    * bit first) selects which of up to 10 fields follow in the stream,
    * in fixed order. Field widths: [0]=1 byte(unsigned), [1]/[2]=2 bytes
    * (unsigned, big-endian), [3]/[4]=1 byte(unsigned), [5..9]=1 byte
    * (signed). Shared by the top-level image-group records and the
    * per-frame/per-subframe records below -- same shape, different
    * field meanings (see {@link #unpackFrame}).
    */
   private static final int readRecord(byte[] data, int pos, int[] out) {
      int mask = (char)((data[pos++] & 255) << 8) | (char)((data[pos++] & 255) << 0);

      for (int i = 0; i < out.length; i++) {
         out[i] = 0;
      }

      if ((mask & 512) > 0) {
         out[0] = (char)(data[pos++] & 0xFF);
      }

      if ((mask & 256) > 0) {
         out[1] = (char)(data[pos++] & 0xFF) << '\b' | (char)(data[pos++] & 0xFF) << 0;
      }

      if ((mask & 128) > 0) {
         out[2] = (char)(data[pos++] & 0xFF) << '\b' | (char)(data[pos++] & 0xFF) << 0;
      }

      if ((mask & 64) > 0) {
         out[3] = (char)(data[pos++] & 0xFF) & 255;
      }

      if ((mask & 32) > 0) {
         out[4] = (char)(data[pos++] & 0xFF) & 255;
      }

      if ((mask & 16) > 0) {
         out[5] = data[pos++];
      }

      if ((mask & 8) > 0) {
         out[6] = data[pos++];
      }

      if ((mask & 4) > 0) {
         out[7] = data[pos++];
      }

      if ((mask & 2) > 0) {
         out[8] = data[pos++];
      }

      if ((mask & 1) > 0) {
         out[9] = data[pos++];
      }

      return pos;
   }

   /** Unpacks a decoded record's 9 used fields into a SpriteFrame. */
   private static final void unpackFrame(int[] rec, SpriteFrame frame) {
      frame.groupId = (byte)rec[0];
      frame.offsetX = (short)rec[1];
      frame.offsetY = (short)rec[2];
      frame.width = (short)rec[3];
      frame.height = (short)rec[4];
      frame.frameDx = (byte)rec[5];
      frame.frameDy = (byte)rec[6];
      frame.holdFlag = (byte)rec[7];
      frame.isSprite = (byte)rec[8];
   }

   /** Parses a .cml resource into a linked list of group SpriteFrames. */
   public static final SpriteFrame load(String path) {
      String pathPrefix = "";
      SpriteFrame head = null;
      SpriteFrame tail = null;
      String[] names = new String[255];
      Object[] frameArgs = new Object[50];
      int[] rec = new int[10];
      int[] frameRec = new int[10];
      int[] subframeRec = new int[10];
      int[] colorKey = null;
      int length = b.a(path);
      int pos = 0;
      byte var15 = 0;
      char prefixLen;
      char nameLen;
      char frameCount;
      char subframeCount;
      char colorKeyCount = '\u0000';
      int colorKeyIdx = 0;
      byte[] data = new byte[length];
      loadedCount = 0;
      totalCount = 0;
      System.arraycopy(b.b, 0, data, 0, length);
      if ((prefixLen = (char)(data[0] & 0xFF)) > 0) {
         pathPrefix = new String(data, 1, prefixLen);
      }

      pos = 1 + prefixLen;

      while (pos != length) {
         char nameId = (char)(data[pos++] & 0xFF);
         nameLen = (char)(data[pos++] & 0xFF);
         String name;
         if (!(name = new String(data, pos, nameLen)).startsWith("/")) {
            name = pathPrefix + name;
         }

         pos = readRecord(data, pos + nameLen, rec);
         if (rec[0] == 0) {
            rec[0] = nameId;
         }

         names[rec[0]] = new String(name);
         colorKey = new int[(colorKeyCount = (char)(data[pos++] & 0xFF)) * 2];

         for (int i = 0; i < colorKeyCount; i++) {
            colorKey[colorKeyIdx++] = (char)(data[pos++] & 0xFF) << 16 | (char)(data[pos++] & 0xFF) << '\b' | (char)(data[pos++] & 0xFF) << 0;
            colorKey[colorKeyIdx++] = (char)(data[pos++] & 0xFF) << 16 | (char)(data[pos++] & 0xFF) << '\b' | (char)(data[pos++] & 0xFF) << 0;
         }

         if (colorKeyCount > 0) {
            frameArgs[rec[0]] = colorKey;
         }

         frameCount = (char)(data[pos++] & 0xFF);
         if (!name.equals("/4.png")) {
            Image image = loadImage(name, colorKeyCount > 0 ? (int[])frameArgs[rec[0]] : null);
            if (frameCount == 0) {
               if (head == null) {
                  tail = head = new SpriteFrame();
               } else {
                  tail.nextGroup = new SpriteFrame();
                  tail = tail.nextGroup;
               }

               unpackFrame(rec, tail);
               tail.imagePath = name;
               tail.currentFrame = tail;
               tail.width = (short)image.getWidth();
               tail.height = (short)image.getHeight();
            } else {
               for (int f = 0; f < frameCount; f++) {
                  pos = readRecord(data, pos, frameRec);
                  if (head == null) {
                     tail = head = new SpriteFrame();
                  } else {
                     tail.nextGroup = new SpriteFrame();
                     tail = tail.nextGroup;
                  }

                  tail.currentFrame = tail;
                  var15 = data[pos++];
                  SpriteFrame cur = tail;

                  for (int s = 0; s < var15; s++) {
                     pos = readRecord(data, pos, subframeRec);
                     unpackFrame(subframeRec, cur);
                     cur.groupId = (byte)frameRec[0];
                     cur.holdFlag = (byte)frameRec[7];
                     cur.imagePath = name;
                     cur.frameChainFlag = 1;
                     if (s < var15 - 1) {
                        cur.nextFrame = new SpriteFrame();
                        cur = cur.nextFrame;
                     }
                  }
               }
            }
         }
      }

      b.b = null;
      b.c(100);
      return head;
   }

   private static final Image loadImage(String path, int[] colorKey) {
      Image image;
      if ((image = (Image)images.get(path)) == null) {
         try {
            image = Image.createImage(path);
            images.put(path, image);
         } catch (Exception e) {
            e.printStackTrace();
            image = null;
         }
      }

      totalCount++;
      b.c(++loadedCount * 100 / totalCount);
      return image;
   }

   /** Walks the top-level group chain looking for {@code groupId}. */
   private static final SpriteFrame findGroup(SpriteFrame head, int groupId) {
      for (SpriteFrame f = head; f != null; f = f.nextGroup) {
         if (f.groupId == groupId) {
            return f;
         }
      }

      return null;
   }

   /** Draws the current subframe of group {@code groupId}; returns its width. */
   public static final int draw(Graphics g, SpriteFrame head, int groupId, int x, int y) {
      SpriteFrame group = findGroup(head, (byte)groupId);
      int clipX = 0;
      int clipY = 0;
      short clipW = 0;
      short clipH = 0;
      String path = group.imagePath;
      Image image = (Image)images.get(path);
      if (group != null && group.currentFrame != null) {
         SpriteFrame frame = group.currentFrame;
         if (frame.frameChainFlag == 0) {
            g.drawImage(image, x + frame.frameDx, y + frame.frameDy, 0);
         } else {
            clipX = x + frame.frameDx;
            clipY = y + frame.frameDy;
            if (clipX < b.a && clipY < b.b) {
               clipW = b.a < frame.width ? b.a : frame.width;
               clipH = b.b < frame.height ? b.b : frame.height;
               g.setClip(clipX, clipY, clipW, clipH);
               g.clipRect(clipX, clipY, clipW, clipH);
               if (frame.isSprite == 1) {
                  Sprite sprite;
                  if ((sprite = (Sprite)sprites.get(path)) == null) {
                     try {
                        (sprite = new Sprite(image)).setTransform(2);
                        sprites.put(path, sprite);
                     } catch (Exception e) {
                        e.printStackTrace();
                     }
                  }

                  sprite.setPosition(x + frame.width - image.getWidth() + frame.offsetX + frame.frameDx, y - frame.frameDx + frame.width);
                  sprite.paint(g);
               } else {
                  g.drawImage(image, x - frame.offsetX + frame.frameDx, y - frame.frameDx + frame.width, 0);
               }

               g.setClip(0, 0, b.a, b.b);
            }
         }

         return frame.width;
      } else {
         return 0;
      }
   }

   public static final int getWidth(SpriteFrame head, int groupId) {
      try {
         return findGroup(head, (byte)groupId).width;
      } catch (Exception e) {
         return 0;
      }
   }

   public static final int getHeight(SpriteFrame head, int groupId) {
      try {
         return findGroup(head, (byte)groupId).height;
      } catch (RuntimeException e) {
         return 0;
      }
   }

   /** Advances group {@code groupId}'s current subframe by one; true if it just looped/finished. */
   public static final boolean advanceFrame(SpriteFrame head, int groupId) {
      SpriteFrame group = findGroup(head, groupId);
      SpriteFrame prevCurrent;
      if (group == null) {
         return true;
      }

      prevCurrent = group.currentFrame;
      group.currentFrame = group.currentFrame.nextFrame;
      if (group.currentFrame == null) {
         if (group.holdFlag != 1) {
            group.currentFrame = prevCurrent;
            return true;
         }

         group.currentFrame = findGroup(head, groupId);
      }

      return false;
   }

   /** Sets group {@code groupId}'s current subframe to the {@code index}-th one. */
   public static final boolean setFrame(SpriteFrame head, int groupId, int index) {
      SpriteFrame group;
      SpriteFrame cur = group = findGroup(head, groupId);
      if (group == null) {
         return true;
      }

      for (int i = 0; i < index; i++) {
         if (cur.nextFrame == null) {
            return true;
         }

         cur = cur.nextFrame;
      }

      group.currentFrame = cur;
      return false;
   }

   public static final void resetFrame(SpriteFrame head, int groupId) {
      findGroup(head, groupId).currentFrame = findGroup(head, groupId);
   }

   /** Evicts every cached image whose path starts with {@code pathPrefix}. */
   public static final void evict(String pathPrefix) {
      Enumeration keys = images.keys();

      while (keys.hasMoreElements()) {
         String key;
         if ((key = (String)keys.nextElement()).startsWith(pathPrefix)) {
            images.remove(key);
            sprites.clear();
         }
      }

      b.b();
   }

   // Standard reversed-polynomial (0xEDB88320) CRC32 table builder. Its
   // result is never stored anywhere a consumer was found in this pass --
   // looks like dead/vestigial code. Left untranslated on purpose; not
   // clear whether the static initializer below even calls this one or
   // the void clearImageCache() overload (Vineflower can't disambiguate
   // two zero-arg static methods named the same in decompiled output).

   static {
      clearImageCache();
   }
}
