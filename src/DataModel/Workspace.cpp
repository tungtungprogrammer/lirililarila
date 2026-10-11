#include "Workspace.h"

std::shared_ptr<Part> Workspace::createPart(const PartState& state) {
    auto p = std::make_shared<Part>();
    p->setState(state);
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
