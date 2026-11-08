#include "globals.h"

static const char* PAUSE_MENU_LABELS[PAUSE_MENU_ITEMS] = { "RESUME", "RESTART", "QUIT TO TITLE", "EXIT GAME" };
static const Color WALL_COLOR = { 60, 140, 255, 255 };

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static Color lerpColor(Color a, Color b, float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t),
    };
}

static Color flashColor(const GameObject* o)
{
    return o->hitFlash > 0.0f ? WHITE : o->color;
}

static void drawTextCentered(const char* text, float cx, float y, int size, Color color)
{
    int w = MeasureText(text, size);
    DrawText(text, (int)(cx - (float)w * 0.5f), (int)y, size, color);
}

static void drawHealthBar(Vector2 center, float width, float fraction, Color color)
{
    Rectangle back = { center.x - width * 0.5f, center.y, width, 4.0f };
    DrawRectangleRec(back, Fade(BLACK, 0.6f));
    DrawRectangleRec((Rectangle){ back.x, back.y, back.width * Clamp(fraction, 0.0f, 1.0f), back.height }, color);
}

Rectangle pauseMenuItemRect(int index)
{
    return (Rectangle){ SCREEN_WIDTH * 0.5f - 160.0f, 290.0f + (float)index * 58.0f, 320.0f, 46.0f };
}

// ---------------------------------------------------------------------------
// Background
// ---------------------------------------------------------------------------
#define STAR_COUNT 220
typedef struct Star { Vector2 pos; float depth; float twinkle; } Star;
static Star stars[STAR_COUNT];
static bool starsReady = false;

static void drawStarfield(void)
{
    if (!starsReady) {
        for (int i = 0; i < STAR_COUNT; i++) {
            stars[i].pos = (Vector2){ randf(0.0f, (float)SCREEN_WIDTH), randf(0.0f, (float)SCREEN_HEIGHT) };
            stars[i].depth = randf(0.2f, 1.0f);
            stars[i].twinkle = randf(0.0f, 2.0f * PI);
        }
        starsReady = true;
    }

    DrawCircleGradient(300, 260, 380.0f, Fade((Color){ 90, 30, 150, 255 }, 0.20f), BLANK);
    DrawCircleGradient(1000, 520, 420.0f, Fade((Color){ 20, 90, 170, 255 }, 0.16f), BLANK);

    float t = (float)GetTime();
    for (int i = 0; i < STAR_COUNT; i++) {
        Star* s = &stars[i];
        float y = fmodf(s->pos.y + t * 14.0f * s->depth, (float)SCREEN_HEIGHT);
        float b = 0.5f + 0.5f * sinf(t * 2.0f + s->twinkle);
        float alpha = (0.25f + 0.6f * s->depth) * (0.6f + 0.4f * b);
        DrawCircleV((Vector2){ s->pos.x, y }, s->depth * 1.6f, Fade(WHITE, alpha));
    }
}

static Rectangle gateRect(int gate)
{
    Vector2 c = ARENA_CENTER;
    switch (gate) {
    case 0:  return (Rectangle){ c.x - GATE_WIDTH * 0.5f, ARENA_TOP - 10.0f, GATE_WIDTH, 12.0f };
    case 1:  return (Rectangle){ c.x - GATE_WIDTH * 0.5f, ARENA_BOTTOM - 2.0f, GATE_WIDTH, 12.0f };
    case 2:  return (Rectangle){ ARENA_LEFT - 10.0f, c.y - GATE_WIDTH * 0.5f, 12.0f, GATE_WIDTH };
    default: return (Rectangle){ ARENA_RIGHT - 2.0f, c.y - GATE_WIDTH * 0.5f, 12.0f, GATE_WIDTH };
    }
}

