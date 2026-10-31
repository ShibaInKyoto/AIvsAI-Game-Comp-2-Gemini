#pragma once

#include "sim/World.h"
#include "render/ShaderPipeline.h"
#include "render/MeshGen.h"
#include "raylib.h"
#include <vector>

namespace aethernaut {

struct DrydockGhostPreview {
    bool active = false;
    Int3 coord{0, 0, 0};
    Int3 mirrorCoord{0, 0, 0};
    bool hasMirror = false;
    BlockId blockType = BlockId::WoodHull;
    uint8_t orientation = 0;
    bool showEngineeringGizmos = true;
};

struct VisualParticle3D {
    Vec3 position;
    Vec3 velocity;
    Color color;
    float size = 1.0f;
    float life = 1.0f;
    float maxLife = 1.0f;
};

class WorldRenderer {
public:
    void init(ShaderPipeline& pipeline);
    void shutdown();

    void updateEffects(World& world, float dt);

    void renderWorld3D(
        const World& world,
        const Camera3D& camera,
        ShaderPipeline& pipeline,
        const DrydockGhostPreview* drydockPreview = nullptr
    );

    void renderSingleBlock3D(
        BlockId id,
        uint8_t orientation,
        float propSpinAngle,
        float rudderAngle,
        float turretYaw,
        float turretPitch,
        float healthRatio,
        ShaderPipeline& pipeline,
        int liveryId = 0
    );

private:
    void renderSkyDomeAndSun(const Vec3& camPos, const Vec3& sunDir, float time, ShaderPipeline& pipeline);
    void renderCloudSea(const Vec3& camPos, float time, ShaderPipeline& pipeline);
    void renderIsland(const SkyIsland& isl, float time, ShaderPipeline& pipeline);
    void renderSkyEcosystemAndWonders(const World& world, const Vec3& camPos, float time, ShaderPipeline& pipeline);
    void renderAirship(
        const Airship& ship,
        ShaderPipeline& pipeline,
        const DrydockGhostPreview* drydockPreview,
        bool isPlayerShip,
        bool showCaptainAvatar,
        bool bowSearchlightOn = false
    );
    void renderLeviathan(const Leviathan& lev, float time, ShaderPipeline& pipeline);

    ProceduralModels models_;
    std::vector<VisualParticle3D> particles_;
    float animTime_ = 0.0f;
};

} // namespace aethernaut
