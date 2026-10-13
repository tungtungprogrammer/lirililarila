#include "Character.h"
#include <algorithm>
#include <cmath>

namespace {

const float kPi = 3.14159265f;
const float kDeg = 57.29577951f;

Vec3 col(const Mat3& r, int c) { return Vec3(r.m[0][c], r.m[1][c], r.m[2][c]); }

float approach(float cur, float target, float maxDelta) {
    if (cur < target) return std::min(target, cur + maxDelta);
    return std::max(target, cur - maxDelta);
}

float wrapAngle(float a) {
    while (a > kPi) a -= 2 * kPi;
    while (a < -kPi) a += 2 * kPi;
    return a;
}

}  // namespace

Character::Character() {
    auto setup = [](Part& p, const char* name, const Vec3& size, int color, SurfaceType top) {
        p.name = name;
        p.size = size;
        p.brickColor = color;
        p.topSurface = top;
        p.anchored = false;
    };
    setup(m_head, "Head", Vec3(1.0f, 1.25f, 1.25f), kBrickBrightYellow, SurfaceType::Smooth);   // cylinder, axis becomes vertical
    m_head.shape = PartShape::Cylinder;
    setup(m_torso, "Torso", Vec3(2, 2, 1), kBrickBrightBlue, SurfaceType::Studs);
    setup(m_armL, "Left Arm", Vec3(1, 2, 1), kBrickBrightYellow, SurfaceType::Studs);
    setup(m_armR, "Right Arm", Vec3(1, 2, 1), kBrickBrightYellow, SurfaceType::Studs);
    setup(m_legL, "Left Leg", Vec3(1, 2, 1), kBrickBrightGreen, SurfaceType::Smooth);
    setup(m_legR, "Right Leg", Vec3(1, 2, 1), kBrickBrightGreen, SurfaceType::Smooth);
    m_parts = {&m_head, &m_torso, &m_armL, &m_armR, &m_legL, &m_legR};
    spawn(Vec3(0, 3, 0), 0);
}

void Character::spawn(const Vec3& f, float y) {
    feet = f;
    yaw = y;
    velocity = Vec3();
    onGround = false;
    walkPhase = 0;
    armSwingL = armSwingR = legSwingL = legSwingR = 0;
    updatePose();
}

void Character::updatePose() {
    const Mat3 root = Mat3::rotationY(yaw * kDeg);
    auto limb = [&](Part& p, const Vec3& pivotLocal, float swing, float halfLen) {
        const Mat3 r = root * Mat3::rotationX(swing * kDeg);
        p.orientation = r;
        p.position = feet + root * pivotLocal + r * Vec3(0, -halfLen, 0);
    };
    limb(m_legL, Vec3(-0.5f, 2.0f, 0), legSwingL, 1.0f);
    limb(m_legR, Vec3(0.5f, 2.0f, 0), legSwingR, 1.0f);
    limb(m_armL, Vec3(-1.5f, 4.0f, 0), armSwingL, 1.0f);
    limb(m_armR, Vec3(1.5f, 4.0f, 0), armSwingR, 1.0f);

    m_torso.orientation = root;
    m_torso.position = feet + root * Vec3(0, 3.0f, 0);

    // The cylinder's axis is its local X; turn it upright.
    m_head.orientation = root * Mat3::rotationZ(90.0f);
    m_head.position = feet + root * Vec3(0, 4.5f, 0);

    // Face: two eyes and a smile, small dark boxes on the front (-Z) of the head.
    m_face.clear();
    const float R = 0.625f;                                   // head radius
    auto addFace = [&](float x, float y2, const Vec3& half) {
        const float z = -std::sqrt(std::max(0.0f, R * R - x * x)) - half.z * 0.5f;
        FaceBox f;
        f.center = feet + root * Vec3(x, 4.5f + y2, z);
        f.rot = root * Mat3::rotationY(-std::asin(std::max(-1.0f, std::min(1.0f, x / R))) * kDeg);
        f.half = half;
        m_face.push_back(f);
    };
    addFace(-0.2f, 0.12f, Vec3(0.075f, 0.18f, 0.02f));
    addFace(0.2f, 0.12f, Vec3(0.075f, 0.18f, 0.02f));
    for (int i = 0; i < 7; ++i) {                              // smile: a shallow arc
        const float t = (i - 3) / 3.0f;                        // -1..1
        addFace(t * 0.36f, -0.17f + 0.15f * t * t, Vec3(0.07f, 0.042f, 0.02f));
    }
}

void Character::animate(float dt, const Vec3& hv) {
    const float speed = std::sqrt(hv.x * hv.x + hv.z * hv.z);
    float tL, tR, lL, lR;
    float rate = 12.0f;                                        // how fast poses blend (radians / second * ...)
    if (!onGround) {
        // jump / fall pose: arms thrown up, legs together
        tL = tR = 2.9f;
        lL = 0.12f; lR = -0.12f;
        rate = 16.0f;
    } else if (speed > 0.5f) {
        walkPhase += speed * dt * 0.75f;
        const float amp = 0.95f * std::min(1.0f, speed / kSpeed);
        const float s = std::sin(walkPhase);
        lL = s * amp;  lR = -s * amp;                          // legs opposite
        tL = -s * amp * 0.9f; tR = s * amp * 0.9f;             // each arm swings against its leg
        rate = 30.0f;
    } else {
        tL = tR = lL = lR = 0;                                 // idle: standing straight
        rate = 10.0f;
        walkPhase = 0;
    }
    const float d = rate * dt;
    armSwingL = approach(armSwingL, tL, d);
    armSwingR = approach(armSwingR, tR, d);
    legSwingL = approach(legSwingL, lL, d);
    legSwingR = approach(legSwingR, lR, d);
}

