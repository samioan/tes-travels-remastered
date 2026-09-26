import org.jetbrains.java.decompiler.main.extern.IIdentifierRenamer;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/**
 * Vineflower identifier renamer for the obfuscated Oblivion classes.
 *
 * The original obfuscator reused one letter across fields of different JVM
 * descriptors, which is not valid Java once decompiled. This renamer gives every
 * field a name keyed on (class, name, descriptor):
 *   - class entries are "class:old=New"; method entries use the same
 *     "cls.name:desc=newName" form as fields;
 *   - entries in the map file (env RENAME_MAP, lines "cls.name:desc=newName")
 *     win;
 *   - otherwise, if RENAME_UNIQUIFY is set, name gets a descriptor suffix
 *     (b + [I -> b_aI) so nothing collides.
 */
public class MapRenamer implements IIdentifierRenamer {
  private final Map<String, String> map = new HashMap<>();
  private final boolean uniq = System.getenv("RENAME_UNIQUIFY") != null;

  public MapRenamer() throws IOException {
    String f = System.getenv("RENAME_MAP");
    if (f != null && Files.exists(Paths.get(f))) {
      for (String l : Files.readAllLines(Paths.get(f))) {
        l = l.trim();
        if (l.isEmpty() || l.startsWith("#")) continue;
        int eq = l.lastIndexOf('=');
        map.put(l.substring(0, eq).trim(), l.substring(eq + 1).trim());
      }
    }
  }

  private static String suffix(String d) {
    StringBuilder sb = new StringBuilder();
    int i = 0;
    while (d.charAt(i) == '[') { sb.append('a'); i++; }
    char c = d.charAt(i);
    switch (c) {
      case 'B': sb.append("By"); break;
      case 'S': sb.append("Sh"); break;
      case 'I': sb.append("In"); break;
      case 'Z': sb.append("Bo"); break;
      case 'C': sb.append("Ch"); break;
      case 'J': sb.append("Lo"); break;
      case 'L': {
        String n = d.substring(i + 1, d.length() - 1);
        sb.append(n.substring(n.lastIndexOf('/') + 1));
        break;
      }
      default: sb.append(c);
    }
    return sb.toString();
  }

  private String key(String cls, String name, String desc) { return cls + "." + name + ":" + desc; }

  public boolean toBeRenamed(Type t, String cls, String el, String desc) {
    if (t == Type.ELEMENT_CLASS) return map.containsKey("class:" + (cls != null ? cls : el));
    if (t == Type.ELEMENT_METHOD) return map.containsKey(key(cls, el, desc));
    return map.containsKey(key(cls, el, desc)) || uniq;
  }
  public String getNextClassName(String full, String shortName) {
    String v = map.get("class:" + full);
    return v != null ? v : shortName;
  }
  public String getNextFieldName(String cls, String f, String desc) {
    String v = map.get(key(cls, f, desc));
    if (v != null) return v;
    return f + "_" + suffix(desc);
  }
  public String getNextMethodName(String cls, String m, String desc) {
    String v = map.get(key(cls, m, desc));
    return v != null ? v : m;
  }
}
