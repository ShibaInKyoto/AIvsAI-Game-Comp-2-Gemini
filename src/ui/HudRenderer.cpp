#include "ui/HudRenderer.h"
#include "sim/ShipBlueprints.h"
#include "sim/Ballistics.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace aethernaut {

namespace {

bool isInFrontOfCamera(const Camera3D& cam, const Vec3& worldPos) {
    Vec3 camPos{cam.position.x, cam.position.y, cam.position.z};
    Vec3 camTarget{cam.target.x, cam.target.y, cam.target.z};
    Vec3 camFwd = (camTarget - camPos).normalized();
    return Vec3::dot(worldPos - camPos, camFwd) > 1.0f;
}

} // namespace

void HudRenderer::drawFlightHUD(
    World& world,
    const Camera3D& camera,
    int screenW,
    int screenH,
    bool canDockAtPort,
    const std::string& nearestPortName,
    bool& outRequestOpenDrydock
) {
    const Airship& ship = world.playerShip;
    char buf[160];

    // -------------------------------------------------------------------------
    // 1. 3D Target Brackets & Ballistic Lead Aim Pips (Leviathans & Enemy Ships)
    // -------------------------------------------------------------------------
    for (const auto& lev : world.leviathans) {
        if (lev.isSlain || lev.segments.empty()) continue;
        Vec3 headPos = lev.segments[0].position;
        float dist = (headPos - ship.position).length();
        if (dist > 1350.0f || !isInFrontOfCamera(camera, headPos)) continue;

        Vector2 scrHead = GetWorldToScreen({headPos.x, headPos.y + 12.0f, headPos.z}, camera);
        if (scrHead.x >= 20 && scrHead.x <= screenW - 20 && scrHead.y >= 20 && scrHead.y <= screenH - 20) {
            std::snprintf(buf, sizeof(buf), "%s [%.0fm]%s",
                          lev.name().c_str(), dist, (world.sonarMarkTimer > 0.0f) ? " [SONAR MARKED +25%]" : "");
            DrawText(buf, static_cast<int>(scrHead.x - 65), static_cast<int>(scrHead.y - 26), 14, Color{255, 205, 85, 255});

            // Boss Health Bar above Leviathan
            float hpPct = std::clamp(lev.totalHp / std::max(1.0f, lev.maxTotalHp), 0.0f, 1.0f);
            DrawRectangle(static_cast<int>(scrHead.x - 65), static_cast<int>(scrHead.y - 8), 130, 7, Color{15, 22, 34, 210});
            DrawRectangle(static_cast<int>(scrHead.x - 65), static_cast<int>(scrHead.y - 8), static_cast<int>(130 * hpPct), 7, Color{245, 75, 95, 255});
            DrawRectangleLines(static_cast<int>(scrHead.x - 65), static_cast<int>(scrHead.y - 8), 130, 7, Color{255, 215, 115, 200});

            // 3D Ballistic Lead Aim Pip (when within firing range)
            if (dist < 480.0f) {
                Vec3 leadWorld = computeBallisticLeadPoint(ship.position, ship.velocity, 185.0f, headPos, lev.velocity);
                if (isInFrontOfCamera(camera, leadWorld)) {
                    Vector2 scrLead = GetWorldToScreen({leadWorld.x, leadWorld.y, leadWorld.z}, camera);
                    DrawLineEx(scrHead, scrLead, 1.5f, Color{255, 195, 65, 150});
                    DrawCircleLines(static_cast<int>(scrLead.x), static_cast<int>(scrLead.y), 11.0f, Color{255, 215, 55, 235});
                    DrawCircleLines(static_cast<int>(scrLead.x), static_cast<int>(scrLead.y), 3.0f, Color{255, 95, 65, 255});
                }
            }
        }
    }

    for (const auto& npc : world.npcShips) {
        if (npc.isDestroyed) continue;
        float dist = (npc.position - ship.position).length();
        if (dist > 950.0f || !isInFrontOfCamera(camera, npc.position)) continue;

        Vector2 scrPos = GetWorldToScreen({npc.position.x, npc.position.y + 8.0f, npc.position.z}, camera);
        bool isConvoy = (npc.id() == world.convoyShipId);
        Color tagCol = isConvoy
            ? Color{255, 215, 75, 255}
            : ((npc.faction() == 1) ? Color{255, 95, 95, 255} : Color{85, 245, 165, 255});
        std::snprintf(buf, sizeof(buf), "%s [%.0fm]%s", npc.name().c_str(), dist, isConvoy ? " [+180G ESCORT]" : "");
        DrawText(buf, static_cast<int>(scrPos.x - 55), static_cast<int>(scrPos.y - 22), 12, tagCol);

        float hpPct = std::clamp(npc.totalHp / std::max(1.0f, npc.totalMaxHp), 0.0f, 1.0f);
        DrawRectangle(static_cast<int>(scrPos.x - 45), static_cast<int>(scrPos.y - 7), 90, 5, Color{12, 18, 28, 200});
        DrawRectangle(static_cast<int>(scrPos.x - 45), static_cast<int>(scrPos.y - 7), static_cast<int>(90 * hpPct), 5, tagCol);

        if (npc.faction() == 1 && dist < 420.0f) {
            Vec3 leadWorld = computeBallisticLeadPoint(ship.position, ship.velocity, 185.0f, npc.position, npc.velocity);
            if (isInFrontOfCamera(camera, leadWorld)) {
                Vector2 scrLead = GetWorldToScreen({leadWorld.x, leadWorld.y, leadWorld.z}, camera);
                DrawCircleLines(static_cast<int>(scrLead.x), static_cast<int>(scrLead.y), 9.0f, Color{255, 115, 85, 225});
            }
        }
    }

    // 1A. 3D Target Brackets for Fortified Stronghold Sky-Bastion Flak Towers & Slipstream Rings
    for (const auto& isl : world.archipelago.islands) {
        if (!isl.isFortifiedStronghold || isl.isLiberated) continue;
        float islDist = (isl.position - ship.position).length();
        if (islDist > 750.0f) continue;
        for (const auto& tur : isl.bastionTurrets) {
            if (tur.destroyed || !isInFrontOfCamera(camera, tur.worldPos)) continue;
            Vector2 scrTur = GetWorldToScreen({tur.worldPos.x, tur.worldPos.y + 8.5f, tur.worldPos.z}, camera);
            float hpPct = std::clamp(tur.hp / std::max(1.0f, tur.maxHp), 0.0f, 1.0f);
            DrawText("BASTION FLAK", static_cast<int>(scrTur.x - 38), static_cast<int>(scrTur.y - 18), 10, Color{255, 145, 75, 255});
            DrawRectangle(static_cast<int>(scrTur.x - 36), static_cast<int>(scrTur.y - 6), 72, 5, Color{15, 20, 30, 210});
            DrawRectangle(static_cast<int>(scrTur.x - 36), static_cast<int>(scrTur.y - 6), static_cast<int>(72.0f * hpPct), 5, Color{255, 95, 65, 255});
        }
    }

    for (const auto& ring : world.archipelago.slipstreams) {
        if (ring.cooldownTimer > 0.0f) continue;
        float rDist = (ring.position - ship.position).length();
        if (rDist > 340.0f || !isInFrontOfCamera(camera, ring.position)) continue;
        Vector2 scrRing = GetWorldToScreen({ring.position.x, ring.position.y + ring.radius + 4.5f, ring.position.z}, camera);
        std::snprintf(buf, sizeof(buf), "SLIPSTREAM GATE [%.0fm]", rDist);
        DrawText(buf, static_cast<int>(scrRing.x - 62), static_cast<int>(scrRing.y - 10), 10, Color{85, 245, 255, 225});
    }

    for (const auto& met : world.meteors) {
        if (!met.active) continue;
        float mDist = (met.position - ship.position).length();
        if (mDist > 420.0f || !isInFrontOfCamera(camera, met.position)) continue;
        Vector2 scrMet = GetWorldToScreen({met.position.x, met.position.y + met.radius + 5.0f, met.position.z}, camera);
        std::snprintf(buf, sizeof(buf), "STAR-AETHER METEOR [%.0fm] (+14Ae)", mDist);
        DrawText(buf, static_cast<int>(scrMet.x - 78), static_cast<int>(scrMet.y - 12), 11, Color{225, 135, 255, 240});
    }

    for (const auto& pod : world.survivorPods) {
        if (pod.rescued) continue;
        float pDist = (pod.position - ship.position).length();
        if (pDist > 420.0f || !isInFrontOfCamera(camera, pod.position)) continue;
        Vector2 scrPod = GetWorldToScreen({pod.position.x, pod.position.y + 8.5f, pod.position.z}, camera);
        std::snprintf(buf, sizeof(buf), "SOS LIFEBOAT [%.0fm] (+140G +5Ae)", pDist);
        DrawText(buf, static_cast<int>(scrPod.x - 72), static_cast<int>(scrPod.y - 12), 11, Color{255, 215, 75, 245});
    }

    // Render 3D Floating Combat Callouts (Crits, Shattered Armor, Hull Fractures)
    for (const auto& co : world.callouts) {
        if (!isInFrontOfCamera(camera, co.worldPos)) continue;
        Vector2 scrCo = GetWorldToScreen({co.worldPos.x, co.worldPos.y, co.worldPos.z}, camera);
        float alphaPct = std::clamp(co.lifetime / std::max(0.1f, co.maxLifetime), 0.0f, 1.0f);
        uint8_t a = static_cast<uint8_t>(alphaPct * 255.0f);
        int tw = MeasureText(co.text.c_str(), 14);
        DrawRectangle(static_cast<int>(scrCo.x - tw / 2 - 5), static_cast<int>(scrCo.y - 2), tw + 10, 18, Color{8, 14, 24, static_cast<uint8_t>(a * 0.7f)});
        DrawText(co.text.c_str(), static_cast<int>(scrCo.x - tw / 2), static_cast<int>(scrCo.y), 14, Color{co.color.r, co.color.g, co.color.b, a});
    }

    // -------------------------------------------------------------------------
    // 1B. Top-Left Active Bounty Contract, Fleet Order, Weather & Relics Card
    // -------------------------------------------------------------------------
    DrawRectangle(18, 18, 376, 112, Color{11, 18, 30, 228});
    DrawRectangleLinesEx({18.0f, 18.0f, 376.0f, 112.0f}, 1.5f, Color{65, 175, 225, 180});

    const Leviathan* nextLev = nullptr;
    float nextLevDist = 1e30f;
    for (const auto& lev : world.leviathans) {
        if (!lev.isSlain && !lev.segments.empty()) {
            float d = (lev.segments[0].position - ship.position).length();
            if (d < nextLevDist) {
                nextLevDist = d;
                nextLev = &lev;
            }
        }
    }
    if (nextLev) {
        std::snprintf(buf, sizeof(buf), "ACTIVE HUNT: %s [%.0fm]", nextLev->name().c_str(), nextLevDist);
        DrawText(buf, 28, 25, 14, Color{255, 205, 75, 255});
        std::snprintf(buf, sizeof(buf), "Bounty: +%d Gold, +%d Aetherium | Shatter Armor -> Core",
                      nextLev->goldBounty, nextLev->aetherBounty);
        DrawText(buf, 28, 44, 11, Color{195, 220, 240, 255});
    } else {
        DrawText("ALL ELDER SKY-LEVIATHANS CONQUERED!", 28, 25, 14, Color{85, 245, 165, 255});
        DrawText("Press [N] to Summon Prestige Apex Leviathan!", 28, 44, 11, Color{255, 215, 95, 255});
    }

    int activeAllies = 0;
    for (const auto& n : world.npcShips) {
        if (!n.isDestroyed && n.faction() == 0) activeAllies++;
    }
    std::snprintf(buf, sizeof(buf), "Wingmen: %d | [Z] Order: %s",
                  activeAllies, World::getFleetOrderName(world.currentFleetOrder));
    DrawText(buf, 28, 63, 11, Color{95, 235, 255, 255});

    std::snprintf(buf, sizeof(buf), "[T] %s | Relics [L]: %d/5 | Medals [O]: %d/10",
                  World::getWeatherName(world.currentWeather), world.unlockedRelicCount(), world.earnedMedalCount());
    DrawText(buf, 28, 81, 11, Color{135, 240, 195, 255});

    DrawText("[J] Market | [L] Relics | [O] Medals | [K] Crew | [Y] Livery", 28, 99, 10, Color{255, 215, 95, 255});

    // -------------------------------------------------------------------------
    // 2. Center Aim Crosshair Reticle
    // -------------------------------------------------------------------------
    Vector2 mouse = GetMousePosition();
    DrawCircleLines(static_cast<int>(mouse.x), static_cast<int>(mouse.y), 14.0f, Color{95, 235, 255, 210});
    DrawLine(static_cast<int>(mouse.x - 20), static_cast<int>(mouse.y), static_cast<int>(mouse.x - 8), static_cast<int>(mouse.y), Color{95, 235, 255, 210});
    DrawLine(static_cast<int>(mouse.x + 8), static_cast<int>(mouse.y), static_cast<int>(mouse.x + 20), static_cast<int>(mouse.y), Color{95, 235, 255, 210});
    DrawLine(static_cast<int>(mouse.x), static_cast<int>(mouse.y - 20), static_cast<int>(mouse.x), static_cast<int>(mouse.y - 8), Color{95, 235, 255, 210});
    DrawLine(static_cast<int>(mouse.x), static_cast<int>(mouse.y + 8), static_cast<int>(mouse.x), static_cast<int>(mouse.y + 20), Color{95, 235, 255, 210});

    // -------------------------------------------------------------------------
    // 3. Bottom-Left Airship Structural Voxel Schematic & Flight Vitals
    // -------------------------------------------------------------------------
    int panelX = 18;
    int panelY = screenH - 224;
    DrawRectangle(panelX, panelY, 385, 206, Color{11, 18, 30, 230});
    DrawRectangleLinesEx({static_cast<float>(panelX), static_cast<float>(panelY), 385.0f, 206.0f}, 1.5f, Color{65, 175, 225, 185});

    // Live 2D Top-Down Voxel Integrity Doll on Left of Panel
    int dollCenterX = panelX + 68;
    int dollCenterY = panelY + 108;
    DrawRectangle(panelX + 12, panelY + 32, 112, 158, Color{6, 11, 20, 230});
    DrawRectangleLines(panelX + 12, panelY + 32, 112, 158, Color{45, 95, 135, 180});
    DrawText("HULL GRID", panelX + 34, panelY + 12, 12, Color{135, 205, 245, 255});

    for (int gz = Airship::minZ(); gz <= Airship::maxZ(); ++gz) {
        for (int gx = Airship::minX(); gx <= Airship::maxX(); ++gx) {
            float maxRatio = -1.0f;
            bool onFire = false;
            for (int gy = Airship::minY(); gy <= Airship::maxY(); ++gy) {
                const VoxelCell& c = ship.getCell(gx, gy, gz);
                if (c.id != BlockId::Empty) {
                    float r = (c.maxHp > 0.0f) ? (c.hp / c.maxHp) : 1.0f;
                    maxRatio = std::max(maxRatio, r);
                    if (c.fireTimer > 0.0f) onFire = true;
                }
            }
            if (maxRatio >= 0.0f) {
                Color cellCol = onFire
                    ? Color{255, 125, 35, 255}
                    : Color{
                        static_cast<uint8_t>((1.0f - maxRatio) * 245.0f),
                        static_cast<uint8_t>(maxRatio * 235.0f),
                        95, 255
                    };
                DrawRectangle(dollCenterX + gx * 6 - 2, dollCenterY + gz * 6 - 2, 5, 5, cellCol);
            }
        }
    }

    // Flight Vitals & Telemetry Readout on Right of Panel
    int vitX = panelX + 136;
    DrawText(ship.name().c_str(), vitX, panelY + 10, 16, Color{85, 235, 255, 255});

    float hullPct = std::clamp(ship.totalHp / std::max(1.0f, ship.totalMaxHp), 0.0f, 1.0f);
    std::snprintf(buf, sizeof(buf), "Hull: %.0f / %.0f (%.0f%%)", ship.totalHp, ship.totalMaxHp, hullPct * 100.0f);
    DrawText(buf, vitX, panelY + 30, 12, WHITE);
    DrawRectangle(vitX, panelY + 46, 230, 7, Color{22, 32, 48, 255});
    DrawRectangle(vitX, panelY + 46, static_cast<int>(230 * hullPct), 7,
                  (hullPct > 0.45f) ? Color{65, 225, 145, 255} : Color{245, 75, 85, 255});

    // Energy Shield Dome Bar
    float shldPct = std::clamp(ship.shieldHp / std::max(1.0f, ship.maxShieldHp), 0.0f, 1.0f);
    std::snprintf(buf, sizeof(buf), "Aether Shield Dome: %.0f / %.0f", ship.shieldHp, ship.maxShieldHp);
    DrawText(buf, vitX, panelY + 58, 12, Color{95, 235, 255, 255});
    DrawRectangle(vitX, panelY + 74, 230, 6, Color{18, 28, 44, 255});
    DrawRectangle(vitX, panelY + 74, static_cast<int>(230 * shldPct), 6, Color{65, 215, 255, 255});

    float speedKnots = std::sqrt(ship.velocity.x * ship.velocity.x + ship.velocity.z * ship.velocity.z) * 1.94f;
    std::snprintf(buf, sizeof(buf), "Airspeed: %.1f kts%s | Alt: %.0fm",
                  speedKnots, ship.isBoosting ? " [BOOST]" : "", ship.position.y);
    DrawText(buf, vitX, panelY + 86, 12, ship.isBoosting ? Color{255, 215, 75, 255} : Color{195, 225, 255, 255});

    float weightF = ship.totalMass * 3.2f;
    std::snprintf(buf, sizeof(buf), "Buoyancy Lift: %.0f / %.0f kN", ship.totalLift, weightF);
    DrawText(buf, vitX, panelY + 104, 12, ship.isSinking ? Color{255, 85, 85, 255} : Color{115, 245, 185, 255});

    std::snprintf(buf, sizeof(buf), "Treasury: %d Gold  |  %d Aetherium", world.playerGold, world.playerAether);
    DrawText(buf, vitX, panelY + 122, 12, Color{255, 215, 85, 255});

    std::snprintf(buf, sizeof(buf), "Leviathans Slain: %d  |  Pirates Sunk: %d", world.leviathansSlain, world.piratesSunk);
    DrawText(buf, vitX, panelY + 140, 12, Color{215, 175, 255, 255});

    const char* modeText = world.isPlayerWalkingOnDeck
        ? "DECK WALK [TAB: Helm] (Repairing Hull)"
        : (world.playerAutopilot ? "AI AUTOPILOT [P: Manual]" : "HELM [SHIFT: Boost | X: Repair | C: Sonar | G: Mine]");
    DrawText(modeText, vitX, panelY + 160, 11, Color{85, 235, 255, 255});

    // Active Harpoon Tether Banner
    if (!world.tethers.empty()) {
        float maxStrain = 0.0f;
        for (const auto& t : world.tethers) {
            maxStrain = std::max(maxStrain, t.tensionForce / std::max(1.0f, t.maxBreakTension));
        }
        std::snprintf(buf, sizeof(buf), "HARPOON TENSION: %.0f%% ([R] Reel In | [F] Cut)", maxStrain * 100.0f);
        DrawText(buf, vitX, panelY + 180, 11, Color{255, 175, 65, 255});
    } else if (world.slipstreamComboCount > 0) {
        std::snprintf(buf, sizeof(buf), "SLIPSTREAM REGATTA CHAIN: x%d (%.0fs left)", world.slipstreamComboCount, world.slipstreamComboTimer);
        DrawText(buf, vitX, panelY + 180, 11, Color{85, 245, 255, 255});
    }

    // -------------------------------------------------------------------------
    // 3B. Bottom-Right Tactical Ability & Afterburner Status Bar
    // -------------------------------------------------------------------------
    int tacW = 302;
    int tacH = 128;
    int tacX = screenW - tacW - 18;
    int tacY = screenH - tacH - 18;
    DrawRectangle(tacX, tacY, tacW, tacH, Color{11, 18, 30, 228});
    DrawRectangleLinesEx({static_cast<float>(tacX), static_cast<float>(tacY), static_cast<float>(tacW), static_cast<float>(tacH)}, 1.5f, Color{65, 175, 225, 180});

    float boostPct = std::clamp(ship.afterburnerFuel / std::max(1.0f, ship.maxAfterburnerFuel), 0.0f, 1.0f);
    std::snprintf(buf, sizeof(buf), "[SHIFT] Overdrive Capacitor: %.0f%%", boostPct * 100.0f);
    DrawText(buf, tacX + 12, tacY + 9, 12, ship.isBoosting ? Color{255, 215, 75, 255} : Color{185, 225, 255, 255});
    DrawRectangle(tacX + 12, tacY + 24, tacW - 24, 7, Color{20, 32, 48, 255});
    DrawRectangle(tacX + 12, tacY + 24, static_cast<int>((tacW - 24) * boostPct), 7, Color{255, 185, 55, 255});

    if (world.damageControlCooldown <= 0.0f) {
        DrawText("[1/X] Damage Control & Shield Surge: READY", tacX + 12, tacY + 37, 11, Color{85, 245, 165, 255});
    } else {
        std::snprintf(buf, sizeof(buf), "[1/X] Damage Control: Recharging (%.0fs)", world.damageControlCooldown);
        DrawText(buf, tacX + 12, tacY + 37, 11, Color{165, 185, 205, 255});
    }

    if (world.sonarPingCooldown <= 0.0f) {
        DrawText("[2/C] Acoustic Aether-Sonar (+25% Dmg): READY", tacX + 12, tacY + 53, 11, Color{95, 235, 255, 255});
    } else {
        std::snprintf(buf, sizeof(buf), "[2/C] Acoustic Sonar: %s (%.0fs)",
                      (world.sonarMarkTimer > 0.0f) ? "ACTIVE!" : "Cooldown",
                      (world.sonarMarkTimer > 0.0f) ? world.sonarMarkTimer : world.sonarPingCooldown);
        DrawText(buf, tacX + 12, tacY + 53, 11, (world.sonarMarkTimer > 0.0f) ? Color{255, 215, 85, 255} : Color{165, 185, 205, 255});
    }

    if (world.skyMineCooldown <= 0.0f) {
        std::snprintf(buf, sizeof(buf), "[3/G] Magnetic Sky-Mine (260 AoE): READY (%zu)", world.skyMines.size());
        DrawText(buf, tacX + 12, tacY + 69, 11, Color{255, 175, 75, 255});
    } else {
        std::snprintf(buf, sizeof(buf), "[3/G] Magnetic Sky-Mine: Reloading (%.1fs)", world.skyMineCooldown);
        DrawText(buf, tacX + 12, tacY + 69, 11, Color{165, 185, 205, 255});
    }

    if (world.empNovaCooldown <= 0.0f) {
        DrawText("[4] Aether-EMP Shockwave Nova (195m): READY", tacX + 12, tacY + 85, 11, Color{225, 135, 255, 255});
    } else {
        std::snprintf(buf, sizeof(buf), "[4] Aether-EMP Nova: Recharging (%.0fs)", world.empNovaCooldown);
        DrawText(buf, tacX + 12, tacY + 85, 11, Color{165, 185, 205, 255});
    }

    DrawText("[J] Market | [L] Relics | [O] Medals | [I] Raid", tacX + 12, tacY + 104, 11, Color{235, 195, 95, 255});

    // -------------------------------------------------------------------------
    // 4. Top-Right 3D Sky-Radar & Compass Minimap
    // -------------------------------------------------------------------------
    int radRadius = 82;
    int radX = screenW - radRadius - 24;
    int radY = radRadius + 24;
    DrawCircle(radX, radY, static_cast<float>(radRadius), Color{10, 18, 30, 225});
    DrawCircleLines(radX, radY, static_cast<float>(radRadius), Color{65, 185, 235, 210});
    DrawCircleLines(radX, radY, static_cast<float>(radRadius) * 0.5f, Color{45, 115, 155, 120});

    const float radarRange = 1350.0f;
    // Islands on Radar
    for (const auto& isl : world.archipelago.islands) {
        Vec3 rel = isl.position - ship.position;
        float dx = (rel.x / radarRange) * static_cast<float>(radRadius);
        float dz = (rel.z / radarRange) * static_cast<float>(radRadius);
        if (dx * dx + dz * dz < static_cast<float>((radRadius - 6) * (radRadius - 6))) {
            Color ic = isl.hasDrydockPort ? Color{75, 245, 145, 220} : Color{135, 155, 175, 190};
            DrawCircle(radX + static_cast<int>(dx), radY + static_cast<int>(dz), isl.hasDrydockPort ? 4.5f : 3.0f, ic);
        }
    }
    // NPC Ships on Radar
    for (const auto& npc : world.npcShips) {
        if (npc.isDestroyed) continue;
        Vec3 rel = npc.position - ship.position;
        float dx = (rel.x / radarRange) * static_cast<float>(radRadius);
        float dz = (rel.z / radarRange) * static_cast<float>(radRadius);
        if (dx * dx + dz * dz < static_cast<float>((radRadius - 4) * (radRadius - 4))) {
            Color nc = (npc.faction() == 1) ? Color{255, 75, 75, 255} : Color{75, 225, 255, 255};
            DrawCircle(radX + static_cast<int>(dx), radY + static_cast<int>(dz), 3.0f, nc);
        }
    }
    // Leviathans on Radar
    for (const auto& lev : world.leviathans) {
        if (lev.isSlain || lev.segments.empty()) continue;
        Vec3 rel = lev.segments[0].position - ship.position;
        float dx = (rel.x / radarRange) * static_cast<float>(radRadius);
        float dz = (rel.z / radarRange) * static_cast<float>(radRadius);
        if (dx * dx + dz * dz < static_cast<float>((radRadius - 5) * (radRadius - 5))) {
            DrawCircle(radX + static_cast<int>(dx), radY + static_cast<int>(dz), 5.5f, Color{255, 195, 45, 255});
            DrawCircleLines(radX + static_cast<int>(dx), radY + static_cast<int>(dz), 7.5f, Color{255, 85, 85, 255});
        }
    }
    // Player Ship Center Arrow
    DrawCircle(radX, radY, 3.5f, WHITE);
    DrawText("N", radX - 4, radY - radRadius + 4, 12, Color{95, 225, 255, 255});
    DrawText("[M] Map | [B] Drydock | [P] AI Pilot", radX - 95, radY + radRadius + 8, 12, Color{185, 205, 225, 255});

    // -------------------------------------------------------------------------
    // 5. Top Status Notification Banner & Drydock Port Prompt
    // -------------------------------------------------------------------------
    if (world.statusBannerTimer > 0.0f && !world.statusBannerText.empty()) {
        int tw = MeasureText(world.statusBannerText.c_str(), 15);
        int bx = (screenW - tw) / 2 - 18;
        DrawRectangle(bx, 18, tw + 36, 36, Color{12, 22, 38, 225});
        DrawRectangleLines(bx, 18, tw + 36, 36, Color{85, 225, 255, 210});
        DrawText(world.statusBannerText.c_str(), bx + 18, 28, 15, Color{255, 235, 135, 255});
    }

    if (canDockAtPort || world.isSandboxMode) {
        std::snprintf(buf, sizeof(buf), "OPEN 3D DRYDOCK SHIPYARD [B]  |  MARKET [J] (%s)",
                      world.isSandboxMode ? "Sandbox" : nearestPortName.c_str());
        int dtw = MeasureText(buf, 13);
        int dw = dtw + 32;
        Rectangle dockBtn{static_cast<float>((screenW - dw) / 2), static_cast<float>(screenH - 54), static_cast<float>(dw), 38.0f};
        bool hover = CheckCollisionPointRec(mouse, dockBtn);
        DrawRectangleRec(dockBtn, hover ? Color{35, 145, 195, 245} : Color{20, 88, 135, 230});
        DrawRectangleLinesEx(dockBtn, 1.5f, Color{95, 235, 255, 255});
        DrawText(buf, static_cast<int>(dockBtn.x + 16), static_cast<int>(dockBtn.y + 12), 13, WHITE);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            outRequestOpenDrydock = true;
        }
    }
}

