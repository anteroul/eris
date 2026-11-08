#include <raylib.h>
#include <resource_dir.h>
#include "globals.h"

Game game = { 0 };

void exitGame(void)
{
    game.quit = true;
}

static void setState(GameState state)
{
    game.state = state;
    game.stateTimer = 0.0f;
    game.menuSelection = 0;
}

// ---------------------------------------------------------------------------
// State machine input (title, pause, game over)
// ---------------------------------------------------------------------------
static void activatePauseItem(int index)
{
    switch (index) {
    case 0: setState(STATE_PLAYING); break;
    case 1: resetGame(); setState(STATE_PLAYING); break;
    case 2: resetGame(); setState(STATE_MENU); break;
    case 3: exitGame(); break;
    default: break;
    }
}

static bool confirmPressed(void)
{
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
}

static void handleStateInput(void)
{
    switch (game.state) {
    case STATE_MENU:
        if (confirmPressed() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            resetGame();
            setState(STATE_PLAYING);
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            exitGame();
        }
        break;

    case STATE_PLAYING:
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || !IsWindowFocused()) {
            setState(STATE_PAUSED);
        }
        break;

    case STATE_PAUSED: {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
            setState(STATE_PLAYING);
            break;
        }
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) game.menuSelection = (game.menuSelection + PAUSE_MENU_ITEMS - 1) % PAUSE_MENU_ITEMS;
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) game.menuSelection = (game.menuSelection + 1) % PAUSE_MENU_ITEMS;

        Vector2 mouse = GetMousePosition();
        bool mouseMoved = Vector2Length(GetMouseDelta()) > 0.0f;
        for (int i = 0; i < PAUSE_MENU_ITEMS; i++) {
            if (!CheckCollisionPointRec(mouse, pauseMenuItemRect(i))) continue;
            if (mouseMoved) game.menuSelection = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                activatePauseItem(i);
                return;
            }
        }
        if (confirmPressed()) activatePauseItem(game.menuSelection);
    } break;

    case STATE_GAMEOVER:
        if (game.stateTimer < 1.0f) break;
        if (confirmPressed() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            resetGame();
            setState(STATE_PLAYING);
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            resetGame();
            setState(STATE_MENU);
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// Game Logic
// ---------------------------------------------------------------------------
void update(GameObjectArray* arr)
{
    float frame = GetFrameTime();
    if (frame > 1.0f / 30.0f) frame = 1.0f / 30.0f; // avoid tunnelling after hitches

    // The cursor runs in every state so menus stay usable.
    for (int i = 0; i < arr->count; i++) {
        GameObject* obj = &arr->objects[i];
        if (obj->active && obj->type == CURSOR && obj->script) obj->script(obj);
    }

    game.stateTimer += frame;

    if (game.state == STATE_GAMEOVER) {
        updateParticles(frame); // let the final explosion play out
        return;
    }
    if (game.state != STATE_PLAYING) {
        game.dt = 0.0f; // paused: the world is frozen
        return;
    }

    game.dt = frame;
    game.time += frame;
    game.player = findObjectById(arr, game.playerId);

    // Spawns go to game.pending, so the array (and pointers into it) stay stable here.
    int count = arr->count;
    for (int i = 0; i < count; i++) {
        GameObject* obj = &arr->objects[i];
        if (!obj->active || !obj->script || obj->type == CURSOR) continue;
        obj->script(obj);
    }

    flushPendingObjects();
    resolveCollisions();
    flushPendingObjects();
    removeInactiveObjects(arr);
    game.player = findObjectById(arr, game.playerId);
    updateWaves();
    flushPendingObjects();

    updateParticles(frame);
    game.shake = fmaxf(0.0f, game.shake - frame * 30.0f);
    if (game.messageTimer > 0.0f) game.messageTimer -= frame;
    for (int g = 0; g < GATE_COUNT; g++) {
        if (game.gateFlash[g] > 0.0f) game.gateFlash[g] -= frame;
    }
}

// ---------------------------------------------------------------------------
// Entry Point
// ---------------------------------------------------------------------------
int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Project Eris");
    SetExitKey(KEY_NULL); // ESC pauses instead of closing the window
    HideCursor();
    SearchAndSetResourceDir("resources");

    initGameObjects(&game.objects, 512);
    initGameObjects(&game.pending, 128);

    resetGame(); // populate the arena behind the title screen
    setState(STATE_MENU);

    while (!WindowShouldClose() && !game.quit)
    {
        handleStateInput();
        update(&game.objects);
        draw(&game.objects);
    }

    freeGameObjects(&game.objects);
    freeGameObjects(&game.pending);
    CloseWindow();

    return 0;
}
