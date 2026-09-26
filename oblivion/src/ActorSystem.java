/**
 * Renamed from decompiled/h.java (see docs/CLASS_MAP.md, docs/rename.map).
 * All-static movement / combat / AI / equipment logic over Actor records.
 */
import java.io.ByteArrayOutputStream;
import javax.microedition.lcdui.Graphics;

public final class ActorSystem {
   private static byte unusedA = -52;
   private static byte unusedB = -39;
   private static short defaultSightRange = 300;
   private static short defaultAttackRange = 200;
   private static final byte[] animStateOffset = new byte[]{0, 4, 8, 12, 16, 20, 24, 25};
   public static final short[] xpForLevel = new short[]{
      0, 0, 100, 210, 340, 500, 700, 950, 1260, 1640, 2100, 2650, 3300, 4060, 4940, 5950, 7100, 8400, 9860, 11490, 13300, 15300, 17500, 19910, 22540, 25400
   };
   private static final short[] xpReward = new short[]{
      0, 10, 12, 15, 19, 24, 30, 37, 45, 54, 64, 75, 87, 100, 114, 129, 145, 162, 180, 199, 219, 240, 262, 285, 309, 334
   };
   private static int[] tmpHalfW = new int[2];
   private static int[] tmpFullW = new int[2];
   private static int[] tmpHalfOffset = new int[2];
   private static int[] tmpFullOffset = new int[2];
   private static int[] tmpCellIdx = new int[3];

   public static final Actor createFromCml(String var0, byte var1) {
      Actor var2 = new Actor();
      boolean var3 = false;
      var2.cmlPath = var0;
      var2.slot = var1;
      var2.sortCell[0] = 0;
      var2.sortCell[1] = 0;
      var2.sprite = SpriteRenderer.load(var0);

      for (int var4 = 0; var4 < var2.wornArmor.length; var4++) {
         var2.wornArmor[var4] = -1;
      }

      var2.spriteWidth = (byte)SpriteRenderer.getWidth(var2.sprite, 1);
      var2.spriteHalfWidth = (byte)(var2.spriteWidth >> 1);
      return var2;
   }

