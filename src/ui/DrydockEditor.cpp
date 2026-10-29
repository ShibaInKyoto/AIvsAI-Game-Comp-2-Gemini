#include "ui/DrydockEditor.h"
#include "sim/ShipBlueprints.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace aethernaut {

void DrydockEditor::update(World& world, const Camera3D& camera, int screenW, int screenH, bool& outExitDrydock) {
    (void)screenW;
    // Keyboard shortcuts for block selection (1..9, 0, -, =, and F1..F4 for advanced blocks)
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
    if (IsKeyPressed(KEY_LEFT_BRACKET))  selectedBlock = BlockId::OrichalcumArmor;
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) selectedBlock = BlockId::ShieldProjector;
    if (IsKeyPressed(KEY_SEMICOLON))     selectedBlock = BlockId::PlasmaLance;
    if (IsKeyPressed(KEY_APOSTROPHE))    selectedBlock = BlockId::AfterburnerJet;
    if (IsKeyPressed(KEY_COMMA))         selectedBlock = BlockId::TorpedoLauncher;
    if (IsKeyPressed(KEY_PERIOD))        selectedBlock = BlockId::ShockHarpoon;
    if (IsKeyPressed(KEY_SLASH))         selectedBlock = BlockId::CapacitorBank;
    if (IsKeyPressed(KEY_BACKSLASH))     selectedBlock = BlockId::FlameMortar;

    if (IsKeyPressed(KEY_R)) {
        selectedOrientation = (selectedOrientation + 1) % 4;
    }
    if (IsKeyPressed(KEY_M)) {
        symmetryMirrorX = !symmetryMirrorX;
    }
    if (IsKeyPressed(KEY_G)) {
        showEngineeringGizmos = !showEngineeringGizmos;
    }
    if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_P)) {
        world.cyclePlayerLivery();
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
    bool overUI = (mousePos.y > static_cast<float>(screenH - 132)) ||
                  (mousePos.y < 64.0f) ||
                  (mousePos.x < 368.0f && mousePos.y < 595.0f);

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

    // 1. Top Drydock Header & 10-Blueprint Preset Loader Bar
    DrawRectangle(0, 0, screenW, 60, Color{10, 16, 28, 240});
    DrawLine(0, 60, screenW, 60, Color{55, 195, 235, 180});

    DrawText("3D SKY-DRYDOCK & AIRSHIP ARCHITECT", 16, 10, 17, Color{65, 225, 255, 255});
    DrawText("LMB: Place | RMB: Remove | [R] Rotate | [M] Mirror | [G] Gizmos", 16, 34, 11, Color{175, 195, 215, 255});

    // 10 Blueprint Preset Buttons arranged in 2 rows of 5 on Top Right
    int bpCount = ShipBlueprints::count();
    int cols = 5;
    int btnW = 136;
    int btnH = 23;
    int gridStartX = screenW - (cols * (btnW + 4)) - 130;
    for (int i = 0; i < bpCount; ++i) {
        int row = i / cols;
        int col = i % cols;
        ShipBlueprintInfo info = ShipBlueprints::getInfo(i);
        Rectangle r{
            static_cast<float>(gridStartX + col * (btnW + 4)),
            static_cast<float>(6 + row * (btnH + 4)),
            static_cast<float>(btnW),
            static_cast<float>(btnH)
        };
        bool hover = CheckCollisionPointRec(mouse, r);
        DrawRectangleRec(r, hover ? Color{35, 95, 145, 245} : Color{22, 36, 58, 235});
        DrawRectangleLinesEx(r, 1.2f, Color{65, 185, 235, 200});
        DrawText(info.name, static_cast<int>(r.x + 5), static_cast<int>(r.y + 6), 10, WHITE);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ShipBlueprints::applyBlueprint(ship, i);
            world.applyCrewMultipliers();
        }
    }

    // Launch / Undock Button
    Rectangle launchBtn{static_cast<float>(screenW - 124), 10.0f, 112.0f, 40.0f};
    bool hoverLaunch = CheckCollisionPointRec(mouse, launchBtn);
    DrawRectangleRec(launchBtn, hoverLaunch ? Color{45, 195, 115, 255} : Color{28, 155, 88, 245});
    DrawRectangleLinesEx(launchBtn, 2.0f, Color{125, 255, 175, 255});
    DrawText("UNDOCK [B]", static_cast<int>(launchBtn.x + 12), static_cast<int>(launchBtn.y + 12), 14, WHITE);
    if (hoverLaunch && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        outExitDrydock = true;
    }

    // 2. Left Engineering & Buoyancy Diagnostics Panel
    DrawRectangle(16, 68, 348, 514, Color{11, 18, 31, 236});
    DrawRectangleLinesEx({16.0f, 68.0f, 348.0f, 514.0f}, 1.5f, Color{55, 165, 215, 185});

    DrawText("VESSEL ENGINEERING METRICS", 28, 80, 16, Color{255, 205, 75, 255});

    char buf[160];
    std::snprintf(buf, sizeof(buf), "Gold Treasury: %d G   |   Aetherium: %d", world.playerGold, world.playerAether);
    DrawText(buf, 28, 104, 14, Color{95, 240, 185, 255});

    float weightForce = ship.totalMass * 3.2f;
    float liftRatio = ship.totalLift / std::max(1.0f, weightForce);
    std::snprintf(buf, sizeof(buf), "Mass: %.0ft (%d Blocks)  |  Shield Dome: %.0f HP",
                  ship.totalMass, ship.occupiedBlockCount, ship.maxShieldHp);
    DrawText(buf, 28, 128, 12, WHITE);

    // Buoyancy Lift Gauge Bar
    std::snprintf(buf, sizeof(buf), "Buoyancy Lift: %.0f kN (%.0f%% Equilibrium)", ship.totalLift, liftRatio * 100.0f);
    DrawText(buf, 28, 150, 12, (liftRatio >= 1.0f) ? Color{85, 245, 155, 255} : Color{255, 95, 95, 255});
    DrawRectangle(28, 166, 320, 8, Color{20, 32, 48, 255});
    float clampedLiftBar = std::clamp(liftRatio / 1.5f, 0.0f, 1.0f);
    DrawRectangle(28, 166, static_cast<int>(320.0f * clampedLiftBar), 8,
                  (liftRatio >= 1.0f) ? Color{65, 235, 145, 255} : Color{245, 85, 85, 255});

    std::snprintf(buf, sizeof(buf), "Forward Thrust: %.0f kN (%d Prop/Jets)", ship.totalThrust, ship.propellerCount);
    DrawText(buf, 28, 184, 12, WHITE);

    std::snprintf(buf, sizeof(buf), "Yaw Authority: %.0f kNm  |  Weapons: %dG / %dH",
                  ship.totalTurnTorque, ship.cannonCount, ship.harpoonCount);
    DrawText(buf, 28, 204, 12, Color{235, 205, 125, 255});

    // Power Grid Gauge Bar
    std::snprintf(buf, sizeof(buf), "Aether Power Grid: %.0f / %.0f MW (%.0f%%)",
                  ship.totalPowerGen, ship.totalPowerDraw, ship.powerEfficiency * 100.0f);
    DrawText(buf, 28, 226, 12, (ship.powerEfficiency >= 0.99f) ? Color{85, 225, 255, 255} : Color{255, 175, 65, 255});
    DrawRectangle(28, 242, 320, 8, Color{20, 32, 48, 255});
    DrawRectangle(28, 242, static_cast<int>(320.0f * std::clamp(ship.powerEfficiency, 0.0f, 1.0f)), 8,
                  (ship.powerEfficiency >= 0.99f) ? Color{65, 215, 255, 255} : Color{255, 165, 55, 255});

    float comColOffset = std::sqrt(
        (ship.centerOfLiftLocal.x - ship.centerOfMassLocal.x) * (ship.centerOfLiftLocal.x - ship.centerOfMassLocal.x) +
        (ship.centerOfLiftLocal.z - ship.centerOfMassLocal.z) * (ship.centerOfLiftLocal.z - ship.centerOfMassLocal.z)
    );
    std::snprintf(buf, sizeof(buf), "CoM/CoL Trim Offset: %.2fm (%s)",
                  comColOffset, (comColOffset < 1.5f) ? "BALANCED" : "LISTING TRIM");
    DrawText(buf, 28, 260, 12, (comColOffset < 1.5f) ? Color{115, 245, 185, 255} : Color{255, 195, 75, 255});

    std::snprintf(buf, sizeof(buf), "X-Symmetry [M]: %s  |  Gizmos [G]: %s",
                  symmetryMirrorX ? "ON" : "OFF", showEngineeringGizmos ? "ON" : "OFF");
    DrawText(buf, 28, 282, 12, symmetryMirrorX ? Color{75, 245, 185, 255} : Color{185, 185, 185, 255});

    const char* orientNames[4] = {"North (-Z Bow)", "East (+X Stbd)", "South (+Z Stern)", "West (-X Port)"};
    std::snprintf(buf, sizeof(buf), "Block Facing [R]: %s", orientNames[selectedOrientation % 4]);
    DrawText(buf, 28, 304, 12, Color{195, 225, 255, 255});

    // Repair All & Livery Theme Buttons inside Drydock
    Rectangle repBtn{28.0f, 326.0f, 155.0f, 28.0f};
    Rectangle livBtn{193.0f, 326.0f, 155.0f, 28.0f};
    bool hoverRep = CheckCollisionPointRec(mouse, repBtn);
    bool hoverLiv = CheckCollisionPointRec(mouse, livBtn);
    DrawRectangleRec(repBtn, hoverRep ? Color{45, 135, 185, 255} : Color{28, 85, 125, 235});
    DrawRectangleRec(livBtn, hoverLiv ? Color{145, 85, 185, 255} : Color{85, 48, 125, 235});
    DrawRectangleLinesEx(repBtn, 1.4f, Color{95, 215, 255, 255});
    DrawRectangleLinesEx(livBtn, 1.4f, Color{205, 145, 255, 255});
    DrawText("REPAIR ALL (FREE)", static_cast<int>(repBtn.x + 18), static_cast<int>(repBtn.y + 8), 11, WHITE);
    DrawText("CYCLE LIVERY [Y/P]", static_cast<int>(livBtn.x + 18), static_cast<int>(livBtn.y + 8), 11, WHITE);
    if (hoverRep && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ship.triggerEmergencyRepair(9999.0f);
    }
    if (hoverLiv && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        world.cyclePlayerLivery();
    }

    // Save / Load Custom Blueprint Buttons ([F5] / [F9])
    Rectangle saveBtn{28.0f, 360.0f, 155.0f, 28.0f};
    Rectangle loadBtn{193.0f, 360.0f, 155.0f, 28.0f};
    bool hoverSave = CheckCollisionPointRec(mouse, saveBtn);
    bool hoverLoad = CheckCollisionPointRec(mouse, loadBtn);
    DrawRectangleRec(saveBtn, hoverSave ? Color{45, 155, 115, 255} : Color{24, 95, 72, 235});
    DrawRectangleRec(loadBtn, hoverLoad ? Color{65, 115, 175, 255} : Color{32, 65, 108, 235});
    DrawRectangleLinesEx(saveBtn, 1.2f, Color{95, 235, 175, 220});
    DrawRectangleLinesEx(loadBtn, 1.2f, Color{95, 195, 255, 220});
    DrawText("SAVE SHIP [F5]", static_cast<int>(saveBtn.x + 24), static_cast<int>(saveBtn.y + 8), 12, WHITE);
    DrawText("LOAD SHIP [F9]", static_cast<int>(loadBtn.x + 24), static_cast<int>(loadBtn.y + 8), 12, WHITE);
    if (hoverSave && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ShipBlueprints::saveCustomBlueprint(ship, "custom_ship.aeth");
    }
    if (hoverLoad && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ShipBlueprints::loadCustomBlueprint(ship, "custom_ship.aeth");
    }

    std::snprintf(buf, sizeof(buf), "Hull Livery: %s", Airship::getHullLiveryName(ship.hullLiveryId));
    DrawText(buf, 28, 394, 12, Color{225, 185, 255, 255});

    // Selected Block Inspector
    const BlockSpec& selSpec = BlockRegistry::get(selectedBlock);
    DrawLine(28, 412, 348, 412, Color{55, 125, 165, 160});
    DrawText(selSpec.name, 28, 420, 15, Color{65, 235, 255, 255});
    std::snprintf(buf, sizeof(buf), "Category: %s  |  Mass: %.0ft  |  Max HP: %.0f",
                  selSpec.category, selSpec.mass, selSpec.maxHp);
    DrawText(buf, 28, 442, 12, Color{195, 215, 235, 255});
    std::snprintf(buf, sizeof(buf), "Build Cost: %d Gold, %d Aetherium",
                  selSpec.goldCost, selSpec.aetherCost);
    DrawText(buf, 28, 462, 12, Color{255, 215, 95, 255});

    DrawText(selSpec.description, 28, 484, 11, Color{165, 188, 210, 255});

    // 3. Bottom 20-Block Engineering Hotbar (2 Rows x 10 Columns)
    static const char* kShortTitles[21] = {
        "",
        "Timber Hull",
        "Iron Plate",
        "Aether Lift",
        "Aether Core",
        "Brass Prop",
        "Gyro Rudder",
        "Siege Gun",
        "Harpoon",
        "Tesla Spire",
        "Flak Gun",
        "Repair Rig",
        "Helm Bridge",
        "Orichalcum",
        "Shield Dome",
        "Plasma Lance",
        "Ramjet Boost",
        "Torpedo Pod",
        "Shock Harpoon",
        "Capacitor",
        "Flame Mortar"
    };
    static const char* kKeyLabels[21] = {
        "",
        "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]", "[0]",
        "[-]", "[=]", "[[]", "[]]", "[;]", "[']", "[,]", "[.]", "[/]", "[\\]"
    };

    int slotCols = 10;
    int slotW = 88;
    int slotH = 54;
    int pitchX = 92;
    int pitchY = 58;
    int barW = slotCols * pitchX + 12;
    int barH = 2 * pitchY + 10;
    int barX = screenW - barW - 16;
    int barY = screenH - barH - 8;

    DrawRectangle(barX, barY, barW, barH, Color{10, 16, 28, 244});
    DrawRectangleLinesEx({static_cast<float>(barX), static_cast<float>(barY), static_cast<float>(barW), static_cast<float>(barH)}, 1.5f, Color{65, 175, 225, 195});

    for (int i = 1; i <= 20; ++i) {
        int rIdx = (i - 1) / slotCols;
        int cIdx = (i - 1) % slotCols;
        BlockId bid = static_cast<BlockId>(i);
        const BlockSpec& sp = BlockRegistry::get(bid);
        Rectangle slot{
            static_cast<float>(barX + 8 + cIdx * pitchX),
            static_cast<float>(barY + 7 + rIdx * pitchY),
            static_cast<float>(slotW),
            static_cast<float>(slotH)
        };
        bool isSel = (selectedBlock == bid);
        bool hover = CheckCollisionPointRec(mouse, slot);

        DrawRectangleRec(slot, isSel ? Color{35, 95, 145, 255} : (hover ? Color{25, 48, 78, 245} : Color{16, 26, 44, 235}));
        DrawRectangleLinesEx(slot, isSel ? 2.2f : 1.0f, isSel ? Color{85, 245, 255, 255} : Color{65, 95, 135, 180});

        // Color swatch indicator
        DrawRectangle(static_cast<int>(slot.x + 5), static_cast<int>(slot.y + 5), 13, 13,
                      Color{sp.primaryColor.r, sp.primaryColor.g, sp.primaryColor.b, 255});
        DrawRectangleLines(static_cast<int>(slot.x + 5), static_cast<int>(slot.y + 5), 13, 13,
                           Color{sp.accentColor.r, sp.accentColor.g, sp.accentColor.b, 255});

        DrawText(kKeyLabels[i], static_cast<int>(slot.x + 58), static_cast<int>(slot.y + 5), 10, Color{165, 185, 205, 255});
        DrawText(kShortTitles[i], static_cast<int>(slot.x + 5), static_cast<int>(slot.y + 22), 10, WHITE);

        if (sp.aetherCost > 0) {
            std::snprintf(buf, sizeof(buf), "%dG+%dAe", sp.goldCost, sp.aetherCost);
        } else {
            std::snprintf(buf, sizeof(buf), "%d Gold", sp.goldCost);
        }
        DrawText(buf, static_cast<int>(slot.x + 5), static_cast<int>(slot.y + 38), 10, Color{255, 215, 85, 255});

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedBlock = bid;
        }
    }
}

} // namespace aethernaut
