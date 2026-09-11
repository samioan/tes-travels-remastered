# Dawnstar -- asset formats

All paths below are relative to `extracted/` (regenerate with
`python ../../tools/extract_jar.py dawnstar`).

| File | Format | Notes |
|---|---|---|
| `icon3650.png`, `splashbot.png`, `splashtop.png` | PNG | Standard, no RE needed. |
| `npcstrings.dat` | Custom, partially understood | Starts `00 00 00 03`, then repeats `<u16 length><UTF-8 bytes>` -- looks like Java's `DataOutputStream.writeUTF` framing (2-byte big-endian length, no null terminator), which is idiomatic for hand-rolled MIDP resource I/O. Leading `00 00 00 03` may be an int32 entry count -- unconfirmed. |
| `datfiles.lmp` | Custom archive, partially understood | Starts `-charin.dat-` then `00 00 04 ec` (1260 = the byte size of stormhold's standalone `charin.dat`). Pattern looks like `-<name>-<u32 size><bytes>` repeated: a simple named-blob archive bundling the same data tables stormhold ships as loose files (`charin.dat`, `droppeditemsin.dat`, `geomin.dat`, ...). Cross-reference against stormhold's individual `.dat` files once those are understood -- likely the exact same per-file layout, just concatenated. |
| `imgfiles.lmp` | Custom archive, unconfirmed | Same likely `-<name>-<u32 size><bytes>` framing as `datfiles.lmp`, bundling image resources instead. Not yet hex-inspected. |

## How to make progress on these

The real spec lives in the loader code, not guesswork from hexdumps --
once `decompiled/*.java` is read through (see `ROADMAP.md` phase 1), find
whichever class opens `datfiles.lmp`/`imgfiles.lmp`/`npcstrings.dat` (grep
decompiled sources for those literal filenames) and read its
`DataInputStream` calls off in order; that's the authoritative format.
Write a `tools/parse_*.py` once confirmed, matching the
shadowkey-decomp convention of one small parser script per format.