   public static final Actor fromRecord(byte[] var0, int var1) {
      Actor var2 = new Actor();
      byte var3 = 0;
      byte var4 = 0;
      boolean var5 = false;
      var2.slot = var0[var1++];
      var2.classId = var0[var1++];
      var2.xp = ((char)var0[var1++] & 255) << 16 | ((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0;
      var2.level = (byte)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.strength = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.intelligence = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.agility = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.speed = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.endurance = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.willpower = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.weapon = (byte)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.sightRange = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.attackRange = (short)(((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0);
      var2.team = var0[var1++];
      Game.gold = ((char)var0[var1++] & 255) << 8 | ((char)var0[var1++] & 255) << 0;
      var3 = var0[var1++];
      var2.cmlPath = new String(var0, var1, var3);
      var2.name = "Champion";
      var1 += var3;
      var4 = var0[var1++];
      setClass(var2, var2.classId, true);

      for (int var43 = 0; var43 < var2.inventory.length; var43++) {
         var2.inventory[var43] = 0;
      }

      for (int var44 = 0; var44 < var4; var44++) {
         int var6 = (char)var0[var1++];
         char var7 = (char)var0[var1++];
         boolean var8;
         if (var8 = (var6 & 128) == 128) {
            int var45;
            var6 = (var45 = var6 & -129) & 0xFF;
         }

         addItemForced(var2, var6, Game.script.getRowChecked(var6, var7), var8);
      }

      recalcDerivedStats(var2);
      var2.sortCell[0] = 0;
      var2.sortCell[1] = 0;
      var2.sprite = SpriteRenderer.load(var2.cmlPath);
      var2.spriteWidth = (byte)SpriteRenderer.getWidth(var2.sprite, 1);
      var2.spriteHalfWidth = (byte)(var2.spriteWidth >> 1);
      recalcDerivedStats(var2);
      return var2;
   }

   public static final void revive(Actor actor) {
      boolean var1 = false;

      for (int var2 = 0; var2 < 25; var2++) {
         if (Game.actors[var2] != null) {
            Game.actors[var2].target = null;
         }
      }

      actor.sortCell[0] = 0;
      actor.sortCell[1] = 0;
      actor.enterScript = -1;
      actor.leaveScript = -1;
      actor.zoneId = -1;
      actor.dead = 0;
      actor.statusIcon = -1;
      actor.target = null;
      actor.deathTimer = 0;
      actor.floatText = null;
      actor.floatTextY = 0;
      actor.floatTextColor = 16711680;
      actor.animState = 0;
      actor.moveTarget[0] = -1;
      actor.moveTarget[1] = -1;
      actor.dotRemaining = 0;
      actor.dotTick = 0;
      actor.dotSource = null;
      actor.hp = actor.maxHp = (short)(actor.level * 4 + (actor.strength + actor.buffStrength) * 2 + actor.endurance * 2 + actor.bonusMaxHp);
      actor.hpRegenInterval = (short)(40000 / actor.maxHp);
      actor.mp = actor.maxMp = (short)(actor.level * 4 + actor.intelligence * 2 + actor.bonusMaxMp);
      actor.mpRegenInterval = (short)(40000 / actor.maxMp);
      recalcDerivedStats(actor);
      updateSortCell(actor);
   }

   private static final void updateScreenPos(Actor actor) {
      actor.screenPos[0] = actor.pos[0] - actor.pos[1] >> 3;
      actor.screenPos[1] = actor.pos[0] + actor.pos[1] >> 4;
   }

   public static final void updateCells(Actor actor) {
      actor.cell[0] = (byte)(actor.pos[0] >> 7);
      actor.cell[1] = (byte)(actor.pos[1] >> 7);
      actor.footBCell[0] = (byte)(actor.footB[0] >> 7);
      actor.footBCell[1] = (byte)(actor.footB[1] >> 7);
      actor.footCCell[0] = (byte)(actor.footC[0] >> 7);
      actor.footCCell[1] = (byte)(actor.footC[1] >> 7);
      updateSortCell(actor);
   }

   public static final boolean isBlocked(Actor actor) {
      if (actor == null) {
         return false;
      } else if (actor.collides == 0) {
         return false;
      } else if (actor.cell[0] < 0) {
         return true;
      } else if (actor.cell[1] >= Game.gridHeight) {
         return true;
      } else if (actor.footCCell[0] >= Game.gridWidth) {
         return true;
      } else if (actor.footCCell[1] < 0) {
         return true;
      } else if (footBlocked(actor, (byte)1)) {
         return true;
      } else {
         return footBlocked(actor, (byte)2) ? true : footBlocked(actor, (byte)3);
      }
   }

   private static final boolean footBlocked(Actor actor, byte var1) {
      byte var2 = 0;
      int var3 = 0;
      int var4 = 0;
      if (Game.collision == null) {
         return false;
      }

      if (actor == null) {
         return false;
      }

      if (actor.cell[0] * Game.gridHeight + actor.cell[1] > Game.collision.length) {
         return true;
      }

      if (actor.footBCell[0] * Game.gridHeight + actor.footBCell[1] > Game.collision.length) {
         return true;
      }

      if (actor.footCCell[0] * Game.gridHeight + actor.footCCell[1] > Game.collision.length) {
         return true;
      }

      switch (var1) {
         case 1:
            var2 = Game.collision[actor.cell[0] * Game.gridHeight + actor.cell[1]];
            var3 = actor.pos[0] % 128;
            var4 = actor.pos[1] % 128;
            break;
         case 2:
            var2 = Game.collision[actor.footBCell[0] * Game.gridHeight + actor.footBCell[1]];
            var3 = actor.footB[0] % 128;
            var4 = actor.footB[1] % 128;
            break;
         case 3:
            var2 = Game.collision[actor.footCCell[0] * Game.gridHeight + actor.footCCell[1]];
            var3 = actor.footC[0] % 128;
            var4 = actor.footC[1] % 128;
      }

      if (var2 == 0) {
         return false;
      }

      switch (var2) {
         case 1:
            return true;
         case 2:
            if (var3 <= var4) {
               return true;
            }

            return false;
         case 3:
            if (var4 >= var3) {
               return true;
            }

            return false;
         case 4:
            if (var4 <= var3) {
               return true;
            }

            return false;
         case 5:
            if (var3 >= var4) {
               return true;
            }

            return false;
         default:
            return false;
      }
   }

   public static final void moveDir(Actor actor, int var1, long var2) {
      System.out.println("moveInWorld()  ");
      if (actor != null) {
         actor.moveTimer = (short)(actor.moveTimer + var2);
         if (actor.moveTimer > 50) {
            if (actor.moveTimer > 400) {
               actor.moveTimer = 50;
            }

            int var4 = actor.speed / (1000 / actor.moveTimer);
            switch (var1) {
               case 1:
                  moveBy(actor, 0, var4);
                  break;
               case 2:
                  moveBy(actor, 0, -var4);
                  break;
               case 3:
                  moveBy(actor, var4, 0);
                  break;
               case 4:
                  moveBy(actor, -var4, 0);
            }

            if (isBlocked(actor)) {
               undoMove(actor);
            }

            actor.moveTimer = 0;
         }
      }
   }

   private static final void moveBy(Actor actor, int var1, int var2) {
      actor.prevPos[0] = actor.pos[0];
      actor.prevPos[1] = actor.pos[1];
      actor.pos[0] = actor.pos[0] + var1;
      actor.pos[1] = actor.pos[1] + var2;
      actor.footB[0] = actor.footB[0] + var1;
      actor.footB[1] = actor.footB[1] + var2;
      actor.footC[0] = actor.footC[0] + var1;
      actor.footC[1] = actor.footC[1] + var2;
      updateScreenPos(actor);
      updateCells(actor);
      if (var1 > 0) {
         actor.facing = 3;
      } else if (var1 < 0) {
         actor.facing = 4;
      } else if (var2 > 0) {
         actor.facing = 1;
      } else if (var2 < 0) {
         actor.facing = 2;
      }

      actor.idleTimer = 500;
   }

   public static final void setPosition(Actor actor, int var1, int var2) {
      tmpHalfW[0] = actor.spriteHalfWidth;
      tmpHalfW[1] = 0;
      tmpFullW[0] = actor.spriteWidth;
      tmpFullW[1] = 0;
      tmpHalfOffset[0] = 0;
      tmpHalfOffset[1] = 0;
      tmpFullOffset[0] = 0;
      tmpFullOffset[1] = 0;
      Game.isoToWorld(tmpHalfOffset, tmpHalfW);
      Game.isoToWorld(tmpFullOffset, tmpFullW);
      actor.pos[0] = var1;
      actor.pos[1] = var2;
      actor.footB[0] = var1 + tmpHalfOffset[0];
      actor.footB[1] = var2 + tmpHalfOffset[1];
      actor.footC[0] = var1 + tmpFullOffset[0];
      actor.footC[1] = var2 + tmpFullOffset[1];
      updateScreenPos(actor);
      updateCells(actor);
   }

   private static final void updateSortCell(Actor actor) {
      if (actor.footCCell[0] > actor.footBCell[0]) {
         actor.sortCell[0] = actor.footCCell[0];
         actor.sortCell[1] = actor.footCCell[1];
      } else if (actor.footBCell[1] <= actor.footCCell[1] && actor.footBCell[0] <= actor.cell[0]) {
         actor.sortCell[0] = actor.cell[0];
         actor.sortCell[1] = actor.cell[1];
      } else {
         actor.sortCell[0] = actor.footBCell[0];
         actor.sortCell[1] = actor.footBCell[1];
      }
   }

   public static final void undoMove(Actor actor) {
      setPosition(actor, actor.prevPos[0], actor.prevPos[1]);
   }

   public static final byte checkTriggerTiles(Actor actor, byte[] var1, byte[] var2) {
      boolean var3 = false;
      actor.enterScript = -1;
      actor.leaveScript = -1;
      if (var1 != null && var2 != null) {
         tmpCellIdx[0] = actor.cell[0] * Game.gridHeight + actor.cell[1];
         tmpCellIdx[1] = actor.footBCell[0] * Game.gridHeight + actor.footBCell[1];
         tmpCellIdx[2] = actor.footCCell[0] * Game.gridHeight + actor.footCCell[1];

         for (int var4 = 0; var4 < tmpCellIdx.length; var4++) {
            if (tmpCellIdx[var4] < 0 || tmpCellIdx[var4] >= var1.length) {
               return -1;
            }

            if (var1[tmpCellIdx[var4]] != 0 && var1[tmpCellIdx[var4]] != -1) {
               actor.enterScript = var1[tmpCellIdx[var4]];
               actor.leaveScript = var2[tmpCellIdx[var4]];
               return var1[tmpCellIdx[var4]];
            }
         }

         return -1;
      } else {
         return -1;
      }
   }

   public static final void setMoveTarget(Actor actor, int var1, int var2) {
      actor.moveTarget[0] = var1;
      actor.moveTarget[1] = var2;
      actor.animState = 1;
   }

   public static final void update(Actor actor, long var1, boolean var3) {
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      if (actor != null) {
         actor.animTimer = (short)(actor.animTimer + var1);
         actor.killTimer = (int)(actor.killTimer + var1);
         actor.attackTimer = (int)(actor.attackTimer + var1);
         if (actor.animTimer > 125 && actor.dead == 0) {
            SpriteRenderer.advanceFrame(actor.sprite, actor.facing + animStateOffset[actor.animState]);
            actor.animTimer = 0;
         }

         if (actor.dead == 0) {
            if (actor.moveTarget[0] != -1) {
               actor.moveTimer = (short)(actor.moveTimer + var1);
               if (actor.moveTimer >= 50) {
                  if (actor.moveTimer > 100) {
                     actor.moveTimer = 100;
                  }

                  var6 = actor.speed / (1000 / actor.moveTimer);
                  if (actor.pos[0] < actor.moveTarget[0]) {
                     var4 = Math.min(var6, actor.moveTarget[0] - actor.pos[0]);
                  } else if (actor.pos[0] > actor.moveTarget[0]) {
                     var4 = Math.max(-var6, actor.moveTarget[0] - actor.pos[0]);
                  } else if (actor.pos[1] < actor.moveTarget[1]) {
                     var5 = Math.min(var6, actor.moveTarget[1] - actor.pos[1]);
                  } else if (actor.pos[1] > actor.moveTarget[1]) {
                     var5 = Math.max(-var6, actor.moveTarget[1] - actor.pos[1]);
                  } else {
                     actor.moveTarget[0] = -1;
                     if (actor.animState != 2) {
                        actor.animState = 0;
                     }
                  }

                  moveBy(actor, var4, var5);
                  actor.moveTimer = 0;
               }
            } else if (actor.slot == 1 && actor.idleTimer > 0) {
               actor.idleTimer = (short)(actor.idleTimer - var1);
               if (actor.idleTimer <= 0) {
                  actor.animState = 0;
               }
            }

            if (actor.dotRemaining > 0) {
               actor.dotRemaining = (short)(actor.dotRemaining - var1);
               actor.dotTick = (short)(actor.dotTick - var1);
               if (actor.dotTick <= 0) {
                  ProjectileManager.spawn(8, actor);
                  actor.dotTick = 1000;
                  applyDamage(actor.dotDamage, actor, actor.dotSource, false, true);
               }
            } else if (actor.effectIcon == -47) {
               actor.effectIcon = -1;
            }

            if (actor.aiType == 2) {
               actor.teleportTimer = (short)(actor.teleportTimer - var1);
            }

            if (actor.slot == 1) {
               if (actor.hp < actor.maxHp) {
                  actor.hpRegenTimer = (short)(actor.hpRegenTimer + var1);
                  if (actor.hpRegenTimer >= actor.hpRegenInterval) {
                     if (actor.hp < actor.maxHp) {
                        actor.hp++;
                     }

                     actor.hpRegenTimer = 0;
                     recalcDerivedStats(actor);
                  }
               }

               if (actor.mp < actor.maxMp) {
                  actor.mpRegenTimer = (short)(actor.mpRegenTimer + var1);
                  if (actor.mpRegenTimer >= actor.mpRegenInterval) {
                     if (actor.mp < actor.maxMp) {
                        actor.mp++;
                     }

                     actor.mpRegenTimer = 0;
                     recalcDerivedStats(actor);
                  }
               }

               if (actor.itemBuffDuration > 0) {
                  if (actor.itemBuffElapsed >= actor.itemBuffDuration) {
                     actor.itemBuffElapsed = 0;
                     actor.itemBuffDuration = 0;
                     actor.bonusMaxMp = 0;
                     actor.buffArmor = 0;
                     actor.buffAttack = 0;
                     actor.buffDefense = 0;
                     actor.buffAttack2 = 0;
                     actor.dodgeScale = 0;
                     actor.effectIcon = -1;
                     ProjectileManager.clear(actor.buffFxSlot);
                     if (actor.bonusMaxMp != 0) {
                        actor.mp = actor.maxMp = (short)(actor.level * 4 + actor.intelligence * 2 + actor.bonusMaxMp);
                        actor.mpRegenInterval = (short)(40000 / actor.maxMp);
                     }

                     if (actor.buffStrength != 0) {
                        actor.buffStrength = 0;
                        actor.maxHp = (short)(actor.level * 4 + (actor.strength + actor.buffStrength) * 2 + actor.endurance * 2 + actor.bonusMaxHp);
                        actor.hpRegenInterval = (short)(40000 / actor.maxHp);
                     }

                     recalcDerivedStats(actor);
                  }

                  actor.itemBuffElapsed = (short)(actor.itemBuffElapsed + var1);
               }
            } else if (actor.aiActive == 1 && aiThink(actor) && actor.target != null && actor.attackTimer >= actor.attackInterval) {
               if ((var3 || actor.target.slot != 1) && attack(actor, actor.target, true)) {
                  actor.target = null;
                  actor.dotSource = null;
                  actor.animState = 0;
               }

               actor.attackTimer = 0;
            }

            if (actor.floatText != null) {
               actor.floatTextTimer = (short)(actor.floatTextTimer + var1);
               if (actor.floatTextTimer > 50) {
                  actor.floatTextY = (short)(actor.floatTextY - 2);
                  actor.floatTextColor = actor.floatTextColor - actor.floatTextShadow;
                  if (actor.floatTextColor <= 0 || Math.abs(actor.floatTextStartY - actor.floatTextY) > 20) {
                     actor.floatTextColor = 0;
                     actor.floatTextY = 0;
                     actor.floatTextStartY = 0;
                     actor.floatText = null;
                  }

                  actor.floatTextTimer = 0;
               }
            }

            if (actor.buffTimer > 0) {
               actor.buffTimer = (short)(actor.buffTimer - var1);
               if (actor.buffTimer <= 0) {
                  actor.itemBuffElapsed = 0;
                  actor.itemBuffDuration = 0;
                  actor.bonusMaxMp = 0;
                  actor.buffArmor = 0;
                  actor.buffAttack = 0;
                  actor.buffDefense = 0;
                  actor.buffAttack2 = 0;
                  actor.dodgeScale = 0;
                  actor.effectIcon = -1;
                  ProjectileManager.clear(actor.buffFxSlot);
                  if (actor.bonusMaxMp != 0) {
                     actor.mp = actor.maxMp = (short)(actor.level * 4 + actor.intelligence * 2 + actor.bonusMaxMp);
                     actor.mpRegenInterval = (short)(40000 / actor.maxMp);
                  }

                  if (actor.buffStrength != 0) {
                     actor.buffStrength = 0;
                     actor.maxHp = (short)(actor.level * 4 + (actor.strength + actor.buffStrength) * 2 + actor.endurance * 2 + actor.bonusMaxHp);
                     actor.hpRegenInterval = (short)(40000 / actor.maxHp);
                  }

                  recalcDerivedStats(actor);
                  return;
               }
            }
         } else {
            if (actor.deathTimer >= 250) {
               Game.removeActor(actor.slot - 1);
            }

            actor.deathTimer = (short)(actor.deathTimer + var1);
         }
      }
   }

   public static final void draw(Actor actor, Graphics var1, int[] var2) {
      if (actor.dead != 1 && actor.animState != 6) {
         SpriteRenderer.draw(
            var1,
            actor.sprite,
            -56,
            actor.screenPos[0]
               + var2[0]
               + (SpriteRenderer.getWidth(actor.sprite, actor.facing + animStateOffset[0]) >> 1)
               - (SpriteRenderer.getWidth(actor.sprite, -56) >> 1),
            actor.screenPos[1] + var2[1] - SpriteRenderer.getHeight(actor.sprite, -56) + 3 + (actor.animState != 2 && actor.animState != 3 ? 0 : 3)
         );
         SpriteRenderer.draw(
            var1,
            actor.sprite,
            actor.facing + animStateOffset[actor.animState],
            actor.screenPos[0] + var2[0],
            actor.screenPos[1] + var2[1] - SpriteRenderer.getHeight(actor.sprite, actor.facing + animStateOffset[actor.animState])
         );
         if (actor.dead == 0 && actor.slot != 1 && actor.team == 0) {
            int var3 = actor.screenPos[0] + var2[0] + (SpriteRenderer.getWidth(actor.sprite, actor.facing) >> 1) - 10;
            int var4 = actor.screenPos[1] + var2[1] - SpriteRenderer.getHeight(actor.sprite, actor.facing) - 6;
            var1.setColor(16777215);
            var1.drawRect(var3, var4, 20, 3);
            var1.setColor(16711680);
            var1.fillRect(var3 + 1, var4 + 1, 19 * actor.hp / actor.maxHp, 2);
         }

         if (actor.dead == 0 && actor.floatText != null) {
            if (actor.floatTextY == 0) {
               actor.floatTextStartY = actor.floatTextY = (short)(
                  actor.screenPos[1] - SpriteRenderer.getHeight(actor.sprite, actor.facing) - (actor.slot == 1 ? 6 : 10)
               );
               if (actor.floatText.equals(Game.getString(471))) {
                  actor.floatTextColor = 65280;
                  actor.floatTextShadow = 8704;
               } else if (actor.floatText.equals(Game.getString(470))) {
                  actor.floatTextColor = 255;
                  actor.floatTextShadow = 34;
               } else {
                  actor.floatTextColor = 16711680;
                  actor.floatTextShadow = 2228224;
               }
            }

            var1.setColor(actor.floatTextColor);
            var1.drawString(
               actor.floatText, actor.screenPos[0] + var2[0] + (SpriteRenderer.getWidth(actor.sprite, actor.facing) >> 1) - 10, actor.floatTextY + var2[1], 0
            );
         }

         if (actor.statusIcon != -1) {
            int var5 = actor.screenPos[0] + var2[0] + SpriteRenderer.getWidth(actor.sprite, actor.facing) - 4;
            int var6 = actor.screenPos[1]
               + var2[1]
               - SpriteRenderer.getHeight(actor.sprite, actor.facing + animStateOffset[actor.animState])
               - SpriteRenderer.getWidth(actor.sprite, -54)
               - 4;
            if (actor.slot != 1) {
               var6 -= 8;
            }

            SpriteRenderer.draw(var1, actor.sprite, -54, var5, var6);
            if (actor.statusIcon != -2) {
               SpriteRenderer.draw(var1, actor.sprite, actor.statusIcon, var5, var6);
            }
         }
      } else {
         SpriteRenderer.draw(
            var1,
            actor.sprite,
            -55,
            actor.screenPos[0]
               + var2[0]
               + (SpriteRenderer.getWidth(actor.sprite, actor.facing + animStateOffset[0]) >> 1)
               - (SpriteRenderer.getWidth(actor.sprite, -55) >> 1),
            actor.screenPos[1] + var2[1] - SpriteRenderer.getHeight(actor.sprite, -55) + 3
         );
      }
   }

   public static final void setAnimState(Actor actor, byte var1) {
      if (var1 == 6) {
         actor.dead = 1;
      } else if (actor.animState != var1) {
         SpriteRenderer.resetFrame(actor.sprite, actor.facing + animStateOffset[var1]);
      }

      actor.animState = var1;
   }

   private static final void recalcDerivedStats(Actor actor) {
      boolean var1 = false;
      int[] var2 = Game.script.getRow(4, actor.weapon);
      actor.weaponPower = (byte)var2[3];
      actor.armor = 0;

      for (int var3 = 0; var3 < actor.wornArmor.length; var3++) {
         if (actor.wornArmor[var3] != -1) {
            actor.armor = (short)(actor.armor + Game.script.getRow(1, actor.wornArmor[var3])[4]);
         }
      }

      switch (actor.classId) {
         case 1:
            if (actor.level == 1) {
               actor.dodgeChance = 5;
            }

            if (actor.level == 7) {
               actor.dodgeChance = 10;
            }

            if (actor.level == 15) {
               actor.dodgeChance = 15;
            }

            if (var2[2] == 4) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 7) {
                  actor.attackRating = 110;
               }

               if (actor.level == 16) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (actor.weapon == 0) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 5) {
                  actor.attackRating = 110;
               }

               if (actor.level == 15) {
                  actor.attackRating = 125;
               }

               if (actor.level == 1) {
                  actor.defenseRating = 110;
               }

               if (actor.level == 5) {
                  actor.defenseRating = 125;
               }

               if (actor.level == 15) {
                  actor.defenseRating = 140;
                  return;
               }
            }
            break;
         case 2:
            if (actor.level == 1) {
               actor.dodgeChance = 5;
            }

            if (actor.level == 7) {
               actor.dodgeChance = 10;
            }

            if (actor.level == 15) {
               actor.dodgeChance = 15;
            }

            if (actor.level == 1) {
               actor.defenseRating = 100;
            }

            if (actor.level == 10) {
               actor.defenseRating = 115;
            }

            if (actor.level == 20) {
               actor.defenseRating = 130;
            }

            if (var2[2] == 2) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 3) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 6) {
                  actor.attackRating = 110;
               }

               if (actor.level == 15) {
                  actor.attackRating = 125;
                  return;
               }
            }
            break;
         case 3:
            if (actor.level == 1) {
               actor.blockChance = 0;
            }

