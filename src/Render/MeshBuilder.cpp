#include "MeshBuilder.h"

namespace {
const float kStudHeight = 0.2f;
const float kStudRadius = 0.3f;
const int   kStudSides = 12;
const int   kMaxRealStuds = 600;   // above this a part gets the shader stud pattern instead
const float kPi = 3.14159265f;
}

int MeshData::addVertex(const Vec3& p, const Vec3& n, const Vec3& c, const Vec3& a) {
    pos.insert(pos.end(), {p.x, p.y, p.z});
    nrm.insert(nrm.end(), {n.x, n.y, n.z});
    col.insert(col.end(), {c.x, c.y, c.z});
    aux.insert(aux.end(), {a.x, a.y, a.z});
    return vertexCount() - 1;
}

void MeshData::addTri(int a, int b, int c, const Vec3& n) {
    auto P = [&](int i) { return Vec3(pos[i * 3], pos[i * 3 + 1], pos[i * 3 + 2]); };
    const Vec3 g = geom::cross(P(b) - P(a), P(c) - P(a));
    if (g.dot(n) >= 0) idx.insert(idx.end(), {a, b, c});
    else               idx.insert(idx.end(), {a, c, b});
}

void MeshData::addQuad(int a, int b, int c, int d, const Vec3& n) {
    addTri(a, b, c, n);
    addTri(a, c, d, n);
}

