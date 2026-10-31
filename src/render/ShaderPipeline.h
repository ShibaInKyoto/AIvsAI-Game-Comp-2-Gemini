#pragma once

#include "core/Math3D.h"
#include "raylib.h"

namespace aethernaut {

enum class WorldMaterialType : int {
    Default = 0,
    IslandRock = 1,
    IslandTurf = 2,
    WoodHull = 3,
    MetalArmor = 4,
    GasEnvelope = 5,
    LeviathanHide = 6,
    CloudSea = 7,
    AetherCrystal = 8
};

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
        float fogDensity,
        float simTime = 0.0f
    );

    void updateDynamicLights(
        const Vec3& searchlightPos,
        const Vec3& searchlightDir,
        float searchlightActive,
        const Vec3& bossLightPos,
        const Vec3& bossLightColor,
        float bossLightIntensity,
        const Vec3& sonarOrigin,
        float sonarRadius,
        float sonarStrength
    );

    void setEmissiveStrength(float strength);
    void setMaterialType(WorldMaterialType matType);

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
    int locMaterialType_ = -1;
    int locTime_ = -1;

    int locSearchlightPos_ = -1;
    int locSearchlightDir_ = -1;
    int locSearchlightActive_ = -1;

    int locBossLightPos_ = -1;
    int locBossLightColor_ = -1;
    int locBossLightIntensity_ = -1;

    int locSonarOrigin_ = -1;
    int locSonarRadius_ = -1;
    int locSonarStrength_ = -1;

    int locResolution_ = -1;
    int locBloomStrength_ = -1;
    int locStormFlash_ = -1;
};

} // namespace aethernaut
