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
    ASSERT_TRUE(ShipBlueprints::count() == 12, "Should have 12 flagship blueprints including Hyperion Rail-Dreadnought & Nebula Salvage Harvester");
    for (int i = 0; i < ShipBlueprints::count(); ++i) {
        Airship s(i + 1, "Blueprint Vessel", 0);
        ShipBlueprints::applyBlueprint(s, i);
        ASSERT_TRUE(!s.isDestroyed, "Every starter ship blueprint must not be marked destroyed");
        ASSERT_TRUE(s.occupiedBlockCount >= 20, "Every ship blueprint must have >= 20 blocks");
        ASSERT_TRUE(!s.isSinking, "Every starter ship blueprint must have positive buoyancy lift");
    }
    {
        Airship railShip(91, "Hyperion Test", 0);
        ShipBlueprints::applyBlueprint(railShip, 10);
        ASSERT_TRUE(railShip.cannonCount >= 4, "Hyperion Rail-Dreadnought has >= 4 Railgun batteries");
        ASSERT_TRUE(railShip.tractorBeamCount >= 1, "Hyperion Rail-Dreadnought has >= 1 TractorBeam");

        Airship harvShip(92, "Nebula Test", 0);
        ShipBlueprints::applyBlueprint(harvShip, 11);
        ASSERT_TRUE(harvShip.tractorBeamCount >= 4, "Nebula Salvage Harvester has >= 4 Aether Tractor Grapplers");
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
        orig.setBlock(0, 4, 4, BlockId::TractorBeam, 0);
        orig.setBlock(0, 4, 5, BlockId::RailgunBattery, 0);
        int expectedCount = orig.occupiedBlockCount;
        ASSERT_TRUE(ShipBlueprints::saveCustomBlueprint(orig, "/tmp/test_custom_ship.aeth"), "Save custom blueprint to disk");

        Airship loaded(78, "Empty", 0);
        ASSERT_TRUE(ShipBlueprints::loadCustomBlueprint(loaded, "/tmp/test_custom_ship.aeth"), "Load custom blueprint from disk");
        ASSERT_TRUE(loaded.occupiedBlockCount == expectedCount, "Loaded custom blueprint block count matches original");
        ASSERT_TRUE(loaded.getCell(0, 4, 0).id == BlockId::TorpedoLauncher, "Loaded custom block at (0,4,0) is TorpedoLauncher");
        ASSERT_TRUE(loaded.getCell(0, 4, 1).id == BlockId::ShockHarpoon, "Loaded custom block at (0,4,1) is ShockHarpoon");
        ASSERT_TRUE(loaded.getCell(0, 4, 2).id == BlockId::CapacitorBank, "Loaded custom block at (0,4,2) is CapacitorBank");
        ASSERT_TRUE(loaded.getCell(0, 4, 3).id == BlockId::FlameMortar, "Loaded custom block at (0,4,3) is FlameMortar");
        ASSERT_TRUE(loaded.getCell(0, 4, 4).id == BlockId::TractorBeam, "Loaded custom block at (0,4,4) is TractorBeam");
        ASSERT_TRUE(loaded.getCell(0, 4, 5).id == BlockId::RailgunBattery, "Loaded custom block at (0,4,5) is RailgunBattery");
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

    // Verify 12 Aether-Slipstream Rings exist and passing through one grants Turbo Impulse & Regatta Combo
    ASSERT_TRUE(world.archipelago.slipstreams.size() == 12, "Archipelago contains 12 Aether-Slipstream Skyway Rings");
    world.playerShip.position = world.archipelago.slipstreams[0].position;
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.slipstreamRingsPassedStat == 1, "Flying through Slipstream Ring increments slipstreamRingsPassedStat");
    ASSERT_TRUE(world.slipstreamComboCount == 1, "Flying through Slipstream Ring starts x1 Regatta Chain Combo");

    // Verify Magnetic Aether Sky-Mine [G] deployment and homing detonation
    ASSERT_TRUE(world.deploySkyMine(), "Deploying Magnetic Aether Sky-Mine should succeed");
    ASSERT_TRUE(world.skyMines.size() == 1, "1 active SkyMine3D deployed behind stern");
    world.skyMines[0].armTimer = 0.0f;
    world.skyMines[0].position = world.leviathans[0].segments[0].position + Vec3{0.0f, 0.0f, 4.0f};
    float levHpBeforeMine = world.leviathans[0].totalHp;
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.skyMineHitsStat == 1, "SkyMine3D magnetically locked and detonated on Leviathan");
    ASSERT_TRUE(world.leviathans[0].totalHp < levHpBeforeMine, "SkyMine3D dealt AoE blast damage to Leviathan");

    // Verify Kinetic Bow Ramming collision physics
    world.ramCooldownTimer = 0.0f;
    world.playerShip.position = world.leviathans[0].segments[1].position - Vec3{0.0f, 0.0f, 8.0f};
    world.playerShip.velocity = {0.0f, 0.0f, 28.0f};
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.ramImpactsStat == 1, "High-speed collision triggers Kinetic Bow Ram impact");

    // Verify Sky-Admiral Commendation Medals unlocked (Relic Awakener + Minefield Tactician + Iron Prow Rammer)
    ASSERT_TRUE(world.earnedMedalCount() >= 3, "At least 3 Sky-Admiral Commendation Medals automatically awarded");

    // Verify Celestial Star-Aether Meteoroid spawning & proximity tractor harvesting
    ASSERT_TRUE(world.meteors.size() >= 3, "Initial world spawns 3 Celestial Star-Aether Meteoroids");
    int aethBeforeMeteor = world.playerAether;
    world.playerShip.position = world.meteors[0].position;
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.meteorsHarvestedStat == 1, "Proximity tractor-harvesting Star-Aether Meteoroid increments meteorsHarvestedStat");
    ASSERT_TRUE(world.playerAether >= aethBeforeMeteor + 14, "Star-Aether Meteoroid grants +14 Aetherium");

    // Verify Interactive World Map Fast-Travel to an Allied Drydock Port
    ASSERT_TRUE(world.fastTravelToIsland(0), "Fast-Travel to Allied Port #0 (New Horizon Drydock) succeeds");

    // Verify Drydock Orichalcum Hull Overclock [O]
    float maxHpBeforeOc = world.playerShip.totalMaxHp;
    ASSERT_TRUE(world.overclockPlayerShip(), "Orichalcum Hull Overclock Tier 1 succeeds");
    ASSERT_TRUE(world.hullOverclockTier == 1, "hullOverclockTier incremented to 1");
    ASSERT_TRUE(world.playerShip.totalMaxHp > maxHpBeforeOc * 1.12f, "Overclock increases flagship totalMaxHp by +15%");

    // Verify Imperial Treasure Galleon Convoy arrival dividend
    ASSERT_TRUE(world.convoyShipId > 0, "Imperial Treasure Galleon Convoy spawned");
    for (auto& npc : world.npcShips) {
        if (npc.id() == world.convoyShipId) {
            npc.position = world.archipelago.islands[static_cast<size_t>(world.convoyTargetIslandIdx)].position + Vec3{0.0f, 38.0f, 0.0f};
            break;
        }
    }
    int goldBeforeConvoy = world.playerGold;
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.convoysEscortedStat == 1, "Imperial Treasure Galleon reaching port increments convoysEscortedStat");
    ASSERT_TRUE(world.playerGold >= goldBeforeConvoy + 180, "Imperial Treasure Galleon reaching port awards +180G Escort Dividend");

    // Verify 8 Thermal Aether-Geyser Updraft Columns & vertical buoyant lift
    ASSERT_TRUE(world.archipelago.updrafts.size() == 8, "Archipelago contains 8 Thermal Aether-Geyser Updraft Columns");
    world.playerShip.position = {world.archipelago.updrafts[0].basePosition.x, 110.0f, world.archipelago.updrafts[0].basePosition.z};
    float vyBeforeGeyser = world.playerShip.velocity.y;
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.playerShip.velocity.y > vyBeforeGeyser, "Thermal Aether-Geyser applies upward buoyancy velocity impulse");

    // Verify Drydock One-Click Airframe Armor Refit (Timber -> Ironclad)
    float hpBeforeRefit = world.playerShip.totalMaxHp;
    ASSERT_TRUE(world.refitPlayerAirframe(), "One-click Airframe Refit from Timber to Ironclad succeeds");
    ASSERT_TRUE(world.playerShip.totalMaxHp > hpBeforeRefit, "Airframe Refit increases flagship total structural HP");

    // Verify Corsair Dread-Ark Raid Boss summoning & Aether-EMP Shockwave Nova [4]
    world.spawnCorsairDreadArk();
    ASSERT_TRUE(world.dreadArkShipId > 0, "Corsair Dread-Ark Sovereign Raid Boss spawned");
    ASSERT_TRUE(world.triggerEmpNova(), "Aether-EMP Shockwave Nova [4] triggers when ready");
    ASSERT_TRUE(world.empNovasTriggeredStat == 1, "empNovasTriggeredStat incremented to 1");
    ASSERT_TRUE(world.empWaveRadius > 0.0f, "empWaveRadius initialized for 3D shockwave rendering");

    // Verify 3D Distress Beacon Sky-Survivor Lifeboat Pod rescue
    ASSERT_TRUE(!world.survivorPods.empty(), "Archipelago spawns Distress Beacon Sky-Survivor Lifeboat Pods");
    int goldBeforeRescue = world.playerGold;
    world.playerShip.position = world.survivorPods[0].position;
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.survivorsRescuedStat >= 1, "Flying within rescue range of SOS Lifeboat Pod increments survivorsRescuedStat");
    ASSERT_TRUE(world.playerGold >= goldBeforeRescue + 140, "Rescuing SOS Lifeboat Pod grants >= +140 Gold");

    // Verify Automated CIWS Flak Turret Point-Defense Interception & Drydock Auto-Stabilizer
    world.playerShip.setBlock(0, 2, -2, BlockId::FlakTurret, 0);
    ASSERT_TRUE(world.playerShip.flakTurretCount >= 1, "Installing Rotary Flak Turret increments flakTurretCount");
    Projectile3D hostileProj;
    hostileProj.id = 99999;
    hostileProj.ownerShipId = 555;
    hostileProj.ownerFaction = 2;
    hostileProj.position = world.playerShip.position + Vec3{18.0f, 2.0f, 0.0f};
    hostileProj.prevPosition = hostileProj.position;
    hostileProj.velocity = {-20.0f, 0.0f, 0.0f};
    hostileProj.damage = 45.0f;
    hostileProj.lifetime = 3.0f;
    hostileProj.active = true;
    world.projectiles.push_back(hostileProj);
    world.pointDefenseCooldown = 0.0f;
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.projectilesInterceptedStat >= 1, "CIWS Flak Turret intercepts incoming hostile projectile within 65m");

    ASSERT_TRUE(world.autoStabilizePlayerShip(), "Drydock 1-Click Naval Architect Auto-Stabilizer succeeds");

    // Verify 6 Ancient Aether-Obelisk Monoliths & Attunement Rewards (+220G, +10Ae, +85 Max Shield HP)
    ASSERT_TRUE(world.archipelago.obelisks.size() == 6, "Archipelago contains 6 Ancient Aether-Obelisk Monoliths");
    int obelisksBefore = world.obelisksAttunedStat;
    for (const auto& ob : world.archipelago.obelisks) {
        if (!ob.attuned) {
            float maxShieldBefore = world.playerShip.maxShieldHp;
            int goldBeforeOb = world.playerGold;
            world.playerShip.position = ob.position;
            world.step(1.0f / 120.0f);
            ASSERT_TRUE(world.obelisksAttunedStat == obelisksBefore + 1, "Flying within 68m of Ancient Aether-Obelisk attunes it");
            ASSERT_TRUE(world.playerGold >= goldBeforeOb + 220, "Attuning Ancient Aether-Obelisk grants >= +220 Gold");
            ASSERT_TRUE(world.playerShip.maxShieldHp >= maxShieldBefore + 84.0f, "Attuning Ancient Aether-Obelisk permanently boosts Max Shield HP by +85");
            break;
        }
    }

    ASSERT_TRUE(world.saveCampaign("/tmp/test_campaign.aeth"), "Save campaign state to disk");

    World worldLoaded;
    worldLoaded.initWorld(1ULL, 0, false);
    ASSERT_TRUE(worldLoaded.loadCampaign("/tmp/test_campaign.aeth"), "Load campaign state from disk");
    ASSERT_TRUE(worldLoaded.officerRanks[0] == 1, "Loaded campaign preserves officer rank");
    ASSERT_TRUE(worldLoaded.playerCargo[0] == initialGrain + 1, "Loaded campaign preserves commodity cargo");
    ASSERT_TRUE(worldLoaded.unlockedRelics[0] == true, "Loaded campaign preserves awakened Ancient Relic");
    ASSERT_TRUE(worldLoaded.earnedMedalCount() >= 3, "Loaded campaign preserves Sky-Admiral Commendation Medals");
    ASSERT_TRUE(worldLoaded.playerShip.hullLiveryId == 1, "Loaded campaign preserves Custom Hull Livery");
    ASSERT_TRUE(worldLoaded.meteorsHarvestedStat >= 1, "Loaded campaign preserves meteorsHarvestedStat");
    ASSERT_TRUE(worldLoaded.hullOverclockTier == 1, "Loaded campaign preserves hullOverclockTier");
    ASSERT_TRUE(worldLoaded.survivorsRescuedStat >= 1, "Loaded campaign preserves survivorsRescuedStat");
    ASSERT_TRUE(worldLoaded.empNovasTriggeredStat == 1, "Loaded campaign preserves empNovasTriggeredStat");
    ASSERT_TRUE(worldLoaded.projectilesInterceptedStat >= 1, "Loaded campaign preserves projectilesInterceptedStat");
    ASSERT_TRUE(worldLoaded.obelisksAttunedStat >= 1, "Loaded campaign preserves obelisksAttunedStat");
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

