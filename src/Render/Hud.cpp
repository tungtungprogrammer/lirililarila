#include "Hud.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hud {

namespace {

// 5x7 bitmap font, one byte per row (bit 4 = leftmost pixel).
struct Glyph { char c; unsigned char rows[7]; };
const Glyph kFont[] = {
    {' ', {0, 0, 0, 0, 0, 0, 0}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}}, {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}}, {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}}, {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}}, {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}}, {'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}}, {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}}, {'N', {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}}, {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}}, {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}}, {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}}, {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}}, {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}}, {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}}, {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}}, {'3', {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}}, {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}}, {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}}, {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {'.', {0, 0, 0, 0, 0, 0x0C, 0x0C}}, {':', {0, 0x0C, 0x0C, 0, 0x0C, 0x0C, 0}},
    {'-', {0, 0, 0, 0x1F, 0, 0, 0}}, {'/', {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10}},
    {'!', {0x04, 0x04, 0x04, 0x04, 0x04, 0, 0x04}}, {',', {0, 0, 0, 0, 0x0C, 0x04, 0x08}},
    {'+', {0, 0x04, 0x04, 0x1F, 0x04, 0x04, 0}},
};

const Glyph* findGlyph(char c) {
    if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
    for (const Glyph& g : kFont) if (g.c == c) return &g;
    return &kFont[0];
}

const float kAdvance = 6.0f;      // glyph cells (5 wide + 1 gap)

}  // namespace

static float ndcX(const HudState& s, float px) { return px / float(s.width) * 2.0f - 1.0f; }
static float ndcY(const HudState& s, float py) { return 1.0f - py / float(s.height) * 2.0f; }

void addRect(MeshData& m, const HudState& s, float x0, float y0, float x1, float y1, const Vec3& color, int tenths) {
    const Vec3 aux(0, 0, 2.0f * float(tenths));
    const Vec3 n(0, 0, 1);
    const int a = m.addVertex(Vec3(ndcX(s, x0), ndcY(s, y0), 0), n, color, aux);
    const int b = m.addVertex(Vec3(ndcX(s, x1), ndcY(s, y0), 0), n, color, aux);
    const int c = m.addVertex(Vec3(ndcX(s, x1), ndcY(s, y1), 0), n, color, aux);
    const int d = m.addVertex(Vec3(ndcX(s, x0), ndcY(s, y1), 0), n, color, aux);
    m.addQuad(a, b, c, d, n);
}

float textWidth(const char* text, float px) { return float(std::strlen(text)) * kAdvance * px - px; }

void addText(MeshData& m, const HudState& s, const char* text, float x, float y, float px, const Vec3& color, int tenths) {
    for (const char* p = text; *p; ++p) {
        const Glyph* g = findGlyph(*p);
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (g->rows[row] & (0x10 >> col))
                    addRect(m, s, x + col * px, y + row * px, x + (col + 1) * px, y + (row + 1) * px, color, tenths);
        x += kAdvance * px;
    }
}

static void shadowedText(MeshData& m, const HudState& s, const char* t, float x, float y, float px, const Vec3& c, int tenths = 0) {
    addText(m, s, t, x + px * 0.8f, y + px * 0.8f, px, Vec3(0, 0, 0), std::min(10, tenths + 3));
    addText(m, s, t, x, y, px, c, tenths);
}

void build(const HudState& s, MeshData& out) {
    out.clear();
    if (!s.solo) return;
    const float u = std::max(1.0f, float(s.height) / 420.0f);          // UI scale
    const float W = float(s.width), H = float(s.height);

    // ---- health bar (top right, like the original) ----
    {
        const float pw = 190 * u, ph = 44 * u, x0 = W - pw - 10 * u, y0 = 10 * u;
        addRect(out, s, x0 - u, y0 - u, x0 + pw + u, y0 + ph + u, Vec3(0.05f, 0.05f, 0.05f), 2);       // border
        addRect(out, s, x0, y0, x0 + pw, y0 + ph, Vec3(0.12f, 0.12f, 0.14f), 3);                         // panel
        shadowedText(out, s, "HEALTH", x0 + 8 * u, y0 + 6 * u, 1.6f * u, Vec3(1, 1, 1));
        const float frac = s.maxHealth > 0 ? std::max(0.0f, std::min(1.0f, s.health / s.maxHealth)) : 0.0f;
        char num[16];
        std::snprintf(num, sizeof(num), "%d", int(s.health + 0.999f));
        const float nw = textWidth(num, 1.6f * u);
        shadowedText(out, s, num, x0 + pw - 8 * u - nw, y0 + 6 * u, 1.6f * u, Vec3(1, 1, 1));

        const float bx0 = x0 + 8 * u, bx1 = x0 + pw - 8 * u, by0 = y0 + 24 * u, by1 = y0 + 36 * u;
        addRect(out, s, bx0 - u, by0 - u, bx1 + u, by1 + u, Vec3(0, 0, 0));
        addRect(out, s, bx0, by0, bx1, by1, Vec3(0.25f, 0.05f, 0.05f));                                 // empty part
        const Vec3 fill = frac > 0.5f ? Vec3(0.20f, 0.78f, 0.20f) : (frac > 0.25f ? Vec3(0.95f, 0.75f, 0.12f) : Vec3(0.88f, 0.15f, 0.12f));
        if (frac > 0) {
            addRect(out, s, bx0, by0, bx0 + (bx1 - bx0) * frac, by1, fill);
            addRect(out, s, bx0, by0, bx0 + (bx1 - bx0) * frac, by0 + (by1 - by0) * 0.4f, Vec3(std::min(1.0f, fill.x + 0.25f), std::min(1.0f, fill.y + 0.25f), std::min(1.0f, fill.z + 0.25f)), 5);   // highlight
        }
    }

    // ---- controls hint (bottom left, fades away) ----
    if (!s.dead && s.soloTime < 10.0f) {
        const int fade = s.soloTime < 7.0f ? 0 : std::min(10, int((s.soloTime - 7.0f) / 3.0f * 10.0f));
        const char* lines[] = {"WASD / ARROWS: MOVE", "SPACE: JUMP", "RIGHT MOUSE: TURN CAMERA", "WHEEL: ZOOM", "SHIFT+F5: STOP"};
        const float px = 1.5f * u, lineH = 11 * px;
        float y = H - 10 * u - lineH * 5;
        for (const char* l : lines) { shadowedText(out, s, l, 12 * u, y, px, Vec3(1, 1, 1), fade); y += lineH; }
    }

    // ---- death screen ----
    if (s.dead) {
        const int tenths = 10 - std::min(5, int(s.deadTime * 6.0f));              // fades to 50% black
        addRect(out, s, 0, 0, W, H, Vec3(0, 0, 0), tenths);
        const float px = 5 * u;
        const char* big = "YOU DIED";
        shadowedText(out, s, big, (W - textWidth(big, px)) * 0.5f, H * 0.38f, px, Vec3(0.95f, 0.25f, 0.2f));
        char sub[48];
        std::snprintf(sub, sizeof(sub), "RESPAWNING IN %d", std::max(1, int(std::ceil(2.0f - s.deadTime))));
        const float px2 = 2.2f * u;
        shadowedText(out, s, sub, (W - textWidth(sub, px2)) * 0.5f, H * 0.38f + 11 * px, px2, Vec3(1, 1, 1));
    }

    // ---- paused ----
    if (s.paused) {
        const float px = 4 * u;
        const char* t = "PAUSED";
        shadowedText(out, s, t, (W - textWidth(t, px)) * 0.5f, H * 0.12f, px, Vec3(1, 1, 1));
    }
}

}  // namespace hud
