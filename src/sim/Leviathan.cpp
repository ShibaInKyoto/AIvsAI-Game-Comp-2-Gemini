#include "sim/Leviathan.h"
#include <cmath>
#include <algorithm>

namespace aethernaut {

Leviathan::Leviathan(
    int id,
    LeviathanSpecies species,
    const Vec3& spawnPos,
    uint64_t seed
) : id_(id), species_(species), rng_(seed) {
    homeCenter = spawnPos;
    int numSegments = 18;
    float baseRadius = 6.2f;
    float baseArmor = 180.0f;

    switch (species) {
        case LeviathanSpecies::AzureSkyWhale:
            name_ = "Cetus Aetheria";
            title_ = "Elder Azure Sky-Whale";
            signatureMechanicName_ = "HARMONIC SONG-SHELL";
            counterplayHint_ = "PULSE SONAR [Q] OR LATCH HARPOON [RMB] TO SHATTER SONG-SHELL";
            numSegments = 18;
            baseRadius = 7.2f;
            baseArmor = 180.0f;
            maxTotalHp = 2400.0f;
            goldBounty = 450;
            aetherBounty = 16;
            bodyColor = {42, 112, 178};
            coreColor = {65, 240, 255};
            break;
        case LeviathanSpecies::CrimsonStormWyrm:
            name_ = "Draconis Fulmen";
            title_ = "Crimson Storm-Wyrm";
            signatureMechanicName_ = "FULMEN ION-STATIC SHROUD";
            counterplayHint_ = "GROUND WITH HARPOON TETHERS [RMB+R] OR EMP NOVA [C]";
            numSegments = 24;
            baseRadius = 5.6f;
            baseArmor = 220.0f;
            maxTotalHp = 3400.0f;
            goldBounty = 700;
            aetherBounty = 26;
            bodyColor = {175, 42, 58};
            coreColor = {255, 195, 55};
            break;
        case LeviathanSpecies::ObsidianDreadManta:
            name_ = "Raja Vorago";
            title_ = "Obsidian Dread-Manta";
            signatureMechanicName_ = "ABYSSAL UMBRA CLOAK";
            counterplayHint_ = "SHINE BOW SEARCHLIGHT [L] OR SONAR [Q] TO BURN AWAY CLOAK";
            numSegments = 16;
            baseRadius = 8.0f;
            baseArmor = 260.0f;
            maxTotalHp = 4200.0f;
            goldBounty = 950;
            aetherBounty = 35;
            bodyColor = {58, 52, 86};
            coreColor = {205, 105, 255};
            break;
        case LeviathanSpecies::MaelstromSovereign:
            name_ = "Apex Charybdis";
            title_ = "The Maelstrom Sovereign";
            signatureMechanicName_ = "SINGULARITY VORTEX AEGIS";
            counterplayHint_ = "TETHER REEL [RMB+R] + SONAR [Q] / EMP [C] TO COLLAPSE VORTEX";
            numSegments = 30;
            baseRadius = 8.6f;
            baseArmor = 320.0f;
            maxTotalHp = 6500.0f;
            goldBounty = 1800;
            aetherBounty = 65;
            bodyColor = {28, 38, 54};
            coreColor = {255, 75, 130};
            break;
        case LeviathanSpecies::GlacialCryoKraken:
            name_ = "Architeuthis Borealis";
            title_ = "Glacial Cryo-Kraken";
            signatureMechanicName_ = "BOREAL PERMAFROST CARAPACE";
            counterplayHint_ = "SHATTER ICE WITH SKY-MINE [X], BROADSIDE SALVOS, OR THERMAL GEYSER";
            numSegments = 22;
            baseRadius = 7.5f;
            baseArmor = 285.0f;
            maxTotalHp = 5200.0f;
            goldBounty = 1250;
            aetherBounty = 45;
            bodyColor = {48, 142, 202};
            coreColor = {145, 248, 255};
            break;
        case LeviathanSpecies::SolarPhoenixSerpent:
            name_ = "Ignis Quetzal";
            title_ = "Solar Phoenix-Serpent";
            signatureMechanicName_ = "HELIOS CORONA MANTLE";
            counterplayHint_ = "SNIPE TAIL EXHAUST VENTS OR TRIGGER EMP NOVA [C] TO QUENCH CORONA";
            numSegments = 26;
            baseRadius = 6.8f;
            baseArmor = 295.0f;
            maxTotalHp = 5800.0f;
            goldBounty = 1500;
            aetherBounty = 55;
            bodyColor = {228, 98, 34};
            coreColor = {255, 238, 88};
            break;
    }

    totalHp = maxTotalHp;
    segments.resize(static_cast<size_t>(numSegments));

    for (int i = 0; i < numSegments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(std::max(1, numSegments - 1));
        // Taper profile: large head/thorax, sleek tapering tail
        float profile = std::sin(t * kPi * 0.88f + 0.35f);
        if (t > 0.75f) {
            profile = std::max(0.28f, (1.0f - t) * 2.2f);
        }
        LeviathanSegment& seg = segments[static_cast<size_t>(i)];
        seg.radius = std::max(2.2f, baseRadius * profile);
        seg.jointSpacing = seg.radius * 1.35f;
        seg.position = spawnPos + Vec3{0.0f, 0.0f, static_cast<float>(i) * seg.jointSpacing};
        seg.maxArmorHp = baseArmor * (0.7f + 0.6f * profile);
        seg.armorHp = seg.maxArmorHp;
        seg.hasWeakSpotCore = (i % 3 == 1);

        if (i == 2 || i == 3 || (species == LeviathanSpecies::GlacialCryoKraken && (i == 5 || i == 7)) ||
            (species == LeviathanSpecies::SolarPhoenixSerpent && (i == 4 || i == 8))) {
            seg.hasLateralFins = true;
            seg.finSpan = (species == LeviathanSpecies::ObsidianDreadManta || species == LeviathanSpecies::SolarPhoenixSerpent)
                ? seg.radius * 4.4f
                : seg.radius * 3.2f;
        } else if (i == numSegments - 1) {
            seg.hasLateralFins = true; // Tail fluke
            seg.finSpan = seg.radius * 3.4f;
        }
    }
}

bool Leviathan::triggerCounterplayDisruption(const char* /*sourceReason*/) {
    if (isSlain) return false;
    if (staggeredWindowTimer > 2.5f && !barrierActive) {
        return false; // Already staggered
    }
    barrierActive = false;
    staggeredWindowTimer = 8.5f;
    barrierCycleTimer = 14.0f;
    aiState = LeviathanState::StaggeredExposed;
    stateTimer = 8.5f;
    timesDisrupted++;
    return true;
}

void Leviathan::update(
    float dt,
    const Vec3& primaryTargetPos,
    const Vec3& externalTetherPullForce,
    int activeTetherCount,
    std::vector<LeviathanAttackBolt>& outAttacks
) {
    if (isSlain || segments.empty()) {
        return;
    }

    swimPhase += dt * ((staggeredWindowTimer > 0.0f) ? 1.8f : 3.4f);
    stateTimer -= dt;
    attackCooldown -= dt;

    if (staggeredWindowTimer > 0.0f) {
        staggeredWindowTimer = std::max(0.0f, staggeredWindowTimer - dt);
        barrierActive = false;
    } else {
        barrierCycleTimer -= dt;
        if (barrierCycleTimer <= 0.0f) {
            barrierActive = !barrierActive;
            barrierCycleTimer = barrierActive ? 11.0f : 9.0f;
        }
    }

    Vec3& headPos = segments[0].position;
    Vec3 toTarget = primaryTargetPos - headPos;
    float distToTarget = toTarget.length();

    // Multi-harpoon tethering on Storm-Wyrm or Azure Whale disrupts their barrier!
    if (barrierActive && activeTetherCount >= 2) {
        triggerCounterplayDisruption("MULTI-HARPOON TETHER OVERLOAD");
    }

    // State transitions
    if (staggeredWindowTimer > 0.0f) {
        aiState = LeviathanState::StaggeredExposed;
    } else if (activeTetherCount > 0) {
        aiState = LeviathanState::TetherStruggle;
    } else if (stateTimer <= 0.0f) {
        if (distToTarget < 440.0f) {
            int roll = rng_.rangeInt(0, 2);
            if (roll == 0) {
                aiState = LeviathanState::BreachCharge;
                stateTimer = 6.5f;
            } else if (roll == 1) {
                aiState = LeviathanState::ArtilleryBarrage;
                stateTimer = 7.5f;
            } else {
                aiState = LeviathanState::StalkingOrbit;
                stateTimer = 9.0f;
            }
        } else {
            aiState = LeviathanState::PatrolCruise;
            stateTimer = 8.0f;
        }
    }

    // Compute desired Steering Target with Species-Specific Flight Signature
    Vec3 desiredPos = homeCenter;
    float desiredSpeed = 38.0f;

    switch (aiState) {
        case LeviathanState::PatrolCruise: {
            float orbitAngle = swimPhase * 0.18f;
            desiredPos = homeCenter + Vec3{
                std::cos(orbitAngle) * 180.0f,
                std::sin(swimPhase * 0.4f) * 28.0f,
                std::sin(orbitAngle) * 180.0f
            };
            desiredSpeed = 36.0f;
            jawAngle = 0.1f;
            break;
        }
        case LeviathanState::StalkingOrbit: {
            float orbitAngle = swimPhase * 0.32f;
            float orbitRad = (species_ == LeviathanSpecies::ObsidianDreadManta) ? 160.0f : 135.0f;
            float altOffset = (species_ == LeviathanSpecies::SolarPhoenixSerpent) ? 52.0f : 16.0f;
            desiredPos = primaryTargetPos + Vec3{
                std::cos(orbitAngle) * orbitRad,
                altOffset + std::sin(swimPhase * 0.7f) * 24.0f,
                std::sin(orbitAngle) * orbitRad
            };
            desiredSpeed = (species_ == LeviathanSpecies::CrimsonStormWyrm) ? 62.0f : 52.0f;
            jawAngle = 0.25f;
            break;
        }
        case LeviathanState::BreachCharge: {
            desiredPos = primaryTargetPos;
            desiredSpeed = (species_ == LeviathanSpecies::CrimsonStormWyrm) ? 84.0f : 76.0f;
            jawAngle = 0.68f;
            break;
        }
        case LeviathanState::ArtilleryBarrage: {
            float orbitAngle = swimPhase * 0.25f;
            desiredPos = primaryTargetPos + Vec3{
                std::cos(orbitAngle) * 165.0f,
                (species_ == LeviathanSpecies::SolarPhoenixSerpent) ? 60.0f : 35.0f,
                std::sin(orbitAngle) * 165.0f
            };
            desiredSpeed = 44.0f;
            jawAngle = 0.55f;

            if (attackCooldown <= 0.0f && distToTarget < 480.0f) {
                attackCooldown = (species_ == LeviathanSpecies::MaelstromSovereign) ? 1.35f : 2.1f;
                Vec3 dir = (primaryTargetPos - headPos).normalized();

                if (species_ == LeviathanSpecies::ObsidianDreadManta && segments.size() > 3) {
                    // Wingtip void-dart twin salvo from lateral fins
                    for (int side = -1; side <= 1; side += 2) {
                        Vec3 rightVec = Vec3{dir.z, 0.0f, -dir.x}.normalized();
                        LeviathanAttackBolt bolt;
                        bolt.origin = segments[2].position + rightVec * (static_cast<float>(side) * 18.0f);
                        bolt.velocity = (primaryTargetPos - bolt.origin).normalized() * 175.0f;
                        bolt.damage = 44.0f;
                        bolt.color = coreColor;
                        outAttacks.push_back(bolt);
                    }
                } else {
                    int spreadCount = (species_ == LeviathanSpecies::AzureSkyWhale || species_ == LeviathanSpecies::MaelstromSovereign) ? 2 : 1;
                    for (int b = -spreadCount; b <= spreadCount; ++b) {
                        Vec3 spread{
                            dir.x + static_cast<float>(b) * 0.08f * dir.z,
                            dir.y + 0.035f,
                            dir.z - static_cast<float>(b) * 0.08f * dir.x
                        };
                        LeviathanAttackBolt bolt;
                        bolt.origin = headPos + segments[0].forward * (segments[0].radius + 2.0f);
                        bolt.velocity = spread.normalized() * 155.0f;
                        bolt.damage = 42.0f;
                        bolt.color = coreColor;
                        outAttacks.push_back(bolt);
                    }
                }
            }
            break;
        }
        case LeviathanState::TetherStruggle: {
            Vec3 away = (headPos - primaryTargetPos).normalized();
            desiredPos = headPos + away * 120.0f + Vec3{
                std::sin(swimPhase * 1.8f) * 45.0f,
                std::cos(swimPhase * 1.3f) * 35.0f,
                std::cos(swimPhase * 1.8f) * 45.0f
            };
            desiredSpeed = std::max(22.0f, 58.0f - static_cast<float>(activeTetherCount) * 9.0f);
            jawAngle = 0.45f + 0.2f * std::sin(swimPhase * 4.0f);

            if (attackCooldown <= 0.0f && distToTarget < 380.0f) {
                attackCooldown = 2.6f;
                LeviathanAttackBolt bolt;
                bolt.origin = headPos + segments[0].forward * segments[0].radius;
                bolt.velocity = (primaryTargetPos - headPos).normalized() * 145.0f;
                bolt.damage = 38.0f;
                bolt.color = coreColor;
                outAttacks.push_back(bolt);
            }
            break;
        }
        case LeviathanState::StaggeredExposed: {
            // Staggered resonance opening: slow drifting bank with exposed glowing cores!
            float driftAngle = swimPhase * 0.12f;
            desiredPos = headPos + Vec3{
                std::cos(driftAngle) * 45.0f,
                -4.0f,
                std::sin(driftAngle) * 45.0f
            };
            desiredSpeed = 19.0f;
            jawAngle = 0.72f;
            break;
        }
    }

    Vec3 steerDir = (desiredPos - headPos).normalized();
    Vec3 desiredVel = steerDir * desiredSpeed;
    velocity += (desiredVel - velocity) * std::min(1.0f, 2.4f * dt);

    // Apply physical harpoon tether pull force onto the Leviathan!
    velocity += externalTetherPullForce * (dt * 0.015f);

    // Keep altitude within sky bounds
    if (headPos.y < 35.0f) velocity.y += (35.0f - headPos.y) * 2.5f * dt;
    if (headPos.y > 320.0f) velocity.y -= (headPos.y - 320.0f) * 2.5f * dt;

    headPos += velocity * dt;
    if (velocity.lengthSq() > 0.1f) {
        segments[0].forward = velocity.normalized();
    }

    // 3D Spine Inverse Kinematics: Follow-the-Leader with Sinusoidal Undulation
    bool isLateralSwimmer = (species_ == LeviathanSpecies::CrimsonStormWyrm || species_ == LeviathanSpecies::SolarPhoenixSerpent);
    for (size_t i = 1; i < segments.size(); ++i) {
        LeviathanSegment& prev = segments[i - 1];
        LeviathanSegment& cur = segments[i];

        float waveOffset = std::sin(swimPhase - static_cast<float>(i) * 0.55f) * 1.8f;
        Vec3 waveVec = isLateralSwimmer
            ? Vec3{ prev.forward.z * waveOffset, 0.0f, -prev.forward.x * waveOffset }
            : Vec3{ 0.0f, waveOffset, 0.0f };

        Vec3 delta = (cur.position + waveVec * dt * 4.0f) - prev.position;
        float dist = delta.length();
        Vec3 dir = (dist > 0.001f) ? (delta / dist) : (-prev.forward);

        cur.position = prev.position + dir * prev.jointSpacing;
        cur.forward = (-dir).normalized();
    }
}

float Leviathan::damageSegment(int segmentIdx, float rawDamage, bool isHarpoonHit) {
    if (isSlain || segmentIdx < 0 || segmentIdx >= static_cast<int>(segments.size())) {
        return 0.0f;
    }

    LeviathanSegment& seg = segments[static_cast<size_t>(segmentIdx)];
    float mult = 1.0f;

    // Species-specific weak-point counterplays:
    if (species_ == LeviathanSpecies::SolarPhoenixSerpent &&
        segmentIdx >= static_cast<int>(segments.size()) - 5 && barrierActive) {
        triggerCounterplayDisruption("TAIL EXHAUST VENT CRIT");
    }

    float prevArmor = seg.armorHp;
    if (seg.armorHp > 0.0f) {
        seg.armorHp = std::max(0.0f, seg.armorHp - rawDamage * (isHarpoonHit ? 1.55f : 1.0f));
        mult = 0.85f; // Armored reduction
        if (prevArmor > 0.0f && seg.armorHp <= 0.0f && species_ == LeviathanSpecies::GlacialCryoKraken && barrierActive) {
            triggerCounterplayDisruption("PERMAFROST CARAPACE SHATTERED");
        }
    } else if (seg.hasWeakSpotCore) {
        mult = 2.4f; // Exposed glowing Aether core weak spot!
    } else {
        mult = 1.35f; // Unarmored flesh
    }

    // Apply Boss Puzzle Barrier mitigation or Staggered Resonance amplification
    if (staggeredWindowTimer > 0.0f) {
        mult *= 1.85f;
    } else if (barrierActive && !isHarpoonHit) {
        mult *= 0.48f;
    }

    float actualDmg = rawDamage * mult;
    totalHp = std::max(0.0f, totalHp - actualDmg);
    if (totalHp <= 0.0f) {
        isSlain = true;
    }
    return actualDmg;
}

bool Leviathan::raycast(
    const Ray3D& ray,
    float maxDist,
    int& outSegmentIdx,
    Vec3& outHitPos,
    float& outDist
) const {
    if (isSlain) {
        return false;
    }
    float bestT = maxDist;
    int bestIdx = -1;

    for (size_t i = 0; i < segments.size(); ++i) {
        float tHit = 0.0f;
        if (intersectRaySphere(ray, segments[i].position, segments[i].radius * 1.15f, tHit)) {
            if (tHit >= 0.0f && tHit < bestT) {
                bestT = tHit;
                bestIdx = static_cast<int>(i);
            }
        }
    }

    if (bestIdx >= 0) {
        outSegmentIdx = bestIdx;
        outDist = bestT;
        outHitPos = ray.origin + ray.dir * bestT;
        return true;
    }
    return false;
}

} // namespace aethernaut
