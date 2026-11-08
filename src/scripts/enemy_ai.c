#include "globals.h"

// ---------------------------------------------------------------------------
// Bullet patterns
// ---------------------------------------------------------------------------
static void fireEnemyBullet(Vector2 origin, float angle, float speed, float radius, Color color, float damage)
{
    GameObject* b = fireProjectile(origin, (Vector2){ cosf(angle), sinf(angle) }, PROJ_ENEMY_BULLET, speed, damage, radius, color, false);
    if (b) b->lifetime = 9.0f;
}

static void fireRing(Vector2 origin, int count, float offset, float speed, float radius, Color color, float damage)
{
    for (int i = 0; i < count; i++) {
        fireEnemyBullet(origin, offset + (float)i * 2.0f * PI / (float)count, speed, radius, color, damage);
    }
}

static void fireSpread(Vector2 origin, float center, int count, float arc, float speed, float radius, Color color, float damage)
{
    if (count <= 1) {
        fireEnemyBullet(origin, center, speed, radius, color, damage);
        return;
    }
    for (int i = 0; i < count; i++) {
        float a = center - arc * 0.5f + arc * (float)i / (float)(count - 1);
        fireEnemyBullet(origin, a, speed, radius, color, damage);
    }
}

// ---------------------------------------------------------------------------
// Boss
// ---------------------------------------------------------------------------
static Vector2 overmindBehavior(GameObject* e, float angleToPlayer)
{
    float dt = game.dt;
    float hp = e->health / e->maxHealth;
    Vector2 c = ARENA_CENTER;
    Color spiral = e->color;
    Color heavy = { 255, 170, 60, 255 };

    // Primary: rotating flower spiral that changes with each phase
    e->param += dt * (hp > 0.33f ? 1.3f : -2.1f);
    if (e->cooldown <= 0.0f) {
        int arms = hp > 0.66f ? 6 : (hp > 0.33f ? 4 : 8);
        for (int a = 0; a < arms; a++) {
            fireEnemyBullet(e->position, e->param + (float)a * 2.0f * PI / (float)arms, 170.0f, 7.0f, spiral, 12.0f);
        }
        e->cooldown = hp > 0.66f ? 0.10f : (hp > 0.33f ? 0.16f : 0.08f);
    }

    // Secondary: rings and aimed bursts
    e->anchor.x -= dt;
    if (e->anchor.x <= 0.0f) {
        if (hp > 0.66f) {
            fireRing(e->position, 20, randf(0.0f, PI), 140.0f, 9.0f, heavy, 15.0f);
            e->anchor.x = 2.6f;
        } else if (hp > 0.33f) {
            fireSpread(e->position, angleToPlayer, 9, 0.9f, 300.0f, 6.0f, YELLOW, 12.0f);
            e->anchor.x = 1.2f;
        } else {
            fireRing(e->position, 28, randf(0.0f, PI), 150.0f, 8.0f, heavy, 15.0f);
            fireSpread(e->position, angleToPlayer, 5, 0.4f, 360.0f, 6.0f, YELLOW, 12.0f);
            e->anchor.x = 1.6f;
        }
    }

    // Below half health it calls in drone escorts
    e->anchor.y -= dt;
    if (hp < 0.5f && e->anchor.y <= 0.0f) {
        spawnEnemy(ENEMY_DRONE, Vector2Add(e->position, (Vector2){ 80.0f, 0.0f }));
        spawnEnemy(ENEMY_DRONE, Vector2Add(e->position, (Vector2){ -80.0f, 0.0f }));
        e->anchor.y = 5.0f;
    }

    Vector2 target = { c.x + cosf(e->timer * 0.35f) * 360.0f, c.y - 40.0f + sinf(e->timer * 0.7f) * 150.0f };
    return Vector2Subtract(target, e->position);
}

