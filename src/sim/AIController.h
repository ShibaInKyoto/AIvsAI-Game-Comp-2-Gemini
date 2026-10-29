#pragma once

#include "sim/Airship.h"
#include "sim/Archipelago.h"
#include "sim/Leviathan.h"
#include "sim/Ballistics.h"
#include <vector>

namespace aethernaut {

struct AIDecision {
    float throttle = 0.8f;
    float rudder = 0.0f;
    float altitude = 0.0f;
    Vec3 aimWorldPos{0.0f, 120.0f, -100.0f};
    bool fireCannons = false;
    bool fireHarpoons = false;
};

class AIController {
public:
    static AIDecision computeShipControl(
        const Airship& selfShip,
        const Airship* playerShip,
        const std::vector<Airship>& allNpcShips,
        const std::vector<Leviathan>& leviathans,
        const Archipelago& archipelago,
        bool isPlayerAutopilot
    );
};

} // namespace aethernaut
