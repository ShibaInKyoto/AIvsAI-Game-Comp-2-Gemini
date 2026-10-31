#pragma once

#include "sim/World.h"
#include "core/AppConfig.h"
#include "raylib.h"
#include <string>

namespace aethernaut {

enum class TitleAction {
    None = 0,
    StartCampaign,
    StartDailyVoyage,
    StartDrydockSandbox,
    StartLeviathanHunt,
    StartFleetSkirmish,
    OpenWondersArchive,
    OpenSettings,
    QuitGame
};

enum class PauseAction {
    None = 0,
    Resume,
    OpenDrydock,
    OpenWondersArchive,
    OpenSettings,
    OpenCodex,
    SaveCampaign,
    LoadCampaign,
    ReturnToTitle
};

enum class RunReportAction {
    None = 0,
    EmergencyRefloat,
    StartNewVoyage,
    ReturnToTitle
};

class HudRenderer {
public:
    static void initFonts();
    static void shutdownFonts();
    static Font& uiFont();
    static void drawTextAA(const char* text, int posX, int posY, int fontSize, Color color, bool dropShadow = true);
    static int measureTextAA(const char* text, int fontSize);

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

    void drawPauseModal(
        World& world,
        int screenW,
        int screenH,
        bool& inoutPaused,
        PauseAction& outAction
    );

    void drawSettingsModal(
        AppConfig& config,
        World& world,
        int screenW,
        int screenH,
        bool& inoutShowSettings
    );

    void drawWondersAndLogsModal(
        World& world,
        int screenW,
        int screenH,
        bool& inoutShowWonders
    );

    void drawRunReportModal(
        World& world,
        int screenW,
        int screenH,
        RunReportAction& outAction
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

    void drawMedalsModal(
        World& world,
        int screenW,
        int screenH,
        bool& inoutShowMedals
    );
};

} // namespace aethernaut