static void drawArena(void)
{
    Rectangle a = { ARENA_LEFT, ARENA_TOP, ARENA_RIGHT - ARENA_LEFT, ARENA_BOTTOM - ARENA_TOP };
    DrawRectangleRec(a, Fade((Color){ 10, 14, 32, 255 }, 0.85f));

    for (float x = a.x; x <= a.x + a.width; x += 48.0f) {
        DrawLineV((Vector2){ x, a.y }, (Vector2){ x, a.y + a.height }, Fade(SKYBLUE, 0.05f));
    }
    for (float y = a.y; y <= a.y + a.height; y += 48.0f) {
        DrawLineV((Vector2){ a.x, y }, (Vector2){ a.x + a.width, y }, Fade(SKYBLUE, 0.05f));
    }

    Rectangle wall = { a.x - 6.0f, a.y - 6.0f, a.width + 12.0f, a.height + 12.0f };
    DrawRectangleLinesEx((Rectangle){ wall.x - 4.0f, wall.y - 4.0f, wall.width + 8.0f, wall.height + 8.0f }, 6.0f, Fade(WALL_COLOR, 0.15f));
    DrawRectangleLinesEx(wall, 3.0f, Fade(WALL_COLOR, 0.9f));

    for (int g = 0; g < GATE_COUNT; g++) {
        Rectangle r = gateRect(g);
        float flash = Clamp(game.gateFlash[g] / 0.6f, 0.0f, 1.0f);
        Color c = lerpColor((Color){ 30, 50, 90, 255 }, (Color){ 255, 60, 80, 255 }, flash);
        DrawRectangleRec(r, (Color){ 6, 8, 18, 255 });
        DrawRectangleLinesEx(r, 2.0f, c);
        if (flash > 0.0f) DrawRectangleRec(r, Fade(c, 0.4f * flash));
    }
}

// ---------------------------------------------------------------------------
// Objects
// ---------------------------------------------------------------------------
static void drawStatic(const GameObject* o)
{
    Rectangle r = objectRect(o);
    DrawRectangleRec(r, (Color){ 18, 26, 50, 255 });
    DrawLineEx((Vector2){ r.x, r.y }, (Vector2){ r.x + r.width, r.y + r.height }, 1.0f, Fade(o->color, 0.25f));
    DrawLineEx((Vector2){ r.x + r.width, r.y }, (Vector2){ r.x, r.y + r.height }, 1.0f, Fade(o->color, 0.25f));
    DrawRectangleLinesEx(r, 2.0f, Fade(o->color, 0.85f));
}

static void drawDebris(const GameObject* o)
{
    float r = objectRadius(o);
    float deg = o->rotation * RAD2DEG;
    DrawPoly(o->position, 7, r, deg, o->hitFlash > 0.0f ? WHITE : (Color){ 55, 55, 70, 255 });
    DrawPolyLinesEx(o->position, 7, r, deg, 2.0f, (Color){ 150, 150, 175, 255 });
    Vector2 crater = Vector2Add(o->position, Vector2Rotate((Vector2){ r * 0.35f, 0.0f }, o->rotation));
    DrawCircleV(crater, r * 0.2f, (Color){ 40, 40, 52, 255 });
}