TitleAction HudRenderer::drawTitleScreen(
    World& backgroundWorld,
    int screenW,
    int screenH,
    int& inoutSelectedBlueprint
) {
    TitleAction action = TitleAction::None;
    Vector2 mouse = GetMousePosition();

    // Left-anchored Command Deck Menu so the 3D Flagship Turntable & Sky-Archipelago shine on the right
    int panelX = 42;
    int panelY = 38;
    int panelW = 635;
    int panelH = screenH - 76;

    DrawRectangle(panelX, panelY, panelW, panelH, Color{10, 17, 30, 236});
    DrawRectangleLinesEx({static_cast<float>(panelX), static_cast<float>(panelY), static_cast<float>(panelW), static_cast<float>(panelH)}, 2.0f, Color{65, 195, 245, 215});

    DrawText("AETHERNAUT", panelX + 28, panelY + 22, 48, Color{85, 235, 255, 255});
    DrawText("3D SKY-FLEET ARCHITECT & COLOSSAL LEVIATHAN HUNTER", panelX + 30, panelY + 76, 14, Color{255, 205, 85, 255});

    // Ship Hull Blueprint Selector Card
    int cardX = panelX + 26;
    int cardY = panelY + 106;
    int cardW = panelW - 52;
    DrawRectangle(cardX, cardY, cardW, 134, Color{16, 27, 46, 245});
    DrawRectangleLines(cardX, cardY, cardW, 134, Color{55, 155, 205, 190});

    ShipBlueprintInfo bp = ShipBlueprints::getInfo(inoutSelectedBlueprint);
    DrawText("ACTIVE 3D FLAGSHIP BLUEPRINT (12 HULL CLASSES):", cardX + 16, cardY + 12, 12, Color{145, 205, 245, 255});
    DrawText(bp.name, cardX + 16, cardY + 30, 22, WHITE);
    DrawText(bp.role, cardX + 16, cardY + 56, 14, Color{255, 205, 85, 255});
    DrawText(bp.description, cardX + 16, cardY + 80, 11, Color{185, 205, 225, 255});

    char statBuf[128];
    const Airship& s = backgroundWorld.playerShip;
    std::snprintf(statBuf, sizeof(statBuf), "Blocks: %d  |  Mass: %.0ft  |  Shield: %.0f  |  Guns: %d  |  Harpoons: %d",
                  s.occupiedBlockCount, s.totalMass, s.maxShieldHp, s.cannonCount, s.harpoonCount);
    DrawText(statBuf, cardX + 16, cardY + 106, 12, Color{95, 240, 185, 255});

    // Prev / Next Blueprint Buttons
    Rectangle prevBtn{static_cast<float>(cardX + cardW - 162), static_cast<float>(cardY + 12), 70.0f, 32.0f};
    Rectangle nextBtn{static_cast<float>(cardX + cardW - 84), static_cast<float>(cardY + 12), 70.0f, 32.0f};
    DrawRectangleRec(prevBtn, CheckCollisionPointRec(mouse, prevBtn) ? Color{45, 125, 175, 255} : Color{25, 58, 92, 255});
    DrawRectangleRec(nextBtn, CheckCollisionPointRec(mouse, nextBtn) ? Color{45, 125, 175, 255} : Color{25, 58, 92, 255});
    DrawRectangleLinesEx(prevBtn, 1.2f, Color{95, 205, 255, 200});
    DrawRectangleLinesEx(nextBtn, 1.2f, Color{95, 205, 255, 200});
    DrawText("< PREV", static_cast<int>(prevBtn.x + 11), static_cast<int>(prevBtn.y + 10), 12, WHITE);
    DrawText("NEXT >", static_cast<int>(nextBtn.x + 11), static_cast<int>(nextBtn.y + 10), 12, WHITE);

    if (CheckCollisionPointRec(mouse, prevBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        inoutSelectedBlueprint = (inoutSelectedBlueprint + ShipBlueprints::count() - 1) % ShipBlueprints::count();
        ShipBlueprints::applyBlueprint(backgroundWorld.playerShip, inoutSelectedBlueprint);
    }
    if (CheckCollisionPointRec(mouse, nextBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        inoutSelectedBlueprint = (inoutSelectedBlueprint + 1) % ShipBlueprints::count();
        ShipBlueprints::applyBlueprint(backgroundWorld.playerShip, inoutSelectedBlueprint);
    }

    // Mode Launch Buttons
    struct MenuBtn {
        const char* label;
        const char* desc;
        TitleAction act;
        Color tint;
    };
    const MenuBtn buttons[] = {
        {"LAUNCH OPEN-SKY EXPEDITION (CAMPAIGN)", "Explore 14 sky-islands, trade commodities, command wingmen & hunt 6 Elder Leviathans.", TitleAction::StartCampaign, Color{28, 135, 185, 245}},
        {"3D DRYDOCK SHIPYARD & SANDBOX", "Unlimited Gold & Aetherium! Build custom 3D voxel airships with 22 functional block types.", TitleAction::StartDrydockSandbox, Color{35, 145, 105, 245}},
        {"COLOSSAL LEVIATHAN HUNT GAUNTLET", "Immediate boss encounter against multi-segmented Sky-Leviathans with Shock Harpoons.", TitleAction::StartLeviathanHunt, Color{165, 75, 55, 245}},
        {"SKY-ARMADA FLEET SKIRMISH", "Large-scale 3D broadside fleet battle alongside AI wingmen against a Corsair Armada.", TitleAction::StartFleetSkirmish, Color{115, 75, 165, 245}}
    };

    int startY = cardY + 152;
    for (int i = 0; i < 4; ++i) {
        Rectangle r{static_cast<float>(cardX), static_cast<float>(startY + i * 68), static_cast<float>(cardW), 58.0f};
        bool hover = CheckCollisionPointRec(mouse, r);
        Color bg = buttons[i].tint;
        if (hover) {
            bg.r = static_cast<uint8_t>(std::min(255, bg.r + 35));
            bg.g = static_cast<uint8_t>(std::min(255, bg.g + 35));
            bg.b = static_cast<uint8_t>(std::min(255, bg.b + 35));
        }
        DrawRectangleRec(r, bg);
        DrawRectangleLinesEx(r, hover ? 2.5f : 1.5f, hover ? WHITE : Color{115, 215, 255, 190});
        DrawText(buttons[i].label, static_cast<int>(r.x + 16), static_cast<int>(r.y + 10), 16, WHITE);
        DrawText(buttons[i].desc, static_cast<int>(r.x + 16), static_cast<int>(r.y + 34), 11, Color{225, 235, 245, 255});

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            action = buttons[i].act;
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        action = TitleAction::StartCampaign;
    }

    // Right-side 3D Flagship Showcase Badge
    int badgeX = screenW - 395;
    int badgeY = screenH - 74;
    DrawRectangle(badgeX, badgeY, 355, 42, Color{10, 17, 30, 215});
    DrawRectangleLines(badgeX, badgeY, 355, 42, Color{65, 195, 245, 180});
    std::snprintf(statBuf, sizeof(statBuf), "LIVE 3D TURNTABLE: %s", bp.name);
    DrawText(statBuf, badgeX + 16, badgeY + 14, 14, Color{85, 235, 255, 255});

    const char* footer = "[W/S] Throttle | [A/D] Rudder | [SHIFT] Boost | [LMB] Guns | [RMB] Harpoon | [J] Market | [B] Drydock";
    DrawText(footer, panelX + 26, panelY + panelH - 28, 11, Color{155, 195, 225, 255});

    return action;
}

void HudRenderer::drawWorldMapModal(World& world, int screenW, int screenH, bool& inoutShowMap) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 215});
    int mapSize = std::min(screenW, screenH) - 100;
    int mapX = (screenW - mapSize) / 2;
    int mapY = (screenH - mapSize) / 2;

    DrawRectangle(mapX, mapY, mapSize, mapSize, Color{11, 20, 34, 248});
    DrawRectangleLinesEx({static_cast<float>(mapX), static_cast<float>(mapY), static_cast<float>(mapSize), static_cast<float>(mapSize)}, 2.0f, Color{75, 205, 255, 230});

    DrawText("SKY-ARCHIPELAGO NAVIGATION CHART & FAST-TRAVEL [M / ESC to Close]", mapX + 18, mapY + 12, 16, Color{85, 235, 255, 255});
    int ftCost = (world.isSandboxMode || world.unlockedRelics[2]) ? 0 : 45;
    char hdrBuf[160];
    std::snprintf(hdrBuf, sizeof(hdrBuf), "Click any GREEN Allied Port / Liberated Citadel to Fast-Travel (%d Gold) | Green=Port, Orange=Bastion, Violet=Meteor", ftCost);
    DrawText(hdrBuf, mapX + 18, mapY + 32, 11, Color{185, 225, 245, 255});

    int cx = mapX + mapSize / 2;
    int cy = mapY + mapSize / 2 + 14;
    float scale = (static_cast<float>(mapSize) * 0.41f) / 1350.0f;
    Vector2 mouse = GetMousePosition();

    // Render Slipstream Rings & Thermal Updraft Geysers on Map
    for (const auto& ring : world.archipelago.slipstreams) {
        int rx = cx + static_cast<int>(ring.position.x * scale);
        int ry = cy + static_cast<int>(ring.position.z * scale);
        DrawCircleLines(rx, ry, 4.0f, Color{75, 235, 255, 160});
    }
    for (const auto& gey : world.archipelago.updrafts) {
        int gx = cx + static_cast<int>(gey.basePosition.x * scale);
        int gy = cy + static_cast<int>(gey.basePosition.z * scale);
        DrawCircleLines(gx, gy, 5.5f, Color{65, 245, 195, 190});
    }

    // Render Star-Aether Meteoroids & SOS Lifeboat Pods on Map
    for (const auto& met : world.meteors) {
        if (!met.active) continue;
        int mx = cx + static_cast<int>(met.position.x * scale);
        int my = cy + static_cast<int>(met.position.z * scale);
        DrawCircle(mx, my, 5.0f, Color{225, 115, 255, 255});
        DrawText("METEOR", mx + 7, my - 5, 10, Color{235, 155, 255, 255});
    }
    for (const auto& pod : world.survivorPods) {
        if (pod.rescued) continue;
        int sx = cx + static_cast<int>(pod.position.x * scale);
        int sy = cy + static_cast<int>(pod.position.z * scale);
        DrawCircle(sx, sy, 4.5f, Color{65, 245, 215, 255});
        DrawText("SOS", sx + 6, sy - 5, 10, Color{125, 255, 225, 255});
    }
    for (const auto& ob : world.archipelago.obelisks) {
        int ox = cx + static_cast<int>(ob.position.x * scale);
        int oy = cy + static_cast<int>(ob.position.z * scale);
        Color obCol = ob.attuned ? Color{85, 245, 255, 255} : Color{255, 205, 75, 255};
        DrawCircleLines(ox, oy, 6.0f, obCol);
        DrawCircle(ox, oy, 3.0f, obCol);
        DrawText(ob.attuned ? "ATTUNED" : "OBELISK", ox + 8, oy - 5, 9, obCol);
    }

    int hoveredIsland = -1;
    for (size_t i = 0; i < world.archipelago.islands.size(); ++i) {
        const auto& isl = world.archipelago.islands[i];
        int ix = cx + static_cast<int>(isl.position.x * scale);
        int iy = cy + static_cast<int>(isl.position.z * scale);
        float rad = std::max(7.0f, isl.radius * scale * 1.4f);

        bool canWarp = isl.hasDrydockPort || isl.isLiberated;
        Color c = canWarp
            ? Color{75, 245, 155, 255}
            : ((isl.isFortifiedStronghold && !isl.isLiberated) ? Color{255, 135, 55, 255} : Color{145, 175, 195, 255});

        float mouseDist = std::hypot(mouse.x - static_cast<float>(ix), mouse.y - static_cast<float>(iy));
        if (mouseDist <= rad + 8.0f) {
            hoveredIsland = static_cast<int>(i);
            DrawCircleLines(ix, iy, rad + 4.5f, WHITE);
        }

        DrawCircle(ix, iy, rad, c);
        DrawText(isl.name.c_str(), ix + static_cast<int>(rad) + 5, iy - 6, 11, canWarp ? Color{155, 255, 205, 255} : WHITE);
    }

    if (hoveredIsland >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (world.fastTravelToIsland(hoveredIsland)) {
            inoutShowMap = false;
            return;
        }
    }

    for (const auto& lev : world.leviathans) {
        if (lev.isSlain || lev.segments.empty()) continue;
        int lx = cx + static_cast<int>(lev.segments[0].position.x * scale);
        int ly = cy + static_cast<int>(lev.segments[0].position.z * scale);
        DrawCircle(lx, ly, 7.0f, Color{255, 95, 75, 255});
        DrawText(lev.name().c_str(), lx + 10, ly - 6, 12, Color{255, 205, 85, 255});
    }

    // Render Imperial Treasure Galleon Convoy & Route on Map
    for (const auto& npc : world.npcShips) {
        if (npc.isDestroyed || npc.id() != world.convoyShipId) continue;
        int gx = cx + static_cast<int>(npc.position.x * scale);
        int gy = cy + static_cast<int>(npc.position.z * scale);
        if (!world.archipelago.islands.empty()) {
            int tgtIdx = std::clamp(world.convoyTargetIslandIdx, 0, static_cast<int>(world.archipelago.islands.size()) - 1);
            const auto& destIsl = world.archipelago.islands[static_cast<size_t>(tgtIdx)];
            int dx = cx + static_cast<int>(destIsl.position.x * scale);
            int dy = cy + static_cast<int>(destIsl.position.z * scale);
            DrawLine(gx, gy, dx, dy, Color{255, 215, 85, 150});
        }
        DrawCircle(gx, gy, 5.5f, Color{255, 215, 65, 255});
        DrawText("CONVOY", gx + 8, gy - 5, 11, Color{255, 225, 95, 255});
    }

    int px = cx + static_cast<int>(world.playerShip.position.x * scale);
    int py = cy + static_cast<int>(world.playerShip.position.z * scale);
    DrawCircle(px, py, 5.5f, WHITE);
    DrawText("YOU", px + 8, py - 6, 12, Color{85, 245, 255, 255});

    // Bottom Action Bar on Map Modal
    Rectangle summonBtn{static_cast<float>(mapX + 20), static_cast<float>(mapY + mapSize - 46), 228.0f, 34.0f};
    Rectangle arkBtn{static_cast<float>(mapX + 258), static_cast<float>(mapY + mapSize - 46), 228.0f, 34.0f};
    bool hovSum = CheckCollisionPointRec(mouse, summonBtn);
    bool hovArk = CheckCollisionPointRec(mouse, arkBtn);
    DrawRectangleRec(summonBtn, hovSum ? Color{185, 65, 55, 255} : Color{135, 45, 38, 240});
    DrawRectangleRec(arkBtn, hovArk ? Color{165, 55, 115, 255} : Color{115, 38, 82, 240});
    DrawRectangleLinesEx(summonBtn, 1.5f, Color{255, 185, 95, 255});
    DrawRectangleLinesEx(arkBtn, 1.5f, Color{255, 155, 225, 255});
    DrawText("SUMMON APEX LEVIATHAN [N]", static_cast<int>(summonBtn.x + 12), static_cast<int>(summonBtn.y + 11), 12, WHITE);
    DrawText("RAID CORSAIR DREAD-ARK [I]", static_cast<int>(arkBtn.x + 12), static_cast<int>(arkBtn.y + 11), 12, WHITE);
    if (hovSum && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        world.summonApexLeviathan();
        inoutShowMap = false;
    }
    if (hovArk && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        world.spawnCorsairDreadArk();
        inoutShowMap = false;
    }

    if (hoveredIsland >= 0) {
        const auto& isl = world.archipelago.islands[static_cast<size_t>(hoveredIsland)];
        bool canWarp = isl.hasDrydockPort || isl.isLiberated;
        std::snprintf(hdrBuf, sizeof(hdrBuf), "%s — %s",
                      isl.name.c_str(),
                      canWarp ? "CLICK TO FAST-TRAVEL" : "Hostile / Uncharted");
        DrawText(hdrBuf, mapX + 498, mapY + mapSize - 35, 11, canWarp ? Color{95, 255, 175, 255} : Color{255, 175, 95, 255});
    }

    if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowMap = false;
    }
}

