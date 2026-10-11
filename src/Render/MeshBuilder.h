#pragma once
#include <vector>
#include "DataModel/Workspace.h"
#include "Geom.h"

// CPU-side triangle mesh with the attributes the Studio shader uses.
struct MeshData {
    std::vector<float> pos, nrm, col, aux;   // 3 floats per vertex each
    std::vector<int>   idx;

    int  vertexCount() const { return int(pos.size() / 3); }
    void clear() { pos.clear(); nrm.clear(); col.clear(); aux.clear(); idx.clear(); }
    int  addVertex(const Vec3& p, const Vec3& n, const Vec3& c, const Vec3& a = Vec3());
    // Adds a triangle, flipping the winding if needed so it faces along n.
    void addTri(int a, int b, int c, const Vec3& n);
    void addQuad(int a, int b, int c, int d, const Vec3& n);   // a,b,c,d in order around the quad
};

namespace meshbuilder {

Vec3 brickRgb(int brickColorId);   // 0..1 sRGB

// A Part as a box, plus studs on top (real geometry for small parts, a shader pattern for huge ones).
void addPart(MeshData& m, const Part& part);

void addBox(MeshData& m, const Vec3& center, const Mat3& rot, const Vec3& half, const Vec3& color);
// Arrow along dir: shaft + pyramid head. base = start of shaft.
void addArrow(MeshData& m, const Vec3& base, const Vec3& dir, float length, float radius, const Vec3& color);
// 12 thin bars around an oriented box (the blue selection box).
void addFrame(MeshData& m, const Vec3& center, const Mat3& rot, const Vec3& half, float thickness, const Vec3& color);

}  // namespace meshbuilder
