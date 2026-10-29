#pragma once

#include "core/Math3D.h"
#include "sim/BlockRegistry.h"
#include <vector>
#include <string>

namespace aethernaut {

enum class SkyBiome {
    AzureHaven = 0,
    WindmillSteppe = 1,
    CrystalReef = 2,
    ObsidianCrag = 3,
    MaelstromEye = 4
};

struct AetherCrystalNode {
    Vec3 worldPos;
    float radius = 4.5f;
    float hp = 90.0f;
    int aetherYield = 6;
    int goldYield = 45;
    bool harvested = false;
};

struct SkyIsland {
    int id = 0;
    std::string name;
    SkyBiome biome = SkyBiome::AzureHaven;
    Vec3 position{0.0f, 110.0f, 0.0f}; // Center of top deck plateau
    float radius = 55.0f;
    float thickness = 38.0f;
    float spireHeight = 28.0f;
    bool hasDrydockPort = false;
    bool hasLighthouse = false;
    bool discovered = false;
    ColorRGB topColor{78, 155, 88};
    ColorRGB rockColor{110, 105, 98};
    std::vector<AetherCrystalNode> crystals;
};

struct SalvageCrate {
    int id = 0;
    Vec3 position;
    Vec3 velocity;
    int goldAmount = 50;
    int aetherAmount = 3;
    float lifetime = 60.0f;
    bool collected = false;
};

struct StormCell {
    Vec3 center{400.0f, 150.0f, -400.0f};
    Vec3 velocity{8.0f, 0.0f, 5.0f};
    float radius = 320.0f;
    float lightningTimer = 4.0f;
    Vec3 lastLightningPos{0.0f, 0.0f, 0.0f};
    float lightningFlashAlpha = 0.0f;
};

class Archipelago {
public:
    void generate(uint64_t seed);
    void update(float dt);

    Vec3 sampleWindAt(const Vec3& worldPos) const;
    const SkyIsland* findNearestPort(const Vec3& pos, float& outDist) const;

    std::vector<SkyIsland> islands;
    std::vector<StormCell> storms;
    Vec3 baseWind{4.5f, 0.0f, -2.5f};
    float timeOfDay = 0.32f; // 0..1 cycle (0.25 = morning, 0.5 = high noon, 0.78 = sunset)

private:
    Rng32 rng_;
};

} // namespace aethernaut
