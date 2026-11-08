#include "globals.h"

// Lightweight visual-only particles (sparks, smoke, shockwaves). They never
// collide with anything, so they live outside the GameObject array.

#define MAX_PARTICLES 3000

typedef struct Particle {
    Vector2 position;
    Vector2 velocity;
    float life;
    float maxLife;
    float size;
    bool ring;      // expanding shockwave ring instead of a dot
    Color color;
} Particle;

static Particle particles[MAX_PARTICLES];
static int nextParticle = 0;

static Particle* allocParticle(void)
{
    Particle* p = &particles[nextParticle];
    nextParticle = (nextParticle + 1) % MAX_PARTICLES;
    return p;
}

void spawnParticles(Vector2 position, int count, Color color, float speed, float life, float size)
{
    for (int i = 0; i < count; i++) {
        Particle* p = allocParticle();
        float angle = randf(0.0f, 2.0f * PI);
        float s = speed * randf(0.25f, 1.0f);
        p->position = position;
        p->velocity = (Vector2){ cosf(angle) * s, sinf(angle) * s };
        p->maxLife = life * randf(0.6f, 1.0f);
        p->life = p->maxLife;
        p->size = size * randf(0.6f, 1.2f);
        p->ring = false;
        p->color = color;
    }
}

void spawnShockwave(Vector2 position, float radius, Color color)
{
    Particle* p = allocParticle();
    p->position = position;
    p->velocity = (Vector2){ 0 };
    p->maxLife = 0.35f;
    p->life = p->maxLife;
    p->size = radius;
    p->ring = true;
    p->color = color;
}

void updateParticles(float dt)
{
    float drag = expf(-3.0f * dt);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle* p = &particles[i];
        if (p->life <= 0.0f) continue;
        p->life -= dt;
        p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
        p->velocity = Vector2Scale(p->velocity, drag);
    }
}

void drawParticles(void)
{
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle* p = &particles[i];
        if (p->life <= 0.0f) continue;
        float t = p->life / p->maxLife;
        if (p->ring) {
            float r = p->size * (1.0f - t * t);
            DrawRing(p->position, fmaxf(r - 6.0f * t, 0.0f), r, 0.0f, 360.0f, 48, Fade(p->color, t * 0.8f));
        } else {
            DrawCircleV(p->position, fmaxf(p->size * t, 0.75f), Fade(p->color, t));
        }
    }
    EndBlendMode();
}

void clearParticles(void)
{
    memset(particles, 0, sizeof(particles));
    nextParticle = 0;
}
