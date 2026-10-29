#include "render/WorldRenderer.h"
#include "rlgl.h"
#include <cmath>
#include <algorithm>

namespace aethernaut {

namespace {

Color toRayColor(const ColorRGB& c, uint8_t alpha = 255) {
    return Color{c.r, c.g, c.b, alpha};
}

Color darkenColor(Color c, float factor) {
    return Color{
        static_cast<uint8_t>(std::clamp(c.r * factor, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp(c.g * factor, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp(c.b * factor, 0.0f, 255.0f)),
        c.a
    };
}

} // namespace

void WorldRenderer::init(ShaderPipeline& pipeline) {
    models_.init(pipeline.worldShader);
    particles_.reserve(1536);
}

void WorldRenderer::shutdown() {
    models_.unload();
}

void WorldRenderer::updateEffects(World& world, float dt) {
    animTime_ += dt;

    // Convert simulation explosion events into 3D visual particles
    for (const auto& exp : world.explosions) {
        int count = exp.isElectricArc ? 16 : 26;
        for (int i = 0; i < count; ++i) {
            float theta = static_cast<float>(GetRandomValue(0, 628)) * 0.01f;
            float phi = static_cast<float>(GetRandomValue(-314, 314)) * 0.005f;
            float spd = static_cast<float>(GetRandomValue(8, 38));
            VisualParticle3D pt;
            pt.position = exp.position;
            pt.velocity = {
                std::cos(phi) * std::cos(theta) * spd,
                std::sin(phi) * spd + 6.0f,
                std::cos(phi) * std::sin(theta) * spd
            };
            pt.color = toRayColor(exp.color);
            pt.size = exp.radius * 0.30f;
            pt.life = 0.48f + static_cast<float>(GetRandomValue(0, 42)) * 0.01f;
            pt.maxLife = pt.life;
            particles_.push_back(pt);
        }
    }
    world.explosions.clear();

    // Update active particles
    for (auto& p : particles_) {
        p.position += p.velocity * dt;
        p.velocity *= std::pow(0.25f, dt);
        p.life -= dt;
    }
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(), [](const VisualParticle3D& p) { return p.life <= 0.0f; }),
        particles_.end()
    );
}

void WorldRenderer::renderWorld3D(
    const World& world,
    const Camera3D& camera,
    ShaderPipeline& pipeline,
    const DrydockGhostPreview* drydockPreview
) {
    Vec3 camPos{camera.position.x, camera.position.y, camera.position.z};

    // Select Sky-Atmosphere Lighting & Fog Palette (0: Golden Dawn, 1: High Noon, 2: Crimson Dusk, 3: Starlit Night)
    Vec3 sunDir = Vec3{-0.42f, -0.62f, 0.66f}.normalized();
    Vec3 sunCol{1.08f, 0.96f, 0.82f};
    Vec3 skyAmb{0.38f, 0.52f, 0.72f};
    Vec3 grdAmb{0.22f, 0.27f, 0.38f};
    Vec3 fogCol{0.50f, 0.68f, 0.88f};
    Color clearBg{96, 146, 206, 255};

    switch (world.skyAtmospherePreset % 4) {
        case 1: // High Azure Noon
            sunDir = Vec3{-0.22f, -0.86f, 0.46f}.normalized();
            sunCol = {1.12f, 1.08f, 0.98f};
            skyAmb = {0.46f, 0.62f, 0.84f};
            grdAmb = {0.26f, 0.34f, 0.45f};
            fogCol = {0.58f, 0.78f, 0.96f};
            clearBg = Color{112, 174, 232, 255};
            break;
        case 2: // Crimson Sundown
            sunDir = Vec3{-0.62f, -0.34f, 0.70f}.normalized();
            sunCol = {1.22f, 0.68f, 0.44f};
            skyAmb = {0.42f, 0.28f, 0.48f};
            grdAmb = {0.20f, 0.14f, 0.26f};
            fogCol = {0.68f, 0.38f, 0.46f};
            clearBg = Color{118, 68, 96, 255};
            break;
        case 3: // Starlit Aether Night
            sunDir = Vec3{0.35f, -0.72f, 0.58f}.normalized();
            sunCol = {0.52f, 0.74f, 1.05f};
            skyAmb = {0.16f, 0.24f, 0.42f};
            grdAmb = {0.08f, 0.12f, 0.22f};
            fogCol = {0.10f, 0.16f, 0.30f};
            clearBg = Color{16, 26, 48, 255};
            break;
        default: // 0: Golden Dawn
            break;
    }

    pipeline.updateWorldUniforms(camPos, sunDir, sunCol, skyAmb, grdAmb, fogCol, 0.00048f);
    pipeline.setEmissiveStrength(0.0f);

    // Clear sky backdrop
    ClearBackground(clearBg);

    BeginMode3D(camera);

    // 0. Render Celestial Sky-Dome, Sun/Moon Disc & High Cirrus Layer
    renderSkyDomeAndSun(camPos, sunDir, animTime_, pipeline);

    // 1. Render Volumetric 3D Cloud Sea Layer
    renderCloudSea(camPos, animTime_, pipeline);

    // 2. Render Floating 3D Sky-Islands
    for (const auto& isl : world.archipelago.islands) {
        if ((isl.position - camPos).length() < 1850.0f) {
            renderIsland(isl, animTime_, pipeline);
        }
    }

    // 3. Render Player Airship
    if (!world.playerShip.isDestroyed) {
        renderAirship(
            world.playerShip,
            pipeline,
            drydockPreview,
            true,
            world.isPlayerWalkingOnDeck
        );
    }

    // 4. Render NPC Airships (Pirate Squadrons & Allied Wingmen)
    for (const auto& npc : world.npcShips) {
        if (!npc.isDestroyed && (npc.position - camPos).length() < 1500.0f) {
            renderAirship(npc, pipeline, nullptr, false, false);
        }
    }

    // 5. Render Colossal 3D Sky-Leviathans
    for (const auto& lev : world.leviathans) {
        if (!lev.isSlain && (lev.homeCenter - camPos).length() < 1750.0f) {
            renderLeviathan(lev, animTime_, pipeline);
        }
    }

    // 5B. Render Expanding Acoustic Aether-Sonar Wave (if active)
    if (world.sonarWaveRadius > 0.0f) {
        pipeline.setEmissiveStrength(1.5f);
        float alphaPct = std::clamp(1.0f - (world.sonarWaveRadius / 650.0f), 0.0f, 1.0f);
        uint8_t a = static_cast<uint8_t>(alphaPct * 220.0f);
        DrawSphereWires(
            {world.sonarWaveOrigin.x, world.sonarWaveOrigin.y, world.sonarWaveOrigin.z},
            world.sonarWaveRadius, 12, 18, Color{75, 235, 255, a}
        );
        pipeline.setEmissiveStrength(0.0f);
    }

    // 6. Render Physical Harpoon Tether Steel Cables (to Leviathans or Enemy Ships)
    for (const auto& t : world.tethers) {
        if (!t.active) continue;
        Vec3 pA = world.playerShip.localToWorldPos(t.sourceLocalOffset);
        Vec3 pB = pA;
        bool validTarget = false;

        if (t.targetLeviathanId >= 0) {
            for (const auto& lev : world.leviathans) {
                if (lev.id() == t.targetLeviathanId && !lev.isSlain && !lev.segments.empty()) {
                    int idx = std::clamp(t.targetSegmentIdx, 0, static_cast<int>(lev.segments.size()) - 1);
                    pB = lev.segments[static_cast<size_t>(idx)].position;
                    validTarget = true;
                    break;
                }
            }
        } else if (t.targetShipId >= 0) {
            for (const auto& npc : world.npcShips) {
                if (npc.id() == t.targetShipId && !npc.isDestroyed) {
                    pB = npc.localToWorldPos(t.targetShipLocalOffset);
                    validTarget = true;
                    break;
                }
            }
        }
        if (!validTarget) continue;

        float strainRatio = std::clamp(t.tensionForce / std::max(1.0f, t.maxBreakTension), 0.0f, 1.0f);
        Color cableCol = t.isShockTether
            ? Color{75, 240, 255, 255}
            : Color{
                static_cast<uint8_t>(185 + 70 * strainRatio),
                static_cast<uint8_t>(185 - 110 * strainRatio),
                static_cast<uint8_t>(160 - 120 * strainRatio),
                255
            };

        if (t.isShockTether) {
            pipeline.setEmissiveStrength(1.45f);
        }
        const int segs = 14;
        float sag = (1.0f - strainRatio) * 4.5f;
        Vec3 prevPt = pA;
        for (int s = 1; s <= segs; ++s) {
            float u = static_cast<float>(s) / static_cast<float>(segs);
            Vec3 pt = Vec3::lerp(pA, pB, u);
            pt.y -= std::sin(u * kPi) * sag;
            if (t.isShockTether && s < segs) {
                float jitter = std::sin(animTime_ * 34.0f + static_cast<float>(s * 3)) * 0.65f;
                pt.x += jitter;
                pt.y += std::cos(animTime_ * 28.0f + static_cast<float>(s * 5)) * 0.65f;
            }
            DrawCylinderEx(
                {prevPt.x, prevPt.y, prevPt.z},
                {pt.x, pt.y, pt.z},
                t.isShockTether ? 0.24f : 0.18f,
                t.isShockTether ? 0.24f : 0.18f,
                6, cableCol
            );
            prevPt = pt;
        }
        if (t.isShockTether) {
            pipeline.setEmissiveStrength(0.0f);
        }
    }

    // 7. Render Projectiles & Glowing Tracer Trails
    pipeline.setEmissiveStrength(1.35f);
    for (const auto& p : world.projectiles) {
        if (!p.active) continue;
        Color c = toRayColor(p.color);
        float pRad = (p.isHomingTorpedo) ? 0.92f : ((p.weaponType == BlockId::PlasmaLance || p.isIncendiary) ? 0.78f : (p.isHarpoon ? 0.62f : 0.50f));
        DrawSphere({p.position.x, p.position.y, p.position.z}, pRad, c);
        Vec3 tail = p.position - p.velocity * (p.isHomingTorpedo ? 0.085f : 0.055f);
        DrawCylinderEx(
            {tail.x, tail.y, tail.z},
            {p.position.x, p.position.y, p.position.z},
            p.isHomingTorpedo ? 0.28f : 0.14f,
            p.isHomingTorpedo ? 0.54f : 0.36f,
            6, c
        );
    }

    // 8. Render Falling 3D Detached Block Debris
    pipeline.setEmissiveStrength(0.0f);
    for (const auto& d : world.debris) {
        rlPushMatrix();
        rlTranslatef(d.worldPos.x, d.worldPos.y, d.worldPos.z);
        rlRotatef(d.rotationEuler.y * kRadToDeg, 0.0f, 1.0f, 0.0f);
        rlRotatef(d.rotationEuler.x * kRadToDeg, 1.0f, 0.0f, 0.0f);
        DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, kVoxelSize * 0.85f, toRayColor(d.color));
        rlPopMatrix();
    }

