import com.nokia.mid.ui.FullCanvas;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Graphics;

public class c extends FullCanvas {
   h a = null;

   public void paint(Graphics var1) {
      if (this.a != null) {
         this.a.e(var1);
      }
   }

   public void addCommand(Command var1) {
      if (this.a != null) {
         this.a.a(var1);
      }
   }

   public void removeCommand(Command var1) {
      if (this.a != null) {
         this.a.b(var1);
      }
   }

   public void setCommandListener(CommandListener var1) {
      if (this.a != null) {
         this.a.a(var1);
      }
   }

   protected void keyPressed(int var1) {
      if (this.a != null) {
         this.a.b(var1);
      }
   }
}
