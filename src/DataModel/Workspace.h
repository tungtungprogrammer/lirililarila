#pragma once
#include "Part.h"

class Workspace : public Instance {
public:
    Workspace() { name = "Workspace"; }
    std::string className() const override { return "Workspace"; }

    std::shared_ptr<Part> createPart(const PartState& state);

    // All Parts under the workspace (recursive), in tree order.
    void collectParts(std::vector<Part*>& out) const;
};