    // 9. Render Floating Salvage Crates
    pipeline.setEmissiveStrength(0.75f);
    for (const auto& c : world.crates) {
        if (c.collected) continue;
        rlPushMatrix();
        rlTranslatef(c.position.x, c.position.y, c.position.z);
        rlRotatef(animTime_ * 55.0f, 0.0f, 1.0f, 0.0f);
        DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 2.4f, Color{235, 185, 55, 255});
        DrawCubeWires({0.0f, 0.0f, 0.0f}, 2.6f, 2.6f, 2.6f, Color{85, 235, 255, 255});
        rlPopMatrix();
    }

    // 10. Render Explosion & Muzzle Flash Particles
    pipeline.setEmissiveStrength(1.5f);
    for (const auto& pt : particles_) {
        float t = std::clamp(pt.life / pt.maxLife, 0.0f, 1.0f);
        Color c = pt.color;
        c.a = static_cast<uint8_t>(t * 240.0f);
        DrawSphere({pt.position.x, pt.position.y, pt.position.z}, pt.size * t, c);
    }

    // 11. Render 3D Dynamic Weather Particles (Cirrus Mist, Ion-Aether Squall, Supercell Tempest)
    if (world.currentWeather != WeatherState::ClearTradeWinds) {
        Color wCol = (world.currentWeather == WeatherState::IonAetherSquall)
            ? Color{75, 240, 255, 185}
            : ((world.currentWeather == WeatherState::SupercellTempest)
                ? Color{255, 115, 75, 175}
                : Color{185, 215, 245, 160});
        for (int i = 0; i < 64; ++i) {
            float rx = std::fmod(static_cast<float>(i * 73) + animTime_ * 18.0f, 120.0f) - 60.0f;
            float ry = 40.0f - std::fmod(static_cast<float>(i * 41) + animTime_ * 68.0f, 80.0f);
            float rz = std::fmod(static_cast<float>(i * 97) + animTime_ * 14.0f, 120.0f) - 60.0f;
            Vector3 pTop{camPos.x + rx, camPos.y + ry, camPos.z + rz};
            Vector3 pBot{pTop.x - 1.4f, pTop.y - 3.6f, pTop.z - 0.8f};
            DrawLine3D(pTop, pBot, wCol);
        }
    }
    pipeline.setEmissiveStrength(0.0f);

    EndMode3D();
}

