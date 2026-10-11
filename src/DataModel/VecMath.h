#pragma once
#include <algorithm>
#include <cmath>

// Minimal math types for the data model (the renderer converts these to G3D types).

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    float length() const { return std::sqrt(dot(*this)); }
    bool operator==(const Vec3& o) const {
        return std::fabs(x - o.x) < 1e-5f && std::fabs(y - o.y) < 1e-5f && std::fabs(z - o.z) < 1e-5f;
    }
    bool operator!=(const Vec3& o) const { return !(*this == o); }
};

// Row-major 3x3 rotation matrix. Euler convention: R = Ry(yaw) * Rx(pitch) * Rz(roll).
struct Mat3 {
    float m[3][3];

    Mat3() { for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) m[r][c] = (r == c) ? 1.0f : 0.0f; }

    static Mat3 rotationX(float deg) {
        Mat3 a; const float r = deg * 0.017453292519943295f, c = std::cos(r), s = std::sin(r);
        a.m[1][1] = c; a.m[1][2] = -s; a.m[2][1] = s; a.m[2][2] = c; return a;
    }
    static Mat3 rotationY(float deg) {
        Mat3 a; const float r = deg * 0.017453292519943295f, c = std::cos(r), s = std::sin(r);
        a.m[0][0] = c; a.m[0][2] = s; a.m[2][0] = -s; a.m[2][2] = c; return a;
    }
    static Mat3 rotationZ(float deg) {
        Mat3 a; const float r = deg * 0.017453292519943295f, c = std::cos(r), s = std::sin(r);
        a.m[0][0] = c; a.m[0][1] = -s; a.m[1][0] = s; a.m[1][1] = c; return a;
    }
    static Mat3 fromEulerYXZ(float yawDeg, float pitchDeg, float rollDeg) {
        return rotationY(yawDeg) * rotationX(pitchDeg) * rotationZ(rollDeg);
    }

    Mat3 operator*(const Mat3& o) const {
        Mat3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                r.m[i][j] = m[i][0] * o.m[0][j] + m[i][1] * o.m[1][j] + m[i][2] * o.m[2][j];
        return r;
    }

    Vec3 operator*(const Vec3& v) const {
        return Vec3(m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                    m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                    m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z);
    }

    bool operator==(const Mat3& o) const {
        for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) if (std::fabs(m[r][c] - o.m[r][c]) > 1e-5f) return false;
        return true;
    }
    bool operator!=(const Mat3& o) const { return !(*this == o); }

    void toEulerYXZ(float& yawDeg, float& pitchDeg, float& rollDeg) const {
        const float k = 57.29577951308232f;
        const float sx = std::max(-1.0f, std::min(1.0f, -m[1][2]));
        const float pitch = std::asin(sx);
        float yaw, roll;
        if (std::fabs(std::cos(pitch)) > 1e-4f) {
            yaw  = std::atan2(m[0][2], m[2][2]);
            roll = std::atan2(m[1][0], m[1][1]);
        } else {
            yaw  = std::atan2(-m[2][0], m[0][0]);
            roll = 0.0f;
        }
        yawDeg = yaw * k; pitchDeg = pitch * k; rollDeg = roll * k;
    }
};
