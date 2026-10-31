#include "ui/HudRenderer.h"
#include "sim/ShipBlueprints.h"
#include "sim/Ballistics.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>

namespace aethernaut {

namespace {

Font gUiFont{};
bool gCustomFontLoaded = false;

bool isInFrontOfCamera(const Camera3D& cam, const Vec3& worldPos) {
    Vec3 camPos{cam.position.x, cam.position.y, cam.position.z};
    Vec3 camTarget{cam.target.x, cam.target.y, cam.target.z};
    Vec3 camFwd = (camTarget - camPos).normalized();
    return Vec3::dot(worldPos - camPos, camFwd) > 1.0f;
}

bool tryReserveLabelRect(std::vector<Rectangle>& occupied, Rectangle r) {
    for (const auto& ex : occupied) {
        if (CheckCollisionRecs(ex, r)) {
            return false;
        }
    }
    occupied.push_back(r);
    return true;
}

} // namespace

void HudRenderer::initFonts() {
    if (gCustomFontLoaded) return;
    const char* candidates[] = {
        "assets/fonts/ChakraPetch-Bold.ttf",
        "assets/fonts/ChakraPetch-Medium.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arialbd.ttf"
    };
    for (const char* path : candidates) {
        if (FileExists(path)) {
            gUiFont = LoadFontEx(path, 48, nullptr, 0);
            if (gUiFont.texture.id != 0) {
                SetTextureFilter(gUiFont.texture, TEXTURE_FILTER_BILINEAR);
                gCustomFontLoaded = true;
                break;
            }
        }
    }
    if (!gCustomFontLoaded) {
        gUiFont = GetFontDefault();
    }
}

void HudRenderer::shutdownFonts() {
    if (gCustomFontLoaded) {
        UnloadFont(gUiFont);
        gCustomFontLoaded = false;
    }
}

Font& HudRenderer::uiFont() {
    if (!gCustomFontLoaded && gUiFont.texture.id == 0) {
        gUiFont = GetFontDefault();
    }
    return gUiFont;
}

void HudRenderer::drawTextAA(const char* text, int posX, int posY, int fontSize, Color color, bool dropShadow) {
    if (!text || !text[0]) return;
    Font& f = uiFont();
    float sz = static_cast<float>(fontSize);
    float spacing = std::max(0.8f, sz * 0.045f);
    if (dropShadow && color.a > 40) {
        uint8_t sa = static_cast<uint8_t>(static_cast<float>(color.a) * 0.72f);
        DrawTextEx(f, text, {static_cast<float>(posX + 1), static_cast<float>(posY + 1)}, sz, spacing, Color{4, 8, 16, sa});
    }
    DrawTextEx(f, text, {static_cast<float>(posX), static_cast<float>(posY)}, sz, spacing, color);
}

int HudRenderer::measureTextAA(const char* text, int fontSize) {
    if (!text || !text[0]) return 0;
    Font& f = uiFont();
    float sz = static_cast<float>(fontSize);
    float spacing = std::max(0.8f, sz * 0.045f);
    Vector2 dim = MeasureTextEx(f, text, sz, spacing);
    return static_cast<int>(std::ceil(dim.x));
}

// Redirect local DrawText / MeasureText inside HudRenderer.cpp to crisp anti-aliased TrueType font!
#define DrawText(...) HudRenderer::drawTextAA(__VA_ARGS__)
#define MeasureText(...) HudRenderer::measureTextAA(__VA_ARGS__)

void HudRenderer::drawFlightHUD(
    World& world,
    const Camera3D& camera,
    int screenW,
    int screenH,
    bool canDockAtPort,
    const std::string& nearestPortName,
    bool& outRequestOpenDrydock
) {
    const Airship& ship = world.playerShip;
    char buf[200];

    // Track screen-space rectangles so 3D target tags & combat callouts NEVER overlap!
    std::vector<Rectangle> occupiedRects;
    // Reserve HUD panel zones so 3D world labels never bleed over 2D HUD panels
    occupiedRects.push_back({16.0f, 16.0f, 355.0f, 108.0f}); // Top-left card
    occupiedRects.push_back({static_cast<float>(screenW - 210), 16.0f, 194.0f, 200.0f}); // Top-right radar
    occupiedRects.push_back({16.0f, static_cast<float>(screenH - 216), 375.0f, 200.0f}); // Bottom-left vitals
    occupiedRects.push_back({static_cast<float>(screenW - 310), static_cast<float>(screenH - 140), 294.0f, 124.0f}); // Bottom-right tactical

    // Find closest active Elder Leviathan for the Top-Center Boss Encounter Bar
    const Leviathan* primaryBoss = nullptr;
    float primaryBossDist = 1e30f;
    for (const auto& lev : world.leviathans) {
        if (!lev.isSlain && !lev.segments.empty()) {
            float d = (lev.segments[0].position - ship.position).length();
            if (d < primaryBossDist) {
                primaryBossDist = d;
                primaryBoss = &lev;
            }
        }
    }

    if (primaryBoss && primaryBossDist < 680.0f) {
        occupiedRects.push_back({static_cast<float>(screenW / 2 - 300), 12.0f, 600.0f, 76.0f});
    }

    // -------------------------------------------------------------------------
    // 1. 3D Target Brackets & Ballistic Lead Aim Pips (Non-Overlapping!)
    // -------------------------------------------------------------------------
    for (const auto& lev : world.leviathans) {
        if (lev.isSlain || lev.segments.empty()) continue;
        Vec3 headPos = lev.segments[0].position;
        float dist = (headPos - ship.position).length();
        if (dist > 1250.0f || !isInFrontOfCamera(camera, headPos)) continue;

        Vector2 scrHead = GetWorldToScreen({headPos.x, headPos.y + 13.0f, headPos.z}, camera);
        if (scrHead.x >= 40 && scrHead.x <= screenW - 40 && scrHead.y >= 40 && scrHead.y <= screenH - 40) {
            std::snprintf(buf, sizeof(buf), "%s [%.0fm]", lev.name().c_str(), dist);
            int tw = MeasureText(buf, 14);
            Rectangle tagRec{scrHead.x - tw * 0.5f - 6.0f, scrHead.y - 30.0f, static_cast<float>(tw + 12), 28.0f};
            if (tryReserveLabelRect(occupiedRects, tagRec)) {
                DrawRectangleRec(tagRec, Color{8, 14, 24, 185});
                DrawText(buf, static_cast<int>(scrHead.x - tw / 2), static_cast<int>(scrHead.y - 26), 14, Color{255, 215, 95, 255});

                float hpPct = std::clamp(lev.totalHp / std::max(1.0f, lev.maxTotalHp), 0.0f, 1.0f);
                DrawRectangle(static_cast<int>(scrHead.x - 55), static_cast<int>(scrHead.y - 8), 110, 5, Color{15, 22, 34, 220});
                DrawRectangle(static_cast<int>(scrHead.x - 55), static_cast<int>(scrHead.y - 8), static_cast<int>(110 * hpPct), 5,
                              (lev.staggeredWindowTimer > 0.0f) ? Color{85, 255, 215, 255} : Color{245, 78, 95, 255});
            }

            // 3D Ballistic Lead Aim Pip (when within firing range)
            if (dist < 460.0f) {
                Vec3 leadWorld = computeBallisticLeadPoint(ship.position, ship.velocity, 185.0f, headPos, lev.velocity);
                if (isInFrontOfCamera(camera, leadWorld)) {
                    Vector2 scrLead = GetWorldToScreen({leadWorld.x, leadWorld.y, leadWorld.z}, camera);
                    DrawLineEx(scrHead, scrLead, 1.4f, Color{255, 205, 75, 145});
                    DrawCircleLines(static_cast<int>(scrLead.x), static_cast<int>(scrLead.y), 10.0f, Color{255, 220, 65, 230});
                    DrawCircleLines(static_cast<int>(scrLead.x), static_cast<int>(scrLead.y), 3.0f, Color{255, 95, 65, 255});
                }
            }
        }
    }

    int npcTagsDrawn = 0;
    for (const auto& npc : world.npcShips) {
        if (npc.isDestroyed || npcTagsDrawn >= 4) continue;
        float dist = (npc.position - ship.position).length();
        if (dist > 680.0f || !isInFrontOfCamera(camera, npc.position)) continue;

        Vector2 scrPos = GetWorldToScreen({npc.position.x, npc.position.y + 8.5f, npc.position.z}, camera);
        bool isConvoy = (npc.id() == world.convoyShipId);
        Color tagCol = isConvoy
            ? Color{255, 215, 75, 255}
            : ((npc.faction() == 1) ? Color{255, 105, 105, 255} : Color{95, 245, 175, 255});
        std::snprintf(buf, sizeof(buf), "%s [%.0fm]", npc.name().c_str(), dist);
        int tw = MeasureText(buf, 13);
        Rectangle tagRec{scrPos.x - tw * 0.5f - 4.0f, scrPos.y - 24.0f, static_cast<float>(tw + 8), 24.0f};
        if (tryReserveLabelRect(occupiedRects, tagRec)) {
            npcTagsDrawn++;
            DrawRectangleRec(tagRec, Color{8, 14, 24, 175});
            DrawText(buf, static_cast<int>(scrPos.x - tw / 2), static_cast<int>(scrPos.y - 22), 13, tagCol);

            float hpPct = std::clamp(npc.totalHp / std::max(1.0f, npc.totalMaxHp), 0.0f, 1.0f);
            DrawRectangle(static_cast<int>(scrPos.x - 42), static_cast<int>(scrPos.y - 6), 84, 4, Color{12, 18, 28, 210});
            DrawRectangle(static_cast<int>(scrPos.x - 42), static_cast<int>(scrPos.y - 6), static_cast<int>(84 * hpPct), 4, tagCol);
        }

        if (npc.faction() == 1 && dist < 400.0f) {
            Vec3 leadWorld = computeBallisticLeadPoint(ship.position, ship.velocity, 185.0f, npc.position, npc.velocity);
            if (isInFrontOfCamera(camera, leadWorld)) {
                Vector2 scrLead = GetWorldToScreen({leadWorld.x, leadWorld.y, leadWorld.z}, camera);
                DrawCircleLines(static_cast<int>(scrLead.x), static_cast<int>(scrLead.y), 8.5f, Color{255, 115, 85, 215});
            }
        }
    }

    // Deduplicated & Staggered 3D Floating Combat Callouts (Max 3 visible, zero overlap!)
    int calloutsShown = 0;
    for (int idx = static_cast<int>(world.callouts.size()) - 1; idx >= 0 && calloutsShown < 3; --idx) {
        const auto& co = world.callouts[static_cast<size_t>(idx)];
        if (!isInFrontOfCamera(camera, co.worldPos)) continue;
        Vector2 scrCo = GetWorldToScreen({co.worldPos.x, co.worldPos.y, co.worldPos.z}, camera);
        float alphaPct = std::clamp(co.lifetime / std::max(0.1f, co.maxLifetime), 0.0f, 1.0f);
        uint8_t a = static_cast<uint8_t>(alphaPct * 255.0f);
        int tw = MeasureText(co.text.c_str(), 14);
        Rectangle cRec{scrCo.x - tw * 0.5f - 6.0f, scrCo.y - 2.0f, static_cast<float>(tw + 12), 20.0f};
        for (int attempt = 0; attempt < 4; ++attempt) {
            if (tryReserveLabelRect(occupiedRects, cRec)) {
                DrawRectangleRec(cRec, Color{8, 14, 24, static_cast<uint8_t>(a * 0.78f)});
                DrawText(co.text.c_str(), static_cast<int>(cRec.x + 6.0f), static_cast<int>(cRec.y + 2.0f), 14,
                         Color{co.color.r, co.color.g, co.color.b, a});
                calloutsShown++;
                break;
            }
            cRec.y -= 23.0f;
        }
    }

    // -------------------------------------------------------------------------
    // 1A. TOP-CENTER ELDER LEVIATHAN BOSS BAR & TACTICAL COUNTERPLAY TELEGRAPH
    // -------------------------------------------------------------------------
    if (primaryBoss && primaryBossDist < 680.0f) {
        int bw = 560;
        int bh = 68;
        int bx = (screenW - bw) / 2;
        int by = 14;
        DrawRectangle(bx, by, bw, bh, Color{9, 15, 26, 232});
        Color borderCol = (primaryBoss->staggeredWindowTimer > 0.0f)
            ? Color{85, 255, 215, 245}
            : (primaryBoss->barrierActive ? Color{255, 185, 65, 235} : Color{225, 85, 95, 215});
        DrawRectangleLinesEx({static_cast<float>(bx), static_cast<float>(by), static_cast<float>(bw), static_cast<float>(bh)}, 1.8f, borderCol);

        std::snprintf(buf, sizeof(buf), "%s - %s [%.0fm]",
                      primaryBoss->name().c_str(), primaryBoss->title().c_str(), primaryBossDist);
        int titleW = MeasureText(buf, 15);
        DrawText(buf, bx + (bw - titleW) / 2, by + 6, 15, Color{255, 232, 145, 255});

        float hpPct = std::clamp(primaryBoss->totalHp / std::max(1.0f, primaryBoss->maxTotalHp), 0.0f, 1.0f);
        DrawRectangle(bx + 18, by + 26, bw - 36, 12, Color{20, 28, 44, 255});
        Color barCol = (primaryBoss->staggeredWindowTimer > 0.0f)
            ? Color{75, 245, 205, 255}
            : (primaryBoss->barrierActive ? Color{255, 165, 55, 255} : Color{235, 68, 85, 255});
        DrawRectangle(bx + 18, by + 26, static_cast<int>((bw - 36) * hpPct), 12, barCol);
        DrawRectangleLines(bx + 18, by + 26, bw - 36, 12, Color{255, 225, 145, 180});

        // Counterplay / Stagger State Line
        if (primaryBoss->staggeredWindowTimer > 0.0f) {
            std::snprintf(buf, sizeof(buf), "EXPOSED RESONANCE STAGGER (%.1fs) - 1.85X CRITICAL BROADSIDE DAMAGE!",
                          primaryBoss->staggeredWindowTimer);
            int sw = MeasureText(buf, 13);
            DrawText(buf, bx + (bw - sw) / 2, by + 44, 13, Color{95, 255, 215, 255});
        } else if (primaryBoss->barrierActive) {
            std::snprintf(buf, sizeof(buf), "AEGIS ACTIVE: %s - %s",
                          primaryBoss->signatureMechanicName().c_str(), primaryBoss->counterplayHint().c_str());
            int sw = MeasureText(buf, 12);
            DrawText(buf, bx + (bw - sw) / 2, by + 45, 12, Color{255, 205, 85, 255});
        } else {
            std::snprintf(buf, sizeof(buf), "HP: %.0f / %.0f  |  %s",
                          primaryBoss->totalHp, primaryBoss->maxTotalHp, primaryBoss->counterplayHint().c_str());
            int sw = MeasureText(buf, 12);
            DrawText(buf, bx + (bw - sw) / 2, by + 45, 12, Color{195, 220, 245, 255});
        }
    } else if (world.statusBannerTimer > 0.0f && !world.statusBannerText.empty()) {
        int tw = MeasureText(world.statusBannerText.c_str(), 14);
        int bw = std::min(screenW - 760, tw + 36);
        int bx = (screenW - bw) / 2;
        DrawRectangle(bx, 16, bw, 34, Color{10, 18, 32, 225});
        DrawRectangleLines(bx, 16, bw, 34, Color{85, 215, 255, 205});
        DrawText(world.statusBannerText.c_str(), bx + 16, 25, 14, Color{255, 232, 135, 255});
    }

    // -------------------------------------------------------------------------
    // 1B. Top-Left Expedition Charter, Active Hunt & Discovery Card
    // -------------------------------------------------------------------------
    DrawRectangle(16, 16, 348, 104, Color{10, 17, 28, 224});
    DrawRectangleLinesEx({16.0f, 16.0f, 348.0f, 104.0f}, 1.5f, Color{65, 175, 225, 175});

    if (primaryBoss) {
        std::snprintf(buf, sizeof(buf), "HUNT TARGET: %s [%.0fm]", primaryBoss->name().c_str(), primaryBossDist);
        DrawText(buf, 26, 23, 14, Color{255, 210, 85, 255});
    } else {
        DrawText("ALL ELDER LEVIATHANS CONQUERED! [N: Summon]", 26, 23, 13, Color{95, 245, 175, 255});
    }

    std::snprintf(buf, sizeof(buf), "Wonders [F2]: %d/10  |  Armada Logs: %d/12  |  Medals [O]: %d/10",
                  world.discoveredWonderCount(), world.recoveredLoreLogCount(), world.earnedMedalCount());
    DrawText(buf, 26, 44, 12, Color{125, 235, 255, 255});

    int activeAllies = 0;
    for (const auto& n : world.npcShips) {
        if (!n.isDestroyed && n.faction() == 0) activeAllies++;
    }
    std::snprintf(buf, sizeof(buf), "Wingmen: %d [Z: %s]  |  [T] %s",
                  activeAllies, World::getFleetOrderName(world.currentFleetOrder), World::getWeatherName(world.currentWeather));
    DrawText(buf, 26, 63, 12, Color{155, 240, 195, 255});

    DrawText("[ESC] Pause | [M] Map | [J] Market | [K] Crew | [F2] Wonders", 26, 84, 11, Color{245, 205, 95, 255});

    // -------------------------------------------------------------------------
    // 2. Crisp Tactical Aim Reticle
    // -------------------------------------------------------------------------
    Vector2 mouse = GetMousePosition();
    DrawCircleLines(static_cast<int>(mouse.x), static_cast<int>(mouse.y), 13.0f, Color{95, 235, 255, 210});
    DrawLine(static_cast<int>(mouse.x - 19), static_cast<int>(mouse.y), static_cast<int>(mouse.x - 7), static_cast<int>(mouse.y), Color{95, 235, 255, 210});
    DrawLine(static_cast<int>(mouse.x + 7), static_cast<int>(mouse.y), static_cast<int>(mouse.x + 19), static_cast<int>(mouse.y), Color{95, 235, 255, 210});
    DrawLine(static_cast<int>(mouse.x), static_cast<int>(mouse.y - 19), static_cast<int>(mouse.x), static_cast<int>(mouse.y - 7), Color{95, 235, 255, 210});
    DrawLine(static_cast<int>(mouse.x), static_cast<int>(mouse.y + 7), static_cast<int>(mouse.x), static_cast<int>(mouse.y + 19), Color{95, 235, 255, 210});

    // -------------------------------------------------------------------------
    // 3. Bottom-Left Airship Structural Voxel Schematic & Flight Vitals
    // -------------------------------------------------------------------------
    int panelX = 16;
    int panelY = screenH - 210;
    DrawRectangle(panelX, panelY, 372, 194, Color{10, 17, 28, 228});
    DrawRectangleLinesEx({static_cast<float>(panelX), static_cast<float>(panelY), 372.0f, 194.0f}, 1.5f, Color{65, 175, 225, 185});

    int dollCenterX = panelX + 64;
    int dollCenterY = panelY + 104;
    DrawRectangle(panelX + 10, panelY + 30, 108, 152, Color{6, 11, 20, 230});
    DrawRectangleLines(panelX + 10, panelY + 30, 108, 152, Color{45, 95, 135, 180});
    DrawText("HULL GRID", panelX + 28, panelY + 10, 12, Color{135, 205, 245, 255});

    for (int gz = Airship::minZ(); gz <= Airship::maxZ(); ++gz) {
        for (int gx = Airship::minX(); gx <= Airship::maxX(); ++gx) {
            float maxRatio = -1.0f;
            bool onFire = false;
            for (int gy = Airship::minY(); gy <= Airship::maxY(); ++gy) {
                const VoxelCell& c = ship.getCell(gx, gy, gz);
                if (c.id != BlockId::Empty) {
                    float r = (c.maxHp > 0.0f) ? (c.hp / c.maxHp) : 1.0f;
                    maxRatio = std::max(maxRatio, r);
                    if (c.fireTimer > 0.0f) onFire = true;
                }
            }
            if (maxRatio >= 0.0f) {
                Color cellCol = onFire
                    ? Color{255, 125, 35, 255}
                    : Color{
                        static_cast<uint8_t>((1.0f - maxRatio) * 245.0f),
                        static_cast<uint8_t>(maxRatio * 235.0f),
                        95, 255
                    };
                DrawRectangle(dollCenterX + gx * 6 - 2, dollCenterY + gz * 6 - 2, 5, 5, cellCol);
            }
        }
    }

    int vitX = panelX + 128;
    DrawText(ship.name().c_str(), vitX, panelY + 10, 16, Color{85, 235, 255, 255});

    float hullPct = std::clamp(ship.totalHp / std::max(1.0f, ship.totalMaxHp), 0.0f, 1.0f);
    std::snprintf(buf, sizeof(buf), "Hull Integrity: %.0f / %.0f (%.0f%%)", ship.totalHp, ship.totalMaxHp, hullPct * 100.0f);
    DrawText(buf, vitX, panelY + 32, 12, WHITE);
    DrawRectangle(vitX, panelY + 48, 228, 7, Color{22, 32, 48, 255});
    DrawRectangle(vitX, panelY + 48, static_cast<int>(228 * hullPct), 7,
                  (hullPct > 0.45f) ? Color{65, 225, 145, 255} : Color{245, 75, 85, 255});

    float shldPct = std::clamp(ship.shieldHp / std::max(1.0f, ship.maxShieldHp), 0.0f, 1.0f);
    std::snprintf(buf, sizeof(buf), "Aether Shield Dome: %.0f / %.0f", ship.shieldHp, ship.maxShieldHp);
    DrawText(buf, vitX, panelY + 60, 12, Color{95, 235, 255, 255});
    DrawRectangle(vitX, panelY + 76, 228, 6, Color{18, 28, 44, 255});
    DrawRectangle(vitX, panelY + 76, static_cast<int>(228 * shldPct), 6, Color{65, 215, 255, 255});

    float speedKnots = std::sqrt(ship.velocity.x * ship.velocity.x + ship.velocity.z * ship.velocity.z) * 1.94f;
    std::snprintf(buf, sizeof(buf), "Speed: %.1f kts | Alt: %.0fm | Lamp [L]: %s",
                  speedKnots, ship.position.y, world.playerSearchlightOn ? "ON" : "OFF");
    DrawText(buf, vitX, panelY + 88, 12, ship.isBoosting ? Color{255, 215, 75, 255} : Color{195, 225, 255, 255});

    std::snprintf(buf, sizeof(buf), "Treasury: %d Gold  |  %d Aetherium", world.playerGold, world.playerAether);
    DrawText(buf, vitX, panelY + 108, 13, Color{255, 215, 85, 255});

    std::snprintf(buf, sizeof(buf), "Leviathans Slain: %d  |  Pirates Sunk: %d", world.leviathansSlain, world.piratesSunk);
    DrawText(buf, vitX, panelY + 128, 12, Color{215, 185, 255, 255});

    if (!world.tethers.empty()) {
        float maxStrain = 0.0f;
        for (const auto& t : world.tethers) {
            maxStrain = std::max(maxStrain, t.tensionForce / std::max(1.0f, t.maxBreakTension));
        }
        std::snprintf(buf, sizeof(buf), "HARPOON CABLE STRAIN: %.0f%% ([R] Reel | [F] Cut)", maxStrain * 100.0f);
        DrawText(buf, vitX, panelY + 150, 12, Color{255, 185, 65, 255});
    } else {
        DrawText("LMB: Broadsides | RMB: Harpoon | R: Reel | L: Lamp", vitX, panelY + 150, 11, Color{145, 215, 245, 255});
    }

    // -------------------------------------------------------------------------
    // 3B. Bottom-Right Tactical Ability & Overdrive Bar
    // -------------------------------------------------------------------------
    int tacW = 296;
    int tacH = 124;
    int tacX = screenW - tacW - 16;
    int tacY = screenH - tacH - 16;
    DrawRectangle(tacX, tacY, tacW, tacH, Color{10, 17, 28, 228});
    DrawRectangleLinesEx({static_cast<float>(tacX), static_cast<float>(tacY), static_cast<float>(tacW), static_cast<float>(tacH)}, 1.5f, Color{65, 175, 225, 180});

    float boostPct = std::clamp(ship.afterburnerFuel / std::max(1.0f, ship.maxAfterburnerFuel), 0.0f, 1.0f);
    std::snprintf(buf, sizeof(buf), "[SHIFT] Overdrive Capacitor: %.0f%%", boostPct * 100.0f);
    DrawText(buf, tacX + 12, tacY + 8, 12, ship.isBoosting ? Color{255, 215, 75, 255} : Color{185, 225, 255, 255});
    DrawRectangle(tacX + 12, tacY + 24, tacW - 24, 6, Color{20, 32, 48, 255});
    DrawRectangle(tacX + 12, tacY + 24, static_cast<int>((tacW - 24) * boostPct), 6, Color{255, 185, 55, 255});

    if (world.damageControlCooldown <= 0.0f) {
        DrawText("[1/X] Damage Control & Repair: READY", tacX + 12, tacY + 36, 12, Color{85, 245, 165, 255});
    } else {
        std::snprintf(buf, sizeof(buf), "[1/X] Damage Control: %.0fs", world.damageControlCooldown);
        DrawText(buf, tacX + 12, tacY + 36, 12, Color{165, 185, 205, 255});
    }

    if (world.sonarPingCooldown <= 0.0f) {
        DrawText("[2/C] Acoustic Sonar (Shatters Shell): READY", tacX + 12, tacY + 54, 12, Color{95, 235, 255, 255});
    } else {
        std::snprintf(buf, sizeof(buf), "[2/C] Acoustic Sonar: %s (%.0fs)",
                      (world.sonarMarkTimer > 0.0f) ? "ACTIVE" : "Cooldown",
                      (world.sonarMarkTimer > 0.0f) ? world.sonarMarkTimer : world.sonarPingCooldown);
        DrawText(buf, tacX + 12, tacY + 54, 12, (world.sonarMarkTimer > 0.0f) ? Color{255, 215, 85, 255} : Color{165, 185, 205, 255});
    }

    if (world.skyMineCooldown <= 0.0f) {
        DrawText("[3/G] Magnetic Sky-Mine (Cracks Ice): READY", tacX + 12, tacY + 72, 12, Color{255, 175, 75, 255});
    } else {
        std::snprintf(buf, sizeof(buf), "[3/G] Magnetic Sky-Mine: %.1fs", world.skyMineCooldown);
        DrawText(buf, tacX + 12, tacY + 72, 12, Color{165, 185, 205, 255});
    }

    if (world.empNovaCooldown <= 0.0f) {
        DrawText("[4] Aether-EMP Nova (Grounds Static): READY", tacX + 12, tacY + 90, 12, Color{225, 135, 255, 255});
    } else {
        std::snprintf(buf, sizeof(buf), "[4] Aether-EMP Nova: %.0fs", world.empNovaCooldown);
        DrawText(buf, tacX + 12, tacY + 90, 12, Color{165, 185, 205, 255});
    }

    // -------------------------------------------------------------------------
    // 4. Top-Right 3D Sky-Radar & Compass Minimap
    // -------------------------------------------------------------------------
    int radRadius = 76;
    int radX = screenW - radRadius - 22;
    int radY = radRadius + 20;
    DrawCircle(radX, radY, static_cast<float>(radRadius), Color{10, 18, 30, 228});
    DrawCircleLines(radX, radY, static_cast<float>(radRadius), Color{65, 185, 235, 215});
    DrawCircleLines(radX, radY, static_cast<float>(radRadius) * 0.5f, Color{45, 115, 155, 120});

    const float radarRange = 1350.0f;
    for (const auto& isl : world.archipelago.islands) {
        Vec3 rel = isl.position - ship.position;
        float dx = (rel.x / radarRange) * static_cast<float>(radRadius);
        float dz = (rel.z / radarRange) * static_cast<float>(radRadius);
        if (dx * dx + dz * dz < static_cast<float>((radRadius - 6) * (radRadius - 6))) {
            Color ic = isl.hasDrydockPort ? Color{75, 245, 145, 220} : Color{135, 155, 175, 190};
            DrawCircle(radX + static_cast<int>(dx), radY + static_cast<int>(dz), isl.hasDrydockPort ? 4.5f : 3.0f, ic);
        }
    }
    for (const auto& w : world.skyWonders) {
        Vec3 rel = w.position - ship.position;
        float dx = (rel.x / radarRange) * static_cast<float>(radRadius);
        float dz = (rel.z / radarRange) * static_cast<float>(radRadius);
        if (dx * dx + dz * dz < static_cast<float>((radRadius - 5) * (radRadius - 5))) {
            DrawCircle(radX + static_cast<int>(dx), radY + static_cast<int>(dz), 2.8f, w.discovered ? Color{95, 255, 215, 230} : Color{255, 225, 95, 220});
        }
    }
    for (const auto& npc : world.npcShips) {
        if (npc.isDestroyed) continue;
        Vec3 rel = npc.position - ship.position;
        float dx = (rel.x / radarRange) * static_cast<float>(radRadius);
        float dz = (rel.z / radarRange) * static_cast<float>(radRadius);
        if (dx * dx + dz * dz < static_cast<float>((radRadius - 4) * (radRadius - 4))) {
            Color nc = (npc.faction() == 1) ? Color{255, 75, 75, 255} : Color{75, 225, 255, 255};
            DrawCircle(radX + static_cast<int>(dx), radY + static_cast<int>(dz), 3.0f, nc);
        }
    }
    for (const auto& lev : world.leviathans) {
        if (lev.isSlain || lev.segments.empty()) continue;
        Vec3 rel = lev.segments[0].position - ship.position;
        float dx = (rel.x / radarRange) * static_cast<float>(radRadius);
        float dz = (rel.z / radarRange) * static_cast<float>(radRadius);
        if (dx * dx + dz * dz < static_cast<float>((radRadius - 5) * (radRadius - 5))) {
            DrawCircle(radX + static_cast<int>(dx), radY + static_cast<int>(dz), 5.5f, Color{255, 195, 45, 255});
            DrawCircleLines(radX + static_cast<int>(dx), radY + static_cast<int>(dz), 7.5f, Color{255, 85, 85, 255});
        }
    }
    DrawCircle(radX, radY, 3.5f, WHITE);
    DrawText("N", radX - 4, radY - radRadius + 4, 12, Color{95, 225, 255, 255});
    DrawText("[B] Drydock | [H] Codex", radX - 68, radY + radRadius + 6, 12, Color{185, 215, 240, 255});

    // Drydock Port Prompt Button at Bottom-Center
    if (canDockAtPort || world.isSandboxMode) {
        std::snprintf(buf, sizeof(buf), "ENTER 3D DRYDOCK SHIPYARD [B]  |  MARKET [J] (%s)",
                      world.isSandboxMode ? "Sandbox" : nearestPortName.c_str());
        int dtw = MeasureText(buf, 13);
        int dw = dtw + 32;
        Rectangle dockBtn{static_cast<float>((screenW - dw) / 2), static_cast<float>(screenH - 50), static_cast<float>(dw), 34.0f};
        bool hover = CheckCollisionPointRec(mouse, dockBtn);
        DrawRectangleRec(dockBtn, hover ? Color{35, 145, 195, 245} : Color{18, 78, 122, 228});
        DrawRectangleLinesEx(dockBtn, 1.5f, Color{95, 235, 255, 255});
        DrawText(buf, static_cast<int>(dockBtn.x + 16), static_cast<int>(dockBtn.y + 9), 13, WHITE);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            outRequestOpenDrydock = true;
        }
    }
}