static void drawEnemy(const GameObject* e)
{
    float r = objectRadius(e);
    Color c = flashColor(e);
    float deg = e->rotation * RAD2DEG;
    Vector2 dir = { cosf(e->rotation), sinf(e->rotation) };

    switch (e->subtype) {
    case ENEMY_DRONE:
        DrawPoly(e->position, 3, r, deg, Fade(c, 0.35f));
        DrawPolyLinesEx(e->position, 3, r, deg, 2.0f, c);
        DrawCircleV(e->position, r * 0.3f, c);
        break;

    case ENEMY_GUNNER:
        DrawLineEx(e->position, Vector2Add(e->position, Vector2Scale(dir, r * 1.5f)), 5.0f, c);
        DrawPoly(e->position, 4, r, deg, Fade(c, 0.3f));
        DrawPolyLinesEx(e->position, 4, r, deg, 2.0f, c);
        DrawCircleV(e->position, r * 0.35f, Fade(c, 0.8f));
        break;

    case ENEMY_WARDEN:
        DrawPoly(e->position, 6, r, e->param * RAD2DEG, Fade(c, 0.25f));
        DrawPolyLinesEx(e->position, 6, r, e->param * RAD2DEG, 2.0f, c);
        DrawPolyLinesEx(e->position, 6, r * 0.55f, -e->param * RAD2DEG * 1.5f, 2.0f, c);
        DrawCircleV(e->position, r * (0.2f + 0.05f * sinf(e->timer * 8.0f)), WHITE);
        break;

    case ENEMY_HULK:
        DrawPoly(e->position, 8, r, deg, Fade(c, 0.35f));
        DrawPolyLinesEx(e->position, 8, r, deg, 4.0f, c);
        DrawRing(e->position, r * 0.42f, r * 0.55f, 0.0f, 360.0f, 24, c);
        break;

    case ENEMY_OVERMIND: {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleV(e->position, r * 1.5f, Fade(c, 0.10f));
        EndBlendMode();
        DrawPoly(e->position, 12, r * 0.9f, 0.0f, Fade(c, 0.18f));
        for (int k = 0; k < 3; k++) {
            float spin = e->timer * 40.0f * (k % 2 ? -1.0f : 1.0f);
            DrawPolyLinesEx(e->position, 5 + k, r * (1.0f - (float)k * 0.22f), spin, 3.0f, c);
        }
        DrawCircleV(e->position, r * 0.3f, (Color){ 20, 0, 20, 255 });
        DrawCircleV(Vector2Add(e->position, Vector2Scale(dir, r * 0.12f)), r * 0.14f, c);
    } break;

    default:
        DrawCircleV(e->position, r, c);
        break;
    }

    if (e->subtype != ENEMY_DRONE && e->subtype != ENEMY_OVERMIND && e->health < e->maxHealth) {
        drawHealthBar((Vector2){ e->position.x, e->position.y - r - 10.0f }, r * 2.0f, e->health / e->maxHealth, e->color);
    }
}

static void drawProjectileGlow(const GameObject* p)
{
    float r = objectRadius(p);
    switch (p->subtype) {
    case PROJ_BULLET:
        DrawLineEx(Vector2Subtract(p->position, Vector2Scale(p->velocity, 0.02f)), p->position, 4.0f, Fade(p->color, 0.8f));
        break;
    case PROJ_PLASMA:
        DrawCircleV(p->position, r * 2.2f, Fade(p->color, 0.25f));
        DrawCircleV(p->position, r * 1.3f, Fade(p->color, 0.6f));
        break;
    case PROJ_ENTANGLED:
        DrawCircleV(p->position, r * 2.0f, Fade(p->color, 0.35f));
        if (p->param == 0.0f && p->linkId >= 0) {
            GameObject* partner = findObjectById(&game.objects, p->linkId);
            if (partner && partner->active) DrawLineEx(p->position, partner->position, 2.0f, Fade(p->color, 0.35f));
        }
        break;
    case PROJ_SINGULARITY:
        DrawCircleV(p->position, r * 2.4f, Fade(p->color, 0.35f));
        break;
    case PROJ_BLACK_HOLE: {
        float life = 1.0f - p->timer / p->lifetime;
        DrawCircleV(p->position, 320.0f, Fade(p->color, 0.03f * life));
        for (int k = 0; k < 4; k++) {
            float start = p->rotation * RAD2DEG * (float)(k + 1) * 0.6f + (float)k * 90.0f;
            Color c = lerpColor(p->color, ORANGE, (float)k / 3.0f);
            DrawRing(p->position, r * (1.1f + (float)k * 0.35f), r * (1.25f + (float)k * 0.35f), start, start + 250.0f, 36, Fade(c, 0.3f));
        }
    } break;
    case PROJ_FLAME: {
        float t = p->timer / p->lifetime;
        Color c = t < 0.5f ? lerpColor((Color){ 255, 240, 140, 255 }, ORANGE, t * 2.0f) : lerpColor(ORANGE, (Color){ 120, 30, 20, 255 }, (t - 0.5f) * 2.0f);
        DrawCircleV(p->position, r, Fade(c, (1.0f - t) * 0.55f));
    } break;
    case PROJ_ROCKET:
        DrawCircleV(Vector2Subtract(p->position, Vector2Scale(Vector2Normalize(p->velocity), 9.0f)), 6.0f, Fade(ORANGE, 0.8f));
        break;
    case PROJ_ENEMY_BULLET:
        DrawCircleV(p->position, r * 1.8f, Fade(p->color, 0.25f));
        break;
    default:
        break;
    }
}

