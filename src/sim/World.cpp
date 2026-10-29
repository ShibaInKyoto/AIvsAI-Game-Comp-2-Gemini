#include "sim/World.h"
#include <cmath>
#include <algorithm>

namespace aethernaut {

World::World() : playerShip(1, "HMS Sky-Breaker", 0), spatialHash_(50.0f) {}

void World::initWorld(uint64_t seed, int playerBlueprintIdx, bool sandboxMode) {
    rng_.seedState(seed, 0xCAFEBABE12345678ULL);
    isSandboxMode = sandboxMode;
    nextEntityId_ = 100;

    npcShips.clear();
    leviathans.clear();
    projectiles.clear();
    tethers.clear();
    debris.clear();
    crates.clear();
    explosions.clear();
    callouts.clear();
    pendingAudioEvents.clear();
    contracts.clear();
    cameraTrauma = 0.0f;

    archipelago.generate(seed);

    // Initialize Player Flagship moored off New Horizon Drydock's northern sky-pier
    ShipBlueprints::applyBlueprint(playerShip, playerBlueprintIdx);
    playerShip.position = {0.0f, 134.0f, -128.0f};
    playerShip.velocity = {0.0f, 0.0f, -8.0f};
    playerShip.yaw = 0.0f;
    playerShip.pitch = 0.0f;
    playerShip.roll = 0.0f;
    playerShip.targetCruiseAltitude = 134.0f;
    playerShip.isDestroyed = false;
    playerShip.isSinking = false;

    playerGold = sandboxMode ? 99999 : 480;
    playerAether = sandboxMode ? 999 : 25;
    leviathansSlain = 0;
    piratesSunk = 0;
    isPlayerWalkingOnDeck = false;

    // Spawn Initial Sky-Leviathans across the 3D Sky Archipelago
    spawnLeviathanEncounter(LeviathanSpecies::AzureSkyWhale, {165.0f, 142.0f, -310.0f});
    spawnLeviathanEncounter(LeviathanSpecies::CrimsonStormWyrm, {-280.0f, 150.0f, -380.0f});
    spawnLeviathanEncounter(LeviathanSpecies::ObsidianDreadManta, {480.0f, 125.0f, 280.0f});
    spawnLeviathanEncounter(LeviathanSpecies::MaelstromSovereign, {0.0f, 155.0f, -1080.0f});

    // Spawn Pirate Corsair Patrols & Allied Wingman
    spawnPirateSquadron(2, {-210.0f, 132.0f, -260.0f});
    spawnPirateSquadron(2, {340.0f, 130.0f, 160.0f});
    spawnAlliedWingman(0, {-38.0f, 136.0f, -142.0f});

    // Populate Bounty Contracts
    contracts.push_back({1, "Hunt the Elder Sky-Whale", "Cetus Aetheria", {165.0f, 142.0f, -310.0f}, 450, 16, false});
    contracts.push_back({2, "Slay the Crimson Storm-Wyrm", "Draconis Fulmen", {-280.0f, 150.0f, -380.0f}, 700, 26, false});
    contracts.push_back({3, "Purge the Obsidian Dread-Manta", "Raja Vorago", {480.0f, 125.0f, 280.0f}, 950, 35, false});
    contracts.push_back({4, "Conquer the Maelstrom Sovereign", "Apex Charybdis", {0.0f, 155.0f, -1080.0f}, 1800, 65, false});
}

void World::spawnLeviathanEncounter(LeviathanSpecies species, const Vec3& pos) {
    int id = nextEntityId_++;
    leviathans.emplace_back(id, species, pos, rng_.nextU32());
}

void World::spawnPirateSquadron(int count, const Vec3& centerPos) {
    for (int i = 0; i < count; ++i) {
        int id = nextEntityId_++;
        Airship pirate(id, "Corsair Raider #" + std::to_string(id - 99), 1);
        int bp = (i % 2 == 0) ? 5 : 0;
        ShipBlueprints::applyBlueprint(pirate, bp);
        float angle = (static_cast<float>(i) / static_cast<float>(std::max(1, count))) * kTwoPi;
        pirate.position = centerPos + Vec3{std::cos(angle) * 55.0f, static_cast<float>(i) * 8.0f, std::sin(angle) * 55.0f};
        pirate.targetCruiseAltitude = pirate.position.y;
        pirate.yaw = angle;
        npcShips.push_back(pirate);
    }
}

void World::spawnAlliedWingman(int blueprintIdx, const Vec3& pos) {
    int id = nextEntityId_++;
    Airship ally(id, "Escort Cutter #" + std::to_string(id - 99), 0);
    ShipBlueprints::applyBlueprint(ally, blueprintIdx);
    ally.position = pos;
    ally.targetCruiseAltitude = pos.y;
    npcShips.push_back(ally);
}

bool World::hireAlliedWingman() {
    const int kHireCost = 200;
    if (!isSandboxMode && playerGold < kHireCost) {
        statusBannerText = "Need 200 Gold to Hire an Allied Escort Wingman!";
        statusBannerTimer = 3.0f;
        return false;
    }
    if (!isSandboxMode) {
        playerGold -= kHireCost;
    }
    int allyCount = 0;
    for (const auto& n : npcShips) {
        if (!n.isDestroyed && n.faction() == 0) allyCount++;
    }
    int bp = (allyCount % 2 == 0) ? 3 : 2; // Alternate Catamaran Interceptor & Ironclad Frigate
    float side = (allyCount % 2 == 0) ? 1.0f : -1.0f;
    Vec3 spawnOffset = playerShip.orientationQuat().right() * (side * 38.0f) - playerShip.orientationQuat().forward() * 22.0f;
    spawnAlliedWingman(bp, playerShip.position + spawnOffset);
    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 1.0f});
    statusBannerText = "Recruited Allied Escort Warship to Your Sky-Fleet! (-200 Gold)";
    statusBannerTimer = 4.5f;
    return true;
}

