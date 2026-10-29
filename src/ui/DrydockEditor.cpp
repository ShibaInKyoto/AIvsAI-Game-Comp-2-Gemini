#include "ui/DrydockEditor.h"
#include "sim/ShipBlueprints.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace aethernaut {

void DrydockEditor::update(World& world, const Camera3D& camera, int screenW, int screenH, bool& outExitDrydock) {
    // Keyboard shortcuts for block selection (1..9, 0, -, =)
    if (IsKeyPressed(KEY_ONE))   selectedBlock = BlockId::WoodHull;
    if (IsKeyPressed(KEY_TWO))   selectedBlock = BlockId::IronArmor;
    if (IsKeyPressed(KEY_THREE)) selectedBlock = BlockId::LiftBalloon;
    if (IsKeyPressed(KEY_FOUR))  selectedBlock = BlockId::AetherCore;
    if (IsKeyPressed(KEY_FIVE))  selectedBlock = BlockId::SteamPropeller;
    if (IsKeyPressed(KEY_SIX))   selectedBlock = BlockId::GyroRudder;
    if (IsKeyPressed(KEY_SEVEN)) selectedBlock = BlockId::DeckCannon;
    if (IsKeyPressed(KEY_EIGHT)) selectedBlock = BlockId::HarpoonWinch;
    if (IsKeyPressed(KEY_NINE))  selectedBlock = BlockId::TeslaCoil;
    if (IsKeyPressed(KEY_ZERO))  selectedBlock = BlockId::FlakTurret;
    if (IsKeyPressed(KEY_MINUS)) selectedBlock = BlockId::RepairCrane;
    if (IsKeyPressed(KEY_EQUAL)) selectedBlock = BlockId::HelmBridge;

    if (IsKeyPressed(KEY_R)) {
        selectedOrientation = (selectedOrientation + 1) % 4;
    }
    if (IsKeyPressed(KEY_M)) {
        symmetryMirrorX = !symmetryMirrorX;
    }
    if (IsKeyPressed(KEY_G)) {
        showEngineeringGizmos = !showEngineeringGizmos;
    }
    if (IsKeyPressed(KEY_F5)) {
        if (ShipBlueprints::saveCustomBlueprint(world.playerShip, "custom_ship.aeth")) {
            world.statusBannerText = "Saved Custom 3D Airship Blueprint to custom_ship.aeth!";
            world.statusBannerTimer = 4.0f;
        }
    }
    if (IsKeyPressed(KEY_F9)) {
        if (ShipBlueprints::loadCustomBlueprint(world.playerShip, "custom_ship.aeth")) {
            world.statusBannerText = "Loaded Custom 3D Airship Blueprint from custom_ship.aeth!";
            world.statusBannerTimer = 4.0f;
        }
    }
    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_ESCAPE)) {
        outExitDrydock = true;
        return;
    }

    Vector2 mousePos = GetMousePosition();
    // Ignore 3D raycast if cursor is over top/left/bottom UI panels
    bool overUI = (mousePos.y > static_cast<float>(screenH - 125)) ||
                  (mousePos.y < 64.0f) ||
                  (mousePos.x < 368.0f && mousePos.y < 580.0f);

    ghostPreview.active = false;
    ghostPreview.showEngineeringGizmos = showEngineeringGizmos;

    if (overUI) {
        return;
    }

    Ray rlRay = GetScreenToWorldRay(mousePos, camera);
    Ray3D worldRay{
        {rlRay.position.x, rlRay.position.y, rlRay.position.z},
        {rlRay.direction.x, rlRay.direction.y, rlRay.direction.z}
    };

    Int3 hitCoord{}, adjCoord{};
    Vec3 hitWorld{};
    float hitDist = 0.0f;
    Airship& ship = world.playerShip;

    if (ship.raycastBlocks(worldRay, 250.0f, hitCoord, adjCoord, hitWorld, hitDist)) {
        if (ship.isValidCoord(adjCoord.x, adjCoord.y, adjCoord.z)) {
            ghostPreview.active = true;
            ghostPreview.coord = adjCoord;
            ghostPreview.blockType = selectedBlock;
            ghostPreview.orientation = selectedOrientation;
            ghostPreview.hasMirror = symmetryMirrorX;
            ghostPreview.mirrorCoord = {-adjCoord.x, adjCoord.y, adjCoord.z};
        }

        // Left Click: Place Block (and X-symmetry twin if enabled)
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ghostPreview.active) {
            const BlockSpec& spec = BlockRegistry::get(selectedBlock);
            bool needTwin = symmetryMirrorX && (ghostPreview.mirrorCoord.x != ghostPreview.coord.x);
            int totalGoldCost = spec.goldCost * (needTwin ? 2 : 1);
            int totalAetherCost = spec.aetherCost * (needTwin ? 2 : 1);

            if (world.isSandboxMode || (world.playerGold >= totalGoldCost && world.playerAether >= totalAetherCost)) {
                if (!world.isSandboxMode) {
                    world.playerGold -= totalGoldCost;
                    world.playerAether -= totalAetherCost;
                }
                ship.setBlock(ghostPreview.coord.x, ghostPreview.coord.y, ghostPreview.coord.z, selectedBlock, selectedOrientation);
                if (needTwin) {
                    uint8_t twinOrient = selectedOrientation;
                    if (selectedOrientation == 1) twinOrient = 3;
                    else if (selectedOrientation == 3) twinOrient = 1;
                    ship.setBlock(ghostPreview.mirrorCoord.x, ghostPreview.mirrorCoord.y, ghostPreview.mirrorCoord.z, selectedBlock, twinOrient);
                }
                world.pendingAudioEvents.push_back({SimAudioEventType::RepairPulse, ship.position, 1.0f});
            }
        }

        // Right Click: Remove Hovered Block (and refund cost)
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            BlockId existing = ship.getCell(hitCoord.x, hitCoord.y, hitCoord.z).id;
            if (existing != BlockId::Empty && ship.occupiedBlockCount > 1) {
                const BlockSpec& spec = BlockRegistry::get(existing);
                ship.removeBlock(hitCoord.x, hitCoord.y, hitCoord.z);
                if (!world.isSandboxMode) {
                    world.playerGold += spec.goldCost;
                    world.playerAether += spec.aetherCost;
                }
                if (symmetryMirrorX && hitCoord.x != 0) {
                    BlockId twinId = ship.getCell(-hitCoord.x, hitCoord.y, hitCoord.z).id;
                    if (twinId != BlockId::Empty && ship.occupiedBlockCount > 1) {
                        ship.removeBlock(-hitCoord.x, hitCoord.y, hitCoord.z);
                        if (!world.isSandboxMode) {
                            world.playerGold += BlockRegistry::get(twinId).goldCost;
                            world.playerAether += BlockRegistry::get(twinId).aetherCost;
                        }
                    }
                }
            }
        }
    }
}

