#include "globals.h"

// Crosshair that tracks the mouse. Runs in every game state so menus can use it.
void followCursor(GameObject* cursor) {
    if (cursor->type != CURSOR) return;
    cursor->position = GetMousePosition();
    cursor->rotation += GetFrameTime() * 1.5f;
    cursor->color = (game.state == STATE_PLAYING) ? WEAPONS[game.currentWeapon].color : RAYWHITE;
}