void WorldRenderer::renderSkyDomeAndSun(const Vec3& camPos, const Vec3& sunDir, float time, ShaderPipeline& pipeline) {
    // Blazing Celestial Sun Disc & Corona in direction of -sunDir
    Vec3 sunWorldPos = camPos - sunDir * 1150.0f;
    sunWorldPos.y = std::max(camPos.y + 280.0f, sunWorldPos.y);

    pipeline.setEmissiveStrength(1.8f);
    DrawSphere({sunWorldPos.x, sunWorldPos.y, sunWorldPos.z}, 58.0f, Color{255, 245, 195, 255});
    pipeline.setEmissiveStrength(1.1f);
    DrawSphere({sunWorldPos.x, sunWorldPos.y, sunWorldPos.z}, 92.0f, Color{255, 210, 125, 110});

    // High-altitude Cirrus Sky-Rings & Distant Horizon Cloud Banks
    pipeline.setEmissiveStrength(0.25f);
    for (int i = 0; i < 12; ++i) {
        float ang = static_cast<float>(i) * (kTwoPi / 12.0f) + time * 0.008f;
        float dist = 980.0f + (i % 3) * 140.0f;
        float cx = camPos.x + std::cos(ang) * dist;
        float cz = camPos.z + std::sin(ang) * dist;
        float cy = 265.0f + static_cast<float>((i * 19) % 45);
        rlPushMatrix();
        rlTranslatef(cx, cy, cz);
        rlRotatef(ang * kRadToDeg, 0.0f, 1.0f, 0.0f);
        rlScalef(165.0f, 6.0f, 42.0f);
        DrawModel(models_.sphere, {0.0f, 0.0f, 0.0f}, 1.0f, Color{235, 244, 255, 190});
        rlPopMatrix();
    }
    pipeline.setEmissiveStrength(0.0f);
}

void WorldRenderer::renderCloudSea(const Vec3& camPos, float time, ShaderPipeline& pipeline) {
    pipeline.setEmissiveStrength(0.06f);
    const float gridStep = 135.0f;
    float baseX = std::floor(camPos.x / gridStep) * gridStep;
    float baseZ = std::floor(camPos.z / gridStep) * gridStep;

    for (int gz = -6; gz <= 6; ++gz) {
        for (int gx = -6; gx <= 6; ++gx) {
            float wx = baseX + static_cast<float>(gx) * gridStep;
            float wz = baseZ + static_cast<float>(gz) * gridStep;
            float wave = std::sin(wx * 0.008f + time * 0.35f) * std::cos(wz * 0.008f + time * 0.25f);
            float cy = 34.0f + wave * 8.5f;

            rlPushMatrix();
            rlTranslatef(wx, cy, wz);
            rlScalef(92.0f, 13.5f, 92.0f);
            DrawModel(models_.sphere, {0.0f, 0.0f, 0.0f}, 1.0f, Color{224, 236, 250, 255});
            rlPopMatrix();
        }
    }
    pipeline.setEmissiveStrength(0.0f);
}