void World::firePlayerWeapons(bool cannons, bool harpoons, const Vec3& aimWorldPos) {
    playerAimWorldPos = aimWorldPos;
    std::vector<MuzzleFireRequest> reqs;
    playerShip.fireWeapons(cannons, harpoons, aimWorldPos, reqs);

    for (const auto& r : reqs) {
        Projectile3D proj;
        proj.id = nextEntityId_++;
        proj.ownerShipId = playerShip.id();
        proj.ownerFaction = playerShip.faction();
        proj.weaponType = r.weaponType;
        proj.position = r.muzzleWorldPos;
        proj.prevPosition = r.muzzleWorldPos;
        proj.velocity = r.aimDirection * r.speed + r.inheritedVelocity;
        proj.launchAnchorLocal = playerShip.worldToLocalPos(r.muzzleWorldPos);
        proj.damage = r.damage;
        proj.isHarpoon = (r.weaponType == BlockId::HarpoonWinch);
        proj.gravityScale = (r.weaponType == BlockId::TeslaCoil) ? 0.0f : 1.0f;
        proj.splashRadius = (r.weaponType == BlockId::DeckCannon) ? 4.8f : 2.8f;
        proj.lifetime = (r.weaponType == BlockId::TeslaCoil) ? 1.2f : 4.5f;
        proj.maxLifetime = proj.lifetime;
        proj.color = (r.weaponType == BlockId::TeslaCoil)
            ? ColorRGB{95, 210, 255}
            : (proj.isHarpoon ? ColorRGB{235, 175, 65} : ColorRGB{255, 195, 80});
        projectiles.push_back(proj);
    }

    if (!reqs.empty()) {
        cameraTrauma = std::min(1.0f, cameraTrauma + 0.16f);
        pendingAudioEvents.push_back({
            harpoons ? SimAudioEventType::HarpoonLaunch : SimAudioEventType::CannonFire,
            playerShip.position,
            1.0f
        });
    }
}

void World::setReelHarpoons(bool reelIn) {
    for (auto& t : tethers) {
        if (t.active && t.sourceShipId == playerShip.id()) {
            t.isReelingIn = reelIn;
        }
    }
}

void World::releaseAllPlayerHarpoons() {
    for (auto& t : tethers) {
        if (t.sourceShipId == playerShip.id()) {
            t.active = false;
        }
    }
}

