#pragma once
#include <chrono>

namespace dawnstar {

// Dawnstar's engine runs a fixed 250ms (4Hz) tick -- GameCanvas.run()'s
// "steady 250ms tick via Thread.sleep" (../../src/GameCanvas.java, around
// line 1248). This reproduces that cadence as a frame-limiter: poll
// ConsumeTick() once per message-pump iteration and run one game tick each
// time it returns true.
class GameClock {
public:
    static constexpr std::chrono::milliseconds kTickInterval{250};

    GameClock() : last_(Clock::now()) {}

    // Returns true once per elapsed 250ms interval since construction/the
    // last true return. Accumulated time is capped at a few intervals so a
    // debugger pause or a minimized window doesn't cause a burst of
    // hundreds of catch-up ticks once the loop resumes.
    bool ConsumeTick() {
        Clock::time_point now = Clock::now();
        accumulated_ += now - last_;
        last_ = now;

        constexpr Clock::duration kMaxCatchUp = kTickInterval * 4;
        if (accumulated_ > kMaxCatchUp) accumulated_ = kMaxCatchUp;

        if (accumulated_ >= kTickInterval) {
            accumulated_ -= kTickInterval;
            return true;
        }
        return false;
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point last_;
    Clock::duration accumulated_{0};
};

}  // namespace dawnstar