void WorldRenderer::renderIsland(const SkyIsland& isl, float time, ShaderPipeline& pipeline) {
    Color rockCol = toRayColor(isl.rockColor);
    Color darkRockCol = darkenColor(rockCol, 0.78f);
    Color topCol = toRayColor(isl.topColor);
    Color terraceCol = darkenColor(topCol, 0.86f);

    rlPushMatrix();
    rlTranslatef(isl.position.x, isl.position.y, isl.position.z);

    // 1. Primary Inverted Rocky Stalactite Underbelly Cone
    rlPushMatrix();
    rlTranslatef(0.0f, -isl.thickness, 0.0f);
    rlScalef(isl.radius * 0.92f, isl.thickness, isl.radius * 0.92f);
    DrawModel(models_.cone, {0.0f, 0.0f, 0.0f}, 1.0f, rockCol);
    rlPopMatrix();

    // 2. Procedural Perimeter Crag Buttresses & Hanging Stalactites (6 around rim)
    for (int k = 0; k < 6; ++k) {
        float ang = static_cast<float>(k) * (kTwoPi / 6.0f) + static_cast<float>(isl.id) * 0.75f;
        float rimDist = isl.radius * (0.68f + 0.14f * std::sin(static_cast<float>(k * 3 + isl.id)));
        float bx = std::cos(ang) * rimDist;
        float bz = std::sin(ang) * rimDist;
        float cragR = isl.radius * (0.28f + 0.06f * (k % 3));
        float cragH = isl.thickness * (0.55f + 0.15f * ((k + isl.id) % 3));

        // Hanging stalactite spire
        rlPushMatrix();
        rlTranslatef(bx, -cragH - 2.0f, bz);
        rlScalef(cragR, cragH, cragR);
        DrawModel(models_.cone, {0.0f, 0.0f, 0.0f}, 1.0f, darkRockCol);
        rlPopMatrix();

        // Stepped cliff terrace block
        rlPushMatrix();
        rlTranslatef(bx * 0.92f, -3.5f, bz * 0.92f);
        rlScalef(cragR * 1.15f, 7.5f, cragR * 1.15f);
        DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, terraceCol);
        rlPopMatrix();
    }

    // 3. Main Top Biome Plateau & Upper Stepped Terrace
    rlPushMatrix();
    rlTranslatef(0.0f, -4.5f, 0.0f);
    rlScalef(isl.radius, 9.0f, isl.radius);
    DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, topCol);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.0f, 1.5f, 0.0f);
    rlScalef(isl.radius * 0.64f, 6.0f, isl.radius * 0.64f);
    DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, terraceCol);
    rlPopMatrix();

    // 4. Central Mountain Crag Spire + Secondary Twin Peak
    rlPushMatrix();
    rlTranslatef(0.0f, 5.0f, 0.0f);
    rlScalef(isl.radius * 0.36f, isl.spireHeight, isl.radius * 0.36f);
    DrawModel(models_.cone, {0.0f, 0.0f, 0.0f}, 1.0f, rockCol);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(-isl.radius * 0.24f, 4.0f, isl.radius * 0.18f);
    rlScalef(isl.radius * 0.24f, isl.spireHeight * 0.68f, isl.radius * 0.24f);
    DrawModel(models_.cone, {0.0f, 0.0f, 0.0f}, 1.0f, darkRockCol);
    rlPopMatrix();

    // 4B. Biome-Specific Architectural Landmarks (Windmills or Ancient Aether Monolith Rings)
    if (isl.biome == SkyBiome::WindmillSteppe) {
        float wx = -isl.radius * 0.42f;
        float wz = -isl.radius * 0.32f;
        rlPushMatrix();
        rlTranslatef(wx, 5.0f, wz);
        rlPushMatrix();
        rlScalef(4.5f, 20.0f, 4.5f);
        DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, Color{215, 195, 165, 255});
        rlPopMatrix();
        // Rotating 4-vane windmill sails
        rlPushMatrix();
        rlTranslatef(0.0f, 17.0f, -4.8f);
        rlRotatef(time * 42.0f, 0.0f, 0.0f, 1.0f);
        rlPushMatrix();
        rlScalef(22.0f, 2.2f, 0.5f);
        DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, Color{240, 232, 210, 255});
        rlPopMatrix();
        rlPushMatrix();
        rlScalef(2.2f, 22.0f, 0.5f);
        DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, Color{240, 232, 210, 255});
        rlPopMatrix();
        rlPopMatrix();
        rlPopMatrix();
    } else if (isl.biome == SkyBiome::CrystalReef || isl.biome == SkyBiome::MaelstromEye) {
        pipeline.setEmissiveStrength(1.1f);
        rlPushMatrix();
        rlTranslatef(0.0f, isl.spireHeight + 14.0f, 0.0f);
        rlRotatef(time * 24.0f, 0.0f, 1.0f, 0.0f);
        rlRotatef(65.0f, 1.0f, 0.0f, 0.0f);
        rlScalef(14.0f, 14.0f, 14.0f);
        DrawModel(models_.torus, {0.0f, 0.0f, 0.0f}, 1.0f, Color{75, 235, 255, 255});
        rlPopMatrix();
        pipeline.setEmissiveStrength(0.0f);
    }

    // 5. Drydock Sky-Port Pier, Steel Gantry Cranes & Docking Beacons (on Northern -Z Rim)
    if (isl.hasDrydockPort) {
        float pierZ = -isl.radius - 16.0f;
        // Heavy Timber & Iron Pier Deck extending toward -Z
        rlPushMatrix();
        rlTranslatef(0.0f, 3.5f, pierZ);
        rlScalef(18.0f, 3.0f, 36.0f);
        DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, Color{155, 112, 68, 255});
        DrawCubeWires({0.0f, 0.0f, 0.0f}, 1.02f, 1.02f, 1.02f, Color{72, 52, 34, 255});
        rlPopMatrix();

        // Twin Gantry Crane Towers & Overhead Cross-Beam
        for (int side : {-1, +1}) {
            rlPushMatrix();
            rlTranslatef(static_cast<float>(side) * 10.5f, 16.0f, pierZ - 8.0f);
            rlScalef(2.4f, 24.0f, 2.4f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, Color{88, 102, 122, 255});
            rlPopMatrix();
        }
        rlPushMatrix();
        rlTranslatef(0.0f, 27.5f, pierZ - 8.0f);
        rlScalef(24.0f, 2.2f, 2.6f);
        DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, Color{185, 142, 65, 255});
        rlPopMatrix();

        // Port Docking Guidance Beacons
        pipeline.setEmissiveStrength(1.3f);
        DrawSphere({-10.5f, 29.5f, pierZ - 8.0f}, 2.0f, Color{65, 245, 175, 255});
        DrawSphere({+10.5f, 29.5f, pierZ - 8.0f}, 2.0f, Color{65, 245, 175, 255});
        pipeline.setEmissiveStrength(0.0f);
    }

    // 6. Lighthouse Tower & Rotating Sky-Beacon
    if (isl.hasLighthouse) {
        float lx = isl.radius * 0.52f;
        float lz = -isl.radius * 0.25f;
        rlPushMatrix();
        rlTranslatef(lx, 5.0f, lz);
        rlScalef(4.2f, 26.0f, 4.2f);
        DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, Color{230, 224, 212, 255});
        rlPopMatrix();

        // Red-and-brass lantern crown
        rlPushMatrix();
        rlTranslatef(lx, 31.0f, lz);
        rlScalef(5.0f, 3.0f, 5.0f);
        DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, Color{195, 65, 55, 255});
        rlPopMatrix();

        pipeline.setEmissiveStrength(1.5f);
        DrawSphere({lx, 35.0f, lz}, 3.4f, Color{255, 225, 115, 255});
        pipeline.setEmissiveStrength(0.0f);
    }

    rlPopMatrix();

    // 7. Harvestable Multi-Shard Aetherium Crystal Clusters (in world coords)
    pipeline.setEmissiveStrength(1.25f);
    for (const auto& cry : isl.crystals) {
        if (cry.harvested) continue;
        rlPushMatrix();
        rlTranslatef(cry.worldPos.x, cry.worldPos.y, cry.worldPos.z);
        rlRotatef(std::sin(time * 2.0f + cry.worldPos.x) * 8.0f, 0.0f, 1.0f, 0.0f);

        // Central tall crystal spire
        rlPushMatrix();
        rlScalef(2.2f, 9.2f, 2.2f);
        DrawModel(models_.cone, {0.0f, 0.0f, 0.0f}, 1.0f, Color{55, 225, 255, 255});
        rlPopMatrix();

        // Twin angled side crystal shards
        for (int s : {-1, +1}) {
            rlPushMatrix();
            rlRotatef(static_cast<float>(s) * 26.0f, 0.0f, 0.0f, 1.0f);
            rlScalef(1.5f, 6.0f, 1.5f);
            DrawModel(models_.cone, {0.0f, 0.0f, 0.0f}, 1.0f, Color{115, 245, 255, 255});
            rlPopMatrix();
        }
        rlPopMatrix();
    }
    pipeline.setEmissiveStrength(0.0f);

    // 8. Fortified Sky-Citadel Bastion Flak Towers & Liberation Beacon (in world coords)
    if (isl.isFortifiedStronghold) {
        for (const auto& tur : isl.bastionTurrets) {
            rlPushMatrix();
            rlTranslatef(tur.worldPos.x, tur.worldPos.y, tur.worldPos.z);

            if (tur.destroyed) {
                // Scorched ruined turret base
                rlPushMatrix();
                rlTranslatef(0.0f, -2.5f, 0.0f);
                rlScalef(6.2f, 4.5f, 6.2f);
                DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, Color{48, 44, 46, 255});
                rlPopMatrix();
            } else {
                // Armored Obsidian & Brass Bastion Tower Base
                rlPushMatrix();
                rlTranslatef(0.0f, -2.0f, 0.0f);
                rlScalef(6.8f, 9.5f, 6.8f);
                DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, Color{58, 52, 64, 255});
                rlPopMatrix();

                // Rotating Twin Heavy Flak Gun Cupola
                rlPushMatrix();
                rlTranslatef(0.0f, 5.5f, 0.0f);
                rlRotatef(tur.barrelYaw * kRadToDeg, 0.0f, 1.0f, 0.0f);
                DrawModel(models_.sphere, {0.0f, 0.0f, 0.0f}, 3.6f, Color{165, 118, 55, 255});

                for (int side : {-1, +1}) {
                    rlPushMatrix();
                    rlTranslatef(static_cast<float>(side) * 1.4f, 0.6f, -3.5f);
                    rlScalef(0.85f, 0.85f, 6.5f);
                    DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, Color{42, 46, 54, 255});
                    rlPopMatrix();
                }

                // Glowing Crimson Targeting Searchlight Eye
                pipeline.setEmissiveStrength(1.5f);
                DrawSphere({0.0f, 1.8f, -2.2f}, 1.15f, Color{255, 75, 55, 255});
                pipeline.setEmissiveStrength(0.0f);
                rlPopMatrix();
            }
            rlPopMatrix();
        }

        // If Liberated, render a towering Allied Sky-Beacon Pylon above the Citadel!
        if (isl.isLiberated) {
            pipeline.setEmissiveStrength(1.5f);
            DrawCylinderEx(
                {isl.position.x, isl.position.y + 8.0f, isl.position.z},
                {isl.position.x, isl.position.y + isl.spireHeight + 48.0f, isl.position.z},
                2.2f, 0.6f, 8, Color{65, 245, 185, 200}
            );
            DrawSphere(
                {isl.position.x, isl.position.y + isl.spireHeight + 48.0f, isl.position.z},
                5.5f, Color{85, 255, 195, 255}
            );
            pipeline.setEmissiveStrength(0.0f);
        }
    }
}

