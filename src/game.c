#include <stdio.h>
#include "globals.h"

const EnemyInfo ENEMIES[ENEMY_KIND_COUNT] = {
    [ENEMY_DRONE]    = { "Void Drone",      30.0f, 14.0f, 150.0f,  100, 15.0f, { 255,  70,  90, 255 } },
    [ENEMY_GUNNER]   = { "Pulse Gunner",    60.0f, 18.0f,  90.0f,  250, 10.0f, { 255, 150,  40, 255 } },
    [ENEMY_WARDEN]   = { "Spiral Warden",  130.0f, 24.0f,  45.0f,  400, 15.0f, { 190,  90, 255, 255 } },
    [ENEMY_HULK]     = { "Graviton Hulk",  260.0f, 32.0f,  60.0f,  600, 30.0f, {  90, 230, 120, 255 } },
    [ENEMY_OVERMIND] = { "The Overmind",  2400.0f, 70.0f,  80.0f, 5000, 40.0f, { 255,  60, 200, 255 } },
};

static const Color DEBRIS_COLOR = { 120, 120, 140, 255 };

// ---------------------------------------------------------------------------
// Feedback
// ---------------------------------------------------------------------------
void addShake(float amount)
{
    game.shake = fminf(game.shake + amount, 18.0f);
}

void showMessage(const char* text, Color color, float duration)
{
    snprintf(game.message, sizeof(game.message), "%s", text);
    game.messageColor = color;
    game.messageTimer = duration;
}

// ---------------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------------
static Vector2 gatePosition(int gate)
{
    Vector2 c = ARENA_CENTER;
    float lateral = randf(-GATE_WIDTH * 0.4f, GATE_WIDTH * 0.4f);
    switch (gate) {
    case 0:  return (Vector2){ c.x + lateral, ARENA_TOP + 30.0f };
    case 1:  return (Vector2){ c.x + lateral, ARENA_BOTTOM - 30.0f };
    case 2:  return (Vector2){ ARENA_LEFT + 30.0f, c.y + lateral };
    default: return (Vector2){ ARENA_RIGHT - 30.0f, c.y + lateral };
    }
}

void spawnEnemy(EnemyKind kind, Vector2 position)
{
    const EnemyInfo* info = &ENEMIES[kind];
    GameObject e = makeGameObject(ENEMY, position, (Vector2){ info->radius * 2.0f, info->radius * 2.0f }, enemyAI);
    e.subtype = kind;

    float scale = 1.0f + (float)(game.wave - 1) * 0.08f;
    if (kind == ENEMY_OVERMIND) scale = 1.0f + (float)(game.wave / 5 - 1) * 0.6f;

    e.health = e.maxHealth = info->health * scale;
    e.damage = info->contactDamage;
    e.color = info->color;
    e.param = randf(0.0f, 2.0f * PI);
    e.cooldown = randf(0.8f, 2.0f);
    spawnGameObject(e);
}

void spawnPowerUp(Vector2 position, int kind)
{
    GameObject p = makeGameObject(POWER_UP, position, (Vector2){ 30.0f, 30.0f }, powerUpBehavior);
    p.subtype = kind;
    p.lifetime = 12.0f;
    p.param = randf(0.0f, 2.0f * PI);
    if (kind < WEAPON_COUNT) p.color = WEAPONS[kind].color;
    else if (kind == POWERUP_SHIELD) p.color = (Color){ 80, 180, 255, 255 };
    else p.color = (Color){ 80, 255, 140, 255 };
    spawnGameObject(p);
}

void spawnExtraLife(Vector2 position)
{
    GameObject p = makeGameObject(EXTRA_LIFE, position, (Vector2){ 28.0f, 28.0f }, extraLifeBehavior);
    p.lifetime = 15.0f;
    p.color = (Color){ 120, 255, 120, 255 };
    spawnGameObject(p);
}

static void spawnDebris(Vector2 position, Vector2 velocity, float size)
{
    GameObject d = makeGameObject(OBJ_DYNAMIC, position, (Vector2){ size, size }, dynamicObjectBehavior);
    d.velocity = velocity;
    d.health = d.maxHealth = size * 1.2f;
    d.rotation = randf(0.0f, 2.0f * PI);
    d.param = randf(-2.0f, 2.0f); // spin speed
    d.color = DEBRIS_COLOR;
    spawnGameObject(d);
}

