# Stormhold -- asset formats

All paths below are relative to `extracted/` (regenerate with
`python ../../tools/extract_jar.py stormhold`).

| File(s) | Format | Notes |
|---|---|---|
| `*.png` (icons, splash, UI) | PNG | Standard, no RE needed. |
| `itemsin.dat` | Custom, mostly understood | `00 11 00 03 "Axe" 00 0c "Blunt Weapon" 00 0a "Long Sword" ...` -- leading `u16` (`0x0011` = 17) is almost certainly an entry count, then each entry is `<u16 length><UTF-8 bytes>`, i.e. Java's `DataOutputStream.writeUTF` framing. Confirm the full record shape (is a name *all* there is per item, or are there trailing numeric fields per record?) against the loader in `decompiled/`. |
| `monstersin.dat`, `spellsin.dat`, `dungnamesin.dat`, `droppeditemsin.dat`, `charin.dat`, `monsterfilenamesin.dat`, `npcstrings.dat` | Custom, unconfirmed | Not yet hex-inspected individually; likely close relatives of `itemsin.dat`'s record framing given they're loaded by the same engine. Check each against its loader rather than assuming. |
| `geomin.dat` | Custom, unconfirmed | Name suggests dungeon/level geometry -- highest-value target once the simpler `*in.dat` tables are cracked, since it's probably what actually defines playable spaces. |
| `*.cus` (`baglarge.cus`, `bagmid.cus`, `bagsmall.cus`, `chestfarclosed.cus`, `chestmidclosed.cus`, `chestnearclosed.cus`, `crystalfar.cus`, `crystalmid.cus`, `crystalnear.cus`, `overseeraxe.cus`, `overseerbodyf3lc.cus`, `overseerclub.cus`, `overseerfar.cus`, `overseerhelmet.cus`, `overseermidcf.cus`, `trainer_male_*.cus`, `trainerfem*.cus`, `undead*.cus`, `wardenbody2f2half.cus`, `wardenfar.cus`, `wardenheads4bit.cus`, `wardenmid.cus`) | Custom binary, unconfirmed | Naming strongly suggests per-bodypart/prop 3D models (LOD suffixes `far`/`mid`/`near` on the same object appear repeatedly -- e.g. `crystalfar/mid/near.cus`, `chest{far,mid,near}closed.cus` -- i.e. distance-based level-of-detail meshes). Binary, no ASCII structure visible in a raw hexdump; needs the loader code read through to get field widths. |

## How to make progress on these

Same approach as dawnstar: grep `decompiled/*.java` for the literal
filename (or its `*in.dat` stem) to find the loader, then read its
`DataInputStream`/array-index calls off in order. `itemsin.dat`'s partial
decode above came from the raw bytes alone and should be treated as a
hypothesis to confirm against `d.java`/`j.java` (both reference `itemsin`
per the initial grep), not a spec.
