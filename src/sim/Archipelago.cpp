#include "sim/Archipelago.h"
#include <cmath>
#include <algorithm>

namespace aethernaut {

void Archipelago::generate(uint64_t seed) {
    rng_.seedState(seed, 0x9E3779B97F4A7C15ULL);
    islands.clear();
    storms.clear();

    struct IslandPreset {
        const char* name;
        SkyBiome biome;
        Vec3 pos;
        float radius;
        bool hasPort;
        bool hasLighthouse;
    };

    const IslandPreset presets[] = {
        {"New Horizon Drydock", SkyBiome::AzureHaven, {0.0f, 108.0f, -140.0f}, 62.0f, true, true},
        {"Gale-Watch Outpost", SkyBiome::WindmillSteppe, {380.0f, 132.0f, -240.0f}, 54.0f, true, false},
        {"Shattered Prism Atoll", SkyBiome::CrystalReef, {-360.0f, 145.0f, -380.0f}, 58.0f, false, false},
        {"Sun-Crest Sanctuary", SkyBiome::AzureHaven, {-480.0f, 118.0f, 220.0f}, 64.0f, true, true},
        {"Cinder-Anvil Bastion", SkyBiome::ObsidianCrag, {560.0f, 95.0f, 360.0f}, 68.0f, false, false},
        {"Whispering Spire Reef", SkyBiome::CrystalReef, {160.0f, 165.0f, -680.0f}, 50.0f, false, true},
        {"Zephyr Trade Hub", SkyBiome::WindmillSteppe, {-140.0f, 125.0f, 540.0f}, 60.0f, true, false},
        {"Dread-Maw Monolith", SkyBiome::MaelstromEye, {0.0f, 138.0f, -1150.0f}, 82.0f, false, true}
    };

    int nextId = 1;
    for (const auto& p : presets) {
        SkyIsland isl;
        isl.id = nextId++;
        isl.name = p.name;
        isl.biome = p.biome;
        isl.position = p.pos;
        isl.radius = p.radius;
        isl.thickness = p.radius * 0.72f;
        isl.spireHeight = p.radius * 0.55f;
        isl.hasDrydockPort = p.hasPort;
        isl.hasLighthouse = p.hasLighthouse;
        isl.discovered = (isl.id == 1);

        switch (p.biome) {
            case SkyBiome::AzureHaven:
                isl.topColor = {76, 158, 86};
                isl.rockColor = {122, 116, 108};
                break;
            case SkyBiome::WindmillSteppe:
                isl.topColor = {198, 162, 74};
                isl.rockColor = {138, 112, 88};
                break;
            case SkyBiome::CrystalReef:
                isl.topColor = {68, 152, 178};
                isl.rockColor = {78, 88, 112};
                break;
            case SkyBiome::ObsidianCrag:
                isl.topColor = {88, 62, 58};
                isl.rockColor = {48, 44, 52};
                break;
            case SkyBiome::MaelstromEye:
                isl.topColor = {62, 52, 92};
                isl.rockColor = {38, 36, 54};
                break;
        }

        // Populate harvestable Aetherium Crystal clusters along island rim
        int crystalCount = (p.biome == SkyBiome::CrystalReef) ? 6 : 3;
        for (int c = 0; c < crystalCount; ++c) {
            float angle = (static_cast<float>(c) / static_cast<float>(crystalCount)) * kTwoPi + rng_.range(-0.2f, 0.2f);
            float dist = isl.radius * rng_.range(0.55f, 0.82f);
            AetherCrystalNode node;
            node.worldPos = isl.position + Vec3{
                std::cos(angle) * dist,
                3.5f,
                std::sin(angle) * dist
            };
            node.radius = 4.2f;
            node.hp = 80.0f;
            node.aetherYield = (p.biome == SkyBiome::CrystalReef) ? 8 : 4;
            node.goldYield = 40;
            isl.crystals.push_back(node);
        }

        islands.push_back(isl);
    }

    // Add 6 procedural outer islands for exploration
    const char* extraNames[] = {
        "Mist-Veil Cay", "Copper-Prospector Rock", "Siren's Roost",
        "Thunderhead Mesa", "Starlight Pinnacle", "Forgotten Reliquary"
    };
    for (int i = 0; i < 6; ++i) {
        float angle = (static_cast<float>(i) / 6.0f) * kTwoPi + 0.45f;
        float dist = rng_.range(780.0f, 1180.0f);
        SkyIsland isl;
        isl.id = nextId++;
        isl.name = extraNames[i];
        isl.biome = static_cast<SkyBiome>(i % 4);
        isl.position = {
            std::cos(angle) * dist,
            rng_.range(90.0f, 175.0f),
            std::sin(angle) * dist
        };
        isl.radius = rng_.range(44.0f, 64.0f);
        isl.thickness = isl.radius * 0.7f;
        isl.spireHeight = isl.radius * 0.5f;
        isl.hasDrydockPort = (i % 3 == 0);
        isl.hasLighthouse = (i % 2 == 1);
        isl.topColor = {88, 145, 98};
        isl.rockColor = {108, 102, 96};

        for (int c = 0; c < 4; ++c) {
            float cAngle = static_cast<float>(c) * (kPi * 0.5f);
            AetherCrystalNode node;
            node.worldPos = isl.position + Vec3{std::cos(cAngle) * (isl.radius * 0.65f), 3.5f, std::sin(cAngle) * (isl.radius * 0.65f)};
            isl.crystals.push_back(node);
        }
        islands.push_back(isl);
    }

    // Spawn 2 roaming storm cells
    StormCell s1;
    s1.center = {420.0f, 140.0f, -520.0f};
    s1.velocity = {-6.0f, 0.0f, 4.0f};
    s1.radius = 280.0f;
    storms.push_back(s1);

    StormCell s2;
    s2.center = {-550.0f, 140.0f, -200.0f};
    s2.velocity = {5.0f, 0.0f, -5.0f};
    s2.radius = 260.0f;
    storms.push_back(s2);
}

void Archipelago::update(float dt) {
    // Slow day/sun cycle
    timeOfDay += dt * 0.0025f;
    if (timeOfDay > 1.0f) {
        timeOfDay -= 1.0f;
    }

    for (auto& storm : storms) {
        storm.center += storm.velocity * dt;
        if (storm.center.length() > 1400.0f) {
            storm.velocity = -storm.velocity;
        }
        if (storm.lightningFlashAlpha > 0.0f) {
            storm.lightningFlashAlpha = std::max(0.0f, storm.lightningFlashAlpha - dt * 2.8f);
        }
        storm.lightningTimer -= dt;
        if (storm.lightningTimer <= 0.0f) {
            storm.lightningTimer = rng_.range(3.5f, 7.5f);
            storm.lightningFlashAlpha = 1.0f;
            float a = rng_.range(0.0f, kTwoPi);
            float r = rng_.range(20.0f, storm.radius * 0.7f);
            storm.lastLightningPos = storm.center + Vec3{std::cos(a) * r, 0.0f, std::sin(a) * r};
        }
    }
}

Vec3 Archipelago::sampleWindAt(const Vec3& worldPos) const {
    Vec3 wind = baseWind;
    for (const auto& storm : storms) {
        Vec3 diff = worldPos - storm.center;
        diff.y = 0.0f;
        float dist = diff.length();
        if (dist < storm.radius && dist > 1.0f) {
            float factor = 1.0f - (dist / storm.radius);
            Vec3 tangent{-diff.z / dist, 0.0f, diff.x / dist};
            wind += tangent * (factor * 18.0f);
        }
    }
    return wind;
}

const SkyIsland* Archipelago::findNearestPort(const Vec3& pos, float& outDist) const {
    const SkyIsland* best = nullptr;
    float bestD = 1e30f;
    for (const auto& isl : islands) {
        if (!isl.hasDrydockPort) {
            continue;
        }
        float d = (isl.position - pos).length();
        if (d < bestD) {
            bestD = d;
            best = &isl;
        }
    }
    outDist = bestD;
    return best;
}

} // namespace aethernaut
