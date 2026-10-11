#include "Part.h"

PartState Part::state() const {
    PartState s;
    s.name = name; s.position = position; s.orientation = orientation; s.size = size;
    s.brickColor = brickColor; s.anchored = anchored; s.topSurface = topSurface;
    s.shape = shape; s.transparency = transparency; s.locked = locked;
    return s;
}

void Part::setState(const PartState& s) {
    name = s.name; position = s.position; orientation = s.orientation; size = s.size;
    brickColor = s.brickColor; anchored = s.anchored; topSurface = s.topSurface;
    shape = s.shape; transparency = s.transparency; locked = s.locked;
}