TitleAction HudRenderer::drawTitleScreen(
    World& backgroundWorld,
    int screenW,
    int screenH,
    int& inoutSelectedBlueprint
) {
    TitleAction action = TitleAction::None;
    Vector2 mouse = GetMousePosition();

    // Sleek Left Command Column (470px wide -> >70% of 1600x900 viewport is pure 3D Sky-Archipelago & Flagship Diorama!)
    int panelX = 36;
    int panelY = 30;
    int panelW = 478;
    int panelH = screenH - 60;

    DrawRectangle(panelX, panelY, panelW, panelH, Color{8, 14, 26, 222});
    DrawRectangleLinesEx({static_cast<float>(panelX), static_cast<float>(panelY), static_cast<float>(panelW), static_cast<float>(panelH)}, 2.0f, Color{75, 205, 255, 220});
    DrawLine(panelX + 22, panelY + 96, panelX + panelW - 22, panelY + 96, Color{235, 185, 75, 180});

    // Display Logotype with Gold-Cyan Accent Crest
    DrawText("AETHERNAUT", panelX + 24, panelY + 18, 46, Color{95, 238, 255, 255});
    DrawText("3D SKY-FLEET ARCHITECT & LEVIATHAN HUNTER", panelX + 26, panelY + 68, 14, Color{255, 208, 85, 255});

    // Flagship Blueprint & Risk Charter Selector Card
    int cardX = panelX + 22;
    int cardY = panelY + 108;
    int cardW = panelW - 44;
    DrawRectangle(cardX, cardY, cardW, 136, Color{14, 24, 42, 240});
    DrawRectangleLines(cardX, cardY, cardW, 136, Color{55, 155, 205, 195});

    ShipBlueprintInfo bp = ShipBlueprints::getInfo(inoutSelectedBlueprint);
    char statBuf[160];
    std::snprintf(statBuf, sizeof(statBuf), "FLAGSHIP CLASS (%d/%d):", inoutSelectedBlueprint + 1, ShipBlueprints::count());
    DrawText(statBuf, cardX + 14, cardY + 10, 12, Color{145, 205, 245, 255});
    DrawText(bp.name, cardX + 14, cardY + 28, 20, WHITE);
    DrawText(bp.role, cardX + 14, cardY + 52, 13, Color{255, 208, 85, 255});

    const Airship& s = backgroundWorld.playerShip;
    std::snprintf(statBuf, sizeof(statBuf), "Blocks: %d | Mass: %.0ft | Shield: %.0f | Guns: %d | Harpoons: %d",
                  s.occupiedBlockCount, s.totalMass, s.maxShieldHp, s.cannonCount, s.harpoonCount);
    DrawText(statBuf, cardX + 14, cardY + 74, 12, Color{95, 240, 185, 255});

    // Prev / Next Blueprint Buttons
    Rectangle prevBtn{static_cast<float>(cardX + cardW - 138), static_cast<float>(cardY + 10), 60.0f, 28.0f};
    Rectangle nextBtn{static_cast<float>(cardX + cardW - 70), static_cast<float>(cardY + 10), 60.0f, 28.0f};
    DrawRectangleRec(prevBtn, CheckCollisionPointRec(mouse, prevBtn) ? Color{45, 135, 185, 255} : Color{22, 54, 88, 255});
    DrawRectangleRec(nextBtn, CheckCollisionPointRec(mouse, nextBtn) ? Color{45, 135, 185, 255} : Color{22, 54, 88, 255});
    DrawRectangleLinesEx(prevBtn, 1.2f, Color{95, 205, 255, 200});
    DrawRectangleLinesEx(nextBtn, 1.2f, Color{95, 205, 255, 200});
    DrawText("< PREV", static_cast<int>(prevBtn.x + 8), static_cast<int>(prevBtn.y + 7), 12, WHITE);
    DrawText("NEXT >", static_cast<int>(nextBtn.x + 8), static_cast<int>(nextBtn.y + 7), 12, WHITE);

    if (CheckCollisionPointRec(mouse, prevBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        inoutSelectedBlueprint = (inoutSelectedBlueprint + ShipBlueprints::count() - 1) % ShipBlueprints::count();
        ShipBlueprints::applyBlueprint(backgroundWorld.playerShip, inoutSelectedBlueprint);
    }
    if (CheckCollisionPointRec(mouse, nextBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        inoutSelectedBlueprint = (inoutSelectedBlueprint + 1) % ShipBlueprints::count();
        ShipBlueprints::applyBlueprint(backgroundWorld.playerShip, inoutSelectedBlueprint);
    }

    // Risk/Reward Charter Selector Strip inside Card
    Rectangle charterBtn{static_cast<float>(cardX + 12), static_cast<float>(cardY + 98), static_cast<float>(cardW - 24), 28.0f};
    bool hovCharter = CheckCollisionPointRec(mouse, charterBtn);
    DrawRectangleRec(charterBtn, hovCharter ? Color{42, 78, 118, 255} : Color{22, 42, 68, 245});
    DrawRectangleLinesEx(charterBtn, 1.2f, Color{255, 205, 85, 210});
    std::snprintf(statBuf, sizeof(statBuf), "CHARTER: %s [Click to Cycle]", World::getRiskContractName(backgroundWorld.activeRiskContractIdx));
    DrawText(statBuf, static_cast<int>(charterBtn.x + 10), static_cast<int>(charterBtn.y + 7), 12, Color{255, 222, 115, 255});
    if (hovCharter && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        backgroundWorld.cycleRiskContract();
    }

    // Primary Game Mode & Shell Buttons
    struct MenuBtn {
        const char* label;
        const char* desc;
        TitleAction act;
        Color tint;
    };
    const MenuBtn buttons[] = {
        {"LAUNCH OPEN-SKY EXPEDITION", "Explore 14 sky-islands, discover 10 Wonders & hunt 6 Elder Leviathans.", TitleAction::StartCampaign, Color{24, 128, 178, 245}},
        {"DAILY SKY-VOYAGE CHALLENGE", "Seeded daily weather, flagship hull & risk charter for global comparison.", TitleAction::StartDailyVoyage, Color{178, 118, 32, 245}},
        {"3D DRYDOCK SHIPYARD & SANDBOX", "Unlimited resources! Engineer custom 3D voxel airships with 22 block types.", TitleAction::StartDrydockSandbox, Color{32, 138, 98, 245}},
        {"COLOSSAL LEVIATHAN HUNT", "Immediate boss encounter with harpoon winches & counterplay telegraphs.", TitleAction::StartLeviathanHunt, Color{162, 62, 52, 245}},
        {"SKY-ARMADA FLEET SKIRMISH", "Broadside naval combat alongside AI wingmen against a Corsair Squadron.", TitleAction::StartFleetSkirmish, Color{102, 68, 158, 245}}
    };

    int startY = cardY + 148;
    for (int i = 0; i < 5; ++i) {
        Rectangle r{static_cast<float>(cardX), static_cast<float>(startY + i * 58), static_cast<float>(cardW), 50.0f};
        bool hover = CheckCollisionPointRec(mouse, r);
        Color bg = buttons[i].tint;
        if (hover) {
            bg.r = static_cast<uint8_t>(std::min(255, bg.r + 35));
            bg.g = static_cast<uint8_t>(std::min(255, bg.g + 35));
            bg.b = static_cast<uint8_t>(std::min(255, bg.b + 35));
        }
        DrawRectangleRec(r, bg);
        DrawRectangleLinesEx(r, hover ? 2.2f : 1.4f, hover ? WHITE : Color{115, 215, 255, 190});
        DrawText(buttons[i].label, static_cast<int>(r.x + 14), static_cast<int>(r.y + 7), 15, WHITE);
        DrawText(buttons[i].desc, static_cast<int>(r.x + 14), static_cast<int>(r.y + 28), 11, Color{225, 238, 250, 255});

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            action = buttons[i].act;
        }
    }

    // Bottom Utility Row: Wonders & Logs Archive | Settings | Quit
    int utilY = startY + 5 * 58 + 8;
    float thirdW = (cardW - 16.0f) / 3.0f;
    Rectangle archBtn{static_cast<float>(cardX), static_cast<float>(utilY), thirdW, 38.0f};
    Rectangle setBtn{static_cast<float>(cardX) + thirdW + 8.0f, static_cast<float>(utilY), thirdW, 38.0f};
    Rectangle quitBtn{static_cast<float>(cardX) + (thirdW + 8.0f) * 2.0f, static_cast<float>(utilY), thirdW, 38.0f};

    bool hovArch = CheckCollisionPointRec(mouse, archBtn);
    bool hovSet  = CheckCollisionPointRec(mouse, setBtn);
    bool hovQuit = CheckCollisionPointRec(mouse, quitBtn);

    DrawRectangleRec(archBtn, hovArch ? Color{48, 108, 148, 255} : Color{24, 52, 78, 240});
    DrawRectangleRec(setBtn,  hovSet  ? Color{48, 108, 148, 255} : Color{24, 52, 78, 240});
    DrawRectangleRec(quitBtn, hovQuit ? Color{148, 48, 58, 255}  : Color{68, 28, 38, 240});
    DrawRectangleLinesEx(archBtn, 1.3f, Color{95, 215, 255, 210});
    DrawRectangleLinesEx(setBtn,  1.3f, Color{95, 215, 255, 210});
    DrawRectangleLinesEx(quitBtn, 1.3f, Color{255, 135, 135, 210});

    DrawText("WONDERS & LOGS", static_cast<int>(archBtn.x + 12), static_cast<int>(archBtn.y + 12), 12, WHITE);
    DrawText("SETTINGS [ESC]", static_cast<int>(setBtn.x + 18),  static_cast<int>(setBtn.y + 12),  12, WHITE);
    DrawText("QUIT DESKTOP",   static_cast<int>(quitBtn.x + 22), static_cast<int>(quitBtn.y + 12), 12, WHITE);

    if (hovArch && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) action = TitleAction::OpenWondersArchive;
    if (hovSet  && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) action = TitleAction::OpenSettings;
    if (hovQuit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) action = TitleAction::QuitGame;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        action = TitleAction::StartCampaign;
    }

    // Bottom-Right Live 3D Showcase Badge
    int badgeW = 370;
    int badgeX = screenW - badgeW - 28;
    int badgeY = screenH - 64;
    DrawRectangle(badgeX, badgeY, badgeW, 38, Color{8, 14, 26, 210});
    DrawRectangleLines(badgeX, badgeY, badgeW, 38, Color{75, 205, 255, 185});
    std::snprintf(statBuf, sizeof(statBuf), "LIVE 3D SHOWCASE: %s  |  [V] Time of Day", bp.name);
    DrawText(statBuf, badgeX + 14, badgeY + 12, 13, Color{95, 235, 255, 255});

    return action;
}

