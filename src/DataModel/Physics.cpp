#include "Physics.h"
#include <algorithm>
#include <cmath>

namespace {

const float kMargin = 0.02f;        // contacts are created slightly before touching
const float kSlop = 0.015f;
const float kFriction = 0.5f;
const float kRestitution = 0.15f;
const float kKillY = -500.0f;

Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
Vec3 normalized(const Vec3& v) {
    const float l = v.length();
    return l > 1e-8f ? v * (1.0f / l) : Vec3(0, 1, 0);
}
Vec3 col(const Mat3& r, int c) { return Vec3(r.m[0][c], r.m[1][c], r.m[2][c]); }
float comp(const Vec3& v, int i) { return i == 0 ? v.x : (i == 1 ? v.y : v.z); }
Vec3 mulT(const Mat3& r, const Vec3& v) {
    return Vec3(r.m[0][0] * v.x + r.m[1][0] * v.y + r.m[2][0] * v.z,
                r.m[0][1] * v.x + r.m[1][1] * v.y + r.m[2][1] * v.z,
                r.m[0][2] * v.x + r.m[1][2] * v.y + r.m[2][2] * v.z);
}

}  // namespace

struct Physics::Contact {
    Body* a = nullptr;
    Body* b = nullptr;
    Vec3  point, normal;          // normal points from a to b
    float depth = 0;
    // solver data
    Vec3  ra, rb, t1, t2;
    float kn = 0, kt1 = 0, kt2 = 0;
    float jn = 0, jt1 = 0, jt2 = 0;
    float bias = 0;
};

namespace {

// Inverse world-space inertia applied to a vector.
Vec3 applyInvInertia(const Physics::Body& b, const Vec3& v) {
    if (b.immovable()) return Vec3();
    const Mat3& R = b.part->orientation;
    const Vec3 l = mulT(R, v);
    return R * Vec3(l.x * b.invInertia.x, l.y * b.invInertia.y, l.z * b.invInertia.z);
}

Vec3 velocityAt(const Physics::Body& b, const Vec3& r) { return b.v + cross(b.w, r); }

// effective inverse mass along direction d for a contact arm r
float effMass(const Physics::Body& b, const Vec3& r, const Vec3& d) {
    if (b.immovable()) return 0.0f;
    const Vec3 rd = cross(r, d);
    return b.invMass + rd.dot(applyInvInertia(b, rd));
}

void applyImpulse(Physics::Body& b, const Vec3& r, const Vec3& impulse, float sign) {
    if (b.immovable()) return;
    b.v = b.v + impulse * (b.invMass * sign);
    b.w = b.w + applyInvInertia(b, cross(r, impulse)) * sign;
}

struct BoxRef { Vec3 c; Vec3 axis[3]; Vec3 half; };
BoxRef boxOf(const Physics::Body& b) {
    BoxRef r;
    r.c = b.part->position;
    for (int i = 0; i < 3; ++i) r.axis[i] = col(b.part->orientation, i);
    r.half = b.half;
    return r;
}

float supportRadius(const BoxRef& b, const Vec3& n) {
    return b.half.x * std::fabs(n.dot(b.axis[0])) + b.half.y * std::fabs(n.dot(b.axis[1])) + b.half.z * std::fabs(n.dot(b.axis[2]));
}

// closest points between two infinite lines p1 + t d1, p2 + s d2
void closestLines(const Vec3& p1, const Vec3& d1, const Vec3& p2, const Vec3& d2, Vec3& c1, Vec3& c2) {
    const Vec3 r = p1 - p2;
    const float a = d1.dot(d1), e = d2.dot(d2), f = d2.dot(r), c = d1.dot(r), b = d1.dot(d2);
    const float den = a * e - b * b;
    const float s = std::fabs(den) > 1e-8f ? (b * f - c * e) / den : 0.0f;
    const float t = (b * s + f) / e;
    c1 = p1 + d1 * s;
    c2 = p2 + d2 * t;
}

bool boxBox(Physics::Body& A, Physics::Body& B, std::vector<Physics::Contact>& out) {
    const BoxRef a = boxOf(A), b = boxOf(B);
    const Vec3 d = b.c - a.c;

    float faceOverlap = 1e30f, edgeOverlap = 1e30f;
    Vec3 faceN, edgeN;
    int edgeI = 0, edgeJ = 0;
    int faceOwner = 0, faceIdx = 0;      // which box (0 = a, 1 = b) and which axis gave the best face

    auto test = [&](const Vec3& L, bool isFace, int i, int j, int ownerBox = 0) -> bool {
        const float ra = supportRadius(a, L), rb = supportRadius(b, L);
        const float overlap = ra + rb - std::fabs(d.dot(L));
        if (overlap < -kMargin) return false;
        if (isFace) {
            if (overlap < faceOverlap) { faceOverlap = overlap; faceN = L; faceOwner = ownerBox; faceIdx = i; }
        } else if (overlap < edgeOverlap) { edgeOverlap = overlap; edgeN = L; edgeI = i; edgeJ = j; }
        return true;
    };

    for (int i = 0; i < 3; ++i) if (!test(a.axis[i], true, i, 0)) return false;
    for (int i = 0; i < 3; ++i) if (!test(b.axis[i], true, i, 0, 1)) return false;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            Vec3 L = cross(a.axis[i], b.axis[j]);
            if (L.length() < 1e-4f) continue;
            if (!test(normalized(L), false, i, j)) return false;
        }

