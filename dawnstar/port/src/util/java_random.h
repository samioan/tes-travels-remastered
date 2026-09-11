#pragma once
#include <cstdint>

namespace dawnstar {

// Bit-exact port of java.util.Random's 48-bit LCG. This is load-bearing,
// not cosmetic: DungeonGenerator.java seeds one of these deterministically
// per level (`new Random(level.number * 8000)`), so every level's room
// layout, monster spawns, and loot placement are a pure function of the
// level number in the original game -- reproducing that exactly (not just
// "a similar-quality PRNG") is what makes a future DungeonGenerator port
// generate the same dungeons the original does. ESGame.r (the global
// gameplay RNG, seeded from System.currentTimeMillis()) doesn't need this
// same bit-exactness against any specific run, but uses the identical
// algorithm here too rather than a second, different RNG.
//
// Every real caller in ../../src/ only ever uses the no-arg nextInt() (see
// Item.java/DungeonGenerator.java/ESGame.java/Monster.java) and does its
// own Math.abs(x % bound) wrapping -- never java.util.Random's more
// involved rejection-sampling nextInt(bound). So that's the only surface
// ported here.
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
// reproduces Java's defined (if odd) behavior instead of "fixing" it, so
// callers matching the original's bound-wrapping idiom
// (Math.abs(rng.nextInt() % bound)) get the exact same rare edge case the
// original game has.
inline int32_t JavaAbs(int32_t v) {
    return v < 0 ? static_cast<int32_t>(0u - static_cast<uint32_t>(v)) : v;
}

// ESGame.lingoRandomInt(Random, int)/Util.randomInt(Random, int): 1 +
// Math.abs(rng.nextInt() % bound). Named for Lingo (Macromedia
// Director's scripting language, whose `random()` is 1-based inclusive)
// -- see ESGame.java's own naming, a hint this engine started life as a
// Director/Shockwave title.
inline int32_t LingoRandomInt(JavaRandom& rng, int32_t bound) {
    return 1 + JavaAbs(rng.NextInt() % bound);
}

// ESGame.nextInt(int)/the global-RNG equivalent: Math.abs(r.nextInt() %
// bound), zero-based (no "+1") -- distinct from LingoRandomInt above.
inline int32_t RandomIntBelow(JavaRandom& rng, int32_t bound) {
    return JavaAbs(rng.NextInt() % bound);
}

}  // namespace dawnstar
