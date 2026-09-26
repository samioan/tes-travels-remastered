#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "assets/image.h"

namespace oblivion {

// One drawable piece: a window into a source PNG (SpriteFrame in src/). For a
// static image (frameChain == 0) the whole PNG is drawn at (dx, dy); for an
// animation subframe the rectangle (offsetX, offsetY, width, height) of the
// sheet is drawn at (dx, dy).
struct SpriteFrame {
    std::string imagePath;
    int8_t groupId = 0;
    int8_t dx = 0, dy = 0;
    int8_t hold = 0;      // 1 = animation loops, else it freezes on the last frame
    int8_t isSprite = 0;  // 1 = drawn mirrored horizontally
    int8_t frameChain = 0;
    int16_t offsetX = 0, offsetY = 0, width = 0, height = 0;
};

// A top-level group (SpriteRenderer.findGroup finds these by id): an
// animation of one or more frames with a playback cursor.
struct SpriteGroup {
    int8_t groupId = 0;
    int8_t hold = 0;
    std::vector<SpriteFrame> frames;
    size_t current = 0;
};

// A parsed .cml: the linked list of groups SpriteRenderer.load builds.
struct SpriteSet {
    std::vector<SpriteGroup> groups;

    // First group with this id (ids are compared as signed bytes, exactly as
    // the engine's `(byte)groupId` cast does), or nullptr.
    SpriteGroup* Find(int groupId);
    const SpriteGroup* Find(int groupId) const;

    int Width(int groupId) const;
    int Height(int groupId) const;
    // SpriteRenderer.advanceFrame / setFrame / resetFrame. The first two
    // return true when the group is missing or has hit its last frame.
    bool AdvanceFrame(int groupId);
    bool SetFrame(int groupId, int index);
    void ResetFrame(int groupId);
};

// Parses a .cml resource. `images` is used for the PNG sizes of static
// images (SpriteRenderer.load reads them off the decoded image). Throws
// std::runtime_error on malformed data or a missing PNG.
SpriteSet ParseCml(const std::vector<uint8_t>& data, ImageCache& images);

}  // namespace oblivion
