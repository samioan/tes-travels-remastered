// Renamed from decompiled/h.java. See ../docs/CLASS_MAP.md.
//
// Reuses Screen's mode dispatch for two unrelated purposes: mode 2 is
// the startup splash/carrier-logo sequence (runs its own background
// Thread), modes 8-11 are plain "<action>... Please Wait" progress bars
// (percent driven externally by whatever's loading, e.g. ESGame's
// background thread during a save/load).
import javax.microedition.lcdui.Graphics;

class LoadingScreen extends Screen implements Runnable {
   volatile int percent;
   private boolean running;
   private Thread thread = null;
   long elapsedMs;

   public LoadingScreen(ESGame game, int mode, int secondaryParam) {
      super(game, mode, secondaryParam);
   }

   // Empty -- called externally right after construction by
   // ESGame.initSplash() (`splashUI.c()` in the original), by its
   // original name. Purpose unconfirmed; preserved as a no-op.
   void unusedHook1() {
   }

   void unusedHook2() {
   }

   protected void onEnter() {
      switch (super.mode) {
         case 2:
            this.startThread();
      }
   }

   protected void onExit() {
      switch (super.mode) {
         case 1:
         case 2:
            this.stopThread();
      }
   }

   public void paint(Graphics g) throws Exception {
      switch (super.mode) {
         case 1:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7:
         default:
            break;
         case 2:
            this.renderSplash(g);
            break;
         case 8:
         case 9:
         case 10:
         case 11:
            this.renderProgress(g);
      }
   }

   private void startThread() {
      this.thread = new Thread(this);
      this.running = true;
      this.thread.start();
   }

   private void stopThread() {
      this.running = false;
   }

   public void run() {
      if (super.mode == 2) {
         this.runSplashSequence();
      } else if (super.mode == 1) {
      }
   }

   // The startup sequence: hold on the current splash frame until either
   // the boot progress (`percent`, driven by ESGame's background loader)
   // reaches 100 or at least 4 seconds have passed (repainting every
   // ~500ms), then show the Vir2L/ZeniMax copyright card for 2s, the
   // carrier logo for 1s, release every splash image, and hand off to
   // `returnDisplay`.
   private void runSplashSequence() {
      try {
         this.elapsedMs = 0L;

         while (this.running && (this.percent < 100 || this.elapsedMs < 4000L)) {
            long start = System.currentTimeMillis();
            super.canvas.repaint();
            super.canvas.serviceRepaints();
            long spent = System.currentTimeMillis() - start;

            try {
               if (spent < 500L) {
                  Thread.sleep(500L - spent);
               }
            } catch (Exception e) {
            }

            spent = System.currentTimeMillis() - start;
            this.elapsedMs += spent;
         }

         ESGame.showCarrierLogo = true;
         super.game.showSplash = false;
         this.waitAtLeast(2000L);
         super.game.showSplash = true;
         ESGame.showCarrierLogo = false;
         this.waitAtLeast(1000L);
         super.game.showSplash = false;
         ESGame.carrierLogoImage = null;
         ESGame.vir2lLogoImage = null;
         ESGame.splashImageTop = null;
         ESGame.splashImageBot = null;
         this.thread = null;
         super.game.setCurrentDisplay(super.returnDisplay);
      } catch (Throwable t) {
         t.printStackTrace();
         super.game.setCurrentDisplay(super.game.errorForm);
      }
   }

   private void waitAtLeast(long ms) {
      long waited = 0L;

      do {
         super.canvas.repaint();
         super.canvas.serviceRepaints();

         try {
            Thread.sleep(500L);
         } catch (Exception e) {
         }

         waited += 500L;
      } while (waited <= ms);
   }

   // Mode 2: Vir2L/ZeniMax copyright card, or carrier splash images +
   // a raw boot-progress bar.
   private void renderSplash(Graphics g) {
      g.setColor(0);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      if (ESGame.showCarrierLogo) {
         g.setColor(16777215);
         g.fillRect(0, 0, this.width(), 20 + this.height());
         g.drawImage(ESGame.vir2lLogoImage, this.width() / 2, 10, 17);
         int y = 10 + ESGame.vir2lLogoImage.getHeight() + 3;
         g.setColor(0);

         for (int i = 0; i < ESGame.copyString.length; i++) {
            g.drawString(ESGame.copyString[i], this.width() / 2, y, 17);
            y += 14;
         }

         g.drawString("Distributed by:", this.width() / 2, 143, 17);
         g.drawImage(ESGame.carrierLogoImage, this.width() / 2, 158, 17);
      } else if (super.game.showSplash) {
         g.setColor(2510210);
         g.fillRect(0, 0, this.width(), 20 + this.height());
         g.drawImage(ESGame.splashImageTop, this.width() / 2, 45, 17);
         g.drawImage(ESGame.splashImageBot, this.width() / 2, 115, 17);
      } else {
         g.setColor(2510210);
         g.fillRect(0, 0, this.width(), 20 + this.height());
         g.drawImage(ESGame.splashImageTop, this.width() / 2, 45, 17);
         g.drawImage(ESGame.splashImageBot, this.width() / 2, 115, 17);
         g.setColor(16777215);
         g.fillRect(12, 165, 152, 22);
         g.setColor(10485760);
         g.fillRect(13, 166, 3 * this.percent / 2, 20);
      }
   }

   // Modes 8-11: "<action>... Please Wait" + a percent progress bar.
   private void renderProgress(Graphics g) {
      g.setColor(2510210);
      g.fillRect(0, 0, this.width(), 20 + this.height());
      g.setFont(Screen.LARGE_TEXT_FONT);
      g.setColor(16777215);
      int cx = this.width() / 2;
      if (super.mode == 8) {
         g.drawString("Creating New Game", cx, 30, 17);
      } else if (super.mode == 9) {
         g.drawString("Loading Game", cx, 30, 17);
      } else if (super.mode == 10) {
         g.drawString("Saving Game", cx, 30, 17);
      } else if (super.mode == 11) {
         g.drawString("Loading Dungeon", cx, 30, 17);
      }

      g.drawString("Please Wait", cx, 45, 17);
      g.setColor(16777215);
      g.fillRect((this.width() - 90) / 2, 60, 90, 20);
      int barWidth = this.percent * 88 / 100;
      g.setColor(255);
      g.fillRect((this.width() - 88) / 2, 61, barWidth, 18);
   }
}
