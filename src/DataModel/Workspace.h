#pragma once
#include "Part.h"

class Workspace : public Instance {
public:
    Workspace() { name = "Workspace"; }
    std::string className() const override { return "Workspace"; }

    std::shared_ptr<Part> createPart(const std::string& partName, const Point3& pos,
                                     const Vector3& size, const Color3& color);

    // Collect every Part under the workspace (recursive)
    void collectParts(std::vector<Part*>& out) const;

    // Nearest Part hit by the ray, or nullptr
    Part* pick(const Ray& ray) const;
};