    const bool useEdge = edgeOverlap < faceOverlap * 0.95f - 0.01f;
    Vec3 n = useEdge ? edgeN : faceN;
    const float overlap = useEdge ? edgeOverlap : faceOverlap;
    if (n.dot(d) < 0) n = n * -1.0f;                       // a -> b

    const size_t first = out.size();
    if (!useEdge) {
        // Reference face on one box, incident face on the other; clip the incident face to the reference sides.
        const BoxRef& ref = faceOwner == 0 ? a : b;
        const BoxRef& inc = faceOwner == 0 ? b : a;
        Vec3 refN = ref.axis[faceIdx];
        if (refN.dot(inc.c - ref.c) < 0) refN = refN * -1.0f;
        const Vec3 refCentre = ref.c + refN * comp(ref.half, faceIdx);

        int k = 0; float bestDot = -1;
        for (int q = 0; q < 3; ++q) {
            const float dq = std::fabs(inc.axis[q].dot(refN));
            if (dq > bestDot) { bestDot = dq; k = q; }
        }
        Vec3 incN = inc.axis[k];
        if (incN.dot(refN) > 0) incN = incN * -1.0f;                 // faces the reference
        const int ku = (k + 1) % 3, kv = (k + 2) % 3;
        const Vec3 ic = inc.c + incN * comp(inc.half, k);
        const Vec3 u = inc.axis[ku] * comp(inc.half, ku), v = inc.axis[kv] * comp(inc.half, kv);
        std::vector<Vec3> poly = {ic - u - v, ic + u - v, ic + u + v, ic - u + v};

        for (int side = 0; side < 3 && !poly.empty(); ++side) {
            if (side == faceIdx) continue;
            for (int sign = -1; sign <= 1; sign += 2) {
                const Vec3 pn = ref.axis[side] * float(sign);
                const float off = comp(ref.half, side);
                std::vector<Vec3> next;
                for (size_t q = 0; q < poly.size(); ++q) {
                    const Vec3 p0 = poly[q], p1 = poly[(q + 1) % poly.size()];
                    const float d0 = (p0 - ref.c).dot(pn) - off, d1 = (p1 - ref.c).dot(pn) - off;
                    if (d0 <= 0) next.push_back(p0);
                    if ((d0 < 0 && d1 > 0) || (d0 > 0 && d1 < 0)) next.push_back(p0 + (p1 - p0) * (d0 / (d0 - d1)));
                }
                poly.swap(next);
                if (poly.empty()) break;
            }
        }
        const Vec3 nAB = faceOwner == 0 ? refN : refN * -1.0f;
        for (const Vec3& p : poly) {
            const float depth = -(p - refCentre).dot(refN);
            if (depth < -kMargin) continue;
            Physics::Contact c; c.a = &A; c.b = &B; c.point = p; c.normal = nAB; c.depth = depth;
            out.push_back(c);
        }
        n = nAB;
    } else {
        // edge / edge: support edge of each box along the contact normal
        Vec3 pa = a.c, pb = b.c;
        for (int k = 0; k < 3; ++k) {
            if (k != edgeI) pa = pa + a.axis[k] * ((n.dot(a.axis[k]) > 0 ? 1.0f : -1.0f) * comp(a.half, k));
            if (k != edgeJ) pb = pb + b.axis[k] * ((n.dot(b.axis[k]) > 0 ? -1.0f : 1.0f) * comp(b.half, k));
        }
        Vec3 c1, c2;
        closestLines(pa, a.axis[edgeI], pb, b.axis[edgeJ], c1, c2);
        Physics::Contact c; c.a = &A; c.b = &B; c.point = (c1 + c2) * 0.5f; c.normal = n; c.depth = overlap;
        out.push_back(c);
    }
    if (out.size() == first) {
        Physics::Contact c; c.a = &A; c.b = &B; c.normal = n; c.depth = overlap;
        c.point = a.c + n * (supportRadius(a, n) - overlap * 0.5f);
        out.push_back(c);
    }
    // keep the deepest 8
    if (out.size() - first > 8) {
        std::sort(out.begin() + long(first), out.end(), [](const Physics::Contact& x, const Physics::Contact& y) { return x.depth > y.depth; });
        out.resize(first + 8);
    }
    return true;
}

