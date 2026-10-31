#include "sim/World.h"
#include <cmath>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace aethernaut {

namespace {

const std::array<OfficerSpec, 5> kOfficerSpecs = {{
    {"Master Gunner Vance", "Ordnance & Ballistics", "+16% Weapon Reload Speed & +10% Shell Damage per Rank", 140, 4},
    {"Chief Engineer Lin", "Aether Core & Shielding", "+25% Shield Dome Capacity & +30% Auto-Repair Speed per Rank", 150, 5},
    {"Harpoon Master Ahab", "Leviathan Tether Winches", "+35% Cable Break Strength & +30% Winch Reel Speed per Rank", 135, 4},
    {"Helmsman Kael", "Turbine & Rudder Trim", "+18% Forward Turbine Thrust & +22% Turn Torque per Rank", 125, 3},
    {"Scholar-Navigator Lyra", "Aetherium Salvage & Lore", "+25% Gold & Aetherium Harvest Yield from Crystals & Bounties", 160, 5}
}};

const std::array<RelicSpec, kRelicCount> kRelicSpecs = {{
    {"Chronos Aether-Dial", "Shattered Prism Atoll", "+22% Weapon Reload Haste & -20% Tactical Ability Cooldowns", 220, 14},
    {"Leviathan Heartstone", "Dread-Maw Monolith", "+16 HP/s Living-Crystal Hull Regen & +25% Shield Dome Capacity", 260, 18},
    {"Orichalcum Gyro-Compass", "Sun-Crest Sanctuary", "+24% Turbine Forward Thrust & +28% Rudder Turn Torque", 200, 12},
    {"Storm-Caller Conduit", "Whispering Spire Reef", "Electrifies All Harpoon Tethers (+36 DPS) & +40% Break Tension", 280, 20},
    {"Imperial Trade Charter", "New Horizon Guildhall", "+25% Market Sale Profit, +25% Bounty Gold & +20 Cargo Hold", 240, 15}
}};

const std::array<MedalSpec, kMedalCount> kMedalSpecs = {{
    {"First Blood of the Clouds", "Sink 1 Corsair Pirate Raider in 3D Broadside Combat", 120, 5},
    {"Leviathan Harpooner", "Slay 1 Colossal Sky-Leviathan in the Archipelago", 250, 10},
    {"Citadel Liberator", "Destroy all Sky-Bastion Flak Towers on a Fortified Island", 220, 8},
    {"Slipstream Regatta Ace", "Fly through 3 Aether-Slipstream Rings", 180, 6},
    {"Iron Prow Rammer", "Execute a high-speed Kinetic Bow Ram impact", 160, 6},
    {"Minefield Tactician", "Detonate a Magnetic Aether Sky-Mine [G] on an enemy", 175, 6},
    {"Merchant Guild Prince", "Earn 300+ Gold in Island Commodity Trade Revenue", 240, 8},
    {"Ancient Relic Awakener", "Awaken 1 Ancient Aether-Relic in the Reliquary [L]", 200, 10},
    {"Academy Commandant", "Promote 3 Specialist Crew Officers in the Academy [K]", 200, 8},
    {"Grand Sky-Sovereign", "Command a 160+ Block Dreadnought or Slay 3 Leviathans", 400, 20}
}};

} // namespace

World::World() : playerShip(1, "HMS Sky-Breaker", 0), spatialHash_(50.0f) {}

const std::array<OfficerSpec, 5>& World::getOfficerSpecs() {
    return kOfficerSpecs;
}

const std::array<RelicSpec, kRelicCount>& World::getRelicSpecs() {
    return kRelicSpecs;
}

const std::array<MedalSpec, kMedalCount>& World::getMedalSpecs() {
    return kMedalSpecs;
}

int World::earnedMedalCount() const {
    int c = 0;
    for (bool b : earnedMedals) {
        if (b) c++;
    }
    return c;
}

void World::checkCommendationMedals() {
    int totalOfficerPromotions = 0;
    for (int r : officerRanks) totalOfficerPromotions += r;

    const bool conditions[kMedalCount] = {
        piratesSunk >= 1,
        leviathansSlain >= 1,
        citadelsLiberatedStat >= 1,
        slipstreamRingsPassedStat >= 3,
        ramImpactsStat >= 1,
        skyMineHitsStat >= 1,
        tradeProfitStat >= 300,
        unlockedRelicCount() >= 1,
        totalOfficerPromotions >= 3,
        (playerShip.occupiedBlockCount >= 160 || leviathansSlain >= 3)
    };

    bool anyNew = false;
    for (int i = 0; i < kMedalCount; ++i) {
        if (!earnedMedals[static_cast<size_t>(i)] && conditions[i]) {
            earnedMedals[static_cast<size_t>(i)] = true;
            anyNew = true;
            const MedalSpec& ms = kMedalSpecs[static_cast<size_t>(i)];
            playerGold += ms.rewardGold;
            playerAether += ms.rewardAether;
            pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 1.35f});
            callouts.push_back({
                playerShip.position + Vec3{0.0f, 11.0f, 0.0f},
                std::string("COMMENDATION EARNED: ") + ms.title + "! (+" + std::to_string(ms.rewardGold) + "G)",
                {255, 225, 75},
                2.8f,
                2.8f
            });
            statusBannerText = std::string("SKY-ADMIRAL MEDAL UNLOCKED [O]: ") + ms.title +
                               " (+" + std::to_string(ms.rewardGold) + "G, +" + std::to_string(ms.rewardAether) + " Ae, +3% Fleet Prestige)!";
            statusBannerTimer = 5.5f;
        }
    }
    if (anyNew) {
        applyCrewMultipliers();
    }
}

int World::unlockedRelicCount() const {
    int c = 0;
    for (bool b : unlockedRelics) {
        if (b) c++;
    }
    return c;
}

bool World::unlockRelic(int relicIdx) {
    if (relicIdx < 0 || relicIdx >= kRelicCount) {
        return false;
    }
    if (unlockedRelics[static_cast<size_t>(relicIdx)]) {
        statusBannerText = std::string(kRelicSpecs[static_cast<size_t>(relicIdx)].name) + " is already awakened in your Fleet Reliquary!";
        statusBannerTimer = 3.0f;
        return false;
    }
    const RelicSpec& spec = kRelicSpecs[static_cast<size_t>(relicIdx)];
    if (!isSandboxMode && (playerGold < spec.goldCost || playerAether < spec.aetherCost)) {
        statusBannerText = "Need " + std::to_string(spec.goldCost) + " Gold & " + std::to_string(spec.aetherCost) + " Aetherium to awaken " + spec.name + "!";
        statusBannerTimer = 3.5f;
        return false;
    }
    if (!isSandboxMode) {
        playerGold -= spec.goldCost;
        playerAether -= spec.aetherCost;
    }
    unlockedRelics[static_cast<size_t>(relicIdx)] = true;
    applyCrewMultipliers();
    playerShip.shieldHp = playerShip.maxShieldHp;
    pendingAudioEvents.push_back({SimAudioEventType::TeslaDischarge, playerShip.position, 1.3f});
    callouts.push_back({playerShip.position + Vec3{0.0f, 9.0f, 0.0f}, std::string("RELIC AWAKENED: ") + spec.name, {255, 215, 75}, 2.4f, 2.4f});
    statusBannerText = std::string("ANCIENT RELIC AWAKENED: ") + spec.name + " (" + spec.bonusDesc + ")!";
    statusBannerTimer = 5.0f;
    checkCommendationMedals();
    return true;
}

void World::applyCrewMultipliers() {
    float dialBonus = unlockedRelics[0] ? 0.22f : 0.0f;
    float heartBonus = unlockedRelics[1] ? 0.25f : 0.0f;
    float gyroThrust = unlockedRelics[2] ? 0.24f : 0.0f;
    float gyroTurn   = unlockedRelics[2] ? 0.28f : 0.0f;
    float medalPrestige = 0.03f * static_cast<float>(earnedMedalCount());
    float ocFactor = static_cast<float>(hullOverclockTier);

    playerShip.reloadMultiplier = 1.0f + 0.16f * static_cast<float>(officerRanks[0]) + dialBonus + medalPrestige + 0.10f * ocFactor;
    playerShip.repairMultiplier = 1.0f + 0.30f * static_cast<float>(officerRanks[1]) + (unlockedRelics[1] ? 0.35f : 0.0f) + medalPrestige + 0.12f * ocFactor;
    playerShip.shieldMultiplier = 1.0f + 0.25f * static_cast<float>(officerRanks[1]) + heartBonus + medalPrestige + 0.20f * ocFactor;
    playerShip.thrustMultiplier = 1.0f + 0.18f * static_cast<float>(officerRanks[3]) + gyroThrust + medalPrestige * 0.5f + 0.08f * ocFactor;
    playerShip.turnMultiplier   = 1.0f + 0.22f * static_cast<float>(officerRanks[3]) + gyroTurn + medalPrestige * 0.5f + 0.08f * ocFactor;
    playerShip.recomputeMetrics();
    playerShip.maxShieldHp += 85.0f * static_cast<float>(obelisksAttunedStat);
    playerShip.maxAfterburnerFuel += 15.0f * static_cast<float>(obelisksAttunedStat);
}

bool World::overclockPlayerShip() {
    if (hullOverclockTier >= 3) {
        statusBannerText = "Flagship Hull & Weapons are already at Maximum Orichalcum Overclock (Tier III)!";
        statusBannerTimer = 3.2f;
        return false;
    }
    int goldCost = 250 * (hullOverclockTier + 1);
    int aethCost = 10 * (hullOverclockTier + 1);
    if (!isSandboxMode && (playerGold < goldCost || playerAether < aethCost)) {
        statusBannerText = "Need " + std::to_string(goldCost) + " Gold & " + std::to_string(aethCost) + " Aetherium for Orichalcum Overclock Tier " + std::to_string(hullOverclockTier + 1) + "!";
        statusBannerTimer = 3.5f;
        return false;
    }
    if (!isSandboxMode) {
        playerGold -= goldCost;
        playerAether -= aethCost;
    }
    hullOverclockTier++;

    // Permanently reinforce all currently installed blocks by +15% structural HP
    for (int gz = Airship::minZ(); gz <= Airship::maxZ(); ++gz) {
        for (int gy = Airship::minY(); gy <= Airship::maxY(); ++gy) {
            for (int gx = Airship::minX(); gx <= Airship::maxX(); ++gx) {
                VoxelCell& cell = playerShip.getCellMut(gx, gy, gz);
                if (cell.id != BlockId::Empty) {
                    cell.maxHp *= 1.15f;
                    cell.hp = cell.maxHp;
                }
            }
        }
    }
    applyCrewMultipliers();
    playerShip.shieldHp = playerShip.maxShieldHp;
    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 1.4f});
    callouts.push_back({
        playerShip.position + Vec3{0.0f, 8.0f, 0.0f},
        "ORICHALCUM OVERCLOCK TIER " + std::to_string(hullOverclockTier) + "! (+15% HULL & +20% SHIELD)",
        {255, 215, 75},
        2.2f,
        2.2f
    });
    statusBannerText = "DRYDOCK OVERCLOCK COMPLETE: Flagship upgraded to Orichalcum Overclock Tier " + std::to_string(hullOverclockTier) + " (+15% Block HP, +20% Shield, +10% Reload)!";
    statusBannerTimer = 4.8f;
    return true;
}

bool World::refitPlayerAirframe() {
    int woodCount = 0;
    int ironCount = 0;
    for (int gz = Airship::minZ(); gz <= Airship::maxZ(); ++gz) {
        for (int gy = Airship::minY(); gy <= Airship::maxY(); ++gy) {
            for (int gx = Airship::minX(); gx <= Airship::maxX(); ++gx) {
                BlockId bid = playerShip.getCell(gx, gy, gz).id;
                if (bid == BlockId::WoodHull) woodCount++;
                else if (bid == BlockId::IronArmor) ironCount++;
            }
        }
    }

    if (woodCount > 0) {
        int goldCost = 180;
        int aethCost = 0;
        if (!isSandboxMode && playerGold < goldCost) {
            statusBannerText = "Need 180 Gold to Refit all Timber Hull blocks into Ironclad Armor!";
            statusBannerTimer = 3.2f;
            return false;
        }
        if (!isSandboxMode) playerGold -= goldCost;
        for (int gz = Airship::minZ(); gz <= Airship::maxZ(); ++gz) {
            for (int gy = Airship::minY(); gy <= Airship::maxY(); ++gy) {
                for (int gx = Airship::minX(); gx <= Airship::maxX(); ++gx) {
                    const VoxelCell& c = playerShip.getCell(gx, gy, gz);
                    if (c.id == BlockId::WoodHull) {
                        playerShip.setBlock(gx, gy, gz, BlockId::IronArmor, c.orientation);
                    }
                }
            }
        }
        applyCrewMultipliers();
        playerShip.shieldHp = playerShip.maxShieldHp;
        pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 1.3f});
        statusBannerText = "AIRFRAME REFIT COMPLETE: Upgraded " + std::to_string(woodCount) + " Timber Hull blocks to Ironclad Armor! (-" + std::to_string(goldCost + aethCost) + "G)";
        statusBannerTimer = 4.5f;
        return true;
    } else if (ironCount > 0) {
        int goldCost = 320;
        int aethCost = 12;
        if (!isSandboxMode && (playerGold < goldCost || playerAether < aethCost)) {
            statusBannerText = "Need 320 Gold & 12 Aetherium to Refit all Ironclad blocks into Orichalcum Royal Armor!";
            statusBannerTimer = 3.2f;
            return false;
        }
        if (!isSandboxMode) {
            playerGold -= goldCost;
            playerAether -= aethCost;
        }
        for (int gz = Airship::minZ(); gz <= Airship::maxZ(); ++gz) {
            for (int gy = Airship::minY(); gy <= Airship::maxY(); ++gy) {
                for (int gx = Airship::minX(); gx <= Airship::maxX(); ++gx) {
                    const VoxelCell& c = playerShip.getCell(gx, gy, gz);
                    if (c.id == BlockId::IronArmor) {
                        playerShip.setBlock(gx, gy, gz, BlockId::OrichalcumArmor, c.orientation);
                    }
                }
            }
        }
        applyCrewMultipliers();
        playerShip.shieldHp = playerShip.maxShieldHp;
        pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 1.4f});
        statusBannerText = "ROYAL AIRFRAME REFIT: Upgraded " + std::to_string(ironCount) + " Ironclad blocks to Orichalcum Royal Armor!";
        statusBannerTimer = 4.5f;
        return true;
    }

    statusBannerText = "Flagship Airframe is already 100% Royal Orichalcum Plate!";
    statusBannerTimer = 3.0f;
    return false;
}

bool World::autoStabilizePlayerShip() {
    int balloonsAdded = 0;
    int ruddersAdded = 0;

    // 1. Ensure stern GyroRudder exists if totalTurnTorque is low
    if (playerShip.totalTurnTorque < 25.0f) {
        for (int gz = 5; gz >= 2; --gz) {
            if (playerShip.getCell(0, 0, gz).id == BlockId::Empty) {
                playerShip.setBlock(0, 0, gz, BlockId::GyroRudder, 2);
                ruddersAdded++;
                break;
            }
        }
    }

    // 2. Ensure forward SteamPropeller pair exists if totalThrust is low
    if (playerShip.totalThrust < 20.0f) {
        if (playerShip.getCell(-2, 0, 4).id == BlockId::Empty) {
            playerShip.setBlock(-2, 0, 4, BlockId::SteamPropeller, 2);
        }
        if (playerShip.getCell(2, 0, 4).id == BlockId::Empty) {
            playerShip.setBlock(2, 0, 4, BlockId::SteamPropeller, 2);
        }
    }

    // 3. Add symmetrical LiftBalloons on upper deck (gy = 2..3) until Lift Ratio >= 1.30x
    auto computeLiftRatio = [this]() -> float {
        float weightForce = std::max(1.0f, playerShip.totalMass * 3.2f);
        return playerShip.totalLift / weightForce;
    };

    for (int gy = 2; gy <= 3 && computeLiftRatio() < 1.30f; ++gy) {
        for (int gz = -4; gz <= 4 && computeLiftRatio() < 1.30f; gz += 2) {
            for (int gxAbs = 0; gxAbs <= 3 && computeLiftRatio() < 1.30f; ++gxAbs) {
                if (gxAbs == 0) {
                    if (playerShip.getCell(0, gy, gz).id == BlockId::Empty) {
                        playerShip.setBlock(0, gy, gz, BlockId::LiftBalloon, 0);
                        balloonsAdded++;
                    }
                } else {
                    if (playerShip.getCell(-gxAbs, gy, gz).id == BlockId::Empty) {
                        playerShip.setBlock(-gxAbs, gy, gz, BlockId::LiftBalloon, 0);
                        balloonsAdded++;
                    }
                    if (playerShip.getCell(gxAbs, gy, gz).id == BlockId::Empty) {
                        playerShip.setBlock(gxAbs, gy, gz, BlockId::LiftBalloon, 0);
                        balloonsAdded++;
                    }
                }
            }
        }
    }

    // 4. Trim longitudinal pitch if Center of Lift is significantly offset from Center of Mass
    float zDelta = playerShip.centerOfLiftLocal.z - playerShip.centerOfMassLocal.z;
    if (std::abs(zDelta) > 0.85f) {
        int trimZ = (zDelta > 0.0f) ? -4 : 4;
        for (int gy = 2; gy <= 3; ++gy) {
            if (playerShip.getCell(0, gy, trimZ).id == BlockId::Empty) {
                playerShip.setBlock(0, gy, trimZ, BlockId::LiftBalloon, 0);
                balloonsAdded++;
                break;
            }
        }
    }

    applyCrewMultipliers();
    playerShip.shieldHp = playerShip.maxShieldHp;
    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 1.25f});
    int newLiftPct = static_cast<int>(std::round(computeLiftRatio() * 100.0f));
    statusBannerText = "NAVAL ARCHITECT AUTO-STABILIZER COMPLETE: Installed " + std::to_string(balloonsAdded) +
                       " Gas Cells & " + std::to_string(ruddersAdded) + " Gyro-Rudders (Buoyancy Equilibrium: " +
                       std::to_string(newLiftPct) + "%)!";
    statusBannerTimer = 4.5f;
    return (balloonsAdded > 0 || ruddersAdded > 0 || newLiftPct >= 100);
}

