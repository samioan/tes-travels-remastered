#include "world/dungeon.h"

#include <algorithm>
#include <cstdlib>

namespace oblivion {

namespace {

class Generator {
public:
    Generator(const int* g, const std::function<int()>& rng) : g_(g), rng_(rng) {
        w_ = static_cast<int8_t>(g[1]);
        h_ = static_cast<int8_t>(g[2]);
    }

    Dungeon Run(int zone, int exitScript) {
        const size_t cells = static_cast<size_t>(w_) * static_cast<size_t>(h_);
        d_.map.width = w_;
        d_.map.height = h_;
        d_.map.layers.assign(4, std::vector<uint8_t>(cells, 0));
        d_.enter.assign(cells, -1);
        d_.leave.assign(cells, -1);
        d_.zone.assign(cells, -1);
        collision_ = &d_.map.layers[0];
        floor_ = &d_.map.layers[1];
        std::fill(collision_->begin(), collision_->end(), uint8_t{1});
        std::fill(floor_->begin(), floor_->end(), static_cast<uint8_t>(g_[3]));

        const int start[2] = {2, 2};
        const int end[2] = {w_ - 2, h_ - 2};
        CarvePath(start, end, 1);
        CarveEndpoints(start, end);
        PickWallTiles(*floor_, d_.map.layers[2]);
        std::vector<uint8_t>& objects = d_.map.layers[3];
        objects[static_cast<size_t>(start[0] * h_ + start[1])] = static_cast<uint8_t>(g_[13]);
        objects[static_cast<size_t>(end[0] * h_ + end[1])] = static_cast<uint8_t>(g_[13]);
        d_.enter[static_cast<size_t>(end[0] * h_ + end[1])] = static_cast<int8_t>(exitScript);
        d_.leave[static_cast<size_t>(start[0] * h_ + start[1])] = -2;
        d_.enter[static_cast<size_t>(start[0] * h_ + start[1])] = -2;
        d_.zone[static_cast<size_t>(start[0] * h_ + start[1])] = static_cast<int8_t>(zone);
        d_.start[0] = start[0];
        d_.start[1] = start[1];
        d_.end[0] = end[0];
        d_.end[1] = end[1];
        d_.branchPoints.assign(branchPoints_, branchPoints_ + branchPointCount_);
        return std::move(d_);
    }

private:
    void CarveCell(int x, int y) {
        const int i = x * h_ + y;
        if (i < static_cast<int>(collision_->size()) && i >= 0) {
            (*floor_)[static_cast<size_t>(i)] = static_cast<uint8_t>(g_[4]);
            (*collision_)[static_cast<size_t>(i)] = 0;
        }
    }

    // Carves a `g_[14]`-wide corridor from `from` towards `to`, one step at a
    // time along a random axis, occasionally spawning a side branch that heads
    // for a random point on another edge. `heading`: 1 +y, 2 -y, 3 +x, 4 -x.
    void CarvePath(const int from[2], const int to[2], int heading) {
        int at[2] = {from[0], from[1]};
        int dir = heading;
        bool done = false;
        const int width = g_[14];
        auto carveRun = [&](bool remember) {
            const bool branch = rng_() % g_[19] == 0;
            segments_++;
            if (dir == 1 || dir == 2)
                for (int i = 0; i < width; i++) CarveCell(at[0] + i, at[1]);
            else if (dir == 3 || dir == 4)
                for (int i = 0; i < width; i++) CarveCell(at[0], at[1] + i);
            if (remember && branch && branchPointCount_ < 50 && segments_ > 10) {
                branchPoints_[branchPointCount_++] = at[0] + (dir == 1 || dir == 2 ? 1 : 0);
                branchPoints_[branchPointCount_++] = at[1] + (dir == 3 || dir == 4 ? 1 : 0);
            }
        };
        while (!done) {
            if (rng_() % g_[16] == 0 && ++branches_ < g_[15]) {
                int side;
                while ((side = std::abs(rng_()) % 4 + 1) == dir) {
                }
                const int along = std::abs(rng_() % std::min<int>(w_, h_));
                int target[2];
                if (side == 2) { target[0] = along; target[1] = 3; }
                else if (side == 1) { target[0] = along; target[1] = h_ - 3; }
                else if (side == 3) { target[0] = w_ - 3; target[1] = along; }
                else { target[0] = 3; target[1] = along; }
                CarvePath(at, target, side);
            }

            if (dir == 1 || dir == 2 || dir == 3 || dir == 4) carveRun(true);

            // Pick the next step: a random axis that still gets closer.
            for (int tries = 0;; tries++) {
                if (tries > 10000) return;  // the original can spin forever here
                const int r = std::abs(rng_() % 4);
                if (to[0] < at[0]) {
                    if (r == 1) dir = 2;
                    if (r == 2) dir = 1;
                    if (r == 3) dir = 4;
                } else if (to[0] > at[0]) {
                    if (r == 1) dir = 2;
                    if (r == 2) dir = 1;
                    if (r == 3) dir = 3;
                } else if (r < 2) {
                    dir = 2;
                } else {
                    dir = 1;
                }
                if (dir == 1 && at[1] < to[1]) { at[1]++; break; }
                if (dir == 3 && at[0] < to[0]) { at[0]++; break; }
                if (dir == 2 && at[1] > to[1]) { at[1]--; break; }
                if (dir == 4 && at[0] > to[0]) { at[0]--; break; }
            }
            done = at[0] == to[0] && at[1] == to[1];
        }
        // The final segment.
        segments_++;
        if (dir == 1 || dir == 2)
            for (int i = 0; i < width; i++) CarveCell(at[0] + i, at[1]);
        else if (dir == 3 || dir == 4)
            for (int i = 0; i < width; i++) CarveCell(at[0], at[1] + i);
    }