// ---------------------------------------------------------------------------
// Enemy script
// ---------------------------------------------------------------------------
void enemyAI(GameObject* e)
{
    if (e->type != ENEMY) return;
    GameObject* player = game.player;
    if (!player) return;

    float dt = game.dt;
    const EnemyInfo* info = &ENEMIES[e->subtype];
    e->timer += dt;
    e->cooldown -= dt;
    if (e->hitFlash > 0.0f) e->hitFlash -= dt;

    Vector2 toPlayer = Vector2Subtract(player->position, e->position);
    float dist = Vector2Length(toPlayer);
    Vector2 dir = dist > 0.01f ? Vector2Scale(toPlayer, 1.0f / dist) : (Vector2){ 0 };
    Vector2 perp = { -dir.y, dir.x };
    float angleToPlayer = atan2f(dir.y, dir.x);
    Vector2 desired = { 0 };
    float speed = info->speed;
    int wave = game.wave;

    switch (e->subtype) {
    case ENEMY_DRONE:
        // Weaving kamikaze run
        desired = Vector2Add(dir, Vector2Scale(perp, sinf(e->timer * 4.0f + e->param) * 0.6f));
        break;

    case ENEMY_GUNNER:
        // Hold a firing distance and strafe
        if (dist > 330.0f) desired = dir;
        else if (dist < 200.0f) desired = Vector2Negate(dir);
        else desired = Vector2Scale(perp, e->param > PI ? 1.0f : -1.0f);
        if (e->cooldown <= 0.0f) {
            fireSpread(e->position, angleToPlayer, wave >= 4 ? 5 : 3, 0.5f, 240.0f, 5.0f, e->color, 10.0f);
            e->cooldown = 1.8f - fminf((float)wave * 0.05f, 0.6f);
        }
        break;

    case ENEMY_WARDEN:
        // Slow advance while spinning out bullet spirals
        desired = dir;
        e->param += dt * 2.2f;
        if (e->cooldown <= 0.0f) {
            int arms = wave >= 8 ? 3 : 2;
            for (int a = 0; a < arms; a++) {
                fireEnemyBullet(e->position, e->param + (float)a * 2.0f * PI / (float)arms, 150.0f, 6.0f, e->color, 10.0f);
            }
            e->cooldown = 0.12f;
        }
        break;

    case ENEMY_HULK:
        // Lumbers toward the player emitting expanding rings
        desired = dir;
        if (e->cooldown <= 0.0f) {
            int count = (int)fminf(14.0f + (float)(wave / 2), 24.0f);
            fireRing(e->position, count, e->timer, 150.0f, 7.0f, e->color, 15.0f);
            e->cooldown = 2.6f;
        }
        break;

    case ENEMY_OVERMIND: {
        Vector2 toTarget = overmindBehavior(e, angleToPlayer);
        float d = Vector2Length(toTarget);
        speed = fminf(info->speed, d * 1.5f);
        desired = toTarget;
    } break;

    default:
        break;
    }

    if (Vector2Length(desired) > 0.0f) desired = Vector2Normalize(desired);

    // Steering: blend toward the desired velocity (also bleeds off knockback)
    e->velocity = Vector2Lerp(e->velocity, Vector2Scale(desired, speed), fminf(1.0f, dt * 4.0f));
    e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));

    // Separation so swarms don't stack into one blob
    if (e->subtype != ENEMY_OVERMIND) {
        float er = objectRadius(e);
        for (int i = 0; i < game.objects.count; i++) {
            GameObject* o = &game.objects.objects[i];
            if (o == e || !o->active || o->type != ENEMY) continue;
            Vector2 d = Vector2Subtract(e->position, o->position);
            float minDist = er + objectRadius(o);
            float d2 = d.x * d.x + d.y * d.y;
            if (d2 < minDist * minDist && d2 > 0.0001f) {
                float len = sqrtf(d2);
                e->position = Vector2Add(e->position, Vector2Scale(d, (minDist - len) * 0.5f / len));
            }
        }
    }

    e->position = clampToArena(e->position, objectRadius(e));
    e->rotation = angleToPlayer;
}