// sphere S against box X; the contact normal points from the box to the sphere
bool sphereBox(const Physics::Body& S, const Physics::Body& X, Vec3& point, Vec3& nBoxToSphere, float& depth) {
    const BoxRef x = boxOf(X);
    const Vec3 c = S.part->position;
    const Vec3 rel = c - x.c;
    Vec3 local(rel.dot(x.axis[0]), rel.dot(x.axis[1]), rel.dot(x.axis[2]));
    Vec3 clamped(std::max(-x.half.x, std::min(x.half.x, local.x)), std::max(-x.half.y, std::min(x.half.y, local.y)),
                 std::max(-x.half.z, std::min(x.half.z, local.z)));
    const Vec3 closest = x.c + x.axis[0] * clamped.x + x.axis[1] * clamped.y + x.axis[2] * clamped.z;
    const Vec3 diff = c - closest;
    const float dist = diff.length();
    if (dist > S.radius + kMargin) return false;
    if (dist > 1e-6f) {
        nBoxToSphere = diff * (1.0f / dist);
        depth = S.radius - dist;
        point = closest;
        return true;
    }
    // centre inside the box: leave through the nearest face
    float best = 1e30f; int bi = 0; float bs = 1;
    for (int i = 0; i < 3; ++i) {
        const float l = comp(local, i), h = comp(x.half, i);
        const float gap = h - std::fabs(l);
        if (gap < best) { best = gap; bi = i; bs = l >= 0 ? 1.0f : -1.0f; }
    }
    nBoxToSphere = x.axis[bi] * bs;
    depth = S.radius + best;
    point = c - nBoxToSphere * S.radius;
    return true;
}

}  // namespace

void Physics::orthonormalize(Mat3& r) {
    Vec3 x = col(r, 0), y = col(r, 1);
    x = normalized(x);
    y = normalized(y - x * y.dot(x));
    const Vec3 z = cross(x, y);
    for (int i = 0; i < 3; ++i) {
        r.m[i][0] = comp(x, i); r.m[i][1] = comp(y, i); r.m[i][2] = comp(z, i);
    }
}

