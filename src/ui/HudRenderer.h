#pragma once

#include "sim/World.h"
#include "core/AppConfig.h"
#include "raylib.h"

namespace aethernaut {

enum class TitleAction {
    None = 0,
    StartCampaign,
    StartDrydockSandbox,
    StartLeviathanHunt,
    StartFleetSkirmish,
    QuitGame
};

class HudRenderer {
public:
    void drawFlightHUD(
        World& world,
        const Camera3D& camera,
        int screenW,
        int screenH,
        bool canDockAtPort,
        const std::string& nearestPortName,
        bool& outRequestOpenDrydock
    );

    TitleAction drawTitleScreen(
        World& backgroundWorld,
        int screenW,
        int screenH,
        int& inoutSelectedBlueprint
    );

    void drawWorldMapModal(
        World& world,
        int screenW,
        int screenH,
        bool& inoutShowMap
    );

    void drawCrewModal(
        World& world,
        int screenW,
        int screenH,
        bool& inoutShowCrew
    );

    void drawCodexModal(
        int screenW,
        int screenH,
        bool& inoutShowCodex
    );

    void drawTradeAndContractsModal(
        World& world,
        int screenW,
        int screenH,
        bool& inoutShowLedger
    );

    void drawRelicVaultModal(
        World& world,
        int screenW,
        int screenH,
        bool& inoutShowRelics
    );
};

} // namespace aethernaut