static void drawProjectileCore(const GameObject* p)
{
    float r = objectRadius(p);
    switch (p->subtype) {
    case PROJ_BULLET:
        DrawCircleV(p->position, 2.0f, WHITE);
        break;
    case PROJ_PLASMA:
    case PROJ_ENTANGLED:
        DrawCircleV(p->position, r * 0.55f, WHITE);
        break;
    case PROJ_SINGULARITY:
        DrawCircleV(p->position, r, BLACK);
        DrawRing(p->position, r, r + 2.0f, 0.0f, 360.0f, 24, p->color);
        break;
    case PROJ_BLACK_HOLE:
        DrawCircleV(p->position, r * 0.9f, BLACK);
        DrawRing(p->position, r * 0.9f, r, 0.0f, 360.0f, 48, Fade(WHITE, 0.7f));
        break;
    case PROJ_ROCKET: {
        float deg = atan2f(p->velocity.y, p->velocity.x) * RAD2DEG;
        DrawRectanglePro((Rectangle){ p->position.x, p->position.y, 16.0f, 6.0f }, (Vector2){ 8.0f, 3.0f }, deg, LIGHTGRAY);
        DrawCircleV(Vector2Add(p->position, Vector2Scale(Vector2Normalize(p->velocity), 8.0f)), 3.0f, p->color);
    } break;
    case PROJ_ENEMY_BULLET:
        DrawCircleV(p->position, r, p->color);
        DrawCircleV(p->position, r * 0.55f, WHITE);
        break;
    default:
        break;
    }
}

static void drawMeleeEffects(const GameObject* p)
{
    float r = objectRadius(p);
    Vector2 dir = game.aim;
    Vector2 perp = { -dir.y, dir.x };

    if (game.currentWeapon == WEAPON_CHAINSAW) {
        const WeaponInfo* info = &WEAPONS[WEAPON_CHAINSAW];
        float len = info->speed * 0.85f;
        Vector2 start = Vector2Add(p->position, Vector2Scale(dir, r * 0.6f));
        Vector2 end = Vector2Add(start, Vector2Scale(dir, len));
        DrawLineEx(start, end, 10.0f, (Color){ 90, 90, 100, 255 });
        float step = len / 8.0f;
        float offset = game.chainsawActive ? fmodf(game.time * 500.0f, step) : 0.0f;
        Color tooth = game.chainsawActive ? ORANGE : LIGHTGRAY;
        for (int k = 0; k < 8; k++) {
            Vector2 at = Vector2Add(start, Vector2Scale(dir, (float)k * step + offset));
            DrawCircleV(Vector2Add(at, Vector2Scale(perp, 6.0f)), 2.0f, tooth);
            DrawCircleV(Vector2Subtract(at, Vector2Scale(perp, 6.0f)), 2.0f, tooth);
        }
    }

    const WeaponInfo* sword = &WEAPONS[WEAPON_ENERGY_SWORD];
    if (game.swordSwingTimer > 0.0f) {
        float progress = 1.0f - game.swordSwingTimer / 0.2f;
        float from = game.swordSwingAngle - sword->spread;
        float sweep = from + 2.0f * sword->spread * progress;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRing(p->position, r, sword->speed, from * RAD2DEG, sweep * RAD2DEG, 32, Fade(sword->color, 0.35f));
        EndBlendMode();
        Vector2 d = { cosf(sweep), sinf(sweep) };
        DrawLineEx(Vector2Add(p->position, Vector2Scale(d, r)), Vector2Add(p->position, Vector2Scale(d, sword->speed)), 6.0f, sword->color);
        DrawLineEx(Vector2Add(p->position, Vector2Scale(d, r)), Vector2Add(p->position, Vector2Scale(d, sword->speed)), 2.0f, WHITE);
    } else if (game.currentWeapon == WEAPON_ENERGY_SWORD) {
        Vector2 d = Vector2Rotate(dir, 0.5f);
        Vector2 a = Vector2Add(p->position, Vector2Scale(d, r));
        Vector2 b = Vector2Add(p->position, Vector2Scale(d, sword->speed * 0.75f));
        DrawLineEx(a, b, 5.0f, Fade(sword->color, 0.8f));
        DrawLineEx(a, b, 2.0f, WHITE);
    }
}