void Physics::start(const std::vector<Part*>& parts) {
    m_bodies.clear();
    m_bodies.reserve(parts.size());
    for (Part* p : parts) {
        Body b;
        b.part = p;
        b.half = p->size * 0.5f;
        b.isStatic = p->anchored;
        b.collides = p->canCollide;
        b.isSphere = p->shape == PartShape::Ball;
        b.radius = std::max(0.1f, std::min(p->size.x, std::min(p->size.y, p->size.z)) * 0.5f);
        const float vol = b.isSphere ? (4.0f / 3.0f) * 3.14159265f * b.radius * b.radius * b.radius : p->size.x * p->size.y * p->size.z;
        const float mass = std::max(0.05f, vol);
        if (b.isStatic) {
            b.invMass = 0;
        } else {
            b.invMass = 1.0f / mass;
            if (b.isSphere) {
                const float i = 0.4f * mass * b.radius * b.radius;
                b.invInertia = Vec3(1 / i, 1 / i, 1 / i);
            } else {
                const Vec3 s = p->size;
                b.invInertia = Vec3(12.0f / (mass * (s.y * s.y + s.z * s.z)), 12.0f / (mass * (s.x * s.x + s.z * s.z)),
                                    12.0f / (mass * (s.x * s.x + s.y * s.y)));
            }
        }
        m_bodies.push_back(b);
    }
}

void Physics::push(const Part* part, const Vec3& dir, float speed) {
    for (Body& b : m_bodies) {
        if (b.part != part) continue;
        if (b.isStatic || b.dead) return;
        b.sleeping = false;
        b.sleepTimer = 0;
        const float along = b.v.dot(dir);
        if (along < speed) b.v = b.v + dir * (speed - along);
        return;
    }
}

void Physics::step(float dt) {
    // two substeps per call keep stacks stable
    substep(dt * 0.5f);
    substep(dt * 0.5f);
    for (Body& b : m_bodies) {
        if (b.isStatic || b.dead) continue;
        if (b.part->position.y < kKillY) { b.dead = true; b.v = b.w = Vec3(); }
    }
}

