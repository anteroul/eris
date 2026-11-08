#include "globals.h"

#define PICKUP_MAGNET_RADIUS 110.0f

// Shared pickup logic: ageing, despawn, magnet pull. Returns true when collected.
static bool updatePickup(GameObject* p)
{
    float dt = game.dt;
    p->timer += dt;
    p->param += dt;
    if (p->lifetime > 0.0f && p->timer >= p->lifetime) {
        p->active = false;
        return false;
    }

    GameObject* player = game.player;
    if (!player) return false;

    Vector2 to = Vector2Subtract(player->position, p->position);
    float d = Vector2Length(to);
    if (d < PICKUP_MAGNET_RADIUS && d > 0.001f) {
        p->position = Vector2Add(p->position, Vector2Scale(to, 240.0f * dt / d));
    }
    return d < objectRadius(p) + objectRadius(player);
}

void powerUpBehavior(GameObject* powerUp)
{
    if (powerUp->type != POWER_UP) return;
    if (!updatePickup(powerUp)) return;

    GameObject* player = game.player;
    if (powerUp->subtype < WEAPON_COUNT) {
        giveWeapon((WeaponType)powerUp->subtype);
    } else if (powerUp->subtype == POWERUP_SHIELD) {
        game.shieldTimer = 8.0f;
        showMessage("PHASE SHIELD ONLINE", powerUp->color, 1.8f);
    } else if (powerUp->subtype == POWERUP_REPAIR) {
        player->health = fminf(player->maxHealth, player->health + 50.0f);
        showMessage("HULL REPAIRED", powerUp->color, 1.5f);
    }

    game.score += 50;
    spawnParticles(powerUp->position, 16, powerUp->color, 200.0f, 0.4f, 3.0f);
    spawnShockwave(powerUp->position, 50.0f, powerUp->color);
    powerUp->active = false;
}

void extraLifeBehavior(GameObject* extraLife)
{
    if (extraLife->type != EXTRA_LIFE) return;
    if (!updatePickup(extraLife)) return;

    game.lives++;
    game.score += 100;
    showMessage("EXTRA SHIP", extraLife->color, 1.8f);
    spawnParticles(extraLife->position, 24, extraLife->color, 240.0f, 0.5f, 3.0f);
    spawnShockwave(extraLife->position, 80.0f, extraLife->color);
    extraLife->active = false;
}
