#include "raylib.h"
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>

const int screenWidth = 1280;
const int screenHeight = 720;
const float enemySpeed = 2.0f;
const float attackCooldown = 1.0f;
const float attackDistance = 60.0f;
const float hitEffectDuration = 0.2f;
const float healthPackRespawnTime = 10.0f; // Интервал появления аптечки

struct Character {
    Vector2 position;
    Vector2 startPosition;
    int health;
    Color color;
    float lastAttackTime;
    float hitEffectTime;
};

struct HealthPack {
    Vector2 position;
    bool active;
    float lastSpawnTime;
};

// Ограничение движения
void KeepInsideScreen(Vector2& position, float size) {
    if (position.x < size) position.x = size;
    if (position.y < size) position.y = size;
    if (position.x > screenWidth - size) position.x = screenWidth - size;
    if (position.y > screenHeight - size) position.y = screenHeight - size;
}

void DrawHealthBar(Vector2 position, int health, int maxHealth, Color color) {
    float healthRatio = (float)health / maxHealth;
    DrawRectangle(position.x, position.y, 200, 20, DARKGRAY);
    DrawRectangle(position.x, position.y, (int)(200 * healthRatio), 20, color);
    DrawRectangleLines(position.x, position.y, 200, 20, BLACK);
}

float CalculateDistance(Vector2 a, Vector2 b) {
    return sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

void MoveTowards(Character& enemy, Vector2 targetPosition) {
    Vector2 direction = { targetPosition.x - enemy.position.x, targetPosition.y - enemy.position.y };
    float length = CalculateDistance(enemy.position, targetPosition);

    if (length > 0) {
        direction.x /= length;
        direction.y /= length;

        enemy.position.x += direction.x * enemySpeed;
        enemy.position.y += direction.y * enemySpeed;
    }

    KeepInsideScreen(enemy.position, 40);
}

bool CanAttack(float& lastAttackTime) {
    return (GetTime() - lastAttackTime) >= attackCooldown;
}

int GetRandomDamage(bool isMonster) {
    return isMonster ? (10 + rand() % 16) : (5 + rand() % 11);
}

HealthPack GenerateHealthPack() {
    HealthPack pack;
    pack.position = { (float)(100 + rand() % (screenWidth - 200)), (float)(100 + rand() % (screenHeight - 200)) };
    pack.active = true;
    pack.lastSpawnTime = GetTime();
    return pack;
}

int main() {
    InitWindow(screenWidth, screenHeight, "Monster Battle");

    srand(time(0));

    Character player = { {200, 300}, {200, 300}, 100, BLUE, 0.0f, 0.0f };
    Character monster = { {600, 300}, {600, 300}, 80, RED, 0.0f, 0.0f };

    HealthPack healthPack = GenerateHealthPack();

    Sound attackSound = LoadSound("attack.wav");

    bool gameOver = false;
    bool gameStarted = false;
    std::string resultMessage = "";

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (!gameStarted) {
            DrawText("Monster Battle", screenWidth / 2 - 100, 200, 40, BLACK);
            DrawText("Press ENTER to start", screenWidth / 2 - 100, 300, 30, DARKGRAY);
            DrawText("Press ESC to exit", screenWidth / 2 - 100, 350, 30, DARKGRAY);

            if (IsKeyPressed(KEY_ENTER)) gameStarted = true;
            if (IsKeyPressed(KEY_ESCAPE)) break;
        }
        else {
            if (!gameOver) {
                if (IsKeyDown(KEY_RIGHT)) player.position.x += 5;
                if (IsKeyDown(KEY_LEFT)) player.position.x -= 5;
                if (IsKeyDown(KEY_UP)) player.position.y -= 5;
                if (IsKeyDown(KEY_DOWN)) player.position.y += 5;

                KeepInsideScreen(player.position, 40);
                MoveTowards(monster, player.position);

                float distance = CalculateDistance(player.position, monster.position);

                if (IsKeyPressed(KEY_SPACE) && distance < attackDistance && CanAttack(player.lastAttackTime)) {
                    monster.health -= GetRandomDamage(false);
                    PlaySound(attackSound);
                    player.lastAttackTime = GetTime();
                    monster.hitEffectTime = GetTime();
                    DrawLine(player.position.x, player.position.y, monster.position.x, monster.position.y, DARKGRAY);
                }

                if (distance < 40 && CanAttack(monster.lastAttackTime)) {
                    player.health -= GetRandomDamage(true);
                    monster.lastAttackTime = GetTime();
                }

                // Проверка на сбор аптечки
                if (healthPack.active && CalculateDistance(player.position, healthPack.position) < 30) {
                    player.health += 20;
                    if (player.health > 100) player.health = 100;
                    healthPack.active = false;
                    healthPack.lastSpawnTime = GetTime();
                }

                // Проверка на появление новой аптечки
                if (!healthPack.active && (GetTime() - healthPack.lastSpawnTime) >= healthPackRespawnTime) {
                    healthPack = GenerateHealthPack();
                }

                if (player.health <= 0) {
                    gameOver = true;
                    resultMessage = "You lost!";
                }
                else if (monster.health <= 0) {
                    gameOver = true;
                    resultMessage = "You won!";
                }

                DrawCircleV(player.position, 40, player.color);
                DrawCircleV(monster.position, 40, monster.color);
                DrawHealthBar({ 20, 20 }, player.health, 100, BLUE);
                DrawHealthBar({ screenWidth - 220, 20 }, monster.health, 80, RED);
                DrawText("Press SPACE to attack", screenWidth / 2 - 100, screenHeight - 50, 20, BLACK);

                if (healthPack.active) {
                    DrawCircleV(healthPack.position, 20, GREEN);
                    DrawText("Health Pack", healthPack.position.x - 30, healthPack.position.y - 30, 20, DARKGREEN);
                }
            }
            else {
                DrawText(resultMessage.c_str(), screenWidth / 2 - 50, screenHeight / 2, 40, BLACK);
                DrawText("Press ENTER to restart", screenWidth / 2 - 100, screenHeight / 2 + 50, 20, GRAY);
                if (IsKeyPressed(KEY_ENTER)) {
                    player.health = 100;
                    monster.health = 80;
                    gameOver = false;
                    resultMessage = "";
                    player.lastAttackTime = 0.0f;
                    monster.lastAttackTime = 0.0f;

                    player.position = player.startPosition;
                    monster.position = monster.startPosition;

                    healthPack = GenerateHealthPack();
                }
            }
        }

        EndDrawing();
    }

    UnloadSound(attackSound);
    CloseWindow();

    return 0;
}
