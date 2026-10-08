#pragma once
#include <G3D/G3DAll.h>
#include "Instance.h"

class Part : public Instance {
public:
    CFrame  cframe;
    Vector3 size     = Vector3(4, 1.2f, 2);        // classic brick
    Color3  color    = Color3(0.64f, 0.64f, 0.64f);
    bool    anchored = true;

    Part() { name = "Part"; }
    std::string className() const override { return "Part"; }

    Box box() const { return cframe.toWorldSpace(AABox(-size / 2.0f, size / 2.0f)); }
};
