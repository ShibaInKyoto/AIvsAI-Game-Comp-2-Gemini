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
            std::snprintf(buf, sizeof(buf), "%s [%.0fm]", lev.name().c_str(), dist);
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
        Color tagCol = (npc.faction() == 1) ? Color{255, 95, 95, 255} : Color{85, 245, 165, 255};
        std::snprintf(buf, sizeof(buf), "%s [%.0fm]", npc.name().c_str(), dist);
        DrawText(buf, static_cast<int>(scrPos.x - 50), static_cast<int>(scrPos.y - 22), 12, tagCol);

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
    // 1B. Top-Left Active Bounty Contract & Fleet Squadron Card
    // -------------------------------------------------------------------------
    DrawRectangle(18, 18, 330, 88, Color{11, 18, 30, 225});
    DrawRectangleLinesEx({18.0f, 18.0f, 330.0f, 88.0f}, 1.5f, Color{65, 175, 225, 180});

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
        DrawText(buf, 30, 28, 14, Color{255, 205, 75, 255});
        std::snprintf(buf, sizeof(buf), "Bounty: +%d Gold, +%d Aetherium | Shatter Armor -> Core",
                      nextLev->goldBounty, nextLev->aetherBounty);
        DrawText(buf, 30, 48, 11, Color{195, 220, 240, 255});
    } else {
        DrawText("ALL 4 ELDER SKY-LEVIATHANS CONQUERED!", 30, 28, 14, Color{85, 245, 165, 255});
        DrawText("Sky-Archipelago Secured — Victory!", 30, 48, 12, Color{225, 235, 245, 255});
    }

    int activeAllies = 0;
    for (const auto& n : world.npcShips) {
        if (!n.isDestroyed && n.faction() == 0) activeAllies++;
    }
    std::snprintf(buf, sizeof(buf), "Allied Wingmen: %d  |  [U] Hire Escort (200G)  |  [H] Codex", activeAllies);
    DrawText(buf, 30, 74, 11, Color{95, 235, 255, 255});

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
    int panelY = screenH - 215;
    DrawRectangle(panelX, panelY, 375, 198, Color{11, 18, 30, 230});
    DrawRectangleLinesEx({static_cast<float>(panelX), static_cast<float>(panelY), 375.0f, 198.0f}, 1.5f, Color{65, 175, 225, 185});

    // Live 2D Top-Down Voxel Integrity Doll on Left of Panel
    int dollCenterX = panelX + 68;
    int dollCenterY = panelY + 104;
    DrawRectangle(panelX + 12, panelY + 32, 112, 152, Color{6, 11, 20, 230});
    DrawRectangleLines(panelX + 12, panelY + 32, 112, 152, Color{45, 95, 135, 180});
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
    DrawText(ship.name().c_str(), vitX, panelY + 12, 16, Color{85, 235, 255, 255});

    float hullPct = std::clamp(ship.totalHp / std::max(1.0f, ship.totalMaxHp), 0.0f, 1.0f);
    std::snprintf(buf, sizeof(buf), "Hull Integrity: %.0f / %.0f (%.0f%%)", ship.totalHp, ship.totalMaxHp, hullPct * 100.0f);
    DrawText(buf, vitX, panelY + 36, 13, WHITE);
    DrawRectangle(vitX, panelY + 54, 220, 9, Color{22, 32, 48, 255});
    DrawRectangle(vitX, panelY + 54, static_cast<int>(220 * hullPct), 9,
                  (hullPct > 0.45f) ? Color{65, 225, 145, 255} : Color{245, 75, 85, 255});

    float speedKnots = std::sqrt(ship.velocity.x * ship.velocity.x + ship.velocity.z * ship.velocity.z) * 1.94f;
    std::snprintf(buf, sizeof(buf), "Airspeed: %.1f kts  |  Alt: %.0fm", speedKnots, ship.position.y);
    DrawText(buf, vitX, panelY + 72, 13, Color{195, 225, 255, 255});

    float weightF = ship.totalMass * 3.2f;
    std::snprintf(buf, sizeof(buf), "Buoyancy Lift: %.0f / %.0f kN", ship.totalLift, weightF);
    DrawText(buf, vitX, panelY + 92, 13, ship.isSinking ? Color{255, 85, 85, 255} : Color{115, 245, 185, 255});

    std::snprintf(buf, sizeof(buf), "Treasury: %d Gold  |  %d Aetherium", world.playerGold, world.playerAether);
    DrawText(buf, vitX, panelY + 112, 13, Color{255, 215, 85, 255});

    std::snprintf(buf, sizeof(buf), "Leviathans Slain: %d/4  |  Pirates Sunk: %d", world.leviathansSlain, world.piratesSunk);
    DrawText(buf, vitX, panelY + 132, 13, Color{215, 175, 255, 255});

    const char* modeText = world.isPlayerWalkingOnDeck
        ? "DECK WALK MODE [TAB to Return to Helm]"
        : (world.playerAutopilot ? "AI AUTOPILOT ENGAGED [P to Manual]" : "HELM FLIGHT [TAB: Walk Deck | B: Drydock]");
    DrawText(modeText, vitX, panelY + 156, 12, Color{85, 235, 255, 255});

    // Active Harpoon Tether Banner
    if (!world.tethers.empty()) {
        float maxStrain = 0.0f;
        for (const auto& t : world.tethers) {
            maxStrain = std::max(maxStrain, t.tensionForce / std::max(1.0f, t.maxBreakTension));
        }
        std::snprintf(buf, sizeof(buf), "HARPOON CABLE TENSION: %.0f%% ([R] Hold Reel In | [F] Release)", maxStrain * 100.0f);
        DrawText(buf, vitX, panelY + 176, 12, Color{255, 175, 65, 255});
    }

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
        int tw = MeasureText(world.statusBannerText.c_str(), 16);
        int bx = (screenW - tw) / 2 - 18;
        DrawRectangle(bx, 18, tw + 36, 36, Color{12, 22, 38, 225});
        DrawRectangleLines(bx, 18, tw + 36, 36, Color{85, 225, 255, 210});
        DrawText(world.statusBannerText.c_str(), bx + 18, 28, 16, Color{255, 235, 135, 255});
    }

    if (canDockAtPort || world.isSandboxMode) {
        Rectangle dockBtn{static_cast<float>(screenW / 2 - 155), static_cast<float>(screenH - 54), 310.0f, 38.0f};
        bool hover = CheckCollisionPointRec(mouse, dockBtn);
        DrawRectangleRec(dockBtn, hover ? Color{35, 145, 195, 245} : Color{20, 88, 135, 230});
        DrawRectangleLinesEx(dockBtn, 1.5f, Color{95, 235, 255, 255});
        std::snprintf(buf, sizeof(buf), "OPEN 3D DRYDOCK SHIPYARD [B] (%s)",
                      world.isSandboxMode ? "Sandbox" : nearestPortName.c_str());
        DrawText(buf, static_cast<int>(dockBtn.x + 14), static_cast<int>(dockBtn.y + 12), 13, WHITE);
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
    int panelW = 580;
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
    DrawText("ACTIVE 3D FLAGSHIP BLUEPRINT:", cardX + 16, cardY + 12, 12, Color{145, 205, 245, 255});
    DrawText(bp.name, cardX + 16, cardY + 30, 22, WHITE);
    DrawText(bp.role, cardX + 16, cardY + 56, 14, Color{255, 205, 85, 255});
    DrawText(bp.description, cardX + 16, cardY + 80, 11, Color{185, 205, 225, 255});

    char statBuf[128];
    const Airship& s = backgroundWorld.playerShip;
    std::snprintf(statBuf, sizeof(statBuf), "Blocks: %d  |  Mass: %.0ft  |  Lift: %.0fkN  |  Guns: %d  |  Harpoons: %d",
                  s.occupiedBlockCount, s.totalMass, s.totalLift, s.cannonCount, s.harpoonCount);
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
        {"LAUNCH OPEN-SKY EXPEDITION (CAMPAIGN)", "Explore 14 sky-islands, mine Aetherium, engineer your ship, and hunt 4 Elder Leviathans.", TitleAction::StartCampaign, Color{28, 135, 185, 245}},
        {"3D DRYDOCK SHIPYARD & SANDBOX", "Unlimited Gold & Aetherium! Build custom 3D voxel airships with live buoyancy & fracture physics.", TitleAction::StartDrydockSandbox, Color{35, 145, 105, 245}},
        {"COLOSSAL LEVIATHAN HUNT GAUNTLET", "Immediate boss encounter against multi-segmented Sky-Leviathans with Harpoon Winches.", TitleAction::StartLeviathanHunt, Color{165, 75, 55, 245}},
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

    const char* footer = "[W/S] Throttle | [A/D] Rudder | [SPACE/CTRL] Alt | [LMB] Broadsides | [RMB] Harpoons | [B] Drydock";
    DrawText(footer, panelX + 26, panelY + panelH - 28, 11, Color{155, 195, 225, 255});

    return action;
}

