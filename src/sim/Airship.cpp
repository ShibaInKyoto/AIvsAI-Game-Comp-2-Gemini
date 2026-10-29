#include "sim/Airship.h"
#include <queue>
#include <cmath>
#include <algorithm>

namespace aethernaut {

Airship::Airship(int id, const std::string& name, int faction)
    : id_(id), name_(name), faction_(faction) {
    grid_.resize(static_cast<size_t>(kShipGridX * kShipGridY * kShipGridZ));
}

bool Airship::isValidCoord(int gx, int gy, int gz) const {
    return gx >= minX() && gx <= maxX() &&
           gy >= minY() && gy <= maxY() &&
           gz >= minZ() && gz <= maxZ();
}

const VoxelCell& Airship::getCell(int gx, int gy, int gz) const {
    if (!isValidCoord(gx, gy, gz)) {
        return emptyCell_;
    }
    return grid_[static_cast<size_t>(flatIndex(gx, gy, gz))];
}

VoxelCell& Airship::getCellMut(int gx, int gy, int gz) {
    if (!isValidCoord(gx, gy, gz)) {
        return emptyCell_;
    }
    return grid_[static_cast<size_t>(flatIndex(gx, gy, gz))];
}

bool Airship::setBlock(int gx, int gy, int gz, BlockId id, uint8_t orientation) {
    if (!isValidCoord(gx, gy, gz)) {
        return false;
    }
    const BlockSpec& spec = BlockRegistry::get(id);
    VoxelCell& cell = grid_[static_cast<size_t>(flatIndex(gx, gy, gz))];
    cell.id = id;
    cell.orientation = orientation % 4;
    cell.hp = spec.maxHp;
    cell.maxHp = spec.maxHp;
    cell.fireTimer = 0.0f;
    cell.weaponCooldown = 0.0f;
    recomputeMetrics();
    return true;
}

bool Airship::removeBlock(int gx, int gy, int gz) {
    if (!isValidCoord(gx, gy, gz)) {
        return false;
    }
    VoxelCell& cell = grid_[static_cast<size_t>(flatIndex(gx, gy, gz))];
    if (cell.id == BlockId::Empty) {
        return false;
    }
    cell = VoxelCell{};
    recomputeMetrics();
    return true;
}

void Airship::clearAllBlocks() {
    std::fill(grid_.begin(), grid_.end(), VoxelCell{});
    recomputeMetrics();
}

Vec3 Airship::gridToLocalPos(int gx, int gy, int gz) const {
    return {
        static_cast<float>(gx) * kVoxelSize,
        static_cast<float>(gy) * kVoxelSize,
        static_cast<float>(gz) * kVoxelSize
    };
}

Vec3 Airship::localToWorldPos(const Vec3& localPos) const {
    Quat q = orientationQuat();
    return position + q.rotate(localPos);
}

Vec3 Airship::worldToLocalPos(const Vec3& worldPos) const {
    Quat qInv = orientationQuat().conjugate();
    return qInv.rotate(worldPos - position);
}

Vec3 Airship::gridToWorldPos(int gx, int gy, int gz) const {
    return localToWorldPos(gridToLocalPos(gx, gy, gz));
}

bool Airship::raycastBlocks(
    const Ray3D& worldRay,
    float maxDist,
    Int3& outHitCoord,
    Int3& outAdjacentCoord,
    Vec3& outHitWorldPos,
    float& outHitDist
) const {
    Quat qInv = orientationQuat().conjugate();
    Ray3D localRay;
    localRay.origin = qInv.rotate(worldRay.origin - position);
    localRay.dir = qInv.rotate(worldRay.dir).normalized();

    float bestT = maxDist;
    bool found = false;
    Int3 bestCoord{};
    Vec3 bestNormal{};

    const float half = kVoxelSize * 0.5f;

    for (int gz = minZ(); gz <= maxZ(); ++gz) {
        for (int gy = minY(); gy <= maxY(); ++gy) {
            for (int gx = minX(); gx <= maxX(); ++gx) {
                const VoxelCell& cell = getCell(gx, gy, gz);
                if (cell.id == BlockId::Empty) {
                    continue;
                }
                Vec3 center = gridToLocalPos(gx, gy, gz);
                AABB3D box{
                    {center.x - half, center.y - half, center.z - half},
                    {center.x + half, center.y + half, center.z + half}
                };
                float tHit = 0.0f;
                Vec3 hitNormal{};
                if (intersectRayAABB(localRay, box, tHit, hitNormal)) {
                    if (tHit >= 0.0f && tHit < bestT) {
                        bestT = tHit;
                        found = true;
                        bestCoord = {gx, gy, gz};
                        bestNormal = hitNormal;
                    }
                }
            }
        }
    }

    if (found) {
        outHitCoord = bestCoord;
        outAdjacentCoord = {
            bestCoord.x + static_cast<int>(std::round(bestNormal.x)),
            bestCoord.y + static_cast<int>(std::round(bestNormal.y)),
            bestCoord.z + static_cast<int>(std::round(bestNormal.z))
        };
        outHitDist = bestT;
        outHitWorldPos = worldRay.origin + worldRay.dir * bestT;
    }
    return found;
}

void Airship::recomputeMetrics() {
    totalMass = 0.0f;
    totalLift = 0.0f;
    totalThrust = 0.0f;
    totalTurnTorque = 0.0f;
    totalPowerGen = 0.0f;
    totalPowerDraw = 0.0f;
    totalHp = 0.0f;
    totalMaxHp = 0.0f;
    occupiedBlockCount = 0;
    cannonCount = 0;
    harpoonCount = 0;
    propellerCount = 0;
    liftCellCount = 0;
    shieldProjectorCount = 0;

    float computedShieldMax = 0.0f;
    int afterburnerCount = 0;

    Vec3 weightedMassSum{0.0f, 0.0f, 0.0f};
    Vec3 weightedLiftSum{0.0f, 0.0f, 0.0f};
    float maxRadiusSq = 16.0f;
    bool hasCommandOrCore = false;

    for (int gz = minZ(); gz <= maxZ(); ++gz) {
        for (int gy = minY(); gy <= maxY(); ++gy) {
            for (int gx = minX(); gx <= maxX(); ++gx) {
                const VoxelCell& cell = getCell(gx, gy, gz);
                if (cell.id == BlockId::Empty) {
                    continue;
                }
                const BlockSpec& spec = BlockRegistry::get(cell.id);
                Vec3 localP = gridToLocalPos(gx, gy, gz);

                occupiedBlockCount++;
                totalMass += spec.mass;
                weightedMassSum += localP * spec.mass;

                // Damaged lift cells lose proportional lift
                float healthRatio = (cell.maxHp > 0.0f) ? std::clamp(cell.hp / cell.maxHp, 0.15f, 1.0f) : 1.0f;
                float effectiveLift = spec.liftForce * healthRatio;
                totalLift += effectiveLift;
                weightedLiftSum += localP * effectiveLift;

                totalThrust += spec.thrustForce * healthRatio;
                totalTurnTorque += spec.turnTorque * healthRatio;
                totalPowerGen += spec.powerGen;
                totalPowerDraw += spec.powerDraw;
                totalHp += std::max(0.0f, cell.hp);
                totalMaxHp += cell.maxHp;
                computedShieldMax += spec.shieldCapacity * healthRatio;

                if (cell.id == BlockId::DeckCannon ||
                    cell.id == BlockId::FlakTurret ||
                    cell.id == BlockId::TeslaCoil ||
                    cell.id == BlockId::PlasmaLance) {
                    cannonCount++;
                }
                if (cell.id == BlockId::HarpoonWinch) {
                    harpoonCount++;
                }
                if (cell.id == BlockId::SteamPropeller || cell.id == BlockId::AfterburnerJet) {
                    propellerCount++;
                }
                if (cell.id == BlockId::AfterburnerJet) {
                    afterburnerCount++;
                }
                if (cell.id == BlockId::LiftBalloon || cell.id == BlockId::AetherCore) {
                    liftCellCount++;
                }
                if (cell.id == BlockId::ShieldProjector) {
                    shieldProjectorCount++;
                }
                if (cell.id == BlockId::HelmBridge || cell.id == BlockId::AetherCore) {
                    hasCommandOrCore = true;
                }

                float rSq = localP.lengthSq();
                if (rSq > maxRadiusSq) {
                    maxRadiusSq = rSq;
                }
            }
        }
    }

    if (totalMass > 0.0f) {
        centerOfMassLocal = weightedMassSum / totalMass;
    } else {
        centerOfMassLocal = {0.0f, 0.0f, 0.0f};
        totalMass = 10.0f;
    }

    if (totalLift > 0.0f) {
        centerOfLiftLocal = weightedLiftSum / totalLift;
    } else {
        centerOfLiftLocal = centerOfMassLocal;
    }

    if (totalPowerDraw <= 0.01f) {
        powerEfficiency = 1.0f;
    } else {
        powerEfficiency = std::clamp(totalPowerGen / totalPowerDraw, 0.35f, 1.25f);
    }

    float prevMaxShield = maxShieldHp;
    maxShieldHp = computedShieldMax * shieldMultiplier;
    if (prevMaxShield <= 1.0f && maxShieldHp > 0.0f) {
        shieldHp = maxShieldHp;
    } else {
        shieldHp = std::min(shieldHp, maxShieldHp);
    }

    maxAfterburnerFuel = 100.0f + static_cast<float>(afterburnerCount) * 35.0f;
    afterburnerFuel = std::min(afterburnerFuel, maxAfterburnerFuel);

    boundingRadius = std::sqrt(maxRadiusSq) + kVoxelSize;

    // A ship sinks if its lift is critically below gravity weight or command/cores are all gone
    float weightForce = totalMass * 3.2f;
    isSinking = (totalLift < weightForce * 0.45f) || !hasCommandOrCore;
    isDestroyed = (occupiedBlockCount == 0 || totalHp <= 0.0f);
}

int Airship::severDisconnectedBlocks(std::vector<DetachedDebris>& outDebris) {
    const int totalCells = kShipGridX * kShipGridY * kShipGridZ;
    std::vector<uint8_t> visited(static_cast<size_t>(totalCells), 0);
    std::queue<Int3> bfsQueue;

    // Seed BFS with all HelmBridge and AetherCore blocks
    for (int gz = minZ(); gz <= maxZ(); ++gz) {
        for (int gy = minY(); gy <= maxY(); ++gy) {
            for (int gx = minX(); gx <= maxX(); ++gx) {
                BlockId bid = getCell(gx, gy, gz).id;
                if (bid == BlockId::HelmBridge || bid == BlockId::AetherCore) {
                    visited[static_cast<size_t>(flatIndex(gx, gy, gz))] = 1;
                    bfsQueue.push({gx, gy, gz});
                }
            }
        }
    }

    // If no bridge or core remains, seed from the first non-empty block near the center
    if (bfsQueue.empty()) {
        for (int gz = minZ(); gz <= maxZ() && bfsQueue.empty(); ++gz) {
            for (int gy = minY(); gy <= maxY() && bfsQueue.empty(); ++gy) {
                for (int gx = minX(); gx <= maxX() && bfsQueue.empty(); ++gx) {
                    if (getCell(gx, gy, gz).id != BlockId::Empty) {
                        visited[static_cast<size_t>(flatIndex(gx, gy, gz))] = 1;
                        bfsQueue.push({gx, gy, gz});
                    }
                }
            }
        }
    }

    constexpr Int3 kNeighbors[6] = {
        {+1, 0, 0}, {-1, 0, 0},
        {0, +1, 0}, {0, -1, 0},
        {0, 0, +1}, {0, 0, -1}
    };

    while (!bfsQueue.empty()) {
        Int3 cur = bfsQueue.front();
        bfsQueue.pop();

        for (const Int3& offset : kNeighbors) {
            Int3 nxt = cur + offset;
            if (!isValidCoord(nxt.x, nxt.y, nxt.z)) {
                continue;
            }
            size_t idx = static_cast<size_t>(flatIndex(nxt.x, nxt.y, nxt.z));
            if (!visited[idx] && grid_[idx].id != BlockId::Empty) {
                visited[idx] = 1;
                bfsQueue.push(nxt);
            }
        }
    }

    int severedCount = 0;
    for (int gz = minZ(); gz <= maxZ(); ++gz) {
        for (int gy = minY(); gy <= maxY(); ++gy) {
            for (int gx = minX(); gx <= maxX(); ++gx) {
                size_t idx = static_cast<size_t>(flatIndex(gx, gy, gz));
                if (grid_[idx].id != BlockId::Empty && !visited[idx]) {
                    const BlockSpec& spec = BlockRegistry::get(grid_[idx].id);
                    Vec3 wPos = gridToWorldPos(gx, gy, gz);
                    Vec3 outward = (wPos - position).normalized();
                    DetachedDebris deb;
                    deb.id = grid_[idx].id;
                    deb.worldPos = wPos;
                    deb.velocity = velocity + outward * 14.0f + Vec3{0.0f, -6.0f, 0.0f};
                    deb.angularVel = {outward.z * 3.5f, 1.5f, -outward.x * 3.5f};
                    deb.rotationEuler = {pitch, yaw, roll};
                    deb.lifetime = 5.5f;
                    deb.color = spec.primaryColor;
                    outDebris.push_back(deb);

                    grid_[idx] = VoxelCell{};
                    severedCount++;
                }
            }
        }
    }

    if (severedCount > 0) {
        recomputeMetrics();
    }
    return severedCount;
}

float Airship::absorbShieldDamage(float incomingDamage) {
    if (shieldHp <= 0.0f || incomingDamage <= 0.0f) {
        return incomingDamage;
    }
    shieldRechargeDelay = 3.8f;
    shieldHitFlashTimer = 0.45f;
    if (shieldHp >= incomingDamage) {
        shieldHp -= incomingDamage;
        return 0.0f;
    }
    float leftover = incomingDamage - shieldHp;
    shieldHp = 0.0f;
    return leftover;
}

int Airship::applyDamageAtWorldPos(
    const Vec3& impactWorldPos,
    float baseDamage,
    float splashRadius,
    bool canIgniteFire,
    std::vector<DetachedDebris>& outDebris
) {
    float hullDamage = absorbShieldDamage(baseDamage);
    if (hullDamage <= 0.05f) {
        return 0;
    }

    Vec3 localImpact = worldToLocalPos(impactWorldPos);
    int destroyedBlocks = 0;
    float radiusSq = splashRadius * splashRadius;

    for (int gz = minZ(); gz <= maxZ(); ++gz) {
        for (int gy = minY(); gy <= maxY(); ++gy) {
            for (int gx = minX(); gx <= maxX(); ++gx) {
                VoxelCell& cell = getCellMut(gx, gy, gz);
                if (cell.id == BlockId::Empty) {
                    continue;
                }
                Vec3 bPos = gridToLocalPos(gx, gy, gz);
                float distSq = (bPos - localImpact).lengthSq();
                if (distSq > radiusSq) {
                    continue;
                }
                float dist = std::sqrt(distSq);
                float falloff = 1.0f - std::clamp(dist / std::max(0.1f, splashRadius), 0.0f, 0.85f);
                const BlockSpec& spec = BlockRegistry::get(cell.id);
                float effectiveDmg = std::max(2.0f, hullDamage * falloff - spec.armor);
                cell.hp -= effectiveDmg;

                if (canIgniteFire && cell.hp > 0.0f && (cell.id == BlockId::WoodHull || cell.id == BlockId::LiftBalloon)) {
                    cell.fireTimer = std::max(cell.fireTimer, 5.0f);
                }

                if (cell.hp <= 0.0f) {
                    Vec3 wPos = gridToWorldPos(gx, gy, gz);
                    Vec3 blastDir = (wPos - impactWorldPos).normalized();
                    DetachedDebris deb;
                    deb.id = cell.id;
                    deb.worldPos = wPos;
                    deb.velocity = velocity + blastDir * 22.0f + Vec3{0.0f, 8.0f, 0.0f};
                    deb.angularVel = {blastDir.y * 5.0f, blastDir.x * 4.0f, blastDir.z * 5.0f};
                    deb.rotationEuler = {pitch, yaw, roll};
                    deb.lifetime = 4.5f;
                    deb.color = spec.primaryColor;
                    outDebris.push_back(deb);

                    cell = VoxelCell{};
                    destroyedBlocks++;
                }
            }
        }
    }

    if (destroyedBlocks > 0) {
        recomputeMetrics();
        destroyedBlocks += severDisconnectedBlocks(outDebris);
    } else {
        recomputeMetrics();
    }

    return destroyedBlocks;
}

void Airship::updatePhysics(
    float dt,
    const Vec3& windVector,
    float targetAimWorldX,
    float targetAimWorldY,
    float targetAimWorldZ
) {
    if (isDestroyed) {
        return;
    }

    // 1. Update Shield Dome Recharge & Hit Flash
    if (shieldHitFlashTimer > 0.0f) {
        shieldHitFlashTimer = std::max(0.0f, shieldHitFlashTimer - dt);
    }
    if (shieldRechargeDelay > 0.0f) {
        shieldRechargeDelay = std::max(0.0f, shieldRechargeDelay - dt);
    } else if (shieldHp < maxShieldHp) {
        float shieldRegen = (18.0f + static_cast<float>(shieldProjectorCount) * 14.0f) * powerEfficiency * shieldMultiplier * dt;
        shieldHp = std::min(maxShieldHp, shieldHp + shieldRegen);
    }

    // 2. Update Afterburner Overdrive Fuel & State
    if (afterburnerInput && afterburnerFuel > 2.0f && std::abs(throttleInput) > 0.1f) {
        isBoosting = true;
        afterburnerFuel = std::max(0.0f, afterburnerFuel - 28.0f * dt);
    } else {
        isBoosting = false;
        afterburnerFuel = std::min(maxAfterburnerFuel, afterburnerFuel + 16.0f * dt);
    }

    // 3. Update Propeller spin visual
    float boostSpin = isBoosting ? 16.0f : 0.0f;
    float spinSpeed = 4.0f + (std::abs(throttleInput) * 18.0f + boostSpin) * powerEfficiency;
    propellerSpinAngle += spinSpeed * dt;
    if (propellerSpinAngle > kTwoPi) {
        propellerSpinAngle -= kTwoPi;
    }

    // 4. Update Automated Repair Cranes, Fire Damage, and Turret Tracking
    int activeRepairCranes = 0;
    for (const auto& c : grid_) {
        if (c.id == BlockId::RepairCrane && c.hp > 0.0f) {
            activeRepairCranes++;
        }
    }

    float repairRate = static_cast<float>(activeRepairCranes) * 6.5f * powerEfficiency * repairMultiplier * dt;
    Vec3 targetWorld{targetAimWorldX, targetAimWorldY, targetAimWorldZ};
    Vec3 targetLocal = worldToLocalPos(targetWorld);

    bool needsMetricRefresh = false;
    for (int gz = minZ(); gz <= maxZ(); ++gz) {
        for (int gy = minY(); gy <= maxY(); ++gy) {
            for (int gx = minX(); gx <= maxX(); ++gx) {
                VoxelCell& cell = getCellMut(gx, gy, gz);
                if (cell.id == BlockId::Empty) {
                    continue;
                }
                if (cell.weaponCooldown > 0.0f) {
                    cell.weaponCooldown = std::max(0.0f, cell.weaponCooldown - dt);
                }
                if (cell.fireTimer > 0.0f) {
                    cell.fireTimer -= dt * (1.0f + static_cast<float>(activeRepairCranes) * 0.8f);
                    cell.hp -= 4.5f * dt;
                    if (cell.hp <= 0.0f) {
                        cell = VoxelCell{};
                        needsMetricRefresh = true;
                        continue;
                    }
                }
                if (repairRate > 0.0f && cell.hp < cell.maxHp) {
                    cell.hp = std::min(cell.maxHp, cell.hp + repairRate);
                }
                // Update turret tracking angles in local ship space
                if (cell.id == BlockId::DeckCannon ||
                    cell.id == BlockId::FlakTurret ||
                    cell.id == BlockId::TeslaCoil ||
                    cell.id == BlockId::PlasmaLance ||
                    cell.id == BlockId::HarpoonWinch) {
                    Vec3 bPos = gridToLocalPos(gx, gy, gz);
                    Vec3 toTarget = targetLocal - bPos;
                    cell.turretYaw = std::atan2(toTarget.x, -toTarget.z);
                    float horizDist = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
                    cell.turretPitch = std::atan2(toTarget.y, std::max(1.0f, horizDist));
                }
            }
        }
    }

    if (needsMetricRefresh) {
        recomputeMetrics();
    }

    // 5. Forward Propulsion & Rudder Yaw Torque
    Quat qHeading = Quat::fromAxisAngle({0.0f, 1.0f, 0.0f}, yaw);
    Vec3 forwardDir = qHeading.forward();

    float boostFactor = isBoosting ? 1.75f : 1.0f;
    float effectiveThrust = totalThrust * powerEfficiency * thrustMultiplier * boostFactor * throttleInput;
    float accelForward = effectiveThrust / std::max(25.0f, totalMass);
    velocity += forwardDir * (accelForward * 9.5f * dt);

    // Subtle wind drift influence
    velocity += windVector * (0.35f * dt);

    // Angular Yaw Authority from GyroRudders + Propellers
    float yawAccel = ((totalTurnTorque + 40.0f) / std::max(40.0f, totalMass)) * turnMultiplier * rudderInput * 1.65f;
    angularVelYaw += yawAccel * dt;
    angularVelYaw *= std::pow(0.18f, dt); // Rotational damping
    yaw += angularVelYaw * dt;

    // 6. Vertical Buoyancy & Altitude Control
    if (isSinking) {
        targetCruiseAltitude = std::max(-150.0f, targetCruiseAltitude - 28.0f * dt);
    } else {
        float climbSpeed = (22.0f + (totalLift / std::max(40.0f, totalMass)) * 3.5f);
        targetCruiseAltitude += altitudeInput * climbSpeed * dt;
        targetCruiseAltitude = std::clamp(targetCruiseAltitude, 25.0f, 360.0f);
    }

    float altError = targetCruiseAltitude - position.y;
    float verticalSpringForce = altError * 2.8f - velocity.y * 2.2f;
    if (isSinking) {
        verticalSpringForce = -24.0f;
    }
    velocity.y += verticalSpringForce * dt;

    // 7. Aerodynamic Drag
    velocity.x *= std::pow(0.45f, dt);
    velocity.z *= std::pow(0.45f, dt);
    velocity.y *= std::pow(0.35f, dt);

    position += velocity * dt;

    // 8. Realistic Pitch & Roll Listing from Asymmetric Center-of-Lift vs Center-of-Mass + Turn Banking
    Vec3 liftOffset = centerOfLiftLocal - centerOfMassLocal;
    float imbalanceRoll = std::clamp(-liftOffset.x * 0.08f, -0.45f, 0.45f);
    float imbalancePitch = std::clamp(liftOffset.z * 0.05f, -0.35f, 0.35f);

    float bankRoll = -angularVelYaw * 0.22f;
    float climbPitch = std::clamp(velocity.y * 0.008f, -0.25f, 0.25f);
    if (isSinking) {
        climbPitch = -0.38f;
    }

    float desiredRoll = imbalanceRoll + bankRoll;
    float desiredPitch = imbalancePitch + climbPitch;

    roll += (desiredRoll - roll) * std::min(1.0f, 4.5f * dt);
    pitch += (desiredPitch - pitch) * std::min(1.0f, 4.5f * dt);
}

void Airship::fireWeapons(
    bool fireCannons,
    bool fireHarpoons,
    const Vec3& targetWorldPos,
    std::vector<MuzzleFireRequest>& outProjectiles
) {
    if (isDestroyed) {
        return;
    }

    for (int gz = minZ(); gz <= maxZ(); ++gz) {
        for (int gy = minY(); gy <= maxY(); ++gy) {
            for (int gx = minX(); gx <= maxX(); ++gx) {
                VoxelCell& cell = getCellMut(gx, gy, gz);
                if (cell.id == BlockId::Empty || cell.weaponCooldown > 0.0f) {
                    continue;
                }

                bool isArtillery = (cell.id == BlockId::DeckCannon ||
                                    cell.id == BlockId::FlakTurret ||
                                    cell.id == BlockId::TeslaCoil ||
                                    cell.id == BlockId::PlasmaLance);
                bool isHarpoon = (cell.id == BlockId::HarpoonWinch);

                if ((fireCannons && isArtillery) || (fireHarpoons && isHarpoon)) {
                    const BlockSpec& spec = BlockRegistry::get(cell.id);
                    Vec3 muzzleWorld = gridToWorldPos(gx, gy, gz) + Vec3{0.0f, 1.1f, 0.0f};
                    Vec3 toTarget = (targetWorldPos - muzzleWorld);
                    float dist = toTarget.length();
                    Vec3 aimDir = (dist > 0.1f) ? (toTarget / dist) : orientationQuat().forward();

                    // Slight ballistic elevation compensation for gravity on cannon shells
                    if (cell.id == BlockId::DeckCannon || cell.id == BlockId::HarpoonWinch) {
                        float flightTime = dist / std::max(50.0f, spec.projectileSpeed);
                        aimDir.y += 0.5f * 9.8f * flightTime / std::max(50.0f, spec.projectileSpeed);
                        aimDir = aimDir.normalized();
                    }

                    MuzzleFireRequest req;
                    req.weaponType = cell.id;
                    req.muzzleWorldPos = muzzleWorld + aimDir * 1.8f;
                    req.aimDirection = aimDir;
                    req.inheritedVelocity = velocity * 0.35f;
                    req.damage = spec.weaponDamage * powerEfficiency;
                    req.speed = spec.projectileSpeed;
                    req.ownerShipId = id_;
                    outProjectiles.push_back(req);

                    cell.weaponCooldown = spec.weaponCooldown / std::max(0.5f, powerEfficiency * reloadMultiplier);

                    // Apply physical cannon recoil impulse to the ship!
                    if (cell.id == BlockId::DeckCannon || cell.id == BlockId::PlasmaLance) {
                        velocity -= aimDir * (18.0f / std::max(50.0f, totalMass));
                    }
                }
            }
        }
    }
}

void Airship::triggerEmergencyRepair(float amount) {
    for (auto& cell : grid_) {
        if (cell.id != BlockId::Empty) {
            cell.hp = std::min(cell.maxHp, cell.hp + amount);
            cell.fireTimer = 0.0f;
        }
    }
    shieldHp = std::min(maxShieldHp, shieldHp + amount * 0.8f);
    recomputeMetrics();
}

} // namespace aethernaut
