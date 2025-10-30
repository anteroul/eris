#pragma once
#include <raylib.h>
#include <string.h>

typedef enum GameState {
	STATE_MENU,
	STATE_PLAYING,
	STATE_PAUSED,
	STATE_GAMEOVER
} GameState;

typedef enum ObjectType {
	OBJ_STATIC,
	OBJ_DYNAMIC,
	PLAYER,
	ENEMY,
	PROJECTILE,
	EXTRA_LIFE,
	POWER_UP,
	CURSOR
} ObjectType;

// Global Type Definitions
typedef struct GameObject {
	ObjectType type;
	Vector2 position;
	Vector2 size;
	Texture2D texture;
	void (*script)(struct GameObject*);
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
void exitGame(void);

// GameObject Script Function Pointer Types
void movePlayer(GameObject* player);
void followCursor(GameObject* cursor);
//void enemyAI(GameObject* enemy);
//void projectileBehavior(GameObject* projectile);
//void powerUpBehavior(GameObject* powerUp);
//void extraLifeBehavior(GameObject* extraLife);