static void drawPlayer(const GameObject* p)
{
    bool blink = game.invulnTimer > 0.0f && fmodf(game.time, 0.16f) < 0.08f;
    float alpha = blink ? 0.3f : 1.0f;
    float r = objectRadius(p);
    Vector2 dir = game.aim;
    Vector2 perp = { -dir.y, dir.x };
    Color body = p->hitFlash > 0.0f ? WHITE : p->color;
    const WeaponInfo* w = &WEAPONS[game.currentWeapon];

    drawMeleeEffects(p);

    if (!w->melee) {
        if (game.currentWeapon == WEAPON_DUAL_UZIS) {
            for (int s = -1; s <= 1; s += 2) {
                Vector2 base = Vector2Add(p->position, Vector2Scale(perp, (float)s * 11.0f));
                DrawLineEx(base, Vector2Add(base, Vector2Scale(dir, r + 6.0f)), 4.0f, Fade(w->color, alpha));
            }
        } else {
            float thick = game.currentWeapon == WEAPON_MINIGUN ? 8.0f : (game.currentWeapon == WEAPON_ROCKET_LAUNCHER ? 9.0f : 5.0f);
            DrawLineEx(p->position, Vector2Add(p->position, Vector2Scale(dir, r + 12.0f)), thick, Fade(w->color, alpha));
        }
    }

    Vector2 nose = Vector2Add(p->position, Vector2Scale(dir, r * 1.25f));
    Vector2 back = Vector2Subtract(p->position, Vector2Scale(dir, r * 0.8f));
    Vector2 wingA = Vector2Subtract(back, Vector2Scale(perp, r * 0.9f));
    Vector2 wingB = Vector2Add(back, Vector2Scale(perp, r * 0.9f));
    DrawTriangle(nose, wingA, wingB, Fade(body, 0.45f * alpha));
    DrawTriangleLines(nose, wingA, wingB, Fade(body, alpha));
    DrawCircleV(p->position, r * 0.3f, Fade(WHITE, 0.9f * alpha));

    if (game.shieldTimer > 0.0f) {
        float pulse = 0.6f + 0.3f * sinf(game.time * 10.0f);
        if (game.shieldTimer > 2.0f || fmodf(game.time, 0.2f) < 0.1f) {
            DrawCircleV(p->position, r + 10.0f, Fade(SKYBLUE, 0.08f));
            DrawRing(p->position, r + 10.0f, r + 13.0f, 0.0f, 360.0f, 36, Fade(SKYBLUE, pulse));
        }
    }
}

static void drawPickup(const GameObject* o)
{
    float remaining = o->lifetime - o->timer;
    if (remaining < 3.0f && fmodf(o->timer, 0.25f) < 0.12f) return; // about to vanish

    float bob = sinf(o->param * 4.0f) * 3.0f;
    Vector2 pos = { o->position.x, o->position.y + bob };
    float pulse = 0.5f + 0.5f * sinf(o->param * 6.0f);

    if (o->type == EXTRA_LIFE) {
        DrawRing(pos, 16.0f + pulse * 4.0f, 18.0f + pulse * 4.0f, 0.0f, 360.0f, 32, Fade(o->color, 0.6f));
        DrawTriangle((Vector2){ pos.x, pos.y - 10.0f }, (Vector2){ pos.x - 9.0f, pos.y + 8.0f }, (Vector2){ pos.x + 9.0f, pos.y + 8.0f }, o->color);
        drawTextCentered("1UP", pos.x, pos.y + 14.0f, 12, o->color);
        return;
    }

    Rectangle box = { pos.x - 15.0f, pos.y - 15.0f, 30.0f, 30.0f };
    const char* label = o->subtype < WEAPON_COUNT ? WEAPONS[o->subtype].shortName : (o->subtype == POWERUP_SHIELD ? "SH" : "HP");
    DrawRectangleRec(box, Fade(o->color, 0.2f + 0.15f * pulse));
    DrawRectangleLinesEx(box, 2.0f, o->color);
    drawTextCentered(label, pos.x, pos.y - 7.0f, 14, WHITE);
}

