#pragma once

#include "core/Math3D.h"
#include <cstdint>
#include <string>
#include <array>

namespace aethernaut {

enum class BlockId : uint8_t {
    Empty = 0,
    WoodHull = 1,
    IronArmor = 2,
    LiftBalloon = 3,
    AetherCore = 4,
    SteamPropeller = 5,
    GyroRudder = 6,
    DeckCannon = 7,
    HarpoonWinch = 8,
    TeslaCoil = 9,
    FlakTurret = 10,
    RepairCrane = 11,
    HelmBridge = 12,
    Count = 13
};

struct ColorRGB {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
};

struct BlockSpec {
    BlockId id = BlockId::Empty;
    const char* name = "Empty";
    const char* category = "None";
    const char* description = "";
    float mass = 0.0f;
    float maxHp = 100.0f;
    float armor = 0.0f;
    float liftForce = 0.0f;     // Upward buoyancy force
    float thrustForce = 0.0f;   // Forward propulsion force
    float turnTorque = 0.0f;    // Yaw & stabilization authority
    float powerGen = 0.0f;      // Reactor output
    float powerDraw = 0.0f;     // Power consumption
    float weaponDamage = 0.0f;
    float weaponCooldown = 1.0f;
    float projectileSpeed = 120.0f;
    int goldCost = 10;
    int aetherCost = 0;
    ColorRGB primaryColor{200, 200, 200};
    ColorRGB accentColor{100, 100, 100};
    bool isEmissive = false;
};

class BlockRegistry {
public:
    static const BlockSpec& get(BlockId id);
    static constexpr size_t count() { return static_cast<size_t>(BlockId::Count); }
};

} // namespace aethernaut
