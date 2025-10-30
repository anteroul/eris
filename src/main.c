#include <raylib.h>
#include <stdlib.h>
#include <resource_dir.h>
#include "globals.h"

// Dynamic Array Functions
void initGameObjects(GameObjectArray* arr, int initialCapacity)
{
    arr->objects = malloc(sizeof(GameObject) * initialCapacity);
    arr->count = 0;
    arr->capacity = initialCapacity;
}

void freeGameObjects(GameObjectArray* arr)
{
    for (int i = 0; i < arr->count; i++) {
        UnloadTexture(arr->objects[i].texture);
    }
    free(arr->objects);
    arr->objects = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

GameObject* createGameObject(GameObjectArray* arr, ObjectType type, const char* texturePath, Vector2 position, Vector2 size, void* script)
{
    // Resize array if needed
    if (arr->count >= arr->capacity) {
        arr->capacity++;
        if (arr->objects == NULL) {
            arr->objects = malloc(sizeof(GameObject) * arr->capacity);
        } else {
            arr->objects = realloc(arr->objects, sizeof(GameObject) * arr->capacity);
		}
    }

    // Create and initialize object
    GameObject obj;
	obj.type = type;
    obj.texture = LoadTexture(texturePath);
    obj.position = position;
    obj.size = size;
	obj.script = script;

    arr->objects[arr->count++] = obj;

    // Return pointer to new object
    return &arr->objects[arr->count - 1];
}

// Game Logic
void update(GameObjectArray* arr)
{
    for (int i = 0; i < arr->count; i++) {
        GameObject* obj = &arr->objects[i];

        if (obj->script) {
            obj->script(obj);  // call the assigned function
        }

        // You can add logic per type here too:
        if (obj->type == ENEMY) {
            // enemy logic...
        }
    }
}

// Rendering
void draw(GameObjectArray* arr)
{
    BeginDrawing();
    ClearBackground(BLACK);

    for (int i = 0; i < arr->count; i++) {
        GameObject* obj = &arr->objects[i];
        DrawTextureEx(obj->texture, obj->position, 0.0f,
            obj->size.x / obj->texture.width, WHITE);
    }

    EndDrawing();
}

// Entry Point
int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 600, "Project Eris");
    //InitWindow(GetScreenWidth(), GetScreenHeight(), "Project Eris");
    //ToggleFullscreen();
    SearchAndSetResourceDir("resources");

    GameObjectArray gameObjects;
    initGameObjects(&gameObjects, 4);

    // Create player
    createGameObject(&gameObjects,
        PLAYER,
        "wabbit_alpha.png",
        (Vector2) {
        GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f
    },
        (Vector2) {
        40, 80
    }, movePlayer
    );

    // Example: add another object
    createGameObject(&gameObjects,
        OBJ_STATIC,
        "wabbit_alpha.png",
        (Vector2) {
        200, 200
    },
        (Vector2) {
        30, 60
	}, NULL
    );

    // Main loop
    while (!WindowShouldClose())
    {
        update(&gameObjects);
        draw(&gameObjects);
    }

    // Cleanup
    freeGameObjects(&gameObjects);
    CloseWindow();

    return 0;
}