void World::updateCaptainDeckWalk(float dt, float moveForward, float moveRight) {
    if (!isPlayerWalkingOnDeck) {
        return;
    }
    const float walkSpeed = 7.5f;
    Vec3 forwardLocal{std::sin(playerShip.captainYaw), 0.0f, -std::cos(playerShip.captainYaw)};
    Vec3 rightLocal{std::cos(playerShip.captainYaw), 0.0f, std::sin(playerShip.captainYaw)};

    Vec3 nextPos = playerShip.captainLocalPos + (forwardLocal * moveForward + rightLocal * moveRight) * (walkSpeed * dt);
    nextPos.x = std::clamp(nextPos.x, -8.0f, 8.0f);
    nextPos.z = std::clamp(nextPos.z, -14.0f, 14.0f);
    playerShip.captainLocalPos = nextPos;

    // Captain passively repairs nearby damaged blocks while walking on deck!
    playerShip.triggerEmergencyRepair(8.0f * dt);
}

void World::step(float dt) {
    if (statusBannerTimer > 0.0f) {
        statusBannerTimer = std::max(0.0f, statusBannerTimer - dt);
    }
    cameraTrauma = std::max(0.0f, cameraTrauma - 1.8f * dt);

    archipelago.update(dt);

    // 1. Player Autopilot (if enabled)
    if (playerAutopilot && !playerShip.isDestroyed) {
        AIDecision ai = AIController::computeShipControl(
            playerShip, &playerShip, npcShips, leviathans, archipelago, true
        );
        playerShip.throttleInput = ai.throttle;
        playerShip.rudderInput = ai.rudder;
        playerShip.altitudeInput = ai.altitude;
        playerAimWorldPos = ai.aimWorldPos;
        if (ai.fireCannons || ai.fireHarpoons) {
            firePlayerWeapons(ai.fireCannons, ai.fireHarpoons, ai.aimWorldPos);
        }
    }

    // 2. Step Player Ship Physics
    Vec3 playerWind = archipelago.sampleWindAt(playerShip.position);
    playerShip.updatePhysics(
        dt,
        playerWind,
        playerAimWorldPos.x,
        playerAimWorldPos.y,
        playerAimWorldPos.z
    );

    // Check Island discovery & gentle island collision bump
    for (auto& isl : archipelago.islands) {
        Vec3 diff = playerShip.position - isl.position;
        float distHoriz = std::sqrt(diff.x * diff.x + diff.z * diff.z);
        if (!isl.discovered && distHoriz < isl.radius + 190.0f) {
            isl.discovered = true;
            playerGold += 35;
            statusBannerText = "Discovered Sky-Island: " + isl.name + " (+35 Gold)";
            statusBannerTimer = 5.0f;
        }
        float effIslRadius = (playerShip.position.y > isl.position.y + 7.5f)
            ? (isl.radius * 0.38f)
            : (isl.radius * 0.96f);
        float minSafe = effIslRadius + playerShip.boundingRadius * 0.55f;
        if (distHoriz < minSafe &&
            playerShip.position.y < isl.position.y + isl.spireHeight &&
            playerShip.position.y > isl.position.y - isl.thickness) {
            Vec3 pushDir = (distHoriz > 0.1f)
                ? Vec3{diff.x / distHoriz, 0.2f, diff.z / distHoriz}
                : Vec3{0.0f, 1.0f, 0.0f};
            playerShip.position += pushDir * ((minSafe - distHoriz) * 0.5f);
            playerShip.velocity = pushDir * 16.0f;
        }
    }

    // 3. Step NPC Ships (Pirates & Allied Wingmen)
    for (auto& npc : npcShips) {
        if (npc.isDestroyed) {
            continue;
        }
        AIDecision ai = AIController::computeShipControl(
            npc, &playerShip, npcShips, leviathans, archipelago, false
        );
        npc.throttleInput = ai.throttle;
        npc.rudderInput = ai.rudder;
        npc.altitudeInput = ai.altitude;
        Vec3 wind = archipelago.sampleWindAt(npc.position);
        npc.updatePhysics(dt, wind, ai.aimWorldPos.x, ai.aimWorldPos.y, ai.aimWorldPos.z);

        if (ai.fireCannons) {
            std::vector<MuzzleFireRequest> reqs;
            npc.fireWeapons(true, false, ai.aimWorldPos, reqs);
            for (const auto& r : reqs) {
                Projectile3D proj;
                proj.id = nextEntityId_++;
                proj.ownerShipId = npc.id();
                proj.ownerFaction = npc.faction();
                proj.weaponType = r.weaponType;
                proj.position = r.muzzleWorldPos;
                proj.prevPosition = r.muzzleWorldPos;
                proj.velocity = r.aimDirection * r.speed + r.inheritedVelocity;
                proj.damage = r.damage * 0.65f;
                proj.splashRadius = 3.8f;
                proj.lifetime = 4.0f;
                proj.maxLifetime = 4.0f;
                proj.color = (npc.faction() == 1) ? ColorRGB{255, 95, 75} : ColorRGB{95, 225, 165};
                projectiles.push_back(proj);
            }
        }

        // Sink & destroy if fallen deep into the abyss below Y = -20
        if (npc.position.y < -15.0f && !npc.isDestroyed) {
            npc.isDestroyed = true;
            if (npc.faction() == 1) {
                piratesSunk++;
                playerGold += 120;
                crates.push_back({nextEntityId_++, npc.position + Vec3{0.0f, 40.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, 95, 5, 60.0f, false});
                statusBannerText = "Enemy Sky-Pirate Vessel Sunk! (+120 Gold)";
                statusBannerTimer = 4.5f;
            }
        }
    }

    // 4. Step Harpoon Tethers & Compute Pull Forces
    stepHarpoonTethers(dt);

    // 5. Step Colossal Sky-Leviathans
    for (auto& lev : leviathans) {
        if (lev.isSlain) {
            continue;
        }
        // Aggregate tether pull force acting on this Leviathan
        Vec3 totalPull{0.0f, 0.0f, 0.0f};
        int activeTethers = 0;
        for (const auto& t : tethers) {
            if (t.active && t.targetLeviathanId == lev.id()) {
                activeTethers++;
                Vec3 shipAnchor = playerShip.localToWorldPos(t.sourceLocalOffset);
                int segIdx = std::clamp(t.targetSegmentIdx, 0, static_cast<int>(lev.segments.size()) - 1);
                Vec3 segPos = lev.segments[static_cast<size_t>(segIdx)].position;
                Vec3 pullDir = (shipAnchor - segPos).normalized();
                totalPull += pullDir * t.tensionForce;
            }
        }

        std::vector<LeviathanAttackBolt> bolts;
        lev.update(dt, playerShip.position, totalPull, activeTethers, bolts);

        for (const auto& b : bolts) {
            Projectile3D proj;
            proj.id = nextEntityId_++;
            proj.ownerLeviathanId = lev.id();
            proj.ownerFaction = 2;
            proj.position = b.origin;
            proj.prevPosition = b.origin;
            proj.velocity = b.velocity;
            proj.damage = b.damage;
            proj.splashRadius = 4.5f;
            proj.gravityScale = 0.2f;
            proj.lifetime = 4.0f;
            proj.maxLifetime = 4.0f;
            proj.color = b.color;
            projectiles.push_back(proj);
        }
    }

    // 6. Step Projectiles & Collisions
    stepProjectiles(dt);

    // 7. Step Falling Debris & Salvage Crates
    stepDebrisAndCrates(dt);
}

void World::stepHarpoonTethers(float dt) {
    for (auto& t : tethers) {
        if (!t.active) {
            continue;
        }
        // Locate target Leviathan
        Leviathan* targetLev = nullptr;
        for (auto& lev : leviathans) {
            if (lev.id() == t.targetLeviathanId && !lev.isSlain) {
                targetLev = &lev;
                break;
            }
        }
        if (!targetLev || targetLev->segments.empty()) {
            t.active = false;
            continue;
        }

        int segIdx = std::clamp(t.targetSegmentIdx, 0, static_cast<int>(targetLev->segments.size()) - 1);
        Vec3 levAnchor = targetLev->segments[static_cast<size_t>(segIdx)].position;
        Vec3 shipAnchor = playerShip.localToWorldPos(t.sourceLocalOffset);

        Vec3 delta = levAnchor - shipAnchor;
        t.currentLength = delta.length();

        if (t.isReelingIn) {
            t.restLength = std::max(32.0f, t.restLength - 28.0f * dt);
        }

        if (t.currentLength > t.restLength) {
            float stretch = t.currentLength - t.restLength;
            t.tensionForce = stretch * 16.0f;

            if (t.tensionForce > t.maxBreakTension) {
                // Cable snaps under extreme strain!
                t.active = false;
                pendingAudioEvents.push_back({SimAudioEventType::CableSnap, shipAnchor, 1.0f});
                statusBannerText = "WARNING: Harpoon Winch Cable Snapped Under Extreme Leviathan Strain!";
                statusBannerTimer = 3.5f;
                continue;
            }

            Vec3 dirToLev = delta.normalized();
            // Pull the player airship along behind the Leviathan (and slightly rotate bow toward it)
            playerShip.velocity += dirToLev * ((t.tensionForce / std::max(60.0f, playerShip.totalMass)) * dt * 2.5f);
        } else {
            t.tensionForce = 0.0f;
        }
    }

    tethers.erase(
        std::remove_if(tethers.begin(), tethers.end(), [](const HarpoonTether& t) { return !t.active; }),
        tethers.end()
    );
}

void World::stepProjectiles(float dt) {
    for (auto& p : projectiles) {
        if (!p.active) {
            continue;
        }
        p.prevPosition = p.position;
        p.velocity.y -= 9.8f * p.gravityScale * dt;
        p.position += p.velocity * dt;
        p.lifetime -= dt;

        if (p.lifetime <= 0.0f || p.position.y < -40.0f) {
            p.active = false;
            continue;
        }

        Vec3 segDelta = p.position - p.prevPosition;
        float stepLen = segDelta.length();
        Ray3D stepRay{p.prevPosition, (stepLen > 1e-4f) ? (segDelta / stepLen) : Vec3{0.0f, 0.0f, -1.0f}};

        // A. Check collision against Colossal Sky-Leviathans (if fired by a ship)
        if (p.ownerShipId > 0) {
            for (auto& lev : leviathans) {
                if (lev.isSlain) {
                    continue;
                }
                int hitSeg = -1;
                Vec3 hitPos{};
                float hitDist = 0.0f;
                if (lev.raycast(stepRay, stepLen + 3.5f, hitSeg, hitPos, hitDist)) {
                    int clampedSeg = std::clamp(hitSeg, 0, static_cast<int>(lev.segments.size()) - 1);
                    float prevArmor = lev.segments[static_cast<size_t>(clampedSeg)].armorHp;
                    bool wasWeakSpot = lev.segments[static_cast<size_t>(clampedSeg)].hasWeakSpotCore || (prevArmor <= 0.0f);

                    lev.damageSegment(hitSeg, p.damage, p.isHarpoon);
                    float newArmor = lev.segments[static_cast<size_t>(clampedSeg)].armorHp;

                    if (p.ownerShipId == playerShip.id()) {
                        if (prevArmor > 0.0f && newArmor <= 0.0f) {
                            callouts.push_back({hitPos + Vec3{0.0f, 6.0f, 0.0f}, "CARAPACE SHATTERED! CORE EXPOSED!", {85, 245, 255}, 2.0f, 2.0f});
                            cameraTrauma = std::min(1.0f, cameraTrauma + 0.35f);
                        } else if (wasWeakSpot) {
                            int critDmg = static_cast<int>(p.damage * 2.4f);
                            callouts.push_back({hitPos + Vec3{0.0f, 5.0f, 0.0f}, "CRIT! -" + std::to_string(critDmg) + " [AETHER CORE]", {255, 95, 95}, 1.5f, 1.5f});
                        } else {
                            callouts.push_back({hitPos + Vec3{0.0f, 4.0f, 0.0f}, "-" + std::to_string(static_cast<int>(p.damage)), {255, 215, 95}, 1.2f, 1.2f});
                        }
                    }

                    explosions.push_back({hitPos, p.splashRadius * 1.4f, p.color, false, p.weaponType == BlockId::TeslaCoil, p.prevPosition});
                    pendingAudioEvents.push_back({
                        p.isHarpoon ? SimAudioEventType::HarpoonLatch : SimAudioEventType::BlockImpact,
                        hitPos,
                        1.0f
                    });

                    // Attach physical Harpoon Tether if this was a Harpoon bolt!
                    if (p.isHarpoon && p.ownerShipId == playerShip.id()) {
                        HarpoonTether tether;
                        tether.id = nextEntityId_++;
                        tether.sourceShipId = playerShip.id();
                        tether.sourceLocalOffset = p.launchAnchorLocal;
                        tether.targetLeviathanId = lev.id();
                        tether.targetSegmentIdx = hitSeg;
                        tether.currentLength = (hitPos - playerShip.localToWorldPos(p.launchAnchorLocal)).length();
                        tether.restLength = std::max(45.0f, tether.currentLength * 0.95f);
                        tethers.push_back(tether);
                        callouts.push_back({hitPos + Vec3{0.0f, 7.0f, 0.0f}, "HARPOON LATCHED!", {85, 255, 165}, 1.8f, 1.8f});
                        statusBannerText = "HARPOON LATCHED onto " + lev.name() + "! Hold [R] to Reel In, [F] to Cut Cable.";
                        statusBannerTimer = 4.5f;
                    }

                    if (lev.isSlain) {
                        leviathansSlain++;
                        playerGold += lev.goldBounty;
                        playerAether += lev.aetherBounty;
                        cameraTrauma = 1.0f;
                        crates.push_back({nextEntityId_++, hitPos, {0.0f, 4.0f, 0.0f}, lev.goldBounty / 2, lev.aetherBounty / 2, 90.0f, false});
                        pendingAudioEvents.push_back({SimAudioEventType::LeviathanRoar, hitPos, 1.5f});
                        statusBannerText = "COLOSSAL LEVIATHAN SLAIN: " + lev.name() + "! (+" + std::to_string(lev.goldBounty) + " Gold, +" + std::to_string(lev.aetherBounty) + " Aether)";
                        statusBannerTimer = 7.0f;

                        for (auto& c : contracts) {
                            if (c.targetName == lev.name()) {
                                c.completed = true;
                            }
                        }
                    }

                    p.active = false;
                    break;
                }
            }
        }
        if (!p.active) continue;

        // B. Check collision against NPC Airships (only opposite faction)
        for (auto& npc : npcShips) {
            if (npc.isDestroyed || npc.id() == p.ownerShipId || npc.faction() == p.ownerFaction) {
                continue;
            }
            if ((p.position - npc.position).length() <= npc.boundingRadius + 4.0f) {
                Int3 hitCoord{}, adjCoord{};
                Vec3 hitWorld{};
                float hitDist = 0.0f;
                if (npc.raycastBlocks(stepRay, stepLen + 3.0f, hitCoord, adjCoord, hitWorld, hitDist)) {
                    int broken = npc.applyDamageAtWorldPos(hitWorld, p.damage, p.splashRadius, true, debris);
                    blocksDestroyedStat += broken;
                    if (p.ownerShipId == playerShip.id()) {
                        if (broken > 1) {
                            callouts.push_back({hitWorld + Vec3{0.0f, 5.0f, 0.0f}, "HULL FRACTURE! [" + std::to_string(broken) + " BLOCKS]", {255, 145, 55}, 1.7f, 1.7f});
                            cameraTrauma = std::min(1.0f, cameraTrauma + 0.28f);
                        } else {
                            callouts.push_back({hitWorld + Vec3{0.0f, 3.5f, 0.0f}, "-" + std::to_string(static_cast<int>(p.damage)), {255, 205, 95}, 1.1f, 1.1f});
                        }
                    }
                    explosions.push_back({hitWorld, p.splashRadius, p.color, false, false, p.prevPosition});
                    pendingAudioEvents.push_back({
                        broken > 1 ? SimAudioEventType::StructuralFracture : SimAudioEventType::BlockImpact,
                        hitWorld,
                        1.0f
                    });

                    if (npc.isDestroyed && npc.faction() == 1) {
                        piratesSunk++;
                        playerGold += 120;
                        crates.push_back({nextEntityId_++, npc.position, {0.0f, 3.0f, 0.0f}, 95, 5, 60.0f, false});
                    }
                    p.active = false;
                    break;
                }
            }
        }
        if (!p.active) continue;

        // C. Check collision against Player Airship (only if fired by hostile Pirate or Leviathan)
        if (p.ownerFaction != playerShip.faction() && !playerShip.isDestroyed) {
            if ((p.position - playerShip.position).length() <= playerShip.boundingRadius + 4.0f) {
                Int3 hitCoord{}, adjCoord{};
                Vec3 hitWorld{};
                float hitDist = 0.0f;
                if (playerShip.raycastBlocks(stepRay, stepLen + 3.0f, hitCoord, adjCoord, hitWorld, hitDist)) {
                    int broken = playerShip.applyDamageAtWorldPos(hitWorld, p.damage * 0.55f, p.splashRadius, true, debris);
                    cameraTrauma = std::min(1.0f, cameraTrauma + (broken > 0 ? 0.35f : 0.12f));
                    explosions.push_back({hitWorld, p.splashRadius, p.color, false, false, p.prevPosition});
                    pendingAudioEvents.push_back({
                        broken > 1 ? SimAudioEventType::StructuralFracture : SimAudioEventType::BlockImpact,
                        hitWorld,
                        1.0f
                    });
                    p.active = false;
                    continue;
                }
            }
        }

        // D. Check collision against Island Aetherium Crystal Nodes (mining!)
        if (p.ownerShipId == playerShip.id()) {
            for (auto& isl : archipelago.islands) {
                if ((p.position - isl.position).length() > isl.radius + 25.0f) {
                    continue;
                }
                for (auto& cry : isl.crystals) {
                    if (cry.harvested) continue;
                    if ((p.position - cry.worldPos).length() <= cry.radius + 2.5f) {
                        cry.hp -= p.damage;
                        explosions.push_back({cry.worldPos, 4.0f, {65, 235, 255}, false, false, p.prevPosition});
                        if (cry.hp <= 0.0f) {
                            cry.harvested = true;
                            playerAether += cry.aetherYield;
                            playerGold += cry.goldYield;
                            statusBannerText = "Shattered Aetherium Crystal Vein! (+" + std::to_string(cry.aetherYield) + " Aether, +" + std::to_string(cry.goldYield) + " Gold)";
                            statusBannerTimer = 4.0f;
                        }
                        p.active = false;
                        break;
                    }
                }
                if (!p.active) break;
            }
        }
    }

    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile3D& p) { return !p.active; }),
        projectiles.end()
    );
}