bool World::upgradeOfficer(int officerIdx) {
    if (officerIdx < 0 || officerIdx >= 5) {
        return false;
    }
    int currentRank = officerRanks[static_cast<size_t>(officerIdx)];
    if (currentRank >= 3) {
        statusBannerText = std::string(kOfficerSpecs[static_cast<size_t>(officerIdx)].name) + " is already at Maximum Rank (III)!";
        statusBannerTimer = 3.0f;
        return false;
    }

    const OfficerSpec& spec = kOfficerSpecs[static_cast<size_t>(officerIdx)];
    int goldCost = spec.baseGoldCost * (currentRank + 1);
    int aetherCost = spec.baseAetherCost * (currentRank + 1);

    if (!isSandboxMode && (playerGold < goldCost || playerAether < aetherCost)) {
        statusBannerText = "Insufficient Resources! Need " + std::to_string(goldCost) + " Gold & " + std::to_string(aetherCost) + " Aetherium.";
        statusBannerTimer = 3.5f;
        return false;
    }

    if (!isSandboxMode) {
        playerGold -= goldCost;
        playerAether -= aetherCost;
    }
    officerRanks[static_cast<size_t>(officerIdx)] = currentRank + 1;
    applyCrewMultipliers();
    playerShip.shieldHp = playerShip.maxShieldHp;

    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 1.2f});
    statusBannerText = std::string("Promoted ") + spec.name + " to Rank " + std::to_string(currentRank + 1) + "! (" + spec.bonusDesc + ")";
    statusBannerTimer = 4.5f;
    checkCommendationMedals();
    return true;
}

bool World::triggerDamageControl() {
    if (damageControlCooldown > 0.0f) {
        statusBannerText = "Emergency Damage Control recharging (" + std::to_string(static_cast<int>( std::ceil(damageControlCooldown))) + "s)...";
        statusBannerTimer = 2.0f;
        return false;
    }
    float capFactor = std::max(0.40f, 1.0f - 0.12f * static_cast<float>(playerShip.capacitorCount) - (unlockedRelics[0] ? 0.20f : 0.0f));
    damageControlCooldown = 16.0f * capFactor;
    float healAmt = 220.0f * playerShip.repairMultiplier;
    playerShip.triggerEmergencyRepair(healAmt);
    playerShip.shieldHp = std::min(playerShip.maxShieldHp, playerShip.shieldHp + 180.0f * playerShip.shieldMultiplier);

    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 1.4f});
    callouts.push_back({playerShip.position + Vec3{0.0f, 8.0f, 0.0f}, "EMERGENCY DAMAGE CONTROL! +HULL & +SHIELD!", {85, 255, 175}, 2.2f, 2.2f});
    statusBannerText = "Emergency Damage Control Activated! Hull Reconstructed, Fires Doused & Shield Overcharged!";
    statusBannerTimer = 4.0f;
    return true;
}

bool World::triggerSonarPing() {
    if (sonarPingCooldown > 0.0f) {
        statusBannerText = "Acoustic Aether-Sonar recharging (" + std::to_string(static_cast<int>(std::ceil(sonarPingCooldown))) + "s)...";
        statusBannerTimer = 2.0f;
        return false;
    }
    float capFactor = std::max(0.40f, 1.0f - 0.12f * static_cast<float>(playerShip.capacitorCount) - (unlockedRelics[0] ? 0.20f : 0.0f));
    sonarPingCooldown = 14.0f * capFactor;
    sonarMarkTimer = 10.0f;
    sonarWaveRadius = 8.0f;
    sonarWaveOrigin = playerShip.position;

    // Trigger Boss Puzzle Counterplay on Azure Sky-Whale, Obsidian Dread-Manta, or tethered Maelstrom Sovereign!
    int disruptedCount = 0;
    for (auto& lev : leviathans) {
        if (lev.isSlain || lev.segments.empty()) continue;
        float dist = (lev.segments[0].position - playerShip.position).length();
        if (dist <= 520.0f) {
            if (lev.species() == LeviathanSpecies::AzureSkyWhale ||
                lev.species() == LeviathanSpecies::ObsidianDreadManta ||
                (lev.species() == LeviathanSpecies::MaelstromSovereign && lev.aiState == LeviathanState::TetherStruggle)) {
                if (lev.triggerCounterplayDisruption("ACOUSTIC SONAR RESONANCE")) {
                    disruptedCount++;
                    callouts.push_back({
                        lev.segments[0].position + Vec3{0.0f, 14.0f, 0.0f},
                        lev.signatureMechanicName() + " SHATTERED! CORE EXPOSED (8.5s)!",
                        {85, 255, 215},
                        2.8f,
                        2.8f
                    });
                }
            }
        }
    }

    // Also reveal any nearby Sky-Wonders or Lore Logs within 340m!
    for (auto& w : skyWonders) {
        if (!w.discovered && (w.position - playerShip.position).length() <= 340.0f) {
            w.discovered = true;
            playerGold += 180;
            playerAether += 8;
            callouts.push_back({w.position + Vec3{0.0f, 16.0f, 0.0f}, "WONDER CHARTED: " + w.name + "! (+180G +8Ae)", {255, 230, 95}, 3.0f, 3.0f});
        }
    }
    for (auto& log : loreLogs) {
        if (!log.recovered && (log.position - playerShip.position).length() <= 340.0f) {
            log.recovered = true;
            playerGold += 90;
            playerAether += 4;
            callouts.push_back({log.position + Vec3{0.0f, 10.0f, 0.0f}, "LORE LOG RECOVERED: " + log.title + " (+90G +4Ae)", {135, 240, 255}, 2.8f, 2.8f});
        }
    }

    pendingAudioEvents.push_back({SimAudioEventType::TeslaDischarge, playerShip.position, 1.15f});
    callouts.push_back({playerShip.position + Vec3{0.0f, 9.0f, 0.0f}, "AETHER SONAR PING! (+25% TARGET DAMAGE)", {95, 235, 255}, 2.2f, 2.2f});
    if (disruptedCount > 0) {
        statusBannerText = "ACOUSTIC SONAR COUNTERPLAY! Shattered Elder Leviathan Barrier — Weak Cores Exposed for 8.5s!";
    } else {
        statusBannerText = "Acoustic Aether-Sonar Emitted! All Leviathan Weak Cores Exposed (+25% Crit Damage for 10s)!";
    }
    statusBannerTimer = 4.5f;
    return true;
}

bool World::deploySkyMine() {
    if (skyMineCooldown > 0.0f) {
        statusBannerText = "Magnetic Sky-Mine Chute reloading (" + std::to_string(static_cast<int>(std::ceil(skyMineCooldown))) + "s)...";
        statusBannerTimer = 2.0f;
        return false;
    }
    float capFactor = std::max(0.40f, 1.0f - 0.10f * static_cast<float>(playerShip.capacitorCount) - (unlockedRelics[0] ? 0.20f : 0.0f));
    skyMineCooldown = 3.2f * capFactor;

    Vec3 sternDir = -playerShip.orientationQuat().forward();
    Vec3 dropPos = playerShip.position + sternDir * (playerShip.boundingRadius + 5.5f);

    SkyMine3D mine;
    mine.id = nextEntityId_++;
    mine.ownerShipId = playerShip.id();
    mine.ownerFaction = playerShip.faction();
    mine.position = dropPos;
    mine.velocity = sternDir * 8.0f;
    mine.triggerRadius = 48.0f;
    mine.blastRadius = 15.0f;
    mine.damage = 260.0f * (1.0f + 0.10f * static_cast<float>(officerRanks[0]) + 0.03f * static_cast<float>(earnedMedalCount()));
    mine.armTimer = 0.45f;
    mine.lifetime = 45.0f;
    mine.pulsePhase = 0.0f;
    mine.active = true;
    skyMines.push_back(mine);

    pendingAudioEvents.push_back({SimAudioEventType::HarpoonLaunch, dropPos, 0.95f});
    callouts.push_back({dropPos + Vec3{0.0f, 4.0f, 0.0f}, "MAGNETIC SKY-MINE DEPLOYED!", {255, 165, 65}, 1.5f, 1.5f});
    statusBannerText = "Deployed Magnetic Aether Sky-Mine [G] in Stern Wake! (260 AoE Blast Damage)";
    statusBannerTimer = 3.2f;
    return true;
}

bool World::triggerEmpNova() {
    if (empNovaCooldown > 0.0f) {
        statusBannerText = "Aether-EMP Shockwave Nova recharging (" + std::to_string(static_cast<int>(std::ceil(empNovaCooldown))) + "s)...";
        statusBannerTimer = 2.0f;
        return false;
    }
    float capFactor = std::max(0.40f, 1.0f - 0.12f * static_cast<float>(playerShip.capacitorCount) - (unlockedRelics[0] ? 0.20f : 0.0f));
    empNovaCooldown = 20.0f * capFactor;
    empWaveRadius = 8.0f;
    empWaveOrigin = playerShip.position;
    empNovasTriggeredStat++;

    const float kEmpRadius = 195.0f;
    float dmgMult = 1.0f + 0.10f * static_cast<float>(officerRanks[0]) + 0.03f * static_cast<float>(earnedMedalCount());
    float empDmg = 165.0f * dmgMult;
    float navRewardMult = 1.0f + 0.25f * static_cast<float>(officerRanks[4]) + (unlockedRelics[4] ? 0.25f : 0.0f);
    int hits = 0;

    // 1. Strip shields, slow velocity & damage all hostile NPC Airships within 195m
    for (auto& npc : npcShips) {
        if (npc.isDestroyed || npc.faction() == playerShip.faction()) continue;
        float d = (npc.position - playerShip.position).length();
        if (d <= kEmpRadius) {
            hits++;
            npc.shieldHp = 0.0f;
            npc.shieldRechargeDelay = 6.0f;
            npc.velocity *= 0.42f;
            int broken = npc.applyDamageAtWorldPos(npc.position, empDmg, 8.5f, true, debris);
            blocksDestroyedStat += broken;
            explosions.push_back({npc.position, 6.5f, {215, 115, 255}, false, true, playerShip.position});
            callouts.push_back({npc.position + Vec3{0.0f, 6.5f, 0.0f}, "EMP NOVA! SHIELD STRIPPED -" + std::to_string(static_cast<int>(empDmg)), {215, 125, 255}, 1.8f, 1.8f});
            if (npc.isDestroyed && npc.faction() == 1) {
                piratesSunk++;
                int bountyG = static_cast<int>(120.0f * navRewardMult);
                playerGold += bountyG;
                crates.push_back({nextEntityId_++, npc.position, {0.0f, 3.0f, 0.0f}, 95, 5, 60.0f, false});
            }
        }
    }

    // 2. Deal armor-piercing resonance damage & trigger EMP Counterplay on Colossal Sky-Leviathans within 225m
    for (auto& lev : leviathans) {
        if (lev.isSlain) continue;
        for (size_t s = 0; s < lev.segments.size(); ++s) {
            float d = (lev.segments[s].position - playerShip.position).length();
            if (d <= 225.0f) {
                hits++;
                if (lev.species() == LeviathanSpecies::CrimsonStormWyrm ||
                    lev.species() == LeviathanSpecies::SolarPhoenixSerpent ||
                    lev.species() == LeviathanSpecies::MaelstromSovereign) {
                    if (lev.triggerCounterplayDisruption("EMP NOVA OVERLOAD")) {
                        callouts.push_back({
                            lev.segments[0].position + Vec3{0.0f, 14.0f, 0.0f},
                            lev.signatureMechanicName() + " COLLAPSED! STAGGERED (8.5s)!",
                            {255, 215, 75},
                            2.8f,
                            2.8f
                        });
                    }
                }
                lev.damageSegment(static_cast<int>(s), empDmg * 1.15f, true);
                explosions.push_back({lev.segments[s].position, 7.0f, {215, 115, 255}, false, true, playerShip.position});
                callouts.push_back({lev.segments[s].position + Vec3{0.0f, 7.0f, 0.0f}, "EMP RESONANCE! -" + std::to_string(static_cast<int>(empDmg * 1.15f)), {215, 125, 255}, 1.8f, 1.8f});
                if (lev.isSlain) {
                    leviathansSlain++;
                    int goldReward = static_cast<int>(static_cast<float>(lev.goldBounty) * navRewardMult * getRiskBountyMultiplier());
                    int aethReward = static_cast<int>(static_cast<float>(lev.aetherBounty) * navRewardMult * getRiskBountyMultiplier());
                    playerGold += goldReward;
                    playerAether += aethReward;
                    playerCargo[3] = std::min(maxCargoCapacity(), playerCargo[3] + 2);
                    for (auto& c : contracts) {
                        if (c.targetName == lev.name()) c.completed = true;
                    }
                    if (lev.species() == LeviathanSpecies::MaelstromSovereign) {
                        victoryAchieved = true;
                    }
                }
                break; // Hit once per Leviathan per Nova
            }
        }
    }

    // 3. Vaporize incoming hostile projectiles within 195m
    for (auto& p : projectiles) {
        if (!p.active || p.ownerFaction == playerShip.faction()) continue;
        if ((p.position - playerShip.position).length() <= kEmpRadius) {
            p.active = false;
            explosions.push_back({p.position, 2.5f, {145, 235, 255}, false, false, playerShip.position});
        }
    }

    // 4. Shatter & harvest Star-Aether Meteoroids within 165m
    for (auto& met : meteors) {
        if (!met.active) continue;
        if ((met.position - playerShip.position).length() <= 165.0f) {
            met.active = false;
            meteorsHarvestedStat++;
            int aethGain = static_cast<int>(std::ceil(static_cast<float>(met.aetherYield) * navRewardMult));
            int goldGain = static_cast<int>(std::ceil(static_cast<float>(met.goldYield) * navRewardMult));
            playerAether += aethGain;
            playerGold += goldGain;
            explosions.push_back({met.position, 7.5f, {215, 115, 255}, false, true, playerShip.position});
        }
    }

    cameraTrauma = std::min(1.0f, cameraTrauma + 0.38f);
    pendingAudioEvents.push_back({SimAudioEventType::TeslaDischarge, playerShip.position, 1.5f});
    callouts.push_back({playerShip.position + Vec3{0.0f, 10.0f, 0.0f}, "AETHER-EMP SHOCKWAVE NOVA! (" + std::to_string(hits) + " TARGETS)", {225, 135, 255}, 2.2f, 2.2f});
    statusBannerText = "AETHER-EMP SHOCKWAVE NOVA [4] UNLEASHED! Enemy Shields Stripped, Projectiles Vaporized & Leviathans Stunned!";
    statusBannerTimer = 4.6f;
    return true;
}

void World::cycleFleetOrder() {
    int next = (static_cast<int>(currentFleetOrder) + 1) % 4;
    currentFleetOrder = static_cast<FleetOrder>(next);
    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 0.9f});
    statusBannerText = std::string("FLEET SQUADRON ORDER [Z]: ") + getFleetOrderName(currentFleetOrder);
    statusBannerTimer = 3.8f;
}

const char* World::getFleetOrderName(FleetOrder order) {
    switch (order) {
        case FleetOrder::EscortFormation:    return "Close Escort Formation";
        case FleetOrder::FocusMyTarget:      return "Focus Flagship Target";
        case FleetOrder::FreeHunt:           return "Free Hunt & Harpoon Assault";
        case FleetOrder::DefensivePerimeter: return "Defensive Perimeter Screen";
    }
    return "Escort Formation";
}

void World::cycleWeatherState() {
    int next = (static_cast<int>(currentWeather) + 1) % 4;
    currentWeather = static_cast<WeatherState>(next);
    statusBannerText = std::string("SKY WEATHER SYSTEM: ") + getWeatherName(currentWeather);
    statusBannerTimer = 3.5f;
}

const char* World::getWeatherName(WeatherState w) {
    switch (w) {
        case WeatherState::ClearTradeWinds:  return "Clear Trade Winds";
        case WeatherState::CirrusMist:       return "High-Altitude Cirrus Mist";
        case WeatherState::IonAetherSquall:  return "Ion Aether-Squall";
        case WeatherState::SupercellTempest: return "Supercell Lightning Tempest";
    }
    return "Clear Trade Winds";
}

void World::cyclePlayerLivery() {
    playerShip.cycleHullLivery();
    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 0.8f});
    statusBannerText = std::string("FLEET HULL LIVERY [P]: ") + Airship::getHullLiveryName(playerShip.hullLiveryId);
    statusBannerTimer = 3.5f;
}

int World::totalCargoUnits() const {
    int sum = 0;
    for (int q : playerCargo) sum += q;
    return sum;
}

int World::maxCargoCapacity() const {
    return 30 + (playerShip.occupiedBlockCount / 10) + officerRanks[4] * 12 + (unlockedRelics[4] ? 20 : 0);
}