void HudRenderer::drawWorldMapModal(const World& world, int screenW, int screenH, bool& inoutShowMap) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 210});
    int mapSize = std::min(screenW, screenH) - 120;
    int mapX = (screenW - mapSize) / 2;
    int mapY = (screenH - mapSize) / 2;

    DrawRectangle(mapX, mapY, mapSize, mapSize, Color{12, 22, 36, 245});
    DrawRectangleLinesEx({static_cast<float>(mapX), static_cast<float>(mapY), static_cast<float>(mapSize), static_cast<float>(mapSize)}, 2.0f, Color{75, 205, 255, 220});

    DrawText("SKY-ARCHIPELAGO NAVIGATION CHART & BOUNTY CONTRACTS [M / ESC to Close]", mapX + 18, mapY + 14, 18, Color{85, 235, 255, 255});

    int cx = mapX + mapSize / 2;
    int cy = mapY + mapSize / 2 + 15;
    float scale = (static_cast<float>(mapSize) * 0.44f) / 1350.0f;

    for (const auto& isl : world.archipelago.islands) {
        int ix = cx + static_cast<int>(isl.position.x * scale);
        int iy = cy + static_cast<int>(isl.position.z * scale);
        Color c = isl.hasDrydockPort ? Color{75, 245, 155, 255} : Color{145, 175, 195, 255};
        DrawCircle(ix, iy, isl.radius * scale * 1.4f, c);
        DrawText(isl.name.c_str(), ix + 10, iy - 6, 12, WHITE);
    }

    for (const auto& lev : world.leviathans) {
        if (lev.isSlain || lev.segments.empty()) continue;
        int lx = cx + static_cast<int>(lev.segments[0].position.x * scale);
        int ly = cy + static_cast<int>(lev.segments[0].position.z * scale);
        DrawCircle(lx, ly, 7.0f, Color{255, 95, 75, 255});
        DrawText(lev.name().c_str(), lx + 10, ly - 6, 12, Color{255, 205, 85, 255});
    }

    int px = cx + static_cast<int>(world.playerShip.position.x * scale);
    int py = cy + static_cast<int>(world.playerShip.position.z * scale);
    DrawCircle(px, py, 5.5f, WHITE);
    DrawText("YOU", px + 8, py - 6, 12, Color{85, 245, 255, 255});

    if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowMap = false;
    }
}

