#include "globals.h"

#define DEBRIS_MAX_SPEED 260.0f

// Drifting asteroid debris: tumbles, bounces off the arena walls, splits when shot.
void dynamicObjectBehavior(GameObject* object)
{
    if (object->type != OBJ_DYNAMIC) return;
    float dt = game.dt;
    float r = objectRadius(object);

    if (object->hitFlash > 0.0f) object->hitFlash -= dt;
    object->rotation += object->param * dt;

    float speed = Vector2Length(object->velocity);
    if (speed > DEBRIS_MAX_SPEED) object->velocity = Vector2Scale(object->velocity, DEBRIS_MAX_SPEED / speed);
    object->position = Vector2Add(object->position, Vector2Scale(object->velocity, dt));

    if (object->position.x - r < ARENA_LEFT)   { object->position.x = ARENA_LEFT + r;   object->velocity.x = fabsf(object->velocity.x); }
    if (object->position.x + r > ARENA_RIGHT)  { object->position.x = ARENA_RIGHT - r;  object->velocity.x = -fabsf(object->velocity.x); }
    if (object->position.y - r < ARENA_TOP)    { object->position.y = ARENA_TOP + r;    object->velocity.y = fabsf(object->velocity.y); }
    if (object->position.y + r > ARENA_BOTTOM) { object->position.y = ARENA_BOTTOM - r; object->velocity.y = -fabsf(object->velocity.y); }
}
