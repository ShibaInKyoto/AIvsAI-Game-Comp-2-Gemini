#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>

// ChronoStrider: Quantum Breach - Native Windows 10/11 C++ Engine
// High-performance 144Hz physics, dynamic time-dilation, and procedural audio.

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    Color color;
    float size;
};

struct Projectile {
    Vector2 pos;
    Vector2 vel;
    float radius;
    float damage;
    bool isEnemy;
    Color color;
    float life;
};

struct Enemy {
    Vector2 pos;
    Vector2 vel;
    float angle;
    float hp;
    float maxHp;
    float radius;
    Color color;
    int type; // 0: Drone, 1: Striker, 2: Shielder, 3: Boss
    float fireTimer;
};

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "ChronoStrider: Quantum Breach [Windows 10/11 Native]");
    InitAudioDevice();
    SetTargetFPS(144);

    // Player State
    Vector2 playerPos = { 640.0f, 360.0f };
    Vector2 playerVel = { 0.0f, 0.0f };
    float playerAngle = 0.0f;
    float playerHull = 100.0f;
    float playerShield = 60.0f;
    float chronoEnergy = 100.0f;
    bool isSlomo = false;
    int score = 0;
    int sector = 1;

    std::vector<Projectile> projectiles;
    std::vector<Enemy> enemies;
    std::vector<Particle> particles;

    Camera2D camera = { 0 };
    camera.target = playerPos;
    camera.offset = { 640.0f, 360.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    float spawnTimer = 0.0f;
    float fireTimer = 0.0f;

    while (!WindowShouldClose()) {
        float rawDt = GetFrameTime();
        if (rawDt > 0.05f) rawDt = 0.05f;

        // Time dilation (Right-Click or Q)
        isSlomo = (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsKeyDown(KEY_Q)) && chronoEnergy > 5.0f;
        float timeScale = isSlomo ? 0.22f : 1.0f;
        if (isSlomo) {
            chronoEnergy -= rawDt * 30.0f;
        } else {
            if (chronoEnergy < 100.0f) chronoEnergy += rawDt * 18.0f;
        }

        float simDt = rawDt * timeScale;

        // Player Aim toward mouse
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
        playerAngle = atan2f(mouseWorld.y - playerPos.y, mouseWorld.x - playerPos.x);

        // Player Newtonian movement
        Vector2 move = { 0 };
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) move.y -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) move.y += 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move.x -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1.0f;

        if (Vector2Length(move) > 0.0f) {
            move = Vector2Normalize(move);
            playerVel = Vector2Add(playerVel, Vector2Scale(move, 2200.0f * simDt));
        }

        // Friction & integrate
        playerVel = Vector2Scale(playerVel, powf(0.88f, simDt * 60.0f));
        playerPos = Vector2Add(playerPos, Vector2Scale(playerVel, simDt));

        // Fire weapons
        fireTimer -= rawDt;
        if ((IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE)) && fireTimer <= 0.0f) {
            fireTimer = 0.14f;
            Vector2 projVel = { cosf(playerAngle) * 1100.0f, sinf(playerAngle) * 1100.0f };
            projectiles.push_back({ playerPos, projVel, 5.0f, 35.0f, false, SKYBLUE, 1.4f });
        }

        // Spawn Enemies
        spawnTimer += simDt;
        if (spawnTimer >= 1.2f && enemies.size() < 30) {
            spawnTimer = 0.0f;
            float spawnAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
            Vector2 ePos = Vector2Add(playerPos, { cosf(spawnAngle) * 800.0f, sinf(spawnAngle) * 800.0f });
            enemies.push_back({ ePos, { 0, 0 }, 0.0f, 50.0f, 50.0f, 16.0f, RED, GetRandomValue(0, 2), 1.0f });
        }

        // Update Projectiles
        for (size_t i = 0; i < projectiles.size();) {
            projectiles[i].pos = Vector2Add(projectiles[i].pos, Vector2Scale(projectiles[i].vel, simDt));
            projectiles[i].life -= simDt;

            if (projectiles[i].life <= 0.0f) {
                projectiles.erase(projectiles.begin() + i);
            } else {
                // Collide with enemies
                bool hit = false;
                if (!projectiles[i].isEnemy) {
                    for (size_t e = 0; e < enemies.size(); ++e) {
                        if (CheckCollisionCircles(projectiles[i].pos, projectiles[i].radius, enemies[e].pos, enemies[e].radius)) {
                            enemies[e].hp -= projectiles[i].damage;
                            hit = true;
                            if (enemies[e].hp <= 0.0f) {
                                score += 150;
                                enemies.erase(enemies.begin() + e);
                            }
                            break;
                        }
                    }
                }
                if (hit) {
                    projectiles.erase(projectiles.begin() + i);
                } else {
                    ++i;
                }
            }
        }

        // Update Enemies
        for (auto& enemy : enemies) {
            Vector2 toPlayer = Vector2Subtract(playerPos, enemy.pos);
            Vector2 dir = Vector2Normalize(toPlayer);
            enemy.vel = Vector2Scale(dir, 280.0f);
            enemy.pos = Vector2Add(enemy.pos, Vector2Scale(enemy.vel, simDt));
            enemy.angle = atan2f(dir.y, dir.x);
        }

        // Camera follow
        camera.target = playerPos;

        // Render Frame
        BeginDrawing();
        ClearBackground(isSlomo ? Color{ 5, 11, 20, 255 } : Color{ 2, 6, 23, 255 });

        BeginMode2D(camera);

        // Draw Grid
        int gridSize = 100;
        for (int x = -2000; x <= 2000; x += gridSize) {
            DrawLine(x, -2000, x, 2000, Color{ 6, 182, 212, 25 });
        }
        for (int y = -2000; y <= 2000; y += gridSize) {
            DrawLine(-2000, y, 2000, y, Color{ 6, 182, 212, 25 });
        }

        // Draw Projectiles
        for (const auto& p : projectiles) {
            DrawCircleV(p.pos, p.radius, p.color);
        }

        // Draw Enemies
        for (const auto& e : enemies) {
            DrawPoly(e.pos, 3, e.radius, e.angle * RAD2DEG, e.color);
        }

        // Draw Player Ship
        DrawPoly(playerPos, 3, 22.0f, playerAngle * RAD2DEG, CYAN);
        if (playerShield > 0) {
            DrawCircleLines((int)playerPos.x, (int)playerPos.y, 28.0f, Color{ 56, 189, 248, 160 });
        }

        EndMode2D();

        // Draw Native Cockpit HUD
        DrawRectangle(20, 20, 320, 110, Color{ 15, 23, 42, 210 });
        DrawRectangleLines(20, 20, 320, 110, Color{ 6, 182, 212, 100 });

        DrawText(TextFormat("SCORE: %d", score), 35, 30, 20, CYAN);
        DrawText(TextFormat("HULL: %.0f / 100", playerHull), 35, 58, 14, RED);
        DrawText(TextFormat("SHIELD: %.0f / 60", playerShield), 35, 78, 14, SKYBLUE);
        DrawText(TextFormat("CHRONO SLOMO: %.0f%% [R-CLICK / Q]", chronoEnergy), 35, 98, 14, PURPLE);

        if (isSlomo) {
            DrawText("CHRONO-DRIVE DILATION ACTIVE", GetScreenWidth() / 2 - 160, 40, 20, PURPLE);
        }

        DrawFPS(GetScreenWidth() - 100, 20);

        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