static int randomWeaponPickup(void)
{
    return GetRandomValue(1, WEAPON_COUNT - 1); // never the default AR-15
}

static int randomPowerUp(void)
{
    int roll = GetRandomValue(0, 99);
    if (roll < 10) return POWERUP_SHIELD;
    if (roll < 22) return POWERUP_REPAIR;
    return randomWeaponPickup();
}

static Vector2 randomOpenArenaPoint(float radius)
{
    Vector2 p = ARENA_CENTER;
    for (int attempt = 0; attempt < 12; attempt++) {
        p = (Vector2){ randf(ARENA_LEFT + 80.0f, ARENA_RIGHT - 80.0f), randf(ARENA_TOP + 80.0f, ARENA_BOTTOM - 80.0f) };
        bool blocked = false;
        for (int i = 0; i < game.objects.count; i++) {
            GameObject* s = &game.objects.objects[i];
            if (s->type == OBJ_STATIC && CheckCollisionCircleRec(p, radius + 10.0f, objectRect(s))) { blocked = true; break; }
        }
        if (!blocked) break;
    }
    return p;
}

// ---------------------------------------------------------------------------
// Waves
// ---------------------------------------------------------------------------
static EnemyKind randomEnemyForWave(int wave)
{
    int roll = GetRandomValue(0, 99);
    if (wave < 2) return ENEMY_DRONE;
    if (wave < 3) return roll < 75 ? ENEMY_DRONE : ENEMY_GUNNER;
    if (wave < 4) return roll < 60 ? ENEMY_DRONE : (roll < 85 ? ENEMY_GUNNER : ENEMY_WARDEN);
    if (roll < 48) return ENEMY_DRONE;
    if (roll < 74) return ENEMY_GUNNER;
    if (roll < 89) return ENEMY_WARDEN;
    return ENEMY_HULK;
}

static void startWave(int wave)
{
    game.wave = wave;
    game.bossWave = (wave % 5 == 0);
    game.waveState = WAVE_SPAWNING;
    game.spawnTimer = 0.4f;

    if (game.bossWave) {
        game.enemiesToSpawn = 4 + wave;
        spawnEnemy(ENEMY_OVERMIND, (Vector2){ ARENA_CENTER.x, ARENA_TOP + 120.0f });
        game.gateFlash[0] = 1.0f;
        showMessage("WARNING: THE OVERMIND HAS ENTERED THE ARENA", (Color){ 255, 60, 200, 255 }, 3.0f);
    } else {
        game.enemiesToSpawn = (int)fminf(8.0f + (float)wave * 3.0f, 70.0f);
        showMessage(TextFormat("WAVE %d", wave), SKYBLUE, 2.0f);
    }

    // Asteroid debris drifts in from the edges.
    int debris = (int)fminf(1.0f + (float)(wave / 2), 5.0f);
    for (int i = 0; i < debris; i++) {
        Vector2 pos = gatePosition(GetRandomValue(0, GATE_COUNT - 1));
        Vector2 vel = Vector2Scale(Vector2Normalize(Vector2Subtract(randomOpenArenaPoint(30.0f), pos)), randf(50.0f, 110.0f));
        spawnDebris(pos, vel, randf(40.0f, 70.0f));
    }

    if (wave >= 2) spawnPowerUp(randomOpenArenaPoint(15.0f), randomWeaponPickup());
}

