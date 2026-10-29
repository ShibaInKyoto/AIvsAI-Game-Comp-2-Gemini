#include "sim/ShipBlueprints.h"
#include <array>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace aethernaut {

namespace {

const std::array<ShipBlueprintInfo, 10> kBlueprints = {{
    {
        0, "Zephyr Sky-Cutter", "Balanced Expedition Sloop",
        "Agile single-hull sky-cutter with overhead lift envelope, bow harpoon winch, and broadside deck guns."
    },
    {
        1, "Pequod's Wrath", "Heavy Leviathan Whaler",
        "Reinforced iron-prow whaling brig equipped with 4 Steam Harpoon Winches and automated repair crane."
    },
    {
        2, "IJN Valkyrie", "Armored Broadside Frigate",
        "Riveted ironclad warship carrying 8 Siege Cannons, Aether Shield Dome, and twin Flak Turrets."
    },
    {
        3, "Cloud-Splitter", "Twin-Hull Catamaran",
        "Wide twin-fuselage interceptor with Aether-Ramjet Afterburners, outrigger lift cells, and Flak batteries."
    },
    {
        4, "Storm-Sovereign", "Voltaic Flagship Dreadnought",
        "Colossal capital dreadnought armed with 6 Tesla Arc Spires, 6 Siege Cannons, and Dual Shield Domes."
    },
    {
        5, "Black Squall", "Corsair Pirate Brigantine",
        "Fast sky-pirate raider with forward-mounted harpoons and twin broadside batteries."
    },
    {
        6, "Solaris-Prime", "Orichalcum Royal Battlecruiser",
        "Imperial gilded battlecruiser clad in Orichalcum Plate with 4 Solaris Plasma Lances and Aether Ramjets."
    },
    {
        7, "Nemesis Torpedo Cruiser", "Guided Torpedo Stealth Frigate",
        "High-speed hunter armed with 4 Homing Aether-Torpedo Tubes, 2 Shock-Harpoons, and Chrono-Capacitors."
    },
    {
        8, "Prometheus Fire-Galleon", "Incendiary Siege Galleon",
        "Heavy armored siege warship carrying 6 Dragonfire Siege Mortars, 2 Shock-Harpoons, and Shield Domes."
    },
    {
        9, "Celestial Ark-Carrier", "Imperial Trimaran Titan",
        "Colossal 3-hull Orichalcum trimaran flagship with Triple Shield Domes, Torpedoes, Plasma Lances & Ramjets."
    }
}};

void buildZephyrSloop(Airship& s) {
    s.clearAllBlocks();
    // Keel & Main Deck (Z from -5 to +5, X from -2 to +2)
    for (int z = -5; z <= 5; ++z) {
        int halfW = (z == -5 || z == 5) ? 0 : ((z == -4 || z == 4) ? 1 : 2);
        for (int x = -halfW; x <= halfW; ++x) {
            s.setBlock(x, -1, z, (x == 0) ? BlockId::IronArmor : BlockId::WoodHull);
            s.setBlock(x, 0, z, BlockId::WoodHull);
        }
    }
    // Internal Aether Core & Gyro
    s.setBlock(0, 0, 0, BlockId::AetherCore);
    s.setBlock(0, 0, 3, BlockId::GyroRudder);
    s.setBlock(0, 1, 2, BlockId::HelmBridge);
    s.setBlock(0, 1, 0, BlockId::RepairCrane);

    // Overhead Lift Balloon Envelope (Y = +2, +3) with support struts
    s.setBlock(-1, 1, -2, BlockId::WoodHull);
    s.setBlock(+1, 1, -2, BlockId::WoodHull);
    s.setBlock(-1, 1, +2, BlockId::WoodHull);
    s.setBlock(+1, 1, +2, BlockId::WoodHull);
    for (int z = -4; z <= 3; ++z) {
        for (int x = -1; x <= 1; ++x) {
            s.setBlock(x, 2, z, BlockId::LiftBalloon);
            if (x == 0 && z >= -3 && z <= 2) {
                s.setBlock(x, 3, z, BlockId::LiftBalloon);
            }
        }
    }

    // Weapons: Bow Harpoon Winch + Port/Starboard Broadside Cannons + Flak
    s.setBlock(0, 1, -4, BlockId::HarpoonWinch, 0);
    s.setBlock(-2, 1, -1, BlockId::DeckCannon, 3);
    s.setBlock(+2, 1, -1, BlockId::DeckCannon, 1);
    s.setBlock(-2, 1, +1, BlockId::DeckCannon, 3);
    s.setBlock(+2, 1, +1, BlockId::DeckCannon, 1);
    s.setBlock(0, 1, -2, BlockId::FlakTurret, 0);

    // Stern Propellers & Rudder
    s.setBlock(-2, 0, 4, BlockId::SteamPropeller, 2);
    s.setBlock(+2, 0, 4, BlockId::SteamPropeller, 2);
    s.setBlock(0, 0, 5, BlockId::GyroRudder, 2);
}

void buildWhalerBrig(Airship& s) {
    s.clearAllBlocks();
    for (int z = -6; z <= 6; ++z) {
        int halfW = (std::abs(z) == 6) ? 1 : ((std::abs(z) == 5) ? 2 : 3);
        for (int x = -halfW; x <= halfW; ++x) {
            bool isProw = (z <= -4) || (std::abs(x) == halfW);
            s.setBlock(x, -1, z, isProw ? BlockId::IronArmor : BlockId::WoodHull);
            s.setBlock(x, 0, z, isProw ? BlockId::IronArmor : BlockId::WoodHull);
        }
    }
    s.setBlock(0, 0, -1, BlockId::AetherCore);
    s.setBlock(0, 0, +2, BlockId::AetherCore);
    s.setBlock(0, 1, +3, BlockId::HelmBridge);
    s.setBlock(0, 1, 0, BlockId::RepairCrane);
    s.setBlock(0, 1, -2, BlockId::TeslaCoil);

    // Twin Outrigger Lift Balloons (X = -3, +3, Y = +1, +2)
    for (int z = -4; z <= 4; ++z) {
        s.setBlock(-3, 1, z, BlockId::LiftBalloon);
        s.setBlock(+3, 1, z, BlockId::LiftBalloon);
        s.setBlock(-3, 2, z, BlockId::LiftBalloon);
        s.setBlock(+3, 2, z, BlockId::LiftBalloon);
    }

    // 4 Heavy Harpoon Winches + 4 Deck Cannons
    s.setBlock(-1, 1, -5, BlockId::HarpoonWinch, 0);
    s.setBlock(+1, 1, -5, BlockId::HarpoonWinch, 0);
    s.setBlock(-2, 1, -3, BlockId::HarpoonWinch, 3);
    s.setBlock(+2, 1, -3, BlockId::HarpoonWinch, 1);

    s.setBlock(-2, 1, 0, BlockId::DeckCannon, 3);
    s.setBlock(+2, 1, 0, BlockId::DeckCannon, 1);
    s.setBlock(-2, 1, 2, BlockId::DeckCannon, 3);
    s.setBlock(+2, 1, 2, BlockId::DeckCannon, 1);

    // Triple Stern Propellers
    s.setBlock(-2, 0, 6, BlockId::SteamPropeller, 2);
    s.setBlock(0, 0, 6, BlockId::AfterburnerJet, 2);
    s.setBlock(+2, 0, 6, BlockId::SteamPropeller, 2);
    s.setBlock(-1, 0, 5, BlockId::GyroRudder, 2);
    s.setBlock(+1, 0, 5, BlockId::GyroRudder, 2);
}

void buildIroncladFrigate(Airship& s) {
    s.clearAllBlocks();
    for (int z = -7; z <= 7; ++z) {
        int halfW = (std::abs(z) >= 6) ? 1 : ((std::abs(z) == 5) ? 2 : 3);
        for (int x = -halfW; x <= halfW; ++x) {
            s.setBlock(x, -1, z, BlockId::IronArmor);
            s.setBlock(x, 0, z, (std::abs(x) == halfW) ? BlockId::IronArmor : BlockId::WoodHull);
        }
    }
    s.setBlock(0, 0, -2, BlockId::AetherCore);
    s.setBlock(0, 0, 0, BlockId::AetherCore);
    s.setBlock(0, 0, +2, BlockId::AetherCore);
    s.setBlock(0, 1, +4, BlockId::HelmBridge);
    s.setBlock(0, 1, +2, BlockId::ShieldProjector);
    s.setBlock(0, 1, +1, BlockId::RepairCrane);
    s.setBlock(0, 1, -1, BlockId::RepairCrane);

    // Overhead Armored Lift Gallery
    for (int z = -5; z <= 4; ++z) {
        s.setBlock(-1, 1, z, BlockId::WoodHull);
        s.setBlock(+1, 1, z, BlockId::WoodHull);
        for (int x = -2; x <= 2; ++x) {
            s.setBlock(x, 2, z, BlockId::LiftBalloon);
            if (std::abs(x) <= 1) {
                s.setBlock(x, 3, z, BlockId::LiftBalloon);
            }
        }
    }

    // 8 Broadside Cannons + 2 Flak + Bow Harpoon
    for (int z : {-3, -1, 1, 3}) {
        s.setBlock(-3, 1, z, BlockId::DeckCannon, 3);
        s.setBlock(+3, 1, z, BlockId::DeckCannon, 1);
    }
    s.setBlock(0, 1, -5, BlockId::HarpoonWinch, 0);
    s.setBlock(0, 1, -3, BlockId::FlakTurret, 0);
    s.setBlock(0, 1, +3, BlockId::FlakTurret, 2);

    // 4 Steam Propellers
    s.setBlock(-3, 0, 5, BlockId::SteamPropeller, 2);
    s.setBlock(+3, 0, 5, BlockId::SteamPropeller, 2);
    s.setBlock(-1, 0, 7, BlockId::SteamPropeller, 2);
    s.setBlock(+1, 0, 7, BlockId::SteamPropeller, 2);
    s.setBlock(0, 0, 6, BlockId::GyroRudder, 2);
}

void buildCatamaran(Airship& s) {
    s.clearAllBlocks();
    // Twin Pontoons at X = -4..-2 and X = +2..+4
    for (int z = -6; z <= 6; ++z) {
        for (int side : {-1, +1}) {
            int cx = side * 3;
            s.setBlock(cx, -1, z, BlockId::IronArmor);
            s.setBlock(cx, 0, z, BlockId::WoodHull);
            s.setBlock(cx - side, 0, z, BlockId::WoodHull);
            s.setBlock(cx, 1, z, BlockId::LiftBalloon);
            s.setBlock(cx, 2, z, BlockId::LiftBalloon);
        }
    }
    // Cross-bridge Deck (Z = -2 .. +2)
    for (int z = -2; z <= 2; ++z) {
        for (int x = -2; x <= 2; ++x) {
            s.setBlock(x, 0, z, BlockId::IronArmor);
        }
    }
    s.setBlock(0, 1, 1, BlockId::HelmBridge);
    s.setBlock(-1, 1, 0, BlockId::AetherCore);
    s.setBlock(+1, 1, 0, BlockId::AetherCore);
    s.setBlock(0, 1, -1, BlockId::RepairCrane);

    // Weapons on cross-bridge & pontoons
    s.setBlock(-2, 1, -2, BlockId::HarpoonWinch, 0);
    s.setBlock(+2, 1, -2, BlockId::HarpoonWinch, 0);
    s.setBlock(-1, 1, -2, BlockId::FlakTurret, 0);
    s.setBlock(+1, 1, -2, BlockId::FlakTurret, 0);
    s.setBlock(0, 2, 0, BlockId::TeslaCoil, 0);

    // High-speed quad propellers + twin Aether-Ramjets
    s.setBlock(-3, 0, 7, BlockId::AfterburnerJet, 2);
    s.setBlock(+3, 0, 7, BlockId::AfterburnerJet, 2);
    s.setBlock(-2, 0, 6, BlockId::SteamPropeller, 2);
    s.setBlock(+2, 0, 6, BlockId::SteamPropeller, 2);
    s.setBlock(0, 0, 2, BlockId::GyroRudder, 2);
}

void buildVoltaicDreadnought(Airship& s) {
    s.clearAllBlocks();
    for (int z = -8; z <= 8; ++z) {
        int halfW = (std::abs(z) >= 7) ? 1 : ((std::abs(z) >= 5) ? 3 : 4);
        for (int x = -halfW; x <= halfW; ++x) {
            s.setBlock(x, -1, z, BlockId::IronArmor);
            s.setBlock(x, 0, z, BlockId::IronArmor);
        }
    }
    for (int z : {-3, 0, 3}) {
        s.setBlock(-1, 0, z, BlockId::AetherCore);
        s.setBlock(+1, 0, z, BlockId::AetherCore);
    }
    s.setBlock(0, 1, 5, BlockId::HelmBridge);
    s.setBlock(0, 2, 5, BlockId::GyroRudder);
    s.setBlock(-1, 1, 4, BlockId::ShieldProjector);
    s.setBlock(+1, 1, 4, BlockId::ShieldProjector);
    s.setBlock(0, 1, -1, BlockId::RepairCrane);
    s.setBlock(0, 1, +2, BlockId::RepairCrane);

    // Massive Upper Lift Superstructure
    for (int z = -6; z <= 5; ++z) {
        s.setBlock(-2, 1, z, BlockId::IronArmor);
        s.setBlock(+2, 1, z, BlockId::IronArmor);
        for (int x = -3; x <= 3; ++x) {
            s.setBlock(x, 2, z, BlockId::LiftBalloon);
            s.setBlock(x, 3, z, BlockId::LiftBalloon);
        }
    }

    // 6 Tesla Arc Spires + 6 Deck Cannons + 2 Harpoon Winches
    for (int z : {-4, 0, 4}) {
        s.setBlock(-4, 1, z, BlockId::TeslaCoil, 3);
        s.setBlock(+4, 1, z, BlockId::TeslaCoil, 1);
    }
    for (int z : {-2, 2, 5}) {
        s.setBlock(-4, 1, z, BlockId::DeckCannon, 3);
        s.setBlock(+4, 1, z, BlockId::DeckCannon, 1);
    }
    s.setBlock(-1, 1, -6, BlockId::HarpoonWinch, 0);
    s.setBlock(+1, 1, -6, BlockId::HarpoonWinch, 0);

    // 6 Heavy Propellers
    for (int x : {-3, -2, -1, 1, 2, 3}) {
        s.setBlock(x, 0, 8, BlockId::SteamPropeller, 2);
    }
}

void buildCorsairRaider(Airship& s) {
    s.clearAllBlocks();
    for (int z = -4; z <= 4; ++z) {
        int halfW = (std::abs(z) == 4) ? 1 : 2;
        for (int x = -halfW; x <= halfW; ++x) {
            s.setBlock(x, 0, z, BlockId::WoodHull);
        }
    }
    s.setBlock(0, 0, 0, BlockId::AetherCore);
    s.setBlock(0, 1, 2, BlockId::HelmBridge);
    for (int z = -3; z <= 2; ++z) {
        s.setBlock(-1, 1, z, BlockId::LiftBalloon);
        s.setBlock(0, 2, z, BlockId::LiftBalloon);
        s.setBlock(+1, 1, z, BlockId::LiftBalloon);
    }
    s.setBlock(0, 1, -3, BlockId::DeckCannon, 0);
    s.setBlock(-2, 1, 0, BlockId::DeckCannon, 3);
    s.setBlock(+2, 1, 0, BlockId::DeckCannon, 1);
    s.setBlock(-1, 0, 4, BlockId::SteamPropeller, 2);
    s.setBlock(+1, 0, 4, BlockId::SteamPropeller, 2);
    s.setBlock(0, 0, 4, BlockId::GyroRudder, 2);
}

void buildSolarisBattlecruiser(Airship& s) {
    s.clearAllBlocks();
    for (int z = -7; z <= 7; ++z) {
        int halfW = (std::abs(z) >= 6) ? 1 : ((std::abs(z) == 5) ? 2 : 3);
        for (int x = -halfW; x <= halfW; ++x) {
            s.setBlock(x, -1, z, BlockId::OrichalcumArmor);
            s.setBlock(x, 0, z, BlockId::OrichalcumArmor);
        }
    }
    for (int z : {-3, 0, 3}) {
        s.setBlock(-1, 0, z, BlockId::AetherCore);
        s.setBlock(+1, 0, z, BlockId::AetherCore);
    }
    s.setBlock(0, 1, 4, BlockId::HelmBridge);
    s.setBlock(-1, 1, 3, BlockId::ShieldProjector);
    s.setBlock(+1, 1, 3, BlockId::ShieldProjector);
    s.setBlock(0, 1, 1, BlockId::RepairCrane);
    s.setBlock(0, 1, -1, BlockId::RepairCrane);

    // Overhead Lift Envelope with Orichalcum ribs
    for (int z = -4; z <= 3; ++z) {
        s.setBlock(-2, 1, z, BlockId::OrichalcumArmor);
        s.setBlock(+2, 1, z, BlockId::OrichalcumArmor);
        for (int x = -2; x <= 2; ++x) {
            s.setBlock(x, 2, z, BlockId::LiftBalloon);
            if (std::abs(x) <= 1) {
                s.setBlock(x, 3, z, BlockId::LiftBalloon);
            }
        }
    }

    // 4 Solaris Plasma Lances + 2 Harpoon Winches + 2 Tesla Spires
    s.setBlock(-3, 1, -2, BlockId::PlasmaLance, 3);
    s.setBlock(+3, 1, -2, BlockId::PlasmaLance, 1);
    s.setBlock(-3, 1, +2, BlockId::PlasmaLance, 3);
    s.setBlock(+3, 1, +2, BlockId::PlasmaLance, 1);
    s.setBlock(-1, 1, -5, BlockId::HarpoonWinch, 0);
    s.setBlock(+1, 1, -5, BlockId::HarpoonWinch, 0);
    s.setBlock(-3, 1, 0, BlockId::TeslaCoil, 3);
    s.setBlock(+3, 1, 0, BlockId::TeslaCoil, 1);

    // 4 Aether-Ramjet Afterburners
    s.setBlock(-2, 0, 7, BlockId::AfterburnerJet, 2);
    s.setBlock(-1, 0, 7, BlockId::AfterburnerJet, 2);
    s.setBlock(+1, 0, 7, BlockId::AfterburnerJet, 2);
    s.setBlock(+2, 0, 7, BlockId::AfterburnerJet, 2);
    s.setBlock(0, 0, 6, BlockId::GyroRudder, 2);
}

void buildNemesisTorpedoCruiser(Airship& s) {
    s.clearAllBlocks();
    for (int z = -7; z <= 6; ++z) {
        int halfW = (z <= -5 || z >= 5) ? 1 : 2;
        for (int x = -halfW; x <= halfW; ++x) {
            s.setBlock(x, -1, z, BlockId::IronArmor);
            s.setBlock(x, 0, z, (std::abs(x) == halfW) ? BlockId::OrichalcumArmor : BlockId::IronArmor);
        }
    }
    s.setBlock(0, 0, -2, BlockId::AetherCore);
    s.setBlock(0, 0, +1, BlockId::AetherCore);
    s.setBlock(-1, 1, +2, BlockId::CapacitorBank);
    s.setBlock(+1, 1, +2, BlockId::CapacitorBank);
    s.setBlock(0, 1, +3, BlockId::HelmBridge);
    s.setBlock(0, 1, 0, BlockId::ShieldProjector);
    s.setBlock(0, 1, -1, BlockId::RepairCrane);

    // Outrigger Torpedo Pods & Lift Cells
    for (int z = -4; z <= 3; ++z) {
        s.setBlock(-3, 0, z, BlockId::IronArmor);
        s.setBlock(+3, 0, z, BlockId::IronArmor);
        s.setBlock(-2, 2, z, BlockId::LiftBalloon);
        s.setBlock(0, 2, z, BlockId::LiftBalloon);
        s.setBlock(+2, 2, z, BlockId::LiftBalloon);
        s.setBlock(-1, 3, z, BlockId::LiftBalloon);
        s.setBlock(+1, 3, z, BlockId::LiftBalloon);
    }

    // 4 Guided Aether-Torpedo Tubes + 2 Voltaic Shock-Harpoons
    s.setBlock(-3, 1, -3, BlockId::TorpedoLauncher, 0);
    s.setBlock(+3, 1, -3, BlockId::TorpedoLauncher, 0);
    s.setBlock(-3, 1, +1, BlockId::TorpedoLauncher, 3);
    s.setBlock(+3, 1, +1, BlockId::TorpedoLauncher, 1);
    s.setBlock(-1, 1, -5, BlockId::ShockHarpoon, 0);
    s.setBlock(+1, 1, -5, BlockId::ShockHarpoon, 0);

    // 3 Aether-Ramjets + Dual Gyro Rudders
    s.setBlock(-2, 0, 6, BlockId::AfterburnerJet, 2);
    s.setBlock(0, 0, 6, BlockId::AfterburnerJet, 2);
    s.setBlock(+2, 0, 6, BlockId::AfterburnerJet, 2);
    s.setBlock(-1, 0, 5, BlockId::GyroRudder, 2);
    s.setBlock(+1, 0, 5, BlockId::GyroRudder, 2);
}

void buildPrometheusFireGalleon(Airship& s) {
    s.clearAllBlocks();
    for (int z = -7; z <= 7; ++z) {
        int halfW = (std::abs(z) >= 6) ? 1 : ((std::abs(z) == 5) ? 2 : 3);
        for (int x = -halfW; x <= halfW; ++x) {
            s.setBlock(x, -1, z, BlockId::IronArmor);
            s.setBlock(x, 0, z, (std::abs(x) == halfW) ? BlockId::OrichalcumArmor : BlockId::WoodHull);
        }
    }
    for (int z : {-3, 0, 3}) {
        s.setBlock(0, 0, z, BlockId::AetherCore);
    }
    s.setBlock(0, 1, 4, BlockId::HelmBridge);
    s.setBlock(-1, 1, 3, BlockId::ShieldProjector);
    s.setBlock(+1, 1, 3, BlockId::ShieldProjector);
    s.setBlock(-1, 1, 1, BlockId::CapacitorBank);
    s.setBlock(+1, 1, 1, BlockId::CapacitorBank);
    s.setBlock(0, 1, -1, BlockId::RepairCrane);

    // Overhead Lift Envelope
    for (int z = -5; z <= 4; ++z) {
        s.setBlock(-2, 1, z, BlockId::IronArmor);
        s.setBlock(+2, 1, z, BlockId::IronArmor);
        for (int x = -2; x <= 2; ++x) {
            s.setBlock(x, 2, z, BlockId::LiftBalloon);
            s.setBlock(x, 3, z, BlockId::LiftBalloon);
        }
    }

    // 6 Dragonfire Siege Mortars + 2 Voltaic Shock-Harpoons
    for (int z : {-3, 0, 3}) {
        s.setBlock(-3, 1, z, BlockId::FlameMortar, 3);
        s.setBlock(+3, 1, z, BlockId::FlameMortar, 1);
    }
    s.setBlock(-1, 1, -5, BlockId::ShockHarpoon, 0);
    s.setBlock(+1, 1, -5, BlockId::ShockHarpoon, 0);

    // Propulsion
    s.setBlock(-2, 0, 7, BlockId::SteamPropeller, 2);
    s.setBlock(-1, 0, 7, BlockId::AfterburnerJet, 2);
    s.setBlock(+1, 0, 7, BlockId::AfterburnerJet, 2);
    s.setBlock(+2, 0, 7, BlockId::SteamPropeller, 2);
    s.setBlock(0, 0, 6, BlockId::GyroRudder, 2);
}

void buildCelestialArkCarrier(Airship& s) {
    s.clearAllBlocks();
    // Central Royal Hull (Z = -8 .. +8, X = -2 .. +2)
    for (int z = -8; z <= 8; ++z) {
        int halfW = (std::abs(z) >= 7) ? 1 : 2;
        for (int x = -halfW; x <= halfW; ++x) {
            s.setBlock(x, -1, z, BlockId::OrichalcumArmor);
            s.setBlock(x, 0, z, BlockId::OrichalcumArmor);
        }
    }
    // Twin Outrigger Trimaran Sponsons (X = -5..-4 and +4..+5, Z = -5..+5)
    for (int z = -5; z <= 5; ++z) {
        for (int side : {-1, +1}) {
            s.setBlock(side * 4, 0, z, BlockId::OrichalcumArmor);
            s.setBlock(side * 5, 0, z, BlockId::OrichalcumArmor);
            s.setBlock(side * 4, 1, z, BlockId::LiftBalloon);
            s.setBlock(side * 5, 2, z, BlockId::LiftBalloon);
            if (std::abs(z) <= 2) {
                s.setBlock(side * 3, 0, z, BlockId::OrichalcumArmor); // Cross-wing struts
            }
        }
    }

    for (int z : {-4, -1, 2, 5}) {
        s.setBlock(-1, 0, z, BlockId::AetherCore);
        s.setBlock(+1, 0, z, BlockId::AetherCore);
    }
    s.setBlock(0, 1, 5, BlockId::HelmBridge);
    s.setBlock(0, 1, 3, BlockId::ShieldProjector);
    s.setBlock(-1, 1, 3, BlockId::ShieldProjector);
    s.setBlock(+1, 1, 3, BlockId::ShieldProjector);
    s.setBlock(-1, 1, 1, BlockId::CapacitorBank);
    s.setBlock(+1, 1, 1, BlockId::CapacitorBank);
    s.setBlock(-1, 1, -1, BlockId::CapacitorBank);
    s.setBlock(+1, 1, -1, BlockId::CapacitorBank);
    s.setBlock(0, 1, 0, BlockId::RepairCrane);
    s.setBlock(0, 1, -2, BlockId::RepairCrane);

    // Central Lift Canopy
    for (int z = -6; z <= 5; ++z) {
        s.setBlock(-2, 1, z, BlockId::OrichalcumArmor);
        s.setBlock(+2, 1, z, BlockId::OrichalcumArmor);
        for (int x = -2; x <= 2; ++x) {
            s.setBlock(x, 2, z, BlockId::LiftBalloon);
            s.setBlock(x, 3, z, BlockId::LiftBalloon);
        }
    }

    // Heavy Armament: 4 Torpedo Tubes, 4 Solaris Plasma Lances, 2 Dragonfire Mortars, 2 Shock-Harpoons
    s.setBlock(-5, 1, -4, BlockId::TorpedoLauncher, 0);
    s.setBlock(+5, 1, -4, BlockId::TorpedoLauncher, 0);
    s.setBlock(-5, 1, +2, BlockId::TorpedoLauncher, 3);
    s.setBlock(+5, 1, +2, BlockId::TorpedoLauncher, 1);

    s.setBlock(-5, 1, -1, BlockId::PlasmaLance, 3);
    s.setBlock(+5, 1, -1, BlockId::PlasmaLance, 1);
    s.setBlock(-2, 1, -4, BlockId::PlasmaLance, 3);
    s.setBlock(+2, 1, -4, BlockId::PlasmaLance, 1);

    s.setBlock(-5, 1, +4, BlockId::FlameMortar, 3);
    s.setBlock(+5, 1, +4, BlockId::FlameMortar, 1);

    s.setBlock(-1, 1, -6, BlockId::ShockHarpoon, 0);
    s.setBlock(+1, 1, -6, BlockId::ShockHarpoon, 0);

    // 4 Aether-Ramjets + 2 Sponson Turbines
    s.setBlock(-2, 0, 8, BlockId::AfterburnerJet, 2);
    s.setBlock(-1, 0, 8, BlockId::AfterburnerJet, 2);
    s.setBlock(+1, 0, 8, BlockId::AfterburnerJet, 2);
    s.setBlock(+2, 0, 8, BlockId::AfterburnerJet, 2);
    s.setBlock(-4, 0, 6, BlockId::SteamPropeller, 2);
    s.setBlock(+4, 0, 6, BlockId::SteamPropeller, 2);
    s.setBlock(0, 0, 7, BlockId::GyroRudder, 2);
}

} // namespace

