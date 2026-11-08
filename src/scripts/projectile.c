#include "globals.h"

#define BLACK_HOLE_PULL_RADIUS 320.0f
#define BLACK_HOLE_CORE_RADIUS 40.0f

// The singularity's event horizon drags enemies, debris and enemy fire inward,
// crushing whatever reaches the core.
static void updateBlackHole(GameObject* bh)
{
    float dt = game.dt;
    bh->rotation += dt * 6.0f;
    bh->size.x = bh->size.y = 56.0f + sinf(bh->timer * 10.0f) * 4.0f;
    game.shake = fmaxf(game.shake, 1.5f);

    for (int i = 0; i < game.objects.count; i++) {
        GameObject* o = &game.objects.objects[i];
        if (!o->active || o == bh) continue;

        bool enemyBullet = (o->type == PROJECTILE && !o->fromPlayer);
        if (o->type != ENEMY && o->type != OBJ_DYNAMIC && !enemyBullet) continue;

        Vector2 to = Vector2Subtract(bh->position, o->position);
        float d = Vector2Length(to);
        if (d > BLACK_HOLE_PULL_RADIUS || d < 0.001f) continue;

        Vector2 dir = Vector2Scale(to, 1.0f / d);
        float strength = 520.0f * (1.0f - d / BLACK_HOLE_PULL_RADIUS) + 60.0f;
        if (o->type == ENEMY && o->subtype == ENEMY_OVERMIND) strength *= 0.15f;
        o->position = Vector2Add(o->position, Vector2Scale(dir, strength * dt));

        if (enemyBullet) {
            o->velocity = Vector2Add(o->velocity, Vector2Scale(dir, strength * 2.0f * dt));
            if (d < BLACK_HOLE_CORE_RADIUS) o->active = false;
            continue;
        }

        if (d < BLACK_HOLE_CORE_RADIUS + objectRadius(o)) damageObject(o, 150.0f * dt, (Vector2){ 0 });
    }

    // Infalling matter
    if (GetRandomValue(0, 1) == 0) {
        float a = randf(0.0f, 2.0f * PI);
        Vector2 from = Vector2Add(bh->position, (Vector2){ cosf(a) * randf(60.0f, 180.0f), sinf(a) * randf(60.0f, 180.0f) });
        spawnParticles(from, 1, bh->color, 30.0f, 0.5f, 2.0f);
    }
}

void projectileBehavior(GameObject* p)
{
    if (p->type != PROJECTILE) return;
    float dt = game.dt;
    p->timer += dt;

    if (p->lifetime > 0.0f && p->timer >= p->lifetime) {
        if (p->subtype == PROJ_ROCKET || p->subtype == PROJ_SINGULARITY || p->subtype == PROJ_BLACK_HOLE) projectileImpact(p);
        else p->active = false;
        return;
    }

    switch (p->subtype) {
    case PROJ_ENTANGLED: {
        // Base path moves straight; the quantum oscillates around it in a helix.
        p->anchor = Vector2Add(p->anchor, Vector2Scale(p->velocity, dt));
        Vector2 dir = Vector2Normalize(p->velocity);
        Vector2 perp = { -dir.y, dir.x };
        float offset = sinf(p->timer * 18.0f + p->param) * 16.0f;
        p->position = Vector2Add(p->anchor, Vector2Scale(perp, offset));
    } break;

    case PROJ_ROCKET: {
        float speed = Vector2Length(p->velocity);
        if (speed < 1100.0f) p->velocity = Vector2Scale(p->velocity, 1.0f + 1.6f * dt);
        p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
        spawnParticles(p->position, 1, GRAY, 30.0f, 0.4f, 3.0f);
    } break;

    case PROJ_FLAME:
        p->velocity = Vector2Scale(p->velocity, expf(-2.5f * dt));
        p->size.x = p->size.y = 14.0f * (1.0f + p->timer * 4.0f);
        p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
        break;

    case PROJ_SINGULARITY:
        p->rotation += dt * 8.0f;
        p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
        spawnParticles(p->position, 1, p->color, 40.0f, 0.3f, 2.5f);
        break;

    case PROJ_BLACK_HOLE:
        updateBlackHole(p);
        return; // stationary, ignores arena bounds

    default:
        p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
        break;
    }

    // Left the arena
    float r = objectRadius(p);
    if (p->position.x < ARENA_LEFT - r || p->position.x > ARENA_RIGHT + r ||
        p->position.y < ARENA_TOP - r || p->position.y > ARENA_BOTTOM + r) {
        if (p->subtype == PROJ_ROCKET || p->subtype == PROJ_SINGULARITY) {
            p->position = clampToArena(p->position, 4.0f);
            projectileImpact(p);
        } else {
            p->active = false;
        }
    }
}