void World::stepDebrisAndCrates(float dt) {
    for (auto& d : debris) {
        d.velocity.y -= 18.0f * dt;
        d.worldPos += d.velocity * dt;
        d.rotationEuler += d.angularVel * dt;
        d.lifetime -= dt;
    }
    debris.erase(
        std::remove_if(debris.begin(), debris.end(), [](const DetachedDebris& d) { return d.lifetime <= 0.0f; }),
        debris.end()
    );

    for (auto& c : crates) {
        if (c.collected) continue;
        c.lifetime -= dt;
        // Tractor pull when player ship is within 65m
        Vec3 toPlayer = playerShip.position - c.position;
        float dist = toPlayer.length();
        if (dist < 65.0f && dist > 0.1f) {
            c.velocity = (toPlayer / dist) * 48.0f;
        } else {
            c.velocity.y = std::sin(c.lifetime * 2.5f) * 1.5f;
        }
        c.position += c.velocity * dt;

        if (dist < playerShip.boundingRadius + 6.0f) {
            c.collected = true;
            playerGold += c.goldAmount;
            playerAether += c.aetherAmount;
            pendingAudioEvents.push_back({SimAudioEventType::CratePickup, c.position, 1.0f});
        }
    }
    crates.erase(
        std::remove_if(crates.begin(), crates.end(), [](const SalvageCrate& c) { return c.collected || c.lifetime <= 0.0f; }),
        crates.end()
    );

    // Step 3D floating combat callouts
    for (auto& co : callouts) {
        co.worldPos.y += 8.5f * dt;
        co.lifetime -= dt;
    }
    callouts.erase(
        std::remove_if(callouts.begin(), callouts.end(), [](const CombatCallout3D& c) { return c.lifetime <= 0.0f; }),
        callouts.end()
    );
}

} // namespace aethernaut
