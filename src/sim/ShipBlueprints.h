#pragma once

#include "sim/Airship.h"
#include <string>
#include <vector>

namespace aethernaut {

struct ShipBlueprintInfo {
    int index = 0;
    const char* name = "";
    const char* role = "";
    const char* description = "";
};

class ShipBlueprints {
public:
    static int count();
    static ShipBlueprintInfo getInfo(int index);
    static void applyBlueprint(Airship& ship, int index);

    // Save/Load player-engineered 3D voxel airships to disk (.aeth format)
    static bool saveCustomBlueprint(const Airship& ship, const std::string& filepath);
    static bool loadCustomBlueprint(Airship& ship, const std::string& filepath);
};

} // namespace aethernaut