void Physics::substep(float dt) {
    // ---- integrate forces ----
    for (Body& b : m_bodies) {
        if (b.immovable() || b.dead) continue;
        b.v.y -= gravity * dt;
        const float damp = std::exp(-0.15f * dt);
        b.w = b.w * damp;
    }

    // ---- collision detection ----
    std::vector<Contact> contacts;
    const size_t n = m_bodies.size();
    for (size_t i = 0; i < n; ++i) {
        Body& A = m_bodies[i];
        if (A.dead) continue;
        const float ra = A.isSphere ? A.radius : A.half.length();
        for (size_t j = i + 1; j < n; ++j) {
            Body& B = m_bodies[j];
            if (B.dead) continue;
            if (!A.collides || !B.collides) continue;
            if (A.immovable() && B.immovable()) continue;
            const float rb = B.isSphere ? B.radius : B.half.length();
            const Vec3 d = B.part->position - A.part->position;
            if (d.length() > ra + rb + 0.1f) continue;

            const size_t before = contacts.size();
            if (!A.isSphere && !B.isSphere) {
                boxBox(A, B, contacts);
            } else if (A.isSphere && B.isSphere) {
                const float dist = d.length();
                if (dist < A.radius + B.radius + kMargin) {
                    Contact c; c.a = &A; c.b = &B;
                    c.normal = dist > 1e-6f ? d * (1.0f / dist) : Vec3(0, 1, 0);
                    c.depth = A.radius + B.radius - dist;
                    c.point = A.part->position + c.normal * A.radius;
                    contacts.push_back(c);
                }
            } else {
                Body& S = A.isSphere ? A : B;
                Body& X = A.isSphere ? B : A;
                Vec3 p, nXS; float depth;
                if (sphereBox(S, X, p, nXS, depth)) {
                    Contact c; c.a = &A; c.b = &B; c.point = p; c.depth = depth;
                    c.normal = A.isSphere ? nXS * -1.0f : nXS;        // keep "a -> b"
                    contacts.push_back(c);
                }
            }
            // a fast body wakes whatever it hits
            if (contacts.size() > before) {
                auto speed = [](const Body& b) { return b.v.length() + b.w.length() * 0.5f; };
                if (A.sleeping && !B.immovable() && speed(B) > 2.0f) { A.sleeping = false; A.sleepTimer = 0; }
                if (B.sleeping && !A.immovable() && speed(A) > 2.0f) { B.sleeping = false; B.sleepTimer = 0; }
            }
        }
    }

    // ---- prepare ----
    for (Contact& c : contacts) {
        c.ra = c.point - c.a->part->position;
        c.rb = c.point - c.b->part->position;
        const Vec3 n = c.normal;
        const Vec3 helper = std::fabs(n.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        c.t1 = normalized(cross(n, helper));
        c.t2 = cross(n, c.t1);
        c.kn = effMass(*c.a, c.ra, n) + effMass(*c.b, c.rb, n);
        c.kt1 = effMass(*c.a, c.ra, c.t1) + effMass(*c.b, c.rb, c.t1);
        c.kt2 = effMass(*c.a, c.ra, c.t2) + effMass(*c.b, c.rb, c.t2);
        const float vn = (velocityAt(*c.b, c.rb) - velocityAt(*c.a, c.ra)).dot(n);
        float bias = std::min(4.0f, 0.2f / dt * std::max(0.0f, c.depth - kSlop));
        if (vn < -3.0f) bias = std::max(bias, -kRestitution * vn);
        c.bias = bias;
    }

    // ---- solve ----
    for (int iter = 0; iter < 12; ++iter) {
        for (Contact& c : contacts) {
            Body& A = *c.a; Body& B = *c.b;
            if (c.kn <= 1e-9f) continue;
            // friction first (bounded by the normal impulse from the previous iteration)
            const float maxF = kFriction * c.jn;
            Vec3 vr = velocityAt(B, c.rb) - velocityAt(A, c.ra);
            if (c.kt1 > 1e-9f) {
                float dj = -vr.dot(c.t1) / c.kt1;
                const float old = c.jt1;
                c.jt1 = std::max(-maxF, std::min(maxF, old + dj));
                dj = c.jt1 - old;
                const Vec3 imp = c.t1 * dj;
                applyImpulse(A, c.ra, imp, -1.0f); applyImpulse(B, c.rb, imp, 1.0f);
            }
            vr = velocityAt(B, c.rb) - velocityAt(A, c.ra);
            if (c.kt2 > 1e-9f) {
                float dj = -vr.dot(c.t2) / c.kt2;
                const float old = c.jt2;
                c.jt2 = std::max(-maxF, std::min(maxF, old + dj));
                dj = c.jt2 - old;
                const Vec3 imp = c.t2 * dj;
                applyImpulse(A, c.ra, imp, -1.0f); applyImpulse(B, c.rb, imp, 1.0f);
            }
            // normal
            vr = velocityAt(B, c.rb) - velocityAt(A, c.ra);
            const float vn = vr.dot(c.normal);
            float dj = (-vn + c.bias) / c.kn;
            const float old = c.jn;
            c.jn = std::max(0.0f, old + dj);
            dj = c.jn - old;
            const Vec3 imp = c.normal * dj;
            applyImpulse(A, c.ra, imp, -1.0f); applyImpulse(B, c.rb, imp, 1.0f);
        }
    }

    // ---- integrate positions ----
    for (Body& b : m_bodies) {
        if (b.immovable() || b.dead) continue;
        Part* p = b.part;
        p->position = p->position + b.v * dt;
        // R += [w]x R dt
        Mat3 R = p->orientation;
        Mat3 dR;
        for (int c = 0; c < 3; ++c) {
            const Vec3 rc = cross(b.w, col(R, c)) * dt;
            dR.m[0][c] = rc.x; dR.m[1][c] = rc.y; dR.m[2][c] = rc.z;
        }
        for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) R.m[r][c] += dR.m[r][c];
        orthonormalize(R);
        p->orientation = R;

        // sleeping
        if (b.v.dot(b.v) < 0.04f && b.w.dot(b.w) < 0.04f) {
            b.sleepTimer += dt;
            if (b.sleepTimer > 0.5f) { b.sleeping = true; b.v = b.w = Vec3(); }
        } else {
            b.sleepTimer = 0;
        }
    }
}
