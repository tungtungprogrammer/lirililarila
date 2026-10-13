#pragma once
#include <vector>
#include "Part.h"
#include "Physics.h"
#include "VecMath.h"

// The classic 2007 player: yellow head with a smiley, blue torso, yellow arms, green legs.
// Sizes are the original ones: torso 2x2x1, arms and legs 1x2x1, head ~1 stud tall. Total height 5 studs.
// Kinematic controller (walk, jump, slide along walls) with the original walk / jump / fall poses.
class Character {
public:
    struct FaceBox { Vec3 center; Mat3 rot; Vec3 half; };

    Character();

    void spawn(const Vec3& feet, float yawRadians);
    // moveDir: world-space, horizontal, length 0..1. world: every part of the place. phys may be null.
    void step(float dt, const Vec3& moveDir, bool jump, const std::vector<Part*>& world, Physics* phys);

    // --- state ---
    Vec3  feet;                 // bottom centre
    float yaw = 0;              // facing; forward = (-sin yaw, 0, -cos yaw)
    Vec3  velocity;
    bool  onGround = false;
    Vec3  headPosition() const { return feet + Vec3(0, 4.5f, 0); }
    Vec3  focusPosition() const { return feet + Vec3(0, 3.5f, 0); }

    // --- for drawing ---
    const std::vector<Part*>& parts() { return m_parts; }          // head, torso, left arm, right arm, left leg, right leg
    const std::vector<FaceBox>& face() const { return m_face; }
    void updatePose();                                               // recompute part transforms from the animation state

    // animation state (exposed for tests)
    float walkPhase = 0;
    float armSwingL = 0, armSwingR = 0, legSwingL = 0, legSwingR = 0;   // radians about the shoulder / hip

    static constexpr float kSpeed = 16.0f;       // studs / second
    static constexpr float kJumpSpeed = 55.0f;   // about 7.2 studs of jump height at gravity 196.2
    static constexpr float kRadius = 0.85f;
    static constexpr float kHeight = 5.0f;

private:
    void resolve(const std::vector<Part*>& world, Physics* phys, const Vec3& horizontalVelocity);
    void animate(float dt, const Vec3& horizontalVelocity);

    Part  m_head, m_torso, m_armL, m_armR, m_legL, m_legR;
    std::vector<Part*> m_parts;
    std::vector<FaceBox> m_face;
    float m_gravity = 196.2f;
};