    // A 3x3 clearing round the entry stairs (walkable) and exit stairs (floor only).
    void CarveEndpoints(const int start[2], const int end[2]) {
        static const int8_t kAround[9][2] = {{-1, 1}, {-1, 0}, {-1, -1}, {0, 1}, {0, 0}, {0, -1}, {1, 1}, {1, 0}, {1, -1}};
        const int total = static_cast<int>(floor_->size());
        for (const auto& d : kAround) {
            const int i = (start[0] + d[0]) * h_ + start[1] + d[1];
            if (i < total && i >= 0) {
                (*floor_)[static_cast<size_t>(i)] = static_cast<uint8_t>(g_[4]);
                (*collision_)[static_cast<size_t>(i)] = 0;
            }
        }
        for (const auto& d : kAround) {
            const int i = (end[0] + d[0]) * h_ + end[1] + d[1];
            if (i < total && i >= 0) (*floor_)[static_cast<size_t>(i)] = static_cast<uint8_t>(g_[4]);
        }
    }

    // Wall edge tiles on the rock cells' side of every path cell.
    void PickWallTiles(const std::vector<uint8_t>& floor, std::vector<uint8_t>& walls) {
        const int total = static_cast<int>(floor.size());
        auto in = [&](int i) { return i >= 0 && i <= total - 1; };
        auto at = [&](int i) { return floor[static_cast<size_t>(i)]; };
        const int rock = g_[3], path = g_[4];
        for (int x = 0; x < w_; x++) {
            for (int y = 0; y < h_; y++) {
                const int i = x * h_ + y;
                if (!(in(i) && in(x * h_ + y - 1) && in(x * h_ + y + 1) && in((x - 1) * h_ + y) &&
                      in((x + 1) * h_ + y) && at(i) == path))
                    continue;
                auto set = [&](int tile) { walls[static_cast<size_t>(i)] = static_cast<uint8_t>(tile); };
                if (at(x * h_ + y - 1) == rock) {
                    if (at((x - 1) * h_ + y) == rock) set(g_[9]);
                    else if (at((x + 1) * h_ + y) == rock) set(g_[10]);
                    else set(g_[5]);
                } else if (at(x * h_ + y + 1) == rock) {
                    if (at((x - 1) * h_ + y) == rock) set(g_[11]);
                    else if (at((x + 1) * h_ + y) == rock) set(g_[12]);
                    else set(g_[6]);
                } else if (at((x + 1) * h_ + y) == rock) {
                    set(g_[7]);
                } else if (at((x - 1) * h_ + y) == rock) {
                    set(g_[8]);
                }
            }
        }
    }

    const int* g_;
    const std::function<int()>& rng_;
    int8_t w_ = 0, h_ = 0;
    Dungeon d_;
    std::vector<uint8_t>* collision_ = nullptr;
    std::vector<uint8_t>* floor_ = nullptr;
    int branches_ = 0, segments_ = 0;
    int branchPoints_[50] = {};
    int branchPointCount_ = 0;
};

}  // namespace

Dungeon GenerateDungeon(const int* group, int zone, int exitScript, const std::function<int()>& rng) {
    return Generator(group, rng).Run(zone, exitScript);
}

}  // namespace oblivion
