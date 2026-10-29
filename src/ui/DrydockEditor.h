#pragma once

#include "sim/World.h"
#include "render/WorldRenderer.h"
#include "raylib.h"

namespace aethernaut {

class DrydockEditor {
public:
    void update(World& world, const Camera3D& camera, int screenW, int screenH, bool& outExitDrydock);
    void drawUI(World& world, int screenW, int screenH, bool& outExitDrydock);

    DrydockGhostPreview ghostPreview{};
    BlockId selectedBlock = BlockId::WoodHull;
    uint8_t selectedOrientation = 0;
    bool symmetryMirrorX = true;
    bool showEngineeringGizmos = true;
};

} // namespace aethernaut