void HudRenderer::drawCrewModal(World& world, int screenW, int screenH, bool& inoutShowCrew) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 215});
    int w = 780;
    int h = 520;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{75, 205, 255, 230});

    DrawText("CAPTAIN'S QUARTERMASTER & SPECIALIST OFFICER ROSTER [K / ESC to Close]", mx + 24, my + 18, 17, Color{85, 235, 255, 255});
    char buf[160];
    std::snprintf(buf, sizeof(buf), "Treasury: %d Gold   |   %d Refined Aetherium Crystals", world.playerGold, world.playerAether);
    DrawText(buf, mx + 24, my + 44, 14, Color{255, 215, 85, 255});
    DrawLine(mx + 24, my + 68, mx + w - 24, my + 68, Color{55, 145, 195, 180});

    const auto& specs = World::getOfficerSpecs();
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < 5; ++i) {
        int cardY = my + 82 + i * 82;
        Rectangle card{static_cast<float>(mx + 24), static_cast<float>(cardY), static_cast<float>(w - 48), 72.0f};
        DrawRectangleRec(card, Color{16, 28, 46, 245});
        DrawRectangleLinesEx(card, 1.2f, Color{55, 145, 195, 190});

        int rank = world.officerRanks[static_cast<size_t>(i)];
        std::snprintf(buf, sizeof(buf), "%s  —  %s [RANK %d / 3]", specs[static_cast<size_t>(i)].name, specs[static_cast<size_t>(i)].role, rank);
        DrawText(buf, static_cast<int>(card.x + 16), static_cast<int>(card.y + 12), 15, Color{95, 240, 185, 255});
        DrawText(specs[static_cast<size_t>(i)].bonusDesc, static_cast<int>(card.x + 16), static_cast<int>(card.y + 38), 12, Color{205, 225, 245, 255});

        Rectangle upgBtn{card.x + card.width - 195.0f, card.y + 16.0f, 180.0f, 40.0f};
        bool hover = CheckCollisionPointRec(mouse, upgBtn);
        if (rank < 3) {
            int gCost = specs[static_cast<size_t>(i)].baseGoldCost * (rank + 1);
            int aCost = specs[static_cast<size_t>(i)].baseAetherCost * (rank + 1);
            bool canAfford = world.isSandboxMode || (world.playerGold >= gCost && world.playerAether >= aCost);
            DrawRectangleRec(upgBtn, hover && canAfford ? Color{45, 165, 115, 255} : (canAfford ? Color{28, 125, 85, 240} : Color{45, 55, 68, 220}));
            DrawRectangleLinesEx(upgBtn, 1.5f, canAfford ? Color{95, 245, 175, 255} : Color{110, 120, 135, 180});
            std::snprintf(buf, sizeof(buf), "PROMOTE (%dG + %dAe)", gCost, aCost);
            DrawText(buf, static_cast<int>(upgBtn.x + 14), static_cast<int>(upgBtn.y + 13), 12, WHITE);
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                world.upgradeOfficer(i);
            }
        } else {
            DrawRectangleRec(upgBtn, Color{35, 95, 145, 220});
            DrawRectangleLinesEx(upgBtn, 1.5f, Color{255, 215, 85, 255});
            DrawText("MAX RANK III", static_cast<int>(upgBtn.x + 42), static_cast<int>(upgBtn.y + 13), 13, Color{255, 215, 85, 255});
        }
    }

    if (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowCrew = false;
    }
}

