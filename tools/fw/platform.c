#include "platform.h"

#include "plat_physics.h"

#include <stddef.h>
#include <stdint.h>

#define PLAT_MOVE_FPS 15
#define PLAT_GRAVITY PLAT_PHYS_GRAVITY
#define PLAT_MAX_FALL PLAT_PHYS_MAX_FALL
/*
 * Scripted jump tables match docs/firmware.md (Platform section). Ascent waits in
 * plat_jump_up_waits, descent in plat_jump_down_waits. While idle on a row (timer
 * expired), if ascent rise is at least PLAT_JUMP_MIN_RISE and button is not held,
 * jump_begin_descent_from_apex picks the descent slice so mirrored timing matches apex
 * height. Walk uses PLAT_MOVE_FPS. Fall outside the script uses gravity and vert_accum.
 */
#define PLAT_JUMP_UP_WAITS_LEN 4
static const uint8_t plat_jump_up_waits[PLAT_JUMP_UP_WAITS_LEN] = {1, 2, 3, 4};
#define PLAT_JUMP_DOWN_WAITS_LEN 5
static const uint8_t plat_jump_down_waits[PLAT_JUMP_DOWN_WAITS_LEN] = {4, 3, 2, 1, 0};
/* Minimum rise is 2 cells above jump_base_py (3 rows including ground before first step). */
#define PLAT_JUMP_MIN_RISE 2
#define PLAT_JUMP_MAX_RISE 5

static int body_blocked(const StageView *view, int x, int y)
{
    if (view == 0 || view->map == 0)
        return 1;
    if (x < 0 || y < 0 || x >= (int)view->map->width || y >= (int)view->map->height)
        return 1;
    return stage_h_solid(view, x, y);
}

static int on_ground(const StageView *view, int px, int py)
{
    if (py + 1 >= (int)view->map->height)
        return 1;
    return body_blocked(view, px, py + 1);
}

static int bump_ceiling_hook(StageView *view, int x, int y, void *ctx)
{
    (void)ctx;
    return stage_h_item_bump(view, x, y) == STAGE_BUMP_COIN ? 1 : 0;
}

static void camera_follow(StageView *view, int px)
{
    stage_h_camera_follow_dead_zone(view, px);
}

static void try_walk(PlatformPlayer *player, StageView *view, int dx)
{
    int nx;

    if (player == 0 || view == 0 || dx == 0)
        return;
    player->walk_accum += PLAT_MOVE_FPS;
    while (player->walk_accum >= STAGE_TICK_HZ) {
        player->walk_accum -= STAGE_TICK_HZ;
        nx = player->px + dx;
        if (!body_blocked(view, nx, player->py))
            player->px = nx;
    }
}

static void apply_gravity(PlatformPlayer *player)
{
    plat_phys_apply_gravity(&player->vy);
}

static void move_vertical(PlatformPlayer *player, StageView *view)
{
    PlatBody b;

    b.px = player->px;
    b.py = player->py;
    b.vy = player->vy;
    b.walk_accum = 0;
    b.vert_accum = player->vert_accum;
    b.grounded = player->grounded;
    plat_phys_move_vertical(&b, view, bump_ceiling_hook, NULL, &player->item_pickups, NULL, view);
    player->py = b.py;
    player->vy = b.vy;
    player->vert_accum = b.vert_accum;
    player->grounded = b.grounded;
}

/* Try one scripted jump step up (−1) or down (+1). Returns 0 if blocked (caller clears script). */
static int jump_script_try_step(PlatformPlayer *player, StageView *view, int dy)
{
    int ny = player->py + dy;

    if (body_blocked(view, player->px, ny)) {
        if (dy > 0)
            player->grounded = 1;
        else {
            StageBumpResult br;

            br = stage_h_item_bump(view, player->px, ny);
            if (br == STAGE_BUMP_COIN)
                player->item_pickups++;
        }
        player->vy = 0;
        return 0;
    }
    player->py = ny;
    player->grounded = on_ground(view, player->px, player->py);
    return 1;
}

/* rise = cells cleared above jump_base_py (>= 1 after first step). */
static int jump_rise_cells(const PlatformPlayer *player)
{
    return player->jump_base_py - player->py;
}

/*
 * Enter descent at the current apex. plat_jump_down_waits[k] is the idle time on the row
 * after (PLAT_JUMP_MAX_RISE - k) upward steps. jump_down_i indexes the next down step.
 */
static void jump_begin_descent_from_apex(PlatformPlayer *player)
{
    int rise = jump_rise_cells(player);
    int descent_steps;

    if (rise < 1)
        rise = 1;
    if (rise > PLAT_JUMP_MAX_RISE)
        rise = PLAT_JUMP_MAX_RISE;
    descent_steps = rise;
    player->jump_phase = 1;
    player->jump_down_i = PLAT_JUMP_DOWN_WAITS_LEN - descent_steps;
    player->jump_script_timer = plat_jump_down_waits[player->jump_down_i];
}

