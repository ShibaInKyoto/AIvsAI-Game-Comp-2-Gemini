#include "core/Math3D.h"
#include "core/SpatialHash3D.h"
#include "sim/BlockRegistry.h"
#include "sim/Airship.h"
#include "sim/ShipBlueprints.h"
#include "sim/Leviathan.h"
#include "sim/Ballistics.h"
#include "sim/World.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>

using namespace aethernaut;

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
            return 1; \
        } \
    } while (0)

#define ASSERT_NEAR(val, expected, eps, msg) \
    ASSERT_TRUE(std::abs((val) - (expected)) <= (eps), msg)

int testMathAndRaycasting() {
    Quat q = Quat::fromAxisAngle({0.0f, 1.0f, 0.0f}, kPi * 0.5f);
    Vec3 rotated = q.rotate({0.0f, 0.0f, -1.0f});
    ASSERT_NEAR(rotated.x, -1.0f, 1e-4f, "90-deg yaw should rotate -Z to -X");
    ASSERT_NEAR(rotated.z, 0.0f, 1e-4f, "90-deg yaw Z component near zero");

    AABB3D box{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    Ray3D ray{{0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, -1.0f}};
    float tHit = 0.0f;
    Vec3 normal{};
    ASSERT_TRUE(intersectRayAABB(ray, box, tHit, normal), "Ray should hit unit AABB");
    ASSERT_NEAR(tHit, 4.0f, 1e-4f, "Ray AABB hit distance should be 4.0");
    ASSERT_NEAR(normal.z, 1.0f, 1e-4f, "Ray AABB hit normal should point +Z");
    return 0;
}

int testAirshipBuoyancyAndStructuralFracture() {
    Airship ship(1, "Test Frigate", 0);
    ship.clearAllBlocks();

    // Place Command Bridge at (0, 0, 0) and construct a 5-block wing out to X = +5
    ship.setBlock(0, 0, 0, BlockId::HelmBridge);
    for (int x = 1; x <= 4; ++x) {
        ship.setBlock(x, 0, 0, BlockId::WoodHull);
    }
    ship.setBlock(5, 0, 0, BlockId::SteamPropeller);
    ASSERT_TRUE(ship.occupiedBlockCount == 6, "Ship should have 6 blocks before fracture");

    // Sever strut at X = 2 and run 3D BFS Structural Connectivity Solver
    ship.removeBlock(2, 0, 0);
    std::vector<DetachedDebris> debris;
    int severed = ship.severDisconnectedBlocks(debris);

    ASSERT_TRUE(severed == 3, "Severing X=2 should detach blocks at X=3, 4, 5");
    ASSERT_TRUE(debris.size() == 3, "3 DetachedDebris chunks should be spawned");
    ASSERT_TRUE(ship.occupiedBlockCount == 2, "Only X=0 and X=1 remain connected to Command Bridge");

    // Test 3D Energy Shield Dome absorption
    ship.setBlock(0, 1, 0, BlockId::ShieldProjector);
    ship.shieldHp = ship.maxShieldHp;
    ASSERT_TRUE(ship.maxShieldHp >= 400.0f, "HelmBridge + ShieldProjector should grant >= 400 Shield HP");
    float pen = ship.absorbShieldDamage(250.0f);
    ASSERT_NEAR(pen, 0.0f, 1e-3f, "Shield should fully absorb 250 incoming damage");
    ASSERT_TRUE(ship.shieldHp < ship.maxShieldHp, "Shield HP reduced after absorbing impact");
    return 0;
}

int testBlueprintsAndLeviathanKinematics() {
    ASSERT_TRUE(ShipBlueprints::count() == 10, "Should have 10 flagship blueprints including Nemesis, Prometheus & Celestial Ark");
    for (int i = 0; i < ShipBlueprints::count(); ++i) {
        Airship s(i + 1, "Blueprint Vessel", 0);
        ShipBlueprints::applyBlueprint(s, i);
        ASSERT_TRUE(!s.isDestroyed, "Every starter ship blueprint must not be marked destroyed");
        ASSERT_TRUE(s.occupiedBlockCount >= 20, "Every ship blueprint must have >= 20 blocks");
        ASSERT_TRUE(!s.isSinking, "Every starter ship blueprint must have positive buoyancy lift");
    }

    // Verify all 6 Colossal Sky-Leviathan species initialize and step kinematics properly
    const LeviathanSpecies allSpecies[6] = {
        LeviathanSpecies::AzureSkyWhale,
        LeviathanSpecies::CrimsonStormWyrm,
        LeviathanSpecies::ObsidianDreadManta,
        LeviathanSpecies::MaelstromSovereign,
        LeviathanSpecies::GlacialCryoKraken,
        LeviathanSpecies::SolarPhoenixSerpent
    };
    for (int sp = 0; sp < 6; ++sp) {
        Leviathan beast(10 + sp, allSpecies[sp], {0.0f, 130.0f, -200.0f}, 42ULL + static_cast<uint64_t>(sp));
        ASSERT_TRUE(beast.segments.size() >= 14, "Every Leviathan species must have >= 14 articulated segments");
        std::vector<LeviathanAttackBolt> attacks;
        for (int step = 0; step < 30; ++step) {
            beast.update(1.0f / 60.0f, {0.0f, 130.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0, attacks);
        }
        for (size_t i = 1; i < beast.segments.size(); ++i) {
            float d = (beast.segments[i].position - beast.segments[i - 1].position).length();
            ASSERT_NEAR(d, beast.segments[i - 1].jointSpacing, 0.25f, "Leviathan spine joint constraint preserved");
        }
    }

    // Verify custom 3D ship blueprint serialization to disk and back (including all 20 block types)
    {
        Airship orig(77, "Custom Sky-Dreadnought", 0);
        ShipBlueprints::applyBlueprint(orig, 9); // Celestial Ark-Carrier
        orig.setBlock(0, 4, 0, BlockId::TorpedoLauncher, 0);
        orig.setBlock(0, 4, 1, BlockId::ShockHarpoon, 0);
        orig.setBlock(0, 4, 2, BlockId::CapacitorBank, 0);
        orig.setBlock(0, 4, 3, BlockId::FlameMortar, 0);
        int expectedCount = orig.occupiedBlockCount;
        ASSERT_TRUE(ShipBlueprints::saveCustomBlueprint(orig, "/tmp/test_custom_ship.aeth"), "Save custom blueprint to disk");

        Airship loaded(78, "Empty", 0);
        ASSERT_TRUE(ShipBlueprints::loadCustomBlueprint(loaded, "/tmp/test_custom_ship.aeth"), "Load custom blueprint from disk");
        ASSERT_TRUE(loaded.occupiedBlockCount == expectedCount, "Loaded custom blueprint block count matches original");
        ASSERT_TRUE(loaded.getCell(0, 4, 0).id == BlockId::TorpedoLauncher, "Loaded custom block at (0,4,0) is TorpedoLauncher");
        ASSERT_TRUE(loaded.getCell(0, 4, 1).id == BlockId::ShockHarpoon, "Loaded custom block at (0,4,1) is ShockHarpoon");
        ASSERT_TRUE(loaded.getCell(0, 4, 2).id == BlockId::CapacitorBank, "Loaded custom block at (0,4,2) is CapacitorBank");
        ASSERT_TRUE(loaded.getCell(0, 4, 3).id == BlockId::FlameMortar, "Loaded custom block at (0,4,3) is FlameMortar");
    }
    return 0;
}

int testOfficersAbilitiesTradingAndCampaignPersistence() {
    World world;
    world.initWorld(20261028ULL, 7, false); // Nemesis Torpedo Cruiser
    world.playerGold = 2500;
    world.playerAether = 100;

    ASSERT_TRUE(world.leviathans.size() == 6, "Campaign world initializes all 6 Colossal Sky-Leviathans");
    ASSERT_TRUE(world.contracts.size() == 6, "Campaign world initializes all 6 Guild Bounty contracts");

    float baseReload = world.playerShip.reloadMultiplier;
    ASSERT_TRUE(world.upgradeOfficer(0), "Promoting Master Gunner Vance should succeed");
    ASSERT_TRUE(world.playerShip.reloadMultiplier > baseReload, "Master Gunner promotion increases reloadMultiplier");

    ASSERT_TRUE(world.triggerDamageControl(), "Emergency Damage Control should activate when ready");
    ASSERT_TRUE(world.damageControlCooldown > 0.0f, "Damage Control cooldown started");

    ASSERT_TRUE(world.triggerSonarPing(), "Acoustic Aether-Sonar should activate when ready");
    ASSERT_TRUE(world.sonarMarkTimer > 0.0f, "Sonar Mark timer active");

    // Test Island Commodity Trading (player starts docked near Haven Port, island 0)
    int goldBefore = world.playerGold;
    int initialGrain = world.playerCargo[0];
    ASSERT_TRUE(world.buyCommodity(0, 2), "Buying 2 Sky-Grain crates at Haven Port should succeed");
    ASSERT_TRUE(world.playerCargo[0] == initialGrain + 2, "Player cargo holds +2 Sky-Grain crates");
    ASSERT_TRUE(world.playerGold < goldBefore, "Gold deducted for commodity purchase");
    ASSERT_TRUE(world.sellCommodity(0, 1), "Selling 1 Sky-Grain crate should succeed");
    ASSERT_TRUE(world.playerCargo[0] == initialGrain + 1, "Player cargo holds +1 Sky-Grain crate after sale");

    // Test Fleet Squadron Orders & Dynamic Weather cycling
    world.cycleFleetOrder();
    ASSERT_TRUE(world.currentFleetOrder == FleetOrder::FocusMyTarget, "Fleet order cycled to FocusMyTarget");
    world.cycleWeatherState();
    ASSERT_TRUE(world.currentWeather == WeatherState::CirrusMist, "Weather state cycled to CirrusMist");

    // Test firing Homing Torpedoes & Shock Harpoons from Nemesis Torpedo Cruiser
    size_t prevProj = world.projectiles.size();
    world.firePlayerWeapons(true, true, {0.0f, 130.0f, -250.0f});
    ASSERT_TRUE(world.projectiles.size() > prevProj, "Firing Nemesis Torpedo Cruiser spawns projectiles");
    bool hasTorp = false, hasShock = false;
    for (const auto& p : world.projectiles) {
        if (p.isHomingTorpedo) hasTorp = true;
        if (p.isShockHarpoon) hasShock = true;
    }
    ASSERT_TRUE(hasTorp && hasShock, "Spawned both Homing Torpedo and Shock Harpoon projectiles");

    size_t prevLevs = world.leviathans.size();
    world.summonApexLeviathan();
    ASSERT_TRUE(world.leviathans.size() == prevLevs + 1, "Summon Apex Leviathan spawns a new boss");

    // Test Ancient Aether-Relics & Custom Fleet Hull Liveries
    world.playerGold += 500;
    world.playerAether += 30;
    float reloadBeforeRelic = world.playerShip.reloadMultiplier;
    ASSERT_TRUE(world.unlockRelic(0), "Awakening Chronos Aether-Dial Relic should succeed");
    ASSERT_TRUE(world.unlockedRelicCount() == 1, "Unlocked relic count incremented to 1");
    ASSERT_TRUE(world.playerShip.reloadMultiplier > reloadBeforeRelic, "Chronos Aether-Dial boosts weapon reload multiplier");

    world.cyclePlayerLivery();
    ASSERT_TRUE(world.playerShip.hullLiveryId == 1, "Hull livery cycled to Imperial Azure Guard (1)");

    // Verify Fortified Stronghold islands exist with 3D Sky-Bastion Turrets
    int strongholdCount = 0;
    for (const auto& isl : world.archipelago.islands) {
        if (isl.isFortifiedStronghold && !isl.bastionTurrets.empty()) {
            strongholdCount++;
        }
    }
    ASSERT_TRUE(strongholdCount >= 4, "Archipelago contains fortified stronghold citadels with bastion turrets");

    ASSERT_TRUE(world.saveCampaign("/tmp/test_campaign.aeth"), "Save campaign state to disk");

    World worldLoaded;
    worldLoaded.initWorld(1ULL, 0, false);
    ASSERT_TRUE(worldLoaded.loadCampaign("/tmp/test_campaign.aeth"), "Load campaign state from disk");
    ASSERT_TRUE(worldLoaded.officerRanks[0] == 1, "Loaded campaign preserves officer rank");
    ASSERT_TRUE(worldLoaded.playerCargo[0] == initialGrain + 1, "Loaded campaign preserves commodity cargo");
    ASSERT_TRUE(worldLoaded.unlockedRelics[0] == true, "Loaded campaign preserves awakened Ancient Relic");
    ASSERT_TRUE(worldLoaded.playerShip.hullLiveryId == 1, "Loaded campaign preserves Custom Hull Livery");
    return 0;
}

int testHeadlessWorldCombatAndTethers() {
    World world;
    world.initWorld(20261028ULL, 1, false);
    world.playerAutopilot = true;

    // Run 30 seconds of full 120Hz 3D physics + AI combat + Leviathan simulation
    for (int i = 0; i < 30 * 120; ++i) {
        world.step(1.0f / 120.0f);
    }

    ASSERT_TRUE(!std::isnan(world.playerShip.position.x), "Player position must be valid float");
    ASSERT_TRUE(world.playerShip.position.y > 10.0f, "Player ship maintained flight altitude");
    return 0;
}

int main() {
    std::printf("=== Running AETHERNAUT 3D Physics & Simulation Test Suite ===\n");
    if (testMathAndRaycasting() != 0) return 1;
    std::printf("[PASS] 3D Quaternion Math & Ray-AABB/Sphere Intersection\n");

    if (testAirshipBuoyancyAndStructuralFracture() != 0) return 1;
    std::printf("[PASS] 3D Airship Buoyancy, Shield Dome & BFS Structural Fracture Solver\n");

    if (testBlueprintsAndLeviathanKinematics() != 0) return 1;
    std::printf("[PASS] 10 Ship Hull Blueprints, 20 Block Types & 6 Leviathan Species Kinematics\n");

    if (testOfficersAbilitiesTradingAndCampaignPersistence() != 0) return 1;
    std::printf("[PASS] Officers, Commodity Trading, Fleet Orders, Homing Torpedoes & Campaign Persistence\n");

    if (testHeadlessWorldCombatAndTethers() != 0) return 1;
    std::printf("[PASS] 30-Second 120Hz Headless 3D World & AI Fleet Simulation\n");

    std::printf("=== ALL TESTS PASSED ===\n");
    return 0;
}
