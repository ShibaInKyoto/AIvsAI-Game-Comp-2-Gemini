#pragma once

#include "core/Math3D.h"
#include "raylib.h"

namespace aethernaut {

class ShaderPipeline {
public:
    void init(int width, int height);
    void shutdown();
    void checkResize(int width, int height);

    void updateWorldUniforms(
        const Vec3& cameraPos,
        const Vec3& sunDir,
        const Vec3& sunColor,
        const Vec3& skyAmbient,
        const Vec3& groundAmbient,
        const Vec3& fogColor,
        float fogDensity
    );

    void setEmissiveStrength(float strength);

    void beginScenePass();
    void endSceneAndDrawPostProcess(int screenW, int screenH, bool enableBloom, float stormFlash);

    Shader worldShader{};
    Shader postShader{};
    bool initialized = false;

private:
    RenderTexture2D sceneTarget_{};
    int targetW_ = 0;
    int targetH_ = 0;

    int locViewPos_ = -1;
    int locSunDir_ = -1;
    int locSunColor_ = -1;
    int locSkyAmbient_ = -1;
    int locGroundAmbient_ = -1;
    int locFogColor_ = -1;
    int locFogDensity_ = -1;
    int locEmissive_ = -1;

    int locResolution_ = -1;
    int locBloomStrength_ = -1;
    int locStormFlash_ = -1;
};

} // namespace aethernaut
