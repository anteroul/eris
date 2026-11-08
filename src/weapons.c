#include "globals.h"

// For melee weapons `speed` is the reach and `spread` is the half arc.
// Flamethrower and chainsaw damage is per second of contact.
const WeaponInfo WEAPONS[WEAPON_COUNT] = {
    [WEAPON_AR15]            = { "AR-15",                "AR", 0.110f,  14.0f,  950.0f, 0.035f,  -1, false, { 255, 225, 120, 255 } },
    [WEAPON_PLASMA_RIFLE]    = { "Plasma Rifle",         "PL", 0.220f,  32.0f,  720.0f, 0.000f,  60, false, {  90, 230, 255, 255 } },
    [WEAPON_Q_ENTANGLER]     = { "Quantum Entangler",    "QE", 0.160f,  18.0f,  640.0f, 0.020f,  80, false, { 255,  90, 230, 255 } },
    [WEAPON_SINGULARITY]     = { "Singularity Launcher", "BH", 1.000f,   0.0f,  380.0f, 0.000f,   1, false, { 170, 110, 255, 255 } },
    [WEAPON_FLAMETHROWER]    = { "Flamethrower",         "FL", 0.025f,  55.0f,  430.0f, 0.220f, 300, false, { 255, 140,  40, 255 } },
    [WEAPON_ROCKET_LAUNCHER] = { "Rocket Launcher",      "RL", 0.700f,  95.0f,  420.0f, 0.000f,  12, false, { 255,  80,  60, 255 } },
    [WEAPON_MINIGUN]         = { "Minigun",              "MG", 0.035f,   9.0f, 1050.0f, 0.110f, 400, false, { 255, 200,  60, 255 } },
    [WEAPON_DUAL_UZIS]       = { "Dual Uzis",            "UZ", 0.060f,   8.0f,  880.0f, 0.140f, 250, false, { 220, 220, 235, 255 } },
    [WEAPON_CHAINSAW]        = { "Chainsaw",             "CS", 0.000f, 170.0f,   78.0f, 0.600f,  -1, true,  { 210, 210, 210, 255 } },
    [WEAPON_ENERGY_SWORD]    = { "Energy Sword",         "ES", 0.350f, 110.0f,  105.0f, 1.300f,  -1, true,  { 120, 255, 200, 255 } },
};

#define SWORD_SWING_TIME 0.2f

// ---------------------------------------------------------------------------
// Inventory
// ---------------------------------------------------------------------------
void resetWeapons(void)
{
    memset(game.owned, 0, sizeof(game.owned));
    memset(game.ammo, 0, sizeof(game.ammo));
    game.owned[WEAPON_AR15] = true;
    game.ammo[WEAPON_AR15] = -1;
    game.currentWeapon = WEAPON_AR15;
    game.fireCooldown = 0.0f;
    game.minigunSpin = 0.0f;
    game.chainsawActive = false;
    game.swordSwingTimer = 0.0f;
    game.swingId = 0;
}

bool weaponAvailable(WeaponType weapon)
{
    if (weapon < 0 || weapon >= WEAPON_COUNT) return false;
    return game.owned[weapon] && game.ammo[weapon] != 0;
}

void selectWeapon(WeaponType weapon)
{
    if (!weaponAvailable(weapon) || weapon == game.currentWeapon) return;
    game.currentWeapon = weapon;
    game.minigunSpin = 0.0f;
}

void cycleWeapon(int direction)
{
    for (int i = 1; i < WEAPON_COUNT; i++) {
        int w = (((int)game.currentWeapon + direction * i) % WEAPON_COUNT + WEAPON_COUNT) % WEAPON_COUNT;
        if (weaponAvailable((WeaponType)w)) {
            selectWeapon((WeaponType)w);
            return;
        }
    }
}

void giveWeapon(WeaponType weapon)
{
    const WeaponInfo* info = &WEAPONS[weapon];
    if (info->pickupAmmo < 0) {
        game.ammo[weapon] = -1;
    } else {
        int current = (game.owned[weapon] && game.ammo[weapon] > 0) ? game.ammo[weapon] : 0;
        game.ammo[weapon] = current + info->pickupAmmo;
    }
    game.owned[weapon] = true;
    game.currentWeapon = weapon;
    showMessage(TextFormat("%s ACQUIRED", info->name), info->color, 1.8f);
}

static void consumeAmmo(int amount)
{
    WeaponType w = game.currentWeapon;
    if (game.ammo[w] < 0) return;

    game.ammo[w] -= amount;
    if (game.ammo[w] <= 0) {
        game.ammo[w] = 0;
        game.owned[w] = false;
        game.currentWeapon = WEAPON_AR15;
        showMessage(TextFormat("%s DEPLETED", WEAPONS[w].name), GRAY, 1.5f);
    }
}