void DrydockEditor::drawUI(World& world, int screenW, int screenH, bool& outExitDrydock) {
    Airship& ship = world.playerShip;
    Vector2 mouse = GetMousePosition();

    // 1. Top Drydock Header & Blueprint Preset Loader Bar
    DrawRectangle(0, 0, screenW, 58, Color{10, 16, 28, 240});
    DrawLine(0, 58, screenW, 58, Color{55, 195, 235, 180});

    DrawText("3D SKY-DRYDOCK & AIRSHIP ARCHITECT", 20, 11, 20, Color{65, 225, 255, 255});
    DrawText("LMB: Place | RMB: Remove | [R] Rotate | [M] X-Mirror | [Q/E or RMB Drag] Orbit Camera", 20, 35, 12, Color{175, 195, 215, 255});

    // Blueprint Preset Buttons on Top Right
    int bpCount = ShipBlueprints::count();
    int btnX = screenW - (bpCount * 130) - 142;
    for (int i = 0; i < bpCount; ++i) {
        ShipBlueprintInfo info = ShipBlueprints::getInfo(i);
        Rectangle r{static_cast<float>(btnX + i * 130), 12.0f, 124.0f, 34.0f};
        bool hover = CheckCollisionPointRec(mouse, r);
        DrawRectangleRec(r, hover ? Color{35, 95, 145, 245} : Color{22, 36, 58, 235});
        DrawRectangleLinesEx(r, 1.5f, Color{65, 185, 235, 200});
        DrawText(info.name, static_cast<int>(r.x + 7), static_cast<int>(r.y + 11), 12, WHITE);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ShipBlueprints::applyBlueprint(ship, i);
        }
    }

    // Launch / Undock Button
    Rectangle launchBtn{static_cast<float>(screenW - 135), 10.0f, 120.0f, 38.0f};
    bool hoverLaunch = CheckCollisionPointRec(mouse, launchBtn);
    DrawRectangleRec(launchBtn, hoverLaunch ? Color{45, 195, 115, 255} : Color{28, 155, 88, 245});
    DrawRectangleLinesEx(launchBtn, 2.0f, Color{125, 255, 175, 255});
    DrawText("UNDOCK [B]", static_cast<int>(launchBtn.x + 16), static_cast<int>(launchBtn.y + 11), 15, WHITE);
    if (hoverLaunch && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        outExitDrydock = true;
    }

    // 2. Left Engineering & Buoyancy Diagnostics Panel
    DrawRectangle(16, 70, 348, 502, Color{11, 18, 31, 236});
    DrawRectangleLinesEx({16.0f, 70.0f, 348.0f, 502.0f}, 1.5f, Color{55, 165, 215, 185});

    DrawText("VESSEL ENGINEERING METRICS", 28, 82, 16, Color{255, 205, 75, 255});

    char buf[160];
    std::snprintf(buf, sizeof(buf), "Gold Treasury: %d G   |   Aetherium: %d", world.playerGold, world.playerAether);
    DrawText(buf, 28, 108, 14, Color{95, 240, 185, 255});

    float weightForce = ship.totalMass * 3.2f;
    float liftRatio = ship.totalLift / std::max(1.0f, weightForce);
    std::snprintf(buf, sizeof(buf), "Total Mass: %.0f tons (%d Voxel Blocks)", ship.totalMass, ship.occupiedBlockCount);
    DrawText(buf, 28, 134, 13, WHITE);

    // Buoyancy Lift Gauge Bar
    std::snprintf(buf, sizeof(buf), "Buoyancy Lift: %.0f kN (%.0f%% Equilibrium)", ship.totalLift, liftRatio * 100.0f);
    DrawText(buf, 28, 156, 13, (liftRatio >= 1.0f) ? Color{85, 245, 155, 255} : Color{255, 95, 95, 255});
    DrawRectangle(28, 174, 320, 8, Color{20, 32, 48, 255});
    float clampedLiftBar = std::clamp(liftRatio / 1.5f, 0.0f, 1.0f);
    DrawRectangle(28, 174, static_cast<int>(320.0f * clampedLiftBar), 8,
                  (liftRatio >= 1.0f) ? Color{65, 235, 145, 255} : Color{245, 85, 85, 255});

    std::snprintf(buf, sizeof(buf), "Forward Thrust: %.0f kN (%d Brass Turbines)", ship.totalThrust, ship.propellerCount);
    DrawText(buf, 28, 192, 13, WHITE);

    std::snprintf(buf, sizeof(buf), "Yaw Authority: %.0f kNm  |  Armament: %dG / %dH",
                  ship.totalTurnTorque, ship.cannonCount, ship.harpoonCount);
    DrawText(buf, 28, 214, 13, Color{235, 205, 125, 255});

    // Power Grid Gauge Bar
    std::snprintf(buf, sizeof(buf), "Aether Power Grid: %.0f / %.0f MW (%.0f%%)",
                  ship.totalPowerGen, ship.totalPowerDraw, ship.powerEfficiency * 100.0f);
    DrawText(buf, 28, 236, 13, (ship.powerEfficiency >= 0.99f) ? Color{85, 225, 255, 255} : Color{255, 175, 65, 255});
    DrawRectangle(28, 254, 320, 8, Color{20, 32, 48, 255});
    DrawRectangle(28, 254, static_cast<int>(320.0f * std::clamp(ship.powerEfficiency, 0.0f, 1.0f)), 8,
                  (ship.powerEfficiency >= 0.99f) ? Color{65, 215, 255, 255} : Color{255, 165, 55, 255});

    float comColOffset = std::sqrt(
        (ship.centerOfLiftLocal.x - ship.centerOfMassLocal.x) * (ship.centerOfLiftLocal.x - ship.centerOfMassLocal.x) +
        (ship.centerOfLiftLocal.z - ship.centerOfMassLocal.z) * (ship.centerOfLiftLocal.z - ship.centerOfMassLocal.z)
    );
    std::snprintf(buf, sizeof(buf), "CoM/CoL Trim Offset: %.2fm (%s)",
                  comColOffset, (comColOffset < 1.5f) ? "BALANCED" : "LISTING TRIM");
    DrawText(buf, 28, 272, 13, (comColOffset < 1.5f) ? Color{115, 245, 185, 255} : Color{255, 195, 75, 255});

    std::snprintf(buf, sizeof(buf), "X-Symmetry [M]: %s  |  Gizmos [G]: %s",
                  symmetryMirrorX ? "ON" : "OFF", showEngineeringGizmos ? "ON" : "OFF");
    DrawText(buf, 28, 294, 13, symmetryMirrorX ? Color{75, 245, 185, 255} : Color{185, 185, 185, 255});

    const char* orientNames[4] = {"North (-Z Bow)", "East (+X Stbd)", "South (+Z Stern)", "West (-X Port)"};
    std::snprintf(buf, sizeof(buf), "Block Facing [R]: %s", orientNames[selectedOrientation % 4]);
    DrawText(buf, 28, 316, 13, Color{195, 225, 255, 255});

    // Repair All Button inside Drydock
    Rectangle repBtn{28.0f, 342.0f, 320.0f, 32.0f};
    bool hoverRep = CheckCollisionPointRec(mouse, repBtn);
    DrawRectangleRec(repBtn, hoverRep ? Color{45, 135, 185, 255} : Color{28, 85, 125, 235});
    DrawRectangleLinesEx(repBtn, 1.5f, Color{95, 215, 255, 255});
    DrawText("REPAIR ALL HULL & LIFT CELLS (FREE)", 54, 352, 12, WHITE);
    if (hoverRep && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ship.triggerEmergencyRepair(9999.0f);
    }

    // Save / Load Custom Blueprint Buttons ([F5] / [F9])
    Rectangle saveBtn{28.0f, 382.0f, 155.0f, 30.0f};
    Rectangle loadBtn{193.0f, 382.0f, 155.0f, 30.0f};
    bool hoverSave = CheckCollisionPointRec(mouse, saveBtn);
    bool hoverLoad = CheckCollisionPointRec(mouse, loadBtn);
    DrawRectangleRec(saveBtn, hoverSave ? Color{45, 155, 115, 255} : Color{24, 95, 72, 235});
    DrawRectangleRec(loadBtn, hoverLoad ? Color{65, 115, 175, 255} : Color{32, 65, 108, 235});
    DrawRectangleLinesEx(saveBtn, 1.2f, Color{95, 235, 175, 220});
    DrawRectangleLinesEx(loadBtn, 1.2f, Color{95, 195, 255, 220});
    DrawText("SAVE SHIP [F5]", static_cast<int>(saveBtn.x + 24), static_cast<int>(saveBtn.y + 9), 12, WHITE);
    DrawText("LOAD SHIP [F9]", static_cast<int>(loadBtn.x + 24), static_cast<int>(loadBtn.y + 9), 12, WHITE);
    if (hoverSave && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ShipBlueprints::saveCustomBlueprint(ship, "custom_ship.aeth");
    }
    if (hoverLoad && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ShipBlueprints::loadCustomBlueprint(ship, "custom_ship.aeth");
    }

    // Selected Block Inspector
    const BlockSpec& selSpec = BlockRegistry::get(selectedBlock);
    DrawLine(28, 422, 348, 422, Color{55, 125, 165, 160});
    DrawText(selSpec.name, 28, 430, 15, Color{65, 235, 255, 255});
    std::snprintf(buf, sizeof(buf), "Category: %s  |  Mass: %.0ft  |  Max HP: %.0f",
                  selSpec.category, selSpec.mass, selSpec.maxHp);
    DrawText(buf, 28, 452, 12, Color{195, 215, 235, 255});
    std::snprintf(buf, sizeof(buf), "Build Cost: %d Gold, %d Aetherium",
                  selSpec.goldCost, selSpec.aetherCost);
    DrawText(buf, 28, 472, 12, Color{255, 215, 95, 255});

    // Truncate or fit description cleanly inside 320px
    DrawText(selSpec.description, 28, 494, 11, Color{165, 188, 210, 255});

    // 3. Bottom 12-Block Engineering Hotbar
    static const char* kShortTitles[13] = {
        "",
        "Timber Hull",
        "Iron Bulkhead",
        "Aether Lift",
        "Aether Core",
        "Brass Prop",
        "Gyro Rudder",
        "Siege Cannon",
        "Harpoon Winch",
        "Voltaic Spire",
        "Flak Turret",
        "Repair Rig",
        "Helm Bridge"
    };
    static const char* kKeyLabels[13] = {
        "", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]", "[0]", "[-]", "[=]"
    };

    int barW = 12 * 102 + 16;
    int barX = (screenW - barW) / 2;
    int barY = screenH - 110;
    DrawRectangle(barX, barY, barW, 98, Color{10, 16, 28, 242});
    DrawRectangleLinesEx({static_cast<float>(barX), static_cast<float>(barY), static_cast<float>(barW), 98.0f}, 1.5f, Color{65, 175, 225, 195});

    for (int i = 1; i <= 12; ++i) {
        BlockId bid = static_cast<BlockId>(i);
        const BlockSpec& sp = BlockRegistry::get(bid);
        Rectangle slot{static_cast<float>(barX + 10 + (i - 1) * 102), static_cast<float>(barY + 8), 94.0f, 82.0f};
        bool isSel = (selectedBlock == bid);
        bool hover = CheckCollisionPointRec(mouse, slot);

        DrawRectangleRec(slot, isSel ? Color{35, 95, 145, 255} : (hover ? Color{25, 48, 78, 245} : Color{16, 26, 44, 235}));
        DrawRectangleLinesEx(slot, isSel ? 2.5f : 1.0f, isSel ? Color{85, 245, 255, 255} : Color{65, 95, 135, 180});

        // Color swatch indicator
        DrawRectangle(static_cast<int>(slot.x + 8), static_cast<int>(slot.y + 8), 18, 18,
                      Color{sp.primaryColor.r, sp.primaryColor.g, sp.primaryColor.b, 255});
        DrawRectangleLines(static_cast<int>(slot.x + 8), static_cast<int>(slot.y + 8), 18, 18,
                           Color{sp.accentColor.r, sp.accentColor.g, sp.accentColor.b, 255});

        DrawText(kKeyLabels[i], static_cast<int>(slot.x + 64), static_cast<int>(slot.y + 8), 12, Color{165, 185, 205, 255});
        DrawText(kShortTitles[i], static_cast<int>(slot.x + 7), static_cast<int>(slot.y + 34), 11, WHITE);
        DrawText(sp.category, static_cast<int>(slot.x + 7), static_cast<int>(slot.y + 49), 10, Color{125, 215, 255, 255});

        if (sp.aetherCost > 0) {
            std::snprintf(buf, sizeof(buf), "%dG + %dAe", sp.goldCost, sp.aetherCost);
        } else {
            std::snprintf(buf, sizeof(buf), "%d Gold", sp.goldCost);
        }
        DrawText(buf, static_cast<int>(slot.x + 7), static_cast<int>(slot.y + 64), 11, Color{255, 215, 85, 255});

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedBlock = bid;
        }
    }
}

} // namespace aethernaut