static void drawCursor(const GameObject* c)
{
    Color col = c->color;
    DrawRing(c->position, 9.0f, 11.0f, 0.0f, 360.0f, 24, Fade(col, 0.9f));
    for (int k = 0; k < 4; k++) {
        float a = c->rotation + (float)k * PI * 0.5f;
        Vector2 d = { cosf(a), sinf(a) };
        DrawLineEx(Vector2Add(c->position, Vector2Scale(d, 5.0f)), Vector2Add(c->position, Vector2Scale(d, 16.0f)), 2.0f, col);
    }
    DrawCircleV(c->position, 1.5f, WHITE);
}

static void drawTextured(const GameObject* o)
{
    Rectangle src = { 0.0f, 0.0f, (float)o->texture.width, (float)o->texture.height };
    Rectangle dst = { o->position.x, o->position.y, o->size.x, o->size.y };
    DrawTexturePro(o->texture, src, dst, (Vector2){ o->size.x * 0.5f, o->size.y * 0.5f }, 0.0f, WHITE);
}

static void drawLayer(GameObjectArray* arr, ObjectType type)
{
    for (int i = 0; i < arr->count; i++) {
        const GameObject* o = &arr->objects[i];
        if (!o->active || o->type != type) continue;
        if (o->texture.id != 0) { drawTextured(o); continue; }

        switch (type) {
        case OBJ_STATIC:  drawStatic(o); break;
        case OBJ_DYNAMIC: drawDebris(o); break;
        case ENEMY:       drawEnemy(o); break;
        case PLAYER:      drawPlayer(o); break;
        case POWER_UP:
        case EXTRA_LIFE:  drawPickup(o); break;
        default: break;
        }
    }
}

static void drawProjectiles(GameObjectArray* arr, bool playerOwned)
{
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < arr->count; i++) {
        const GameObject* o = &arr->objects[i];
        if (o->active && o->type == PROJECTILE && o->fromPlayer == playerOwned) drawProjectileGlow(o);
    }
    EndBlendMode();
    for (int i = 0; i < arr->count; i++) {
        const GameObject* o = &arr->objects[i];
        if (o->active && o->type == PROJECTILE && o->fromPlayer == playerOwned) drawProjectileCore(o);
    }
}