void updateWaves(void)
{
    float dt = game.dt;

    switch (game.waveState) {
    case WAVE_INTERMISSION:
        game.waveTimer -= dt;
        if (game.waveTimer <= 0.0f) startWave(game.wave + 1);
        break;

    case WAVE_SPAWNING:
        game.spawnTimer -= dt;
        if (game.spawnTimer <= 0.0f && game.enemiesToSpawn > 0) {
            int gate = GetRandomValue(0, GATE_COUNT - 1);
            EnemyKind kind = game.bossWave ? (GetRandomValue(0, 2) == 0 ? ENEMY_GUNNER : ENEMY_DRONE) : randomEnemyForWave(game.wave);
            spawnEnemy(kind, gatePosition(gate));
            game.gateFlash[gate] = 0.6f;
            game.enemiesToSpawn--;
            game.spawnTimer = fmaxf(0.15f, 0.7f - (float)game.wave * 0.03f);
        }
        if (game.enemiesToSpawn <= 0) game.waveState = WAVE_FIGHTING;
        break;

    case WAVE_FIGHTING:
        if (countObjectsOfType(&game.objects, ENEMY) + countObjectsOfType(&game.pending, ENEMY) == 0) {
            int bonus = game.wave * 500;
            game.score += bonus;
            game.waveState = WAVE_INTERMISSION;
            game.waveTimer = 3.0f;
            showMessage(TextFormat("WAVE %d CLEARED  +%d", game.wave, bonus), (Color){ 120, 255, 160, 255 }, 2.5f);
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// Damage
// ---------------------------------------------------------------------------
static void destroyObject(GameObject* obj)
{
    float r = objectRadius(obj);

    if (obj->type == ENEMY) {
        const EnemyInfo* info = &ENEMIES[obj->subtype];
        game.score += info->score;
        spawnParticles(obj->position, (int)(r * 1.5f) + 8, obj->color, 60.0f + r * 6.0f, 0.6f, 3.0f);
        spawnParticles(obj->position, 6, WHITE, 220.0f, 0.25f, 2.0f);
        spawnShockwave(obj->position, r * 2.5f, obj->color);
        addShake(r * 0.12f);

        if (obj->subtype == ENEMY_OVERMIND) {
            for (int i = 0; i < 5; i++) {
                spawnParticles(obj->position, 40, i % 2 ? obj->color : ORANGE, 500.0f, 1.2f, 5.0f);
            }
            spawnShockwave(obj->position, 400.0f, obj->color);
            addShake(18.0f);
            spawnExtraLife(obj->position);
            spawnPowerUp(Vector2Add(obj->position, (Vector2){ 50.0f, 0.0f }), randomWeaponPickup());
            spawnPowerUp(Vector2Add(obj->position, (Vector2){ -50.0f, 0.0f }), WEAPON_SINGULARITY);
            showMessage("THE OVERMIND HAS BEEN DESTROYED", (Color){ 255, 60, 200, 255 }, 3.0f);
            return;
        }

        static const int dropChance[ENEMY_KIND_COUNT] = { 50, 110, 180, 300, 0 }; // per 1000
        int roll = GetRandomValue(0, 999);
        if (roll < 12) spawnExtraLife(obj->position);
        else if (roll < 12 + dropChance[obj->subtype]) spawnPowerUp(obj->position, randomPowerUp());
    } else if (obj->type == OBJ_DYNAMIC) {
        game.score += 25;
        spawnParticles(obj->position, (int)r + 6, DEBRIS_COLOR, 40.0f + r * 4.0f, 0.7f, 3.5f);
        if (obj->size.x >= 44.0f) {
            for (int k = 0; k < 2; k++) {
                Vector2 v = Vector2Rotate(obj->velocity, k == 0 ? 1.2f : -1.2f);
                v = Vector2Scale(Vector2Normalize(v), randf(70.0f, 140.0f));
                spawnDebris(obj->position, v, obj->size.x * 0.55f);
            }
        }
    }
}

void damageObject(GameObject* obj, float amount, Vector2 knockback)
{
    if (!obj->active || amount <= 0.0f) return;
    if (obj->type != ENEMY && obj->type != OBJ_DYNAMIC) return;

    obj->health -= amount;
    obj->hitFlash = 0.08f;
    if (!(obj->type == ENEMY && obj->subtype == ENEMY_OVERMIND)) {
        obj->velocity = Vector2Add(obj->velocity, knockback);
    }

    if (obj->health <= 0.0f) {
        obj->active = false;
        destroyObject(obj);
    }
}

bool damagePlayer(float amount)
{
    GameObject* p = game.player;
    if (!p || game.state != STATE_PLAYING) return false;
    if (game.invulnTimer > 0.0f || game.shieldTimer > 0.0f) return false;

    p->health -= amount;
    p->hitFlash = 0.15f;
    game.invulnTimer = 0.5f;
    addShake(6.0f);
    spawnParticles(p->position, 10, RED, 200.0f, 0.3f, 2.5f);

    if (p->health <= 0.0f) {
        spawnParticles(p->position, 60, p->color, 420.0f, 1.0f, 4.0f);
        spawnParticles(p->position, 30, ORANGE, 300.0f, 0.7f, 3.0f);
        spawnShockwave(p->position, 220.0f, p->color);
        addShake(16.0f);
        game.lives--;

        if (game.lives <= 0) {
            game.lives = 0;
            p->health = 0.0f;
            game.state = STATE_GAMEOVER;
            game.stateTimer = 0.0f;
        } else {
            p->health = p->maxHealth;
            p->position = ARENA_CENTER;
            p->velocity = (Vector2){ 0 };
            game.invulnTimer = 2.5f;
            // Mercy: clear incoming fire on respawn.
            for (int i = 0; i < game.objects.count; i++) {
                GameObject* o = &game.objects.objects[i];
                if (o->type == PROJECTILE && !o->fromPlayer) o->active = false;
            }
            showMessage("SHIP LOST - REDEPLOYING", RED, 2.0f);
        }
    }
    return true;
}

void explode(Vector2 position, float radius, float damage, Color color)
{
    for (int i = 0; i < game.objects.count; i++) {
        GameObject* o = &game.objects.objects[i];
        if (!o->active) continue;

        if (o->type == ENEMY || o->type == OBJ_DYNAMIC) {
            Vector2 to = Vector2Subtract(o->position, position);
            float dist = Vector2Length(to) - objectRadius(o);
            if (dist < radius) {
                float falloff = 1.0f - fmaxf(dist, 0.0f) / radius * 0.5f;
                damageObject(o, damage * falloff, Vector2Scale(Vector2Normalize(to), 260.0f));
            }
        } else if (o->type == PROJECTILE && !o->fromPlayer && Vector2Distance(o->position, position) < radius) {
            o->active = false;
        }
    }

    spawnParticles(position, 40, color, radius * 3.0f, 0.5f, 4.0f);
    spawnParticles(position, 20, YELLOW, radius * 2.0f, 0.3f, 3.0f);
    spawnParticles(position, 14, DARKGRAY, radius, 0.9f, 6.0f);
    spawnShockwave(position, radius, color);
    addShake(radius * 0.06f);
}

static void becomeBlackHole(GameObject* p)
{
    p->subtype = PROJ_BLACK_HOLE;
    p->velocity = (Vector2){ 0 };
    p->position = clampToArena(p->position, 30.0f);
    p->timer = 0.0f;
    p->lifetime = 4.0f;
    p->size = (Vector2){ 56.0f, 56.0f };
    spawnShockwave(p->position, 160.0f, p->color);
    addShake(6.0f);
}

void projectileImpact(GameObject* p)
{
    switch (p->subtype) {
    case PROJ_ROCKET:
        explode(p->position, 110.0f, p->damage, (Color){ 255, 120, 40, 255 });
        p->active = false;
        break;
    case PROJ_SINGULARITY:
        becomeBlackHole(p);
        break;
    case PROJ_BLACK_HOLE:
        explode(p->position, 170.0f, 80.0f, (Color){ 170, 110, 255, 255 });
        p->active = false;
        break;
    case PROJ_FLAME:
        p->active = false;
        break;
    default:
        spawnParticles(p->position, 3, p->color, 120.0f, 0.2f, 2.0f);
        p->active = false;
        break;
    }
}

// ---------------------------------------------------------------------------
// Collisions
// ---------------------------------------------------------------------------
static bool pushOutOfRect(Vector2* pos, float r, Rectangle rect, Vector2* normalOut)
{
    float cx = Clamp(pos->x, rect.x, rect.x + rect.width);
    float cy = Clamp(pos->y, rect.y, rect.y + rect.height);
    Vector2 delta = { pos->x - cx, pos->y - cy };
    float d2 = delta.x * delta.x + delta.y * delta.y;
    if (d2 >= r * r) return false;

    Vector2 n;
    float d = sqrtf(d2);
    if (d > 0.0001f) {
        n = (Vector2){ delta.x / d, delta.y / d };
    } else {
        // Center is inside the rectangle: leave through the nearest side.
        float left = pos->x - rect.x, right = rect.x + rect.width - pos->x;
        float top = pos->y - rect.y, bottom = rect.y + rect.height - pos->y;
        float m = fminf(fminf(left, right), fminf(top, bottom));
        if (m == left)       { n = (Vector2){ -1.0f, 0.0f }; cx = rect.x; }
        else if (m == right) { n = (Vector2){ 1.0f, 0.0f };  cx = rect.x + rect.width; }
        else if (m == top)   { n = (Vector2){ 0.0f, -1.0f }; cy = rect.y; }
        else                 { n = (Vector2){ 0.0f, 1.0f };  cy = rect.y + rect.height; }
    }

    pos->x = cx + n.x * r;
    pos->y = cy + n.y * r;
    if (normalOut) *normalOut = n;
    return true;
}

static Vector2 reflect(Vector2 v, Vector2 n)
{
    return Vector2Subtract(v, Vector2Scale(n, 2.0f * Vector2DotProduct(v, n)));
}

static void playerProjectileHits(GameObject* p)
{
    float pr = objectRadius(p);

    for (int j = 0; j < game.objects.count; j++) {
        GameObject* t = &game.objects.objects[j];
        if (!t->active || (t->type != ENEMY && t->type != OBJ_DYNAMIC)) continue;
        if (!CheckCollisionCircles(p->position, pr, t->position, objectRadius(t))) continue;

        Vector2 knock = Vector2Scale(Vector2Normalize(p->velocity), 40.0f);
        switch (p->subtype) {
        case PROJ_FLAME:
            damageObject(t, p->damage * game.dt, (Vector2){ 0 });
            break;
        case PROJ_PLASMA:
            if (p->hitTag == t->id) break;
            damageObject(t, p->damage, Vector2Scale(knock, 2.0f));
            spawnParticles(p->position, 5, p->color, 180.0f, 0.25f, 2.5f);
            p->hitTag = t->id;
            if (--p->pierce <= 0) p->active = false;
            break;
        case PROJ_ROCKET:
        case PROJ_SINGULARITY:
            projectileImpact(p);
            break;
        default:
            damageObject(t, p->damage, knock);
            projectileImpact(p);
            break;
        }

        if (!p->active || p->subtype == PROJ_BLACK_HOLE) return;
    }
}

void resolveCollisions(void)
{
    GameObjectArray* arr = &game.objects;
    GameObject* player = game.player;

    // Projectiles
    for (int i = 0; i < arr->count; i++) {
        GameObject* p = &arr->objects[i];
        if (!p->active || p->type != PROJECTILE || p->subtype == PROJ_BLACK_HOLE) continue;
        float pr = objectRadius(p);

        for (int j = 0; j < arr->count; j++) {
            GameObject* s = &arr->objects[j];
            if (s->type == OBJ_STATIC && CheckCollisionCircleRec(p->position, pr, objectRect(s))) {
                projectileImpact(p);
                break;
            }
        }
        if (!p->active || p->subtype == PROJ_BLACK_HOLE) continue;

        if (p->fromPlayer) {
            playerProjectileHits(p);
        } else if (player) {
            // Small hitbox, bullet-hell style.
            if (CheckCollisionCircles(p->position, pr, player->position, objectRadius(player) * 0.55f)) {
                if (game.shieldTimer > 0.0f) {
                    spawnParticles(p->position, 4, SKYBLUE, 150.0f, 0.2f, 2.0f);
                    p->active = false;
                } else if (damagePlayer(p->damage)) {
                    p->active = false;
                }
            }
        }
    }

    // Player contact with enemies and debris
    if (player && game.state == STATE_PLAYING) {
        float prd = objectRadius(player);
        for (int j = 0; j < arr->count; j++) {
            GameObject* o = &arr->objects[j];
            if (!o->active || (o->type != ENEMY && o->type != OBJ_DYNAMIC)) continue;

            float orr = objectRadius(o);
            Vector2 delta = Vector2Subtract(o->position, player->position);
            float dist = Vector2Length(delta);
            if (dist >= prd + orr) continue;

            Vector2 push = dist > 0.001f ? Vector2Scale(delta, 1.0f / dist) : (Vector2){ 1.0f, 0.0f };
            float overlap = prd + orr - dist;

            if (o->type == ENEMY) {
                bool hurt = damagePlayer(o->damage);
                if (game.shieldTimer > 0.0f) damageObject(o, 120.0f * game.dt, Vector2Scale(push, 60.0f));
                if (hurt && o->subtype == ENEMY_DRONE) {
                    damageObject(o, o->health + 1.0f, (Vector2){ 0 }); // kamikaze
                    continue;
                }
                if (o->subtype == ENEMY_OVERMIND) {
                    player->position = Vector2Subtract(player->position, Vector2Scale(push, overlap));
                } else {
                    o->position = Vector2Add(o->position, Vector2Scale(push, overlap));
                    o->velocity = Vector2Scale(push, 260.0f);
                }
            } else {
                if (damagePlayer(10.0f)) addShake(3.0f);
                o->position = Vector2Add(o->position, Vector2Scale(push, overlap));
                o->velocity = reflect(o->velocity, push);
            }
        }
    }

    // Movers vs static cover
    for (int i = 0; i < arr->count; i++) {
        GameObject* s = &arr->objects[i];
        if (s->type != OBJ_STATIC) continue;
        Rectangle rect = objectRect(s);

        for (int j = 0; j < arr->count; j++) {
            GameObject* o = &arr->objects[j];
            if (!o->active) continue;
            if (o->type != PLAYER && o->type != ENEMY && o->type != OBJ_DYNAMIC) continue;
            if (o->type == ENEMY && o->subtype == ENEMY_OVERMIND) continue; // it hovers over cover

            Vector2 n;
            if (pushOutOfRect(&o->position, objectRadius(o), rect, &n) && o->type == OBJ_DYNAMIC) {
                o->velocity = reflect(o->velocity, n);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void resetGame(void)
{
    clearGameObjects(&game.objects);
    clearGameObjects(&game.pending);
    clearParticles();

    game.score = 0;
    game.lives = PLAYER_START_LIVES;
    game.wave = 0;
    game.waveState = WAVE_INTERMISSION;
    game.waveTimer = 1.5f;
    game.spawnTimer = 0.0f;
    game.enemiesToSpawn = 0;
    game.bossWave = false;
    memset(game.gateFlash, 0, sizeof(game.gateFlash));

    game.time = 0.0f;
    game.invulnTimer = 2.0f;
    game.shieldTimer = 0.0f;
    game.shake = 0.0f;
    game.messageTimer = 0.0f;
    game.aim = (Vector2){ 0.0f, -1.0f };
    game.firing = false;
    resetWeapons();

    GameObject* player = createGameObject(&game.objects, PLAYER, NULL, ARENA_CENTER, (Vector2){ 34.0f, 34.0f }, movePlayer);
    player->health = player->maxHealth = PLAYER_MAX_HEALTH;
    player->color = (Color){ 80, 200, 255, 255 };
    game.playerId = player->id;

    GameObject* cursor = createGameObject(&game.objects, CURSOR, NULL, GetMousePosition(), (Vector2){ 26.0f, 26.0f }, followCursor);
    cursor->color = RAYWHITE;

    // Cover pillars
    Vector2 c = ARENA_CENTER;
    const float ox[2] = { -330.0f, 330.0f };
    const float oy[2] = { -150.0f, 150.0f };
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            GameObject* s = createGameObject(&game.objects, OBJ_STATIC, NULL, (Vector2){ c.x + ox[x], c.y + oy[y] }, (Vector2){ 64.0f, 64.0f }, NULL);
            s->color = (Color){ 60, 140, 255, 255 };
        }
    }

    game.player = findObjectById(&game.objects, game.playerId);
}