            if (actor.level == 5) {
               actor.blockChance = 3;
            }

            if (actor.level == 17) {
               actor.blockChance = 10;
            }

            if (actor.level == 1) {
               actor.defenseRating = 100;
            }

            if (actor.level == 10) {
               actor.defenseRating = 115;
            }

            if (actor.level == 20) {
               actor.defenseRating = 130;
            }

            if (var2[2] == 1) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 2) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 3) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 6) {
                  actor.attackRating = 110;
               }

               if (actor.level == 15) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (actor.weapon == 0) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 5) {
                  actor.attackRating = 110;
               }

               if (actor.level == 15) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 0) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            }
            break;
         case 4:
            if (actor.level == 1) {
               actor.defenseRating = 100;
            }

            if (actor.level == 10) {
               actor.defenseRating = 115;
            }

            if (actor.level == 20) {
               actor.defenseRating = 130;
            }

            if (var2[2] == 1) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 4) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 7) {
                  actor.attackRating = 110;
               }

               if (actor.level == 16) {
                  actor.attackRating = 125;
                  return;
               }
            }
            break;
         case 5:
            if (actor.level == 1) {
               actor.blockChance = 0;
            }

            if (actor.level == 5) {
               actor.blockChance = 3;
            }

            if (actor.level == 17) {
               actor.blockChance = 10;
            }

            if (actor.level == 1) {
               actor.defenseRating = 100;
            }

            if (actor.level == 10) {
               actor.defenseRating = 115;
            }

            if (actor.level == 20) {
               actor.defenseRating = 130;
            }

            if (actor.level == 1) {
               actor.defenseRating = 100;
            }

            if (actor.level == 7) {
               actor.defenseRating = 115;
            }

            if (actor.level == 17) {
               actor.defenseRating = 130;
            }

            if (var2[2] == 1) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 2) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 0) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            }
            break;
         case 6:
            if (actor.level == 1) {
               actor.blockChance = 0;
            }

            if (actor.level == 5) {
               actor.blockChance = 3;
            }

            if (actor.level == 17) {
               actor.blockChance = 10;
            }

            if (actor.level == 1) {
               actor.defenseRating = 100;
            }

            if (actor.level == 10) {
               actor.defenseRating = 115;
            }

            if (actor.level == 20) {
               actor.defenseRating = 130;
            }

            if (var2[2] == 2) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 4) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 7) {
                  actor.attackRating = 110;
               }

               if (actor.level == 16) {
                  actor.attackRating = 125;
               }
            }
            break;
         case 7:
            if (actor.level == 1) {
               actor.dodgeChance = 5;
            }

            if (actor.level == 7) {
               actor.dodgeChance = 10;
            }

            if (actor.level == 15) {
               actor.dodgeChance = 15;
            }

            if (actor.level == 1) {
               actor.defenseRating = 100;
            }

            if (actor.level == 10) {
               actor.defenseRating = 115;
            }

            if (actor.level == 20) {
               actor.defenseRating = 130;
               return;
            }
            break;
         case 8:
            if (actor.level == 1) {
               actor.defenseRating = 100;
            }

            if (actor.level == 10) {
               actor.defenseRating = 115;
            }

            if (actor.level == 20) {
               actor.defenseRating = 130;
            }

            if (actor.level == 1) {
               actor.defenseRating = 100;
            }

            if (actor.level == 7) {
               actor.defenseRating = 115;
            }

            if (actor.level == 17) {
               actor.defenseRating = 130;
            }

            if (var2[2] == 1) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 2) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            } else if (var2[2] == 0) {
               if (actor.level == 1) {
                  actor.attackRating = 100;
               }

               if (actor.level == 8) {
                  actor.attackRating = 110;
               }

               if (actor.level == 18) {
                  actor.attackRating = 125;
                  return;
               }
            }
      }
   }

   public static final int distance(int[] var0, int[] var1) {
      boolean var2 = false;
      int var3 = 0;
      int var4 = 0;
      int var5 = 0;
      int var6 = 0;
      int var7 = 0;
      var6 = var0[0] - var1[0];
      var7 = var0[1] - var1[1];
      var6 = var6 < 0 ? -var6 : var6;
      var7 = var7 < 0 ? -var7 : var7;
      if (var6 < var7) {
         var4 = var6;
         var5 = var7;
      } else {
         var4 = var7;
         var5 = var6;
      }

      var3 = var5 * 1007 + var4 * 441;
      if (var5 < var4 << 4) {
         var3 -= var5 * 40;
      }

      return Math.abs(var3 + 512 >> 10);
   }

   private static final Actor findNearestEnemy(Actor actor) {
      Actor var1 = null;
      int var2 = 16777215;
      int var3 = 0;
      boolean var4 = false;

      for (int var6 = 0; var6 < Game.actors.length; var6++) {
         if (Game.actors[var6] != null
            && Game.actors[var6].dead != 1
            && Game.actors[var6].team != actor.team
            && Game.actors[var6].slot != actor.slot
            && (var3 = distance(actor.pos, Game.actors[var6].pos)) < var2) {
            var2 = var3;
            var1 = Game.actors[var6];
         }
      }

      return var1;
   }

   private static final void stepAwayFrom(Actor actor, Actor var1) {
      int var2 = actor.pos[0] - var1.pos[0];
      int var3 = actor.pos[1] - var1.pos[1];
      if (Math.abs(var2) > Math.abs(var3)) {
         if (var2 > 0) {
            setMoveTarget(actor, actor.pos[0] - 20, actor.pos[1]);
         } else {
            setMoveTarget(actor, actor.pos[0] + 20, actor.pos[1]);
         }
      } else if (var3 > 0) {
         setMoveTarget(actor, actor.pos[0], actor.pos[1] - 20);
      } else {
         setMoveTarget(actor, actor.pos[0], actor.pos[1] + 20);
      }
   }

   private static final void faceTowards(Actor actor, Actor var1) {
      if (actor.screenPos[0] < var1.screenPos[0] && actor.screenPos[1] > var1.screenPos[1]) {
         actor.facing = 2;
      } else if (actor.screenPos[0] > var1.screenPos[0] && actor.screenPos[1] < var1.screenPos[1]) {
         actor.facing = 1;
      } else if (actor.screenPos[0] < var1.screenPos[0] && actor.screenPos[1] < var1.screenPos[1]) {
         actor.facing = 3;
      } else {
         if (actor.screenPos[0] > var1.screenPos[0] && actor.screenPos[1] > var1.screenPos[1]) {
            actor.facing = 4;
         }
      }
   }

   private static final boolean aiThink(Actor actor) {
      Actor var1 = findNearestEnemy(actor);
      int var2 = 0;
      if (var1 != null) {
         if ((var2 = distance(actor.pos, var1.pos)) <= actor.sightRange) {
            if (var2 >= actor.attackRange) {
               if (actor.slot != 1) {
                  stepAwayFrom(actor, var1);
                  return false;
               }
            } else {
               actor.moveTarget[0] = -1;
               actor.target = var1;
               actor.animState = 4;
               faceTowards(actor, var1);
            }
         } else if (actor.target != null && actor.aiType != 2) {
            actor.moveTarget[0] = -1;
            actor.target = null;
            actor.animState = 0;
         }
      } else if (actor.target != null) {
         actor.target = null;
         actor.animState = 0;
      }

      return true;
   }

   public static final void initFromTemplate(Actor actor, int[] var1) {
      int var2 = 0;
      if (actor != null && var1 != null) {
         actor.spawnRow = var1;
         actor.level = (byte)var1[2];
         if (actor.slot != 1) {
            actor.strength = (short)var1[3];
            actor.intelligence = (short)var1[4];
            actor.willpower = (short)var1[5];
            actor.agility = (short)var1[6];
            actor.speed = (short)var1[7];
            actor.endurance = (short)var1[8];
            actor.personality = (short)var1[9];
            actor.sightRange = (short)var1[14];
            actor.attackRange = (short)var1[15];
            actor.weapon = (byte)var1[10];
            actor.aiType = (byte)var1[18];
            var2 = var1[11];
            actor.special = Game.script.specials[var1[19]];
            actor.ranged = (byte)(actor.aiType == 4 ? 1 : 0);
            if (var1[20] > 0) {
               actor.attackInterval = (short)(var1[20] * 1000);
            }

            if (actor.ranged == 1 || actor.aiType == 0) {
               actor.special = null;
            }

            if (actor.weapon > 0) {
               addItem(actor, 0, Game.script.getRow(4, actor.weapon));
            }

            if (var2 > 0) {
               addItem(actor, 1, Game.script.getRow(1, var2));
            }
         }

         actor.team = (byte)var1[13];
         actor.hp = actor.maxHp = (short)(actor.level * 4 + (actor.strength + actor.buffStrength) * 2 + actor.endurance * 2 + actor.bonusMaxHp);
         actor.hpRegenInterval = (short)(40000 / actor.maxHp);
         actor.mp = actor.maxMp = (short)(actor.level * 4 + actor.intelligence * 2 + actor.bonusMaxMp);
         actor.mpRegenInterval = (short)(40000 / actor.maxMp);
         if (actor.sightRange == 0) {
            actor.sightRange = defaultSightRange;
         }

         if (actor.attackRange == 0) {
            actor.attackRange = defaultAttackRange;
         }

         recalcDerivedStats(actor);
      }
   }

   private static final boolean applyDamage(int var0, Actor var1, Actor var2, boolean var3, boolean var4) {
      if (var1 != null && (var2 != null || var4) && var1.invulnerable != 1) {
         int var5 = var1.dodgeChance + (var1.dodgeChance >> 1);
         int var6 = var1.blockChance + (var1.blockChance >> 1);
         int var7 = (var1.agility + var1.armor + var1.buffArmor >> 3) + var1.buffDefense;
         if (var4) {
            var7 = 0;
            var6 = -1000;
            var5 = -1000;
         }

         int var8 = var0 - var7;
         int var9 = Game.random.nextInt() % 100;
         int var10 = Game.random.nextInt() % 100;
         int var12;
         var5 = (var12 = var5 * var1.dodgeScale) / 100;
         var9 = var9 < 0 ? -var9 : var9;
         var10 = var10 < 0 ? -var10 : var10;
         if (var1.target == null) {
            var1.target = var2;
         }

         if (var9 <= var5) {
            var1.floatText = Game.getString(471);
            var1.floatTextY = 0;
         } else if (var10 <= var6) {
            var1.floatText = Game.getString(470);
            var1.floatTextY = 0;
         } else if (var8 > 0) {
            if (var1.slot != 1 && var1.target != null && !var4) {
               var1.sightRange = (short)Math.max(distance(var1.pos, var1.target.pos), var1.sightRange);
            }

            if (var2 != null && var2.ranged == 0) {
               Game.random.nextInt();
            }

            var1.hp = (short)(var1.hp - var8);
            var1.floatText = (var3 ? Game.getString(472) : "") + Integer.toString(var8);
            var1.floatTextY = 0;
            var1.dead = (byte)(var1.hp <= 0 ? 1 : 0);
            if (var1.dead == 1) {
               if (var2 != null) {
                  var2.killTimer = 0;
                  grantXp(var2, var1.level);
               }

               Game.random.nextInt();
               var1.animState = 6;
               updateSortCell(var1);
               if (var1.deathScript >= 0) {
                  ScriptInterpreter.runScript((char)var1.deathScript);
               }

               int var11;
               if (var1.dropsLoot == 1 && (var11 = Game.script.rollLoot()) != 0) {
                  Game.instance.spawnItem(var11, false, var1.footBCell[0], var1.footBCell[1]);
               }
            }
         }

         return var1.dead == 1;
      } else {
         return false;
      }
   }

   private static final boolean teleportStep(Actor actor) {
      int var1 = 0;
      int var2 = 0;
      boolean var3 = false;
      boolean var4 = false;
      int var5 = 0;
      int var6 = 0;
      int var7 = 0;
      Object var8 = null;
      byte[][] var9 = new byte[][]{{-1, 0}, {0, -1}, {0, 0}, {0, 1}, {1, 0}};
      if (actor.vanished == 1) {
         if (actor.teleportTimer <= -1000 && Game.actors[0].dead == 0) {
            for (byte[] var14 = (byte[])Game.layers.elementAt(0); var14 != null && !var4 && var5 < 100; var5++) {
               var4 = true;
               var1 = Math.abs(Game.actors[0].pos[0] + Game.random.nextInt() % 500);
               var2 = Math.abs(Game.actors[0].pos[1] + Game.random.nextInt() % 500);
               var6 = var1 >> 7;
               var7 = var2 >> 7;

               for (int var11 = 0; var11 < var9.length; var11++) {
                  int var10;
                  if ((var10 = (var6 + var9[var11][0]) * Game.gridHeight + var7 + var9[var11][1]) >= 0 && var10 < var14.length) {
                     if (Game.collision[var10] != 0 || var14[var10] == 0) {
                        var4 = false;
                        break;
                     }
                  } else {
                     var4 = false;
                  }
               }
            }

            setPosition(actor, var1, var2);
            ProjectileManager.spawn(8, actor.pos[0], actor.pos[1]);
            if (var4) {
               actor.vanished = 0;
               return true;
            }
         }
      } else if (Game.actors[0].dead == 0) {
         ProjectileManager.spawn(8, actor.pos[0], actor.pos[1]);
         actor.moveTarget[0] = -1;
         actor.moveTarget[1] = -1;
         setPosition(actor, -10000, -10000);
         actor.vanished = 1;
      }

      return false;
   }

   public static final boolean attack(Actor actor, Actor var1, boolean var2) {
      if (actor != null && var1 != null) {
         if (actor.slot != 1 && (actor.special != null || actor.ranged == 1) && var2) {
            useSpecialAttack(actor, false);
            if (actor.aiType == 3) {
               actor.special = null;
               actor.attackRange = (short)(actor.attackRange >> 1);
            } else if (actor.aiType == 2 && actor.teleportTimer <= 0 && teleportStep(actor)) {
               actor.teleportTimer = (short)(Math.abs(Game.random.nextInt()) % 2000 + 2000);
            }

            return false;
         } else {
            int var3 = (actor.strength + actor.buffStrength + actor.weaponPower >> 1) + actor.buffAttack + actor.buffAttack2;
            int var4 = Game.random.nextInt() % 16;
            boolean var5 = false;
            int var6;
            var3 = (var6 = var3 * actor.attackRating) / 100;
            if (actor.special != null) {
               if (actor.level >= actor.special[10]) {
                  var3 = actor.special[5];
               } else if (actor.level >= actor.special[9]) {
                  var3 = actor.special[4];
               } else {
                  var3 = actor.special[3];
               }

               if (actor.slot != 1 && actor.special[2] != 4) {
                  var3 >>= 1;
               }
            }

            if ((var4 < 0 ? -var4 : var4) == 1) {
               var3 = Math.max(var1.hp >> 2, var3 << 1);
               var5 = true;
            }

            return applyDamage(var3, var1, actor, var5, false);
         }
      } else {
         return false;
      }
   }

   private static final void grantXp(Actor actor, int var1) {
      if (actor.owner != null) {
         actor = actor.owner;
      }

      if (actor.slot == 1) {
         actor.xp = actor.xp + xpReward[var1];
         if (actor.level < 25 && actor.xp >= xpForLevel[actor.level + 1]) {
            short var2 = actor.strength;
            short var3 = actor.intelligence;
            short var4 = actor.willpower;
            short var5 = actor.agility;
            short var6 = actor.endurance;
            short var7 = actor.personality;
            StringBuffer var8 = new StringBuffer(Game.getString(41));
            actor.level++;
            actor.strength++;
            actor.intelligence++;
            actor.willpower++;
            actor.agility++;
            actor.speed++;
            actor.endurance++;
            actor.personality++;
            applyLevelUpBonus(actor);
            var8.append(" ");
            var8.append(actor.level);
            var8.append(": +");
            var8.append(actor.strength - var2);
            var8.append(" ");
            var8.append(Game.getString(415));
            var8.append(", +");
            var8.append(actor.intelligence - var3);
            var8.append(" ");
            var8.append(Game.getString(416));
            var8.append(", +");
            var8.append(actor.willpower - var4);
            var8.append(" ");
            var8.append(Game.getString(417));
            var8.append(", +");
            var8.append(actor.agility - var5);
            var8.append(" ");
            var8.append(Game.getString(418));
            var8.append(", +");
            var8.append(actor.endurance - var6);
            var8.append(" ");
            var8.append(Game.getString(419));
            var8.append(", +");
            var8.append(actor.personality - var7);
            var8.append(" ");
            var8.append(Game.getString(420));
            actor.maxHp = (short)(actor.level * 4 + (actor.strength + actor.buffStrength) * 2 + actor.endurance * 2 + actor.bonusMaxHp);
            actor.hpRegenInterval = (short)(40000 / actor.maxHp);
            actor.maxMp = (short)(actor.level * 4 + actor.intelligence * 2 + actor.bonusMaxMp);
            actor.mpRegenInterval = (short)(40000 / actor.maxMp);
            recalcDerivedStats(actor);
            Game.showMessage(var8.toString(), 30, 4, 3);
            return;
         }

         Game.showMessage(xpReward[var1] + " " + Game.getString(42) + "!!!", 3, 4, 1);
      }
   }

   private static final void applyLevelUpBonus(Actor actor) {
      switch (actor.classId) {
         case 1:
            if (actor.level == 5) {
               actor.speed = (short)(actor.speed + 25);
               return;
            }

            if (actor.level == 10) {
               actor.agility++;
               return;
            }

            if (actor.level == 15) {
               actor.endurance = (short)(actor.endurance + 2);
               return;
            }

            if (actor.level == 20) {
               actor.strength = (short)(actor.strength + 2);
               return;
            }
            break;
         case 2:
            if (actor.level == 5) {
               actor.agility++;
               return;
            }

            if (actor.level == 10) {
               actor.willpower++;
               return;
            }

            if (actor.level == 15) {
               actor.intelligence = (short)(actor.intelligence + 2);
               return;
            }

            if (actor.level == 20) {
               actor.agility = (short)(actor.agility + 2);
               return;
            }
            break;
         case 3:
            if (actor.level == 5) {
               actor.strength++;
               return;
            }

            if (actor.level == 10) {
               actor.endurance++;
               return;
            }

            if (actor.level == 15) {
               actor.endurance = (short)(actor.endurance + 2);
               return;
            }

            if (actor.level == 20) {
               actor.strength = (short)(actor.strength + 2);
               return;
            }
            break;
         case 4:
            if (actor.level == 5) {
               actor.speed = (short)(actor.speed + 25);
               return;
            }

            if (actor.level == 10) {
               actor.agility = (short)(actor.agility + 2);
               return;
            }

            if (actor.level == 15) {
               actor.strength++;
               return;
            }

            if (actor.level == 20) {
               actor.strength = (short)(actor.strength + 2);
               return;
            }
            break;
         case 5:
            if (actor.level == 5) {
               actor.strength++;
               return;
            }

            if (actor.level == 10) {
               actor.endurance++;
               return;
            }

            if (actor.level == 15) {
               actor.strength = (short)(actor.strength + 2);
               return;
            }

            if (actor.level == 20) {
               actor.endurance = (short)(actor.endurance + 2);
               return;
            }
            break;
         case 6:
            if (actor.level == 5) {
               actor.agility++;
               return;
            }

            if (actor.level == 10) {
               actor.willpower++;
               return;
            }

            if (actor.level == 15) {
               actor.intelligence = (short)(actor.intelligence + 2);
               return;
            }

            if (actor.level == 20) {
               actor.willpower = (short)(actor.willpower + 2);
               return;
            }
            break;
         case 7:
            if (actor.level == 5) {
               actor.intelligence++;
               return;
            }

            if (actor.level == 10) {
               actor.willpower++;
               return;
            }

            if (actor.level == 15) {
               actor.willpower = (short)(actor.willpower + 2);
               return;
            }

            if (actor.level == 20) {
               actor.intelligence = (short)(actor.intelligence + 2);
               return;
            }
            break;
         case 8:
            if (actor.level == 5) {
               actor.willpower++;
               return;
            }

            if (actor.level == 10) {
               actor.strength++;
               return;
            }

            if (actor.level == 15) {
               actor.intelligence = (short)(actor.intelligence + 2);
               return;
            }

            if (actor.level == 20) {
               actor.willpower = (short)(actor.willpower + 2);
            }
      }
   }

   public static final byte checkZoneTiles(Actor actor, byte[] var1) {
      boolean var2 = false;
      actor.zoneId = -1;
      if (var1 == null) {
         return actor.zoneId;
      }

      int[] var3 = new int[]{
         actor.cell[0] * Game.gridHeight + actor.cell[1], actor.footBCell[0] * Game.gridHeight + actor.footBCell[1], actor.footCCell[0] * Game.gridHeight + actor.footCCell[1]
      };

      for (int var4 = 0; var4 < var3.length; var4++) {
         if (var3[var4] < 0 || var3[var4] >= var1.length) {
            return -1;
         }

         if (var1[var3[var4]] >= 0 && var1[var3[var4]] < 255) {
            actor.zoneId = var1[var3[var4]];
            return var1[var3[var4]];
         }
      }

      if (actor.ranged == 0 && actor.special == null) {
         actor.idleTimer = 500;
         actor.animState = 4;
      }

      if (actor.attackTimer >= actor.attackInterval) {
         actor.attackTimer = 0;
         if (actor.special == null && actor.ranged != 1) {
            aiThink(actor);
            if (actor.target != null) {
               faceTowards(actor, actor.target);
               if (attack(actor, actor.target, true)) {
                  actor.target = null;
                  actor.dotSource = null;
                  actor.animState = 0;
               }
            }
         } else {
            useSpecialAttack(actor, true);
         }
      }

      return -1;
   }

   private static final void useSpecialAttack(Actor actor, boolean var1) {
      int var2 = 0;
      boolean var3 = false;
      if (actor.ranged == 1) {
         if (actor.slot == 1) {
            actor.idleTimer = 500;
            actor.animState = 7;
         }

         ProjectileManager.spawn(11, actor.facing, actor);
      } else {
         if (actor.level >= actor.special[10]) {
            if (actor.mp < actor.special[13] && var1) {
               return;
            }

            var2 = actor.special[5];
            actor.mp = (short)(actor.mp - actor.special[13]);
         } else if (actor.level >= actor.special[9]) {
            if (actor.mp < actor.special[12] && var1) {
               return;
            }

            var2 = actor.special[4];
            actor.mp = (short)(actor.mp - actor.special[12]);
         } else {
            if (actor.mp < actor.special[11] && var1) {
               return;
            }

            var2 = actor.special[3];
            actor.mp = (short)(actor.mp - actor.special[11]);
         }

         switch (actor.special[2]) {
            case 0:
               actor.buffTimer = (short)actor.special[6];
               actor.buffArmor = (short)var2;
               actor.effectIcon = -48;
               ProjectileManager.clear(actor.buffFxSlot);
               actor.buffFxSlot = (byte)ProjectileManager.spawn(9, actor, 5000);
               break;
            case 1:
               actor.buffTimer = (short)actor.special[6];
               actor.buffAttack2 = (short)var2;
               actor.effectIcon = -50;
               ProjectileManager.clear(actor.buffFxSlot);
               actor.buffFxSlot = (byte)ProjectileManager.spawn(9, actor, 5000);
               break;
            case 2:
               if (actor.summon != null) {
                  Game.removeActor(actor.summon.slot - 1);
               }

               actor.summon = Game.instance.spawnActor("/oh_scamp.cml", actor.pos[0], actor.pos[1], actor.spawnRow);
               actor.summon.owner = actor;
               setDropsLoot(actor.summon, false);
            case 4:
               for (int var6 = 0; var6 < Game.actors.length; var6++) {
                  if (Game.actors[var6] != null
                     && Game.actors[var6] != actor
                     && Game.actors[var6].team != actor.team
                     && distance(actor.pos, Game.actors[var6].pos) <= actor.special[14]) {
                     applyPoison(actor, Game.actors[var6], var2, actor.special[6]);
                  }
               }
               break;
            case 3:
               if (actor.special[1] == 61618) {
                  for (int var5 = 0; var5 < Game.actors.length; var5++) {
                     if (Game.actors[var5] != null
                        && Game.actors[var5] != actor
                        && Game.actors[var5].team != actor.team
                        && distance(actor.pos, Game.actors[var5].pos) <= actor.special[14]) {
                        applyMagicHit(actor, Game.actors[var5], var2);
                     }
                  }
               } else if (actor.special[1] == 61619) {
                  ProjectileManager.spawn(8, actor);
                  actor.hp = (short)Math.min(actor.maxHp, actor.hp + Math.abs(var2));
               } else {
                  ProjectileManager.spawn(0, actor.facing, actor);
               }
               break;
            case 5:
               actor.buffTimer = (short)actor.special[6];
               actor.dodgeScale = (short)(var2 + 100);
               actor.effectIcon = -48;
               ProjectileManager.clear(actor.buffFxSlot);
               actor.buffFxSlot = (byte)ProjectileManager.spawn(9, actor, 5000);
               break;
            case 6:
               ProjectileManager.spawn(8, actor);
               actor.dotRemaining = 0;
               actor.dotTick = 0;
               actor.effectIcon = -1;
         }

         recalcDerivedStats(actor);
      }
   }

   public static final void setDeathScript(Actor actor, int var1, int var2) {
      actor.deathScript = (byte)var2;
   }

   public static final void clearDeathScript(Actor actor, int var1) {
      actor.deathScript = -1;
   }

   public static final void setStat(Actor actor, int var1, int var2, ScriptInterpreter var3) {
      switch (var1) {
         case 2:
            actor.level = (byte)var2;
            break;
         case 3:
            actor.strength = (short)var2;
            break;
         case 4:
            actor.intelligence = (short)var2;
            break;
         case 5:
            actor.willpower = (short)var2;
            break;
         case 6:
            actor.agility = (short)var2;
            break;
         case 7:
            actor.speed = (short)var2;
            break;
         case 8:
            actor.endurance = (short)var2;
            break;
         case 9:
            actor.personality = (short)var2;
            break;
         case 10:
            actor.weapon = (byte)var2;
         case 11:
         case 12:
         case 16:
         case 17:
         default:
            break;
         case 13:
            actor.team = (byte)var2;
            break;
         case 14:
            actor.sightRange = (short)var2;
            break;
         case 15:
            actor.attackRange = (short)var2;
            break;
         case 18:
            actor.aiType = (byte)var2;
            actor.ranged = (byte)(actor.aiType == 4 ? 1 : 0);
            if (actor.ranged == 1 || actor.aiType == 0) {
               actor.special = null;
            }
            break;
         case 19:
            actor.special = Game.script.specials[var2];
            break;
         case 20:
            actor.attackInterval = (short)(var2 * 1000);
      }

      actor.maxHp = (short)(actor.level * 4 + (actor.strength + actor.buffStrength) * 2 + actor.endurance * 2 + actor.bonusMaxHp);
      actor.hp = (short)Math.min(actor.hp, actor.maxHp);
      actor.hpRegenInterval = (short)(40000 / actor.maxHp);
      actor.maxMp = (short)(actor.level * 4 + actor.intelligence * 2 + actor.bonusMaxMp);
      actor.mp = (short)Math.min(actor.mp, actor.maxMp);
      actor.mpRegenInterval = (short)(40000 / actor.maxMp);
      if (actor.weapon > 0) {
         actor.weaponPower = (byte)var3.getRow(4, actor.weapon)[3];
      }

      if (actor.sightRange == 0) {
         actor.sightRange = defaultSightRange;
      }

      if (actor.attackRange == 0) {
         actor.attackRange = defaultAttackRange;
      }

      recalcDerivedStats(actor);
   }

   public static final void addItem(Actor actor, int var1, int[] var2) {
      addItemForced(actor, var1, var2, false);
   }

   public static final void addItemForced(Actor actor, int var1, int[] var2, boolean var3) {
      int var4 = 0;

      while (var4 < actor.inventory.length && actor.inventory[var4] != 0) {
         var4++;
      }

      if (var4 < actor.inventory.length) {
         switch (var1) {
            case 0:
               actor.inventory[var4] = 0 | var2[0];
               if (actor.weapon == 0 && actor.special == null && canUseItem(actor, 0, var2) || var3) {
                  actor.weapon = (byte)var2[0];
               }
               break;
            case 1:
               if (actor.wornArmor[var2[3]] == -1 || var3) {
                  equipArmor(actor, var2);
               }

               actor.inventory[var4] = 256 | var2[0];
               return;
            case 2:
               actor.inventory[var4] = 512 | var2[0];
               if (var2[5] == 0) {
                  if (actor.hpPotion == null && var2[2] > 0) {
                     actor.hpPotion = var2;
                     return;
                  }

                  if (actor.mpPotion == null && var2[3] > 0) {
                     actor.mpPotion = var2;
                     return;
                  }
               }
         }
      }
   }

   private static final void reselectBestWeapon(Actor actor) {
      boolean var1 = false;
      int[] var2 = null;
      int[] var3 = null;

      for (int var4 = 0; var4 < actor.inventory.length && actor.inventory[var4] != 0; var4++) {
         if (actor.inventory[var4] <= 255) {
            var3 = Game.script.getRow(4, actor.inventory[var4] & 0xFF);
            if (canUseItem(actor, 0, var3) && (var2 == null || var3[3] > var2[3])) {
               var2 = var3;
            }
         }
      }

      if (var2 != null) {
         actor.weapon = (byte)var2[0];
         actor.ranged = (byte)(var2[2] == 4 ? 1 : 0);
      }
   }

   public static final void removeItem(Actor actor, int var1, int[] var2) {
      int var3 = 0;
      boolean var4 = false;
      int var5 = 0;
      boolean var6 = false;
      switch (var1) {
         case 0:
            var5 = 0 | var2[0];
            if (actor.weapon == var2[0]) {
               actor.weapon = 0;
               actor.ranged = 0;
               var6 = true;
            }
            break;
         case 1:
            var5 = 256 | var2[0];
            break;
         case 2:
            var5 = 512 | var2[0];
      }

      while (var3 < actor.inventory.length && actor.inventory[var3] != 0) {
         if (actor.inventory[var3] == var5) {
            for (int var7 = var3; var7 < actor.inventory.length - 1; var7++) {
               actor.inventory[var7] = actor.inventory[var7 + 1];
            }
            break;
         }

         var3++;
      }

      if (var6) {
         reselectBestWeapon(actor);
      }

      recalcDerivedStats(actor);
   }

   public static final int spriteHeight(Actor actor) {
      return actor.animState == 6 ? 0 : SpriteRenderer.getHeight(actor.sprite, actor.facing + animStateOffset[actor.animState]);
   }

   public static final int spriteWidth(Actor actor) {
      return actor.animState == 6 ? 0 : SpriteRenderer.getWidth(actor.sprite, actor.facing + animStateOffset[actor.animState]);
   }

   public static final void setClass(Actor actor, byte var1, boolean var2) {
      actor.classId = var1;
      if (actor.classId == 4) {
         actor.ranged = 1;
      }

      actor.classRow = Game.script.getRow(5, var1);
      actor.classList = Game.script.classLists[var1];
      if (!var2) {
         addItem(actor, 0, Game.script.getRow(4, actor.classRow[4]));
         addItem(actor, 1, Game.script.getRow(1, actor.classRow[5]));
         actor.strength = (short)Game.script.classBase[var1][7];
         actor.intelligence = (short)Game.script.classBase[var1][8];
         actor.willpower = (short)Game.script.classBase[var1][9];
         actor.agility = (short)Game.script.classBase[var1][10];
         actor.speed = (short)Game.script.classBase[var1][6];
         actor.endurance = (short)Game.script.classBase[var1][11];
         actor.personality = (short)Game.script.classBase[var1][12];
         actor.attackRange = (short)Game.script.classBase[var1][13];
         actor.sightRange = (short)Game.script.classBase[var1][14];
      }

      recalcDerivedStats(actor);
   }

   public static final void setStatusIcon(Actor actor, byte var1) {
      switch (var1) {
         case 0:
            actor.statusIcon = -1;
         default:
            return;
         case 1:
            actor.statusIcon = -53;
            return;
         case 2:
            actor.statusIcon = -52;
            return;
         case 3:
            actor.statusIcon = -51;
            return;
         case 4:
            actor.statusIcon = -2;
      }
   }

   public static final void serialize(Actor actor, ByteArrayOutputStream var1) throws Exception {
      int var2 = 0;
      boolean var3 = false;
      boolean var4 = false;
      var1.write(actor.slot);
      var1.write(actor.classId);
      var1.write((byte)(actor.xp >> 16));
      var1.write((byte)(actor.xp >> 8));
      var1.write((byte)(actor.xp >> 0));
      var1.write((byte)(actor.level >> 8));
      var1.write((byte)(actor.level >> 0));
      var1.write((byte)(actor.strength >> 8));
      var1.write((byte)(actor.strength >> 0));
      var1.write((byte)(actor.intelligence >> 8));
      var1.write((byte)(actor.intelligence >> 0));
      var1.write((byte)(actor.agility >> 8));
      var1.write((byte)(actor.agility >> 0));
      var1.write((byte)(actor.speed >> 8));
      var1.write((byte)(actor.speed >> 0));
      var1.write((byte)(actor.endurance >> 8));
      var1.write((byte)(actor.endurance >> 0));
      var1.write((byte)(actor.willpower >> 8));
      var1.write((byte)(actor.willpower >> 0));
      var1.write((byte)(actor.weapon >> 8));
      var1.write((byte)(actor.weapon >> 0));
      var1.write((byte)(actor.sightRange >> 8));
      var1.write((byte)(actor.sightRange >> 0));
      var1.write((byte)(actor.attackRange >> 8));
      var1.write((byte)(actor.attackRange >> 0));
      var1.write(actor.team);
      var1.write((byte)(Game.gold >> 8));
      var1.write((byte)(Game.gold >> 0));
      var1.write(actor.cmlPath.length());
      var1.write(actor.cmlPath.getBytes());

      while (actor.inventory[var2] != 0) {
         var2++;
      }

      var1.write(var2);

      for (int var5 = 0; var5 < actor.inventory.length && actor.inventory[var5] != 0; var5++) {
         if ((actor.inventory[var5] >> 8 & 0xFF) == 1) {
            var4 = hasArmor(actor, actor.inventory[var5] & 0xFF);
         } else if ((actor.inventory[var5] >> 8 & 0xFF) == 0) {
            var4 = isWeaponEquipped(actor, actor.inventory[var5] & 0xFF, false);
         } else {
            var4 = false;
         }

         var1.write((byte)((var4 ? 128 : 0) | actor.inventory[var5] >> 8));
         var1.write((byte)(actor.inventory[var5] >> 0));
      }
   }

   public static final void useConsumable(Actor actor, int[] var1) {
      if (var1[5] == 0) {
         if (Game.script.getItemName(var1[1]).equals(Game.getString(158))) {
            actor.hp = (short)Math.min(actor.maxHp, actor.hp + var1[2]);
            actor.mp = (short)Math.min(actor.maxMp, actor.mp + var1[3]);
            removeItem(actor, 2, var1);
            recalcDerivedStats(actor);
         }

         if (var1[2] > 0) {
            actor.hpPotion = var1;
            return;
         }

         if (var1[3] > 0) {
            actor.mpPotion = var1;
            return;
         }

         if (var1[4] > 0) {
            actor.dotRemaining = 0;
            actor.dotTick = 0;
            actor.effectIcon = -1;
            removeItem(actor, 2, var1);
            recalcDerivedStats(actor);
            return;
         }
      } else {
         if (var1[3] > 0) {
            actor.bonusMaxMp = (short)var1[3];
            actor.mp = actor.maxMp = (short)(actor.level * 4 + actor.intelligence * 2 + actor.bonusMaxMp);
            actor.mpRegenInterval = (short)(40000 / actor.maxMp);
         }

         if (var1[6] > 0) {
            actor.buffAttack = (short)var1[6];
         }

         if (var1[7] > 0) {
            actor.buffArmor = (short)var1[7];
         }

         if (var1[8] > 0) {
            actor.buffDefense = (short)var1[8];
         }

         if (var1[10] > 0) {
            actor.buffAttack2 = (short)var1[10];
         }

         if (var1[11] > 0) {
            actor.buffStrength = (short)var1[11];
            actor.maxHp = (short)(actor.level * 4 + (actor.strength + actor.buffStrength) * 2 + actor.endurance * 2 + actor.bonusMaxHp);
            actor.hpRegenInterval = (short)(40000 / actor.maxHp);
         }

         if (var1[4] > 0) {
            actor.dotRemaining = 0;
            actor.dotTick = 0;
            actor.effectIcon = -1;
         }

         actor.itemBuffElapsed = 0;
         actor.itemBuffDuration = (short)var1[5];
         removeItem(actor, 2, var1);
         recalcDerivedStats(actor);
      }
   }

   public static final void quaffPotion(Actor actor, boolean var1) {
      boolean var2 = false;
      if (var1) {
         if (actor.hpPotion != null && actor.hp < actor.maxHp) {
            removeItem(actor, 2, actor.hpPotion);
            actor.hp = (short)Math.min(actor.maxHp, actor.hp + actor.hpPotion[2]);
            recalcDerivedStats(actor);
            actor.hpPotion = null;

            for (int var6 = 0; var6 < actor.inventory.length; var6++) {
               int var3 = actor.inventory[var6] >> 8 & 0xFF;
               int var4 = actor.inventory[var6] >> 0 & 0xFF;
               int[] var5;
               if (var3 == 2 && (var5 = Game.script.getRow(2, var4))[2] > 0) {
                  actor.hpPotion = var5;
               }
            }
         }
      } else if (actor.mpPotion != null && actor.mp < actor.maxMp) {
         removeItem(actor, 2, actor.mpPotion);
         actor.mp = (short)Math.min(actor.maxMp, actor.mp + actor.mpPotion[3]);
         recalcDerivedStats(actor);
         actor.mpPotion = null;

         for (int var7 = 0; var7 < actor.inventory.length; var7++) {
            int var8 = actor.inventory[var7] >> 8 & 0xFF;
            int var9 = actor.inventory[var7] >> 0 & 0xFF;
            int[] var10;
            if (var8 == 2 && (var10 = Game.script.getRow(2, var9))[3] > 0) {
               actor.mpPotion = var10;
            }
         }
      }
   }

   public static final boolean equipFromString(Actor actor, String var1) {
      if (var1.startsWith(Game.getString(304))) {
         var1 = var1.substring(Game.getString(304).length());
         actor.altSpecial = Game.script.findByName(var1);
         actor.ranged = 0;
         if (actor.special != null) {
            actor.special = actor.altSpecial;
            updateSpecialIcon(actor);
         }

         return true;
      } else {
         if (var1.startsWith(Game.getString(400))) {
            var1 = var1.substring(Game.getString(400).length());
            actor.ranged = 1;
         } else {
            var1 = var1.substring(Game.getString(305).length());
            actor.ranged = 0;
         }

         int[] var2 = Game.script.findByName(var1);
         actor.weapon = (byte)var2[0];
         return false;
      }
   }

   public static final void applyPoison(Actor actor, Actor var1, int var2, int var3) {
      var1.dotDamage = (byte)var2;
      var1.dotRemaining = (short)var3;
      var1.dotSource = actor;
      var1.effectIcon = -47;
      ProjectileManager.spawn(8, var1);
      applyDamage(var2, var1, actor, false, true);
   }

   public static final void applyMagicHit(Actor actor, Actor var1, int var2) {
      ProjectileManager.spawn(10, var1);
      applyDamage(var2, var1, actor, false, false);
   }

   public static final void equipArmor(Actor actor, int[] var1) {
      if (var1 != null && canUseItem(actor, 1, var1)) {
         actor.wornArmor[var1[3]] = var1[0];
         recalcDerivedStats(actor);
      }
   }

   public static final boolean hasArmor(Actor actor, int var1) {
      boolean var2 = false;

      for (int var3 = 0; var3 < actor.wornArmor.length; var3++) {
         if (actor.wornArmor[var3] == var1) {
            return true;
         }
      }

      return false;
   }

   public static final boolean isWeaponEquipped(Actor actor, int var1, boolean var2) {
      return isWeaponRowEquipped(actor, Game.script.getRow(4, var1), var2);
   }

   public static final boolean isWeaponRowEquipped(Actor actor, int[] var1, boolean var2) {
      if (actor.altSpecial != null && var2) {
         return var1[0] == actor.altSpecial[0];
      } else {
         return !var2 ? actor.weapon == var1[0] : false;
      }
   }

   public static final void setDropsLoot(Actor actor, boolean var1) {
      actor.dropsLoot = (byte)(var1 ? 1 : 0);
   }

   public static final void levelUpTo(Actor actor, int var1) {
      while (actor.level < var1) {
         actor.level++;
         actor.strength++;
         actor.intelligence++;
         actor.willpower++;
         actor.agility++;
         actor.speed++;
         actor.endurance++;
         actor.personality++;
         applyLevelUpBonus(actor);
         actor.maxHp = (short)(actor.level * 4 + (actor.strength + actor.buffStrength) * 2 + actor.endurance * 2 + actor.bonusMaxHp);
         actor.hpRegenInterval = (short)(40000 / actor.maxHp);
         actor.maxMp = (short)(actor.level * 4 + actor.intelligence * 2 + actor.bonusMaxMp);
         actor.mpRegenInterval = (short)(40000 / actor.maxMp);
         recalcDerivedStats(actor);
      }
   }

   public static final boolean handleAction(Actor actor, int var1, long var2) {
      switch (var1) {
         case 2:
            if (actor.special == null && actor.altSpecial != null) {
               actor.special = actor.altSpecial;
            } else {
               actor.special = null;
            }

            updateSpecialIcon(actor);
            return true;
         case 3:
            setAnimState(actor, (byte)1);
            moveDir(actor, 2, var2);
            break;
         case 4:
            setAnimState(actor, (byte)1);
            moveDir(actor, 1, var2);
            break;
         case 5:
            setAnimState(actor, (byte)1);
            moveDir(actor, 4, var2);
            break;
         case 6:
            setAnimState(actor, (byte)1);
            moveDir(actor, 3, var2);
            break;
         case 7:
            checkZoneTiles(actor, Game.zoneLayer);
      }

      return false;
   }

   private static final void updateSpecialIcon(Actor actor) {
      if (actor.special != null) {
         switch (actor.special[2]) {
            case 0:
               actor.attackIcon = -48;
               break;
            case 1:
               actor.attackIcon = -50;
               break;
            case 2:
               actor.attackIcon = -46;
               break;
            case 3:
               if (actor.special[1] == 61618) {
                  actor.attackIcon = -44;
               } else if (actor.special[1] == 61619) {
                  actor.attackIcon = -43;
               } else {
                  actor.attackIcon = -50;
               }
               break;
            case 4:
               actor.attackIcon = -47;
               break;
            case 5:
               actor.attackIcon = -48;
               break;
            case 6:
               actor.attackIcon = -43;
         }
      } else {
         actor.attackIcon = -45;
      }
   }

   public static final boolean canUseItem(Actor actor, int var1, int[] var2) {
      if (actor != null && var2 != null && actor.classId != -1) {
         if (var1 == 0) {
            if (var2[2] == 1) {
               return Game.script.classAllows(actor.classId, 5);
            }

            if (var2[2] == 2) {
               return Game.script.classAllows(actor.classId, 6);
            }

            if (var2[2] == 3) {
               return Game.script.classAllows(actor.classId, 7);
            }

            if (var2[2] == 4) {
               return Game.script.classAllows(actor.classId, 8);
            }

            if (var2[2] == 0) {
               return Game.script.classAllows(actor.classId, 14);
            }
         } else if (var1 == 1) {
            if (var2[2] == 2) {
               return Game.script.classAllows(actor.classId, 4);
            }

            if (var2[2] == 1) {
               return Game.script.classAllows(actor.classId, 3);
            }

            if (var2[2] == 0) {
               return Game.script.classAllows(actor.classId, 1);
            }
         }

         return true;
      } else {
         return false;
      }
   }
}
