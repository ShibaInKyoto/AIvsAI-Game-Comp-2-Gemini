#pragma once

#include <string>
#include <cstdint>

namespace aethernaut {

enum class InitialScene {
    Title,
    Flight,
    Drydock,
    LeviathanHunt,
    FleetSkirmish
};

struct AppConfig {
    int windowWidth = 1600;
    int windowHeight = 900;
    bool windowed = true;
    bool vsync = true;
    bool enableAudio = true;
    bool enableBloom = true;
    bool enableClouds = true;
    bool reduceFlashing = false;
    bool showPerfOverlay = false;
    float masterVolume = 0.82f;
    float musicVolume = 0.58f;
    float sfxVolume = 0.86f;
    float cameraFov = 58.0f;
    uint64_t worldSeed = 20261028ULL;

    // CLI automation & verification flags
    float autotestSeconds = 0.0f;
    std::string screenshotPath;
    float screenshotAtSeconds = 1.5f;
    int initialBlueprintIndex = 0;
    InitialScene startScene = InitialScene::Title;

    static AppConfig parseArgs(int argc, char** argv);
};

} // namespace aethernaut
