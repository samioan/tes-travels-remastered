package blt;

import javax.microedition.lcdui.Display;
import javax.microedition.midlet.MIDlet;

/**
 * MIDlet entry point (MIDlet-1: Elder Scrolls,/icon.png,blt.Main). Builds the
 * single Game canvas on first start and forwards pause/destroy to it.
 * (Game lives in the default package, as in the original bytecode; Java
 * source cannot import a default-package class, so this file is not
 * compilable as-is -- everything else in src/ is.)
 */
public class Main extends MIDlet {
   private Game game = null;

   public final void startApp() {
      if (this.game == null) {
         this.game = new Game(this, "/startup.scr", "/oh_menu.cml", this.getAppProperty("MIDlet-Version"));
         Display.getDisplay(this).setCurrent(this.game);
         this.game.start();
      }
   }

   public final void pauseApp() {
      if (!this.game.isLoadingState()) {
         this.game.suspended = true;
      }
   }

   public final void destroyApp(boolean var1) {
      this.game.quit();
   }
}