void HudRenderer::drawPauseModal(
    World& world,
    int screenW,
    int screenH,
    bool& inoutPaused,
    PauseAction& outAction
) {
    outAction = PauseAction::None;
    DrawRectangle(0, 0, screenW, screenH, Color{4, 8, 16, 210});

    int w = 520;
    int h = 540;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;
    DrawRectangle(mx, my, w, h, Color{10, 18, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{85, 215, 255, 235});

    DrawText("VOYAGE COMMAND PAUSED", mx + 128, my + 20, 22, Color{95, 238, 255, 255});
    char buf[160];
    std::snprintf(buf, sizeof(buf), "Charter: %s  |  Wonders: %d/10  |  Logs: %d/12",
                  World::getRiskContractName(world.activeRiskContractIdx),
                  world.discoveredWonderCount(), world.recoveredLoreLogCount());
    DrawText(buf, mx + 36, my + 52, 13, Color{255, 215, 85, 255});
    DrawLine(mx + 24, my + 76, mx + w - 24, my + 76, Color{55, 145, 195, 180});

    struct PItem {
        const char* label;
        PauseAction act;
    };
    const PItem items[8] = {
        {"RESUME SKY-VOYAGE [ESC]", PauseAction::Resume},
        {"ENTER 3D DRYDOCK SHIPYARD [B]", PauseAction::OpenDrydock},
        {"EXPLORER'S WONDERS & ARMADA LOGS [F2]", PauseAction::OpenWondersArchive},
        {"SETTINGS, AUDIO & ACCESSIBILITY", PauseAction::OpenSettings},
        {"ENGINEERING & HUNTER'S CODEX [H]", PauseAction::OpenCodex},
        {"SAVE CAMPAIGN PROGRESS [F6]", PauseAction::SaveCampaign},
        {"LOAD SAVED CAMPAIGN [F7]", PauseAction::LoadCampaign},
        {"RETURN TO TITLE SCREEN", PauseAction::ReturnToTitle}
    };

    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < 8; ++i) {
        Rectangle r{static_cast<float>(mx + 36), static_cast<float>(my + 92 + i * 52), static_cast<float>(w - 72), 42.0f};
        bool hov = CheckCollisionPointRec(mouse, r);
        DrawRectangleRec(r, hov ? Color{36, 118, 168, 255} : Color{18, 36, 62, 240});
        DrawRectangleLinesEx(r, 1.4f, hov ? WHITE : Color{75, 185, 235, 195});
        int tw = MeasureText(items[i].label, 15);
        DrawText(items[i].label, static_cast<int>(r.x + (r.width - tw) * 0.5f), static_cast<int>(r.y + 13), 15, WHITE);
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            outAction = items[i].act;
            if (outAction == PauseAction::Resume) {
                inoutPaused = false;
            }
        }
    }
}