// ---------------------------------------------------------------------------
// HUD & overlays
// ---------------------------------------------------------------------------
static void drawHUD(void)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, HUD_HEIGHT, (Color){ 8, 10, 24, 240 });
    DrawLine(0, HUD_HEIGHT, SCREEN_WIDTH, HUD_HEIGHT, Fade(SKYBLUE, 0.4f));

    DrawText(TextFormat("SCORE %08d", game.score), 20, 10, 22, RAYWHITE);
    DrawText(TextFormat("WAVE %d", game.wave), 20, 38, 18, SKYBLUE);

    // Hull & ships
    GameObject* p = game.player;
    float hp = p ? Clamp(p->health / p->maxHealth, 0.0f, 1.0f) : 0.0f;
    Rectangle bar = { 230.0f, 12.0f, 240.0f, 14.0f };
    DrawRectangleRec(bar, (Color){ 30, 30, 45, 255 });
    DrawRectangleRec((Rectangle){ bar.x, bar.y, bar.width * hp, bar.height }, hp > 0.3f ? (Color){ 80, 220, 140, 255 } : RED);
    DrawRectangleLinesEx(bar, 1.0f, GRAY);
    DrawText("HULL", (int)bar.x, 34, 14, GRAY);
    for (int i = 0; i < game.lives && i < 10; i++) {
        float x = bar.x + 50.0f + (float)i * 18.0f;
        DrawTriangle((Vector2){ x, 33.0f }, (Vector2){ x - 6.0f, 47.0f }, (Vector2){ x + 6.0f, 47.0f }, (Color){ 80, 200, 255, 255 });
    }
    if (game.shieldTimer > 0.0f) DrawText(TextFormat("SHIELD %.1f", game.shieldTimer), (int)bar.x + 150, 34, 14, SKYBLUE);

    // Weapon slots
    for (int w = 0; w < WEAPON_COUNT; w++) {
        Rectangle slot = { 500.0f + (float)w * 42.0f, 10.0f, 38.0f, 40.0f };
        bool owned = weaponAvailable((WeaponType)w);
        bool current = (game.currentWeapon == (WeaponType)w);
        Color c = owned ? WEAPONS[w].color : (Color){ 60, 60, 75, 255 };
        if (current) DrawRectangleRec(slot, Fade(c, 0.3f));
        DrawRectangleLinesEx(slot, current ? 2.0f : 1.0f, c);
        DrawText(TextFormat("%d", (w + 1) % 10), (int)slot.x + 3, (int)slot.y + 2, 10, Fade(c, 0.8f));
        drawTextCentered(WEAPONS[w].shortName, slot.x + slot.width * 0.5f, slot.y + 18.0f, 14, owned ? WHITE : (Color){ 70, 70, 85, 255 });
    }

    // Current weapon & ammo
    const WeaponInfo* info = &WEAPONS[game.currentWeapon];
    int ammo = game.ammo[game.currentWeapon];
    const char* ammoText = ammo < 0 ? "AMMO INF" : TextFormat("AMMO %d", ammo);
    DrawText(info->name, SCREEN_WIDTH - 20 - MeasureText(info->name, 20), 10, 20, info->color);
    DrawText(ammoText, SCREEN_WIDTH - 20 - MeasureText(ammoText, 18), 36, 18, RAYWHITE);
    if (game.currentWeapon == WEAPON_MINIGUN) {
        DrawRectangle(SCREEN_WIDTH - 220, 40, (int)(80.0f * game.minigunSpin), 8, info->color);
        DrawRectangleLines(SCREEN_WIDTH - 220, 40, 80, 8, GRAY);
    }

    // Boss bar
    GameObject* boss = findFirstEnemyOfKind(ENEMY_OVERMIND);
    if (boss) {
        Rectangle bb = { SCREEN_WIDTH * 0.5f - 300.0f, ARENA_TOP + 10.0f, 600.0f, 12.0f };
        DrawRectangleRec(bb, Fade(BLACK, 0.6f));
        DrawRectangleRec((Rectangle){ bb.x, bb.y, bb.width * Clamp(boss->health / boss->maxHealth, 0.0f, 1.0f), bb.height }, boss->color);
        DrawRectangleLinesEx(bb, 1.0f, boss->color);
        drawTextCentered(ENEMIES[ENEMY_OVERMIND].name, SCREEN_WIDTH * 0.5f, bb.y + 16.0f, 16, boss->color);
    }

    // Center message
    if (game.messageTimer > 0.0f) {
        float a = fminf(1.0f, game.messageTimer / 0.4f);
        drawTextCentered(game.message, SCREEN_WIDTH * 0.5f, 150.0f, 34, Fade(game.messageColor, a));
    }
    if (game.state == STATE_PLAYING && game.waveState == WAVE_INTERMISSION && game.wave > 0) {
        drawTextCentered(TextFormat("NEXT WAVE IN %d", (int)ceilf(game.waveTimer)), SCREEN_WIDTH * 0.5f, 195.0f, 20, GRAY);
    }
}

