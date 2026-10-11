#pragma once
// Small vector helpers on top of the data-model math types (no G3D, no Qt).
#include <algorithm>
#include <cmath>
#include "DataModel/Math.h"

namespace geom {

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
inline Vec3 normalize(const Vec3& v) {
    const float l = v.length();
    return l > 1e-8f ? v * (1.0f / l) : Vec3(0, 1, 0);
}
inline Vec3 column(const Mat3& r, int c) { return Vec3(r.m[0][c], r.m[1][c], r.m[2][c]); }
inline Vec3 mulTransposed(const Mat3& r, const Vec3& v) {   // R^T * v
    return Vec3(r.m[0][0] * v.x + r.m[1][0] * v.y + r.m[2][0] * v.z,
                r.m[0][1] * v.x + r.m[1][1] * v.y + r.m[2][1] * v.z,
                r.m[0][2] * v.x + r.m[1][2] * v.y + r.m[2][2] * v.z);
}
inline float comp(const Vec3& v, int i) { return i == 0 ? v.x : (i == 1 ? v.y : v.z); }
inline void setComp(Vec3& v, int i, float f) { (i == 0 ? v.x : (i == 1 ? v.y : v.z)) = f; }

struct Ray {
    Vec3 origin;
    Vec3 dir;   // unit length
};

// Distance along the ray to an oriented box, or a negative number on a miss.
inline float intersectBox(const Ray& ray, const Vec3& center, const Mat3& rot, const Vec3& half) {
    const Vec3 o = mulTransposed(rot, ray.origin - center);
    const Vec3 d = mulTransposed(rot, ray.dir);
    float t0 = 0.0f, t1 = 1e30f;
    for (int i = 0; i < 3; ++i) {
        const float oi = comp(o, i), di = comp(d, i), h = comp(half, i);
        if (std::fabs(di) < 1e-9f) {
            if (oi < -h || oi > h) return -1.0f;
        } else {
            float a = (-h - oi) / di, b = (h - oi) / di;
            if (a > b) std::swap(a, b);
            t0 = std::max(t0, a);
            t1 = std::min(t1, b);
            if (t0 > t1) return -1.0f;
        }
    }
    return t0;
}

inline float intersectSphere(const Ray& ray, const Vec3& c, float r) {
    const Vec3 oc = ray.origin - c;
    const float b = oc.dot(ray.dir);
    const float disc = b * b - (oc.dot(oc) - r * r);
    if (disc < 0) return -1.0f;
    const float t = -b - std::sqrt(disc);
    return t >= 0 ? t : (-b + std::sqrt(disc) >= 0 ? 0.0f : -1.0f);
}

// Parameter t along the line P0 + axis*t that is closest to the ray. Returns false for (near) parallel lines.
inline bool closestOnAxis(const Vec3& p0, const Vec3& axis, const Ray& ray, float& t) {
    const Vec3 w0 = p0 - ray.origin;
    const float b = axis.dot(ray.dir);
    const float denom = 1.0f - b * b;
    if (denom < 1e-4f) return false;
    const float dd = axis.dot(w0), e = ray.dir.dot(w0);
    t = (b * e - dd) / denom;
    return true;
}

inline float snapTo(float v, float step) { return step > 0 ? std::floor(v / step + 0.5f) * step : v; }

}  // namespace geom