void Character::resolve(const std::vector<Part*>& world, Physics* phys, const Vec3& hv) {
    // three spheres stacked from the feet to the top of the head
    static const float kOffsets[3] = {kRadius, kHeight * 0.5f, kHeight - kRadius};
    bool ground = false;
    for (int pass = 0; pass < 3; ++pass) {
        for (int s = 0; s < 3; ++s) {
            const Vec3 c = feet + Vec3(0, kOffsets[s], 0);
            for (Part* p : world) {
                if (p->transparency >= 0.95f) continue;
                const Vec3 half = p->size * 0.5f;
                if ((p->position - c).length() > half.length() + kRadius + 0.2f) continue;

                Vec3 closest, n;
                float dist;
                if (p->shape == PartShape::Ball) {
                    const float rr = std::min(p->size.x, std::min(p->size.y, p->size.z)) * 0.5f;
                    const Vec3 d = c - p->position;
                    const float l = d.length();
                    dist = l - rr;
                    n = l > 1e-5f ? d * (1.0f / l) : Vec3(0, 1, 0);
                    closest = p->position + n * rr;
                } else {
                    const Vec3 rel = c - p->position;
                    const Vec3 local(rel.dot(col(p->orientation, 0)), rel.dot(col(p->orientation, 1)), rel.dot(col(p->orientation, 2)));
                    const Vec3 clamped(std::max(-half.x, std::min(half.x, local.x)), std::max(-half.y, std::min(half.y, local.y)),
                                       std::max(-half.z, std::min(half.z, local.z)));
                    closest = p->position + col(p->orientation, 0) * clamped.x + col(p->orientation, 1) * clamped.y + col(p->orientation, 2) * clamped.z;
                    const Vec3 d = c - closest;
                    const float l = d.length();
                    if (l > 1e-5f) {
                        n = d * (1.0f / l);
                        dist = l;
                    } else {                                                   // centre inside: leave through the nearest face
                        float best = 1e30f; int bi = 0; float bs = 1;
                        const float lv[3] = {local.x, local.y, local.z}, hv3[3] = {half.x, half.y, half.z};
                        for (int i = 0; i < 3; ++i) {
                            const float gap = hv3[i] - std::fabs(lv[i]);
                            if (gap < best) { best = gap; bi = i; bs = lv[i] >= 0 ? 1.0f : -1.0f; }
                        }
                        n = col(p->orientation, bi) * bs;
                        dist = -best;
                    }
                }

                // standing on a walkable surface (within a hair, not only when penetrating)
                if (n.y > 0.55f && dist < kRadius + 0.05f && s == 0) ground = true;
                if (dist >= kRadius) continue;

                const float push = kRadius - dist;
                Vec3 up = n * push;
                // leave walls horizontally so the character slides instead of climbing them
                feet = feet + up;
                if (n.y > 0.55f && velocity.y < 0) velocity.y = 0;
                if (n.y < -0.5f && velocity.y > 0) velocity.y = 0;

                if (phys && !p->anchored) {
                    const Vec3 towards = n * -1.0f;
                    const float into = hv.x * towards.x + hv.z * towards.z;
                    if (into > 0.5f) {
                        const float mass = std::max(0.5f, p->size.x * p->size.y * p->size.z);
                        const float k = std::min(1.0f, 6.0f / mass);
                        phys->push(p, Vec3(towards.x, 0, towards.z), into * k);
                    }
                }
            }
        }
    }
    onGround = ground;
}

void Character::step(float dt, const Vec3& moveDir, bool jump, const std::vector<Part*>& world, Physics* phys) {
    Vec3 dir(moveDir.x, 0, moveDir.z);
    const float len = std::sqrt(dir.x * dir.x + dir.z * dir.z);
    if (len > 1.0f) dir = dir * (1.0f / len);
    const Vec3 hv = dir * kSpeed;

    // turn towards the walking direction
    if (len > 0.05f) {
        const float target = std::atan2(-dir.x, -dir.z);
        const float diff = wrapAngle(target - yaw);
        const float maxTurn = 14.0f * dt;
        yaw += std::max(-maxTurn, std::min(maxTurn, diff));
        yaw = wrapAngle(yaw);
    }

    if (onGround && jump) { velocity.y = kJumpSpeed; onGround = false; }
    velocity.y = std::max(-250.0f, velocity.y - m_gravity * dt);

    const Vec3 disp(hv.x * dt, velocity.y * dt, hv.z * dt);
    const float dl = std::sqrt(disp.x * disp.x + disp.y * disp.y + disp.z * disp.z);
    const int subs = std::max(1, std::min(16, int(std::ceil(dl / 0.35f))));
    for (int i = 0; i < subs; ++i) {
        feet = feet + disp * (1.0f / subs);
        resolve(world, phys, hv);
    }

    animate(dt, hv);
    updatePose();
}
