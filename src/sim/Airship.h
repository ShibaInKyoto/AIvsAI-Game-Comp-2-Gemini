#pragma once

#include "core/Math3D.h"
#include "sim/BlockRegistry.h"
#include <vector>
#include <string>
#include <cstdint>

namespace aethernaut {

constexpr int kShipGridX = 15; // X: -7 .. +7
constexpr int kShipGridY = 11; // Y: -5 .. +5
constexpr int kShipGridZ = 23; // Z: -11 .. +11
constexpr float kVoxelSize = 2.0f;

struct VoxelCell {
    BlockId id = BlockId::Empty;
    uint8_t orientation = 0; // 0: -Z (Fwd), 1: +X (Right), 2: +Z (Back), 3: -X (Left)
    float hp = 0.0f;
    float maxHp = 0.0f;
    float fireTimer = 0.0f;
    float weaponCooldown = 0.0f;
    float turretYaw = 0.0f;
    float turretPitch = 0.0f;
    ColorRGB customTint{255, 255, 255};
    bool hasCustomTint = false;
};

struct DetachedDebris {
    BlockId id = BlockId::WoodHull;
    Vec3 worldPos;
    Vec3 velocity;
    Vec3 angularVel;
    Vec3 rotationEuler;
    float lifetime = 6.0f;
    ColorRGB color{148, 98, 56};
};

struct MuzzleFireRequest {
    BlockId weaponType = BlockId::DeckCannon;
    Vec3 muzzleWorldPos;
    Vec3 aimDirection;
    Vec3 inheritedVelocity;
    float damage = 50.0f;
    float speed = 180.0f;
    int ownerShipId = 0;
};

class Airship {
public:
    Airship(int id = 0, const std::string& name = "Unnamed Vessel", int faction = 0);

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    void setName(const std::string& n) { name_ = n; }
    int faction() const { return faction_; } // 0: Player Fleet, 1: Sky Pirates, 2: Imperial Navy

    // Grid coordinate helpers: grid coords are centered around (0, 0, 0)
    static constexpr int minX() { return -(kShipGridX / 2); }
    static constexpr int maxX() { return +(kShipGridX / 2); }
    static constexpr int minY() { return -(kShipGridY / 2); }
    static constexpr int maxY() { return +(kShipGridY / 2); }
    static constexpr int minZ() { return -(kShipGridZ / 2); }
    static constexpr int maxZ() { return +(kShipGridZ / 2); }

    bool isValidCoord(int gx, int gy, int gz) const;
    const VoxelCell& getCell(int gx, int gy, int gz) const;
    VoxelCell& getCellMut(int gx, int gy, int gz);

    bool setBlock(int gx, int gy, int gz, BlockId id, uint8_t orientation = 0);
    bool removeBlock(int gx, int gy, int gz);
    void clearAllBlocks();

    Vec3 gridToLocalPos(int gx, int gy, int gz) const;
    Vec3 gridToWorldPos(int gx, int gy, int gz) const;
    Vec3 localToWorldPos(const Vec3& localPos) const;
    Vec3 worldToLocalPos(const Vec3& worldPos) const;

    // Raycast against all occupied 3D blocks of this airship (in world space)
    bool raycastBlocks(
        const Ray3D& worldRay,
        float maxDist,
        Int3& outHitCoord,
        Int3& outAdjacentCoord,
        Vec3& outHitWorldPos,
        float& outHitDist
    ) const;

    // Recomputes total mass, Center of Mass, Center of Lift, thrust, power, and integrity
    void recomputeMetrics();

    // Performs 3D BFS from Command Bridge / Reactor Cores; severs disconnected islands into debris
    int severDisconnectedBlocks(std::vector<DetachedDebris>& outDebris);

    // Apply localized damage (explosive radius or direct impact)
    int applyDamageAtWorldPos(
        const Vec3& impactWorldPos,
        float baseDamage,
        float splashRadius,
        bool canIgniteFire,
        std::vector<DetachedDebris>& outDebris
    );

    // Step physics, buoyancy, aerodynamics, repair cranes, and turret tracking
    void updatePhysics(float dt, const Vec3& windVector, float targetAimWorldX, float targetAimWorldY, float targetAimWorldZ);

    // Fire all ready weapons of a given type aimed toward targetWorldPos
    void fireWeapons(
        bool fireCannons,
        bool fireHarpoons,
        const Vec3& targetWorldPos,
        std::vector<MuzzleFireRequest>& outProjectiles
    );

    // Emergency crew repair burst
    void triggerEmergencyRepair(float amount);

    // Transform & Kinematic State
    Vec3 position{0.0f, 120.0f, 0.0f};
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;           // Heading in radians
    float pitch = 0.0f;         // Pitch angle (listing + aerodynamic dive/climb)
    float roll = 0.0f;          // Roll angle (bank + asymmetric lift listing)
    float angularVelYaw = 0.0f;

    // Helm Control Inputs (-1 .. +1)
    float throttleInput = 0.0f;
    float rudderInput = 0.0f;
    float altitudeInput = 0.0f; // +1 Ascend ballast, -1 Descend ballast
    float targetCruiseAltitude = 120.0f;

    // Walk-on-Deck Captain State (when player walks on deck)
    Vec3 captainLocalPos{0.0f, 2.5f, 0.0f};
    float captainYaw = 0.0f;
    float captainPitch = 0.0f;

    // Computed Engineering Metrics
    float totalMass = 100.0f;
    float totalLift = 100.0f;
    float totalThrust = 100.0f;
    float totalTurnTorque = 100.0f;
    float totalPowerGen = 60.0f;
    float totalPowerDraw = 20.0f;
    float powerEfficiency = 1.0f; // 0..1 based on reactor supply vs demand
    float totalHp = 100.0f;
    float totalMaxHp = 100.0f;
    int occupiedBlockCount = 0;
    int cannonCount = 0;
    int harpoonCount = 0;
    int propellerCount = 0;
    int liftCellCount = 0;
    Vec3 centerOfMassLocal{0.0f, 0.0f, 0.0f};
    Vec3 centerOfLiftLocal{0.0f, 0.0f, 0.0f};
    float boundingRadius = 18.0f;
    float propellerSpinAngle = 0.0f;
    bool isSinking = false;
    bool isDestroyed = false;

    Quat orientationQuat() const {
        return Quat::fromEuler(pitch, yaw, roll);
    }

private:
    int flatIndex(int gx, int gy, int gz) const {
        int ix = gx - minX();
        int iy = gy - minY();
        int iz = gz - minZ();
        return (iz * kShipGridY + iy) * kShipGridX + ix;
    }

    int id_ = 0;
    std::string name_;
    int faction_ = 0;
    std::vector<VoxelCell> grid_;
    VoxelCell emptyCell_{};
};

} // namespace aethernaut
