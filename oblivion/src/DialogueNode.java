import java.util.Vector;

/**
 * Renamed from decompiled/c.java (see docs/CLASS_MAP.md).
 * One node of an NPC dialogue/choice tree, or of an equipment/spell
 * picker list (DialogueScreen -- decompiled/f.java -- uses the same
 * class for both: "children.size() == 0" means it's a leaf/actionable
 * line, non-empty means it's a submenu).
 */
public final class DialogueNode {
   public DialogueNode parent = null;
   // Single-select "radio group" marker: true for the currently
   // equipped/chosen sibling in an equipment- or spell-slot picker list.
   // DialogueScreen clears it on every sibling before setting it on the
   // newly picked leaf (see its select-handler, action code 7).
   public boolean marked = false;
   // Gates whether this leaf can be activated at all; false renders the
   // line in red and (outside the root Buy/Sell screens) blocks selection.
   public boolean available = true;
   public String text = null;
   public Vector children = new Vector();
   // Rumor/answer text shown when this node is picked, or null.
   public String[] answerLines = null;
   public String tooltip = null;

   public DialogueNode(String text, String tooltip, boolean marked) {
      this.text = text;
      this.tooltip = tooltip;
      this.marked = marked;
   }
}
