#pragma once
#include "Geom.h"

// The Studio camera: free-fly, right-drag to look, WASD/QE to move, wheel to zoom.
struct FlyCamera {
    Vec3  position = Vec3(14, 9, 16);
    float yaw = 0.0f, pitch = 0.0f;       // radians; yaw 0 looks down -Z
    float fovY = 70.0f * 3.14159265f / 180.0f;
    float nearZ = 0.3f, farZ = 3000.0f;

    FlyCamera() { lookAt(Vec3(0, 1, 0)); }

    Vec3 forward() const { return Vec3(-std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)); }
    Vec3 right() const { return Vec3(std::cos(yaw), 0, -std::sin(yaw)); }
    Vec3 up() const { return geom::cross(right(), forward()); }
    float tanHalfFov() const { return std::tan(fovY * 0.5f); }

    void lookAt(const Vec3& target) {
        const Vec3 d = geom::normalize(target - position);
        pitch = std::asin(std::max(-1.0f, std::min(1.0f, d.y)));
        yaw = std::atan2(-d.x, -d.z);
    }
    void look(float dxPixels, float dyPixels) {
        yaw -= dxPixels * 0.005f;
        pitch = std::max(-1.5f, std::min(1.5f, pitch - dyPixels * 0.005f));
    }

    // ndc in [-1,1], +y up.
    geom::Ray ray(float ndcX, float ndcY, float aspect) const {
        const float t = tanHalfFov();
        geom::Ray r;
        r.origin = position;
        r.dir = geom::normalize(forward() + right() * (ndcX * t * aspect) + up() * (ndcY * t));
        return r;
    }

    // Column-major view-projection (OpenGL clip space), written to out[16].
    void viewProjection(float aspect, float out[16]) const {
        const Vec3 f = forward(), r = right(), u = up();
        // view matrix rows: r, u, -f
        const float v[4][4] = {
            { r.x,  r.y,  r.z, -r.dot(position)},
            { u.x,  u.y,  u.z, -u.dot(position)},
            {-f.x, -f.y, -f.z,  f.dot(position)},
            {0, 0, 0, 1}};
        const float t = 1.0f / tanHalfFov();
        const float n = nearZ, fa = farZ;
        const float p[4][4] = {
            {t / aspect, 0, 0, 0},
            {0, t, 0, 0},
            {0, 0, -(fa + n) / (fa - n), -2 * fa * n / (fa - n)},
            {0, 0, -1, 0}};
        for (int c = 0; c < 4; ++c)
            for (int rr = 0; rr < 4; ++rr) {
                float s = 0;
                for (int k = 0; k < 4; ++k) s += p[rr][k] * v[k][c];
                out[c * 4 + rr] = s;
            }
    }
};
