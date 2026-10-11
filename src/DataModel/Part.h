#pragma once
#include "BrickColor.h"
#include "Instance.h"
#include "VecMath.h"

enum class SurfaceType { Smooth = 0, Studs = 1 };
enum class PartShape { Block = 0, Ball = 1, Cylinder = 2, Wedge = 3 };

// Everything about a Part that can be edited, used for undo/redo, copy/paste and save/load.
struct PartState {
    std::string name = "Part";
    Vec3  position;
    Mat3  orientation;
    Vec3  size = Vec3(4, 1.2f, 2);
    int   brickColor = kBrickMediumStoneGrey;
    bool  anchored = true;
    SurfaceType topSurface = SurfaceType::Studs;
    PartShape shape = PartShape::Block;
    float transparency = 0.0f;     // 0 opaque .. 1 invisible (stored in steps of 0.1)
    bool  locked = false;          // cannot be picked in the viewport

    bool operator==(const PartState& o) const {
        return name == o.name && position == o.position && orientation == o.orientation && size == o.size &&
               brickColor == o.brickColor && anchored == o.anchored && topSurface == o.topSurface &&
               shape == o.shape && transparency == o.transparency && locked == o.locked;
    }
    bool operator!=(const PartState& o) const { return !(*this == o); }
};

class Part : public Instance {
public:
    Vec3  position;
    Mat3  orientation;
    Vec3  size = Vec3(4, 1.2f, 2);
    int   brickColor = kBrickMediumStoneGrey;
    bool  anchored = true;
    SurfaceType topSurface = SurfaceType::Studs;
    PartShape shape = PartShape::Block;
    float transparency = 0.0f;
    bool  locked = false;

    Part() { name = "Part"; }
    std::string className() const override { return "Part"; }

    PartState state() const;
    void setState(const PartState& s);
};
