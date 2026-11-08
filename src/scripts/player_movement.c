#include "globals.h"

#define PLAYER_SPEED 270.0f

void movePlayer(GameObject* player) {
    if (player->type != PLAYER) return;
    float dt = game.dt;

    // Movement (WASD)
    Vector2 input = { 0 };
    if (IsKeyDown(KEY_W)) input.y -= 1.0f;
    if (IsKeyDown(KEY_S)) input.y += 1.0f;
    if (IsKeyDown(KEY_A)) input.x -= 1.0f;
    if (IsKeyDown(KEY_D)) input.x += 1.0f;
    if (Vector2Length(input) > 0.0f) input = Vector2Normalize(input);

    float speed = PLAYER_SPEED * (1.0f - 0.35f * game.minigunSpin); // heavy barrels slow you down
    player->velocity = Vector2Scale(input, speed);
    player->position = Vector2Add(player->position, Vector2Scale(player->velocity, dt));
    player->position = clampToArena(player->position, objectRadius(player));

    // Aiming: arrow keys shoot in 8 directions (twin-stick style), otherwise aim at the mouse.
    Vector2 arrows = { 0 };
    if (IsKeyDown(KEY_UP))    arrows.y -= 1.0f;
    if (IsKeyDown(KEY_DOWN))  arrows.y += 1.0f;
    if (IsKeyDown(KEY_LEFT))  arrows.x -= 1.0f;
    if (IsKeyDown(KEY_RIGHT)) arrows.x += 1.0f;

    bool arrowFire = Vector2Length(arrows) > 0.0f;
    if (arrowFire) {
        game.aim = Vector2Normalize(arrows);
    } else {
        Vector2 toMouse = Vector2Subtract(GetMousePosition(), player->position);
        if (Vector2Length(toMouse) > 4.0f) game.aim = Vector2Normalize(toMouse);
    }
    player->rotation = atan2f(game.aim.y, game.aim.x);
    game.firing = arrowFire || IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE);

    // Weapon selection: 1-9, 0, mouse wheel, Q / E
    for (int w = 0; w < WEAPON_COUNT; w++) {
        int key = (w == 9) ? KEY_ZERO : KEY_ONE + w;
        if (IsKeyPressed(key)) selectWeapon((WeaponType)w);
    }
    float wheel = GetMouseWheelMove();
    if (wheel > 0.0f || IsKeyPressed(KEY_Q)) cycleWeapon(-1);
    if (wheel < 0.0f || IsKeyPressed(KEY_E)) cycleWeapon(1);

    // Timers
    if (game.invulnTimer > 0.0f) game.invulnTimer -= dt;
    if (game.shieldTimer > 0.0f) game.shieldTimer -= dt;
    if (player->hitFlash > 0.0f) player->hitFlash -= dt;

    // Engine trail
    if (Vector2Length(input) > 0.0f && GetRandomValue(0, 2) == 0) {
        Vector2 back = Vector2Subtract(player->position, Vector2Scale(game.aim, objectRadius(player)));
        spawnParticles(back, 1, (Color){ 80, 160, 255, 255 }, 40.0f, 0.3f, 2.5f);
    }

    updatePlayerWeapon(player);
}