void WorldRenderer::renderSingleBlock3D(
    BlockId id,
    uint8_t orientation,
    float propSpinAngle,
    float rudderAngle,
    float turretYaw,
    float turretPitch,
    float healthRatio,
    ShaderPipeline& pipeline,
    int liveryId
) {
    const BlockSpec& spec = BlockRegistry::get(id);
    ColorRGB basePri = spec.primaryColor;
    ColorRGB baseAcc = spec.accentColor;

    // Apply Custom Fleet Hull Livery Paint Themes (1..5) to structural & envelope blocks
    if (liveryId > 0) {
        int liv = liveryId % 6;
        if (id == BlockId::WoodHull || id == BlockId::IronArmor || id == BlockId::HelmBridge) {
            if (liv == 1)      { basePri = {48, 96, 175};  baseAcc = {215, 232, 250}; } // Imperial Azure Guard
            else if (liv == 2) { basePri = {152, 36, 38};  baseAcc = {46, 42, 48};    } // Crimson Corsair Armada
            else if (liv == 3) { basePri = {218, 172, 62}; baseAcc = {250, 238, 195}; } // Solaris Gilded Sovereign
            else if (liv == 4) { basePri = {38, 36, 48};   baseAcc = {168, 95, 248};  } // Abyssal Void-Stalker
            else if (liv == 5) { basePri = {42, 128, 82};  baseAcc = {215, 168, 72};  } // Emerald Sky-Ranger
        } else if (id == BlockId::LiftBalloon) {
            if (liv == 1)      { basePri = {235, 244, 255}; baseAcc = {55, 125, 215}; }
            else if (liv == 2) { basePri = {52, 46, 54};    baseAcc = {205, 48, 48};  }
            else if (liv == 3) { basePri = {252, 235, 175}; baseAcc = {215, 155, 45}; }
            else if (liv == 4) { basePri = {54, 45, 78};    baseAcc = {185, 105, 255};}
            else if (liv == 5) { basePri = {215, 242, 222}; baseAcc = {48, 145, 92};  }
        }
    }

    float dmgDarken = 0.45f + 0.55f * std::clamp(healthRatio, 0.0f, 1.0f);
    Color priCol{
        static_cast<uint8_t>(basePri.r * dmgDarken),
        static_cast<uint8_t>(basePri.g * dmgDarken),
        static_cast<uint8_t>(basePri.b * dmgDarken),
        255
    };
    Color accCol{
        static_cast<uint8_t>(baseAcc.r * dmgDarken),
        static_cast<uint8_t>(baseAcc.g * dmgDarken),
        static_cast<uint8_t>(baseAcc.b * dmgDarken),
        255
    };

    pipeline.setEmissiveStrength(spec.isEmissive ? 0.95f : 0.0f);

    rlPushMatrix();
    rlRotatef(static_cast<float>(orientation) * -90.0f, 0.0f, 1.0f, 0.0f);

    switch (id) {
        case BlockId::WoodHull:
        case BlockId::IronArmor:
        case BlockId::OrichalcumArmor: {
            rlPushMatrix();
            rlScalef(kVoxelSize * 0.96f, kVoxelSize * 0.96f, kVoxelSize * 0.96f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            DrawCubeWires({0.0f, 0.0f, 0.0f}, 1.01f, 1.01f, 1.01f, accCol);
            rlPopMatrix();

            // Upper deck trim / riveted plate band
            if (id == BlockId::OrichalcumArmor) {
                pipeline.setEmissiveStrength(0.75f);
            }
            rlPushMatrix();
            rlTranslatef(0.0f, kVoxelSize * 0.42f, 0.0f);
            rlScalef(kVoxelSize * 0.88f, kVoxelSize * 0.14f, kVoxelSize * 0.88f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            break;
        }
        case BlockId::LiftBalloon: {
            // Pressurized gas bag envelope
            rlPushMatrix();
            rlScalef(kVoxelSize * 0.64f, kVoxelSize * 0.58f, kVoxelSize * 0.68f);
            DrawModel(models_.sphere, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();

            // Equatorial brass rigging harness & vertical rope band
            rlPushMatrix();
            rlScalef(kVoxelSize * 0.56f, kVoxelSize * 0.26f, kVoxelSize * 0.56f);
            DrawModel(models_.torus, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();

            rlPushMatrix();
            rlTranslatef(0.0f, -kVoxelSize * 0.35f, 0.0f);
            rlScalef(kVoxelSize * 0.24f, kVoxelSize * 0.35f, kVoxelSize * 0.24f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            break;
        }
        case BlockId::AetherCore: {
            // Outer brass reactor containment cage + glowing Aetherium crystal sphere
            DrawCubeWires({0.0f, 0.0f, 0.0f}, kVoxelSize * 0.94f, kVoxelSize * 0.94f, kVoxelSize * 0.94f, accCol);
            rlPushMatrix();
            rlTranslatef(0.0f, -kVoxelSize * 0.36f, 0.0f);
            rlScalef(kVoxelSize * 0.82f, kVoxelSize * 0.22f, kVoxelSize * 0.82f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();

            pipeline.setEmissiveStrength(1.45f);
            rlScalef(kVoxelSize * 0.46f, kVoxelSize * 0.46f, kVoxelSize * 0.46f);
            DrawModel(models_.sphere, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            break;
        }
        case BlockId::SteamPropeller: {
            // Brass Turbine Nacelle
            rlPushMatrix();
            rlScalef(kVoxelSize * 0.58f, kVoxelSize * 0.58f, kVoxelSize * 0.88f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            // Spinning 4-blade brass rotor
            rlPushMatrix();
            rlTranslatef(0.0f, 0.0f, -kVoxelSize * 0.54f);
            rlRotatef(propSpinAngle * kRadToDeg, 0.0f, 0.0f, 1.0f);
            rlPushMatrix();
            rlScalef(kVoxelSize * 1.42f, kVoxelSize * 0.18f, 0.14f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            rlPushMatrix();
            rlScalef(kVoxelSize * 0.18f, kVoxelSize * 1.42f, 0.14f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            rlPopMatrix();
            break;
        }
        case BlockId::AfterburnerJet: {
            // Armored Jet Nacelle
            rlPushMatrix();
            rlScalef(kVoxelSize * 0.68f, kVoxelSize * 0.68f, kVoxelSize * 0.94f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            // Glowing Aether-Ramjet Exhaust Cone
            pipeline.setEmissiveStrength(1.65f);
            rlPushMatrix();
            rlTranslatef(0.0f, 0.0f, -kVoxelSize * 0.62f);
            rlRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            rlScalef(0.65f, 1.85f, 0.65f);
            DrawModel(models_.cone, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            break;
        }
        case BlockId::GyroRudder: {
            // Gyro housing base
            rlPushMatrix();
            rlScalef(kVoxelSize * 0.65f, kVoxelSize * 0.65f, kVoxelSize * 0.65f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            // Articulated Canvas Sky-Sail / Rudder Vane
            rlPushMatrix();
            rlRotatef(rudderAngle * 28.0f, 0.0f, 1.0f, 0.0f);
            rlTranslatef(0.0f, kVoxelSize * 0.35f, -kVoxelSize * 0.35f);
            rlScalef(0.18f, kVoxelSize * 1.25f, kVoxelSize * 1.15f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, Color{238, 228, 206, 255});
            rlPopMatrix();
            break;
        }
        case BlockId::DeckCannon:
        case BlockId::FlakTurret:
        case BlockId::PlasmaLance: {
            // Armored gun carriage base
            rlPushMatrix();
            rlTranslatef(0.0f, -0.35f, 0.0f);
            rlScalef(kVoxelSize * 0.85f, kVoxelSize * 0.45f, kVoxelSize * 0.85f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();

            // Aim-tracking turret head & barrel
            if (id == BlockId::PlasmaLance) {
                pipeline.setEmissiveStrength(1.35f);
            }
            rlPushMatrix();
            rlRotatef(turretYaw * kRadToDeg - static_cast<float>(orientation) * -90.0f, 0.0f, 1.0f, 0.0f);
            rlRotatef(std::clamp(turretPitch * kRadToDeg, -25.0f, 45.0f), 1.0f, 0.0f, 0.0f);
            rlTranslatef(0.0f, 0.15f, -0.68f);
            rlScalef(0.46f, 0.46f, (id == BlockId::PlasmaLance) ? 2.15f : 1.72f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            break;
        }
        case BlockId::HarpoonWinch: {
            rlPushMatrix();
            rlTranslatef(0.0f, -0.3f, 0.0f);
            rlScalef(kVoxelSize * 0.82f, kVoxelSize * 0.52f, kVoxelSize * 0.82f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            // Harpoon launcher rail & spear tip
            rlPushMatrix();
            rlRotatef(turretYaw * kRadToDeg - static_cast<float>(orientation) * -90.0f, 0.0f, 1.0f, 0.0f);
            rlTranslatef(0.0f, 0.25f, -0.75f);
            rlScalef(0.34f, 0.34f, 2.05f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            break;
        }
        case BlockId::TeslaCoil: {
            rlPushMatrix();
            rlTranslatef(0.0f, -0.4f, 0.0f);
            rlScalef(0.62f, 1.55f, 0.62f);
            DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            pipeline.setEmissiveStrength(1.55f);
            DrawSphere({0.0f, 0.98f, 0.0f}, 0.54f, priCol);
            break;
        }
        case BlockId::ShieldProjector: {
            // Pedestal + rotating emitter ring + glowing shield core
            rlPushMatrix();
            rlTranslatef(0.0f, -0.45f, 0.0f);
            rlScalef(kVoxelSize * 0.78f, kVoxelSize * 0.35f, kVoxelSize * 0.78f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();

            pipeline.setEmissiveStrength(1.45f);
            rlPushMatrix();
            rlTranslatef(0.0f, 0.35f, 0.0f);
            rlRotatef(propSpinAngle * 25.0f, 0.0f, 1.0f, 0.0f);
            rlScalef(0.85f, 0.85f, 0.85f);
            DrawModel(models_.torus, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            DrawSphere({0.0f, 0.45f, 0.0f}, 0.48f, priCol);
            break;
        }
        case BlockId::RepairCrane: {
            rlPushMatrix();
            rlTranslatef(0.0f, -0.35f, 0.0f);
            rlScalef(kVoxelSize * 0.72f, kVoxelSize * 0.45f, kVoxelSize * 0.72f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            // Rotating jib arm
            rlPushMatrix();
            rlRotatef(propSpinAngle * 14.0f, 0.0f, 1.0f, 0.0f);
            rlTranslatef(0.0f, 0.35f, -0.35f);
            rlScalef(0.28f, 0.28f, 1.35f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            break;
        }
        case BlockId::HelmBridge: {
            rlPushMatrix();
            rlScalef(kVoxelSize * 0.95f, kVoxelSize * 0.95f, kVoxelSize * 0.95f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            pipeline.setEmissiveStrength(1.15f);
            rlPushMatrix();
            rlTranslatef(0.0f, 0.22f, -0.55f);
            rlScalef(kVoxelSize * 0.78f, 0.46f, 0.96f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            break;
        }
        case BlockId::TorpedoLauncher: {
            // Armored dual torpedo tube rack
            rlPushMatrix();
            rlTranslatef(0.0f, -0.35f, 0.0f);
            rlScalef(kVoxelSize * 0.88f, kVoxelSize * 0.45f, kVoxelSize * 0.88f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();

            rlPushMatrix();
            rlRotatef(turretYaw * kRadToDeg - static_cast<float>(orientation) * -90.0f, 0.0f, 1.0f, 0.0f);
            for (int side : {-1, +1}) {
                rlPushMatrix();
                rlTranslatef(static_cast<float>(side) * 0.36f, 0.18f, -0.45f);
                rlScalef(0.32f, 0.32f, 1.85f);
                DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
                rlPopMatrix();

                pipeline.setEmissiveStrength(1.5f);
                DrawSphere({static_cast<float>(side) * 0.36f, 0.18f, -1.42f}, 0.22f, accCol);
                pipeline.setEmissiveStrength(0.0f);
            }
            rlPopMatrix();
            break;
        }
        case BlockId::ShockHarpoon: {
            // Electrified Shock Harpoon Winch + Glowing Induction Coils
            rlPushMatrix();
            rlTranslatef(0.0f, -0.3f, 0.0f);
            rlScalef(kVoxelSize * 0.84f, kVoxelSize * 0.52f, kVoxelSize * 0.84f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();

            pipeline.setEmissiveStrength(1.45f);
            rlPushMatrix();
            rlRotatef(turretYaw * kRadToDeg - static_cast<float>(orientation) * -90.0f, 0.0f, 1.0f, 0.0f);
            rlTranslatef(0.0f, 0.25f, -0.78f);
            rlScalef(0.36f, 0.36f, 2.15f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            DrawSphere({0.0f, 0.55f, 0.0f}, 0.35f, priCol);
            break;
        }
        case BlockId::CapacitorBank: {
            // Heavy brass base + twin glowing Chrono-Aether glass cylinders
            rlPushMatrix();
            rlTranslatef(0.0f, -0.42f, 0.0f);
            rlScalef(kVoxelSize * 0.88f, kVoxelSize * 0.32f, kVoxelSize * 0.88f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();

            pipeline.setEmissiveStrength(1.45f);
            for (int side : {-1, +1}) {
                rlPushMatrix();
                rlTranslatef(static_cast<float>(side) * 0.36f, -0.25f, 0.0f);
                rlScalef(0.28f, 1.25f, 0.28f);
                DrawModel(models_.cylinder, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
                rlPopMatrix();
            }
            rlPushMatrix();
            rlTranslatef(0.0f, 0.45f, 0.0f);
            rlRotatef(propSpinAngle * 32.0f, 0.0f, 1.0f, 0.0f);
            rlScalef(0.76f, 0.45f, 0.76f);
            DrawModel(models_.torus, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();
            break;
        }
        case BlockId::FlameMortar: {
            // Heavy bronze siege howitzer carriage + wide elevated magma barrel
            rlPushMatrix();
            rlTranslatef(0.0f, -0.35f, 0.0f);
            rlScalef(kVoxelSize * 0.88f, kVoxelSize * 0.48f, kVoxelSize * 0.88f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, accCol);
            rlPopMatrix();

            pipeline.setEmissiveStrength(1.35f);
            rlPushMatrix();
            rlRotatef(turretYaw * kRadToDeg - static_cast<float>(orientation) * -90.0f, 0.0f, 1.0f, 0.0f);
            rlRotatef(26.0f, 1.0f, 0.0f, 0.0f);
            rlTranslatef(0.0f, 0.22f, -0.55f);
            rlScalef(0.62f, 0.62f, 1.55f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            rlPopMatrix();
            break;
        }
        default: {
            rlScalef(kVoxelSize * 0.9f, kVoxelSize * 0.9f, kVoxelSize * 0.9f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, priCol);
            break;
        }
    }

    rlPopMatrix();
    pipeline.setEmissiveStrength(0.0f);
}

void WorldRenderer::renderAirship(
    const Airship& ship,
    ShaderPipeline& pipeline,
    const DrydockGhostPreview* drydockPreview,
    bool isPlayerShip,
    bool showCaptainAvatar
) {
    rlPushMatrix();
    rlTranslatef(ship.position.x, ship.position.y, ship.position.z);

    // Apply Yaw -> Pitch -> Roll in degrees matching Quat::fromEuler(pitch, yaw, roll)
    rlRotatef(ship.yaw * kRadToDeg, 0.0f, 1.0f, 0.0f);
    rlRotatef(ship.pitch * kRadToDeg, 1.0f, 0.0f, 0.0f);
    rlRotatef(ship.roll * kRadToDeg, 0.0f, 0.0f, 1.0f);

    for (int gz = Airship::minZ(); gz <= Airship::maxZ(); ++gz) {
        for (int gy = Airship::minY(); gy <= Airship::maxY(); ++gy) {
            for (int gx = Airship::minX(); gx <= Airship::maxX(); ++gx) {
                const VoxelCell& cell = ship.getCell(gx, gy, gz);
                if (cell.id == BlockId::Empty) {
                    continue;
                }
                Vec3 localPos = ship.gridToLocalPos(gx, gy, gz);
                float hpRatio = (cell.maxHp > 0.0f) ? (cell.hp / cell.maxHp) : 1.0f;

                rlPushMatrix();
                rlTranslatef(localPos.x, localPos.y, localPos.z);
                renderSingleBlock3D(
                    cell.id,
                    cell.orientation,
                    ship.propellerSpinAngle,
                    ship.rudderInput,
                    cell.turretYaw,
                    cell.turretPitch,
                    hpRatio,
                    pipeline,
                    ship.hullLiveryId
                );

                // Render active deck fire plume if burning
                if (cell.fireTimer > 0.0f) {
                    pipeline.setEmissiveStrength(1.5f);
                    float flicker = 0.65f + 0.25f * std::sin(animTime_ * 18.0f + static_cast<float>(gx * 7 + gz));
                    DrawSphere({0.0f, 1.1f, 0.0f}, flicker, Color{255, 115, 35, 220});
                    pipeline.setEmissiveStrength(0.0f);
                }
                rlPopMatrix();
            }
        }
    }

    // 3D Hexagonal Energy Shield Dome Visualization (when shield projectors active or shield struck)
    if (ship.shieldHp > 0.0f && (ship.shieldProjectorCount > 0 || ship.shieldHitFlashTimer > 0.0f)) {
        pipeline.setEmissiveStrength(1.35f);
        float flashRatio = std::clamp(ship.shieldHitFlashTimer / 0.45f, 0.0f, 1.0f);
        uint8_t alpha = static_cast<uint8_t>(38.0f + flashRatio * 155.0f);
        DrawSphereWires(
            {0.0f, 1.0f, 0.0f},
            ship.boundingRadius * 0.92f,
            8, 12,
            Color{75, 235, 255, alpha}
        );
        pipeline.setEmissiveStrength(0.0f);
    }

    // Walk-on-Deck Captain Avatar (when player walks on deck)
    if (isPlayerShip && showCaptainAvatar) {
        rlPushMatrix();
        rlTranslatef(ship.captainLocalPos.x, ship.captainLocalPos.y, ship.captainLocalPos.z);
        pipeline.setEmissiveStrength(0.8f);
        DrawCylinder({0.0f, -0.8f, 0.0f}, 0.45f, 0.55f, 1.7f, 10, Color{45, 215, 255, 255});
        DrawSphere({0.0f, 1.15f, 0.0f}, 0.42f, Color{255, 225, 175, 255});
        pipeline.setEmissiveStrength(0.0f);
        rlPopMatrix();
    }

    // Drydock Shipyard Ghost Preview, Symmetry Plane & Engineering CoM/CoL Gizmos
    if (isPlayerShip && drydockPreview) {
        if (drydockPreview->showEngineeringGizmos) {
            pipeline.setEmissiveStrength(1.35f);
            // Holographic X=0 Symmetry Plane Line along keel
            DrawCylinderEx({0.0f, -2.5f, -18.0f}, {0.0f, -2.5f, 18.0f}, 0.08f, 0.08f, 6, Color{55, 215, 255, 180});

            // Center of Mass (Gold Sphere)
            Vec3 com = ship.centerOfMassLocal;
            DrawSphere({com.x, com.y, com.z}, 0.78f, Color{255, 195, 35, 255});
            // Center of Lift (Cyan Sphere + Upward Buoyancy Vector Arrow)
            Vec3 col = ship.centerOfLiftLocal;
            DrawSphere({col.x, col.y, col.z}, 0.78f, Color{45, 235, 255, 255});
            DrawCylinderEx(
                {col.x, col.y, col.z},
                {col.x, col.y + 4.8f, col.z},
                0.18f, 0.05f, 8, Color{45, 235, 255, 255}
            );
            pipeline.setEmissiveStrength(0.0f);
        }

        if (drydockPreview->active) {
            Vec3 gPos = ship.gridToLocalPos(drydockPreview->coord.x, drydockPreview->coord.y, drydockPreview->coord.z);
            DrawCubeWires({gPos.x, gPos.y, gPos.z}, kVoxelSize * 1.05f, kVoxelSize * 1.05f, kVoxelSize * 1.05f, Color{65, 255, 165, 255});

            if (drydockPreview->hasMirror && drydockPreview->mirrorCoord != drydockPreview->coord) {
                Vec3 mPos = ship.gridToLocalPos(drydockPreview->mirrorCoord.x, drydockPreview->mirrorCoord.y, drydockPreview->mirrorCoord.z);
                DrawCubeWires({mPos.x, mPos.y, mPos.z}, kVoxelSize * 1.05f, kVoxelSize * 1.05f, kVoxelSize * 1.05f, Color{65, 215, 255, 255});
            }
        }
    }

    rlPopMatrix();
}

void WorldRenderer::renderLeviathan(const Leviathan& lev, float time, ShaderPipeline& pipeline) {
    Color armorCol = toRayColor(lev.bodyColor);
    Color spineCol = darkenColor(armorCol, 0.72f);
    Color coreCol = toRayColor(lev.coreColor);

    for (size_t i = 0; i < lev.segments.size(); ++i) {
        const LeviathanSegment& seg = lev.segments[i];
        float yawDeg = std::atan2(-seg.forward.x, -seg.forward.z) * kRadToDeg;
        float pitchDeg = std::asin(std::clamp(seg.forward.y, -1.0f, 1.0f)) * kRadToDeg;

        rlPushMatrix();
        rlTranslatef(seg.position.x, seg.position.y, seg.position.z);
        rlRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
        rlRotatef(pitchDeg, 1.0f, 0.0f, 0.0f);

        // Glowing inner Aether Core if armor shattered or weak spot
        bool armorBroken = (seg.armorHp <= 0.0f);
        if (seg.hasWeakSpotCore || armorBroken) {
            pipeline.setEmissiveStrength(armorBroken ? 1.65f : 0.85f);
            rlPushMatrix();
            rlScalef(seg.radius * 0.82f, seg.radius * 0.82f, seg.radius * 0.82f);
            DrawModel(models_.sphere, {0.0f, 0.0f, 0.0f}, 1.0f, coreCol);
            rlPopMatrix();
            pipeline.setEmissiveStrength(0.0f);
        }

        // Outer Carapace Segment + Dorsal Crest Spine
        if (!armorBroken) {
            rlPushMatrix();
            rlScalef(seg.radius, seg.radius * 0.88f, seg.radius * 1.25f);
            DrawModel(models_.sphere, {0.0f, 0.0f, 0.0f}, 1.0f, armorCol);
            rlPopMatrix();

            // Dorsal ridge fin
            rlPushMatrix();
            rlTranslatef(0.0f, seg.radius * 0.72f, 0.0f);
            rlRotatef(-22.0f, 1.0f, 0.0f, 0.0f);
            rlScalef(seg.radius * 0.25f, seg.radius * 0.75f, seg.radius * 0.55f);
            DrawModel(models_.cone, {0.0f, 0.0f, 0.0f}, 1.0f, spineCol);
            rlPopMatrix();
        }

        // Bioluminescent energy ring on every 3rd segment
        if (i % 3 == 1) {
            pipeline.setEmissiveStrength(1.2f);
            rlPushMatrix();
            rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            rlScalef(seg.radius * 0.78f, seg.radius * 0.35f, seg.radius * 0.78f);
            DrawModel(models_.torus, {0.0f, 0.0f, 0.0f}, 1.0f, coreCol);
            rlPopMatrix();
            pipeline.setEmissiveStrength(0.0f);
        }

        // Articulated Head, Mandibles & Lower Jaw on Segment 0
        if (i == 0) {
            rlPushMatrix();
            rlTranslatef(0.0f, -seg.radius * 0.35f, -seg.radius * 0.55f);
            rlRotatef(-lev.jawAngle * kRadToDeg, 1.0f, 0.0f, 0.0f);
            rlScalef(seg.radius * 1.12f, seg.radius * 0.36f, seg.radius * 1.38f);
            DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, armorCol);
            rlPopMatrix();

            // Glowing Leviathan Eyes & Lateral Horns
            pipeline.setEmissiveStrength(1.55f);
            DrawSphere({-seg.radius * 0.65f, seg.radius * 0.25f, -seg.radius * 0.65f}, 0.95f, coreCol);
            DrawSphere({+seg.radius * 0.65f, seg.radius * 0.25f, -seg.radius * 0.65f}, 0.95f, coreCol);
            pipeline.setEmissiveStrength(0.0f);
        }

        // Sweeping Pectoral Wings / Tail Flukes
        if (seg.hasLateralFins) {
            float flapDeg = std::sin(time * 2.8f - static_cast<float>(i)) * 18.0f;
            for (int side : {-1, +1}) {
                rlPushMatrix();
                rlRotatef(static_cast<float>(side) * flapDeg, 0.0f, 0.0f, 1.0f);
                rlTranslatef(static_cast<float>(side) * seg.finSpan * 0.55f, 0.0f, 0.0f);
                rlScalef(seg.finSpan, seg.radius * 0.18f, seg.radius * 1.15f);
                DrawModel(models_.cube, {0.0f, 0.0f, 0.0f}, 1.0f, armorCol);
                rlPopMatrix();
            }
        }

        rlPopMatrix();
    }
}

} // namespace aethernaut
