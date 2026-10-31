#include "core/AppConfig.h"
#include "core/Math3D.h"
#include "sim/World.h"
#include "sim/ShipBlueprints.h"
#include "render/ShaderPipeline.h"
#include "render/WorldRenderer.h"
#include "audio/SynthAudio.h"
#include "ui/DrydockEditor.h"
#include "ui/HudRenderer.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace aethernaut;

namespace {

enum class ActiveScreen {
    Title,
    Flight,
    Drydock
};

// Raycasts the 3D world from the mouse cursor to determine where the player's turrets should aim in 3D
Vec3 computePlayer3DAimPoint(const World& world, const Camera3D& camera, Vector2 mousePos) {
    Ray rlRay = GetScreenToWorldRay(mousePos, camera);
    Ray3D ray{
        {rlRay.position.x, rlRay.position.y, rlRay.position.z},
        {rlRay.direction.x, rlRay.direction.y, rlRay.direction.z}
    };

    float bestDist = 380.0f;
    Vec3 bestHit = ray.origin + ray.dir * 240.0f;

    // 1. Check ray against Colossal Sky-Leviathans
    for (const auto& lev : world.leviathans) {
        if (lev.isSlain) continue;
        int segIdx = -1;
        Vec3 hitPos{};
        float d = 0.0f;
        if (lev.raycast(ray, bestDist, segIdx, hitPos, d)) {
            bestDist = d;
            bestHit = hitPos;
        }
    }

    // 2. Check ray against Enemy Airships
    for (const auto& npc : world.npcShips) {
        if (npc.isDestroyed || npc.faction() == 0) continue;
        Int3 hc{}, ac{};
        Vec3 hp{};
        float d = 0.0f;
        if (npc.raycastBlocks(ray, bestDist, hc, ac, hp, d)) {
            bestDist = d;
            bestHit = hp;
        }
    }

    return bestHit;
}

void setupLeviathanHuntScenario(World& world, float& camYaw, float& camPitch, float& camDist) {
    world.initWorld(1337ULL, 1, false); // Pequod's Wrath Whaler Brig
    world.skyAtmospherePreset = 2;      // Dramatic Crimson Sundown
    world.playerShip.position = {0.0f, 148.0f, -480.0f};
    world.playerShip.velocity = {0.0f, 0.0f, -15.0f};
    world.playerShip.targetCruiseAltitude = 148.0f;
    world.playerSearchlightOn = true;
    world.leviathans.clear();
    world.spawnLeviathanEncounter(LeviathanSpecies::CrimsonStormWyrm, {-28.0f, 155.0f, -568.0f});
    world.spawnLeviathanEncounter(LeviathanSpecies::AzureSkyWhale, {58.0f, 145.0f, -595.0f});
    // Fire an initial harpoon & broadside toward the Crimson Storm-Wyrm!
    if (!world.leviathans.empty() && !world.leviathans[0].segments.empty()) {
        Vec3 targetPos = world.leviathans[0].segments[2].position;
        world.firePlayerWeapons(true, true, targetPos);
        // Also latch an active Harpoon Tether onto segment 2 for immediate tension physics
        HarpoonTether tether;
        tether.id = 9001;
        tether.sourceShipId = world.playerShip.id();
        tether.sourceLocalOffset = {0.0f, 2.0f, -10.0f};
        tether.targetLeviathanId = world.leviathans[0].id();
        tether.targetSegmentIdx = 2;
        tether.currentLength = (targetPos - world.playerShip.position).length();
        tether.restLength = tether.currentLength * 0.85f;
        world.tethers.push_back(tether);
        world.statusBannerText = "HARPOON LATCHED onto Draconis Fulmen! Hold [R] to Reel In, [LMB] Broadsides!";
        world.statusBannerTimer = 6.0f;
    }
    camYaw = 0.26f;
    camPitch = 0.21f;
    camDist = 31.0f;
}

void setupFleetSkirmishScenario(World& world, float& camYaw, float& camPitch, float& camDist) {
    world.initWorld(1337ULL, 2, false); // IJN Valkyrie Ironclad Frigate
    world.skyAtmospherePreset = 0;      // Golden Dawn
    world.playerShip.position = {0.0f, 148.0f, -480.0f};
    world.playerShip.velocity = {0.0f, 0.0f, -14.0f};
    world.playerShip.targetCruiseAltitude = 148.0f;
    world.npcShips.clear();
    world.spawnAlliedWingman(3, {32.0f, 150.0f, -504.0f});
    world.spawnPirateSquadron(3, {-16.0f, 151.0f, -534.0f});
    // Upgrade pirate squadron to armored Frigates & Dreadnoughts in broadside formation
    if (world.npcShips.size() >= 4) {
        ShipBlueprints::applyBlueprint(world.npcShips[1], 2);
        world.npcShips[1].setName("Corsair Ironclad");
        world.npcShips[1].position = {-34.0f, 150.0f, -528.0f};
        world.npcShips[1].yaw = 0.18f;

        ShipBlueprints::applyBlueprint(world.npcShips[2], 4);
        world.npcShips[2].setName("Corsair Flagship");
        world.npcShips[2].position = {-4.0f, 154.0f, -560.0f};
        world.npcShips[2].yaw = 1.45f;

        ShipBlueprints::applyBlueprint(world.npcShips[3], 1);
        world.npcShips[3].setName("Corsair Raider");
        world.npcShips[3].position = {28.0f, 151.0f, -542.0f};
        world.npcShips[3].yaw = -0.35f;
    }
    world.firePlayerWeapons(true, false, {-34.0f, 150.0f, -528.0f});
    world.statusBannerText = "SKY-ARMADA SKIRMISH: Allied Squadron Engaging Corsair Armada!";
    world.statusBannerTimer = 6.0f;
    camYaw = 0.28f;
    camPitch = 0.20f;
    camDist = 32.0f;
}

} // namespace