void HudRenderer::drawSettingsModal(
    AppConfig& config,
    World& world,
    int screenW,
    int screenH,
    bool& inoutShowSettings
) {
    DrawRectangle(0, 0, screenW, screenH, Color{4, 8, 16, 215});
    int w = 680;
    int h = 520;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{10, 18, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{85, 215, 255, 235});

    DrawText("SETTINGS, GRAPHICS, AUDIO & ACCESSIBILITY [ESC to Close]", mx + 26, my + 20, 18, Color{95, 238, 255, 255});
    DrawLine(mx + 26, my + 50, mx + w - 26, my + 50, Color{55, 145, 195, 180});

    Vector2 mouse = GetMousePosition();
    char buf[160];

    auto drawSliderRow = [&](int rowY, const char* label, float& val, float minV, float maxV) {
        DrawText(label, mx + 36, rowY + 8, 15, WHITE);
        Rectangle bar{static_cast<float>(mx + 310), static_cast<float>(rowY + 6), 240.0f, 22.0f};
        float pct = std::clamp((val - minV) / (maxV - minV), 0.0f, 1.0f);
        DrawRectangleRec(bar, Color{18, 30, 48, 255});
        DrawRectangle(static_cast<int>(bar.x), static_cast<int>(bar.y), static_cast<int>(bar.width * pct), 22, Color{55, 185, 235, 255});
        DrawRectangleLinesEx(bar, 1.2f, Color{95, 215, 255, 220});
        if (CheckCollisionPointRec(mouse, bar) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float np = std::clamp((mouse.x - bar.x) / bar.width, 0.0f, 1.0f);
            val = minV + np * (maxV - minV);
        }
        std::snprintf(buf, sizeof(buf), "%.0f%%", pct * 100.0f);
        DrawText(buf, mx + 566, rowY + 8, 14, Color{255, 215, 85, 255});
    };

    drawSliderRow(my + 72,  "Master Audio Volume", config.masterVolume, 0.0f, 1.0f);
    drawSliderRow(my + 116, "Orchestral Music Volume", config.musicVolume, 0.0f, 1.0f);
    drawSliderRow(my + 160, "Combat & Harpoon SFX Volume", config.sfxVolume, 0.0f, 1.0f);
    drawSliderRow(my + 204, "3D Camera Field of View (FOV)", config.cameraFov, 45.0f, 75.0f);

    auto drawToggleRow = [&](int rowY, const char* label, bool& flag, const char* onTxt, const char* offTxt) {
        DrawText(label, mx + 36, rowY + 8, 15, WHITE);
        Rectangle btn{static_cast<float>(mx + 380), static_cast<float>(rowY + 2), 230.0f, 32.0f};
        bool hov = CheckCollisionPointRec(mouse, btn);
        DrawRectangleRec(btn, flag ? Color{28, 128, 92, 245} : Color{52, 38, 58, 245});
        DrawRectangleLinesEx(btn, 1.4f, hov ? WHITE : (flag ? Color{95, 245, 175, 255} : Color{185, 135, 165, 220}));
        const char* txt = flag ? onTxt : offTxt;
        int tw = MeasureText(txt, 13);
        DrawText(txt, static_cast<int>(btn.x + (btn.width - tw) * 0.5f), static_cast<int>(btn.y + 9), 13, WHITE);
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            flag = !flag;
        }
    };

    drawToggleRow(my + 256, "ACES Filmic HDR Bloom Shader", config.enableBloom, "ENABLED (HDR BLOOM)", "DISABLED");
    drawToggleRow(my + 302, "Reduce Storm Lightning Flashes", config.reduceFlashing, "REDUCED FLASH (SAFE)", "STANDARD LIGHTNING");
    drawToggleRow(my + 348, "Flagship Bow Aether-Searchlight [L]", world.playerSearchlightOn, "ENGAGED (ILLUMINATING)", "EXTINGUISHED");

    // Sky Atmosphere Selector Row
    DrawText("Sky-Archipelago Time of Day [V]", mx + 36, my + 402, 15, WHITE);
    const char* kAtmosNames[4] = {"Golden Dawn", "High Azure Noon", "Crimson Sundown", "Starlit Night"};
    Rectangle atmosBtn{static_cast<float>(mx + 380), static_cast<float>(my + 396), 230.0f, 32.0f};
    bool hovAtm = CheckCollisionPointRec(mouse, atmosBtn);
    DrawRectangleRec(atmosBtn, hovAtm ? Color{48, 118, 168, 255} : Color{24, 58, 92, 245});
    DrawRectangleLinesEx(atmosBtn, 1.4f, Color{95, 215, 255, 230});
    DrawText(kAtmosNames[world.skyAtmospherePreset % 4], static_cast<int>(atmosBtn.x + 24), static_cast<int>(atmosBtn.y + 9), 13, Color{255, 220, 95, 255});
    if (hovAtm && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        world.skyAtmospherePreset = (world.skyAtmospherePreset + 1) % 4;
    }

    Rectangle closeBtn{static_cast<float>(mx + w / 2 - 110), static_cast<float>(my + h - 54), 220.0f, 38.0f};
    bool hovClose = CheckCollisionPointRec(mouse, closeBtn);
    DrawRectangleRec(closeBtn, hovClose ? Color{45, 145, 195, 255} : Color{24, 78, 118, 245});
    DrawRectangleLinesEx(closeBtn, 1.5f, WHITE);
    DrawText("APPLY & CLOSE [ESC]", static_cast<int>(closeBtn.x + 34), static_cast<int>(closeBtn.y + 11), 14, WHITE);
    if ((hovClose && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowSettings = false;
    }
}