bool World::buyCommodity(int commodityIdx, int qty) {
    if (commodityIdx < 0 || commodityIdx >= kCommodityCount || qty <= 0) {
        return false;
    }
    float dist = 1e30f;
    const SkyIsland* port = archipelago.findNearestPort(playerShip.position, dist);
    if (!port) {
        return false;
    }
    if (totalCargoUnits() + qty > maxCargoCapacity()) {
        statusBannerText = "Cargo Hold Full! Upgrade Ship Hull or Promote Scholar-Navigator Lyra [K] for +Capacity.";
        statusBannerTimer = 3.5f;
        return false;
    }
    float libDiscount = port->isLiberated ? 0.80f : 1.0f;
    int unitPrice = std::max(5, static_cast<int>(std::round(static_cast<float>(port->marketPrices[static_cast<size_t>(commodityIdx)]) * libDiscount)));
    int totalCost = unitPrice * qty;
    if (!isSandboxMode && playerGold < totalCost) {
        statusBannerText = "Insufficient Gold to purchase " + std::string(Archipelago::getCommoditySpecs()[static_cast<size_t>(commodityIdx)].name) + "!";
        statusBannerTimer = 3.0f;
        return false;
    }
    if (!isSandboxMode) {
        playerGold -= totalCost;
    }
    playerCargo[static_cast<size_t>(commodityIdx)] += qty;
    pendingAudioEvents.push_back({SimAudioEventType::CratePickup, playerShip.position, 1.0f});
    statusBannerText = "Purchased " + std::to_string(qty) + "x " +
                       Archipelago::getCommoditySpecs()[static_cast<size_t>(commodityIdx)].name +
                       " at " + port->name + " (-" + std::to_string(totalCost) + "G)";
    statusBannerTimer = 3.5f;
    return true;
}

bool World::sellCommodity(int commodityIdx, int qty) {
    if (commodityIdx < 0 || commodityIdx >= kCommodityCount || qty <= 0) {
        return false;
    }
    int held = playerCargo[static_cast<size_t>(commodityIdx)];
    if (held <= 0) {
        return false;
    }
    int sellQty = std::min(held, qty);
    float dist = 1e30f;
    const SkyIsland* port = archipelago.findNearestPort(playerShip.position, dist);
    if (!port) {
        return false;
    }
    float navBonus = 1.0f + 0.12f * static_cast<float>(officerRanks[4]) + (unlockedRelics[4] ? 0.25f : 0.0f);
    int unitPrice = static_cast<int>(std::round(static_cast<float>(port->marketPrices[static_cast<size_t>(commodityIdx)]) * navBonus));
    int revenue = unitPrice * sellQty;
    playerCargo[static_cast<size_t>(commodityIdx)] -= sellQty;
    playerGold += revenue;
    tradeProfitStat += revenue;
    pendingAudioEvents.push_back({SimAudioEventType::CratePickup, playerShip.position, 1.1f});
    statusBannerText = "Sold " + std::to_string(sellQty) + "x " +
                       Archipelago::getCommoditySpecs()[static_cast<size_t>(commodityIdx)].name +
                       " at " + port->name + " (+" + std::to_string(revenue) + " Gold!)";
    statusBannerTimer = 3.5f;
    return true;
}

void World::summonApexLeviathan() {
    LeviathanSpecies sp = static_cast<LeviathanSpecies>((leviathansSlain + prestigeHuntTier) % 6);
    Vec3 fwd = playerShip.orientationQuat().forward();
    Vec3 spawnPos = playerShip.position + fwd * 260.0f + Vec3{0.0f, 18.0f, 0.0f};
    spawnPos.y = std::clamp(spawnPos.y, 115.0f, 175.0f);

    int id = nextEntityId_++;
    leviathans.emplace_back(id, sp, spawnPos, rng_.nextU32());
    Leviathan& lev = leviathans.back();
    float scaleMult = 1.0f + 0.20f * static_cast<float>(prestigeHuntTier - 1);
    lev.maxTotalHp *= scaleMult;
    lev.totalHp = lev.maxTotalHp;
    lev.goldBounty = static_cast<int>(static_cast<float>(lev.goldBounty) * scaleMult);
    lev.aetherBounty = static_cast<int>(static_cast<float>(lev.aetherBounty) * scaleMult);
    prestigeHuntTier++;

    pendingAudioEvents.push_back({SimAudioEventType::LeviathanRoar, spawnPos, 1.5f});
    statusBannerText = "APEX ELDER LEVIATHAN SUMMONED: " + lev.name() + " (" + lev.title() + ") breached the clouds ahead!";
    statusBannerTimer = 5.5f;
}

void World::spawnStarMeteor(const Vec3& pos, const Vec3& vel) {
    AetherMeteor3D met;
    met.id = nextEntityId_++;
    met.position = pos;
    met.velocity = vel;
    met.radius = 5.5f;
    met.maxHp = 110.0f;
    met.hp = 110.0f;
    met.aetherYield = 14;
    met.goldYield = 160;
    met.spinAngle = 0.0f;
    met.active = true;
    meteors.push_back(met);
}

void World::spawnSurvivorPod(const Vec3& pos) {
    SkySurvivorPod pod;
    pod.id = nextEntityId_++;
    pod.position = pos;
    pod.velocity = {0.0f, -0.5f, 0.0f};
    pod.goldReward = 140;
    pod.aetherReward = 5;
    pod.flarePhase = 0.0f;
    pod.rescued = false;
    survivorPods.push_back(pod);
}

bool World::fastTravelToIsland(int islandIdx) {
    if (islandIdx < 0 || islandIdx >= static_cast<int>(archipelago.islands.size())) {
        return false;
    }
    SkyIsland& isl = archipelago.islands[static_cast<size_t>(islandIdx)];
    if (!isSandboxMode && !isl.hasDrydockPort && !isl.isLiberated) {
        statusBannerText = "Cannot Aether-Jump to " + isl.name + " until its Sky-Citadel is Liberated or it has an Allied Port!";
        statusBannerTimer = 3.8f;
        return false;
    }
    int jumpCost = (isSandboxMode || unlockedRelics[2]) ? 0 : 2;
    if (playerAether < jumpCost) {
        statusBannerText = "Need 2 Aetherium to activate Sky-Beacon Aether-Jump (Free with Orichalcum Gyro-Compass Relic)!";
        statusBannerTimer = 3.5f;
        return false;
    }
    playerAether -= jumpCost;
    isl.discovered = true;
    releaseAllPlayerHarpoons();

    Vec3 moorPos = isl.position + Vec3{0.0f, 24.0f, isl.radius + 26.0f};
    playerShip.position = moorPos;
    playerShip.velocity = {0.0f, 0.0f, 0.0f};
    playerShip.targetCruiseAltitude = moorPos.y;
    playerShip.shieldHp = playerShip.maxShieldHp;

    // Teleport allied wingmen alongside player flagship
    int allyIdx = 0;
    for (auto& npc : npcShips) {
        if (!npc.isDestroyed && npc.faction() == 0) {
            allyIdx++;
            float side = (allyIdx % 2 == 0) ? 1.0f : -1.0f;
            npc.position = moorPos + Vec3{side * 36.0f * static_cast<float>((allyIdx + 1) / 2), 2.0f, 18.0f};
            npc.velocity = {0.0f, 0.0f, 0.0f};
            npc.targetCruiseAltitude = npc.position.y;
        }
    }

    pendingAudioEvents.push_back({SimAudioEventType::TeslaDischarge, moorPos, 1.3f});
    callouts.push_back({moorPos + Vec3{0.0f, 9.0f, 0.0f}, "AETHER-BEACON JUMP: " + isl.name, {85, 245, 255}, 2.2f, 2.2f});
    statusBannerText = "AETHER-BEACON FAST TRAVEL COMPLETE: Moored off " + isl.name + "!";
    statusBannerTimer = 4.2f;
    return true;
}

bool World::saveCampaign(const std::string& filepath) {
    if (!ShipBlueprints::saveCustomBlueprint(playerShip, filepath + ".ship.aeth")) {
        return false;
    }
    std::ofstream out(filepath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << "AETHERNAUT_CAMPAIGN_V1\n";
    out << "GOLD " << playerGold << "\n";
    out << "AETHER " << playerAether << "\n";
    out << "SLAIN " << leviathansSlain << "\n";
    out << "PIRATES " << piratesSunk << "\n";
    out << "CITADELS " << citadelsLiberatedStat << "\n";
    out << "PRESTIGE " << prestigeHuntTier << "\n";
    out << "ATMOS " << skyAtmospherePreset << "\n";
    out << "WEATHER " << static_cast<int>(currentWeather) << "\n";
    out << "ORDER " << static_cast<int>(currentFleetOrder) << "\n";
    out << "OFFICERS "
        << officerRanks[0] << " "
        << officerRanks[1] << " "
        << officerRanks[2] << " "
        << officerRanks[3] << " "
        << officerRanks[4] << "\n";
    out << "CARGO "
        << playerCargo[0] << " "
        << playerCargo[1] << " "
        << playerCargo[2] << " "
        << playerCargo[3] << "\n";
    out << "RELICS "
        << (unlockedRelics[0] ? 1 : 0) << " "
        << (unlockedRelics[1] ? 1 : 0) << " "
        << (unlockedRelics[2] ? 1 : 0) << " "
        << (unlockedRelics[3] ? 1 : 0) << " "
        << (unlockedRelics[4] ? 1 : 0) << "\n";
    out << "MEDALS";
    for (int m = 0; m < kMedalCount; ++m) {
        out << " " << (earnedMedals[static_cast<size_t>(m)] ? 1 : 0);
    }
    out << "\n";
    out << "STATS " << slipstreamRingsPassedStat << " " << ramImpactsStat << " " << skyMineHitsStat << " " << tradeProfitStat << " " << meteorsHarvestedStat << " " << convoysEscortedStat << " " << hullOverclockTier << " " << dreadnoughtsSunkStat << " " << survivorsRescuedStat << " " << empNovasTriggeredStat << " " << projectilesInterceptedStat << " " << obelisksAttunedStat << "\n";
    out << "POS " << playerShip.position.x << " " << playerShip.position.y << " " << playerShip.position.z << "\n";
    statusBannerText = "Saved Campaign Progress, Relics, Medals, Cargo Hold & Flagship Blueprint to Disk!";
    statusBannerTimer = 4.0f;
    return out.good();
}

bool World::loadCampaign(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        return false;
    }
    std::string header;
    if (!(in >> header) || header != "AETHERNAUT_CAMPAIGN_V1") {
        return false;
    }
    std::string tag;
    while (in >> tag) {
        if (tag == "GOLD") in >> playerGold;
        else if (tag == "AETHER") in >> playerAether;
        else if (tag == "SLAIN") in >> leviathansSlain;
        else if (tag == "PIRATES") in >> piratesSunk;
        else if (tag == "CITADELS") in >> citadelsLiberatedStat;
        else if (tag == "PRESTIGE") in >> prestigeHuntTier;
        else if (tag == "ATMOS") in >> skyAtmospherePreset;
        else if (tag == "WEATHER") {
            int w = 0; in >> w;
            currentWeather = static_cast<WeatherState>(std::clamp(w, 0, 3));
        } else if (tag == "ORDER") {
            int o = 0; in >> o;
            currentFleetOrder = static_cast<FleetOrder>(std::clamp(o, 0, 3));
        } else if (tag == "OFFICERS") {
            in >> officerRanks[0] >> officerRanks[1] >> officerRanks[2] >> officerRanks[3] >> officerRanks[4];
        } else if (tag == "CARGO") {
            in >> playerCargo[0] >> playerCargo[1] >> playerCargo[2] >> playerCargo[3];
        } else if (tag == "RELICS") {
            int r0 = 0, r1 = 0, r2 = 0, r3 = 0, r4 = 0;
            in >> r0 >> r1 >> r2 >> r3 >> r4;
            unlockedRelics = {r0 != 0, r1 != 0, r2 != 0, r3 != 0, r4 != 0};
        } else if (tag == "MEDALS") {
            for (int m = 0; m < kMedalCount; ++m) {
                int mv = 0; in >> mv;
                earnedMedals[static_cast<size_t>(m)] = (mv != 0);
            }
        } else if (tag == "STATS") {
            in >> slipstreamRingsPassedStat >> ramImpactsStat >> skyMineHitsStat >> tradeProfitStat >> meteorsHarvestedStat >> convoysEscortedStat >> hullOverclockTier >> dreadnoughtsSunkStat >> survivorsRescuedStat >> empNovasTriggeredStat >> projectilesInterceptedStat >> obelisksAttunedStat;
        } else if (tag == "POS") {
            in >> playerShip.position.x >> playerShip.position.y >> playerShip.position.z;
            playerShip.targetCruiseAltitude = playerShip.position.y;
        }
    }
    for (int b = 0; b < static_cast<int>(archipelago.obelisks.size()); ++b) {
        archipelago.obelisks[static_cast<size_t>(b)].attuned = (b < obelisksAttunedStat);
    }
    ShipBlueprints::loadCustomBlueprint(playerShip, filepath + ".ship.aeth");
    applyCrewMultipliers();
    playerShip.shieldHp = playerShip.maxShieldHp;
    statusBannerText = "Loaded Campaign Save, Relics, Medals, Cargo Hold & Flagship from Disk!";
    statusBannerTimer = 4.0f;
    return true;
}

void World::initWorld(uint64_t seed, int playerBlueprintIdx, bool sandboxMode) {
    rng_.seedState(seed, 0xCAFEBABE12345678ULL);
    isSandboxMode = sandboxMode;
    nextEntityId_ = 100;

    npcShips.clear();
    leviathans.clear();
    projectiles.clear();
    skyMines.clear();
    meteors.clear();
    survivorPods.clear();
    tethers.clear();
    debris.clear();
    crates.clear();
    explosions.clear();
    callouts.clear();
    pendingAudioEvents.clear();
    contracts.clear();
    cameraTrauma = 0.0f;
    damageControlCooldown = 0.0f;
    sonarPingCooldown = 0.0f;
    skyMineCooldown = 0.0f;
    empNovaCooldown = 0.0f;
    pointDefenseCooldown = 0.0f;
    ramCooldownTimer = 0.0f;
    meteorSpawnTimer = 28.0f;
    sonarMarkTimer = 0.0f;
    sonarWaveRadius = 0.0f;
    empWaveRadius = 0.0f;
    officerRanks = {0, 0, 0, 0, 0};
    playerCargo = {4, 2, 0, 0};
    unlockedRelics = {false, false, false, false, false};
    earnedMedals.fill(false);
    citadelsLiberatedStat = 0;
    tradeProfitStat = 0;
    slipstreamRingsPassedStat = 0;
    slipstreamComboCount = 0;
    slipstreamComboTimer = 0.0f;
    ramImpactsStat = 0;
    skyMineHitsStat = 0;
    meteorsHarvestedStat = 0;
    convoysEscortedStat = 0;
    convoyShipId = -1;
    convoyTargetIslandIdx = 1;
    convoyRespawnTimer = 0.0f;
    hullOverclockTier = 0;
    dreadnoughtsSunkStat = 0;
    dreadArkShipId = -1;
    survivorsRescuedStat = 0;
    empNovasTriggeredStat = 0;
    projectilesInterceptedStat = 0;
    obelisksAttunedStat = 0;
    prestigeHuntTier = 1;
    currentFleetOrder = FleetOrder::EscortFormation;
    currentWeather = WeatherState::ClearTradeWinds;

    archipelago.generate(seed);

    // Initialize Player Flagship moored off New Horizon Drydock's sky-pier approach
    ShipBlueprints::applyBlueprint(playerShip, playerBlueprintIdx);
    applyCrewMultipliers();
    playerShip.position = {0.0f, 138.0f, -62.0f};
    playerShip.velocity = {0.0f, 0.0f, -6.0f};
    playerShip.yaw = 0.0f;
    playerShip.pitch = 0.0f;
    playerShip.roll = 0.0f;
    playerShip.targetCruiseAltitude = 138.0f;
    playerShip.shieldHp = playerShip.maxShieldHp;
    playerShip.afterburnerFuel = playerShip.maxAfterburnerFuel;
    playerShip.isDestroyed = false;
    playerShip.isSinking = false;

    playerGold = sandboxMode ? 99999 : 480;
    playerAether = sandboxMode ? 999 : 25;
    leviathansSlain = 0;
    piratesSunk = 0;
    isPlayerWalkingOnDeck = false;

    // Spawn 6 Colossal Sky-Leviathans across the 3D Sky Archipelago
    spawnLeviathanEncounter(LeviathanSpecies::AzureSkyWhale, {165.0f, 142.0f, -310.0f});
    spawnLeviathanEncounter(LeviathanSpecies::CrimsonStormWyrm, {-280.0f, 150.0f, -380.0f});
    spawnLeviathanEncounter(LeviathanSpecies::ObsidianDreadManta, {480.0f, 125.0f, 280.0f});
    spawnLeviathanEncounter(LeviathanSpecies::MaelstromSovereign, {0.0f, 155.0f, -1080.0f});
    spawnLeviathanEncounter(LeviathanSpecies::GlacialCryoKraken, {240.0f, 162.0f, -720.0f});
    spawnLeviathanEncounter(LeviathanSpecies::SolarPhoenixSerpent, {-420.0f, 148.0f, 310.0f});

    // Spawn Pirate Corsair Patrols, Allied Wingman & Imperial Treasure Galleon Convoy
    spawnPirateSquadron(2, {-210.0f, 132.0f, -260.0f});
    spawnPirateSquadron(2, {340.0f, 130.0f, 160.0f});
    spawnAlliedWingman(0, {-38.0f, 136.0f, -142.0f});
    spawnImperialConvoy();

    // Spawn 3 Celestial Star-Aether Meteoroids drifting across the upper sky-archipelago
    spawnStarMeteor({-55.0f, 158.0f, -225.0f}, {2.2f, -1.8f, -1.5f});
    spawnStarMeteor({185.0f, 166.0f, -285.0f}, {-2.5f, -1.6f, 1.2f});
    spawnStarMeteor({-235.0f, 162.0f, -340.0f}, {1.8f, -1.5f, 2.0f});

    // Spawn 4 Distress Beacon Sky-Survivor Lifeboat Pods across the Archipelago
    spawnSurvivorPod({92.0f, 136.0f, -195.0f});
    spawnSurvivorPod({-145.0f, 140.0f, -210.0f});
    spawnSurvivorPod({240.0f, 138.0f, -45.0f});
    spawnSurvivorPod({-85.0f, 142.0f, 165.0f});

    // Populate 6 Guild Bounty Contracts
    contracts.push_back({1, "Hunt the Elder Sky-Whale", "Cetus Aetheria", {165.0f, 142.0f, -310.0f}, 450, 16, false});
    contracts.push_back({2, "Slay the Crimson Storm-Wyrm", "Draconis Fulmen", {-280.0f, 150.0f, -380.0f}, 700, 26, false});
    contracts.push_back({3, "Purge the Obsidian Dread-Manta", "Raja Vorago", {480.0f, 125.0f, 280.0f}, 950, 35, false});
    contracts.push_back({4, "Shatter the Glacial Cryo-Kraken", "Architeuthis Borealis", {240.0f, 162.0f, -720.0f}, 1250, 45, false});
    contracts.push_back({5, "Extinguish the Solar Phoenix-Serpent", "Ignis Quetzal", {-420.0f, 148.0f, 310.0f}, 1500, 55, false});
    contracts.push_back({6, "Conquer the Maelstrom Sovereign", "Apex Charybdis", {0.0f, 155.0f, -1080.0f}, 1800, 65, false});

    voyageElapsedSeconds = 0.0f;
    victoryAchieved = false;
    playerSearchlightOn = true;
    initSkyEcosystemAndWonders();
}

