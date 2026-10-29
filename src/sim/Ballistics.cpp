#include "sim/Ballistics.h"
#include <cmath>
#include <algorithm>

namespace aethernaut {

Vec3 computeBallisticLeadPoint(
    const Vec3& shooterPos,
    const Vec3& shooterVel,
    float projectileSpeed,
    const Vec3& targetPos,
    const Vec3& targetVel
) {
    Vec3 relPos = targetPos - shooterPos;
    Vec3 relVel = targetVel - shooterVel * 0.35f;
    float dist = relPos.length();
    float speed = std::max(40.0f, projectileSpeed);

    // Two-iteration time-of-flight refinement
    float t = dist / speed;
    Vec3 predictedPos = targetPos + relVel * t;
    t = (predictedPos - shooterPos).length() / speed;
    predictedPos = targetPos + relVel * t;

    // Gravity drop compensation
    predictedPos.y += 0.5f * 9.8f * t * t * 0.45f;
    return predictedPos;
}

} // namespace aethernaut
