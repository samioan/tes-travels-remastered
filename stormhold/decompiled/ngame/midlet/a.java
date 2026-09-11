package ngame.midlet;

import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

public abstract class a extends MIDlet implements CommandListener {
   protected boolean Z = true;
   public String aF;
   public Display d = Display.getDisplay(this);
   public static final Command aK = new Command("Exit", 7, 1);
   public static final Command r = new Command("Exit", 7, 1);
   public static final Command aX = new Command("Exit", 7, 1);

   protected final void startApp() throws MIDletStateChangeException {
      this.m();
   }

   protected abstract void m() throws MIDletStateChangeException;

   public void pauseApp() {
   }

   public void destroyApp(boolean var1) {
   }

   public void b() {
      this.n();
   }

   private void n() {
      this.destroyApp(true);
      this.notifyDestroyed();
   }

   public void commandAction(Command var1, Displayable var2) {
      if (var1 == aK) {
         this.b();
      }
   }
}
