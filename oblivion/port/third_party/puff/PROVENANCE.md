# Vendored: puff (zlib's reference inflate)

- Source: https://raw.githubusercontent.com/madler/zlib/master/contrib/puff/{puff.c,puff.h}
- Copied from `dawnstar/port/third_party/puff/` (sibling project in this
  same monorepo, same upstream fetch -- see its own PROVENANCE.md for the
  original fetch date and the shadowkey-decomp chain it came from before
  that).
- License: zlib License (see `LICENSE.txt`, copied from the zlib project's
  own top-level LICENSE, the same terms appear inline in puff.h's header).
  Copyright Mark Adler.
- Unmodified upstream source. `puff()` decompresses **raw DEFLATE** data
  only (not the zlib-wrapped stream format) -- exactly the format a zip
  archive's `method == 8` (deflate) entries store, with no zlib header or
  Adler-32 trailer to skip (see `port/src/launcher/zip_reader.cpp`'s
  `Inflate()`).
- Why this instead of vendoring full zlib: the launcher only ever needs to
  *decompress* zip entries (the initial `.jar` the user provides, and
  update `.zip`s downloaded from GitHub releases) -- `puff` is the zlib
  project's own ~800-line single-purpose reference decoder for exactly
  this (no build system, no huge API surface), a much smaller footprint
  for a real, well-tested implementation of the same algorithm than
  writing one from scratch.
