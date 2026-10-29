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
#include <array>

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

struct OfficerSpec {
    const char* name;
    const char* role;
    const char* bonusDesc;
    int baseGoldCost;
    int baseAetherCost;
};

constexpr int kRelicCount = 5;

struct RelicSpec {
    const char* name;
    const char* originLore;
    const char* bonusDesc;
    int goldCost;
    int aetherCost;
};

enum class FleetOrder {
    EscortFormation = 0,
    FocusMyTarget = 1,
    FreeHunt = 2,
    DefensivePerimeter = 3
};

enum class WeatherState {
    ClearTradeWinds = 0,
    CirrusMist = 1,
    IonAetherSquall = 2,
    SupercellTempest = 3
};

class World {
public:
    World();

    void initWorld(uint64_t seed, int playerBlueprintIdx = 0, bool sandboxMode = false);
    void spawnLeviathanEncounter(LeviathanSpecies species, const Vec3& pos);
    void spawnPirateSquadron(int count, const Vec3& centerPos);
    void spawnAlliedWingman(int blueprintIdx, const Vec3& pos);
    bool hireAlliedWingman();
    void summonApexLeviathan();

    // Officer Crew Roster & Upgrades (0..4)
    static const std::array<OfficerSpec, 5>& getOfficerSpecs();
    bool upgradeOfficer(int officerIdx);
    void applyCrewMultipliers();

    // Ancient Aether-Relic Vault (0..4)
    static const std::array<RelicSpec, kRelicCount>& getRelicSpecs();
    bool unlockRelic(int relicIdx);
    int unlockedRelicCount() const;

    // Fleet Admiral Squadron Orders, Weather & Hull Livery Control
    void cycleFleetOrder();
    static const char* getFleetOrderName(FleetOrder order);
    void cycleWeatherState();
    static const char* getWeatherName(WeatherState w);
    void cyclePlayerLivery();

    // Island Commodity Market Trading
    int totalCargoUnits() const;
    int maxCargoCapacity() const;
    bool buyCommodity(int commodityIdx, int qty = 1);
    bool sellCommodity(int commodityIdx, int qty = 1);

    // Active Tactical Ship Abilities
    bool triggerDamageControl();
    bool triggerSonarPing();

    // Campaign Save / Load
    bool saveCampaign(const std::string& filepath);
    bool loadCampaign(const std::string& filepath);

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

    // Player Resources, Cargo Hold, Crew Ranks, Relics & Progression
    int playerGold = 450;
    int playerAether = 24;
    int leviathansSlain = 0;
    int piratesSunk = 0;
    int citadelsLiberatedStat = 0;
    int blocksDestroyedStat = 0;
    int tradeProfitStat = 0;
    int prestigeHuntTier = 1;
    std::array<int, 5> officerRanks{{0, 0, 0, 0, 0}}; // 0..3 per officer
    std::array<int, kCommodityCount> playerCargo{{0, 0, 0, 0}};
    std::array<bool, kRelicCount> unlockedRelics{{false, false, false, false, false}};
    FleetOrder currentFleetOrder = FleetOrder::EscortFormation;
    WeatherState currentWeather = WeatherState::ClearTradeWinds;

    // Tactical Ability Cooldowns & States
    float damageControlCooldown = 0.0f;
    float sonarPingCooldown = 0.0f;
    float sonarMarkTimer = 0.0f;
    float sonarWaveRadius = 0.0f;
    Vec3 sonarWaveOrigin{0.0f, 0.0f, 0.0f};
    float weatherTimer = 0.0f;
    int skyAtmospherePreset = 0; // 0: Golden Dawn, 1: High Noon, 2: Crimson Dusk, 3: Starlit Night

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