void World::spawnImperialConvoy() {
    int id = nextEntityId_++;
    Airship convoy(id, "Imperial Treasure Galleon", 0);
    ShipBlueprints::applyBlueprint(convoy, 4); // Merchant Cargo-Galleon
    convoy.hullLiveryId = 3; // Solaris Gilded Sovereign
    Vec3 startPos = !archipelago.islands.empty()
        ? (archipelago.islands[0].position + Vec3{48.0f, 38.0f, -55.0f})
        : Vec3{48.0f, 138.0f, -155.0f};
    convoy.position = startPos;
    convoy.targetCruiseAltitude = startPos.y;
    convoyShipId = id;
    convoyTargetIslandIdx = (archipelago.islands.size() > 1) ? 1 : 0;
    npcShips.push_back(convoy);
}

void World::spawnCorsairDreadArk() {
    int id = nextEntityId_++;
    Airship boss(id, "Corsair Dread-Ark Sovereign", 1);
    ShipBlueprints::applyBlueprint(boss, 9); // Celestial Ark-Carrier dreadnought frame
    boss.hullLiveryId = 2; // Crimson Corsair Armada
    boss.shieldMultiplier = 1.6f;
    boss.recomputeMetrics();
    boss.shieldHp = boss.maxShieldHp;

    Vec3 fwd = playerShip.orientationQuat().forward();
    Vec3 spawnPos = playerShip.position + fwd * 145.0f + Vec3{0.0f, 12.0f, 0.0f};
    boss.position = spawnPos;
    boss.targetCruiseAltitude = spawnPos.y;
    boss.yaw = playerShip.yaw + kPi;
    dreadArkShipId = id;
    npcShips.push_back(boss);

    pendingAudioEvents.push_back({SimAudioEventType::LeviathanRoar, spawnPos, 1.4f});
    callouts.push_back({spawnPos + Vec3{0.0f, 12.0f, 0.0f}, "RAID BOSS: CORSAIR DREAD-ARK SOVEREIGN!", {255, 85, 75}, 3.0f, 3.0f});
    statusBannerText = "CORSAIR DREAD-ARK SOVEREIGN HAS WARPED INTO THE SECTOR! Sink the Pirate Flagship for +400 Gold & +16 Aetherium!";
    statusBannerTimer = 6.0f;
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

    float gunnerDmgMult = 1.0f + 0.10f * static_cast<float>(officerRanks[0]) + 0.03f * static_cast<float>(earnedMedalCount());

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
        proj.damage = r.damage * gunnerDmgMult;
        proj.isHarpoon = (r.weaponType == BlockId::HarpoonWinch || r.weaponType == BlockId::ShockHarpoon);
        proj.isShockHarpoon = (r.weaponType == BlockId::ShockHarpoon);
        proj.isHomingTorpedo = (r.weaponType == BlockId::TorpedoLauncher);
        proj.isIncendiary = (r.weaponType == BlockId::FlameMortar);

        if (r.weaponType == BlockId::TeslaCoil || r.weaponType == BlockId::PlasmaLance || r.weaponType == BlockId::TorpedoLauncher) {
            proj.gravityScale = 0.0f;
        } else if (r.weaponType == BlockId::RailgunBattery) {
            proj.gravityScale = 0.05f;
        } else if (r.weaponType == BlockId::FlameMortar) {
            proj.gravityScale = 1.45f;
        } else {
            proj.gravityScale = 1.0f;
        }

        if (r.weaponType == BlockId::TorpedoLauncher) {
            proj.splashRadius = 7.5f;
            proj.lifetime = 6.0f;
            proj.color = ColorRGB{255, 95, 55};
        } else if (r.weaponType == BlockId::RailgunBattery) {
            proj.splashRadius = 4.8f;
            proj.lifetime = 4.2f;
            proj.color = ColorRGB{125, 245, 255};
        } else if (r.weaponType == BlockId::FlameMortar) {
            proj.splashRadius = 6.8f;
            proj.lifetime = 5.0f;
            proj.color = ColorRGB{255, 115, 35};
        } else if (r.weaponType == BlockId::ShockHarpoon) {
            proj.splashRadius = 3.5f;
            proj.lifetime = 4.5f;
            proj.color = ColorRGB{75, 235, 255};
        } else if (r.weaponType == BlockId::TeslaCoil) {
            proj.splashRadius = 2.8f;
            proj.lifetime = 1.2f;
            proj.color = ColorRGB{95, 210, 255};
        } else if (r.weaponType == BlockId::PlasmaLance) {
            proj.splashRadius = 5.2f;
            proj.lifetime = 4.5f;
            proj.color = ColorRGB{255, 135, 55};
        } else if (proj.isHarpoon) {
            proj.splashRadius = 2.8f;
            proj.lifetime = 4.5f;
            proj.color = ColorRGB{235, 175, 65};
        } else {
            proj.splashRadius = 5.2f;
            proj.lifetime = 4.5f;
            proj.color = ColorRGB{255, 195, 80};
        }
        proj.maxLifetime = proj.lifetime;
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
    if (damageControlCooldown > 0.0f) {
        damageControlCooldown = std::max(0.0f, damageControlCooldown - dt);
    }
    if (sonarPingCooldown > 0.0f) {
        sonarPingCooldown = std::max(0.0f, sonarPingCooldown - dt);
    }
    if (skyMineCooldown > 0.0f) {
        skyMineCooldown = std::max(0.0f, skyMineCooldown - dt);
    }
    if (empNovaCooldown > 0.0f) {
        empNovaCooldown = std::max(0.0f, empNovaCooldown - dt);
    }
    if (pointDefenseCooldown > 0.0f) {
        pointDefenseCooldown = std::max(0.0f, pointDefenseCooldown - dt);
    }
    if (ramCooldownTimer > 0.0f) {
        ramCooldownTimer = std::max(0.0f, ramCooldownTimer - dt);
    }
    if (slipstreamComboTimer > 0.0f) {
        slipstreamComboTimer = std::max(0.0f, slipstreamComboTimer - dt);
        if (slipstreamComboTimer <= 0.0f) {
            slipstreamComboCount = 0;
        }
    }
    if (sonarMarkTimer > 0.0f) {
        sonarMarkTimer = std::max(0.0f, sonarMarkTimer - dt);
    }
    if (sonarWaveRadius > 0.0f) {
        sonarWaveRadius += 260.0f * dt;
        if (sonarWaveRadius > 650.0f) {
            sonarWaveRadius = 0.0f;
        }
    }
    if (empWaveRadius > 0.0f) {
        empWaveRadius += 310.0f * dt;
        if (empWaveRadius > 205.0f) {
            empWaveRadius = 0.0f;
        }
    }
    cameraTrauma = std::max(0.0f, cameraTrauma - 1.8f * dt);

    // Dynamic Weather Progression
    weatherTimer += dt;
    float weatherWindMult = 1.0f;
    if (currentWeather == WeatherState::CirrusMist) {
        weatherWindMult = 1.25f;
    } else if (currentWeather == WeatherState::IonAetherSquall) {
        weatherWindMult = 1.55f;
        // Ion Storm supercharges Aether Capacitor & Shield regeneration slightly!
        if (!playerShip.isDestroyed && playerShip.maxShieldHp > 0.0f) {
            playerShip.shieldHp = std::min(playerShip.maxShieldHp, playerShip.shieldHp + 2.5f * dt);
        }
    } else if (currentWeather == WeatherState::SupercellTempest) {
        weatherWindMult = 1.95f;
    }

    archipelago.update(dt);
    voyageElapsedSeconds += dt;
    stepSkyEcosystemAndWonders(dt);

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
    Vec3 playerWind = archipelago.sampleWindAt(playerShip.position) * weatherWindMult;
    playerShip.updatePhysics(
        dt,
        playerWind,
        playerAimWorldPos.x,
        playerAimWorldPos.y,
        playerAimWorldPos.z
    );

    // 2B. Check Aether-Slipstream Rings for Turbo Impulse & Regatta Chain Combo!
    if (!playerShip.isDestroyed) {
        for (auto& ring : archipelago.slipstreams) {
            if (ring.cooldownTimer > 0.0f) continue;
            float dist = (playerShip.position - ring.position).length();
            if (dist <= ring.radius + 7.5f) {
                ring.cooldownTimer = 10.0f;
                slipstreamRingsPassedStat++;
                slipstreamComboCount = (slipstreamComboTimer > 0.0f) ? std::min(5, slipstreamComboCount + 1) : 1;
                slipstreamComboTimer = 24.0f;

                Vec3 shipFwd = playerShip.orientationQuat().forward();
                Vec3 boostDir = (Vec3::dot(shipFwd, ring.forward) >= 0.0f) ? ring.forward : -ring.forward;
                playerShip.velocity += boostDir * 34.0f;
                playerShip.afterburnerFuel = std::min(playerShip.maxAfterburnerFuel, playerShip.afterburnerFuel + 42.0f);
                playerShip.shieldHp = std::min(playerShip.maxShieldHp, playerShip.shieldHp + 85.0f);

                int ringGold = 30 * slipstreamComboCount;
                playerGold += ringGold;
                playerAether += 2;
                pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, ring.position, 1.3f});
                callouts.push_back({
                    ring.position + Vec3{0.0f, 10.0f, 0.0f},
                    "SLIPSTREAM TURBO! x" + std::to_string(slipstreamComboCount) + " COMBO (+" + std::to_string(ringGold) + "G +2Ae)",
                    {85, 245, 255},
                    2.1f,
                    2.1f
                });
                statusBannerText = "AETHER-SLIPSTREAM RING PASSED! Turbo Velocity Boost + Overdrive Fuel + Shield (x" +
                                   std::to_string(slipstreamComboCount) + " Chain: +" + std::to_string(ringGold) + "G, +2 Ae)!";
                statusBannerTimer = 4.5f;
                break;
            }
        }

        // 2C. Check Thermal Aether-Geyser Updraft Columns for Buoyant Lift & Shield/Overdrive Recharge
        for (const auto& gey : archipelago.updrafts) {
            float dx = playerShip.position.x - gey.basePosition.x;
            float dz = playerShip.position.z - gey.basePosition.z;
            float horizDist = std::sqrt(dx * dx + dz * dz);
            if (horizDist <= gey.radius &&
                playerShip.position.y >= gey.basePosition.y - 12.0f &&
                playerShip.position.y <= gey.topY + 12.0f) {
                playerShip.velocity.y += gey.updraftSpeed * dt;
                playerShip.targetCruiseAltitude = std::min(182.0f, playerShip.targetCruiseAltitude + 15.0f * dt);
                playerShip.shieldHp = std::min(playerShip.maxShieldHp, playerShip.shieldHp + 28.0f * dt);
                playerShip.afterburnerFuel = std::min(playerShip.maxAfterburnerFuel, playerShip.afterburnerFuel + 26.0f * dt);
            }
        }

        // 2D. Check Ancient Aether-Obelisk Monolith Attunement (+220G, +10Ae, +85 Max Shield HP, +15 Max Overdrive)
        for (auto& ob : archipelago.obelisks) {
            if (ob.attuned) continue;
            float dist = (playerShip.position - ob.position).length();
            if (dist <= ob.attuneRadius) {
                ob.attuned = true;
                obelisksAttunedStat++;
                playerGold += 220;
                playerAether += 10;
                applyCrewMultipliers();
                playerShip.shieldHp = playerShip.maxShieldHp;
                playerShip.afterburnerFuel = playerShip.maxAfterburnerFuel;

                pendingAudioEvents.push_back({SimAudioEventType::TeslaDischarge, ob.position, 1.4f});
                explosions.push_back({ob.position + Vec3{0.0f, 18.0f, 0.0f}, 12.0f, {85, 245, 255}, false, true, playerShip.position});
                callouts.push_back({
                    ob.position + Vec3{0.0f, 14.0f, 0.0f},
                    "OBELISK ATTUNED: " + ob.name + "! (+220G +10Ae +85 Max Shield)",
                    {95, 245, 255},
                    2.6f,
                    2.6f
                });
                if (obelisksAttunedStat >= static_cast<int>(archipelago.obelisks.size())) {
                    playerGold += 500;
                    playerAether += 25;
                    statusBannerText = "ASTRAL CONVERGENCE AWAKENED! All 6 Ancient Aether-Obelisks Attuned (+500 Gold, +25 Aetherium, +510 Max Shield Dome)!";
                    statusBannerTimer = 6.5f;
                } else {
                    statusBannerText = "ANCIENT AETHER-OBELISK ATTUNED (" + std::to_string(obelisksAttunedStat) + "/6): " +
                                       ob.name + " (+220G, +10 Ae, +85 Max Shield HP, +15 Overdrive)!";
                    statusBannerTimer = 5.0f;
                }
                break;
            }
        }
    }

    // Check Island discovery, gentle island collision bump, & Fortified Stronghold Bastion Turret AI
    if (unlockedRelics[1] && !playerShip.isDestroyed) {
        playerShip.triggerEmergencyRepair(16.0f * dt);
    }

    for (auto& isl : archipelago.islands) {
        Vec3 diff = playerShip.position - isl.position;
        float distHoriz = std::sqrt(diff.x * diff.x + diff.z * diff.z);
        if (!isl.discovered && distHoriz < isl.radius + 190.0f) {
            isl.discovered = true;
            int discGold = static_cast<int>(35.0f * (1.0f + 0.25f * static_cast<float>(officerRanks[4])));
            playerGold += discGold;
            statusBannerText = "Discovered Sky-Island: " + isl.name + " (+" + std::to_string(discGold) + " Gold)";
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

        // Step Fortified Stronghold Bastion Turrets (if not yet liberated)
        if (isl.isFortifiedStronghold && !isl.isLiberated && !playerShip.isDestroyed && distHoriz < 320.0f) {
            for (auto& tur : isl.bastionTurrets) {
                if (tur.destroyed) continue;
                Vec3 toShip = (playerShip.position + playerShip.velocity * 0.45f) - tur.worldPos;
                float d = toShip.length();
                if (d > 5.0f && d < 305.0f) {
                    tur.barrelYaw = std::atan2(toShip.x, -toShip.z);
                    tur.reloadTimer -= dt;
                    if (tur.reloadTimer <= 0.0f) {
                        tur.reloadTimer = 2.65f;
                        Projectile3D flak;
                        flak.id = nextEntityId_++;
                        flak.ownerShipId = -isl.id;
                        flak.ownerFaction = 1; // Hostile Pirate Stronghold
                        flak.weaponType = BlockId::FlakTurret;
                        flak.position = tur.worldPos + Vec3{0.0f, 5.5f, 0.0f};
                        flak.prevPosition = flak.position;
                        flak.velocity = toShip.normalized() * 165.0f;
                        flak.damage = 28.0f;
                        flak.splashRadius = 4.2f;
                        flak.gravityScale = 0.25f;
                        flak.lifetime = 3.2f;
                        flak.maxLifetime = 3.2f;
                        flak.color = ColorRGB{255, 105, 65};
                        projectiles.push_back(flak);
                        pendingAudioEvents.push_back({SimAudioEventType::CannonFire, tur.worldPos, 0.8f});
                    }
                }
            }
        }
    }

    // 3. Step NPC Ships (Pirates, Allied Wingmen & Imperial Treasure Galleon Convoy)
    bool convoyAlive = false;
    int allyFormationIdx = 0;
    for (auto& npc : npcShips) {
        if (npc.isDestroyed) {
            continue;
        }
        AIDecision ai = AIController::computeShipControl(
            npc, &playerShip, npcShips, leviathans, archipelago, false
        );

        // Special Route Logic for the Imperial Treasure Galleon Convoy
        if (npc.id() == convoyShipId && !archipelago.islands.empty()) {
            convoyAlive = true;
            int tgtIdx = std::clamp(convoyTargetIslandIdx, 0, static_cast<int>(archipelago.islands.size()) - 1);
            const SkyIsland& destIsl = archipelago.islands[static_cast<size_t>(tgtIdx)];
            Vec3 destPos = destIsl.position + Vec3{0.0f, 40.0f, 0.0f};
            Vec3 toDest = destPos - npc.position;
            float distDest = toDest.length();

            Vec3 right = npc.orientationQuat().right();
            Vec3 dir = toDest.normalized();
            ai.rudder = std::clamp(Vec3::dot(dir, right) * 2.2f, -1.0f, 1.0f);
            ai.throttle = 0.88f;
            ai.altitude = std::clamp((destPos.y - npc.position.y) * 0.08f, -1.0f, 1.0f);

            if (distDest <= destIsl.radius + 58.0f) {
                convoysEscortedStat++;
                float navRewardMult = 1.0f + 0.25f * static_cast<float>(officerRanks[4]) + (unlockedRelics[4] ? 0.25f : 0.0f);
                int divGold = static_cast<int>(180.0f * navRewardMult);
                int divAeth = 6;
                playerGold += divGold;
                playerAether += divAeth;
                npc.triggerEmergencyRepair(260.0f);
                convoyTargetIslandIdx = (tgtIdx + 3) % static_cast<int>(archipelago.islands.size());
                const std::string& nextName = archipelago.islands[static_cast<size_t>(convoyTargetIslandIdx)].name;
                pendingAudioEvents.push_back({SimAudioEventType::CratePickup, npc.position, 1.2f});
                callouts.push_back({
                    npc.position + Vec3{0.0f, 10.0f, 0.0f},
                    "CONVOY ARRIVED! +" + std::to_string(divGold) + "G +" + std::to_string(divAeth) + "Ae",
                    {255, 220, 75},
                    2.5f,
                    2.5f
                });
                statusBannerText = "IMPERIAL TREASURE CONVOY safely reached " + destIsl.name + "! Escort Dividend: +" + std::to_string(divGold) + " Gold, +6 Aetherium! Next stop: " + nextName;
                statusBannerTimer = 5.2f;
            }
        } else if (npc.faction() == 0 && !playerShip.isDestroyed) {
            // Apply Fleet Admiral Squadron Order overrides for Allied Wingmen (faction == 0)
            allyFormationIdx++;
            float sideSign = (allyFormationIdx % 2 == 0) ? 1.0f : -1.0f;
            float wingRank = static_cast<float>((allyFormationIdx + 1) / 2);
            if (currentFleetOrder == FleetOrder::EscortFormation) {
                Vec3 escortSlot = playerShip.position
                    + playerShip.orientationQuat().right() * (sideSign * 42.0f * wingRank)
                    - playerShip.orientationQuat().forward() * (24.0f * wingRank);
                Vec3 toSlot = escortSlot - npc.position;
                float distSlot = toSlot.length();
                if (distSlot > 18.0f) {
                    Vec3 fwd = npc.orientationQuat().forward();
                    Vec3 right = npc.orientationQuat().right();
                    Vec3 dir = toSlot.normalized();
                    ai.rudder = std::clamp(Vec3::dot(dir, right) * 2.2f, -1.0f, 1.0f);
                    ai.throttle = std::clamp(Vec3::dot(dir, fwd) * 1.2f + 0.35f, 0.2f, 1.0f);
                    ai.altitude = std::clamp((playerShip.position.y - npc.position.y) * 0.08f, -1.0f, 1.0f);
                }
            } else if (currentFleetOrder == FleetOrder::FocusMyTarget) {
                ai.aimWorldPos = playerAimWorldPos;
                Vec3 toAim = playerAimWorldPos - npc.position;
                if (toAim.length() < 340.0f) {
                    ai.fireCannons = true;
                }
            } else if (currentFleetOrder == FleetOrder::DefensivePerimeter) {
                float orbitAngle = weatherTimer * 0.45f + static_cast<float>(allyFormationIdx) * 1.57f;
                Vec3 orbitPos = playerShip.position + Vec3{std::cos(orbitAngle) * 68.0f, 6.0f, std::sin(orbitAngle) * 68.0f};
                Vec3 toOrbit = orbitPos - npc.position;
                Vec3 right = npc.orientationQuat().right();
                ai.rudder = std::clamp(Vec3::dot(toOrbit.normalized(), right) * 2.4f, -1.0f, 1.0f);
                ai.throttle = 0.85f;
                ai.altitude = std::clamp((orbitPos.y - npc.position.y) * 0.08f, -1.0f, 1.0f);
            }
        }

        npc.throttleInput = ai.throttle;
        npc.rudderInput = ai.rudder;
        npc.altitudeInput = ai.altitude;
        Vec3 wind = archipelago.sampleWindAt(npc.position) * weatherWindMult;
        npc.updatePhysics(dt, wind, ai.aimWorldPos.x, ai.aimWorldPos.y, ai.aimWorldPos.z);

        bool wingmanHarpoonOrder = (npc.faction() == 0 && currentFleetOrder == FleetOrder::FreeHunt);
        if (ai.fireCannons || wingmanHarpoonOrder) {
            std::vector<MuzzleFireRequest> reqs;
            npc.fireWeapons(ai.fireCannons, wingmanHarpoonOrder, ai.aimWorldPos, reqs);
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
                proj.isHomingTorpedo = (r.weaponType == BlockId::TorpedoLauncher);
                proj.isIncendiary = (r.weaponType == BlockId::FlameMortar);
                proj.gravityScale = proj.isHomingTorpedo ? 0.0f : 1.0f;
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
                int bountyG = static_cast<int>(120.0f * (1.0f + 0.25f * static_cast<float>(officerRanks[4])));
                playerGold += bountyG;
                crates.push_back({nextEntityId_++, npc.position + Vec3{0.0f, 40.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, 95, 5, 60.0f, false});
                statusBannerText = "Enemy Sky-Pirate Vessel Sunk! (+" + std::to_string(bountyG) + " Gold)";
                statusBannerTimer = 4.5f;
            }
        }
    }

    if (!convoyAlive) {
        convoyRespawnTimer += dt;
        if (convoyRespawnTimer >= 40.0f) {
            convoyRespawnTimer = 0.0f;
            spawnImperialConvoy();
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

    // 6. Step Automated CIWS Point-Defense, Projectiles, Magnetic Sky-Mines, Star-Aether Meteoroids, Survivor Pods & Kinetic Ram Collisions
    stepPointDefense(dt);
    stepProjectiles(dt);
    stepSkyMines(dt);
    stepMeteors(dt);
    stepSurvivorPods(dt);
    stepKineticCollisions(dt);

    // 7. Step Falling Debris & Salvage Crates + Commendation Medal Checks
    stepDebrisAndCrates(dt);
    checkCommendationMedals();
}

void World::stepHarpoonTethers(float dt) {
    float harpoonerReelMult  = 1.0f + 0.30f * static_cast<float>(officerRanks[2]);
    float harpoonerBreakMult = 1.0f + 0.35f * static_cast<float>(officerRanks[2]) + (unlockedRelics[3] ? 0.40f : 0.0f);

    for (auto& t : tethers) {
        if (!t.active) {
            continue;
        }

        Vec3 shipAnchor = playerShip.localToWorldPos(t.sourceLocalOffset);
        Vec3 targetAnchor = shipAnchor;
        Leviathan* targetLev = nullptr;
        Airship* targetNpc = nullptr;

        if (t.targetLeviathanId >= 0) {
            for (auto& lev : leviathans) {
                if (lev.id() == t.targetLeviathanId && !lev.isSlain && !lev.segments.empty()) {
                    targetLev = &lev;
                    int segIdx = std::clamp(t.targetSegmentIdx, 0, static_cast<int>(lev.segments.size()) - 1);
                    targetAnchor = lev.segments[static_cast<size_t>(segIdx)].position;
                    break;
                }
            }
            if (!targetLev) {
                t.active = false;
                continue;
            }
        } else if (t.targetShipId >= 0) {
            for (auto& npc : npcShips) {
                if (npc.id() == t.targetShipId && !npc.isDestroyed) {
                    targetNpc = &npc;
                    targetAnchor = npc.localToWorldPos(t.targetShipLocalOffset);
                    break;
                }
            }
            if (!targetNpc) {
                t.active = false;
                continue;
            }
        } else {
            t.active = false;
            continue;
        }

        // Continuous Electric Shock Tether Pulse DPS! (Also active if Storm-Caller Conduit Relic is awakened!)
        if (t.isShockTether || unlockedRelics[3]) {
            t.shockPulseTimer += dt;
            if (t.shockPulseTimer >= 0.85f) {
                t.shockPulseTimer -= 0.85f;
                const float kShockDmg = 36.0f;
                explosions.push_back({targetAnchor, 4.2f, {75, 240, 255}, false, true, shipAnchor});
                if (targetLev) {
                    targetLev->damageSegment(t.targetSegmentIdx, kShockDmg, true);
                    callouts.push_back({targetAnchor + Vec3{0.0f, 5.0f, 0.0f}, "SHOCK TETHER! -36", {75, 245, 255}, 1.0f, 1.0f});
                    if (targetLev->isSlain) {
                        leviathansSlain++;
                        float navRewardMult = 1.0f + 0.25f * static_cast<float>(officerRanks[4]) + (unlockedRelics[4] ? 0.25f : 0.0f);
                        int goldReward = static_cast<int>(static_cast<float>(targetLev->goldBounty) * navRewardMult);
                        int aethReward = static_cast<int>(static_cast<float>(targetLev->aetherBounty) * navRewardMult);
                        playerGold += goldReward;
                        playerAether += aethReward;
                        playerCargo[3] = std::min(maxCargoCapacity(), playerCargo[3] + 2);
                        statusBannerText = "COLOSSAL LEVIATHAN SLAIN BY SHOCK TETHER: " + targetLev->name() + "! (+" + std::to_string(goldReward) + "G, +2 Ambergris)";
                        statusBannerTimer = 6.5f;
                        for (auto& c : contracts) {
                            if (c.targetName == targetLev->name()) c.completed = true;
                        }
                    }
                } else if (targetNpc) {
                    targetNpc->applyDamageAtWorldPos(targetAnchor, kShockDmg, 3.8f, true, debris);
                    targetNpc->velocity *= 0.78f; // Electro-stasis drag!
                    callouts.push_back({targetAnchor + Vec3{0.0f, 4.5f, 0.0f}, "SHOCK PULSE! -36", {75, 245, 255}, 1.0f, 1.0f});
                }
            }
        }

        Vec3 delta = targetAnchor - shipAnchor;
        t.currentLength = delta.length();

        if (t.isReelingIn) {
            t.restLength = std::max(28.0f, t.restLength - 30.0f * harpoonerReelMult * dt);
        }

        float effectiveMaxBreak = t.maxBreakTension * harpoonerBreakMult * (t.isShockTether ? 1.25f : 1.0f);
        if (t.currentLength > t.restLength) {
            float stretch = t.currentLength - t.restLength;
            t.tensionForce = stretch * 16.0f;

            if (t.tensionForce > effectiveMaxBreak) {
                t.active = false;
                pendingAudioEvents.push_back({SimAudioEventType::CableSnap, shipAnchor, 1.0f});
                statusBannerText = "WARNING: Harpoon Winch Cable Snapped Under Extreme Strain!";
                statusBannerTimer = 3.5f;
                continue;
            }

            Vec3 dirToTarget = delta.normalized();
            playerShip.velocity += dirToTarget * ((t.tensionForce / std::max(60.0f, playerShip.totalMass)) * dt * 2.4f);

            // If tethered to an enemy Pirate Airship, pull the enemy ship toward the player as well!
            if (targetNpc) {
                targetNpc->velocity -= dirToTarget * ((t.tensionForce / std::max(45.0f, targetNpc->totalMass)) * dt * 3.8f);
            }
        } else {
            t.tensionForce = 0.0f;
        }
    }

    tethers.erase(
        std::remove_if(tethers.begin(), tethers.end(), [](const HarpoonTether& t) { return !t.active; }),
        tethers.end()
    );
}

void World::stepPointDefense(float dt) {
    (void)dt;
    if (playerShip.isDestroyed || playerShip.flakTurretCount <= 0 || pointDefenseCooldown > 0.0f) {
        return;
    }

    const float ciwsRange = 65.0f + 8.0f * static_cast<float>(std::min(4, playerShip.flakTurretCount));
    Projectile3D* closestHostileProj = nullptr;
    float bestDist = ciwsRange;

    for (auto& p : projectiles) {
        if (!p.active || p.ownerFaction == playerShip.faction() || p.ownerShipId == playerShip.id()) {
            continue;
        }
        float d = (p.position - playerShip.position).length();
        if (d < bestDist) {
            bestDist = d;
            closestHostileProj = &p;
        }
    }

    if (closestHostileProj) {
        closestHostileProj->active = false;
        projectilesInterceptedStat++;
        playerGold += 8;
        // Cooldown scales down with more Rotary Flak Turrets installed
        pointDefenseCooldown = std::max(0.32f, 1.35f / static_cast<float>(playerShip.flakTurretCount));

        Vec3 turretOrigin = playerShip.position + Vec3{0.0f, 3.5f, 0.0f};
        explosions.push_back({closestHostileProj->position, 3.8f, {255, 220, 95}, false, true, turretOrigin});
        pendingAudioEvents.push_back({SimAudioEventType::CannonFire, turretOrigin, 0.65f});
        callouts.push_back({
            closestHostileProj->position + Vec3{0.0f, 3.5f, 0.0f},
            "CIWS FLAK INTERCEPT! (+8G)",
            {255, 230, 115},
            1.1f,
            1.1f
        });
    }
}

void World::stepProjectiles(float dt) {
    float sonarDmgBonus = (sonarMarkTimer > 0.0f) ? 1.25f : 1.0f;
    float navRewardMult = 1.0f + 0.25f * static_cast<float>(officerRanks[4]) + (unlockedRelics[4] ? 0.25f : 0.0f);

    for (auto& p : projectiles) {
        if (!p.active) {
            continue;
        }
        p.prevPosition = p.position;

        // Homing Torpedo active guidance toward nearest hostile Leviathan or Pirate ship!
        if (p.isHomingTorpedo && p.ownerShipId > 0) {
            Vec3 bestTargetPos = p.position + p.velocity;
            float bestDist = 520.0f;
            bool foundTarget = false;

            for (const auto& lev : leviathans) {
                if (lev.isSlain || lev.segments.empty()) continue;
                Vec3 headPos = lev.segments[0].position;
                float d = (headPos - p.position).length();
                if (d < bestDist) {
                    bestDist = d;
                    bestTargetPos = headPos;
                    foundTarget = true;
                }
            }
            for (const auto& npc : npcShips) {
                if (npc.isDestroyed || npc.faction() == p.ownerFaction) continue;
                float d = (npc.position - p.position).length();
                if (d < bestDist) {
                    bestDist = d;
                    bestTargetPos = npc.position;
                    foundTarget = true;
                }
            }
            if (foundTarget) {
                float spd = std::max(45.0f, p.velocity.length());
                Vec3 curDir = p.velocity.normalized();
                Vec3 desiredDir = (bestTargetPos - p.position).normalized();
                float turnBlend = std::clamp(2.8f * dt, 0.0f, 1.0f);
                Vec3 newDir = (curDir * (1.0f - turnBlend) + desiredDir * turnBlend).normalized();
                p.velocity = newDir * spd;
            }
        }

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

                    float effectiveDmg = p.damage * sonarDmgBonus;
                    if (p.isIncendiary && prevArmor > 0.0f) {
                        effectiveDmg *= 1.75f; // Flame Mortar super-melts heavy Leviathan carapace!
                    }
                    bool armorPierce = p.isHarpoon || p.weaponType == BlockId::PlasmaLance || p.isHomingTorpedo || p.weaponType == BlockId::RailgunBattery;
                    lev.damageSegment(hitSeg, effectiveDmg, armorPierce);
                    float newArmor = lev.segments[static_cast<size_t>(clampedSeg)].armorHp;

                    if (p.ownerShipId == playerShip.id()) {
                        if (prevArmor > 0.0f && newArmor <= 0.0f) {
                            callouts.push_back({hitPos + Vec3{0.0f, 6.0f, 0.0f}, "CARAPACE SHATTERED! CORE EXPOSED!", {85, 245, 255}, 2.0f, 2.0f});
                            cameraTrauma = std::min(1.0f, cameraTrauma + 0.35f);
                        } else if (wasWeakSpot) {
                            int critDmg = static_cast<int>(effectiveDmg * 2.4f);
                            callouts.push_back({hitPos + Vec3{0.0f, 5.0f, 0.0f}, "CRIT! -" + std::to_string(critDmg) + " [AETHER CORE]", {255, 95, 95}, 1.5f, 1.5f});
                        } else {
                            callouts.push_back({hitPos + Vec3{0.0f, 4.0f, 0.0f}, "-" + std::to_string(static_cast<int>(effectiveDmg)), {255, 215, 95}, 1.2f, 1.2f});
                        }
                    }

                    explosions.push_back({hitPos, p.splashRadius * 1.4f, p.color, false, p.weaponType == BlockId::TeslaCoil || p.isShockHarpoon, p.prevPosition});
                    pendingAudioEvents.push_back({
                        p.isHarpoon ? SimAudioEventType::HarpoonLatch : SimAudioEventType::BlockImpact,
                        hitPos,
                        1.0f
                    });

                    // Attach physical Harpoon Tether if this was a Harpoon or Shock Harpoon bolt!
                    if (p.isHarpoon && p.ownerShipId == playerShip.id()) {
                        HarpoonTether tether;
                        tether.id = nextEntityId_++;
                        tether.sourceShipId = playerShip.id();
                        tether.sourceLocalOffset = p.launchAnchorLocal;
                        tether.targetLeviathanId = lev.id();
                        tether.targetSegmentIdx = hitSeg;
                        tether.currentLength = (hitPos - playerShip.localToWorldPos(p.launchAnchorLocal)).length();
                        tether.restLength = std::max(45.0f, tether.currentLength * 0.95f);
                        tether.isShockTether = p.isShockHarpoon;
                        tethers.push_back(tether);
                        callouts.push_back({
                            hitPos + Vec3{0.0f, 7.0f, 0.0f},
                            p.isShockHarpoon ? "SHOCK HARPOON LATCHED!" : "HARPOON LATCHED!",
                            p.isShockHarpoon ? ColorRGB{75, 245, 255} : ColorRGB{85, 255, 165},
                            1.8f,
                            1.8f
                        });
                        statusBannerText = (p.isShockHarpoon ? "SHOCK HARPOON LATCHED onto " : "HARPOON LATCHED onto ") + lev.name() + "! Hold [R] to Reel In, [F] to Cut Cable.";
                        statusBannerTimer = 4.5f;
                    }

                    if (lev.isSlain) {
                        leviathansSlain++;
                        int goldReward = static_cast<int>(static_cast<float>(lev.goldBounty) * navRewardMult);
                        int aethReward = static_cast<int>(static_cast<float>(lev.aetherBounty) * navRewardMult);
                        playerGold += goldReward;
                        playerAether += aethReward;
                        playerCargo[3] = std::min(maxCargoCapacity(), playerCargo[3] + 2);
                        cameraTrauma = 1.0f;
                        crates.push_back({nextEntityId_++, hitPos, {0.0f, 4.0f, 0.0f}, goldReward / 2, aethReward / 2, 90.0f, false});
                        pendingAudioEvents.push_back({SimAudioEventType::LeviathanRoar, hitPos, 1.5f});
                        statusBannerText = "COLOSSAL LEVIATHAN SLAIN: " + lev.name() + "! (+" + std::to_string(goldReward) + " Gold, +" + std::to_string(aethReward) + " Aether, +2 Ambergris)";
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
                    float effDmg = p.damage * sonarDmgBonus * (p.isIncendiary ? 1.35f : 1.0f);
                    float prevShield = npc.shieldHp;
                    int broken = npc.applyDamageAtWorldPos(hitWorld, effDmg, p.splashRadius, true, debris);
                    blocksDestroyedStat += broken;

                    // Attach Harpoon Tether to Enemy Pirate Airship if harpoon hit!
                    if (p.isHarpoon && p.ownerShipId == playerShip.id()) {
                        HarpoonTether tether;
                        tether.id = nextEntityId_++;
                        tether.sourceShipId = playerShip.id();
                        tether.sourceLocalOffset = p.launchAnchorLocal;
                        tether.targetShipId = npc.id();
                        tether.targetShipLocalOffset = npc.worldToLocalPos(hitWorld);
                        tether.currentLength = (hitWorld - playerShip.localToWorldPos(p.launchAnchorLocal)).length();
                        tether.restLength = std::max(32.0f, tether.currentLength * 0.92f);
                        tether.isShockTether = p.isShockHarpoon;
                        tethers.push_back(tether);
                        callouts.push_back({
                            hitWorld + Vec3{0.0f, 6.0f, 0.0f},
                            p.isShockHarpoon ? "SHOCK HARPOON LATCHED!" : "GRAPPLING HARPOON LATCHED!",
                            p.isShockHarpoon ? ColorRGB{75, 245, 255} : ColorRGB{85, 255, 165},
                            1.8f,
                            1.8f
                        });
                        statusBannerText = "GRAPPLING HARPOON LATCHED onto " + npc.name() + "! Hold [R] to Reel & Tow!";
                        statusBannerTimer = 4.0f;
                    }

                    if (p.ownerShipId == playerShip.id()) {
                        if (prevShield > 0.0f && npc.shieldHp > 0.0f) {
                            callouts.push_back({hitWorld + Vec3{0.0f, 4.0f, 0.0f}, "SHIELD HIT! -" + std::to_string(static_cast<int>(effDmg)), {85, 225, 255}, 1.1f, 1.1f});
                        } else if (broken > 1) {
                            callouts.push_back({hitWorld + Vec3{0.0f, 5.0f, 0.0f}, "HULL FRACTURE! [" + std::to_string(broken) + " BLOCKS]", {255, 145, 55}, 1.7f, 1.7f});
                            cameraTrauma = std::min(1.0f, cameraTrauma + 0.28f);
                        } else {
                            callouts.push_back({hitWorld + Vec3{0.0f, 3.5f, 0.0f}, "-" + std::to_string(static_cast<int>(effDmg)), {255, 205, 95}, 1.1f, 1.1f});
                        }
                    }
                    explosions.push_back({hitWorld, p.splashRadius, p.color, false, p.isShockHarpoon, p.prevPosition});
                    pendingAudioEvents.push_back({
                        broken > 1 ? SimAudioEventType::StructuralFracture : SimAudioEventType::BlockImpact,
                        hitWorld,
                        1.0f
                    });

                    if (npc.isDestroyed && npc.faction() == 1) {
                        piratesSunk++;
                        if (npc.id() == dreadArkShipId) {
                            dreadnoughtsSunkStat++;
                            int arkGold = static_cast<int>(400.0f * navRewardMult);
                            playerGold += arkGold;
                            playerAether += 16;
                            crates.push_back({nextEntityId_++, npc.position, {0.0f, 4.0f, 0.0f}, 200, 10, 75.0f, false});
                            statusBannerText = "CORSAIR DREAD-ARK SOVEREIGN SUNK! (+" + std::to_string(arkGold) + " Gold, +16 Aetherium, Royal Salvage Dropped!)";
                            statusBannerTimer = 6.0f;
                        } else {
                            int bountyG = static_cast<int>(120.0f * navRewardMult);
                            playerGold += bountyG;
                            crates.push_back({nextEntityId_++, npc.position, {0.0f, 3.0f, 0.0f}, 95, 5, 60.0f, false});
                        }
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
                    cameraTrauma = std::min(1.0f, cameraTrauma + (broken > 0 ? 0.35f : 0.10f));
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

        // D. Check collision against Island Aetherium Crystal Nodes & Fortified Sky-Bastion Turrets
        if (p.ownerFaction == 0) {
            for (auto& isl : archipelago.islands) {
                if ((p.position - isl.position).length() > isl.radius + 32.0f) {
                    continue;
                }
                // D1. Crystal mining
                for (auto& cry : isl.crystals) {
                    if (cry.harvested) continue;
                    if ((p.position - cry.worldPos).length() <= cry.radius + 2.5f) {
                        cry.hp -= p.damage;
                        explosions.push_back({cry.worldPos, 4.0f, {65, 235, 255}, false, false, p.prevPosition});
                        if (cry.hp <= 0.0f) {
                            cry.harvested = true;
                            int aethGain = static_cast<int>(std::ceil(static_cast<float>(cry.aetherYield) * navRewardMult));
                            int goldGain = static_cast<int>(std::ceil(static_cast<float>(cry.goldYield) * navRewardMult));
                            playerAether += aethGain;
                            playerGold += goldGain;
                            callouts.push_back({cry.worldPos + Vec3{0.0f, 5.0f, 0.0f}, "+" + std::to_string(aethGain) + " AETHERIUM!", {75, 245, 255}, 1.8f, 1.8f});
                            statusBannerText = "Shattered Aetherium Crystal Vein! (+" + std::to_string(aethGain) + " Aether, +" + std::to_string(goldGain) + " Gold)";
                            statusBannerTimer = 4.0f;
                        }
                        p.active = false;
                        break;
                    }
                }
                if (!p.active) break;

                // D2. Fortified Stronghold Sky-Bastion Turrets
                if (isl.isFortifiedStronghold && !isl.isLiberated) {
                    for (auto& tur : isl.bastionTurrets) {
                        if (tur.destroyed) continue;
                        if ((p.position - tur.worldPos).length() <= tur.radius + 3.2f) {
                            float siegeDmg = p.damage * sonarDmgBonus * (p.isIncendiary ? 1.75f : 1.0f);
                            tur.hp -= siegeDmg;
                            explosions.push_back({tur.worldPos + Vec3{0.0f, 3.0f, 0.0f}, 5.5f, p.color, false, false, p.prevPosition});
                            pendingAudioEvents.push_back({SimAudioEventType::BlockImpact, tur.worldPos, 1.1f});
                            if (tur.hp <= 0.0f) {
                                tur.hp = 0.0f;
                                tur.destroyed = true;
                                int turG = static_cast<int>(85.0f * navRewardMult);
                                playerGold += turG;
                                playerAether += 4;
                                cameraTrauma = std::min(1.0f, cameraTrauma + 0.35f);
                                callouts.push_back({tur.worldPos + Vec3{0.0f, 8.0f, 0.0f}, "BASTION TOWER DESTROYED! +" + std::to_string(turG) + "G", {255, 165, 55}, 2.0f, 2.0f});

                                bool allDead = true;
                                for (const auto& bt : isl.bastionTurrets) {
                                    if (!bt.destroyed) {
                                        allDead = false;
                                        break;
                                    }
                                }
                                if (allDead) {
                                    isl.isLiberated = true;
                                    isl.hasDrydockPort = true;
                                    citadelsLiberatedStat++;
                                    int libGold = static_cast<int>(350.0f * navRewardMult);
                                    playerGold += libGold;
                                    playerAether += 18;
                                    playerShip.triggerEmergencyRepair(999.0f);
                                    playerShip.shieldHp = playerShip.maxShieldHp;
                                    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, isl.position, 1.5f});
                                    callouts.push_back({isl.position + Vec3{0.0f, 22.0f, 0.0f}, "STRONGHOLD LIBERATED! +" + std::to_string(libGold) + "G +18Ae", {85, 255, 185}, 3.0f, 3.0f});
                                    statusBannerText = "SKY-CITADEL LIBERATED: " + isl.name + "! (+350 Gold, +18 Aetherium, Drydock Opened & -20% Trade Prices!)";
                                    statusBannerTimer = 6.5f;
                                } else {
                                    statusBannerText = "Destroyed Bastion Flak Tower at " + isl.name + "! (+" + std::to_string(turG) + "G, +4 Aether)";
                                    statusBannerTimer = 4.0f;
                                }
                            } else {
                                callouts.push_back({tur.worldPos + Vec3{0.0f, 6.0f, 0.0f}, "BASTION HIT! -" + std::to_string(static_cast<int>(siegeDmg)), {255, 205, 85}, 1.2f, 1.2f});
                            }
                            p.active = false;
                            break;
                        }
                    }
                }
                if (!p.active) break;
            }

            // D3. Check collision against Celestial Star-Aether Meteoroids
            if (p.active) {
                for (auto& met : meteors) {
                    if (!met.active) continue;
                    if ((p.position - met.position).length() <= met.radius + 3.0f) {
                        met.active = false;
                        p.active = false;
                        meteorsHarvestedStat++;
                        int aethGain = static_cast<int>(std::ceil(static_cast<float>(met.aetherYield) * navRewardMult));
                        int goldGain = static_cast<int>(std::ceil(static_cast<float>(met.goldYield) * navRewardMult));
                        playerAether += aethGain;
                        playerGold += goldGain;
                        cameraTrauma = std::min(1.0f, cameraTrauma + 0.25f);
                        explosions.push_back({met.position, 8.0f, {215, 115, 255}, false, false, p.prevPosition});
                        pendingAudioEvents.push_back({SimAudioEventType::CratePickup, met.position, 1.3f});
                        callouts.push_back({
                            met.position + Vec3{0.0f, 6.0f, 0.0f},
                            "STAR-AETHER METEOR SHATTERED! +" + std::to_string(aethGain) + "Ae +" + std::to_string(goldGain) + "G",
                            {225, 135, 255},
                            2.2f,
                            2.2f
                        });
                        statusBannerText = "SHATTERED CELESTIAL STAR-AETHER METEOROID! (+" + std::to_string(aethGain) + " Aetherium, +" + std::to_string(goldGain) + " Gold)";
                        statusBannerTimer = 4.8f;
                        break;
                    }
                }
            }
        }
    }

    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile3D& p) { return !p.active; }),
        projectiles.end()
    );
}

