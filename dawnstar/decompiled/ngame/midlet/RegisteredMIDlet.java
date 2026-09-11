package ngame.midlet;

import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Item;
import javax.microedition.lcdui.StringItem;
import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

public abstract class RegisteredMIDlet extends MIDlet implements CommandListener {
   public static RegisteredMIDlet midlet;
   protected boolean verifyLicence = true;
   public String appName;
   public Display display = Display.getDisplay(this);
   static final int UNLOCK_CODE_KEY = 1;
   static final int USER_ID_KEY = 2;
   static final int PASSWORD_KEY = 3;
   private String unlockCode;
   public static final Command cmdExit = new Command("Exit", 7, 1);
   public static final Command cmdFatalExit = new Command("Exit", 7, 1);
   public static final Command cmdFinalExit = new Command("Exit", 7, 1);

   protected final void startApp() throws MIDletStateChangeException {
      this.startRegisteredApp();
   }

   protected abstract void startRegisteredApp() throws MIDletStateChangeException;

   public void pauseApp() {
   }

   public void destroyApp(boolean var1) {
   }

   public void exit() {
      this.finalExit();
   }

   private void finalExit() {
      this.destroyApp(true);
      this.notifyDestroyed();
   }

   private void registerApp() {
   }

   private void getUnlockCode() {
   }

   private void confirmedGetUnlockCode() {
   }

   private void checkUnlockCode(String var1) throws MIDletStateChangeException {
      this.startRegisteredApp();
   }

   public void errorAlert(String var1) {
   }

   public void fatalErrorAlert(String var1) {
      Form var2 = new Form("Fatal Error!", new Item[]{new StringItem(null, var1)});
      var2.addCommand(cmdFinalExit);
      var2.setCommandListener(this);
      this.display.setCurrent(var2);
   }

   public void billingEvent(String var1, String var2) {
   }

   public void postUserData(String var1, String var2) {
   }

   public void commandAction(Command var1, Displayable var2) {
      if (var1 == cmdExit) {
         this.exit();
      }
   }
}