// ---------------------------------------------------------------------------
// Projectiles
// ---------------------------------------------------------------------------
GameObject* fireProjectile(Vector2 origin, Vector2 direction, ProjectileKind kind, float speed, float damage, float radius, Color color, bool fromPlayer)
{
    GameObject p = makeGameObject(PROJECTILE, origin, (Vector2){ radius * 2.0f, radius * 2.0f }, projectileBehavior);
    p.subtype = kind;
    p.velocity = Vector2Scale(direction, speed);
    p.damage = damage;
    p.color = color;
    p.fromPlayer = fromPlayer;
    p.rotation = atan2f(direction.y, direction.x);
    p.lifetime = 3.0f;
    p.anchor = origin;
    return spawnGameObject(p);
}

static void muzzleFlash(Vector2 muzzle, Color color, int count)
{
    spawnParticles(muzzle, count, color, 160.0f, 0.12f, 2.5f);
}

// ---------------------------------------------------------------------------
// Melee
// ---------------------------------------------------------------------------
static bool inArc(Vector2 origin, float facing, float halfArc, float reach, const GameObject* target, Vector2* dirOut)
{
    Vector2 to = Vector2Subtract(target->position, origin);
    float dist = Vector2Length(to);
    if (dist - objectRadius(target) > reach) return false;
    if (dist > 0.001f && fabsf(angleDiff(facing, atan2f(to.y, to.x))) > halfArc) return false;
    if (dirOut) *dirOut = dist > 0.001f ? Vector2Scale(to, 1.0f / dist) : (Vector2){ cosf(facing), sinf(facing) };
    return true;
}

static void updateChainsaw(GameObject* player, const WeaponInfo* info)
{
    float facing = atan2f(game.aim.y, game.aim.x);
    bool hit = false;

    for (int i = 0; i < game.objects.count; i++) {
        GameObject* o = &game.objects.objects[i];
        if (!o->active) continue;
        Vector2 dir;

        if ((o->type == ENEMY || o->type == OBJ_DYNAMIC) && inArc(player->position, facing, info->spread, info->speed, o, &dir)) {
            damageObject(o, info->damage * game.dt, Vector2Scale(dir, 25.0f));
            hit = true;
        } else if (o->type == PROJECTILE && !o->fromPlayer && inArc(player->position, facing, info->spread, info->speed, o, NULL)) {
            o->active = false; // the spinning chain shreds incoming fire
        }
    }

    game.shake = fmaxf(game.shake, hit ? 2.5f : 1.0f);
    if (hit && GetRandomValue(0, 2) == 0) {
        Vector2 tip = Vector2Add(player->position, Vector2Scale(game.aim, info->speed * 0.8f));
        spawnParticles(tip, 3, (Color){ 255, 120, 60, 255 }, 260.0f, 0.25f, 2.5f);
    }
}

static void updateSwordSwing(GameObject* player, const WeaponInfo* info)
{
    for (int i = 0; i < game.objects.count; i++) {
        GameObject* o = &game.objects.objects[i];
        if (!o->active) continue;
        Vector2 dir;

        if ((o->type == ENEMY || o->type == OBJ_DYNAMIC) && o->hitTag != game.swingId &&
            inArc(player->position, game.swordSwingAngle, info->spread, info->speed, o, &dir)) {
            o->hitTag = game.swingId;
            damageObject(o, info->damage, Vector2Scale(dir, 380.0f));
            spawnParticles(o->position, 8, info->color, 240.0f, 0.3f, 3.0f);
            addShake(3.0f);
        } else if (o->type == PROJECTILE && !o->fromPlayer &&
                   inArc(player->position, game.swordSwingAngle, info->spread, info->speed + 10.0f, o, &dir)) {
            // Deflect enemy fire back into the arena.
            float speed = Vector2Length(o->velocity) * 1.4f + 120.0f;
            o->velocity = Vector2Scale(dir, speed);
            o->fromPlayer = true;
            o->damage = 25.0f;
            o->color = info->color;
            o->timer = 0.0f;
            o->lifetime = 1.5f;
        }
    }
}