void World::stepSkyMines(float dt) {
    float navRewardMult = 1.0f + 0.25f * static_cast<float>(officerRanks[4]) + (unlockedRelics[4] ? 0.25f : 0.0f);

    for (auto& m : skyMines) {
        if (!m.active) continue;
        m.pulsePhase += dt * 4.5f;
        m.lifetime -= dt;
        if (m.armTimer > 0.0f) {
            m.armTimer = std::max(0.0f, m.armTimer - dt);
        }
        if (m.lifetime <= 0.0f) {
            m.active = false;
            continue;
        }

        // Gentle buoyant hover damping before magnetic lock
        m.velocity *= std::max(0.0f, 1.0f - 2.2f * dt);

        if (m.armTimer <= 0.0f) {
            Vec3 bestTargetPos = m.position;
            float bestDist = m.triggerRadius;
            Leviathan* hitLev = nullptr;
            int hitLevSeg = -1;
            Airship* hitNpc = nullptr;

            // Check proximity to Colossal Sky-Leviathan segments
            for (auto& lev : leviathans) {
                if (lev.isSlain) continue;
                for (size_t s = 0; s < lev.segments.size(); ++s) {
                    float d = (lev.segments[s].position - m.position).length();
                    if (d < bestDist) {
                        bestDist = d;
                        bestTargetPos = lev.segments[s].position;
                        hitLev = &lev;
                        hitLevSeg = static_cast<int>(s);
                        hitNpc = nullptr;
                    }
                }
            }

            // Check proximity to Hostile Pirate Airships
            for (auto& npc : npcShips) {
                if (npc.isDestroyed || npc.faction() == m.ownerFaction) continue;
                float d = (npc.position - m.position).length();
                if (d < bestDist) {
                    bestDist = d;
                    bestTargetPos = npc.position;
                    hitNpc = &npc;
                    hitLev = nullptr;
                }
            }

            if (hitLev || hitNpc) {
                Vec3 dir = (bestTargetPos - m.position).normalized();
                m.velocity = dir * 68.0f;

                float detonateRadius = hitLev
                    ? (hitLev->segments[static_cast<size_t>(hitLevSeg)].radius + 5.5f)
                    : (hitNpc->boundingRadius * 0.75f + 4.5f);

                if (bestDist <= detonateRadius) {
                    m.active = false;
                    skyMineHitsStat++;
                    cameraTrauma = std::min(1.0f, cameraTrauma + 0.42f);
                    explosions.push_back({m.position, m.blastRadius, {255, 145, 45}, false, true, bestTargetPos});
                    pendingAudioEvents.push_back({SimAudioEventType::StructuralFracture, m.position, 1.35f});

                    if (hitLev) {
                        hitLev->damageSegment(hitLevSeg, m.damage, true);
                        callouts.push_back({
                            m.position + Vec3{0.0f, 7.0f, 0.0f},
                            "SKY-MINE BLAST! -" + std::to_string(static_cast<int>(m.damage)),
                            {255, 145, 55},
                            1.8f,
                            1.8f
                        });
                        if (hitLev->isSlain) {
                            leviathansSlain++;
                            int goldReward = static_cast<int>(static_cast<float>(hitLev->goldBounty) * navRewardMult);
                            int aethReward = static_cast<int>(static_cast<float>(hitLev->aetherBounty) * navRewardMult);
                            playerGold += goldReward;
                            playerAether += aethReward;
                            playerCargo[3] = std::min(maxCargoCapacity(), playerCargo[3] + 2);
                            statusBannerText = "LEVIATHAN SLAIN BY SKY-MINE: " + hitLev->name() + "! (+" + std::to_string(goldReward) + "G, +2 Ambergris)";
                            statusBannerTimer = 6.5f;
                            for (auto& c : contracts) {
                                if (c.targetName == hitLev->name()) c.completed = true;
                            }
                        }
                    } else if (hitNpc) {
                        int broken = hitNpc->applyDamageAtWorldPos(m.position, m.damage, m.blastRadius * 0.65f, true, debris);
                        blocksDestroyedStat += broken;
                        callouts.push_back({
                            m.position + Vec3{0.0f, 6.0f, 0.0f},
                            "SKY-MINE DETONATION! -" + std::to_string(static_cast<int>(m.damage)),
                            {255, 145, 55},
                            1.8f,
                            1.8f
                        });
                        if (hitNpc->isDestroyed && hitNpc->faction() == 1) {
                            piratesSunk++;
                            int bountyG = static_cast<int>(120.0f * navRewardMult);
                            playerGold += bountyG;
                            crates.push_back({nextEntityId_++, hitNpc->position, {0.0f, 3.0f, 0.0f}, 95, 5, 60.0f, false});
                        }
                    }
                    continue;
                }
            }
        }

        m.position += m.velocity * dt;
    }

    skyMines.erase(
        std::remove_if(skyMines.begin(), skyMines.end(), [](const SkyMine3D& m) { return !m.active; }),
        skyMines.end()
    );
}