int main(int argc, char** argv) {
    AppConfig config = AppConfig::parseArgs(argc, argv);

    unsigned int flags = FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT;
    if (config.vsync) {
        flags |= FLAG_VSYNC_HINT;
    }
    SetConfigFlags(flags);
    InitWindow(config.windowWidth, config.windowHeight, "AETHERNAUT: 3D Sky-Fleet Architect & Leviathan Hunter");
    SetExitKey(KEY_NULL);
    SetTargetFPS(144);

    HudRenderer::initFonts();

    ShaderPipeline shaderPipeline;
    shaderPipeline.init(GetScreenWidth(), GetScreenHeight());

    WorldRenderer worldRenderer;
    worldRenderer.init(shaderPipeline);

    SynthAudio synthAudio;
    synthAudio.init(config.enableAudio);

    World world;
    int selectedBlueprint = config.initialBlueprintIndex;
    world.initWorld(config.worldSeed, selectedBlueprint, false);

    // 3D Orbit Camera State
    Camera3D camera{};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = config.cameraFov;
    camera.projection = CAMERA_PERSPECTIVE;

    float camYaw = 0.55f;
    float camPitch = 0.23f;
    float camDist = 29.0f;

    ActiveScreen currentScreen = ActiveScreen::Title;
    if (config.startScene == InitialScene::Flight) {
        currentScreen = ActiveScreen::Flight;
        camYaw = 0.18f;
        camPitch = 0.24f;
        camDist = 34.0f;
    } else if (config.startScene == InitialScene::Drydock) {
        currentScreen = ActiveScreen::Drydock;
        world.isSandboxMode = true;
        world.playerGold = 99999;
        world.playerAether = 999;
        camYaw = 0.62f;
        camPitch = 0.32f;
        camDist = 28.0f;
    } else if (config.startScene == InitialScene::LeviathanHunt) {
        currentScreen = ActiveScreen::Flight;
        setupLeviathanHuntScenario(world, camYaw, camPitch, camDist);
    } else if (config.startScene == InitialScene::FleetSkirmish) {
        currentScreen = ActiveScreen::Flight;
        setupFleetSkirmishScenario(world, camYaw, camPitch, camDist);
    }

    if (config.autotestSeconds > 0.0f) {
        world.playerAutopilot = true;
    }

    DrydockEditor drydockEditor;
    HudRenderer hudRenderer;
    bool showWorldMap = false;
    bool showCrewModal = false;
    bool showCodex = false;
    bool showLedgerModal = false;
    bool showRelicsModal = false;
    bool showMedalsModal = false;
    bool showWondersModal = false;
    bool showSettingsModal = false;
    bool showPauseModal = false;
    bool shouldExitGame = false;

    auto closeSubModals = [&]() {
        showWorldMap = false;
        showCrewModal = false;
        showCodex = false;
        showLedgerModal = false;
        showRelicsModal = false;
        showMedalsModal = false;
        showWondersModal = false;
        showSettingsModal = false;
    };

    auto anySubModalOpen = [&]() -> bool {
        return showWorldMap || showCrewModal || showCodex || showLedgerModal ||
               showRelicsModal || showMedalsModal || showWondersModal || showSettingsModal;
    };

    float totalElapsed = 0.0f;
    float physicsAccumulator = 0.0f;
    constexpr float kFixedStep = 1.0f / 120.0f;
    bool screenshotCaptured = false;

    while (!WindowShouldClose() && !shouldExitGame) {
        float frameDt = std::min(0.05f, GetFrameTime());
        totalElapsed += frameDt;

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        shaderPipeline.checkResize(screenW, screenH);
        camera.fovy = config.cameraFov;

        if (IsKeyPressed(KEY_F3)) {
            config.showPerfOverlay = !config.showPerfOverlay;
        }
        if (IsKeyPressed(KEY_V)) {
            world.skyAtmospherePreset = (world.skyAtmospherePreset + 1) % 4;
            const char* kAtmosNames[4] = {"Golden Dawn", "High Azure Noon", "Crimson Sundown", "Starlit Aether Night"};
            world.statusBannerText = std::string("Sky Atmosphere Set: ") + kAtmosNames[world.skyAtmospherePreset];
            world.statusBannerTimer = 3.0f;
        }

        // Camera Orbit Controls (Right-Mouse Drag in Drydock, or Mouse Wheel Zoom, or Q/E Orbit)
        float wheel = GetMouseWheelMove();
        if (std::abs(wheel) > 0.01f && !anySubModalOpen() && !showPauseModal) {
            camDist = std::clamp(camDist - wheel * 4.5f, 16.0f, 145.0f);
        }
        if (!showPauseModal && !anySubModalOpen()) {
            if (IsKeyDown(KEY_Q)) camYaw -= 1.4f * frameDt;
            if (IsKeyDown(KEY_E)) camYaw += 1.4f * frameDt;
        }

        if (currentScreen == ActiveScreen::Drydock && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            camYaw -= delta.x * 0.007f;
            camPitch = std::clamp(camPitch + delta.y * 0.007f, -0.25f, 1.25f);
        }

        // Check nearest Drydock Port distance
        float portDist = 1e30f;
        const SkyIsland* nearestPort = world.archipelago.findNearestPort(world.playerShip.position, portDist);
        bool canDockAtPort = (nearestPort != nullptr && portDist < nearestPort->radius + 95.0f);
        std::string nearestPortName = nearestPort ? nearestPort->name : "Drydock";

        bool isRunEnded = (currentScreen == ActiveScreen::Flight) && (world.playerShip.isDestroyed || world.victoryAchieved);

        if (currentScreen == ActiveScreen::Title) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (anySubModalOpen()) closeSubModals();
            }
            // Live AI attract battle in background on title screen while orbiting the flagship
            world.playerAutopilot = true;
            physicsAccumulator += frameDt;
            while (physicsAccumulator >= kFixedStep) {
                world.step(kFixedStep);
                physicsAccumulator -= kFixedStep;
            }
            camYaw += 0.30f * frameDt;
            camPitch = 0.23f;
            camDist = 28.0f;
        } else if (currentScreen == ActiveScreen::Flight) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (anySubModalOpen()) {
                    closeSubModals();
                } else if (!isRunEnded) {
                    showPauseModal = !showPauseModal;
                }
            }

            if (!showPauseModal && !isRunEnded) {
                if (IsKeyPressed(KEY_P)) {
                    world.playerAutopilot = !world.playerAutopilot;
                }
                if (IsKeyPressed(KEY_Y)) {
                    world.cyclePlayerLivery();
                }
                if (IsKeyPressed(KEY_L)) {
                    world.togglePlayerSearchlight();
                }
                if (IsKeyPressed(KEY_TAB)) {
                    world.isPlayerWalkingOnDeck = !world.isPlayerWalkingOnDeck;
                }
                if (IsKeyPressed(KEY_M)) {
                    bool next = !showWorldMap;
                    closeSubModals();
                    showWorldMap = next;
                }
                if (IsKeyPressed(KEY_K)) {
                    bool next = !showCrewModal;
                    closeSubModals();
                    showCrewModal = next;
                }
                if (IsKeyPressed(KEY_J)) {
                    bool next = !showLedgerModal;
                    closeSubModals();
                    showLedgerModal = next;
                }
                if (IsKeyPressed(KEY_U)) {
                    bool next = !showRelicsModal;
                    closeSubModals();
                    showRelicsModal = next;
                }
                if (IsKeyPressed(KEY_O)) {
                    bool next = !showMedalsModal;
                    closeSubModals();
                    showMedalsModal = next;
                }
                if (IsKeyPressed(KEY_F2)) {
                    bool next = !showWondersModal;
                    closeSubModals();
                    showWondersModal = next;
                }
                if (IsKeyPressed(KEY_H) || IsKeyPressed(KEY_F1)) {
                    bool next = !showCodex;
                    closeSubModals();
                    showCodex = next;
                }
                if (IsKeyPressed(KEY_Z)) {
                    world.cycleFleetOrder();
                }
                if (IsKeyPressed(KEY_T)) {
                    world.cycleWeatherState();
                }
                if (IsKeyPressed(KEY_F8)) {
                    world.hireAlliedWingman();
                }
                if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ONE)) {
                    world.triggerDamageControl();
                }
                if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_TWO)) {
                    world.triggerSonarPing();
                }
                if (IsKeyPressed(KEY_G) || IsKeyPressed(KEY_THREE)) {
                    world.deploySkyMine();
                }
                if (IsKeyPressed(KEY_FOUR)) {
                    world.triggerEmpNova();
                }
                if (IsKeyPressed(KEY_N)) {
                    world.summonApexLeviathan();
                }
                if (IsKeyPressed(KEY_I)) {
                    world.spawnCorsairDreadArk();
                }
                if (IsKeyPressed(KEY_F6)) {
                    world.saveCampaign("campaign_save.aeth");
                }
                if (IsKeyPressed(KEY_F7)) {
                    world.loadCampaign("campaign_save.aeth");
                }
                if (IsKeyPressed(KEY_B) && (canDockAtPort || world.isSandboxMode)) {
                    closeSubModals();
                    currentScreen = ActiveScreen::Drydock;
                    camYaw = 0.62f;
                    camPitch = 0.32f;
                    camDist = 28.0f;
                }
            }

            // Player Manual Flight / Deck Controls
            if (!world.playerAutopilot && !showPauseModal && !isRunEnded && !anySubModalOpen()) {
                if (world.isPlayerWalkingOnDeck) {
                    float fwd = 0.0f, right = 0.0f;
                    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) fwd += 1.0f;
                    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) fwd -= 1.0f;
                    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) right += 1.0f;
                    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) right -= 1.0f;
                    world.updateCaptainDeckWalk(frameDt, fwd, right);
                } else {
                    float throttle = 0.0f;
                    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) throttle += 1.0f;
                    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) throttle -= 0.7f;
                    world.playerShip.throttleInput = throttle;
                    world.playerShip.afterburnerInput = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));

                    float rudder = 0.0f;
                    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) rudder += 1.0f;
                    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) rudder -= 1.0f;
                    world.playerShip.rudderInput = rudder;

                    float alt = 0.0f;
                    if (IsKeyDown(KEY_SPACE)) alt += 1.0f;
                    if (IsKeyDown(KEY_LEFT_CONTROL)) alt -= 1.0f;
                    world.playerShip.altitudeInput = alt;
                }

                // Weapon Firing & Harpoon Winch Reeling
                Vec3 aimWorld = computePlayer3DAimPoint(world, camera, GetMousePosition());
                world.playerAimWorldPos = aimWorld;
                bool fireCannons = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
                bool fireHarpoons = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
                if (fireCannons || fireHarpoons) {
                    world.firePlayerWeapons(fireCannons, fireHarpoons, aimWorld);
                }
                world.setReelHarpoons(IsKeyDown(KEY_R));
                if (IsKeyPressed(KEY_F)) {
                    world.releaseAllPlayerHarpoons();
                }
            }

            // Fixed-timestep 120Hz 3D Physics Step (paused when Pause Modal or Run Report is up)
            if (!showPauseModal && !isRunEnded) {
                physicsAccumulator += frameDt;
                while (physicsAccumulator >= kFixedStep) {
                    world.step(kFixedStep);
                    physicsAccumulator -= kFixedStep;
                }
            }
        } else if (currentScreen == ActiveScreen::Drydock) {
            bool exitDrydock = false;
            drydockEditor.update(world, camera, screenW, screenH, exitDrydock);
            if (exitDrydock || IsKeyPressed(KEY_ESCAPE)) {
                currentScreen = ActiveScreen::Flight;
                camYaw = 0.18f;
                camPitch = 0.24f;
                camDist = 34.0f;
            }
        }

        // Update 3D Camera Position around Player Flagship
        Vec3 shipPos = world.playerShip.position;
        if (world.isPlayerWalkingOnDeck && currentScreen == ActiveScreen::Flight) {
            Vec3 capWorld = world.playerShip.localToWorldPos(world.playerShip.captainLocalPos + Vec3{0.0f, 3.2f, 6.5f});
            Vec3 capLook = world.playerShip.localToWorldPos(world.playerShip.captainLocalPos + Vec3{0.0f, 1.5f, -12.0f});
            camera.position = {capWorld.x, capWorld.y, capWorld.z};
            camera.target = {capLook.x, capLook.y, capLook.z};
        } else {
            float effectiveYaw = (currentScreen == ActiveScreen::Drydock || currentScreen == ActiveScreen::Title)
                ? camYaw
                : (world.playerShip.yaw + camYaw);
            Vec3 offset{
                std::sin(effectiveYaw) * std::cos(camPitch) * camDist,
                std::sin(camPitch) * camDist + 4.5f,
                std::cos(effectiveYaw) * std::cos(camPitch) * camDist
            };
            // On Title Screen, shift camera target slightly left so the 3D flagship sits prominently in the right 68% of the viewport
            Vec3 targetShift{0.0f, 2.2f, 0.0f};
            if (currentScreen == ActiveScreen::Title) {
                Vec3 camRight{std::cos(effectiveYaw), 0.0f, -std::sin(effectiveYaw)};
                targetShift -= camRight * 5.8f;
            }
            // Subtle physical 3D camera recoil / impact shake from world.cameraTrauma (scaled down when reduceFlashing is active)
            float traumaScale = config.reduceFlashing ? 0.25f : 1.35f;
            float shakeMag = world.cameraTrauma * world.cameraTrauma * traumaScale;
            Vec3 shakeOffset{
                std::sin(totalElapsed * 47.0f) * shakeMag,
                std::cos(totalElapsed * 53.0f) * shakeMag * 0.7f,
                std::sin(totalElapsed * 39.0f) * shakeMag
            };
            camera.position = {shipPos.x + offset.x + shakeOffset.x, shipPos.y + offset.y + shakeOffset.y, shipPos.z + offset.z + shakeOffset.z};
            camera.target = {shipPos.x + targetShift.x + shakeOffset.x * 0.4f, shipPos.y + targetShift.y + shakeOffset.y * 0.4f, shipPos.z + targetShift.z};
        }

        // Update Visual Particles & Procedural Audio
        if (!showPauseModal) {
            worldRenderer.updateEffects(world, frameDt);
        }
        synthAudio.processWorldEvents(world, shipPos, config.sfxVolume, config.musicVolume);

        // Compute storm lightning flash intensity
        float stormFlash = 0.0f;
        for (const auto& st : world.archipelago.storms) {
            stormFlash = std::max(stormFlash, st.lightningFlashAlpha);
        }
        if (config.reduceFlashing) {
            stormFlash *= 0.15f;
        }

        // ---------------------------------------------------------------------
        // Render 3D Scene into HDR Offscreen Buffer + Post-Process Bloom
        // ---------------------------------------------------------------------
        shaderPipeline.beginScenePass();
        worldRenderer.renderWorld3D(
            world,
            camera,
            shaderPipeline,
            (currentScreen == ActiveScreen::Drydock) ? &drydockEditor.ghostPreview : nullptr
        );

        BeginDrawing();
        shaderPipeline.endSceneAndDrawPostProcess(screenW, screenH, config.enableBloom, stormFlash);

        // ---------------------------------------------------------------------
        // Render 2D Vector HUD / Drydock Shipyard UI / Title Menu
        // ---------------------------------------------------------------------
        if (currentScreen == ActiveScreen::Title) {
            TitleAction act = hudRenderer.drawTitleScreen(world, screenW, screenH, selectedBlueprint);
            if (act == TitleAction::StartCampaign) {
                world.initWorld(config.worldSeed, selectedBlueprint, false);
                world.playerAutopilot = false;
                closeSubModals();
                camYaw = 0.18f;
                camPitch = 0.24f;
                camDist = 34.0f;
                currentScreen = ActiveScreen::Flight;
            } else if (act == TitleAction::StartDailyVoyage) {
                world.startDailySkyVoyage();
                world.playerAutopilot = false;
                closeSubModals();
                camYaw = 0.18f;
                camPitch = 0.24f;
                camDist = 34.0f;
                currentScreen = ActiveScreen::Flight;
            } else if (act == TitleAction::StartDrydockSandbox) {
                world.initWorld(config.worldSeed, selectedBlueprint, true);
                world.playerAutopilot = false;
                closeSubModals();
                camYaw = 0.62f;
                camPitch = 0.32f;
                camDist = 28.0f;
                currentScreen = ActiveScreen::Drydock;
            } else if (act == TitleAction::StartLeviathanHunt) {
                world.playerAutopilot = false;
                closeSubModals();
                setupLeviathanHuntScenario(world, camYaw, camPitch, camDist);
                currentScreen = ActiveScreen::Flight;
            } else if (act == TitleAction::StartFleetSkirmish) {
                world.playerAutopilot = false;
                closeSubModals();
                setupFleetSkirmishScenario(world, camYaw, camPitch, camDist);
                currentScreen = ActiveScreen::Flight;
            } else if (act == TitleAction::OpenWondersArchive) {
                closeSubModals();
                showWondersModal = true;
            } else if (act == TitleAction::OpenSettings) {
                closeSubModals();
                showSettingsModal = true;
            } else if (act == TitleAction::QuitGame) {
                shouldExitGame = true;
            }

            if (showWondersModal) {
                hudRenderer.drawWondersAndLogsModal(world, screenW, screenH, showWondersModal);
            }
            if (showSettingsModal) {
                hudRenderer.drawSettingsModal(config, world, screenW, screenH, showSettingsModal);
            }
        } else if (currentScreen == ActiveScreen::Flight) {
            bool requestOpenDrydock = false;
            hudRenderer.drawFlightHUD(
                world, camera, screenW, screenH,
                canDockAtPort, nearestPortName, requestOpenDrydock
            );
            if (requestOpenDrydock) {
                closeSubModals();
                currentScreen = ActiveScreen::Drydock;
                camYaw = 0.62f;
                camPitch = 0.32f;
                camDist = 28.0f;
            }
            if (showWorldMap) {
                hudRenderer.drawWorldMapModal(world, screenW, screenH, showWorldMap);
            }
            if (showCrewModal) {
                hudRenderer.drawCrewModal(world, screenW, screenH, showCrewModal);
            }
            if (showLedgerModal) {
                hudRenderer.drawTradeAndContractsModal(world, screenW, screenH, showLedgerModal);
            }
            if (showRelicsModal) {
                hudRenderer.drawRelicVaultModal(world, screenW, screenH, showRelicsModal);
            }
            if (showMedalsModal) {
                hudRenderer.drawMedalsModal(world, screenW, screenH, showMedalsModal);
            }
            if (showWondersModal) {
                hudRenderer.drawWondersAndLogsModal(world, screenW, screenH, showWondersModal);
            }
            if (showCodex) {
                hudRenderer.drawCodexModal(screenW, screenH, showCodex);
            }
            if (showSettingsModal) {
                hudRenderer.drawSettingsModal(config, world, screenW, screenH, showSettingsModal);
            }
            if (isRunEnded) {
                RunReportAction rAct = RunReportAction::None;
                hudRenderer.drawRunReportModal(world, screenW, screenH, rAct);
                if (rAct == RunReportAction::EmergencyRefloat) {
                    if (world.playerShip.isDestroyed) {
                        world.emergencyRefloatPlayerShip();
                    } else {
                        world.victoryAchieved = false; // Continue free-roaming voyage
                    }
                } else if (rAct == RunReportAction::StartNewVoyage) {
                    if (world.isDailyVoyage) {
                        world.startDailySkyVoyage();
                    } else {
                        world.initWorld(config.worldSeed, selectedBlueprint, false);
                    }
                    world.playerAutopilot = false;
                } else if (rAct == RunReportAction::ReturnToTitle) {
                    closeSubModals();
                    showPauseModal = false;
                    currentScreen = ActiveScreen::Title;
                }
            } else if (showPauseModal) {
                PauseAction pAct = PauseAction::None;
                hudRenderer.drawPauseModal(world, screenW, screenH, showPauseModal, pAct);
                if (pAct == PauseAction::Resume) {
                    showPauseModal = false;
                } else if (pAct == PauseAction::OpenDrydock) {
                    showPauseModal = false;
                    closeSubModals();
                    currentScreen = ActiveScreen::Drydock;
                    camYaw = 0.62f;
                    camPitch = 0.32f;
                    camDist = 28.0f;
                } else if (pAct == PauseAction::OpenSettings) {
                    showPauseModal = false;
                    showSettingsModal = true;
                } else if (pAct == PauseAction::OpenWondersArchive) {
                    showPauseModal = false;
                    showWondersModal = true;
                } else if (pAct == PauseAction::OpenCodex) {
                    showPauseModal = false;
                    showCodex = true;
                } else if (pAct == PauseAction::SaveCampaign) {
                    world.saveCampaign("campaign_save.aeth");
                    showPauseModal = false;
                } else if (pAct == PauseAction::LoadCampaign) {
                    world.loadCampaign("campaign_save.aeth");
                    showPauseModal = false;
                } else if (pAct == PauseAction::ReturnToTitle) {
                    world.saveCampaign("campaign_save.aeth");
                    showPauseModal = false;
                    closeSubModals();
                    currentScreen = ActiveScreen::Title;
                }
            }
        } else if (currentScreen == ActiveScreen::Drydock) {
            bool exitDrydock = false;
            drydockEditor.drawUI(world, screenW, screenH, exitDrydock);
            if (exitDrydock) {
                currentScreen = ActiveScreen::Flight;
                camYaw = 0.18f;
                camPitch = 0.24f;
                camDist = 34.0f;
            }
        }

        if (config.showPerfOverlay) {
            DrawRectangle(14, 14, 210, 54, Color{8, 14, 24, 215});
            DrawFPS(24, 20);
            char pbuf[64];
            std::snprintf(pbuf, sizeof(pbuf), "Blocks: %d | Proj: %zu",
                          world.playerShip.occupiedBlockCount, world.projectiles.size());
            HudRenderer::drawTextAA(pbuf, 24, 42, 13, Color{145, 225, 255, 255});
        }

        EndDrawing();

        // Automated Screenshot Capture Flag (supports nested paths like docs/screenshots/*.png)
        if (!config.screenshotPath.empty() && !screenshotCaptured && totalElapsed >= config.screenshotAtSeconds) {
            Image shot = LoadImageFromScreen();
            ExportImage(shot, config.screenshotPath.c_str());
            UnloadImage(shot);
            screenshotCaptured = true;
            if (config.autotestSeconds <= 0.0f) {
                break;
            }
        }

        if (config.autotestSeconds > 0.0f && totalElapsed >= config.autotestSeconds) {
            break;
        }
    }

    synthAudio.shutdown();
    worldRenderer.shutdown();
    shaderPipeline.shutdown();
    HudRenderer::shutdownFonts();
    CloseWindow();
    return 0;
}