void HudRenderer::drawCodexModal(int screenW, int screenH, bool& inoutShowCodex) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 215});
    int w = 880;
    int h = 580;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{75, 205, 255, 230});

    DrawText("AETHERNAUT ENGINEERING & LEVIATHAN HUNTER'S CODEX [H / ESC to Close]", mx + 24, my + 18, 19, Color{85, 235, 255, 255});
    DrawLine(mx + 24, my + 48, mx + w - 24, my + 48, Color{55, 145, 195, 180});

    // Left Column: 3D Airship Engineering & Physics
    int col1 = mx + 26;
    DrawText("1. 3D VOXEL BUOYANCY, SHIELDS & STRUCTURAL FRACTURE", col1, my + 64, 15, Color{255, 205, 75, 255});
    DrawText("- 22 Block Types: Timber, Ironclad, Orichalcum, Shield Domes, Homing Torpedoes,", col1, my + 90, 12, WHITE);
    DrawText("  Shock Harpoons, Capacitors, Flame Mortars, Tractor Grapplers & Railguns.", col1, my + 108, 12, WHITE);
    DrawText("- Aether Shield Dome Projectors & CIWS Flak Turrets intercept enemy fire!", col1, my + 128, 12, Color{95, 235, 255, 255});
    DrawText("- 3D BFS Connectivity Solver: If enemy fire severs the struts connecting", col1, my + 148, 12, Color{255, 165, 85, 255});
    DrawText("  a wing or pontoon to the Command Bridge, the entire wing detaches", col1, my + 166, 12, Color{255, 165, 85, 255});
    DrawText("  and tumbles into the clouds below as falling 3D debris!", col1, my + 184, 12, Color{255, 165, 85, 255});

    DrawText("2. 6 COLOSSAL SKY-LEVIATHANS & SHOCK HARPOONS", col1, my + 218, 15, Color{255, 205, 75, 255});
    DrawText("- Right-Click [RMB] fires barbed Steam or Shock Harpoon cables into", col1, my + 244, 12, WHITE);
    DrawText("  any segment of a Leviathan OR an enemy Pirate Airship!", col1, my + 262, 12, WHITE);
    DrawText("- Shock Harpoons pulse continuous electric DPS down the cable while", col1, my + 282, 12, Color{95, 240, 185, 255});
    DrawText("  Hyper-Velocity Railguns & Homing Torpedoes pierce heavy armor!", col1, my + 300, 12, Color{95, 240, 185, 255});
    DrawText("- Shatter outer carapace armor plates with broadsides [LMB] to expose", col1, my + 320, 12, Color{255, 125, 125, 255});
    DrawText("  glowing inner Aether Core weak spots for 2.4x CRITICAL DAMAGE!", col1, my + 338, 12, Color{255, 125, 125, 255});

    DrawText("3. COMMODITY MARKET, FLEET ORDERS & WEATHER", col1, my + 372, 15, Color{255, 205, 75, 255});
    DrawText("- Press [J] to open the Island Commodity Exchange & Bounty Ledger.", col1, my + 398, 12, WHITE);
    DrawText("- Press [Z] to issue Squadron Orders to Allied Wingmen; [T] for Weather;", col1, my + 416, 12, Color{95, 245, 175, 255});
    DrawText("  [K] Crew; [1/X] Repair; [2/C] Sonar; [3/G] Sky-Mine; [4] EMP Nova.", col1, my + 436, 12, Color{95, 235, 255, 255});

    // Right Column: Complete Keybind Reference
    int col2 = mx + 475;
    DrawLine(col2 - 18, my + 60, col2 - 18, my + h - 24, Color{45, 95, 135, 160});
    DrawText("FLIGHT & COMBAT CONTROLS", col2, my + 64, 15, Color{85, 235, 255, 255});
    DrawText("[W / S] & [A / D]        : Turbine Throttle & Rudder Yaw", col2, my + 90, 12, WHITE);
    DrawText("[SPACE] / [LEFT CTRL]    : Ascend / Descend Cruise Altitude", col2, my + 110, 12, WHITE);
    DrawText("[Hold LEFT SHIFT]        : Aether Overdrive Afterburner Boost", col2, my + 130, 12, Color{255, 215, 85, 255});
    DrawText("[Hold LMB] / [Click RMB] : Fire Artillery / Launch Harpoon Winch", col2, my + 150, 12, Color{255, 215, 95, 255});
    DrawText("[Hold R] / [F]           : Reel In Harpoon Winch / Cut Cables", col2, my + 170, 12, Color{95, 240, 185, 255});
    DrawText("[1..4] or [X/C/G/4]      : Repair / Sonar / Sky-Mine / EMP Nova", col2, my + 190, 12, Color{95, 245, 175, 255});
    DrawText("[J] / [K] / [U]          : Trade & Contracts / Crew / Hire Wingman", col2, my + 210, 12, Color{135, 225, 255, 255});
    DrawText("[L] / [O] / [Y]          : Ancient Relics / Medals / Hull Livery", col2, my + 230, 12, Color{255, 215, 95, 255});
    DrawText("[Z] / [T] / [V]          : Squadron Order / Weather / Sky Time", col2, my + 250, 12, Color{95, 245, 185, 255});
    DrawText("[N] / [I] / [TAB] / [B]  : Summon Boss / Dread-Ark / Walk / Drydock", col2, my + 270, 12, WHITE);
    DrawText("[F6] / [F7]              : Quick Save / Quick Load Full Campaign", col2, my + 290, 12, Color{255, 205, 95, 255});
    DrawText("[M] / [P] / [F3]         : World Map & Warp / Autopilot / Perf HUD", col2, my + 310, 12, WHITE);

    DrawText("3D DRYDOCK SHIPYARD CONTROLS (22 BLOCKS & 12 SHIPS)", col2, my + 336, 15, Color{85, 235, 255, 255});
    DrawText("[LMB] / [RMB]            : Place 3D Block / Remove & Refund Block", col2, my + 364, 12, WHITE);
    DrawText("[1..0, -, =, [, ], ;, '] : Select Block Slots 1..16", col2, my + 386, 12, WHITE);
    DrawText("[, . / \\ Z X]           : Torpedo, Shock, Cap, Mortar, Tractor, Rail", col2, my + 406, 12, Color{255, 215, 95, 255});
    DrawText("[R] / [M] / [G]          : Rotate Block / X-Mirror / CoM Gizmos", col2, my + 428, 12, Color{95, 240, 185, 255});
    DrawText("[O] / [T] / [H] / [F5/9] : Overclock / Refit / Auto-Trim / Save-Load", col2, my + 450, 12, Color{255, 215, 95, 255});

    if (IsKeyPressed(KEY_ESCAPE)) {
        inoutShowCodex = false;
    }
}

