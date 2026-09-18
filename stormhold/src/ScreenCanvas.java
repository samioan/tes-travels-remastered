// Renamed from decompiled/c.java. See ../docs/CLASS_MAP.md.
//
// A thin `FullCanvas` adapter with no rendering logic of its own: every
// method forwards to a held `UIScreen` instance (see UIScreen.java, renamed
// from decompiled/h.java -- named UIScreen rather than "Screen" because
// that class itself holds a field of the real `javax.microedition.lcdui.
// Screen` type, which would collide with a same-named class in the same
// default package). `UIScreen` does the actual menu/dialog painting and
// command handling but does NOT itself extend FullCanvas -- this class
// exists purely so a `UIScreen` can be registered as a MIDP Displayable/
// Canvas. Two static instances of this class are created and alternated
// between (`UIScreen.canvasInstances[0]`/`[1]`) -- see UIScreen.java's
// static initializer and doc comment for why.
import com.nokia.mid.ui.FullCanvas;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Graphics;

public class ScreenCanvas extends FullCanvas {
   UIScreen owner = null;

   public void paint(Graphics g) {
      if (this.owner != null) {
         this.owner.paint(g);
      }
   }

   public void addCommand(Command cmd) {
      if (this.owner != null) {
         this.owner.addCommand(cmd);
      }
   }

   public void removeCommand(Command cmd) {
      if (this.owner != null) {
         this.owner.removeCommand(cmd);
      }
   }

   public void setCommandListener(CommandListener listener) {
      if (this.owner != null) {
         this.owner.setCommandListener(listener);
      }
   }

   protected void keyPressed(int key) {
      if (this.owner != null) {
         this.owner.keyPressed(key);
      }
   }
}
