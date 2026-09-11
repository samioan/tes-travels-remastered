package blt;

import javax.microedition.lcdui.Display;
import javax.microedition.midlet.MIDlet;

public class Main extends MIDlet {
   private b a = null;

   public final void startApp() {
      if (this.a == null) {
         this.a = new b(this, "/startup.scr", "/oh_menu.cml", this.getAppProperty("MIDlet-Version"));
         Display.getDisplay(this).setCurrent(this.a);
         this.a.d();
      }
   }

   public final void pauseApp() {
      if (!this.a.c()) {
         this.a.d = true;
      }
   }

   public final void destroyApp(boolean var1) {
      this.a.c();
   }
}