void HudRenderer::drawWondersAndLogsModal(
    World& world,
    int screenW,
    int screenH,
    bool& inoutShowWonders
) {
    DrawRectangle(0, 0, screenW, screenH, Color{4, 8, 16, 220});
    int w = 1040;
    int h = 620;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{10, 18, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{245, 205, 85, 235});

    char buf[220];
    std::snprintf(buf, sizeof(buf),
                  "EXPLORER'S ARCHIVE: 10 SKY-WONDERS (%d/10) & 12 FIRST ARMADA LOGS (%d/12) [F2 / ESC to Close]",
                  world.discoveredWonderCount(), world.recoveredLoreLogCount());
    DrawText(buf, mx + 24, my + 16, 17, Color{255, 220, 85, 255});
    DrawText("Tip: Fly within 165m, shine Bow Searchlight [L], or emit Acoustic Sonar [2/C] to chart Wonders & Logs!", mx + 24, my + 40, 12, Color{125, 235, 255, 255});
    DrawLine(mx + 24, my + 62, mx + w - 24, my + 62, Color{185, 145, 55, 180});

    // Left Column: 10 Monumental Sky-Wonders
    int leftX = mx + 24;
    int leftW = 480;
    DrawText("10 MONUMENTAL SKY-WONDERS (+180G +8Ae Each)", leftX, my + 72, 14, Color{95, 245, 195, 255});
    for (size_t i = 0; i < world.skyWonders.size() && i < 10; ++i) {
        const auto& sw = world.skyWonders[i];
        int ry = my + 96 + static_cast<int>(i) * 50;
        Rectangle card{static_cast<float>(leftX), static_cast<float>(ry), static_cast<float>(leftW), 45.0f};
        DrawRectangleRec(card, sw.discovered ? Color{18, 42, 36, 245} : Color{15, 24, 40, 240});
        DrawRectangleLinesEx(card, 1.1f, sw.discovered ? Color{85, 245, 175, 225} : Color{75, 115, 155, 165});

        std::snprintf(buf, sizeof(buf), "%zu. %s [%.0f, %.0f]", i + 1, sw.name.c_str(), sw.position.x, sw.position.z);
        DrawText(buf, static_cast<int>(card.x + 10), static_cast<int>(card.y + 6), 13, sw.discovered ? Color{255, 225, 95, 255} : Color{185, 205, 225, 255});
        DrawText(sw.discovered ? sw.loreText.c_str() : "Uncharted - Fly near coordinates or pulse Acoustic Sonar [2/C].",
                 static_cast<int>(card.x + 10), static_cast<int>(card.y + 25), 11, sw.discovered ? Color{195, 240, 220, 255} : Color{125, 145, 168, 255});
    }

    // Right Column: 12 First Armada Expedition Lore Logs
    int rightX = mx + 524;
    int rightW = 492;
    DrawText("12 FIRST SKY-ARMADA EXPEDITION LOGS (+90G +4Ae Each)", rightX, my + 72, 14, Color{125, 235, 255, 255});
    for (size_t i = 0; i < world.loreLogs.size() && i < 12; ++i) {
        const auto& lg = world.loreLogs[i];
        int ry = my + 96 + static_cast<int>(i) * 42;
        Rectangle card{static_cast<float>(rightX), static_cast<float>(ry), static_cast<float>(rightW), 38.0f};
        DrawRectangleRec(card, lg.recovered ? Color{18, 36, 52, 245} : Color{14, 22, 36, 240});
        DrawRectangleLinesEx(card, 1.0f, lg.recovered ? Color{95, 235, 255, 225} : Color{68, 98, 135, 160});

        std::snprintf(buf, sizeof(buf), "%s (%s)", lg.title.c_str(), lg.author.c_str());
        DrawText(buf, static_cast<int>(card.x + 10), static_cast<int>(card.y + 4), 12, lg.recovered ? Color{125, 240, 255, 255} : Color{165, 185, 205, 255});
        DrawText(lg.recovered ? lg.excerpt.c_str() : "Signal Beacon Active - Fly within 145m to recover log.",
                 static_cast<int>(card.x + 10), static_cast<int>(card.y + 20), 10, lg.recovered ? Color{225, 238, 250, 255} : Color{118, 138, 158, 255});
    }

    if (IsKeyPressed(KEY_F2) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowWonders = false;
    }
}

