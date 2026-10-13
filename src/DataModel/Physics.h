#pragma once
#include <memory>
#include <vector>
#include "Part.h"
#include "VecMath.h"

// A small rigid-body engine for "Run": gravity, box / sphere collisions, friction, sleeping.
// Anchored parts are immovable; everything else is simulated. Free of Qt and G3D.
class Physics {
public:
    struct Body {
        Part*  part = nullptr;
        bool   isStatic = false;
        bool   isSphere = false;
        float  radius = 0.5f;
        Vec3   half;
        float  invMass = 0;
        Vec3   invInertia;             // diagonal, body space
        Vec3   v, w;
        float  sleepTimer = 0;
        bool   sleeping = false;
        bool   dead = false;           // fell out of the world
        bool   immovable() const { return isStatic || sleeping; }
    };

    // Collects every part of the place.
    void start(const std::vector<Part*>& parts);
    void clear() { m_bodies.clear(); }
    // Advances by dt seconds (callers use a fixed step); writes the results back into the parts.
    void step(float dt);

    // Makes sure a part moves at least `speed` along `dir` (used when the player pushes bricks); wakes it if asleep.
    void push(const Part* part, const Vec3& dir, float speed);

    float gravity = 196.2f;            // studs / s^2, the classic value
    const std::vector<Body>& bodies() const { return m_bodies; }

    struct Contact;   // implementation detail, public only so the helpers in Physics.cpp can name it

private:
    void substep(float dt);
    static void orthonormalize(Mat3& r);

    std::vector<Body> m_bodies;
};
