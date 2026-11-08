#pragma once
#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Screen & arena layout
// ---------------------------------------------------------------------------
#define SCREEN_WIDTH        1280
#define SCREEN_HEIGHT       720
#define HUD_HEIGHT          64

#define ARENA_LEFT          24.0f
#define ARENA_TOP           (HUD_HEIGHT + 16.0f)
#define ARENA_RIGHT         (SCREEN_WIDTH - 24.0f)
#define ARENA_BOTTOM        (SCREEN_HEIGHT - 16.0f)
#define ARENA_CENTER        ((Vector2){ (ARENA_LEFT + ARENA_RIGHT) * 0.5f, (ARENA_TOP + ARENA_BOTTOM) * 0.5f })
#define GATE_WIDTH          120.0f
#define GATE_COUNT          4

#define PLAYER_MAX_HEALTH   100.0f
#define PLAYER_START_LIVES  3
#define PAUSE_MENU_ITEMS    4

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------
typedef enum GameState {
	STATE_MENU,
	STATE_PLAYING,
	STATE_PAUSED,
	STATE_GAMEOVER
} GameState;

typedef enum ObjectType {
	OBJ_STATIC,     // indestructible cover pillars
	OBJ_DYNAMIC,    // drifting, destructible asteroid debris
	PLAYER,
	ENEMY,
	PROJECTILE,
	EXTRA_LIFE,
	POWER_UP,
	CURSOR
} ObjectType;

typedef enum EnemyKind {
	ENEMY_DRONE,    // fast kamikaze chaser
	ENEMY_GUNNER,   // keeps distance, fires aimed spreads
	ENEMY_WARDEN,   // slow, emits rotating bullet spirals
	ENEMY_HULK,     // tanky, fires bullet rings
	ENEMY_OVERMIND, // boss, every 5th wave
	ENEMY_KIND_COUNT
} EnemyKind;

typedef enum ProjectileKind {
	PROJ_BULLET,
	PROJ_PLASMA,
	PROJ_ENTANGLED,
	PROJ_SINGULARITY,
	PROJ_BLACK_HOLE,
	PROJ_FLAME,
	PROJ_ROCKET,
	PROJ_ENEMY_BULLET
} ProjectileKind;

typedef enum WeaponType {
	WEAPON_AR15,
	WEAPON_PLASMA_RIFLE,
	WEAPON_Q_ENTANGLER,
	WEAPON_SINGULARITY,
	WEAPON_FLAMETHROWER,
	WEAPON_ROCKET_LAUNCHER,
	WEAPON_MINIGUN,
	WEAPON_DUAL_UZIS,
	WEAPON_CHAINSAW,
	WEAPON_ENERGY_SWORD,
	WEAPON_COUNT
} WeaponType;

// POWER_UP subtypes below WEAPON_COUNT are weapon pickups (subtype == WeaponType).
typedef enum PowerUpKind {
	POWERUP_SHIELD = WEAPON_COUNT,
	POWERUP_REPAIR
} PowerUpKind;

typedef enum WaveState {
	WAVE_INTERMISSION,
	WAVE_SPAWNING,
	WAVE_FIGHTING
} WaveState;

// ---------------------------------------------------------------------------
// Global Type Definitions
// ---------------------------------------------------------------------------
typedef struct GameObject {
	int id;
	ObjectType type;
	int subtype;            // EnemyKind / ProjectileKind / PowerUpKind / WeaponType
	bool active;            // false = removed at end of frame
	Vector2 position;       // center
	Vector2 velocity;
	Vector2 size;
	Vector2 anchor;         // projectile base path, boss secondary timers
	float rotation;         // radians
	float health;
	float maxHealth;
	float damage;           // hit damage, damage per second for continuous sources
	float timer;            // age in seconds
	float lifetime;         // max age (0 = unlimited)
	float cooldown;         // attack timer
	float param;            // per-type extra (spin, phase, ...)
	float hitFlash;
	int pierce;
	int hitTag;             // last hit target id (projectiles) / last sword swing (targets)
	int linkId;             // entangled partner id
	bool fromPlayer;
	Color color;
	Texture2D texture;      // optional; procedural drawing is used when id == 0
	void (*script)(struct GameObject*);
} GameObject;

typedef struct GameObjectArray {
	GameObject* objects;
	int count;
	int capacity;
} GameObjectArray;

typedef struct WeaponInfo {
	const char* name;
	const char* shortName;
	float fireInterval;     // seconds between shots
	float damage;           // per hit (flames & chainsaw: per second)
	float speed;            // projectile speed, melee: reach
	float spread;           // random spread in radians, melee: half arc
	int pickupAmmo;         // -1 = unlimited
	bool melee;
	Color color;
} WeaponInfo;