void HudRenderer::drawRunReportModal(
    World& world,
    int screenW,
    int screenH,
    RunReportAction& outAction
) {
    outAction = RunReportAction::None;
    DrawRectangle(0, 0, screenW, screenH, Color{4, 8, 16, 225});

    int w = 640;
    int h = 440;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;
    bool victory = world.victoryAchieved && !world.playerShip.isDestroyed;

    DrawRectangle(mx, my, w, h, Color{10, 18, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.2f,
                         victory ? Color{255, 215, 85, 245} : Color{255, 95, 95, 245});

    const char* hdr = victory
        ? "MAELSTROM SOVEREIGN CONQUERED - VOYAGE TRIUMPH!"
        : "FLAGSHIP STRUCTURAL COLLAPSE - SHIPWRECK REPORT";
    int hw = MeasureText(hdr, 20);
    DrawText(hdr, mx + (w - hw) / 2, my + 22, 20, victory ? Color{255, 225, 85, 255} : Color{255, 105, 95, 255});
    DrawLine(mx + 28, my + 56, mx + w - 28, my + 56, Color{75, 165, 215, 180});

    char buf[180];
    int mins = static_cast<int>(world.voyageElapsedSeconds) / 60;
    int secs = static_cast<int>(world.voyageElapsedSeconds) % 60;
    std::snprintf(buf, sizeof(buf), "Voyage Duration: %02d:%02d   |   Charter: %s",
                  mins, secs, World::getRiskContractName(world.activeRiskContractIdx));
    DrawText(buf, mx + 36, my + 74, 15, Color{135, 235, 255, 255});

    std::snprintf(buf, sizeof(buf), "Elder Leviathans Slain: %d / 6   |   Corsair Raiders Sunk: %d",
                  world.leviathansSlain, world.piratesSunk);
    DrawText(buf, mx + 36, my + 106, 15, WHITE);

    std::snprintf(buf, sizeof(buf), "Sky-Wonders Charted: %d / 10   |   First Armada Logs Recovered: %d / 12",
                  world.discoveredWonderCount(), world.recoveredLoreLogCount());
    DrawText(buf, mx + 36, my + 138, 15, Color{95, 245, 185, 255});

    std::snprintf(buf, sizeof(buf), "Citadels Liberated: %d   |   Ancient Obelisks Attuned: %d / 6",
                  world.citadelsLiberatedStat, world.obelisksAttunedStat);
    DrawText(buf, mx + 36, my + 170, 15, Color{255, 215, 95, 255});

    std::snprintf(buf, sizeof(buf), "Final Treasury: %d Gold   |   %d Refined Aetherium   |   Medals: %d / 10",
                  world.playerGold, world.playerAether, world.earnedMedalCount());
    DrawText(buf, mx + 36, my + 202, 15, Color{225, 175, 255, 255});

    Vector2 mouse = GetMousePosition();
    Rectangle btn1{static_cast<float>(mx + 36), static_cast<float>(my + 265), static_cast<float>(w - 72), 44.0f};
    Rectangle btn2{static_cast<float>(mx + 36), static_cast<float>(my + 320), static_cast<float>(w - 72), 44.0f};
    Rectangle btn3{static_cast<float>(mx + 36), static_cast<float>(my + 375), static_cast<float>(w - 72), 44.0f};

    bool h1 = CheckCollisionPointRec(mouse, btn1);
    bool h2 = CheckCollisionPointRec(mouse, btn2);
    bool h3 = CheckCollisionPointRec(mouse, btn3);

    DrawRectangleRec(btn1, h1 ? Color{42, 155, 115, 255} : Color{24, 105, 78, 245});
    DrawRectangleRec(btn2, h2 ? Color{42, 125, 175, 255} : Color{24, 78, 118, 245});
    DrawRectangleRec(btn3, h3 ? Color{145, 55, 65, 255}  : Color{88, 34, 42, 245});
    DrawRectangleLinesEx(btn1, 1.5f, WHITE);
    DrawRectangleLinesEx(btn2, 1.5f, Color{95, 215, 255, 220});
    DrawRectangleLinesEx(btn3, 1.5f, Color{255, 145, 145, 220});

    DrawText(victory ? "CONTINUE OPEN-SKY VOYAGE [R]" : "EMERGENCY SALVAGE REFLOAT AT HARBOR [R]",
             static_cast<int>(btn1.x + 110), static_cast<int>(btn1.y + 14), 15, WHITE);
    DrawText("LAUNCH FRESH SKY-VOYAGE EXPEDITION", static_cast<int>(btn2.x + 135), static_cast<int>(btn2.y + 14), 15, WHITE);
    DrawText("RETURN TO TITLE SCREEN", static_cast<int>(btn3.x + 190), static_cast<int>(btn3.y + 14), 15, WHITE);

    if ((h1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_R)) outAction = RunReportAction::EmergencyRefloat;
    if (h2 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) outAction = RunReportAction::StartNewVoyage;
    if (h3 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) outAction = RunReportAction::ReturnToTitle;
}

void HudRenderer::drawWorldMapModal(World& world, int screenW, int screenH, bool& inoutShowMap) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 215});
    int mapSize = std::min(screenW, screenH) - 100;
    int mapX = (screenW - mapSize) / 2;
    int mapY = (screenH - mapSize) / 2;

    DrawRectangle(mapX, mapY, mapSize, mapSize, Color{11, 20, 34, 248});
    DrawRectangleLinesEx({static_cast<float>(mapX), static_cast<float>(mapY), static_cast<float>(mapSize), static_cast<float>(mapSize)}, 2.0f, Color{75, 205, 255, 230});

    DrawText("SKY-ARCHIPELAGO NAVIGATION CHART & FAST-TRAVEL [M / ESC to Close]", mapX + 18, mapY + 12, 16, Color{85, 235, 255, 255});
    int ftCost = (world.isSandboxMode || world.unlockedRelics[2]) ? 0 : 2;
    char hdrBuf[160];
    std::snprintf(hdrBuf, sizeof(hdrBuf), "Click any GREEN Allied Port / Liberated Citadel to Fast-Travel (%d Ae) | Gold Star = Sky-Wonder", ftCost);
    DrawText(hdrBuf, mapX + 18, mapY + 32, 12, Color{185, 225, 245, 255});

    int cx = mapX + mapSize / 2;
    int cy = mapY + mapSize / 2 + 14;
    float scale = (static_cast<float>(mapSize) * 0.41f) / 1350.0f;
    Vector2 mouse = GetMousePosition();

    for (const auto& ring : world.archipelago.slipstreams) {
        int rx = cx + static_cast<int>(ring.position.x * scale);
        int ry = cy + static_cast<int>(ring.position.z * scale);
        DrawCircleLines(rx, ry, 4.0f, Color{75, 235, 255, 160});
    }
    for (const auto& w : world.skyWonders) {
        int wx = cx + static_cast<int>(w.position.x * scale);
        int wy = cy + static_cast<int>(w.position.z * scale);
        DrawCircle(wx, wy, 4.0f, w.discovered ? Color{95, 255, 195, 255} : Color{255, 225, 85, 230});
    }
    for (const auto& ob : world.archipelago.obelisks) {
        int ox = cx + static_cast<int>(ob.position.x * scale);
        int oy = cy + static_cast<int>(ob.position.z * scale);
        Color obCol = ob.attuned ? Color{85, 245, 255, 255} : Color{255, 205, 75, 255};
        DrawCircleLines(ox, oy, 6.0f, obCol);
        DrawCircle(ox, oy, 3.0f, obCol);
    }

    int hoveredIsland = -1;
    for (size_t i = 0; i < world.archipelago.islands.size(); ++i) {
        const auto& isl = world.archipelago.islands[i];
        int ix = cx + static_cast<int>(isl.position.x * scale);
        int iy = cy + static_cast<int>(isl.position.z * scale);
        float rad = std::max(7.0f, isl.radius * scale * 1.4f);

        bool canWarp = isl.hasDrydockPort || isl.isLiberated;
        Color c = canWarp
            ? Color{75, 245, 155, 255}
            : ((isl.isFortifiedStronghold && !isl.isLiberated) ? Color{255, 135, 55, 255} : Color{145, 175, 195, 255});

        float mouseDist = std::hypot(mouse.x - static_cast<float>(ix), mouse.y - static_cast<float>(iy));
        if (mouseDist <= rad + 8.0f) {
            hoveredIsland = static_cast<int>(i);
            DrawCircleLines(ix, iy, rad + 4.5f, WHITE);
        }

        DrawCircle(ix, iy, rad, c);
        DrawText(isl.name.c_str(), ix + static_cast<int>(rad) + 5, iy - 6, 12, canWarp ? Color{155, 255, 205, 255} : WHITE);
    }

    if (hoveredIsland >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (world.fastTravelToIsland(hoveredIsland)) {
            inoutShowMap = false;
            return;
        }
    }

    for (const auto& lev : world.leviathans) {
        if (lev.isSlain || lev.segments.empty()) continue;
        int lx = cx + static_cast<int>(lev.segments[0].position.x * scale);
        int ly = cy + static_cast<int>(lev.segments[0].position.z * scale);
        DrawCircle(lx, ly, 7.0f, Color{255, 95, 75, 255});
        DrawText(lev.name().c_str(), lx + 10, ly - 6, 12, Color{255, 205, 85, 255});
    }

    int px = cx + static_cast<int>(world.playerShip.position.x * scale);
    int py = cy + static_cast<int>(world.playerShip.position.z * scale);
    DrawCircle(px, py, 5.5f, WHITE);
    DrawText("YOU", px + 8, py - 6, 12, Color{85, 245, 255, 255});

    Rectangle summonBtn{static_cast<float>(mapX + 20), static_cast<float>(mapY + mapSize - 46), 235.0f, 34.0f};
    Rectangle arkBtn{static_cast<float>(mapX + 265), static_cast<float>(mapY + mapSize - 46), 235.0f, 34.0f};
    bool hovSum = CheckCollisionPointRec(mouse, summonBtn);
    bool hovArk = CheckCollisionPointRec(mouse, arkBtn);
    DrawRectangleRec(summonBtn, hovSum ? Color{185, 65, 55, 255} : Color{135, 45, 38, 240});
    DrawRectangleRec(arkBtn, hovArk ? Color{165, 55, 115, 255} : Color{115, 38, 82, 240});
    DrawRectangleLinesEx(summonBtn, 1.5f, Color{255, 185, 95, 255});
    DrawRectangleLinesEx(arkBtn, 1.5f, Color{255, 155, 225, 255});
    DrawText("SUMMON APEX LEVIATHAN [N]", static_cast<int>(summonBtn.x + 14), static_cast<int>(summonBtn.y + 10), 12, WHITE);
    DrawText("RAID CORSAIR DREAD-ARK [I]", static_cast<int>(arkBtn.x + 14), static_cast<int>(arkBtn.y + 10), 12, WHITE);
    if (hovSum && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        world.summonApexLeviathan();
        inoutShowMap = false;
    }
    if (hovArk && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        world.spawnCorsairDreadArk();
        inoutShowMap = false;
    }

    if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowMap = false;
    }
}

