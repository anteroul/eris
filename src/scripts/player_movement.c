#include "globals.h"

void movePlayer(GameObject* player) {
	if (player->type != PLAYER) return;
    if (IsKeyDown(KEY_W)) player->position.y -= 2.0f;
    if (IsKeyDown(KEY_A)) player->position.x -= 2.0f;
    if (IsKeyDown(KEY_S)) player->position.y += 2.0f;
    if (IsKeyDown(KEY_D)) player->position.x += 2.0f;
}