void World::stepMeteors(float dt) {
    float navRewardMult = 1.0f + 0.25f * static_cast<float>(officerRanks[4]) + (unlockedRelics[4] ? 0.25f : 0.0f);

    meteorSpawnTimer -= dt;
    if (meteorSpawnTimer <= 0.0f) {
        meteorSpawnTimer = 32.0f;
        if (meteors.size() < 5) {
            float angle = rng_.range(0.0f, kTwoPi);
            float dist  = rng_.range(120.0f, 280.0f);
            Vec3 spawnPos = playerShip.position + Vec3{std::cos(angle) * dist, rng_.range(25.0f, 55.0f), std::sin(angle) * dist};
            Vec3 vel = Vec3{-std::cos(angle) * 3.2f, -1.6f, -std::sin(angle) * 3.2f};
            spawnStarMeteor(spawnPos, vel);
            statusBannerText = "CELESTIAL EVENT: A Star-Aether Meteoroid has entered the Archipelago! Shoot or intercept it for +14 Aetherium!";
            statusBannerTimer = 4.5f;
        }
    }

    for (auto& met : meteors) {
        if (!met.active) continue;
        met.spinAngle += dt * 45.0f;
        if (met.position.y < 15.0f) {
            met.active = false;
            continue;
        }

        // Gentle buoyant hover near cruise altitude so players can hunt them across the sky
        if (met.position.y < 126.0f && met.velocity.y < 0.0f) {
            met.velocity.y += 4.5f * dt;
        }
        met.position += met.velocity * dt;

        // Tractor-harvest when the player flagship flies within range (or pull via Aether Tractor Grapplers!)
        if (!playerShip.isDestroyed) {
            Vec3 toPlayer = playerShip.position - met.position;
            float dist = toPlayer.length();
            float tractorRange = (playerShip.tractorBeamCount > 0)
                ? (115.0f + 85.0f * static_cast<float>(playerShip.tractorBeamCount))
                : 0.0f;
            if (dist < tractorRange && dist > 1.0f) {
                float pullSpd = 38.0f + 18.0f * static_cast<float>(playerShip.tractorBeamCount);
                met.velocity = (toPlayer / dist) * pullSpd;
            }
            if (dist <= playerShip.boundingRadius + met.radius + 14.0f) {
                met.active = false;
                meteorsHarvestedStat++;
                int aethGain = static_cast<int>(std::ceil(static_cast<float>(met.aetherYield) * navRewardMult));
                int goldGain = static_cast<int>(std::ceil(static_cast<float>(met.goldYield) * navRewardMult));
                playerAether += aethGain;
                playerGold += goldGain;
                playerShip.afterburnerFuel = playerShip.maxAfterburnerFuel;
                explosions.push_back({met.position, 7.5f, {215, 115, 255}, false, false, playerShip.position});
                pendingAudioEvents.push_back({SimAudioEventType::CratePickup, met.position, 1.35f});
                callouts.push_back({
                    met.position + Vec3{0.0f, 6.0f, 0.0f},
                    "STAR-AETHER HARVESTED! +" + std::to_string(aethGain) + "Ae +" + std::to_string(goldGain) + "G",
                    {225, 135, 255},
                    2.2f,
                    2.2f
                });
                statusBannerText = "HARVESTED CELESTIAL STAR-AETHER METEOROID! (+" + std::to_string(aethGain) + " Aetherium, +" + std::to_string(goldGain) + " Gold, Overdrive Full)";
                statusBannerTimer = 4.8f;
            }
        }
    }

    meteors.erase(
        std::remove_if(meteors.begin(), meteors.end(), [](const AetherMeteor3D& m) { return !m.active; }),
        meteors.end()
    );
}

