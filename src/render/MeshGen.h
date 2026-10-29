#pragma once

#include "raylib.h"

namespace aethernaut {

struct ProceduralModels {
    Model cube{};
    Model sphere{};
    Model cylinder{};
    Model cone{};
    Model torus{};
    bool loaded = false;

    void init(Shader worldShader);
    void unload();
};

} // namespace aethernaut