int ShipBlueprints::count() {
    return static_cast<int>(kBlueprints.size());
}

ShipBlueprintInfo ShipBlueprints::getInfo(int index) {
    int clamped = std::clamp(index, 0, count() - 1);
    return kBlueprints[static_cast<size_t>(clamped)];
}

void ShipBlueprints::applyBlueprint(Airship& ship, int index) {
    int clamped = std::clamp(index, 0, count() - 1);
    ship.setName(kBlueprints[static_cast<size_t>(clamped)].name);
    switch (clamped) {
        case 0: buildZephyrSloop(ship); break;
        case 1: buildWhalerBrig(ship); break;
        case 2: buildIroncladFrigate(ship); break;
        case 3: buildCatamaran(ship); break;
        case 4: buildVoltaicDreadnought(ship); break;
        case 5: buildCorsairRaider(ship); break;
        case 6: buildSolarisBattlecruiser(ship); break;
        case 7: buildNemesisTorpedoCruiser(ship); break;
        case 8: buildPrometheusFireGalleon(ship); break;
        case 9: buildCelestialArkCarrier(ship); break;
        default: buildZephyrSloop(ship); break;
    }
    ship.recomputeMetrics();
    ship.shieldHp = ship.maxShieldHp;
    ship.afterburnerFuel = ship.maxAfterburnerFuel;
}

