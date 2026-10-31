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

constexpr int kMedalCount = 10;

struct MedalSpec {
    const char* title;
    const char* conditionDesc;
    int rewardGold;
    int rewardAether;
};

struct AetherMeteor3D {
    int id = 0;
    Vec3 position{0.0f, 220.0f, 0.0f};
    Vec3 velocity{0.0f, -6.5f, 0.0f};
    float radius = 5.2f;
    float hp = 110.0f;
    float maxHp = 110.0f;
    int aetherYield = 14;
    int goldYield = 160;
    float spinAngle = 0.0f;
    bool active = true;
};

struct SkySurvivorPod {
    int id = 0;
    Vec3 position{0.0f, 138.0f, 0.0f};
    Vec3 velocity{0.0f, -0.6f, 0.0f};
    int goldReward = 140;
    int aetherReward = 5;
    float flarePhase = 0.0f;
    bool rescued = false;
};

// Ambient 3D Sky-Ecosystem Fauna: Cohesive Sky-Ray Boid Shoals
struct SkyRayBoid {
    Vec3 position{0.0f, 145.0f, 0.0f};
    Vec3 velocity{12.0f, 0.0f, 0.0f};
    Vec3 homeCenter{0.0f, 145.0f, 0.0f};
    float wingPhase = 0.0f;
    float scale = 2.4f;
    int flockId = 0;
    ColorRGB color{75, 195, 235};
};

// Ambient 3D Sky-Ecosystem Fauna: Bioluminescent Aether-Medusae
struct SkyJellyfish {
    Vec3 position{0.0f, 130.0f, 0.0f};
    float baseAltitude = 130.0f;
    float pulsePhase = 0.0f;
    float radius = 2.6f;
    ColorRGB bellColor{135, 235, 255};
    ColorRGB tentacleColor{235, 125, 255};
};

// 10 Discoverable Architectural & Natural Sky-Wonders
struct SkyWonder {
    int id = 0;
    std::string name;
    std::string subtitle;
    std::string loreText;
    Vec3 position{0.0f, 140.0f, 0.0f};
    int styleVariant = 0; // 0: Sun-Dial Arch, 1: Crystal Spire Ring, 2: Titan Anchor, 3: Astrolabe Sanctuary
    ColorRGB accentColor{95, 235, 255};
    bool discovered = false;
};

// 12 Recoverable Expedition Lore Logs from the First Sky-Armada
struct LoreLogEntry {
    int id = 0;
    std::string title;
    std::string author;
    std::string excerpt;
    Vec3 position{0.0f, 136.0f, 0.0f};
    bool recovered = false;
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
    void startDailySkyVoyage();
    void cycleRiskContract();
    static const char* getRiskContractName(int idx);
    static const char* getRiskContractDesc(int idx);
    float getRiskBountyMultiplier() const;
    void togglePlayerSearchlight();
    void emergencyRefloatPlayerShip();
    int discoveredWonderCount() const;
    int recoveredLoreLogCount() const;

    void spawnLeviathanEncounter(LeviathanSpecies species, const Vec3& pos);
    void spawnPirateSquadron(int count, const Vec3& centerPos);
    void spawnAlliedWingman(int blueprintIdx, const Vec3& pos);
    bool hireAlliedWingman();
    void summonApexLeviathan();
    void spawnStarMeteor(const Vec3& pos, const Vec3& vel);
    void spawnSurvivorPod(const Vec3& pos);
    bool fastTravelToIsland(int islandIdx);
    void spawnImperialConvoy();
    bool overclockPlayerShip();
    void spawnCorsairDreadArk();
    bool refitPlayerAirframe();
    bool autoStabilizePlayerShip();

    // Officer Crew Roster & Upgrades (0..4)
    static const std::array<OfficerSpec, 5>& getOfficerSpecs();
    bool upgradeOfficer(int officerIdx);
    void applyCrewMultipliers();

    // Ancient Aether-Relic Vault (0..4)
    static const std::array<RelicSpec, kRelicCount>& getRelicSpecs();
    bool unlockRelic(int relicIdx);
    int unlockedRelicCount() const;

    // Sky-Admiral Commendation Medals (0..9)
    static const std::array<MedalSpec, kMedalCount>& getMedalSpecs();
    int earnedMedalCount() const;
    void checkCommendationMedals();

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
    bool deploySkyMine();
    bool triggerEmpNova();

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
    std::vector<SkyMine3D> skyMines;
    std::vector<AetherMeteor3D> meteors;
    std::vector<SkySurvivorPod> survivorPods;
    std::vector<SkyRayBoid> skyRayBoids;
    std::vector<SkyJellyfish> skyJellies;
    std::vector<SkyWonder> skyWonders;
    std::vector<LoreLogEntry> loreLogs;
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
    int slipstreamRingsPassedStat = 0;
    int slipstreamComboCount = 0;
    float slipstreamComboTimer = 0.0f;
    int ramImpactsStat = 0;
    int skyMineHitsStat = 0;
    int meteorsHarvestedStat = 0;
    int convoysEscortedStat = 0;
    int convoyShipId = -1;
    int convoyTargetIslandIdx = 1;
    float convoyRespawnTimer = 0.0f;
    int hullOverclockTier = 0;
    int dreadnoughtsSunkStat = 0;
    int dreadArkShipId = -1;
    int survivorsRescuedStat = 0;
    int empNovasTriggeredStat = 0;
    int projectilesInterceptedStat = 0;
    int obelisksAttunedStat = 0;
    int prestigeHuntTier = 1;
    std::array<int, 5> officerRanks{{0, 0, 0, 0, 0}}; // 0..3 per officer
    std::array<int, kCommodityCount> playerCargo{{0, 0, 0, 0}};
    std::array<bool, kRelicCount> unlockedRelics{{false, false, false, false, false}};
    std::array<bool, kMedalCount> earnedMedals{{false, false, false, false, false, false, false, false, false, false}};
    FleetOrder currentFleetOrder = FleetOrder::EscortFormation;
    WeatherState currentWeather = WeatherState::ClearTradeWinds;

    // Daily Sky-Voyage, Risk/Reward Contracts & Searchlight state
    bool playerSearchlightOn = true;
    bool isDailyVoyage = false;
    std::string dailyDateLabel = "2026-10-30";
    int activeRiskContractIdx = 0; // 0..3
    float voyageElapsedSeconds = 0.0f;
    bool victoryAchieved = false;

    // Tactical Ability Cooldowns & States
    float damageControlCooldown = 0.0f;
    float sonarPingCooldown = 0.0f;
    float skyMineCooldown = 0.0f;
    float empNovaCooldown = 0.0f;
    float pointDefenseCooldown = 0.0f;
    float ramCooldownTimer = 0.0f;
    float meteorSpawnTimer = 28.0f;
    float sonarMarkTimer = 0.0f;
    float sonarWaveRadius = 0.0f;
    Vec3 sonarWaveOrigin{0.0f, 0.0f, 0.0f};
    float empWaveRadius = 0.0f;
    Vec3 empWaveOrigin{0.0f, 0.0f, 0.0f};
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
    void initSkyEcosystemAndWonders();
    void stepSkyEcosystemAndWonders(float dt);
    void stepProjectiles(float dt);
    void stepPointDefense(float dt);
    void stepSkyMines(float dt);
    void stepMeteors(float dt);
    void stepSurvivorPods(float dt);
    void stepKineticCollisions(float dt);
    void stepHarpoonTethers(float dt);
    void stepDebrisAndCrates(float dt);

    int nextEntityId_ = 100;
    Rng32 rng_;
    SpatialHash3D spatialHash_;
};

} // namespace aethernaut
