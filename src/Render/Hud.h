#pragma once
#include "MeshBuilder.h"

// The on-screen display for Play Solo, built as a flat mesh in normalised device coordinates (no G3D, no Qt).
// Colours are plain RGB; translucency uses the same aux.z "tenths" encoding as the 3D scene (see MeshBuilder).
struct HudState {
    int   width = 800, height = 600;     // viewport size in pixels
    bool  solo = false;                  // Play Solo is active
    bool  paused = false;
    float health = 100, maxHealth = 100;
    bool  dead = false;
    float deadTime = 0;                  // seconds since dying
    float soloTime = 0;                  // seconds since Play Solo started (controls hint fades out)
    const char* playerName = "Player";  // shown on the leaderboard
    int   points = 0;                    // leaderboard columns
    int   knockouts = 0;
    int   wipeouts = 0;                  // deaths
};

namespace hud {

// Fills `out` (cleared first). The mesh is drawn with an identity view-projection and no depth test.
void build(const HudState& s, MeshData& out);

// Text helpers, exposed for tests. Pixel coordinates have their origin at the top-left.
void addRect(MeshData& m, const HudState& s, float x0, float y0, float x1, float y1, const Vec3& color, int transparencyTenths = 0);
float textWidth(const char* text, float pixelSize);
void addText(MeshData& m, const HudState& s, const char* text, float x, float y, float pixelSize, const Vec3& color, int transparencyTenths = 0);

}  // namespace hud
