#include "Workspace.h"

std::shared_ptr<Part> Workspace::createPart(const std::string& partName, const Point3& pos,
                                            const Vector3& size, const Color3& color) {
    auto p = std::make_shared<Part>();
    p->name = partName;
    p->cframe = CFrame(pos);
    p->size = size;
    p->color = color;
    addChild(p);
    return p;
}

static void collect(const Instance* node, std::vector<Part*>& out) {
    for (const auto& c : node->children()) {
        if (Part* p = dynamic_cast<Part*>(c.get())) { out.push_back(p); }
        collect(c.get(), out);
    }
}

void Workspace::collectParts(std::vector<Part*>& out) const { collect(this, out); }

Part* Workspace::pick(const Ray& ray) const {
    std::vector<Part*> parts;
    collectParts(parts);
    Part* best = nullptr;
    float bestT = finf();
    for (Part* p : parts) {
        float t = ray.intersectionTime(p->box());
        if (t < bestT) { bestT = t; best = p; }
    }
    return best;
}