void HudRenderer::drawCrewModal(World& world, int screenW, int screenH, bool& inoutShowCrew) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 215});
    int w = 780;
    int h = 520;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{75, 205, 255, 230});

    DrawText("CAPTAIN'S QUARTERMASTER & SPECIALIST OFFICER ROSTER [K / ESC to Close]", mx + 24, my + 18, 17, Color{85, 235, 255, 255});
    char buf[160];
    std::snprintf(buf, sizeof(buf), "Treasury: %d Gold   |   %d Refined Aetherium Crystals", world.playerGold, world.playerAether);
    DrawText(buf, mx + 24, my + 44, 14, Color{255, 215, 85, 255});
    DrawLine(mx + 24, my + 68, mx + w - 24, my + 68, Color{55, 145, 195, 180});

    const auto& specs = World::getOfficerSpecs();
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < 5; ++i) {
        int cardY = my + 82 + i * 82;
        Rectangle card{static_cast<float>(mx + 24), static_cast<float>(cardY), static_cast<float>(w - 48), 72.0f};
        DrawRectangleRec(card, Color{16, 28, 46, 245});
        DrawRectangleLinesEx(card, 1.2f, Color{55, 145, 195, 190});

        int rank = world.officerRanks[static_cast<size_t>(i)];
        std::snprintf(buf, sizeof(buf), "%s  -  %s [RANK %d / 3]", specs[static_cast<size_t>(i)].name, specs[static_cast<size_t>(i)].role, rank);
        DrawText(buf, static_cast<int>(card.x + 16), static_cast<int>(card.y + 12), 15, Color{95, 240, 185, 255});
        DrawText(specs[static_cast<size_t>(i)].bonusDesc, static_cast<int>(card.x + 16), static_cast<int>(card.y + 38), 12, Color{205, 225, 245, 255});

        Rectangle upgBtn{card.x + card.width - 195.0f, card.y + 16.0f, 180.0f, 40.0f};
        bool hover = CheckCollisionPointRec(mouse, upgBtn);
        if (rank < 3) {
            int gCost = specs[static_cast<size_t>(i)].baseGoldCost * (rank + 1);
            int aCost = specs[static_cast<size_t>(i)].baseAetherCost * (rank + 1);
            bool canAfford = world.isSandboxMode || (world.playerGold >= gCost && world.playerAether >= aCost);
            DrawRectangleRec(upgBtn, hover && canAfford ? Color{45, 165, 115, 255} : (canAfford ? Color{28, 125, 85, 240} : Color{45, 55, 68, 220}));
            DrawRectangleLinesEx(upgBtn, 1.5f, canAfford ? Color{95, 245, 175, 255} : Color{110, 120, 135, 180});
            std::snprintf(buf, sizeof(buf), "PROMOTE (%dG + %dAe)", gCost, aCost);
            DrawText(buf, static_cast<int>(upgBtn.x + 14), static_cast<int>(upgBtn.y + 13), 12, WHITE);
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                world.upgradeOfficer(i);
            }
        } else {
            DrawRectangleRec(upgBtn, Color{35, 95, 145, 220});
            DrawRectangleLinesEx(upgBtn, 1.5f, Color{255, 215, 85, 255});
            DrawText("MAX RANK III", static_cast<int>(upgBtn.x + 42), static_cast<int>(upgBtn.y + 13), 13, Color{255, 215, 85, 255});
        }
    }

    if (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowCrew = false;
    }
}

void HudRenderer::drawCodexModal(int screenW, int screenH, bool& inoutShowCodex) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 215});
    int w = 920;
    int h = 580;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{75, 205, 255, 230});

    DrawText("AETHERNAUT ENGINEERING, BOSS COUNTERPLAY & FLIGHT CODEX [H / ESC to Close]", mx + 24, my + 18, 18, Color{85, 235, 255, 255});
    DrawLine(mx + 24, my + 48, mx + w - 24, my + 48, Color{55, 145, 195, 180});

    int col1 = mx + 26;
    DrawText("1. ELDER LEVIATHAN BOSS PUZZLE COUNTERPLAYS", col1, my + 64, 15, Color{255, 205, 75, 255});
    DrawText("- Cetus Aetheria (Azure Whale): Shatter Harmonic Song-Shell with Sonar [2/C] or Harpoon [RMB].", col1, my + 90, 12, WHITE);
    DrawText("- Draconis Fulmen (Storm-Wyrm): Ground Ion-Static Shroud with Harpoon Reel [R] or EMP [4].", col1, my + 112, 12, Color{95, 235, 255, 255});
    DrawText("- Raja Vorago (Dread-Manta): Burn away Abyssal Umbra Cloak with Bow Searchlight [L].", col1, my + 134, 12, Color{255, 225, 95, 255});
    DrawText("- Architeuthis Borealis (Cryo-Kraken): Crack Permafrost Carapace with Sky-Mine [3/G] or Geyser.", col1, my + 156, 12, Color{135, 245, 215, 255});
    DrawText("- Ignis Quetzal (Phoenix-Serpent): Snipe rear tail exhaust vents or trigger EMP Nova [4].", col1, my + 178, 12, Color{255, 165, 85, 255});
    DrawText("- Apex Charybdis (Maelstrom Sovereign): Combine Tether Reel [RMB+R] + Sonar [2/C] / EMP [4]!", col1, my + 200, 12, Color{255, 115, 155, 255});

    DrawText("2. 3D VOXEL BUOYANCY, SHIELDS & STRUCTURAL FRACTURE", col1, my + 236, 15, Color{255, 205, 75, 255});
    DrawText("- 22 Functional Block Types: Timber, Ironclad, Orichalcum, Dirigible Envelopes,", col1, my + 262, 12, WHITE);
    DrawText("  Shield Domes, Homing Torpedoes, Shock Harpoons, Flame Mortars & Railguns.", col1, my + 282, 12, WHITE);
    DrawText("- 3D BFS Connectivity Solver: Severed wings detach as falling 3D debris!", col1, my + 302, 12, Color{255, 165, 85, 255});

    DrawText("3. FLIGHT, TACTICAL & EXPLORATION KEYBINDS", col1, my + 340, 15, Color{85, 235, 255, 255});
    DrawText("[W/S] Throttle  |  [A/D] Rudder Yaw  |  [SPACE / CTRL] Altitude  |  [SHIFT] Overdrive", col1, my + 366, 12, WHITE);
    DrawText("[LMB] Broadside Cannons  |  [RMB] Launch Harpoon  |  [Hold R] Reel In  |  [F] Cut", col1, my + 388, 12, Color{255, 215, 95, 255});
    DrawText("[1/X] Damage Control  |  [2/C] Acoustic Sonar  |  [3/G] Sky-Mine  |  [4] EMP Nova", col1, my + 410, 12, Color{95, 245, 175, 255});
    DrawText("[L] Bow Searchlight  |  [F2] Wonders & Logs Archive  |  [J] Market  |  [K] Crew", col1, my + 432, 12, Color{135, 225, 255, 255});
    DrawText("[M] Navigation Map & Warp  |  [B] 3D Drydock Shipyard  |  [ESC] Pause Menu", col1, my + 454, 12, WHITE);

    if (IsKeyPressed(KEY_H) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowCodex = false;
    }
}

void HudRenderer::drawTradeAndContractsModal(
    World& world,
    int screenW,
    int screenH,
    bool& inoutShowLedger
) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 220});
    int w = 960;
    int h = 580;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{75, 205, 255, 230});

    DrawText("SKY-GUILD BOUNTY LEDGER & ISLAND COMMODITY EXCHANGE [J / ESC to Close]", mx + 24, my + 16, 18, Color{85, 235, 255, 255});

    char buf[180];
    std::snprintf(buf, sizeof(buf), "Treasury: %d Gold  |  %d Aetherium  |  Cargo Hold: %d / %d Crates  |  Charter: %s",
                  world.playerGold, world.playerAether, world.totalCargoUnits(), world.maxCargoCapacity(),
                  World::getRiskContractName(world.activeRiskContractIdx));
    DrawText(buf, mx + 24, my + 42, 13, Color{255, 215, 85, 255});
    DrawLine(mx + 24, my + 64, mx + w - 24, my + 64, Color{55, 145, 195, 180});

    Vector2 mouse = GetMousePosition();

    int nearestIdx = -1;
    float bestDist = 1e30f;
    for (size_t i = 0; i < world.archipelago.islands.size(); ++i) {
        float d = (world.archipelago.islands[i].position - world.playerShip.position).length();
        if (d < bestDist) {
            bestDist = d;
            nearestIdx = static_cast<int>(i);
        }
    }

    int leftX = mx + 24;
    int leftW = 445;
    DrawText("ISLAND COMMODITY EXCHANGE", leftX, my + 76, 15, Color{95, 240, 185, 255});

    if (nearestIdx >= 0) {
        const SkyIsland& isl = world.archipelago.islands[static_cast<size_t>(nearestIdx)];
        bool inRange = world.isSandboxMode || (bestDist <= isl.radius + 260.0f);
        std::snprintf(buf, sizeof(buf), "Nearest Port: %s (%.0fm) - %s",
                      isl.name.c_str(), bestDist, inRange ? "DOCKED / IN TRADE RANGE" : "FLY CLOSER TO BUY/SELL");
        DrawText(buf, leftX, my + 98, 12, inRange ? Color{85, 245, 165, 255} : Color{255, 145, 95, 255});

        const auto& cSpecs = Archipelago::getCommoditySpecs();
        for (int c = 0; c < kCommodityCount; ++c) {
            int cardY = my + 122 + c * 104;
            Rectangle card{static_cast<float>(leftX), static_cast<float>(cardY), static_cast<float>(leftW), 94.0f};
            DrawRectangleRec(card, Color{16, 28, 46, 245});
            DrawRectangleLinesEx(card, 1.2f, Color{55, 145, 195, 185});

            int localPrice = isl.marketPrices[static_cast<size_t>(c)];
            int basePrice = cSpecs[static_cast<size_t>(c)].basePrice;
            int owned = world.playerCargo[static_cast<size_t>(c)];

            const char* trendTag = (localPrice < basePrice - 8)
                ? "LOW PRICE (EXPORT SURPLUS)"
                : ((localPrice > basePrice + 8) ? "HIGH DEMAND (PREMIUM)" : "STANDARD MARKET RATE");
            Color trendCol = (localPrice < basePrice - 8)
                ? Color{85, 245, 155, 255}
                : ((localPrice > basePrice + 8) ? Color{255, 205, 75, 255} : Color{185, 205, 225, 255});

            DrawText(cSpecs[static_cast<size_t>(c)].name, static_cast<int>(card.x + 12), static_cast<int>(card.y + 10), 14, WHITE);
            DrawText(trendTag, static_cast<int>(card.x + 12), static_cast<int>(card.y + 30), 11, trendCol);
            DrawText(cSpecs[static_cast<size_t>(c)].unitDesc, static_cast<int>(card.x + 12), static_cast<int>(card.y + 48), 10, Color{170, 195, 215, 255});

            std::snprintf(buf, sizeof(buf), "Port Price: %dG (Base %dG)  |  In Cargo: %d", localPrice, basePrice, owned);
            DrawText(buf, static_cast<int>(card.x + 12), static_cast<int>(card.y + 68), 12, Color{255, 215, 95, 255});

            Rectangle buyBtn{card.x + card.width - 144.0f, card.y + 12.0f, 64.0f, 32.0f};
            Rectangle sellBtn{card.x + card.width - 72.0f, card.y + 12.0f, 64.0f, 32.0f};
            bool hovBuy = CheckCollisionPointRec(mouse, buyBtn);
            bool hovSell = CheckCollisionPointRec(mouse, sellBtn);

            DrawRectangleRec(buyBtn, (inRange && hovBuy) ? Color{45, 165, 115, 255} : Color{26, 95, 68, 235});
            DrawRectangleLinesEx(buyBtn, 1.2f, Color{95, 240, 175, 220});
            DrawText("BUY 1", static_cast<int>(buyBtn.x + 13), static_cast<int>(buyBtn.y + 10), 12, WHITE);

            DrawRectangleRec(sellBtn, (inRange && owned > 0 && hovSell) ? Color{185, 125, 45, 255} : Color{95, 65, 28, 235});
            DrawRectangleLinesEx(sellBtn, 1.2f, Color{255, 205, 95, 220});
            DrawText("SELL 1", static_cast<int>(sellBtn.x + 11), static_cast<int>(sellBtn.y + 10), 12, WHITE);

            if (hovBuy && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                world.buyCommodity(c, 1);
            }
            if (hovSell && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                world.sellCommodity(c, 1);
            }
        }
    }

    int rightX = mx + 490;
    int rightW = 445;
    DrawLine(rightX - 10, my + 72, rightX - 10, my + h - 20, Color{45, 95, 135, 160});
    DrawText("SKY-GUILD ELDER LEVIATHAN BOUNTIES (6 SPECIES)", rightX, my + 76, 15, Color{255, 205, 75, 255});

    int numContracts = static_cast<int>(world.contracts.size());
    for (int i = 0; i < numContracts; ++i) {
        const BountyContract& bc = world.contracts[static_cast<size_t>(i)];
        int cardY = my + 104 + i * 74;
        Rectangle card{static_cast<float>(rightX), static_cast<float>(cardY), static_cast<float>(rightW), 66.0f};
        DrawRectangleRec(card, bc.completed ? Color{18, 42, 34, 240} : Color{22, 28, 46, 245});
        DrawRectangleLinesEx(card, 1.2f, bc.completed ? Color{75, 235, 155, 220} : Color{225, 145, 75, 190});

        DrawText(bc.title.c_str(), static_cast<int>(card.x + 12), static_cast<int>(card.y + 8), 13, WHITE);
        std::snprintf(buf, sizeof(buf), "Target: %s", bc.targetName.c_str());
        DrawText(buf, static_cast<int>(card.x + 12), static_cast<int>(card.y + 27), 11, Color{185, 215, 240, 255});

        std::snprintf(buf, sizeof(buf), "Reward: +%d Gold, +%d Aetherium, +2 Ambergris", bc.rewardGold, bc.rewardAether);
        DrawText(buf, static_cast<int>(card.x + 12), static_cast<int>(card.y + 45), 11, Color{255, 215, 85, 255});

        const char* statusLbl = bc.completed ? "CLAIMED" : "ACTIVE";
        Color statusCol = bc.completed ? Color{85, 245, 165, 255} : Color{255, 165, 65, 255};
        DrawText(statusLbl, static_cast<int>(card.x + card.width - 78), static_cast<int>(card.y + 12), 12, statusCol);
    }

    if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowLedger = false;
    }
}

