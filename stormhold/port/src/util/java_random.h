#pragma once
#include <cstdint>

namespace stormhold {

// Bit-exact port of java.util.Random's 48-bit LCG. This is load-bearing,
// not cosmetic: Dungeon.generate() seeds one of these deterministically
// per level (`new Random(seed)`, seed = `levelNumber * 5000` -- confirmed
// in `../src/Dungeon.java`'s own header comment, NOT dawnstar's
// `levelNumber * 8000`), so every level's room layout, monster spawns, and
// loot placement are a pure function of the level number in the original
// game. Identical algorithm to dawnstar's own JavaRandom (java.util.Random
// itself doesn't differ between the two games -- only the seed formula and
// call-site naming do, see below), reproduced here rather than shared
// across projects per docs/PORT_ROADMAP.md's "duplicated for now" note.
class JavaRandom {
public:
    explicit JavaRandom(int64_t seed) { SetSeed(seed); }

    void SetSeed(int64_t seed) { seed_ = (seed ^ kMultiplier) & kMask; }

    // java.util.Random.next(32), inlined for the one bit-width this
    // codebase ever needs.
    int32_t NextInt() {
        seed_ = (seed_ * kMultiplier + kAddend) & kMask;
        return static_cast<int32_t>(static_cast<uint64_t>(seed_) >> 16);
    }

private:
    static constexpr int64_t kMultiplier = 0x5DEECE66DLL;
    static constexpr int64_t kAddend = 0xBLL;
    static constexpr int64_t kMask = (1LL << 48) - 1;

    int64_t seed_ = 0;
};

// Mirrors Java's Math.abs(int) exactly, including its one quirk:
// Integer.MIN_VALUE has no positive two's-complement counterpart, so Java
// returns it unchanged (still negative) instead of overflowing. Plain
// `-v`/std::abs(int) is undefined behavior in C++ for INT_MIN; this
// reproduces Java's defined (if odd) behavior instead of "fixing" it.
inline int32_t JavaAbs(int32_t v) {
    return v < 0 ? static_cast<int32_t>(0u - static_cast<uint32_t>(v)) : v;
}

// 1-based: 1 + abs(rng.nextInt() % bound). Matches ESGame.nextInt(bound)
// [the global ESGame.rng] AND ESGame.randomInt(Random, bound)/
// Util.randomInt(Random, bound) [an explicit rng, e.g. Dungeon's own
// per-level generator RNG] -- both are the exact same formula in
// ../src/ESGame.java (lines 2245-2246 and 2249-2250), just invoked on a
// different backing java.util.Random instance. No separate C++ overload
// needed for the two since JavaRandom is always passed explicitly here.
//
// **NAMING WARNING, confirmed directly from source, do not assume parity
// with dawnstar:** dawnstar's own JavaRandom calls this exact 1-based
// formula `LingoRandomInt`. Stormhold's decompiled/renamed source uses
// the name `nextInt`/`randomInt` for it instead, and reserves
// `lingoRandomInt` for the *0-based* formula below -- the two games'
// naming is swapped relative to each other. This port keeps each game's
// own real behavior; see RandomInt0Based's comment for the other half.
inline int32_t RandomInt1Based(JavaRandom& rng, int32_t bound) {
    return 1 + JavaAbs(rng.NextInt() % bound);
}

// 0-based: abs(rng.nextInt() % bound), no "+1". Matches
// ESGame.lingoRandomInt(bound) (../src/ESGame.java lines 2241-2242) AND
// every inline `Math.abs(this.rng.nextInt() % bound)` call site in
// Dungeon.java/Item.java, which don't route through a named helper at
// all -- they spell the same formula out directly at each call site.
inline int32_t RandomInt0Based(JavaRandom& rng, int32_t bound) {
    return JavaAbs(rng.NextInt() % bound);
}

}  // namespace stormhold
