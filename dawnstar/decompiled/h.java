import javax.microedition.lcdui.Graphics;

class h extends g implements Runnable {
   volatile int G;
   private boolean l;
   private Thread f = null;
   long d;

   public h(ESGame var1, int var2, int var3) {
      super(var1, var2, var3);
   }

   void c() {
   }

   void j() {
   }

   protected void e() {
      switch (super.h) {
         case 2:
            this.d();
      }
   }

   protected void l() {
      switch (super.h) {
         case 1:
         case 2:
            this.f();
      }
   }

   public void b(Graphics var1) throws Exception {
      switch (super.h) {
         case 1:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7:
         default:
            break;
         case 2:
            this.h(var1);
            break;
         case 8:
         case 9:
         case 10:
         case 11:
            this.e(var1);
      }
   }

   private void d() {
      this.f = new Thread(this);
      this.l = true;
      this.f.start();
   }

   private void f() {
      this.l = false;
   }

   public void run() {
      if (super.h == 2) {
         this.i();
      } else if (super.h == 1) {
      }
   }

   private void i() {
      try {
         this.d = 0L;

         while (this.l && (this.G < 100 || this.d < 4000L)) {
            long var1 = System.currentTimeMillis();
            super.C.repaint();
            super.C.serviceRepaints();
            long var3 = System.currentTimeMillis() - var1;

            try {
               if (var3 < 500L) {
                  long var5 = 500L - var3;
                  Thread.sleep(500L - var3);
               }
            } catch (Exception var7) {
            }

            var3 = System.currentTimeMillis() - var1;
            this.d += var3;
         }

         ESGame.showCarrierLogo = true;
         super.w.showSplash = false;
         this.a(2000L);
         super.w.showSplash = true;
         ESGame.showCarrierLogo = false;
         this.a(1000L);
         super.w.showSplash = false;
         ESGame.carrierLogoImage = null;
         ESGame.vir2lLogoImage = null;
         ESGame.splashImageTop = null;
         ESGame.splashImageBot = null;
         this.f = null;
         super.w.setCurrentDisplay(super.t);
      } catch (Throwable var8) {
         var8.printStackTrace();
         super.w.setCurrentDisplay(super.w.errorForm);
      }
   }

   private void a(long var1) {
      long var3 = 0L;

      do {
         super.C.repaint();
         super.C.serviceRepaints();

         try {
            Thread.sleep(500L);
         } catch (Exception var6) {
         }

         var3 += 500L;
      } while (var3 <= var1);
   }

   private void h(Graphics var1) {
      var1.setColor(0);
      var1.fillRect(0, 0, this.b(), 20 + this.h());
      if (ESGame.showCarrierLogo) {
         var1.setColor(16777215);
         var1.fillRect(0, 0, this.b(), 20 + this.h());
         var1.drawImage(ESGame.vir2lLogoImage, this.b() / 2, 10, 17);
         int var2 = 10 + ESGame.vir2lLogoImage.getHeight() + 3;
         var1.setColor(0);

         for (int var3 = 0; var3 < ESGame.copyString.length; var3++) {
            var1.drawString(ESGame.copyString[var3], this.b() / 2, var2, 17);
            var2 += 14;
         }

         var1.drawString("Distributed by:", this.b() / 2, 143, 17);
         var1.drawImage(ESGame.carrierLogoImage, this.b() / 2, 158, 17);
      } else if (super.w.showSplash) {
         var1.setColor(2510210);
         var1.fillRect(0, 0, this.b(), 20 + this.h());
         var1.drawImage(ESGame.splashImageTop, this.b() / 2, 45, 17);
         var1.drawImage(ESGame.splashImageBot, this.b() / 2, 115, 17);
      } else {
         var1.setColor(2510210);
         var1.fillRect(0, 0, this.b(), 20 + this.h());
         var1.drawImage(ESGame.splashImageTop, this.b() / 2, 45, 17);
         var1.drawImage(ESGame.splashImageBot, this.b() / 2, 115, 17);
         var1.setColor(16777215);
         var1.fillRect(12, 165, 152, 22);
         var1.setColor(10485760);
         var1.fillRect(13, 166, 3 * this.G / 2, 20);
      }
   }

   private void e(Graphics var1) {
      var1.setColor(2510210);
      var1.fillRect(0, 0, this.b(), 20 + this.h());
      var1.setFont(g.D);
      var1.setColor(16777215);
      int var2 = this.b() / 2;
      if (super.h == 8) {
         var1.drawString("Creating New Game", var2, 30, 17);
      } else if (super.h == 9) {
         var1.drawString("Loading Game", var2, 30, 17);
      } else if (super.h == 10) {
         var1.drawString("Saving Game", var2, 30, 17);
      } else if (super.h == 11) {
         var1.drawString("Loading Dungeon", var2, 30, 17);
      }

      var1.drawString("Please Wait", var2, 45, 17);
      var1.setColor(16777215);
      var1.fillRect((this.b() - 90) / 2, 60, 90, 20);
      int var3 = this.G * 88 / 100;
      var1.setColor(255);
      var1.fillRect((this.b() - 88) / 2, 61, var3, 18);
   }
}
