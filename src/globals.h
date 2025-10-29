#pragma once
#include <raylib.h>

// Global Type Definitions
typedef struct GameObject {
	Vector2 position;
	Vector2 size;
	Texture2D texture;
} GameObject;

typedef struct {
	GameObject* objects;
	int count;
	int capacity;
} GameObjectArray;

// Function Declarations
GameObject* createGameObject(GameObjectArray* arr, const char* texturePath, Vector2 position, Vector2 size);
void initGameObjects(GameObjectArray* arr, int initialCapacity);
void addGameObject(GameObjectArray* arr, GameObject obj);
void freeGameObjects(GameObjectArray* arr);
void draw(struct GameObject* obj);
void update(struct GameObject* player);
void exitGame();