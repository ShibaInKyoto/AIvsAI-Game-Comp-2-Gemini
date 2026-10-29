#include "sim/AIController.h"
#include <cmath>
#include <algorithm>

namespace aethernaut {

namespace {

float normalizeAngleSigned(float a) {
    while (a > kPi) a -= kTwoPi;
    while (a < -kPi) a += kTwoPi;
    return a;
}

} // namespace

AIDecision AIController::computeShipControl(
    const Airship& selfShip,
    const Airship* playerShip,
    const std::vector<Airship>& allNpcShips,
    const std::vector<Leviathan>& leviathans,
    const Archipelago& archipelago,
    bool isPlayerAutopilot
) {
    AIDecision out;
    Vec3 myPos = selfShip.position;

    // 1. Acquire nearest hostile target (Enemy Ship or Leviathan)
    Vec3 targetPos = myPos + selfShip.orientationQuat().forward() * 150.0f;
    Vec3 targetVel{0.0f, 0.0f, 0.0f};
    float bestDist = 1e30f;
    bool hasTarget = false;
    bool targetIsLeviathan = false;

    if (selfShip.faction() == 1 && playerShip && !playerShip->isDestroyed) {
        // Pirate ship targets player or allied ships
        float d = (playerShip->position - myPos).length();
        bestDist = d;
        targetPos = playerShip->position;
        targetVel = playerShip->velocity;
        hasTarget = true;
    }

    for (const auto& other : allNpcShips) {
        if (other.id() == selfShip.id() || other.isDestroyed || other.faction() == selfShip.faction()) {
            continue;
        }
        float d = (other.position - myPos).length();
        if (d < bestDist) {
            bestDist = d;
            targetPos = other.position;
            targetVel = other.velocity;
            hasTarget = true;
            targetIsLeviathan = false;
        }
    }

    // Player autopilot or allied whalers also prioritize Leviathans within 650m
    if (isPlayerAutopilot || selfShip.faction() == 0) {
        for (const auto& lev : leviathans) {
            if (lev.isSlain || lev.segments.empty()) {
                continue;
            }
            float d = (lev.segments[0].position - myPos).length();
            if (d < bestDist && d < 750.0f) {
                bestDist = d;
                targetPos = lev.segments[0].position;
                targetVel = lev.velocity;
                hasTarget = true;
                targetIsLeviathan = true;
            }
        }
    }

    // 2. Compute Desired Navigation Heading (Broadside Orbit at ~115m)
    float desiredYaw = selfShip.yaw;
    float desiredAlt = 128.0f;

    if (hasTarget) {
        Vec3 toTarget = targetPos - myPos;
        float horizDist = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
        float angleToTarget = std::atan2(-toTarget.x, -toTarget.z);

        float idealRange = targetIsLeviathan ? 135.0f : 110.0f;
        if (horizDist > idealRange + 45.0f) {
            // Approach target at a slight 25-degree lead angle
            desiredYaw = angleToTarget + 0.35f;
            out.throttle = 1.0f;
        } else if (horizDist < idealRange - 35.0f) {
            // Open distance
            desiredYaw = angleToTarget + kPi - 0.45f;
            out.throttle = 0.85f;
        } else {
            // Broadside tangential orbit!
            desiredYaw = angleToTarget + (kPi * 0.5f);
            out.throttle = 0.75f;
        }
        desiredAlt = std::clamp(targetPos.y + 8.0f, 55.0f, 260.0f);

        // Compute 3D ballistic lead aim point
        out.aimWorldPos = computeBallisticLeadPoint(myPos, selfShip.velocity, 185.0f, targetPos, targetVel);
        out.fireCannons = (horizDist < 340.0f);
        out.fireHarpoons = targetIsLeviathan && (horizDist < 220.0f);
    } else {
        out.throttle = 0.65f;
        out.aimWorldPos = myPos + selfShip.orientationQuat().forward() * 160.0f;
    }

    // 3. 3D Island Obstacle Avoidance
    for (const auto& isl : archipelago.islands) {
        Vec3 toIsl = isl.position - myPos;
        float distHoriz = std::sqrt(toIsl.x * toIsl.x + toIsl.z * toIsl.z);
        float safeRadius = isl.radius + selfShip.boundingRadius + 38.0f;
        if (distHoriz < safeRadius && std::abs(myPos.y - isl.position.y) < 55.0f) {
            float awayAngle = std::atan2(toIsl.x, toIsl.z);
            desiredYaw = awayAngle;
            desiredAlt = std::max(desiredAlt, isl.position.y + isl.spireHeight + 25.0f);
            break;
        }
    }

    float yawDiff = normalizeAngleSigned(desiredYaw - selfShip.yaw);
    out.rudder = std::clamp(yawDiff * 1.8f, -1.0f, 1.0f);

    float altDiff = desiredAlt - selfShip.targetCruiseAltitude;
    if (std::abs(altDiff) > 4.0f) {
        out.altitude = std::clamp(altDiff * 0.1f, -1.0f, 1.0f);
    } else {
        out.altitude = 0.0f;
    }

    return out;
}

} // namespace aethernaut