namespace meshbuilder {

Vec3 brickRgb(int id) {
    const BrickColorInfo& b = brickColorById(id);
    return Vec3(b.r / 255.0f, b.g / 255.0f, b.b / 255.0f);
}

namespace {

struct Face { int n, u, v; float sign; };
// For each face: normal axis, tangent axes (u x v = +/- normal handled by addTri).
const Face kFaces[6] = {{0, 1, 2, +1}, {0, 2, 1, -1}, {1, 2, 0, +1}, {1, 0, 2, -1}, {2, 0, 1, +1}, {2, 1, 0, -1}};


void addStud(MeshData& m, const Vec3& base, const Mat3& rot, const Vec3& color, const Vec3& aux) {
    const Vec3 up = geom::column(rot, 1);
    int ring0[kStudSides], ring1[kStudSides];
    Vec3 radial[kStudSides];
    for (int i = 0; i < kStudSides; ++i) {
        const float a = 2.0f * kPi * i / kStudSides;
        const Vec3 l(std::cos(a) * kStudRadius, 0, std::sin(a) * kStudRadius);
        const Vec3 w = rot * l;
        radial[i] = geom::normalize(w);
        ring0[i] = m.addVertex(base + w, radial[i], color, aux);
        ring1[i] = m.addVertex(base + w + up * kStudHeight, radial[i], color, aux);
    }
    for (int i = 0; i < kStudSides; ++i) {
        const int j = (i + 1) % kStudSides;
        m.addQuad(ring0[i], ring0[j], ring1[j], ring1[i], radial[i] + radial[j]);
    }
    // top cap with its own vertices so the normal is flat
    const int centre = m.addVertex(base + up * kStudHeight, up, color, aux);
    int cap[kStudSides];
    for (int i = 0; i < kStudSides; ++i) {
        const float a = 2.0f * kPi * i / kStudSides;
        cap[i] = m.addVertex(base + rot * Vec3(std::cos(a) * kStudRadius, kStudHeight, std::sin(a) * kStudRadius), up, color, aux);
    }
    for (int i = 0; i < kStudSides; ++i) m.addTri(centre, cap[i], cap[(i + 1) % kStudSides], up);
}

}  // namespace

void addBox(MeshData& m, const Vec3& center, const Mat3& rot, const Vec3& half, const Vec3& color) {
    for (const Face& f : kFaces) {
        const Vec3 n = geom::column(rot, f.n) * f.sign;
        const Vec3 u = geom::column(rot, f.u) * geom::comp(half, f.u);
        const Vec3 v = geom::column(rot, f.v) * geom::comp(half, f.v);
        const Vec3 c = center + n * geom::comp(half, f.n);
        const int a = m.addVertex(c - u - v, n, color);
        const int b = m.addVertex(c + u - v, n, color);
        const int d = m.addVertex(c + u + v, n, color);
        const int e = m.addVertex(c - u + v, n, color);
        m.addQuad(a, b, d, e, n);
    }
}

namespace {

// aux.z packs two things: bit 0 = "paint studs in the shader", the rest = 2 * transparency in tenths.
float auxZ(const Part& p, bool shaderStuds) {
    const int tenths = std::max(0, std::min(10, int(std::floor(p.transparency * 10.0f + 0.5f))));
    return (shaderStuds ? 1.0f : 0.0f) + 2.0f * float(tenths);
}

void addEllipsoid(MeshData& m, const Part& p, const Vec3& color, const Vec3& aux) {
    const int slices = 20, stacks = 12;
    const Vec3 half = p.size * 0.5f;
    std::vector<int> v;
    for (int i = 0; i <= stacks; ++i) {
        const float phi = kPi * i / stacks;                       // 0 = top pole
        for (int k = 0; k <= slices; ++k) {
            const float th = 2.0f * kPi * k / slices;
            const Vec3 d(std::sin(phi) * std::cos(th), std::cos(phi), std::sin(phi) * std::sin(th));
            const Vec3 local(d.x * half.x, d.y * half.y, d.z * half.z);
            // normal of an ellipsoid: gradient of the implicit surface
            const Vec3 nl(d.x / std::max(half.x, 0.01f), d.y / std::max(half.y, 0.01f), d.z / std::max(half.z, 0.01f));
            v.push_back(m.addVertex(p.position + p.orientation * local, geom::normalize(p.orientation * nl), color, aux));
        }
    }
    for (int i = 0; i < stacks; ++i)
        for (int k = 0; k < slices; ++k) {
            const int a = v[i * (slices + 1) + k], b = v[i * (slices + 1) + k + 1];
            const int c = v[(i + 1) * (slices + 1) + k + 1], d = v[(i + 1) * (slices + 1) + k];
            const Vec3 n(m.nrm[a * 3] + m.nrm[c * 3], m.nrm[a * 3 + 1] + m.nrm[c * 3 + 1], m.nrm[a * 3 + 2] + m.nrm[c * 3 + 2]);
            m.addTri(a, b, c, n);
            m.addTri(a, c, d, n);
        }
}

// Cylinder along the part's local X axis (like the original).
void addCylinder(MeshData& m, const Part& p, const Vec3& color, const Vec3& aux) {
    const int sides = 24;
    const Vec3 half = p.size * 0.5f;
    const Vec3 ax = geom::column(p.orientation, 0);
    int ringA[sides + 1], ringB[sides + 1], capA[sides + 1], capB[sides + 1];
    for (int k = 0; k <= sides; ++k) {
        const float a = 2.0f * kPi * k / sides;
        const Vec3 local(0, std::cos(a) * half.y, std::sin(a) * half.z);
        const Vec3 nl(0, std::cos(a) / std::max(half.y, 0.01f), std::sin(a) / std::max(half.z, 0.01f));
        const Vec3 n = geom::normalize(p.orientation * nl);
        const Vec3 w = p.orientation * local;
        ringA[k] = m.addVertex(p.position - ax * half.x + w, n, color, aux);
        ringB[k] = m.addVertex(p.position + ax * half.x + w, n, color, aux);
        capA[k] = m.addVertex(p.position - ax * half.x + w, ax * -1.0f, color, aux);
        capB[k] = m.addVertex(p.position + ax * half.x + w, ax, color, aux);
    }
    const int cA = m.addVertex(p.position - ax * half.x, ax * -1.0f, color, aux);
    const int cB = m.addVertex(p.position + ax * half.x, ax, color, aux);
    for (int k = 0; k < sides; ++k) {
        const Vec3 n(m.nrm[ringA[k] * 3] + m.nrm[ringA[k + 1] * 3], m.nrm[ringA[k] * 3 + 1] + m.nrm[ringA[k + 1] * 3 + 1],
                     m.nrm[ringA[k] * 3 + 2] + m.nrm[ringA[k + 1] * 3 + 2]);
        m.addQuad(ringA[k], ringA[k + 1], ringB[k + 1], ringB[k], n);
        m.addTri(cA, capA[k], capA[k + 1], ax * -1.0f);
        m.addTri(cB, capB[k], capB[k + 1], ax);
    }
}

// Right-angle wedge: vertical face at +Z, slope rising from the front-bottom edge to the back-top edge.
void addWedge(MeshData& m, const Part& p, const Vec3& color, const Vec3& aux) {
    const Vec3 h = p.size * 0.5f;
    auto W = [&](float x, float y, float z) { return p.position + p.orientation * Vec3(x * h.x, y * h.y, z * h.z); };
    auto N = [&](float x, float y, float z) { return geom::normalize(p.orientation * Vec3(x, y, z)); };
    auto quad = [&](const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& n) {
        const int ia = m.addVertex(a, n, color, aux), ib = m.addVertex(b, n, color, aux);
        const int ic = m.addVertex(c, n, color, aux), id = m.addVertex(d, n, color, aux);
        m.addQuad(ia, ib, ic, id, n);
    };
    auto tri = [&](const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& n) {
        const int ia = m.addVertex(a, n, color, aux), ib = m.addVertex(b, n, color, aux), ic = m.addVertex(c, n, color, aux);
        m.addTri(ia, ib, ic, n);
    };
    quad(W(-1, -1, -1), W(1, -1, -1), W(1, -1, 1), W(-1, -1, 1), N(0, -1, 0));               // bottom
    quad(W(-1, -1, 1), W(1, -1, 1), W(1, 1, 1), W(-1, 1, 1), N(0, 0, 1));                    // back
    const Vec3 sn = N(0, p.size.z, -p.size.y);                                               // slope
    quad(W(-1, -1, -1), W(1, -1, -1), W(1, 1, 1), W(-1, 1, 1), sn);
    tri(W(-1, -1, -1), W(-1, -1, 1), W(-1, 1, 1), N(-1, 0, 0));                              // sides
    tri(W(1, -1, -1), W(1, -1, 1), W(1, 1, 1), N(1, 0, 0));
}

}  // namespace

void addPart(MeshData& m, const Part& p) {
    const Vec3 color = brickRgb(p.brickColor);
    const Vec3 half = p.size * 0.5f;

    if (p.shape != PartShape::Block) {
        const Vec3 aux(0, 0, auxZ(p, false));
        if (p.shape == PartShape::Ball) addEllipsoid(m, p, color, aux);
        else if (p.shape == PartShape::Cylinder) addCylinder(m, p, color, aux);
        else addWedge(m, p, color, aux);
        return;
    }

    const bool studs = p.topSurface == SurfaceType::Studs;
    const int nx = std::max(1, int(std::floor(p.size.x + 0.5f)));
    const int nz = std::max(1, int(std::floor(p.size.z + 0.5f)));
    const bool realStuds = studs && nx * nz <= kMaxRealStuds;
    const bool shaderStuds = studs && !realStuds;
    const float az = auxZ(p, shaderStuds);

    for (const Face& f : kFaces) {
        const Vec3 n = geom::column(p.orientation, f.n) * f.sign;
        const Vec3 u = geom::column(p.orientation, f.u) * geom::comp(half, f.u);
        const Vec3 v = geom::column(p.orientation, f.v) * geom::comp(half, f.v);
        const Vec3 c = p.position + n * geom::comp(half, f.n);
        // Local coordinates on the top face (in studs), used by the shader pattern.
        const bool top = (f.n == 1 && f.sign > 0);
        auto aux = [&](float su, float sv) {
            if (!(top && shaderStuds)) return Vec3(0, 0, az - (shaderStuds ? 1.0f : 0.0f));
            const float uu = (f.u == 0 ? su * half.x : sv * half.x) + half.x;
            const float vv = (f.u == 0 ? sv * half.z : su * half.z) + half.z;
            return Vec3(uu, vv, az);
        };
        const int a = m.addVertex(c - u - v, n, color, aux(-1, -1));
        const int b = m.addVertex(c + u - v, n, color, aux(+1, -1));
        const int d = m.addVertex(c + u + v, n, color, aux(+1, +1));
        const int e = m.addVertex(c - u + v, n, color, aux(-1, +1));
        m.addQuad(a, b, d, e, n);
    }

    if (realStuds) {
        const Vec3 studAux(0, 0, az);
        for (int i = 0; i < nx; ++i)
            for (int k = 0; k < nz; ++k) {
                const Vec3 local(-half.x + (i + 0.5f) * p.size.x / nx, half.y, -half.z + (k + 0.5f) * p.size.z / nz);
                addStud(m, p.position + p.orientation * local, p.orientation, color, studAux);
            }
    }
}

void addArrow(MeshData& m, const Vec3& base, const Vec3& dir, float length, float radius, const Vec3& color) {
    const Vec3 d = geom::normalize(dir);
    Vec3 ref = std::fabs(d.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    const Vec3 s = geom::normalize(geom::cross(d, ref));
    const Vec3 t = geom::cross(d, s);
    // shaft: a thin box
    const float shaft = length * 0.6f, sr = radius * 0.35f;
    Mat3 r;
    for (int row = 0; row < 3; ++row) {
        r.m[row][0] = geom::comp(s, row);
        r.m[row][1] = geom::comp(t, row);
        r.m[row][2] = geom::comp(d, row);
    }
    addBox(m, base + d * (shaft * 0.5f), r, Vec3(sr, sr, shaft * 0.5f), color);
    // head: 4-sided pyramid
    const Vec3 hb = base + d * shaft, tip = base + d * length;
    const Vec3 corner[4] = {s * radius + t * radius, s * radius - t * radius, s * -radius - t * radius, s * -radius + t * radius};
    for (int i = 0; i < 4; ++i) {
        const Vec3 a = hb + corner[i], b = hb + corner[(i + 1) % 4];
        const Vec3 n = geom::normalize(geom::cross(b - a, tip - a));
        const int ia = m.addVertex(a, n, color), ib = m.addVertex(b, n, color), it = m.addVertex(tip, n, color);
        m.addTri(ia, ib, it, (corner[i] + corner[(i + 1) % 4]) * 0.5f + d * 0.3f);
    }
    const int c0 = m.addVertex(hb + corner[0], d * -1.0f, color), c1 = m.addVertex(hb + corner[1], d * -1.0f, color);
    const int c2 = m.addVertex(hb + corner[2], d * -1.0f, color), c3 = m.addVertex(hb + corner[3], d * -1.0f, color);
    m.addQuad(c0, c1, c2, c3, d * -1.0f);
}

void addFrame(MeshData& m, const Vec3& center, const Mat3& rot, const Vec3& half, float th, const Vec3& color) {
    for (int axis = 0; axis < 3; ++axis) {
        const int a1 = (axis + 1) % 3, a2 = (axis + 2) % 3;
        for (int s1 = -1; s1 <= 1; s1 += 2)
            for (int s2 = -1; s2 <= 1; s2 += 2) {
                Vec3 off, h;
                geom::setComp(off, a1, s1 * geom::comp(half, a1));
                geom::setComp(off, a2, s2 * geom::comp(half, a2));
                geom::setComp(h, axis, geom::comp(half, axis) + th);
                geom::setComp(h, a1, th);
                geom::setComp(h, a2, th);
                addBox(m, center + rot * off, rot, h, color);
            }
    }
}

}  // namespace meshbuilder