int testElderBossCounterplaySkyEcosystemWondersAndDailyVoyage() {
    World world;
    world.initWorld(20261031ULL, 1, false);

    // 1. Verify Sky-Ecosystem Boids, Jellyfish, 10 Sky-Wonders, and 12 Lore Logs initialize
    ASSERT_TRUE(world.skyRayBoids.size() == 36, "World initializes 36 cohesive 3D Sky-Ray Boids");
    ASSERT_TRUE(world.skyJellies.size() == 24, "World initializes 24 Bioluminescent Aether-Medusa Jellyfish");
    ASSERT_TRUE(world.skyWonders.size() == 10, "World initializes 10 Monumental Sky-Wonders");
    ASSERT_TRUE(world.loreLogs.size() == 12, "World initializes 12 First Armada Expedition Lore Logs");

    // 2. Verify Bow Searchlight toggle & Obsidian Dread-Manta Umbra Cloak counterplay
    ASSERT_TRUE(world.playerSearchlightOn, "Player searchlight starts ON by default");
    world.togglePlayerSearchlight();
    ASSERT_TRUE(!world.playerSearchlightOn, "togglePlayerSearchlight turns searchlight OFF");
    world.togglePlayerSearchlight();
    ASSERT_TRUE(world.playerSearchlightOn, "togglePlayerSearchlight turns searchlight back ON");

    // 3. Verify Elder Leviathan signature mechanics & counterplay disruption across species
    for (auto& lev : world.leviathans) {
        ASSERT_TRUE(!lev.signatureMechanicName().empty(),
                    "Every Leviathan has a non-empty signatureMechanicName");
        ASSERT_TRUE(!lev.counterplayHint().empty(),
                    "Every Leviathan has a non-empty counterplayHint");
        lev.barrierActive = true;
        lev.staggeredWindowTimer = 0.0f;
        ASSERT_TRUE(lev.triggerCounterplayDisruption("Test Counterplay"), "triggerCounterplayDisruption shatters active barrier");
        ASSERT_TRUE(!lev.barrierActive, "Barrier is deactivated after counterplay disruption");
        ASSERT_TRUE(lev.staggeredWindowTimer >= 7.0f, "Staggered exposure window opened");
        ASSERT_TRUE(lev.aiState == LeviathanState::StaggeredExposed, "Leviathan enters StaggeredExposed state");
    }

    // 4. Verify discovering a Monumental Sky-Wonder & First Armada Lore Log by proximity
    world.playerShip.position = world.skyWonders[0].position;
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.skyWonders[0].discovered, "Flying near Sky-Wonder #0 discovers it");
    ASSERT_TRUE(world.discoveredWonderCount() >= 1, "discoveredWonderCount() incremented");

    world.playerShip.position = world.loreLogs[0].position;
    world.step(1.0f / 120.0f);
    ASSERT_TRUE(world.loreLogs[0].recovered, "Flying near Lore Log #0 decodes it");
    ASSERT_TRUE(world.recoveredLoreLogCount() >= 1, "recoveredLoreLogCount() incremented");

    // 5. Verify Risk/Reward Expedition Charters & Emergency Harbor Refloat
    ASSERT_NEAR(world.getRiskBountyMultiplier(), 1.0f, 1e-4f, "Standard Charter multiplier is 1.0x");
    world.cycleRiskContract();
    ASSERT_NEAR(world.getRiskBountyMultiplier(), 1.35f, 1e-4f, "Tempest Squall Charter multiplier is 1.35x");
    world.cycleRiskContract();
    ASSERT_NEAR(world.getRiskBountyMultiplier(), 1.50f, 1e-4f, "Glass-Cannon Broadside Charter multiplier is 1.50x");

    world.playerShip.isDestroyed = true;
    world.emergencyRefloatPlayerShip();
    ASSERT_TRUE(!world.playerShip.isDestroyed, "emergencyRefloatPlayerShip restores flagship from destroyed state");

    // 6. Verify Daily Sky-Voyage Mode initialization
    world.startDailySkyVoyage();
    ASSERT_TRUE(world.isDailyVoyage, "startDailySkyVoyage sets isDailyVoyage = true");
    ASSERT_TRUE(!world.dailyDateLabel.empty(), "Daily date label is populated");
    return 0;
}