void World::stepSurvivorPods(float dt) {
    float navRewardMult = 1.0f + 0.25f * static_cast<float>(officerRanks[4]) + (unlockedRelics[4] ? 0.25f : 0.0f);

    for (auto& pod : survivorPods) {
        if (pod.rescued) continue;
        pod.flarePhase += dt * 4.0f;
        pod.velocity.y = std::sin(pod.flarePhase * 0.6f) * 1.4f;

        if (!playerShip.isDestroyed) {
            Vec3 toPlayer = playerShip.position - pod.position;
            float dist = toPlayer.length();
            float pullRange = 65.0f + 85.0f * static_cast<float>(playerShip.tractorBeamCount);
            if (dist < pullRange && dist > 1.0f) {
                pod.velocity = (toPlayer / dist) * (36.0f + 16.0f * static_cast<float>(playerShip.tractorBeamCount));
            }
            pod.position += pod.velocity * dt;

            if (dist <= playerShip.boundingRadius + 14.0f) {
                pod.rescued = true;
                survivorsRescuedStat++;
                int gReward = static_cast<int>(std::ceil(static_cast<float>(pod.goldReward) * navRewardMult));
                int aReward = pod.aetherReward;
                playerGold += gReward;
                playerAether += aReward;
                playerShip.triggerEmergencyRepair(180.0f);
                pendingAudioEvents.push_back({SimAudioEventType::CratePickup, pod.position, 1.3f});
                callouts.push_back({
                    pod.position + Vec3{0.0f, 6.0f, 0.0f},
                    "SURVIVORS RESCUED! +" + std::to_string(gReward) + "G +" + std::to_string(aReward) + "Ae (+180 HP)",
                    {95, 250, 175},
                    2.2f,
                    2.2f
                });
                statusBannerText = "RESCUED STRANDED SKY-SAILORS! (+" + std::to_string(gReward) + " Gold, +" + std::to_string(aReward) + " Aetherium, Grateful Crew Repaired +180 Hull HP!)";
                statusBannerTimer = 4.6f;
            }
        } else {
            pod.position += pod.velocity * dt;
        }
    }

    survivorPods.erase(
        std::remove_if(survivorPods.begin(), survivorPods.end(), [](const SkySurvivorPod& p) { return p.rescued; }),
        survivorPods.end()
    );

    if (survivorPods.size() < 2) {
        float ang = rng_.range(0.0f, kTwoPi);
        float d = rng_.range(180.0f, 420.0f);
        spawnSurvivorPod(playerShip.position + Vec3{std::cos(ang) * d, 4.0f, std::sin(ang) * d});
    }
}

void World::stepKineticCollisions(float dt) {
    (void)dt;
    if (playerShip.isDestroyed || ramCooldownTimer > 0.0f) {
        return;
    }
    float navRewardMult = 1.0f + 0.25f * static_cast<float>(officerRanks[4]) + (unlockedRelics[4] ? 0.25f : 0.0f);

    // 1. Check Kinetic Bow Ram against Enemy NPC Airships
    for (auto& npc : npcShips) {
        if (npc.isDestroyed || npc.faction() == playerShip.faction()) continue;
        Vec3 delta = npc.position - playerShip.position;
        float dist = delta.length();
        float minSep = (playerShip.boundingRadius + npc.boundingRadius) * 0.62f;
        if (dist < minSep && dist > 0.1f) {
            Vec3 normal = delta / dist;
            Vec3 relVel = playerShip.velocity - npc.velocity;
            float closingSpeed = Vec3::dot(relVel, normal);
            if (closingSpeed > 8.5f) {
                ramCooldownTimer = 1.15f;
                ramImpactsStat++;
                float prowMult = playerShip.afterburnerInput ? 1.50f : 1.0f;
                float ramDmg = closingSpeed * (6.2f + 0.014f * playerShip.totalMass) * prowMult;
                Vec3 contactPt = playerShip.position + normal * (playerShip.boundingRadius * 0.55f);

                int broken = npc.applyDamageAtWorldPos(contactPt, ramDmg, 6.2f, true, debris);
                blocksDestroyedStat += broken;
                // Recoil damage to player ship is heavily absorbed by shield/prow
                playerShip.applyDamageAtWorldPos(contactPt, ramDmg * 0.18f, 3.5f, true, debris);

                // Elastic impulse separation
                playerShip.velocity -= normal * (closingSpeed * 0.55f);
                npc.velocity += normal * (closingSpeed * 0.85f);
                playerShip.position -= normal * ((minSep - dist) * 0.5f);
                npc.position += normal * ((minSep - dist) * 0.5f);

                cameraTrauma = std::min(1.0f, cameraTrauma + 0.48f);
                explosions.push_back({contactPt, 7.5f, {255, 185, 65}, false, false, playerShip.position});
                pendingAudioEvents.push_back({SimAudioEventType::StructuralFracture, contactPt, 1.4f});
                callouts.push_back({
                    contactPt + Vec3{0.0f, 6.5f, 0.0f},
                    "KINETIC PROW RAM! -" + std::to_string(static_cast<int>(ramDmg)) + " [" + std::to_string(broken) + " BLOCKS]",
                    {255, 165, 55},
                    2.0f,
                    2.0f
                });
                statusBannerText = "KINETIC BOW RAM IMPACT on " + npc.name() + "! Dealt " + std::to_string(static_cast<int>(ramDmg)) + " Structural Damage!";
                statusBannerTimer = 4.0f;

                if (npc.isDestroyed && npc.faction() == 1) {
                    piratesSunk++;
                    int bountyG = static_cast<int>(120.0f * navRewardMult);
                    playerGold += bountyG;
                    crates.push_back({nextEntityId_++, npc.position, {0.0f, 3.0f, 0.0f}, 95, 5, 60.0f, false});
                }
                return;
            }
        }
    }

    // 2. Check Kinetic Bow Ram against Colossal Sky-Leviathans
    for (auto& lev : leviathans) {
        if (lev.isSlain) continue;
        for (size_t s = 0; s < lev.segments.size(); ++s) {
            Vec3 delta = lev.segments[s].position - playerShip.position;
            float dist = delta.length();
            float minSep = playerShip.boundingRadius * 0.55f + lev.segments[s].radius + 2.5f;
            if (dist < minSep && dist > 0.1f) {
                Vec3 normal = delta / dist;
                Vec3 relVel = playerShip.velocity - lev.velocity;
                float closingSpeed = Vec3::dot(relVel, normal);
                if (closingSpeed > 9.0f) {
                    ramCooldownTimer = 1.25f;
                    ramImpactsStat++;
                    float prowMult = playerShip.afterburnerInput ? 1.55f : 1.0f;
                    float ramDmg = closingSpeed * (7.0f + 0.015f * playerShip.totalMass) * prowMult;
                    Vec3 contactPt = lev.segments[s].position - normal * lev.segments[s].radius;

                    lev.damageSegment(static_cast<int>(s), ramDmg, true);
                    playerShip.applyDamageAtWorldPos(contactPt, ramDmg * 0.16f, 3.5f, true, debris);

                    playerShip.velocity = -normal * std::max(16.0f, closingSpeed * 0.65f);
                    playerShip.position -= normal * ((minSep - dist) + 2.0f);

                    cameraTrauma = std::min(1.0f, cameraTrauma + 0.55f);
                    explosions.push_back({contactPt, 8.5f, {255, 165, 55}, false, false, playerShip.position});
                    pendingAudioEvents.push_back({SimAudioEventType::StructuralFracture, contactPt, 1.45f});
                    callouts.push_back({
                        contactPt + Vec3{0.0f, 7.5f, 0.0f},
                        "LEVIATHAN PROW RAM! -" + std::to_string(static_cast<int>(ramDmg)),
                        {255, 145, 55},
                        2.1f,
                        2.1f
                    });
                    statusBannerText = "RAMMED COLOSSAL LEVIATHAN " + lev.name() + " FOR " + std::to_string(static_cast<int>(ramDmg)) + " KINETIC DAMAGE!";
                    statusBannerTimer = 4.2f;

                    if (lev.isSlain) {
                        leviathansSlain++;
                        int goldReward = static_cast<int>(static_cast<float>(lev.goldBounty) * navRewardMult);
                        int aethReward = static_cast<int>(static_cast<float>(lev.aetherBounty) * navRewardMult);
                        playerGold += goldReward;
                        playerAether += aethReward;
                        playerCargo[3] = std::min(maxCargoCapacity(), playerCargo[3] + 2);
                        for (auto& c : contracts) {
                            if (c.targetName == lev.name()) c.completed = true;
                        }
                    }
                    return;
                }
            }
        }
    }
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

    float navRewardMult = 1.0f + 0.25f * static_cast<float>(officerRanks[4]);
    for (auto& c : crates) {
        if (c.collected) continue;
        c.lifetime -= dt;
        // Tractor pull when player ship is within 75m (+85m per Aether Tractor Grappler block!)
        Vec3 toPlayer = playerShip.position - c.position;
        float dist = toPlayer.length();
        float cratePullRange = 75.0f + 85.0f * static_cast<float>(playerShip.tractorBeamCount);
        float cratePullSpeed = 52.0f + 24.0f * static_cast<float>(playerShip.tractorBeamCount);
        if (dist < cratePullRange && dist > 0.1f) {
            c.velocity = (toPlayer / dist) * cratePullSpeed;
        } else {
            c.velocity.y = std::sin(c.lifetime * 2.5f) * 1.5f;
        }
        c.position += c.velocity * dt;

        if (dist < playerShip.boundingRadius + 6.0f) {
            c.collected = true;
            int gGain = static_cast<int>(std::ceil(static_cast<float>(c.goldAmount) * navRewardMult));
            int aGain = static_cast<int>(std::ceil(static_cast<float>(c.aetherAmount) * navRewardMult));
            playerGold += gGain;
            playerAether += aGain;
            callouts.push_back({playerShip.position + Vec3{0.0f, 6.0f, 0.0f}, "SALVAGE! +" + std::to_string(gGain) + "G +" + std::to_string(aGain) + "Ae", {255, 225, 85}, 1.6f, 1.6f});
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

void World::togglePlayerSearchlight() {
    playerSearchlightOn = !playerSearchlightOn;
    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, playerShip.position, 0.75f});
    statusBannerText = std::string("BOW AETHER-SEARCHLIGHT [L]: ") + (playerSearchlightOn ? "ENGAGED (Illuminates Darkness & Burns Umbra Cloaks)" : "EXTINGUISHED (Stealth Running)");
    statusBannerTimer = 3.2f;
}

void World::emergencyRefloatPlayerShip() {
    Vec3 harborPos = !archipelago.islands.empty()
        ? (archipelago.islands[0].position + Vec3{0.0f, 34.0f, -128.0f})
        : Vec3{0.0f, 134.0f, -128.0f};
    playerShip.position = harborPos;
    playerShip.velocity = {0.0f, 0.0f, 0.0f};
    playerShip.angularVelYaw = 0.0f;
    playerShip.pitch = 0.0f;
    playerShip.roll = 0.0f;
    playerShip.targetCruiseAltitude = harborPos.y;
    playerShip.isDestroyed = false;
    playerShip.isSinking = false;
    playerShip.triggerEmergencyRepair(9999.0f);
    playerShip.shieldHp = playerShip.maxShieldHp;
    playerShip.afterburnerFuel = playerShip.maxAfterburnerFuel;
    releaseAllPlayerHarpoons();
    pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, harborPos, 1.4f});
    statusBannerText = "EMERGENCY SALVAGE TOW COMPLETE: Flagship refloated and repaired at New Horizon Sky-Pier!";
    statusBannerTimer = 4.5f;
}

void World::startDailySkyVoyage() {
    std::time_t now = std::time(nullptr);
    std::tm* gmt = std::gmtime(&now);
    int year = gmt ? (gmt->tm_year + 1900) : 2026;
    int mon  = gmt ? (gmt->tm_mon + 1) : 10;
    int day  = gmt ? gmt->tm_mday : 30;
    uint64_t dailySeedVal = static_cast<uint64_t>(year * 10000 + mon * 100 + day);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, mon, day);
    dailyDateLabel = buf;

    int dailyShipIdx = static_cast<int>(dailySeedVal % 8ULL);
    initWorld(dailySeedVal * 2654435761ULL, dailyShipIdx, false);
    isDailyVoyage = true;
    activeRiskContractIdx = static_cast<int>((dailySeedVal % 3ULL) + 1ULL);
    skyAtmospherePreset = static_cast<int>(dailySeedVal % 4ULL);
    currentWeather = static_cast<WeatherState>((dailySeedVal / 3ULL) % 4ULL);
    statusBannerText = "DAILY SKY-VOYAGE (" + dailyDateLabel + "): Seed #" + std::to_string(dailySeedVal) +
                       " | Charter: " + getRiskContractName(activeRiskContractIdx) + "!";
    statusBannerTimer = 6.0f;
}

void World::cycleRiskContract() {
    activeRiskContractIdx = (activeRiskContractIdx + 1) % 4;
    pendingAudioEvents.push_back({SimAudioEventType::CratePickup, playerShip.position, 0.9f});
    statusBannerText = std::string("EXPEDITION RISK CHARTER: ") + getRiskContractName(activeRiskContractIdx) +
                       " — " + getRiskContractDesc(activeRiskContractIdx);
    statusBannerTimer = 4.2f;
}

const char* World::getRiskContractName(int idx) {
    switch (idx % 4) {
        case 0: return "Standard Guild Charter (1.0x Bounty)";
        case 1: return "Tempest Squall Charter (1.35x Bounty)";
        case 2: return "Glass-Cannon Broadside (1.50x Bounty)";
        case 3: return "Apex Elder Hunt Charter (1.75x Bounty)";
    }
    return "Standard Guild Charter";
}