void HudRenderer::drawRelicVaultModal(
    World& world,
    int screenW,
    int screenH,
    bool& inoutShowRelics
) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 220});
    int w = 820;
    int h = 540;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{235, 185, 65, 230});

    DrawText("ANCIENT AETHER-RELIC VAULT & CITADEL LIBERATION [ESC to Close]", mx + 24, my + 18, 17, Color{255, 215, 85, 255});

    char buf[180];
    std::snprintf(buf, sizeof(buf), "Treasury: %d Gold  |  %d Aetherium  |  Awakened Relics: %d / 5  |  Citadels Liberated: %d",
                  world.playerGold, world.playerAether, world.unlockedRelicCount(), world.citadelsLiberatedStat);
    DrawText(buf, mx + 24, my + 44, 13, Color{95, 240, 185, 255});
    DrawLine(mx + 24, my + 68, mx + w - 24, my + 68, Color{185, 145, 55, 180});

    const auto& rSpecs = World::getRelicSpecs();
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < kRelicCount; ++i) {
        int cardY = my + 82 + i * 84;
        Rectangle card{static_cast<float>(mx + 24), static_cast<float>(cardY), static_cast<float>(w - 48), 74.0f};
        bool awakened = world.unlockedRelics[static_cast<size_t>(i)];

        DrawRectangleRec(card, awakened ? Color{22, 42, 36, 245} : Color{16, 26, 44, 245});
        DrawRectangleLinesEx(card, 1.3f, awakened ? Color{85, 245, 175, 230} : Color{195, 155, 65, 190});

        std::snprintf(buf, sizeof(buf), "%d. %s  (%s)",
                      i + 1, rSpecs[static_cast<size_t>(i)].name, rSpecs[static_cast<size_t>(i)].originLore);
        DrawText(buf, static_cast<int>(card.x + 16), static_cast<int>(card.y + 12), 15,
                 awakened ? Color{95, 255, 185, 255} : Color{255, 215, 95, 255});

        DrawText(rSpecs[static_cast<size_t>(i)].bonusDesc, static_cast<int>(card.x + 16), static_cast<int>(card.y + 38), 12, Color{205, 225, 245, 255});

        Rectangle btn{card.x + card.width - 205.0f, card.y + 17.0f, 190.0f, 40.0f};
        bool hover = CheckCollisionPointRec(mouse, btn);

        if (!awakened) {
            int gCost = rSpecs[static_cast<size_t>(i)].goldCost;
            int aCost = rSpecs[static_cast<size_t>(i)].aetherCost;
            bool canAfford = world.isSandboxMode || (world.playerGold >= gCost && world.playerAether >= aCost);
            DrawRectangleRec(btn, (hover && canAfford) ? Color{195, 135, 45, 255} : (canAfford ? Color{135, 92, 32, 240} : Color{45, 55, 68, 220}));
            DrawRectangleLinesEx(btn, 1.5f, canAfford ? Color{255, 215, 95, 255} : Color{110, 120, 135, 180});
            std::snprintf(buf, sizeof(buf), "AWAKEN (%dG + %dAe)", gCost, aCost);
            DrawText(buf, static_cast<int>(btn.x + 14), static_cast<int>(btn.y + 13), 12, WHITE);
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                world.unlockRelic(i);
            }
        } else {
            DrawRectangleRec(btn, Color{28, 105, 78, 230});
            DrawRectangleLinesEx(btn, 1.5f, Color{95, 255, 185, 255});
            DrawText("RELIC AWAKENED", static_cast<int>(btn.x + 34), static_cast<int>(btn.y + 13), 13, Color{95, 255, 185, 255});
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        inoutShowRelics = false;
    }
}

void HudRenderer::drawMedalsModal(
    World& world,
    int screenW,
    int screenH,
    bool& inoutShowMedals
) {
    DrawRectangle(0, 0, screenW, screenH, Color{6, 10, 18, 222});
    int w = 920;
    int h = 580;
    int mx = (screenW - w) / 2;
    int my = (screenH - h) / 2;

    DrawRectangle(mx, my, w, h, Color{11, 19, 32, 248});
    DrawRectangleLinesEx({static_cast<float>(mx), static_cast<float>(my), static_cast<float>(w), static_cast<float>(h)}, 2.0f, Color{245, 195, 65, 235});

    DrawText("SKY-ADMIRAL COMMENDATIONS & CAREER DOSSIER [O / ESC to Close]", mx + 24, my + 18, 17, Color{255, 220, 85, 255});

    char buf[200];
    int medalsEarned = world.earnedMedalCount();
    std::snprintf(buf, sizeof(buf),
                  "Earned Medals: %d / %d (+%d%% Global Fleet Damage, Shield & Repair Prestige Bonus)",
                  medalsEarned, kMedalCount, medalsEarned * 3);
    DrawText(buf, mx + 24, my + 44, 13, Color{95, 245, 185, 255});

    std::snprintf(buf, sizeof(buf),
                  "Stats: Leviathans:%d | Pirates:%d | Citadels:%d | Wonders:%d/10 | Logs:%d/12 | Obelisks:%d/6",
                  world.leviathansSlain, world.piratesSunk, world.citadelsLiberatedStat,
                  world.discoveredWonderCount(), world.recoveredLoreLogCount(), world.obelisksAttunedStat);
    DrawText(buf, mx + 24, my + 64, 12, Color{185, 215, 245, 255});
    DrawLine(mx + 24, my + 86, mx + w - 24, my + 86, Color{185, 145, 55, 180});

    const auto& mSpecs = World::getMedalSpecs();
    int colW = (w - 64) / 2;

    for (int i = 0; i < kMedalCount; ++i) {
        int col = i / 5;
        int row = i % 5;
        int cx = mx + 24 + col * (colW + 16);
        int cy = my + 100 + row * 90;

        Rectangle card{static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(colW), 80.0f};
        bool earned = world.earnedMedals[static_cast<size_t>(i)];

        DrawRectangleRec(card, earned ? Color{24, 44, 34, 245} : Color{16, 25, 42, 245});
        DrawRectangleLinesEx(card, 1.3f, earned ? Color{95, 250, 175, 235} : Color{115, 135, 165, 180});

        std::snprintf(buf, sizeof(buf), "%d. %s", i + 1, mSpecs[static_cast<size_t>(i)].title);
        DrawText(buf, static_cast<int>(card.x + 14), static_cast<int>(card.y + 10), 14,
                 earned ? Color{255, 225, 85, 255} : Color{215, 225, 240, 255});

        DrawText(mSpecs[static_cast<size_t>(i)].conditionDesc, static_cast<int>(card.x + 14), static_cast<int>(card.y + 32), 11, Color{185, 205, 230, 255});

        std::snprintf(buf, sizeof(buf), "Reward: +%dG, +%d Ae, +3%% Prestige",
                      mSpecs[static_cast<size_t>(i)].rewardGold, mSpecs[static_cast<size_t>(i)].rewardAether);
        DrawText(buf, static_cast<int>(card.x + 14), static_cast<int>(card.y + 54), 11, Color{95, 235, 255, 255});

        const char* badge = earned ? "AWARDED" : "LOCKED";
        Color badgeCol = earned ? Color{95, 255, 175, 255} : Color{145, 155, 175, 220};
        DrawText(badge, static_cast<int>(card.x + card.width - 78), static_cast<int>(card.y + 12), 11, badgeCol);
    }

    if (IsKeyPressed(KEY_O) || IsKeyPressed(KEY_ESCAPE)) {
        inoutShowMedals = false;
    }
}

#undef DrawText
#undef MeasureText

} // namespace aethernaut