/* One upward scripted step. Sets timer for the row landed on, or switches to descent at max rise. */
static void jump_script_tick_ascent(PlatformPlayer *player, StageView *view)
{
    if (player->jump_up_i >= PLAT_JUMP_UP_WAITS_LEN) {
        jump_begin_descent_from_apex(player);
        return;
    }
    if (!jump_script_try_step(player, view, -1)) {
        player->jump_script = 0;
        return;
    }
    player->jump_up_i++;
    if (player->jump_up_i >= PLAT_JUMP_UP_WAITS_LEN)
        jump_begin_descent_from_apex(player);
    else
        player->jump_script_timer = plat_jump_up_waits[player->jump_up_i];
}

/* One downward scripted step. Sets timer for the row landed on, or ends the script. */
static void jump_script_tick_descent(PlatformPlayer *player, StageView *view)
{
    if (player->jump_down_i >= PLAT_JUMP_DOWN_WAITS_LEN) {
        player->jump_script = 0;
        return;
    }
    if (!jump_script_try_step(player, view, 1)) {
        player->jump_script = 0;
        return;
    }
    player->jump_down_i++;
    if (player->jump_down_i >= PLAT_JUMP_DOWN_WAITS_LEN)
        player->jump_script = 0;
    else
        player->jump_script_timer = plat_jump_down_waits[player->jump_down_i];
}

/*
 * One game tick of scripted jump: count down jump_script_timer while idle on a row.
 * When the timer is zero, run one ascent or descent step (tables match docs/firmware.md).
 * During ascent, if the button is not held and rise is already at least PLAT_JUMP_MIN_RISE,
 * the apex is fixed and descent starts with the same mirrored waits (no extra up step).
 */
static void jump_script_tick(PlatformPlayer *player, StageView *view, const Input *in)
{
    int rise;

    if (!player->jump_script || in == 0)
        return;
    if (player->jump_script_timer > 0) {
        player->jump_script_timer--;
        return;
    }
    if (player->jump_phase == 0) {
        rise = jump_rise_cells(player);
        if (player->jump_up_i < PLAT_JUMP_UP_WAITS_LEN && !in->button &&
            rise >= PLAT_JUMP_MIN_RISE) {
            jump_begin_descent_from_apex(player);
            return;
        }
        jump_script_tick_ascent(player, view);
    } else
        jump_script_tick_descent(player, view);
}

void platform_player_spawn(PlatformPlayer *player, const StageMap *map)
{
    if (player == 0 || map == 0)
        return;
    player->px = (int)map->spawn_x;
    player->py = (int)map->spawn_y;
    player->vy = 0;
    player->walk_accum = 0;
    player->vert_accum = 0;
    player->grounded = 0;
    player->jump_script = 0;
    player->jump_phase = 0;
    player->jump_script_timer = 0;
    player->jump_up_i = 0;
    player->jump_down_i = 0;
    player->jump_base_py = 0;
    player->item_pickups = 0;
}

void platform_player_bind(PlatformPlayer *player, StageView *view, const StageMap *map)
{
    platform_player_spawn(player, map);
    if (view != 0)
        camera_follow(view, player->px);
}

int platform_player_tick(PlatformPlayer *player, StageView *view, const Input *in)
{
    int dx = 0;

    if (player == 0 || view == 0 || in == 0)
        return 0;
    player->item_pickups = 0;
    stage_h_items_tick(view);
    if (in->left && !in->right)
        dx = -1;
    else if (in->right && !in->left)
        dx = 1;
    else
        player->walk_accum = 0;
    try_walk(player, view, dx);
    if (in->button_pressed && player->grounded && !player->jump_script) {
        player->jump_base_py = player->py;
        if (!jump_script_try_step(player, view, -1))
            goto after_jump_start;
        player->grounded = 0;
        player->vy = 0;
        player->vert_accum = 0;
        player->jump_script = 1;
        player->jump_phase = 0;
        player->jump_up_i = 0;
        /* One idle tick at position 2 before moving to 3 (plat_jump_up_waits[0] == 1). */
        player->jump_script_timer = plat_jump_up_waits[0];
    }
after_jump_start:
    if (player->jump_script) {
        jump_script_tick(player, view, in);
        if (!player->jump_script)
            player->vy = 0;
    } else {
        apply_gravity(player);
        move_vertical(player, view);
    }
    if (!player->grounded)
        player->grounded = on_ground(view, player->px, player->py);
    camera_follow(view, player->px);
    return player->item_pickups;
}

void platform_h_paint(const StageView *view, const PlatformPlayer *player)
{
    int sx;
    int sy;

    if (view == 0)
        return;
    if (player == 0 || view->map == 0) {
        stage_h_paint(view);
        return;
    }
    stage_h_paint_ex(view, player->px, player->py, 1);
    sx = player->px - stage_h_camera_get(view);
    sy = player->py;
    if (sx < 0 || sx > FIELD_LAST || sy < 0 || sy > FIELD_LAST)
        return;
    /* FG green, or BG art with empty FG: leave the LED off so acetate green stays dim. */
    if (stage_h_suppress_player_led(view, player->px, player->py))
        return;
    pixel((uint8_t)sx, (uint8_t)sy, true);
}

#if !defined(__AVR__)
int platform_player_x(const PlatformPlayer *player)
{
    if (player == 0)
        return 0;
    return player->px;
}

int platform_player_y(const PlatformPlayer *player)
{
    if (player == 0)
        return 0;
    return player->py;
}

int platform_player_vy(const PlatformPlayer *player)
{
    if (player == 0)
        return 0;
    return player->vy;
}
#endif
