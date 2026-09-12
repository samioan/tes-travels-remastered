#pragma once
#include <array>
#include <string>

#include "assets/dat_archive.h"

namespace dawnstar {

// Renamed-source counterpart of ESGame.java's monster_filenames --
// monsterfilenamesin.dat (../../docs/ASSET_FORMATS.md): a fixed 5x7
// grid of PNG filenames, one row per monster-type "bucket"
// (render/visible_object_renderer.h's kMonsterImageIndexInfo has the
// exact [objectSprites-start-index, count] pairs each bucket maps to --
// not every bucket uses all 7 slots).
struct MonsterImageNames {
    std::array<std::array<std::string, 7>, 5> names;

    static MonsterImageNames Load(DatArchive& archive);
};

}  // namespace dawnstar