void HudRenderer::drawTradeAndContractsModal(
    World& world,
    int screenW,
    int screenH,
    bool& inoutShowLedger
) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 220});
    int w = 960;
    int h = 580;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{75, 205, 255, 230});

    DrawText("SKY-GUILD BOUNTY LEDGER & ISLAND COMMODITY EXCHANGE [J / ESC to Close]", mx + 24, my + 16, 18, Color{85, 235, 255, 255});

    char buf[180];
    std::snprintf(buf, sizeof(buf), "Treasury: %d Gold  |  %d Aetherium  |  Cargo Hold: %d / %d Crates  |  Squadron Order [Z]: %s",
                  world.playerGold, world.playerAether, world.totalCargoUnits(), world.maxCargoCapacity(),
                  World::getFleetOrderName(world.currentFleetOrder));
    DrawText(buf, mx + 24, my + 42, 13, Color{255, 215, 85, 255});
    DrawLine(mx + 24, my + 64, mx + w - 24, my + 64, Color{55, 145, 195, 180});

    Vector2 mouse = GetMousePosition();

    // Find nearest Sky-Island for Commodity Trading
    int nearestIdx = -1;
    float bestDist = 1e30f;
    for (size_t i = 0; i < world.archipelago.islands.size(); ++i) {
        float d = (world.archipelago.islands[i].position - world.playerShip.position).length();
        if (d < bestDist) {
            bestDist = d;
            nearestIdx = static_cast<int>(i);
        }
    }

    // LEFT COLUMN: Island Commodity Exchange
    int leftX = mx + 24;
    int leftW = 445;
    DrawText("ISLAND COMMODITY EXCHANGE", leftX, my + 76, 15, Color{95, 240, 185, 255});

    if (nearestIdx >= 0) {
        const SkyIsland& isl = world.archipelago.islands[static_cast<size_t>(nearestIdx)];
        bool inRange = world.isSandboxMode || (bestDist <= isl.radius + 260.0f);
        std::snprintf(buf, sizeof(buf), "Nearest Port: %s (%.0fm) — %s",
                      isl.name.c_str(), bestDist, inRange ? "DOCKED / IN TRADE RANGE" : "FLY CLOSER TO BUY/SELL");
        DrawText(buf, leftX, my + 98, 12, inRange ? Color{85, 245, 165, 255} : Color{255, 145, 95, 255});

        const auto& cSpecs = Archipelago::getCommoditySpecs();
        for (int c = 0; c < kCommodityCount; ++c) {
            int cardY = my + 122 + c * 104;
            Rectangle card{static_cast<float>(leftX), static_cast<float>(cardY), static_cast<float>(leftW), 94.0f};
            DrawRectangleRec(card, Color{16, 28, 46, 245});
            DrawRectangleLinesEx(card, 1.2f, Color{55, 145, 195, 185});

            int localPrice = isl.marketPrices[static_cast<size_t>(c)];
            int basePrice = cSpecs[static_cast<size_t>(c)].basePrice;
            int owned = world.playerCargo[static_cast<size_t>(c)];

            const char* trendTag = (localPrice < basePrice - 8)
                ? "LOW PRICE (EXPORT SURPLUS)"
                : ((localPrice > basePrice + 8) ? "HIGH DEMAND (PREMIUM)" : "STANDARD MARKET RATE");
            Color trendCol = (localPrice < basePrice - 8)
                ? Color{85, 245, 155, 255}
                : ((localPrice > basePrice + 8) ? Color{255, 205, 75, 255} : Color{185, 205, 225, 255});

            DrawText(cSpecs[static_cast<size_t>(c)].name, static_cast<int>(card.x + 12), static_cast<int>(card.y + 10), 14, WHITE);
            DrawText(trendTag, static_cast<int>(card.x + 12), static_cast<int>(card.y + 30), 11, trendCol);
            DrawText(cSpecs[static_cast<size_t>(c)].unitDesc, static_cast<int>(card.x + 12), static_cast<int>(card.y + 48), 10, Color{170, 195, 215, 255});

            std::snprintf(buf, sizeof(buf), "Port Price: %dG (Base %dG)  |  In Cargo: %d", localPrice, basePrice, owned);
            DrawText(buf, static_cast<int>(card.x + 12), static_cast<int>(card.y + 68), 12, Color{255, 215, 95, 255});

            // BUY & SELL Buttons
            Rectangle buyBtn{card.x + card.width - 144.0f, card.y + 12.0f, 64.0f, 32.0f};
            Rectangle sellBtn{card.x + card.width - 72.0f, card.y + 12.0f, 64.0f, 32.0f};
            bool hovBuy = CheckCollisionPointRec(mouse, buyBtn);
            bool hovSell = CheckCollisionPointRec(mouse, sellBtn);

            DrawRectangleRec(buyBtn, (inRange && hovBuy) ? Color{45, 165, 115, 255} : Color{26, 95, 68, 235});
            DrawRectangleLinesEx(buyBtn, 1.2f, Color{95, 240, 175, 220});
            DrawText("BUY 1", static_cast<int>(buyBtn.x + 13), static_cast<int>(buyBtn.y + 10), 12, WHITE);

            DrawRectangleRec(sellBtn, (inRange && owned > 0 && hovSell) ? Color{185, 125, 45, 255} : Color{95, 65, 28, 235});
            DrawRectangleLinesEx(sellBtn, 1.2f, Color{255, 205, 95, 220});
            DrawText("SELL 1", static_cast<int>(sellBtn.x + 11), static_cast<int>(sellBtn.y + 10), 12, WHITE);

            if (hovBuy && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                world.buyCommodity(c, 1);
            }
            if (hovSell && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                world.sellCommodity(c, 1);
            }
        }
    }

    // RIGHT COLUMN: 6 Colossal Sky-Leviathan Guild Bounty Contracts
    int rightX = mx + 490;
    int rightW = 445;
    DrawLine(rightX - 10, my + 72, rightX - 10, my + h - 20, Color{45, 95, 135, 160});
    DrawText("SKY-GUILD ELDER LEVIATHAN BOUNTIES (6 SPECIES)", rightX, my + 76, 15, Color{255, 205, 75, 255});

    int numContracts = static_cast<int>(world.contracts.size());
    for (int i = 0; i < numContracts; ++i) {
        const BountyContract& bc = world.contracts[static_cast<size_t>(i)];
        int cardY = my + 104 + i * 74;
        Rectangle card{static_cast<float>(rightX), static_cast<float>(cardY), static_cast<float>(rightW), 66.0f};
        DrawRectangleRec(card, bc.completed ? Color{18, 42, 34, 240} : Color{22, 28, 46, 245});
        DrawRectangleLinesEx(card, 1.2f, bc.completed ? Color{75, 235, 155, 220} : Color{225, 145, 75, 190});

        DrawText(bc.title.c_str(), static_cast<int>(card.x + 12), static_cast<int>(card.y + 8), 13, WHITE);
        std::snprintf(buf, sizeof(buf), "Target: %s", bc.targetName.c_str());
        DrawText(buf, static_cast<int>(card.x + 12), static_cast<int>(card.y + 27), 11, Color{185, 215, 240, 255});

        std::snprintf(buf, sizeof(buf), "Reward: +%d Gold, +%d Aetherium, +2 Ambergris", bc.rewardGold, bc.rewardAether);
        DrawText(buf, static_cast<int>(card.x + 12), static_cast<int>(card.y + 45), 11, Color{255, 215, 85, 255});

        const char* statusLbl = bc.completed ? "CLAIMED" : "ACTIVE";
        Color statusCol = bc.completed ? Color{85, 245, 165, 255} : Color{255, 165, 65, 255};
        DrawText(statusLbl, static_cast<int>(card.x + card.width - 78), static_cast<int>(card.y + 12), 12, statusCol);
    }

    if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowLedger = false;
    }
}

