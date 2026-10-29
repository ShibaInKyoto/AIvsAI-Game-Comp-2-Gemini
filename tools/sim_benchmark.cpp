#include "sim/World.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>

using namespace aethernaut;

int main(int argc, char** argv) {
    int simSeconds = (argc > 1) ? std::atoi(argv[1]) : 120;
    std::printf("[Benchmark] Running %d seconds of 120Hz 3D Archipelago & Leviathan simulation...\n", simSeconds);

    World world;
    world.initWorld(20261028ULL, 2, false);
    world.playerAutopilot = true;

    auto t0 = std::chrono::high_resolution_clock::now();
    const int totalSteps = simSeconds * 120;
    for (int i = 0; i < totalSteps; ++i) {
        world.step(1.0f / 120.0f);
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    double elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double msPerStep = elapsedMs / static_cast<double>(totalSteps);

    std::printf("[Benchmark] Completed %d physics steps in %.2f ms (%.4f ms/step, %.0f steps/sec)\n",
                totalSteps, elapsedMs, msPerStep, 1000.0 / std::max(1e-6, msPerStep));
    std::printf("[Benchmark] Final Player Hull: %.0f/%.0f | Gold: %d | Pirates Sunk: %d | Leviathans Slain: %d\n",
                world.playerShip.totalHp, world.playerShip.totalMaxHp,
                world.playerGold, world.piratesSunk, world.leviathansSlain);
    return 0;
}
