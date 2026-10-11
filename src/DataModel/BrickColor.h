#pragma once
#include <vector>

// The classic BrickColor palette (id, name, sRGB).
struct BrickColorInfo {
    int id;
    const char* name;
    unsigned char r, g, b;
};

const std::vector<BrickColorInfo>& brickColors();
const BrickColorInfo& brickColorById(int id);        // falls back to Medium stone grey
int brickColorIdByName(const char* name, int fallbackId);

enum { kBrickMediumStoneGrey = 194, kBrickBrightGreen = 37, kBrickBrightRed = 21,
       kBrickBrightBlue = 23, kBrickBrightYellow = 24 };