void HudRenderer::drawRelicVaultModal(
    World& world,
    int screenW,
    int screenH,
    bool& inoutShowRelics
) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 220});
    int w = 820;
    int h = 540;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{235, 185, 65, 230});

    DrawText("ANCIENT AETHER-RELIC VAULT & CITADEL LIBERATION [L / ESC to Close]", mx + 24, my + 18, 17, Color{255, 215, 85, 255});

    char buf[180];
    std::snprintf(buf, sizeof(buf), "Treasury: %d Gold  |  %d Aetherium  |  Awakened Relics: %d / 5  |  Citadels Liberated: %d",
                  world.playerGold, world.playerAether, world.unlockedRelicCount(), world.citadelsLiberatedStat);
    DrawText(buf, mx + 24, my + 44, 13, Color{95, 240, 185, 255});
    DrawLine(mx + 24, my + 68, mx + w - 24, my + 68, Color{185, 145, 55, 180});

    const auto& rSpecs = World::getRelicSpecs();
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < kRelicCount; ++i) {
        int cardY = my + 82 + i * 84;
        Rectangle card{static_cast<float>(mx + 24), static_cast<float>(cardY), static_cast<float>(w - 48), 74.0f};
        bool awakened = world.unlockedRelics[static_cast<size_t>(i)];

        DrawRectangleRec(card, awakened ? Color{22, 42, 36, 245} : Color{16, 26, 44, 245});
        DrawRectangleLinesEx(card, 1.3f, awakened ? Color{85, 245, 175, 230} : Color{195, 155, 65, 190});

        std::snprintf(buf, sizeof(buf), "%d. %s  (%s)",
                      i + 1, rSpecs[static_cast<size_t>(i)].name, rSpecs[static_cast<size_t>(i)].originLore);
        DrawText(buf, static_cast<int>(card.x + 16), static_cast<int>(card.y + 12), 15,
                 awakened ? Color{95, 255, 185, 255} : Color{255, 215, 95, 255});

        DrawText(rSpecs[static_cast<size_t>(i)].bonusDesc, static_cast<int>(card.x + 16), static_cast<int>(card.y + 38), 12, Color{205, 225, 245, 255});

        Rectangle btn{card.x + card.width - 205.0f, card.y + 17.0f, 190.0f, 40.0f};
        bool hover = CheckCollisionPointRec(mouse, btn);

        if (!awakened) {
            int gCost = rSpecs[static_cast<size_t>(i)].goldCost;
            int aCost = rSpecs[static_cast<size_t>(i)].aetherCost;
            bool canAfford = world.isSandboxMode || (world.playerGold >= gCost && world.playerAether >= aCost);
            DrawRectangleRec(btn, (hover && canAfford) ? Color{195, 135, 45, 255} : (canAfford ? Color{135, 92, 32, 240} : Color{45, 55, 68, 220}));
            DrawRectangleLinesEx(btn, 1.5f, canAfford ? Color{255, 215, 95, 255} : Color{110, 120, 135, 180});
            std::snprintf(buf, sizeof(buf), "AWAKEN (%dG + %dAe)", gCost, aCost);
            DrawText(buf, static_cast<int>(btn.x + 14), static_cast<int>(btn.y + 13), 12, WHITE);
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                world.unlockRelic(i);
            }
        } else {
            DrawRectangleRec(btn, Color{28, 105, 78, 230});
            DrawRectangleLinesEx(btn, 1.5f, Color{95, 255, 185, 255});
            DrawText("RELIC AWAKENED", static_cast<int>(btn.x + 34), static_cast<int>(btn.y + 13), 13, Color{95, 255, 185, 255});
        }
    }

    if (IsKeyPressed(KEY_L) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowRelics = false;
    }
}