typedef struct EnemyInfo {
	const char* name;
	float health;
	float radius;
	float speed;
	int score;
	float contactDamage;
	Color color;
} EnemyInfo;

typedef struct Game {
	GameState state;
	float stateTimer;
	int menuSelection;
	bool quit;

	GameObjectArray objects;
	GameObjectArray pending;    // objects spawned during the frame, merged after updates
	GameObject* player;         // refreshed every frame
	int playerId;

	float dt;
	float time;

	int score;
	int lives;
	int wave;
	WaveState waveState;
	float waveTimer;
	float spawnTimer;
	int enemiesToSpawn;
	bool bossWave;
	float gateFlash[GATE_COUNT];

	WeaponType currentWeapon;
	bool owned[WEAPON_COUNT];
	int ammo[WEAPON_COUNT];     // -1 = unlimited
	Vector2 aim;
	bool firing;
	float fireCooldown;
	float minigunSpin;
	bool uziLeft;
	bool chainsawActive;
	float swordSwingTimer;
	float swordSwingAngle;
	int swingId;

	float invulnTimer;
	float shieldTimer;
	float shake;

	char message[96];
	Color messageColor;
	float messageTimer;
} Game;

extern Game game;
extern const WeaponInfo WEAPONS[WEAPON_COUNT];
extern const EnemyInfo ENEMIES[ENEMY_KIND_COUNT];

// ---------------------------------------------------------------------------
// Function Declarations
// ---------------------------------------------------------------------------

// objects.c - dynamic array & helpers
void initGameObjects(GameObjectArray* arr, int initialCapacity);
void clearGameObjects(GameObjectArray* arr);
void freeGameObjects(GameObjectArray* arr);
GameObject makeGameObject(ObjectType type, Vector2 position, Vector2 size, void (*script)(GameObject*));
GameObject* createGameObject(GameObjectArray* arr, ObjectType type, const char* texturePath, Vector2 position, Vector2 size, void (*script)(GameObject*));
GameObject* addGameObject(GameObjectArray* arr, GameObject obj);
GameObject* spawnGameObject(GameObject obj);   // deferred add; pointer valid until the next spawn
void flushPendingObjects(void);
void removeInactiveObjects(GameObjectArray* arr);
GameObject* findObjectById(GameObjectArray* arr, int id);
GameObject* findFirstEnemyOfKind(EnemyKind kind);
int countObjectsOfType(GameObjectArray* arr, ObjectType type);
float objectRadius(const GameObject* obj);
Rectangle objectRect(const GameObject* obj);
Vector2 clampToArena(Vector2 position, float radius);
float randf(float min, float max);
float angleDiff(float from, float to);

// main.c
void update(GameObjectArray* arr);
void exitGame(void);

// game.c - rules, waves, collisions
void resetGame(void);
void updateWaves(void);
void resolveCollisions(void);
void damageObject(GameObject* obj, float amount, Vector2 knockback);
bool damagePlayer(float amount);
void explode(Vector2 position, float radius, float damage, Color color);
void projectileImpact(GameObject* projectile);
void spawnEnemy(EnemyKind kind, Vector2 position);
void spawnPowerUp(Vector2 position, int kind);
void spawnExtraLife(Vector2 position);
void addShake(float amount);
void showMessage(const char* text, Color color, float duration);

// weapons.c
void resetWeapons(void);
void giveWeapon(WeaponType weapon);
bool weaponAvailable(WeaponType weapon);
void selectWeapon(WeaponType weapon);
void cycleWeapon(int direction);
void updatePlayerWeapon(GameObject* player);
GameObject* fireProjectile(Vector2 origin, Vector2 direction, ProjectileKind kind, float speed, float damage, float radius, Color color, bool fromPlayer);

// particles.c
void spawnParticles(Vector2 position, int count, Color color, float speed, float life, float size);
void spawnShockwave(Vector2 position, float radius, Color color);
void updateParticles(float dt);
void drawParticles(void);
void clearParticles(void);

// render.c
void draw(GameObjectArray* arr);
Rectangle pauseMenuItemRect(int index);

// GameObject Script Function Pointer Types
void movePlayer(GameObject* player);
void followCursor(GameObject* cursor);
void enemyAI(GameObject* enemy);
void projectileBehavior(GameObject* projectile);
void powerUpBehavior(GameObject* powerUp);
void extraLifeBehavior(GameObject* extraLife);
void dynamicObjectBehavior(GameObject* object);
