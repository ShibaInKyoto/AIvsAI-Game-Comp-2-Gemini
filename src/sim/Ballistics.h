#pragma once

#include "core/Math3D.h"
#include "sim/BlockRegistry.h"
#include <vector>

namespace aethernaut {

struct Projectile3D {
    int id = 0;
    int ownerShipId = -1;
    int ownerLeviathanId = -1;
    int ownerFaction = 0; // 0: Player/Allied, 1: Pirate, 2: Leviathan
    BlockId weaponType = BlockId::DeckCannon;
    Vec3 position;
    Vec3 prevPosition;
    Vec3 velocity;
    Vec3 launchAnchorLocal; // On firing ship (for harpoon cable attachment)
    float damage = 50.0f;
    float splashRadius = 3.2f;
    float gravityScale = 1.0f;
    float lifetime = 5.0f;
    float maxLifetime = 5.0f;
    ColorRGB color{255, 200, 90};
    bool isHarpoon = false;
    bool isShockHarpoon = false;
    bool isHomingTorpedo = false;
    bool isIncendiary = false;
    bool active = true;
};

struct HarpoonTether {
    int id = 0;
    int sourceShipId = 0;
    Vec3 sourceLocalOffset{0.0f, 1.0f, -3.0f};
    int targetLeviathanId = -1;
    int targetSegmentIdx = 0;
    int targetShipId = -1;
    Vec3 targetShipLocalOffset{0.0f, 0.0f, 0.0f};

    float restLength = 95.0f;
    float currentLength = 95.0f;
    float tensionForce = 0.0f;
    float maxBreakTension = 950.0f;
    bool isShockTether = false;
    float shockPulseTimer = 0.0f;
    bool isReelingIn = false;
    bool active = true;
};

struct ExplosionEvent3D {
    Vec3 position;
    float radius = 6.0f;
    ColorRGB color{255, 160, 50};
    bool isWaterOrCloudSplash = false;
    bool isElectricArc = false;
    Vec3 arcStart;
};

struct SkyMine3D {
    int id = 0;
    int ownerShipId = 0;
    int ownerFaction = 0;
    Vec3 position{0.0f, 130.0f, 0.0f};
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    float triggerRadius = 46.0f;
    float blastRadius = 15.0f;
    float damage = 260.0f;
    float armTimer = 0.55f;
    float lifetime = 45.0f;
    float pulsePhase = 0.0f;
    bool active = true;
};

// Computes 3D ballistic lead position for a moving target
Vec3 computeBallisticLeadPoint(
    const Vec3& shooterPos,
    const Vec3& shooterVel,
    float projectileSpeed,
    const Vec3& targetPos,
    const Vec3& targetVel
);

} // namespace aethernaut
