// M5 smoke test: JavaRandom must reproduce java.util.Random's nextInt()
// sequence bit-for-bit. Reference values below were captured from a real
// JVM (java.version 25-ish, this machine's `java`/`javac`) via:
//
//   Random r = new Random(seed);
//   for (i = 0; i < 8; i++) System.out.print(r.nextInt() + " ");
//
// for seeds {0, 42, 16000 (= DungeonGenerator's level-2 seed,
// level.number * 8000), -1, 4294967295 (2^32-1, exercises the seed's
// upper bits)}.
#include <cstdio>

#include "util/java_random.h"

namespace {

struct Case {
    int64_t seed;
    int32_t expected[8];
};

const Case kCases[] = {
    {0, {-1155484576, -723955400, 1033096058, -1690734402, -1557280266, 1327362106, -1930858313,
         502539523}},
    {42, {-1170105035, 234785527, -1360544799, 205897768, 1325939940, -248792245, 1190043011,
          -1255373459}},
    {16000, {1809790521, -1688846164, 798734875, 812685755, -1781096877, 1974701534, 473520795,
              -913331937}},
    {-1, {1155099827, 1887904451, 52699159, -1941176418, -1451336087, -1714570420, 1788588954,
          1714930956}},
    {4294967295LL, {730361011, -485875005, 1995120663, -1576599650, -333619607, -780092596,
                     -204295270, 1017300236}},
};

}  // namespace

int main() {
    bool ok = true;

    for (const Case& c : kCases) {
        dawnstar::JavaRandom rng(c.seed);
        std::printf("seed=%lld:", static_cast<long long>(c.seed));
        for (int32_t expected : c.expected) {
            int32_t actual = rng.NextInt();
            std::printf(" %d", actual);
            if (actual != expected) {
                std::printf("(FAIL, expected %d)", expected);
                ok = false;
            }
        }
        std::printf("\n");
    }

    if (!ok) {
        std::fprintf(stderr, "m5_java_random_smoke: FAILED -- output did not match the JVM reference\n");
        return 1;
    }

    std::printf("all sequences match the real JVM reference\n");
    return 0;
}