static void drawTitle(void)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
    float cx = SCREEN_WIDTH * 0.5f;
    float t = (float)GetTime();

    drawTextCentered("PROJECT ERIS", cx + 3.0f, 113.0f, 80, Fade((Color){ 255, 60, 200, 255 }, 0.6f));
    drawTextCentered("PROJECT ERIS", cx, 110.0f, 80, (Color){ 120, 220, 255, 255 });
    drawTextCentered("QUANTUM ARENA SURVIVAL", cx, 200.0f, 24, RAYWHITE);

    const char* lines[] = {
        "WASD  move",
        "MOUSE aim / LMB fire      ARROWS fire in 8 directions",
        "1-0 / WHEEL / Q E  switch weapons",
        "ESC / P  pause",
        "",
        "Grab weapon drops: plasma, quantum entangler, singularity launcher,",
        "flamethrower, rockets, minigun, dual uzis, chainsaw, energy sword.",
        "The energy sword deflects enemy fire. Black holes swallow it.",
    };
    for (int i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++) {
        drawTextCentered(lines[i], cx, 270.0f + (float)i * 28.0f, 20, (Color){ 170, 180, 210, 255 });
    }

    if (fmodf(t, 1.0f) < 0.65f) drawTextCentered("CLICK OR PRESS ENTER TO DEPLOY", cx, 560.0f, 28, YELLOW);
    drawTextCentered("ESC to exit", cx, 610.0f, 16, GRAY);
}

static void drawPauseMenu(void)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));
    drawTextCentered("PAUSED", SCREEN_WIDTH * 0.5f, 180.0f, 64, RAYWHITE);

    for (int i = 0; i < PAUSE_MENU_ITEMS; i++) {
        Rectangle r = pauseMenuItemRect(i);
        bool selected = (game.menuSelection == i);
        Color c = selected ? (Color){ 120, 220, 255, 255 } : GRAY;
        DrawRectangleRec(r, Fade(c, selected ? 0.25f : 0.08f));
        DrawRectangleLinesEx(r, selected ? 2.0f : 1.0f, c);
        drawTextCentered(PAUSE_MENU_LABELS[i], r.x + r.width * 0.5f, r.y + 12.0f, 24, selected ? WHITE : LIGHTGRAY);
    }
    drawTextCentered("ESC / P to resume", SCREEN_WIDTH * 0.5f, 540.0f, 18, GRAY);
}

static void drawGameOver(void)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.65f));
    float cx = SCREEN_WIDTH * 0.5f;
    drawTextCentered("SIGNAL LOST", cx, 200.0f, 72, RED);
    drawTextCentered(TextFormat("FINAL SCORE  %08d", game.score), cx, 300.0f, 30, RAYWHITE);
    drawTextCentered(TextFormat("REACHED WAVE %d", game.wave), cx, 345.0f, 24, SKYBLUE);
    if (game.stateTimer > 1.0f) {
        drawTextCentered("ENTER / CLICK to redeploy      ESC for title", cx, 440.0f, 22, YELLOW);
    }
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
void draw(GameObjectArray* arr)
{
    BeginDrawing();
    ClearBackground((Color){ 5, 5, 14, 255 });

    drawStarfield();

    Camera2D cam = { 0 };
    cam.zoom = 1.0f;
    if (game.state == STATE_PLAYING && game.shake > 0.0f) {
        cam.offset = (Vector2){ randf(-game.shake, game.shake) * 0.6f, randf(-game.shake, game.shake) * 0.6f };
    }

    BeginMode2D(cam);
    drawArena();
    drawLayer(arr, OBJ_STATIC);
    drawLayer(arr, POWER_UP);
    drawLayer(arr, EXTRA_LIFE);
    drawLayer(arr, OBJ_DYNAMIC);
    drawLayer(arr, ENEMY);
    drawProjectiles(arr, true);
    drawParticles();
    if (game.state != STATE_GAMEOVER) drawLayer(arr, PLAYER);
    drawProjectiles(arr, false); // enemy fire on top for readability
    EndMode2D();

    drawHUD();

    switch (game.state) {
    case STATE_MENU:     drawTitle(); break;
    case STATE_PAUSED:   drawPauseMenu(); break;
    case STATE_GAMEOVER: drawGameOver(); break;
    default: break;
    }

    for (int i = 0; i < arr->count; i++) {
        if (arr->objects[i].active && arr->objects[i].type == CURSOR) drawCursor(&arr->objects[i]);
    }

    EndDrawing();
}
