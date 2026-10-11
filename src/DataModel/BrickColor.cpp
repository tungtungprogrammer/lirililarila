#include "BrickColor.h"
#include <cstring>

const std::vector<BrickColorInfo>& brickColors() {
    static const std::vector<BrickColorInfo> table = {
        {1,    "White",                 242, 243, 243},
        {208,  "Light stone grey",      229, 228, 223},
        {194,  "Medium stone grey",     163, 162, 165},
        {199,  "Dark stone grey",        99,  95,  98},
        {26,   "Black",                  27,  42,  53},
        {21,   "Bright red",            196,  40,  28},
        {106,  "Bright orange",         218, 133,  65},
        {24,   "Bright yellow",         245, 205,  48},
        {226,  "Cool yellow",           253, 234, 141},
        {119,  "Br. yellowish green",   164, 189,  71},
        {37,   "Bright green",           75, 151,  75},
        {28,   "Dark green",             40, 127,  71},
        {45,   "Light blue",            180, 210, 228},
        {23,   "Bright blue",            13, 105, 172},
        {104,  "Bright violet",         107,  50, 124},
        {221,  "Bright reddish violet", 205,  98, 152},
        {18,   "Nougat",                204, 142, 105},
        {5,    "Brick yellow",          215, 197, 154},
        {217,  "Brown",                 124,  92,  70},
        {38,   "Dark orange",           160,  95,  53},
    };
    return table;
}

const BrickColorInfo& brickColorById(int id) {
    for (const BrickColorInfo& c : brickColors()) { if (c.id == id) return c; }
    for (const BrickColorInfo& c : brickColors()) { if (c.id == kBrickMediumStoneGrey) return c; }
    return brickColors().front();
}

int brickColorIdByName(const char* name, int fallbackId) {
    for (const BrickColorInfo& c : brickColors()) { if (std::strcmp(c.name, name) == 0) return c.id; }
    return fallbackId;
}
