/**
 * Renamed from decompiled/d.java (see docs/CLASS_MAP.md).
 * One entry of a parsed .cml resource. The same struct plays two roles,
 * distinguished only by which chain field is walked:
 *  - a top-level "group" node, one per animation/state id, linked
 *    through {@link #nextGroup} and found by id via
 *    SpriteRenderer.findGroup(SpriteFrame, int);
 *  - a "subframe" node within a group's animation cycle, linked through
 *    {@link #nextFrame}. {@link #currentFrame} on the group head is the
 *    playback cursor, advanced/reset by SpriteRenderer.
 * See docs/ASSET_FORMATS.md for the binary layout this is parsed from.
 */
public final class SpriteFrame {
   public String imagePath;
   // Animation/state id this group represents (top-level nodes only).
   public byte groupId;
   // Pixel offset added to the draw position.
   public byte frameDx;
   public byte frameDy;
   // Controls whether SpriteRenderer.advanceFrame loops back to the
   // first subframe or freezes on the last one when a cycle ends.
   // UNCONFIRMED beyond that one call site.
   public byte holdFlag;
   // 1 = draw via a cached javax.microedition.lcdui.game.Sprite (mirrored
   // transform), 0 = plain Graphics.drawImage.
   public byte isSprite;
   // Set to 1 on every subframe of a multi-frame (animated) group; never
   // observed being read in this pass. UNCONFIRMED purpose.
   public byte frameChainFlag;
   public short offsetX;
   public short offsetY;
   public short width;
   public short height;
   public SpriteFrame nextFrame;
   public SpriteFrame currentFrame;
   public SpriteFrame nextGroup;
}
