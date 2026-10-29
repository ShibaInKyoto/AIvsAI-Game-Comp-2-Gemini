#pragma once

#include "core/Math3D.h"
#include "core/SpatialHash3D.h"
#include "sim/Airship.h"
#include "sim/ShipBlueprints.h"
#include "sim/Leviathan.h"
#include "sim/Ballistics.h"
#include "sim/Archipelago.h"
#include "sim/AIController.h"
#include <vector>
#include <string>

namespace aethernaut {

enum class SimAudioEventType {
    CannonFire,
    HarpoonLaunch,
    HarpoonLatch,
    CableSnap,
    TeslaDischarge,
    BlockImpact,
    StructuralFracture,
    LeviathanRoar,
    CratePickup,
    RepairPulse
};

struct SimAudioEvent {
    SimAudioEventType type;
    Vec3 worldPos;
    float intensity = 1.0f;
};

struct BountyContract {
    int id = 0;
    std::string title;
    std::string targetName;
    Vec3 lastKnownPos;
    int rewardGold = 500;
    int rewardAether = 20;
    bool completed = false;
};

struct CombatCallout3D {
    Vec3 worldPos;
    std::string text;
    ColorRGB color{255, 220, 95};
    float lifetime = 1.6f;
    float maxLifetime = 1.6f;
};

class World {
public:
    World();

    void initWorld(uint64_t seed, int playerBlueprintIdx = 0, bool sandboxMode = false);
    void spawnLeviathanEncounter(LeviathanSpecies species, const Vec3& pos);
    void spawnPirateSquadron(int count, const Vec3& centerPos);
    void spawnAlliedWingman(int blueprintIdx, const Vec3& pos);
    bool hireAlliedWingman();

    // Advance 3D simulation by dt seconds
    void step(float dt);

    // Player weapon & tether actions
    void firePlayerWeapons(bool cannons, bool harpoons, const Vec3& aimWorldPos);
    void setReelHarpoons(bool reelIn);
    void releaseAllPlayerHarpoons();

    // Walk-on-Deck movement step
    void updateCaptainDeckWalk(float dt, float moveForward, float moveRight);

    Airship playerShip;
    std::vector<Airship> npcShips;
    std::vector<Leviathan> leviathans;
    Archipelago archipelago;

    std::vector<Projectile3D> projectiles;
    std::vector<HarpoonTether> tethers;
    std::vector<DetachedDebris> debris;
    std::vector<SalvageCrate> crates;
    std::vector<ExplosionEvent3D> explosions;
    std::vector<CombatCallout3D> callouts;
    std::vector<SimAudioEvent> pendingAudioEvents;
    std::vector<BountyContract> contracts;

    // Player Resources & Progression
    int playerGold = 450;
    int playerAether = 24;
    int leviathansSlain = 0;
    int piratesSunk = 0;
    int blocksDestroyedStat = 0;
    float cameraTrauma = 0.0f;
    bool isSandboxMode = false;
    bool playerAutopilot = false;
    bool isPlayerWalkingOnDeck = false;
    Vec3 playerAimWorldPos{0.0f, 120.0f, -150.0f};
    std::string statusBannerText = "Welcome to New Horizon Sky-Archipelago! Hold LMB for Broadsides, RMB for Harpoons.";
    float statusBannerTimer = 8.0f;

private:
    void stepProjectiles(float dt);
    void stepHarpoonTethers(float dt);
    void stepDebrisAndCrates(float dt);

    int nextEntityId_ = 100;
    Rng32 rng_;
    SpatialHash3D spatialHash_;
};

} // namespace aethernaut