void HudRenderer::drawCodexModal(int screenW, int screenH, bool& inoutShowCodex) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 215});
    int w = 860;
    int h = 560;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{75, 205, 255, 230});

    DrawText("AETHERNAUT ENGINEERING & LEVIATHAN HUNTER'S CODEX [H / ESC to Close]", mx + 24, my + 18, 19, Color{85, 235, 255, 255});
    DrawLine(mx + 24, my + 48, mx + w - 24, my + 48, Color{55, 145, 195, 180});

    // Left Column: 3D Airship Engineering & Physics
    int col1 = mx + 26;
    DrawText("1. 3D VOXEL BUOYANCY & STRUCTURAL FRACTURE", col1, my + 64, 15, Color{255, 205, 75, 255});
    DrawText("- Every airship block has real 3D mass, HP, armor, and buoyancy/power.", col1, my + 90, 12, WHITE);
    DrawText("- Center of Lift (CoL) vs Center of Mass (CoM) governs roll & pitch trim.", col1, my + 110, 12, WHITE);
    DrawText("- Punctured port/starboard Lift Cells cause realistic roll listing!", col1, my + 130, 12, Color{185, 215, 240, 255});
    DrawText("- 3D BFS Connectivity Solver: If enemy fire severs the struts connecting", col1, my + 150, 12, Color{255, 165, 85, 255});
    DrawText("  a wing or pontoon to the Command Bridge, the entire wing detaches", col1, my + 168, 12, Color{255, 165, 85, 255});
    DrawText("  and tumbles into the clouds below as falling 3D debris!", col1, my + 186, 12, Color{255, 165, 85, 255});

    DrawText("2. COLOSSAL SKY-LEVIATHAN HUNTING & HARPOONS", col1, my + 220, 15, Color{255, 205, 75, 255});
    DrawText("- Right-Click [RMB] fires barbed Steam Harpoon Winch cables into", col1, my + 246, 12, WHITE);
    DrawText("  any segment of a multi-segmented 3D Sky-Leviathan.", col1, my + 264, 12, WHITE);
    DrawText("- Hold [R] to reel the winch cable in; press [F] to cut cables if", col1, my + 284, 12, Color{95, 240, 185, 255});
    DrawText("  tension exceeds 95% to avoid snapping steel cables!", col1, my + 302, 12, Color{95, 240, 185, 255});
    DrawText("- Shatter outer carapace armor plates with broadsides [LMB] to expose", col1, my + 322, 12, Color{255, 125, 125, 255});
    DrawText("  glowing inner Aether Core weak spots for 2.4x CRITICAL DAMAGE!", col1, my + 340, 12, Color{255, 125, 125, 255});

    DrawText("3. MINING AETHERIUM & HIRING WINGMEN", col1, my + 374, 15, Color{255, 205, 75, 255});
    DrawText("- Shoot glowing cyan Aetherium Crystal spires on floating islands", col1, my + 400, 12, WHITE);
    DrawText("  to harvest Aetherium for Reactor Cores, Voltaic Spires & Winches.", col1, my + 418, 12, WHITE);
    DrawText("- Press [U] (or use Drydock) to hire Allied Escort Wingmen (200G).", col1, my + 438, 12, Color{95, 235, 255, 255});

    // Right Column: Complete Keybind Reference
    int col2 = mx + 465;
    DrawLine(col2 - 18, my + 60, col2 - 18, my + h - 24, Color{45, 95, 135, 160});
    DrawText("FLIGHT & COMBAT CONTROLS", col2, my + 64, 15, Color{85, 235, 255, 255});
    DrawText("[W / S] or [Up / Down]   : Forward / Reverse Turbine Throttle", col2, my + 92, 12, WHITE);
    DrawText("[A / D] or [Left / Right]: Rudder Yaw Port / Starboard", col2, my + 114, 12, WHITE);
    DrawText("[SPACE] / [LEFT CTRL]    : Ascend / Descend Cruise Altitude", col2, my + 136, 12, WHITE);
    DrawText("[Hold LMB]               : Fire Broadside Cannons, Flak & Tesla", col2, my + 158, 12, Color{255, 215, 95, 255});
    DrawText("[Click RMB]              : Launch Barbed Steam Harpoon Winch", col2, my + 180, 12, Color{255, 215, 95, 255});
    DrawText("[Hold R] / [F]           : Reel In Harpoon Winch / Cut Cables", col2, my + 202, 12, Color{95, 240, 185, 255});
    DrawText("[Q / E] & [Mouse Wheel]  : Orbit & Zoom 3D External Camera", col2, my + 224, 12, WHITE);
    DrawText("[TAB]                    : Walk on Deck (Passively Repairs Hull!)", col2, my + 246, 12, Color{135, 225, 255, 255});
    DrawText("[U]                      : Hire Allied Escort Wingman (200 Gold)", col2, my + 268, 12, Color{95, 245, 175, 255});
    DrawText("[B]                      : Open 3D Drydock Shipyard (at Port)", col2, my + 290, 12, WHITE);
    DrawText("[M] / [P] / [F3]         : World Map / AI Autopilot / Perf HUD", col2, my + 312, 12, WHITE);

    DrawText("3D DRYDOCK SHIPYARD CONTROLS", col2, my + 350, 15, Color{85, 235, 255, 255});
    DrawText("[LMB] / [RMB]            : Place 3D Block / Remove & Refund Block", col2, my + 378, 12, WHITE);
    DrawText("[1..9, 0, -, =]          : Select Block Type from 12-Slot Palette", col2, my + 400, 12, WHITE);
    DrawText("[R] / [M] / [G]          : Rotate Block / X-Mirror / CoM Gizmos", col2, my + 422, 12, Color{95, 240, 185, 255});
    DrawText("[F5] / [F9]              : Save / Load Custom Ship (custom_ship.aeth)", col2, my + 444, 12, Color{255, 215, 95, 255});

    if (IsKeyPressed(KEY_ESCAPE)) {
        inoutShowCodex = false;
    }
}

} // namespace aethernaut
