#include <stdlib.h>
#include "globals.h"

static int nextObjectId = 1;

// ---------------------------------------------------------------------------
// Dynamic Array Functions
// ---------------------------------------------------------------------------
void initGameObjects(GameObjectArray* arr, int initialCapacity)
{
    if (initialCapacity < 1) initialCapacity = 1;
    arr->objects = malloc(sizeof(GameObject) * initialCapacity);
    arr->count = 0;
    arr->capacity = arr->objects ? initialCapacity : 0;
}

static bool reserveGameObjects(GameObjectArray* arr, int needed)
{
    if (needed <= arr->capacity) return true;

    int newCapacity = arr->capacity > 0 ? arr->capacity * 2 : 16;
    while (newCapacity < needed) newCapacity *= 2;

    GameObject* grown = realloc(arr->objects, sizeof(GameObject) * newCapacity);
    if (!grown) return false;

    arr->objects = grown;
    arr->capacity = newCapacity;
    return true;
}

void clearGameObjects(GameObjectArray* arr)
{
    for (int i = 0; i < arr->count; i++) {
        if (arr->objects[i].texture.id != 0) UnloadTexture(arr->objects[i].texture);
    }
    arr->count = 0;
}

void freeGameObjects(GameObjectArray* arr)
{
    clearGameObjects(arr);
    free(arr->objects);
    arr->objects = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

GameObject makeGameObject(ObjectType type, Vector2 position, Vector2 size, void (*script)(GameObject*))
{
    GameObject obj = { 0 };
    obj.type = type;
    obj.active = true;
    obj.position = position;
    obj.size = size;
    obj.health = 1.0f;
    obj.maxHealth = 1.0f;
    obj.pierce = 1;
    obj.hitTag = -1;
    obj.linkId = -1;
    obj.color = WHITE;
    obj.script = script;
    return obj;
}

GameObject* addGameObject(GameObjectArray* arr, GameObject obj)
{
    if (!reserveGameObjects(arr, arr->count + 1)) return NULL;
    if (obj.id == 0) obj.id = nextObjectId++;
    arr->objects[arr->count++] = obj;
    return &arr->objects[arr->count - 1];
}

GameObject* createGameObject(GameObjectArray* arr, ObjectType type, const char* texturePath, Vector2 position, Vector2 size, void (*script)(GameObject*))
{
    GameObject obj = makeGameObject(type, position, size, script);
    if (texturePath) obj.texture = LoadTexture(texturePath);
    return addGameObject(arr, obj);
}

GameObject* spawnGameObject(GameObject obj)
{
    return addGameObject(&game.pending, obj);
}

void flushPendingObjects(void)
{
    for (int i = 0; i < game.pending.count; i++) {
        addGameObject(&game.objects, game.pending.objects[i]);
    }
    game.pending.count = 0;
}

void removeInactiveObjects(GameObjectArray* arr)
{
    int write = 0;
    for (int read = 0; read < arr->count; read++) {
        GameObject* obj = &arr->objects[read];
        if (obj->active) {
            if (write != read) arr->objects[write] = *obj;
            write++;
        } else if (obj->texture.id != 0) {
            UnloadTexture(obj->texture);
        }
    }
    arr->count = write;
}

GameObject* findObjectById(GameObjectArray* arr, int id)
{
    for (int i = 0; i < arr->count; i++) {
        if (arr->objects[i].id == id) return &arr->objects[i];
    }
    return NULL;
}

GameObject* findFirstEnemyOfKind(EnemyKind kind)
{
    for (int i = 0; i < game.objects.count; i++) {
        GameObject* obj = &game.objects.objects[i];
        if (obj->active && obj->type == ENEMY && obj->subtype == (int)kind) return obj;
    }
    return NULL;
}

int countObjectsOfType(GameObjectArray* arr, ObjectType type)
{
    int count = 0;
    for (int i = 0; i < arr->count; i++) {
        if (arr->objects[i].active && arr->objects[i].type == type) count++;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------
float objectRadius(const GameObject* obj)
{
    return obj->size.x * 0.5f;
}

Rectangle objectRect(const GameObject* obj)
{
    return (Rectangle){ obj->position.x - obj->size.x * 0.5f, obj->position.y - obj->size.y * 0.5f, obj->size.x, obj->size.y };
}

Vector2 clampToArena(Vector2 position, float radius)
{
    position.x = Clamp(position.x, ARENA_LEFT + radius, ARENA_RIGHT - radius);
    position.y = Clamp(position.y, ARENA_TOP + radius, ARENA_BOTTOM - radius);
    return position;
}

float randf(float min, float max)
{
    return min + (max - min) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}

float angleDiff(float from, float to)
{
    float d = fmodf(to - from + PI, 2.0f * PI);
    if (d < 0.0f) d += 2.0f * PI;
    return d - PI;
}
