// Renamed from decompiled/b.java. See ../docs/CLASS_MAP.md and
// ../docs/ASSET_FORMATS.md (spellsin.dat).
import java.io.DataInputStream;

public class Spell {
   String name;
   byte skillRequired;
   byte magickaCost;
   byte power;
   byte school;
   byte durationMultiplier;
   byte icon;
   String description;
   static int count;
   static Spell[] all;

   Spell() {
   }

   private static int index0(int spellId) {
      return spellId - 1;
   }

   static Spell byId(int spellId) {
      return all[index0(spellId)];
   }

   static boolean isValidId(int spellId) {
      return spellId >= 1 && spellId <= count;
   }

   // school == 2 marks an offensive (monster-targeted) spell.
   static boolean isOffensive(int spellId) {
      int i = index0(spellId);
      return all[i].school == 2;
   }

   static void load() {
      try {
         DataInputStream in = ESGame.getResource("spellsin.dat");
         count = in.readShort();
         all = new Spell[count];

         for (int i = 0; i < count; i++) {
            all[i] = new Spell();
         }

         for (int i = 0; i < count; i++) {
            all[i].name = in.readUTF();
         }

         for (int i = 0; i < count; i++) {
            all[i].skillRequired = in.readByte();
         }

         for (int i = 0; i < count; i++) {
            all[i].magickaCost = in.readByte();
         }

         for (int i = 0; i < count; i++) {
            all[i].power = in.readByte();
         }

         for (int i = 0; i < count; i++) {
            all[i].school = in.readByte();
         }

         for (int i = 0; i < count; i++) {
            all[i].durationMultiplier = in.readByte();
         }

         for (int i = 0; i < count; i++) {
            all[i].icon = in.readByte();
         }

         for (int i = 0; i < count; i++) {
            all[i].description = in.readUTF();
         }

         in.close();
      } catch (Exception e) {
         System.out.println("ERROR: cannot load spells!");
         System.out.println(e);
      }
   }
}
