import javax.microedition.lcdui.Graphics;

/**
 * Renamed from decompiled/i.java (see docs/CLASS_MAP.md).
 * Static pool of magic-effect / ranged-attack projectiles, either
 * fired in a fixed direction or spawned at an Actor's position. All
 * slots share one SpriteFrame tree (/oh_magic.cml) -- the
 * SpriteRenderer.setFrame/advanceFrame calls pass a projectile's "type"
 * as the group id, so it's really per-type animation state, not
 * per-projectile-instance.
 *
 * NOTE: `j` (Actor) is not renamed yet (see docs/CLASS_MAP.md), so its
 * fields are referenced here by their original decompiled letters, same
 * as `b` (Game)'s. Do not guess English names for them here -- that
 * disambiguation belongs to Actor.java's own dedicated rename pass.
 */
public final class ProjectileManager {
   private static SpriteFrame magicEffectFrames = null;
   // 11 slots * 9 shorts: [0]=packed type+facing (or, high nibble 0xF00,
   // an encoded 1-based index into b.a[] identifying the actor this
   // projectile came from), [1]/[2]=x/y, [3]=ms since last move step,
   // [4]=anim state (high byte 0xFF00 = "finished" sentinel), [5]/[6]=
   // origin x/y (or a homed-on actor's last-known x/y), [7]=lifetime ms
   // (0=until hit), [8]=age ms.
   private static short[] pool = new short[99];

   private static final int findFreeSlot() {
      for (byte i = 0; i < pool.length - 9; i += 9) {
         if (pool[i + 0] == -1) {
            return i;
         }
      }

      return -1;
   }

   public static final void clearAll() {
      for (byte i = 0; i < pool.length; i += 9) {
         clear(i);
      }
   }

   public static final void clear(int slot) {
      if (slot >= 0 && slot < pool.length) {
         pool[slot + 0] = -1;
         pool[slot + 1] = -1;
         pool[slot + 2] = -1;
         pool[slot + 3] = -1;
         pool[slot + 4] = -1;
         pool[slot + 5] = -1;
         pool[slot + 6] = -1;
         pool[slot + 7] = -1;
         pool[slot + 8] = -1;
      }
   }

   /** Clears whichever live slot currently sits at grid cell (x, y). */
   public static final void clearAt(int x, int y) {
      for (byte i = 0; i < pool.length; i += 9) {
         if (pool[i + 0] != -1 && pool[i + 1] == x && pool[i + 2] == y) {
            clear(i);
            return;
         }
      }
   }

   public static final int spawn(int dir, j actor) {
      return spawn(dir, 0, actor);
   }

   public static final int spawn(int dir, j actor, int duration) {
      return spawnFromActor(dir, 0, actor, duration);
   }

   public static final int spawn(int dir, int subtype, j actor) {
      return spawnFromActor(dir, subtype, actor, 0);
   }

   /** Spawns a projectile at {@code actor}'s current position. */
   public static final int spawnFromActor(int dir, int subtype, j actor, int duration) {
      int slot;
      if ((slot = findFreeSlot()) == -1) {
         return slot;
      }

      if (dir == 0) {
         if (subtype == 2) {
            dir = 0;
         } else if (subtype == 1) {
            dir = 2;
         } else if (subtype == 3) {
            dir = 4;
         } else if (subtype == 4) {
            dir = 6;
         }
      } else if (dir == 11) {
         if (subtype == 2) {
            dir = 11;
         } else if (subtype == 1) {
            dir = 12;
         } else if (subtype == 3) {
            dir = 13;
         } else if (subtype == 4) {
            dir = 14;
         }
      }

      pool[slot + 0] = (short)(-4096 | actor.c << 8 | dir);
      pool[slot + 1] = (short)actor.b[0];
      pool[slot + 2] = (short)actor.b[1];
      pool[slot + 5] = (short)actor.b[0];
      pool[slot + 6] = (short)actor.b[1];
      pool[slot + 3] = 0;
      pool[slot + 4] = 0;
      pool[slot + 7] = (short)duration;
      pool[slot + 8] = 0;
      return slot;
   }

   public static final int spawn(int type, int x, int y) {
      return spawnFixed(type, x, y, 0);
   }

   /** Spawns a projectile at a fixed (x, y), travelling in a fixed direction. */
   public static final int spawnFixed(int type, int x, int y, int duration) {
      int slot;
      if ((slot = findFreeSlot()) == -1) {
         return -1;
      }

      pool[slot + 0] = (short)type;
      pool[slot + 1] = (short)x;
      pool[slot + 2] = (short)y;
      pool[slot + 5] = (short)x;
      pool[slot + 6] = (short)y;
      pool[slot + 3] = 0;
      pool[slot + 4] = 0;
      pool[slot + 7] = (short)duration;
      pool[slot + 8] = 0;
      return slot;
   }

