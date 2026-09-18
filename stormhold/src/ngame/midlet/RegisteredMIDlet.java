// Renamed from decompiled/ngame/midlet/a.java. See ../../../docs/CLASS_MAP.md.
//
// Unlike dawnstar (whose decompiled/ngame/midlet/RegisteredMIDlet.java came
// out of Vineflower with its real name already, having apparently been
// compiled without the licensing/registration stub methods stripped), this
// build's copy of the shared Vir2L "ngame" MIDlet-lifecycle base class *is*
// single-letter obfuscated, and is missing the trial/unlock-code gating and
// billing/error-alert stub methods dawnstar's copy has entirely (registerApp,
// getUnlockCode, confirmedGetUnlockCode, checkUnlockCode, errorAlert,
// fatalErrorAlert, billingEvent, postUserData) -- either a leaner build of
// the same engine class or dead code the compiler/obfuscator stripped as
// provably unused. Confirmed identity from the surviving structure: MIDlet
// lifecycle (startApp delegating to an abstract hook, empty pauseApp/
// destroyApp), a Display field, three identical "Exit" Commands, and an
// exit -> destroyApp(true)+notifyDestroyed() chain wired through
// commandAction -- the same shape as dawnstar's RegisteredMIDlet minus the
// registration/billing stubs.
package ngame.midlet;

import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

public abstract class RegisteredMIDlet extends MIDlet implements CommandListener {
   protected boolean verifyLicence = true;
   public String appName;
   public Display display = Display.getDisplay(this);
   public static final Command cmdExit = new Command("Exit", 7, 1);
   public static final Command cmdFatalExit = new Command("Exit", 7, 1);
   public static final Command cmdFinalExit = new Command("Exit", 7, 1);

   protected final void startApp() throws MIDletStateChangeException {
      this.startRegisteredApp();
   }

   protected abstract void startRegisteredApp() throws MIDletStateChangeException;

   public void pauseApp() {
   }

   public void destroyApp(boolean unconditional) {
   }

   public void exit() {
      this.finalExit();
   }

   private void finalExit() {
      this.destroyApp(true);
      this.notifyDestroyed();
   }

   public void commandAction(Command cmd, Displayable src) {
      if (cmd == cmdExit) {
         this.exit();
      }
   }
}