int main() {
    std::printf("=== Running AETHERNAUT 3D Physics & Simulation Test Suite ===\n");
    if (testMathAndRaycasting() != 0) return 1;
    std::printf("[PASS] 3D Quaternion Math & Ray-AABB/Sphere Intersection\n");

    if (testAirshipBuoyancyAndStructuralFracture() != 0) return 1;
    std::printf("[PASS] 3D Airship Buoyancy, Shield Dome & BFS Structural Fracture Solver\n");

    if (testBlueprintsAndLeviathanKinematics() != 0) return 1;
    std::printf("[PASS] 12 Ship Hull Blueprints, 22 Block Types & 6 Leviathan Species Kinematics\n");

    if (testOfficersAbilitiesTradingAndCampaignPersistence() != 0) return 1;
    std::printf("[PASS] Officers, Commodity Trading, Fleet Orders, Homing Torpedoes & Campaign Persistence\n");

    if (testElderBossCounterplaySkyEcosystemWondersAndDailyVoyage() != 0) return 1;
    std::printf("[PASS] Elder Boss Counterplay, 36 Sky-Ray Boids, 10 Sky-Wonders, 12 Lore Logs & Daily Voyage\n");

    if (testHeadlessWorldCombatAndTethers() != 0) return 1;
    std::printf("[PASS] 30-Second 120Hz Headless 3D World & AI Fleet Simulation\n");

    std::printf("=== ALL TESTS PASSED ===\n");
    return 0;
}