   /**
    * A projectile's move step overshot its target cell, or its timer
    * expired: identify the actor it came from (encoded in slot[0]'s high
    * nibble as a 1-based b.a[] index) and look for the nearest actor of
    * a different faction within 200 (ActorSystem's distance metric),
    * then hand off resolution to ActorSystem's hit resolver. Returns
    * true if it hit something (caller then clears the slot).
    */
   private static final boolean onExpire(int slot) {
      int[] pos = new int[]{pool[slot + 1], pool[slot + 2]};
      int bestSlot = -1;
      int dist = 0;
      int bestDist = 16777215;
      int sourceIndex = (pool[slot + 0] & 4095) >> 8;
      j source = null;
      if (sourceIndex > 0 && sourceIndex < b.a.length) {
         if ((source = b.a[sourceIndex - 1]) == null) {
            clear(slot);
            return false;
         }

         for (int i = 0; i < 25; i++) {
            if (b.a[i] != null && b.a[i].q != 1 && source != b.a[i] && source.r != b.a[i].r && (dist = h.a(pos, b.a[i].b)) < 200 && dist < bestDist) {
               bestSlot = i;
               bestDist = dist;
            }
         }

         if (bestSlot != -1) {
            h.a(source, b.a[bestSlot], false);
            return true;
         } else {
            return false;
         }
      } else {
         clear(slot);
         return false;
      }
   }

   /** Per-frame update: advances every live projectile one grid step. */
   public static final void tick(long dt) {
      int[] pos = new int[]{0, 0};
      int[] origin = new int[]{0, 0};
      int dir = 0;
      int sourceIndex = 0;

      for (byte i = 0; i < pool.length; i += 9) {
         if (pool[i + 0] != -1) {
            pool[i + 3] = (short)(pool[i + 3] + dt);
            pool[i + 8] = (short)(pool[i + 8] + dt);
            if (pool[i + 7] > 0 && pool[i + 8] >= pool[i + 7]) {
               pool[i + 4] = 0;
               pool[i + 8] = 0;
            }

            if ((pool[i + 4] & '＀') != 65280 && pool[i + 3] > 100) {
               pool[i + 3] = 0;
               pool[i + 4]++;
               if ((dir = pool[i + 0] & 255) >= 0 && dir <= 6 || dir >= 11 && dir <= 14) {
                  if (dir == 0) {
                     pool[i + 2] = (short)(pool[i + 2] - 60);
                  } else if (dir == 2) {
                     pool[i + 2] = (short)(pool[i + 2] + 60);
                  } else if (dir == 4) {
                     pool[i + 1] = (short)(pool[i + 1] + 60);
                  } else if (dir == 6) {
                     pool[i + 1] = (short)(pool[i + 1] - 60);
                  }

                  if (dir == 11) {
                     pool[i + 2] = (short)(pool[i + 2] - 150);
                  } else if (dir == 12) {
                     pool[i + 2] = (short)(pool[i + 2] + 150);
                  } else if (dir == 13) {
                     pool[i + 1] = (short)(pool[i + 1] + 150);
                  } else if (dir == 14) {
                     pool[i + 1] = (short)(pool[i + 1] - 150);
                  }

                  if (SpriteRenderer.setFrame(magicEffectFrames, dir, pool[i + 4])) {
                     if (dir != 1 && dir != 3 && dir != 5 && dir != 7) {
                        pool[i + 4] = 0;
                        SpriteRenderer.setFrame(magicEffectFrames, dir, pool[i + 4]);
                     } else {
                        clear(i);
                     }
                  }

                  pos[0] = pool[i + 1];
                  pos[1] = pool[i + 2];
                  origin[0] = pool[i + 5];
                  origin[1] = pool[i + 6];
                  if (h.a(pos, origin) > 750 || onExpire(i)) {
                     if (dir == 11 || dir == 12 || dir == 13 || dir == 14) {
                        clear(i);
                     } else if (dir == 0 || dir == 2 || dir == 4 || dir == 6) {
                        pool[i + 0]++;
                     }
                  }
               } else {
                  if ((pool[i + 0] & -4096) == -4096) {
                     if ((sourceIndex = ((pool[i + 0] & 4095) >> 8) - 1) < 0 || sourceIndex > b.a.length || b.a[sourceIndex] == null) {
                        clear(i);
                        continue;
                     }

                     pool[i + 1] = (short)b.a[sourceIndex].b[0];
                     pool[i + 2] = (short)b.a[sourceIndex].b[1];
                  }

                  if (SpriteRenderer.setFrame(magicEffectFrames, dir, pool[i + 4])) {
                     if (pool[i + 7] <= 0) {
                        clear(i);
                     } else {
                        pool[i + 4] = (short)(pool[i + 4] | 0xFF00);
                     }
                  }
               }
            }
         }
      }
   }

   public static final void draw(Graphics g, int[] cameraOffset) {
      int[] worldPos = new int[]{0, 0};
      int[] screenPos = new int[]{0, 0};

      for (byte i = 0; i < pool.length; i += 9) {
         if (pool[i + 0] != -1 && (pool[i + 4] & '＀') != 65280) {
            worldPos[0] = pool[i + 1];
            worldPos[1] = pool[i + 2];
            b.a(worldPos, screenPos);
            if (screenPos[0] + cameraOffset[0] >= 0 && screenPos[0] + cameraOffset[0] <= b.a && screenPos[1] + cameraOffset[1] >= 0 && screenPos[1] + cameraOffset[1] <= b.b) {
               SpriteRenderer.setFrame(magicEffectFrames, pool[i + 0] & 255, pool[i + 4]);
               SpriteRenderer.draw(g, magicEffectFrames, pool[i + 0] & 255, screenPos[0] + cameraOffset[0], screenPos[1] + cameraOffset[1]);
            }
         }
      }
   }

   static {
      magicEffectFrames = SpriteRenderer.load("/oh_magic.cml");
      clearAll();
   }
}
