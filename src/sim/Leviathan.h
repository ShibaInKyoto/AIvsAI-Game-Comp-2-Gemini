#pragma once

#include "core/Math3D.h"
#include "sim/BlockRegistry.h"
#include <vector>
#include <string>

namespace aethernaut {

enum class LeviathanSpecies {
    AzureSkyWhale = 0,
    CrimsonStormWyrm = 1,
    ObsidianDreadManta = 2,
    MaelstromSovereign = 3,
    GlacialCryoKraken = 4,
    SolarPhoenixSerpent = 5
};

enum class LeviathanState {
    PatrolCruise,
    StalkingOrbit,
    BreachCharge,
    ArtilleryBarrage,
    TetherStruggle,
    StaggeredExposed
};

struct LeviathanSegment {
    Vec3 position{0.0f, 120.0f, 0.0f};
    Vec3 forward{0.0f, 0.0f, -1.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    float radius = 5.0f;
    float jointSpacing = 6.5f;
    float armorHp = 200.0f;
    float maxArmorHp = 200.0f;
    bool hasWeakSpotCore = false;
    bool hasLateralFins = false;
    float finSpan = 0.0f;
};

struct LeviathanAttackBolt {
    Vec3 origin;
    Vec3 velocity;
    float damage = 45.0f;
    ColorRGB color{110, 215, 255};
};

class Leviathan {
public:
    Leviathan(
        int id,
        LeviathanSpecies species,
        const Vec3& spawnPos,
        uint64_t seed = 1337ULL
    );

    int id() const { return id_; }
    LeviathanSpecies species() const { return species_; }
    const std::string& name() const { return name_; }
    const std::string& title() const { return title_; }
    const std::string& signatureMechanicName() const { return signatureMechanicName_; }
    const std::string& counterplayHint() const { return counterplayHint_; }

    void update(
        float dt,
        const Vec3& primaryTargetPos,
        const Vec3& externalTetherPullForce,
        int activeTetherCount,
        std::vector<LeviathanAttackBolt>& outAttacks
    );

    // Trigger species-specific tactical counterplay disruption (Sonar, Searchlight, EMP, Tether Reel, Thermal)
    bool triggerCounterplayDisruption(const char* sourceReason);

    // Apply damage to a specific segment; returns actual damage dealt to total HP
    float damageSegment(int segmentIdx, float rawDamage, bool isHarpoonHit);

    // Raycast against all 3D spheres of the Leviathan's articulated spine
    bool raycast(const Ray3D& ray, float maxDist, int& outSegmentIdx, Vec3& outHitPos, float& outDist) const;

    std::vector<LeviathanSegment> segments;
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    Vec3 homeCenter{0.0f, 130.0f, 0.0f};
    LeviathanState aiState = LeviathanState::PatrolCruise;

    float totalHp = 2000.0f;
    float maxTotalHp = 2000.0f;
    float swimPhase = 0.0f;
    float stateTimer = 8.0f;
    float attackCooldown = 3.0f;
    float jawAngle = 0.0f;

    // Boss Puzzle Barrier & Resonance Stagger state
    bool barrierActive = false;
    float barrierCycleTimer = 9.0f;
    float staggeredWindowTimer = 0.0f;
    int timesDisrupted = 0;

    int goldBounty = 450;
    int aetherBounty = 18;
    ColorRGB bodyColor{45, 115, 185};
    ColorRGB coreColor{65, 235, 255};
    bool isSlain = false;

private:
    int id_ = 0;
    LeviathanSpecies species_;
    std::string name_;
    std::string title_;
    std::string signatureMechanicName_;
    std::string counterplayHint_;
    Rng32 rng_;
};

} // namespace aethernaut