// ---------------------------------------------------------------------------
// Firing
// ---------------------------------------------------------------------------
void updatePlayerWeapon(GameObject* player)
{
    float dt = game.dt;
    WeaponType w = game.currentWeapon;
    const WeaponInfo* info = &WEAPONS[w];
    Vector2 dir = game.aim;
    Vector2 perp = { -dir.y, dir.x };
    Vector2 muzzle = Vector2Add(player->position, Vector2Scale(dir, objectRadius(player) + 10.0f));
    bool trigger = game.firing;

    if (game.fireCooldown > 0.0f) game.fireCooldown -= dt;

    if (w == WEAPON_MINIGUN && trigger) game.minigunSpin = fminf(1.0f, game.minigunSpin + dt / 0.7f);
    else game.minigunSpin = fmaxf(0.0f, game.minigunSpin - dt / 0.5f);

    game.chainsawActive = (w == WEAPON_CHAINSAW && trigger);
    if (game.chainsawActive) updateChainsaw(player, info);

    if (game.swordSwingTimer > 0.0f) {
        game.swordSwingTimer -= dt;
        updateSwordSwing(player, &WEAPONS[WEAPON_ENERGY_SWORD]);
    }

    if (!trigger || game.fireCooldown > 0.0f) return;

    GameObject* p = NULL;
    switch (w) {
    case WEAPON_AR15:
        p = fireProjectile(muzzle, Vector2Rotate(dir, randf(-info->spread, info->spread)), PROJ_BULLET, info->speed, info->damage, 3.0f, info->color, true);
        if (p) p->lifetime = 1.2f;
        muzzleFlash(muzzle, info->color, 3);
        game.fireCooldown = info->fireInterval;
        break;

    case WEAPON_PLASMA_RIFLE:
        p = fireProjectile(muzzle, dir, PROJ_PLASMA, info->speed, info->damage, 7.0f, info->color, true);
        if (p) { p->pierce = 3; p->lifetime = 1.6f; }
        muzzleFlash(muzzle, info->color, 6);
        game.fireCooldown = info->fireInterval;
        consumeAmmo(1);
        break;

    case WEAPON_Q_ENTANGLER:
        // Two entangled quanta spiral around a shared path in superposition.
        for (int k = 0; k < 2; k++) {
            p = fireProjectile(muzzle, Vector2Rotate(dir, randf(-info->spread, info->spread)), PROJ_ENTANGLED, info->speed, info->damage, 5.0f, info->color, true);
            if (p) { p->param = (float)k * PI; p->lifetime = 1.6f; }
        }
        if (game.pending.count >= 2) {
            GameObject* a = &game.pending.objects[game.pending.count - 2];
            GameObject* b = &game.pending.objects[game.pending.count - 1];
            a->linkId = b->id;
            b->linkId = a->id;
        }
        muzzleFlash(muzzle, info->color, 5);
        game.fireCooldown = info->fireInterval;
        consumeAmmo(1);
        break;

    case WEAPON_SINGULARITY:
        p = fireProjectile(muzzle, dir, PROJ_SINGULARITY, info->speed, 0.0f, 10.0f, info->color, true);
        if (p) p->lifetime = 0.9f;
        muzzleFlash(muzzle, info->color, 14);
        addShake(4.0f);
        game.fireCooldown = info->fireInterval;
        consumeAmmo(1);
        break;

    case WEAPON_FLAMETHROWER:
        for (int k = 0; k < 2; k++) {
            Vector2 d = Vector2Rotate(dir, randf(-info->spread, info->spread));
            p = fireProjectile(muzzle, d, PROJ_FLAME, info->speed * randf(0.75f, 1.15f), info->damage, 7.0f, info->color, true);
            if (p) {
                p->velocity = Vector2Add(p->velocity, Vector2Scale(player->velocity, 0.5f));
                p->lifetime = randf(0.35f, 0.5f);
            }
        }
        game.fireCooldown = info->fireInterval;
        consumeAmmo(1);
        break;

    case WEAPON_ROCKET_LAUNCHER:
        p = fireProjectile(muzzle, dir, PROJ_ROCKET, info->speed, info->damage, 6.0f, info->color, true);
        if (p) p->lifetime = 2.5f;
        muzzleFlash(muzzle, ORANGE, 10);
        addShake(3.0f);
        game.fireCooldown = info->fireInterval;
        consumeAmmo(1);
        break;

    case WEAPON_MINIGUN:
        if (game.minigunSpin < 0.3f) break; // barrels still spinning up
        p = fireProjectile(muzzle, Vector2Rotate(dir, randf(-info->spread, info->spread)), PROJ_BULLET, info->speed, info->damage, 3.0f, info->color, true);
        if (p) p->lifetime = 1.0f;
        muzzleFlash(muzzle, info->color, 2);
        game.shake = fmaxf(game.shake, 1.5f);
        game.fireCooldown = Lerp(0.12f, info->fireInterval, game.minigunSpin);
        consumeAmmo(1);
        break;

    case WEAPON_DUAL_UZIS: {
        float side = game.uziLeft ? -1.0f : 1.0f;
        Vector2 origin = Vector2Add(muzzle, Vector2Scale(perp, side * 11.0f));
        p = fireProjectile(origin, Vector2Rotate(dir, randf(-info->spread, info->spread)), PROJ_BULLET, info->speed, info->damage, 2.5f, info->color, true);
        if (p) p->lifetime = 0.9f;
        muzzleFlash(origin, info->color, 2);
        game.uziLeft = !game.uziLeft;
        game.fireCooldown = info->fireInterval;
        consumeAmmo(1);
    } break;

    case WEAPON_CHAINSAW:
        break; // continuous damage handled above

    case WEAPON_ENERGY_SWORD:
        game.swingId++;
        game.swordSwingTimer = SWORD_SWING_TIME;
        game.swordSwingAngle = atan2f(dir.y, dir.x);
        game.fireCooldown = info->fireInterval;
        spawnParticles(muzzle, 6, info->color, 200.0f, 0.2f, 2.0f);
        break;

    default:
        break;
    }
}
