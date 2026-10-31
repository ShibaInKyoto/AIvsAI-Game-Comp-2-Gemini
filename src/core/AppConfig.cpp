#include "core/AppConfig.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace aethernaut {

AppConfig AppConfig::parseArgs(int argc, char** argv) {
    AppConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--windowed") {
            cfg.windowed = true;
        } else if (arg == "--fullscreen") {
            cfg.windowed = false;
        } else if (arg == "--no-audio") {
            cfg.enableAudio = false;
        } else if (arg == "--reduce-flashing") {
            cfg.reduceFlashing = true;
        } else if (arg == "--perf") {
            cfg.showPerfOverlay = true;
        } else if (arg == "--size" && i + 1 < argc) {
            std::string dim = argv[++i];
            int w = 1600, h = 900;
            if (std::sscanf(dim.c_str(), "%dx%d", &w, &h) == 2 && w >= 640 && h >= 360) {
                cfg.windowWidth = w;
                cfg.windowHeight = h;
            }
        } else if (arg == "--seed" && i + 1 < argc) {
            cfg.worldSeed = static_cast<uint64_t>(std::strtoull(argv[++i], nullptr, 10));
        } else if (arg == "--autotest" && i + 1 < argc) {
            cfg.autotestSeconds = std::strtof(argv[++i], nullptr);
        } else if (arg == "--screenshot" && i + 1 < argc) {
            cfg.screenshotPath = argv[++i];
        } else if (arg == "--screenshot-at" && i + 1 < argc) {
            cfg.screenshotAtSeconds = std::strtof(argv[++i], nullptr);
        } else if (arg == "--ship" && i + 1 < argc) {
            cfg.initialBlueprintIndex = std::atoi(argv[++i]);
        } else if (arg == "--scene" && i + 1 < argc) {
            std::string sc = argv[++i];
            if (sc == "title" || sc == "0") cfg.startScene = InitialScene::Title;
            else if (sc == "flight" || sc == "campaign" || sc == "1") cfg.startScene = InitialScene::Flight;
            else if (sc == "drydock" || sc == "shipyard" || sc == "2") cfg.startScene = InitialScene::Drydock;
            else if (sc == "leviathan" || sc == "leviathan_hunt" || sc == "hunt" || sc == "3") cfg.startScene = InitialScene::LeviathanHunt;
            else if (sc == "skirmish" || sc == "fleet_skirmish" || sc == "4") cfg.startScene = InitialScene::FleetSkirmish;
        }
    }
    return cfg;
}

} // namespace aethernaut
