#pragma once

#include "sim/World.h"
#include "raylib.h"
#include <vector>

namespace aethernaut {

class SynthAudio {
public:
    void init(bool enableAudio);
    void shutdown();

    void processWorldEvents(World& world, const Vec3& listenerPos, float sfxVol, float musicVol);

private:
    Sound generateCannonBoom();
    Sound generateHarpoonLaunch();
    Sound generateHarpoonLatch();
    Sound generateCableSnap();
    Sound generateBlockImpact();
    Sound generateStructuralFracture();
    Sound generateLeviathanRoar();
    Sound generateDiscoveryChime();
    Sound generateSkyVoyageMusicLoop();

    bool audioEnabled_ = false;
    Sound sfxCannon_{};
    Sound sfxHarpoon_{};
    Sound sfxLatch_{};
    Sound sfxSnap_{};
    Sound sfxImpact_{};
    Sound sfxFracture_{};
    Sound sfxRoar_{};
    Sound sfxChime_{};
    Sound musicLoop_{};
};

} // namespace aethernaut