bool ShipBlueprints::saveCustomBlueprint(const Airship& ship, const std::string& filepath) {
    std::ofstream out(filepath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << "AETHERNAUT_SHIP_V1\n";
    out << "NAME " << ship.name() << "\n";
    out << "LIVERY " << ship.hullLiveryId << "\n";
    for (int gz = Airship::minZ(); gz <= Airship::maxZ(); ++gz) {
        for (int gy = Airship::minY(); gy <= Airship::maxY(); ++gy) {
            for (int gx = Airship::minX(); gx <= Airship::maxX(); ++gx) {
                const VoxelCell& c = ship.getCell(gx, gy, gz);
                if (c.id != BlockId::Empty) {
                    out << "BLK " << gx << " " << gy << " " << gz << " "
                        << static_cast<int>(c.id) << " "
                        << static_cast<int>(c.orientation) << "\n";
                }
            }
        }
    }
    return out.good();
}

bool ShipBlueprints::loadCustomBlueprint(Airship& ship, const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        return false;
    }
    std::string header;
    if (!(in >> header) || header != "AETHERNAUT_SHIP_V1") {
        return false;
    }

    ship.clearAllBlocks();
    std::string line;
    std::getline(in, line); // consume newline
    int loadedBlocks = 0;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;
        if (tag == "NAME") {
            std::string rest;
            std::getline(iss >> std::ws, rest);
            if (!rest.empty()) {
                ship.setName(rest);
            }
        } else if (tag == "LIVERY") {
            int liv = 0;
            if (iss >> liv) {
                ship.hullLiveryId = std::clamp(liv, 0, 5);
            }
        } else if (tag == "BLK") {
            int gx = 0, gy = 0, gz = 0, bid = 0, orient = 0;
            if (iss >> gx >> gy >> gz >> bid >> orient) {
                if (bid > 0 && bid < static_cast<int>(BlockId::Count)) {
                    ship.setBlock(gx, gy, gz, static_cast<BlockId>(bid), static_cast<uint8_t>(orient & 3));
                    loadedBlocks++;
                }
            }
        }
    }

    if (loadedBlocks == 0) {
        applyBlueprint(ship, 0);
        return false;
    }
    ship.recomputeMetrics();
    ship.shieldHp = ship.maxShieldHp;
    return true;
}

} // namespace aethernaut