const char* World::getRiskContractDesc(int idx) {
    switch (idx % 4) {
        case 0: return "Balanced wind currents, standard hull integrity, 1.0x Gold & Aetherium rewards.";
        case 1: return "Turbulent Ion Squalls & +35% Gold/Aetherium bounty payouts.";
        case 2: return "+35% Player Broadside Cannon Damage, +25% Hull Damage Taken, 1.50x Bounty.";
        case 3: return "Elder Leviathans regenerate barriers faster, 1.75x Gold & Aetherium rewards.";
    }
    return "";
}

float World::getRiskBountyMultiplier() const {
    switch (activeRiskContractIdx % 4) {
        case 1: return 1.35f;
        case 2: return 1.50f;
        case 3: return 1.75f;
        default: return 1.0f;
    }
}

int World::discoveredWonderCount() const {
    int c = 0;
    for (const auto& w : skyWonders) {
        if (w.discovered) c++;
    }
    return c;
}

int World::recoveredLoreLogCount() const {
    int c = 0;
    for (const auto& l : loreLogs) {
        if (l.recovered) c++;
    }
    return c;
}

void World::initSkyEcosystemAndWonders() {
    skyRayBoids.clear();
    skyJellies.clear();
    skyWonders.clear();
    loreLogs.clear();

    // 1. Spawn 36 Cohesive 3D Sky-Ray Boids in 3 Flocks across the Archipelago
    const Vec3 flockCenters[3] = {
        {95.0f, 152.0f, -210.0f},
        {-210.0f, 158.0f, -140.0f},
        {260.0f, 148.0f, 185.0f}
    };
    const ColorRGB flockColors[3] = {
        {75, 210, 245},
        {135, 185, 255},
        {245, 185, 95}
    };
    for (int f = 0; f < 3; ++f) {
        for (int i = 0; i < 12; ++i) {
            SkyRayBoid b;
            float ang = static_cast<float>(i) * (kTwoPi / 12.0f);
            b.homeCenter = flockCenters[f];
            b.position = flockCenters[f] + Vec3{std::cos(ang) * 28.0f, static_cast<float>(i % 4) * 4.5f, std::sin(ang) * 28.0f};
            b.velocity = Vec3{-std::sin(ang) * 18.0f, 0.0f, std::cos(ang) * 18.0f};
            b.wingPhase = static_cast<float>(i) * 0.65f;
            b.scale = 2.1f + (i % 3) * 0.45f;
            b.flockId = f;
            b.color = flockColors[f];
            skyRayBoids.push_back(b);
        }
    }

    // 2. Spawn 24 Bioluminescent Aether-Medusa Jellyfish near Obelisks & Spires
    for (int i = 0; i < 24; ++i) {
        SkyJellyfish j;
        Vec3 anchor = !archipelago.obelisks.empty()
            ? archipelago.obelisks[static_cast<size_t>(i) % archipelago.obelisks.size()].position
            : Vec3{0.0f, 140.0f, -240.0f};
        float ang = static_cast<float>(i) * 1.15f;
        float dist = 34.0f + static_cast<float>(i % 5) * 14.0f;
        j.position = anchor + Vec3{std::cos(ang) * dist, 18.0f + static_cast<float>(i % 4) * 7.0f, std::sin(ang) * dist};
        j.baseAltitude = j.position.y;
        j.pulsePhase = static_cast<float>(i) * 0.8f;
        j.radius = 2.2f + static_cast<float>(i % 3) * 0.65f;
        j.bellColor = (i % 2 == 0) ? ColorRGB{95, 240, 255} : ColorRGB{215, 130, 255};
        j.tentacleColor = (i % 2 == 0) ? ColorRGB{235, 135, 255} : ColorRGB{95, 245, 215};
        skyJellies.push_back(j);
    }

    // 3. Populate 10 Monumental Sky-Wonders across the Archipelago
    struct WonderTemplate {
        const char* name;
        const char* sub;
        const char* lore;
        Vec3 pos;
        int style;
        ColorRGB col;
    };
    const WonderTemplate kWonderDefs[10] = {
        {"The Sun-Dial Colossus", "Ancient Orichalcum Horizon Ring",
         "Built by the First Navigators to align sky-currents with the vernal equinox.",
         {115.0f, 152.0f, -185.0f}, 0, {255, 215, 85}},
        {"Whispering Crystal Arch", "Harmonic Aetherium Natural Bridge",
         "Wind passing through its hollow quartz pillars hums in a perfect C-minor chord.",
         {-175.0f, 148.0f, -245.0f}, 1, {85, 245, 255}},
        {"Titan's Anchor Chains", "Relic of the Sunken Continent",
         "Three cyclopean iron links suspended by anti-gravity lodestones above the abyss.",
         {295.0f, 144.0f, -95.0f}, 2, {255, 155, 75}},
        {"Astrolabe of Vael", "Celestial Gyro-Sanctuary",
         "Concentric brass rings that still track the migration of Elder Sky-Leviathans.",
         {-290.0f, 160.0f, 110.0f}, 3, {215, 135, 255}},
        {"Sunken Cathedral of Orias", "Ruined Cloud-Spire Basilica",
         "Its stained-glass rose window was shattered when Cetus Aetheria first breached.",
         {65.0f, 156.0f, 275.0f}, 0, {115, 235, 255}},
        {"Emerald Cascade Sanctuary", "Hanging Waterfall Atoll",
         "Meltwater from high-altitude ice clouds feeds an eternal mist waterfall.",
         {-95.0f, 142.0f, -445.0f}, 1, {95, 255, 175}},
        {"Boreal Leviathan Graveyard", "Fossilized Sky-Whale Ribs",
         "Where ancient sky-whales drift when their internal Aether-bladders finally cool.",
         {215.0f, 164.0f, -590.0f}, 2, {155, 235, 255}},
        {"Crown of the Storm-King", "Fulmen Fulmination Spire",
         "Perpetually struck by upward-forking lightning from the lower cloud sea.",
         {-355.0f, 158.0f, -310.0f}, 3, {255, 195, 75}},
        {"Observatory of the First Fleet", "brass-Domed Star Chart",
         "Contains the original copper plates charting the Maelstrom at the rim of the world.",
         {410.0f, 146.0f, 220.0f}, 0, {255, 225, 115}},
        {"The Singing Monolith", "Apex Resonance Pillar",
         "A black basalt needle whose vibrations calm nearby storm squalls.",
         {0.0f, 165.0f, -860.0f}, 1, {255, 95, 155}}
    };
    for (int i = 0; i < 10; ++i) {
        SkyWonder w;
        w.id = i + 1;
        w.name = kWonderDefs[i].name;
        w.subtitle = kWonderDefs[i].sub;
        w.loreText = kWonderDefs[i].lore;
        w.position = kWonderDefs[i].pos;
        w.styleVariant = kWonderDefs[i].style;
        w.accentColor = kWonderDefs[i].col;
        w.discovered = false;
        skyWonders.push_back(w);
    }

    // 4. Populate 12 Recoverable Expedition Lore Logs
    struct LogTemplate {
        const char* title;
        const char* author;
        const char* excerpt;
        Vec3 pos;
    };
    const LogTemplate kLogDefs[12] = {
        {"Log I: The Great Upwelling", "Grand Admiral Alistair Vance",
         "When the terrestrial seas boiled into the firmament, only vessels fitted with Aetherium lift-bladders survived the ascent.",
         {52.0f, 138.0f, -155.0f}},
        {"Log II: Anatomy of Cetus Aetheria", "Chief Naturalist Lyra",
         "The Azure Sky-Whale weaves a harmonic song-shell around its flanks. Only an Acoustic Sonar pulse or harpoon barb can fracture the harmony.",
         {145.0f, 144.0f, -275.0f}},
        {"Log III: Fulmen's Static Fury", "Captain Kaelen of the Iron Sparrow",
         "Do not fire broadsides blind into the Crimson Wyrm's static shroud! Ground it with twin harpoon cables or an EMP Nova first.",
         {-245.0f, 148.0f, -345.0f}},
        {"Log IV: Shadows in the Cirrus", "Lookout Officer Sera",
         "Raja Vorago drinks the sunlight itself. Keep the Bow Searchlight [L] trained on its wings to burn away its Abyssal Umbra Cloak!",
         {430.0f, 132.0f, 245.0f}},
        {"Log V: The Boreal Permafrost", "Master Gunner Torin",
         "The Cryo-Kraken's ice carapace deflects light shot. Lure it near a thermal updraft geyser or drop a Sky-Mine [X] in its wake!",
         {215.0f, 158.0f, -660.0f}},
        {"Log VI: Quetzal's Solar Vents", "Engineer Lin",
         "Ignis Quetzal vents superheated plasma through its tail segments. Aim for the rear exhaust vents to quench its Corona Mantle!",
         {-385.0f, 150.0f, 275.0f}},
        {"Log VII: The Maelstrom Sovereign", "Last Transmission of the Vanguard",
         "Apex Charybdis anchors the storm at Z=-1080. Only a multi-tethered harpoon reel combined with Sonar or EMP can collapse its Vortex Aegis.",
         {-35.0f, 154.0f, -940.0f}},
        {"Log VIII: Orichalcum Overclocking", "Master Shipwright Orin",
         "By infusing refined Aetherium directly into timber and iron ribs, a single frigate can withstand the jaw-crush of an Elder Leviathan.",
         {-115.0f, 138.0f, -125.0f}},
        {"Log IX: The Singing Obelisks", "Scholar-Navigator Lyra",
         "Six monoliths form a protective ward across the archipelago. Attuning all six awakens the Astral Convergence shield.",
         {190.0f, 142.0f, 85.0f}},
        {"Log X: Corsair Dread-Arks", "Commodore Helena",
         "Renegade sky-pirates have lashed three dreadnought hulls together into a floating fortress. Strike their outer turbines to cripple their turn rate.",
         {-205.0f, 136.0f, 175.0f}},
        {"Log XI: Star-Aether Meteoroids", "Astronomer Vael",
         "Falling star-shards hover at the buoyancy equilibrium layer before dissolving. Tractor-grapplers can harvest their pure Aether cores.",
         {290.0f, 155.0f, -280.0f}},
        {"Log XII: New Horizon Covenant", "Council of Sky-Captains",
         "So long as the Drydock beacons burn at Haven Harbor, no storm shall claim the last children of the sky.",
         {0.0f, 142.0f, -95.0f}}
    };
    for (int i = 0; i < 12; ++i) {
        LoreLogEntry l;
        l.id = i + 1;
        l.title = kLogDefs[i].title;
        l.author = kLogDefs[i].author;
        l.excerpt = kLogDefs[i].excerpt;
        l.position = kLogDefs[i].pos;
        l.recovered = false;
        loreLogs.push_back(l);
    }
}

void World::stepSkyEcosystemAndWonders(float dt) {
    // 1. Step 3D Sky-Ray Boid Flocks
    for (size_t i = 0; i < skyRayBoids.size(); ++i) {
        SkyRayBoid& b = skyRayBoids[i];
        b.wingPhase += dt * 4.6f;

        Vec3 sep{0.0f, 0.0f, 0.0f};
        Vec3 ali{0.0f, 0.0f, 0.0f};
        Vec3 coh{0.0f, 0.0f, 0.0f};
        int neighbors = 0;

        for (size_t j = 0; j < skyRayBoids.size(); ++j) {
            if (i == j || skyRayBoids[j].flockId != b.flockId) continue;
            Vec3 diff = b.position - skyRayBoids[j].position;
            float d = diff.length();
            if (d < 42.0f && d > 0.01f) {
                neighbors++;
                ali += skyRayBoids[j].velocity;
                coh += skyRayBoids[j].position;
                if (d < 11.0f) {
                    sep += diff / d;
                }
            }
        }

        Vec3 steer{0.0f, 0.0f, 0.0f};
        if (neighbors > 0) {
            float invN = 1.0f / static_cast<float>(neighbors);
            ali = (ali * invN).normalized() * 20.0f;
            Vec3 center = coh * invN;
            Vec3 toCenter = (center - b.position).normalized() * 18.0f;
            steer += (ali - b.velocity) * 0.8f + (toCenter - b.velocity) * 0.6f + sep * 14.0f;
        }

        // Gentle orbit around homeCenter
        Vec3 toHome = b.homeCenter - b.position;
        Vec3 tangent{-toHome.z, 0.0f, toHome.x};
        steer += toHome * 0.18f + tangent.normalized() * 8.0f;
        steer.y += (b.homeCenter.y + std::sin(b.wingPhase * 0.35f) * 8.0f - b.position.y) * 0.8f;

        b.velocity += steer * dt;
        float spd = b.velocity.length();
        if (spd > 24.0f) b.velocity = (b.velocity / spd) * 24.0f;
        else if (spd < 11.0f) b.velocity = b.velocity.normalized() * 11.0f;

        b.position += b.velocity * dt;
    }

    // 2. Step Bioluminescent Aether-Medusae
    for (auto& j : skyJellies) {
        j.pulsePhase += dt * 2.2f;
        j.position.y = j.baseAltitude + std::sin(j.pulsePhase) * 3.2f;
    }

    if (playerShip.isDestroyed) return;

    Vec3 shipFwd = playerShip.orientationQuat().forward();

    // 3. Bow Searchlight Counterplay on Obsidian Dread-Manta & Thermal Geyser Counterplay on Cryo-Kraken
    for (auto& lev : leviathans) {
        if (lev.isSlain || lev.segments.empty()) continue;

        // Bow Searchlight burns away Raja Vorago's Abyssal Umbra Cloak!
        if (playerSearchlightOn && lev.species() == LeviathanSpecies::ObsidianDreadManta && lev.barrierActive) {
            Vec3 toBoss = lev.segments[0].position - playerShip.position;
            float dist = toBoss.length();
            if (dist < 360.0f && dist > 1.0f) {
                float coneDot = Vec3::dot(toBoss / dist, shipFwd);
                if (coneDot > 0.78f) {
                    if (lev.triggerCounterplayDisruption("BOW SEARCHLIGHT ILLUMINATION")) {
                        pendingAudioEvents.push_back({SimAudioEventType::TeslaDischarge, lev.segments[0].position, 1.2f});
                        callouts.push_back({
                            lev.segments[0].position + Vec3{0.0f, 14.0f, 0.0f},
                            "SEARCHLIGHT BURNED UMBRA CLOAK! CORE EXPOSED (8.5s)!",
                            {255, 235, 95},
                            2.8f,
                            2.8f
                        });
                        statusBannerText = "BOW SEARCHLIGHT COUNTERPLAY! Raja Vorago's Abyssal Umbra Cloak burned away — Weak Cores Exposed!";
                        statusBannerTimer = 4.5f;
                    }
                }
            }
        }

        // Thermal Updraft Geyser cracks Glacial Cryo-Kraken's Permafrost Carapace!
        if (lev.species() == LeviathanSpecies::GlacialCryoKraken && lev.barrierActive) {
            for (const auto& gey : archipelago.updrafts) {
                float dx = lev.segments[0].position.x - gey.basePosition.x;
                float dz = lev.segments[0].position.z - gey.basePosition.z;
                if (dx * dx + dz * dz <= (gey.radius + 45.0f) * (gey.radius + 45.0f)) {
                    if (lev.triggerCounterplayDisruption("THERMAL GEYSER MELTDOWN")) {
                        callouts.push_back({
                            lev.segments[0].position + Vec3{0.0f, 14.0f, 0.0f},
                            "THERMAL GEYSER SHATTERED PERMAFROST CARAPACE!",
                            {255, 195, 75},
                            2.8f,
                            2.8f
                        });
                        statusBannerText = "THERMAL COUNTERPLAY! Superheated updraft shattered the Cryo-Kraken's Permafrost Carapace!";
                        statusBannerTimer = 4.5f;
                    }
                    break;
                }
            }
        }
    }

    // 4. Proximity & Searchlight Discovery of 10 Sky-Wonders & 12 Expedition Lore Logs
    for (auto& w : skyWonders) {
        if (w.discovered) continue;
        Vec3 toW = w.position - playerShip.position;
        float d = toW.length();
        bool inBeam = playerSearchlightOn && d < 280.0f && d > 1.0f && Vec3::dot(toW / d, shipFwd) > 0.85f;
        if (d <= 165.0f || inBeam) {
            w.discovered = true;
            playerGold += 180;
            playerAether += 8;
            pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, w.position, 1.3f});
            callouts.push_back({
                w.position + Vec3{0.0f, 16.0f, 0.0f},
                "SKY-WONDER DISCOVERED: " + w.name + " (+180G +8Ae)",
                {255, 225, 85},
                3.2f,
                3.2f
            });
            statusBannerText = "SKY-WONDER CHARTED [" + std::to_string(discoveredWonderCount()) + "/10]: " +
                               w.name + " — " + w.loreText + " (+180G, +8 Ae)";
            statusBannerTimer = 5.5f;
        }
    }

    for (auto& log : loreLogs) {
        if (log.recovered) continue;
        float d = (log.position - playerShip.position).length();
        if (d <= 145.0f) {
            log.recovered = true;
            playerGold += 90;
            playerAether += 4;
            pendingAudioEvents.push_back({SimAudioEventType::CratePickup, log.position, 1.2f});
            callouts.push_back({
                log.position + Vec3{0.0f, 10.0f, 0.0f},
                "EXPEDITION LOG [" + std::to_string(recoveredLoreLogCount()) + "/12]: " + log.title,
                {125, 240, 255},
                3.0f,
                3.0f
            });
            statusBannerText = "RECOVERED ARMADA LOG [" + std::to_string(recoveredLoreLogCount()) + "/12] " +
                               log.title + ": \"" + log.excerpt + "\"";
            statusBannerTimer = 5.5f;
        }
    }
}

} // namespace aethernaut
