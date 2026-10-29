#include "sim/BlockRegistry.h"

namespace aethernaut {

namespace {

const std::array<BlockSpec, static_cast<size_t>(BlockId::Count)> kSpecs = {{
    // 0: Empty
    {
        BlockId::Empty, "Empty Space", "None", "Unoccupied 3D grid cell.",
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0, 0,
        {0, 0, 0}, {0, 0, 0}, false
    },
    // 1: WoodHull
    {
        BlockId::WoodHull, "Heartwood Timber Hull", "Structure",
        "Lightweight treated sky-oak decking and structural framing.",
        10.0f, 140.0f, 2.0f, 12.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 10, 0,
        {148, 98, 56}, {102, 65, 36}, false
    },
    // 2: IronArmor
    {
        BlockId::IronArmor, "Ironclad Bulkhead", "Structure",
        "Heavy riveted steel armor plate that absorbs high-caliber cannon fire.",
        32.0f, 420.0f, 16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 25, 0,
        {92, 104, 118}, {60, 68, 78}, false
    },
    // 3: LiftBalloon
    {
        BlockId::LiftBalloon, "Aether Lift Cell", "Buoyancy",
        "Pressurized silk-and-brass gas cell generating strong upward buoyancy.",
        6.0f, 110.0f, 0.0f, 340.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 22, 0,
        {235, 220, 185}, {180, 130, 65}, false
    },
    // 4: AetherCore
    {
        BlockId::AetherCore, "Aetherium Reactor Core", "Power & Lift",
        "Refined crystal core providing 60 MW power and anti-gravity lift.",
        24.0f, 280.0f, 8.0f, 420.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.0f, 1.0f, 0.0f, 75, 5,
        {45, 212, 245}, {14, 145, 195}, true
    },
    // 5: SteamPropeller
    {
        BlockId::SteamPropeller, "Brass Turbine Propeller", "Propulsion",
        "Twin-blade steam turbine delivering high forward & reverse thrust.",
        18.0f, 175.0f, 4.0f, 0.0f, 220.0f, 25.0f, 0.0f, 10.0f, 0.0f, 1.0f, 0.0f, 38, 0,
        {205, 155, 65}, {120, 85, 35}, false
    },
    // 6: GyroRudder
    {
        BlockId::GyroRudder, "Gyro-Stabilizer & Rudder", "Control",
        "Active gyroscope and control vane granting high turning torque and roll stability.",
        15.0f, 165.0f, 5.0f, 20.0f, 35.0f, 160.0f, 0.0f, 6.0f, 0.0f, 1.0f, 0.0f, 32, 0,
        {175, 135, 78}, {90, 140, 175}, false
    },
    // 7: DeckCannon
    {
        BlockId::DeckCannon, "Broadside Siege Cannon", "Artillery",
        "Heavy rifled deck gun firing high-explosive shells on ballistic arcs.",
        22.0f, 200.0f, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 85.0f, 1.35f, 185.0f, 50, 0,
        {72, 78, 88}, {215, 160, 60}, false
    },
    // 8: HarpoonWinch
    {
        BlockId::HarpoonWinch, "Steam Harpoon Winch", "Leviathan Hunting",
        "Launches barbed steel harpoons with tensioned winch cables to tether Leviathans.",
        24.0f, 220.0f, 8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 8.0f, 65.0f, 2.6f, 210.0f, 65, 2,
        {185, 95, 45}, {230, 180, 80}, false
    },
    // 9: TeslaCoil
    {
        BlockId::TeslaCoil, "Voltaic Arc Spire", "Energy Weapon",
        "Discharges high-velocity lightning bolts that pierce armor plating.",
        20.0f, 180.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 18.0f, 52.0f, 0.75f, 320.0f, 80, 4,
        {88, 175, 255}, {165, 110, 255}, true
    },
    // 10: FlakTurret
    {
        BlockId::FlakTurret, "Sky-Sweeper Flak Turret", "Artillery",
        "Rapid-firing twin autocannon effective against fast raiders and Leviathan fins.",
        18.0f, 175.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6.0f, 26.0f, 0.28f, 240.0f, 60, 1,
        {115, 125, 110}, {225, 130, 50}, false
    },
    // 11: RepairCrane
    {
        BlockId::RepairCrane, "Automated Steam Repair Rig", "Utility",
        "Continuously reconstructs damaged structural blocks and douses hull fires.",
        20.0f, 190.0f, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f, 12.0f, 0.0f, 1.0f, 0.0f, 70, 2,
        {75, 185, 125}, {215, 175, 65}, false
    },
    // 12: HelmBridge
    {
        BlockId::HelmBridge, "Captain's Command Bridge", "Command",
        "Armored brass wheelhouse and structural anchor of the airship.",
        28.0f, 650.0f, 18.0f, 85.0f, 40.0f, 95.0f, 25.0f, 0.0f, 0.0f, 1.0f, 0.0f, 100, 0,
        {215, 170, 75}, {65, 195, 235}, true
    }
}};

} // namespace

const BlockSpec& BlockRegistry::get(BlockId id) {
    size_t idx = static_cast<size_t>(id);
    if (idx >= kSpecs.size()) {
        return kSpecs[0];
    }
    return kSpecs[idx];
}

} // namespace aethernaut