void HudRenderer::drawMedalsModal(
    World& world,
    int screenW,
    int screenH,
    bool& inoutShowMedals
) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 222});
    int w = 920;
    int h = 580;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{245, 195, 65, 235});

    DrawText("SKY-ADMIRAL COMMENDATIONS & CAREER DOSSIER [O / ESC to Close]", mx + 24, my + 18, 17, Color{255, 220, 85, 255});

    char buf[200];
    int medalsEarned = world.earnedMedalCount();
    std::snprintf(buf, sizeof(buf),
                  "Earned Medals: %d / %d (+%d%% Global Fleet Damage, Shield & Repair Prestige Bonus)",
                  medalsEarned, kMedalCount, medalsEarned * 3);
    DrawText(buf, mx + 24, my + 44, 13, Color{95, 245, 185, 255});

    std::snprintf(buf, sizeof(buf),
                  "Stats: Leviathans:%d | Pirates:%d | Citadels:%d | Rings:%d | Rams:%d | Mines:%d | Meteors:%d | SOS:%d | CIWS:%d | Obelisks:%d/6",
                  world.leviathansSlain, world.piratesSunk, world.citadelsLiberatedStat,
                  world.slipstreamRingsPassedStat, world.ramImpactsStat, world.skyMineHitsStat,
                  world.meteorsHarvestedStat, world.survivorsRescuedStat, world.projectilesInterceptedStat,
                  world.obelisksAttunedStat);
    DrawText(buf, mx + 24, my + 64, 11, Color{185, 215, 245, 255});
    DrawLine(mx + 24, my + 86, mx + w - 24, my + 86, Color{185, 145, 55, 180});

    const auto& mSpecs = World::getMedalSpecs();
    int colW = (w - 64) / 2;

    for (int i = 0; i < kMedalCount; ++i) {
        int col = i / 5;
        int row = i % 5;
        int cx = mx + 24 + col * (colW + 16);
        int cy = my + 100 + row * 90;

        Rectangle card{static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(colW), 80.0f};
        bool earned = world.earnedMedals[static_cast<size_t>(i)];

        DrawRectangleRec(card, earned ? Color{24, 44, 34, 245} : Color{16, 25, 42, 245});
        DrawRectangleLinesEx(card, 1.3f, earned ? Color{95, 250, 175, 235} : Color{115, 135, 165, 180});

        std::snprintf(buf, sizeof(buf), "%d. %s", i + 1, mSpecs[static_cast<size_t>(i)].title);
        DrawText(buf, static_cast<int>(card.x + 14), static_cast<int>(card.y + 10), 14,
                 earned ? Color{255, 225, 85, 255} : Color{215, 225, 240, 255});

        DrawText(mSpecs[static_cast<size_t>(i)].conditionDesc, static_cast<int>(card.x + 14), static_cast<int>(card.y + 32), 11, Color{185, 205, 230, 255});

        std::snprintf(buf, sizeof(buf), "Reward: +%dG, +%d Ae, +3%% Prestige",
                      mSpecs[static_cast<size_t>(i)].rewardGold, mSpecs[static_cast<size_t>(i)].rewardAether);
        DrawText(buf, static_cast<int>(card.x + 14), static_cast<int>(card.y + 54), 11, Color{95, 235, 255, 255});

        const char* badge = earned ? "AWARDED" : "LOCKED";
        Color badgeCol = earned ? Color{95, 255, 175, 255} : Color{145, 155, 175, 220};
        DrawText(badge, static_cast<int>(card.x + card.width - 78), static_cast<int>(card.y + 12), 11, badgeCol);
    }

    if (IsKeyPressed(KEY_O) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowMedals = false;
    }
}

} // namespace aethernaut
