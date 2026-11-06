#include "platform.h"

#include "plat_physics.h"

#include <stddef.h>
#include <stdint.h>

#define PLAT_MOVE_FPS 15
/*
 * Scripted jump tables match docs/firmware.md (Platform section). Ascent waits in
 * plat_jump_up_waits. Descent waits are plat_phys_fall_waits (4, 3, 2, 1, 1). While idle
 * on a row (timer expired), if ascent rise is at least PLAT_JUMP_MIN_RISE and button is
 * not held, jump_begin_descent_from_apex picks the descent slice so mirrored timing
 * matches apex height. Walk uses PLAT_MOVE_FPS. A fall that is not a jump uses the same
 * descent waits. The fastest wait is one idle tick, one cell every two ticks.
 */
#define PLAT_JUMP_UP_WAITS_LEN 4
static const uint8_t plat_jump_up_waits[PLAT_JUMP_UP_WAITS_LEN] = {1, 2, 3, 4};
/* Minimum rise is 2 cells above jump_base_py (3 rows including ground before first step). */
#define PLAT_JUMP_MIN_RISE 2
#define PLAT_JUMP_MAX_RISE 5

/* Growth cutscene: 0.8s total at 60 Hz, each 0.1s alternates 1-tall vs 2-tall on screen. */
#define GROWTH_ANIM_PHASE_TICKS (STAGE_TICK_HZ / 10)
#define GROWTH_ANIM_TOTAL_TICKS (STAGE_TICK_HZ * 8 / 10)

static int player_grown_hitbox(const PlatformPlayer *player);

/* A body cell above the stage is above the screen. It is open air, not a wall. */
static int player_body_blocked_horiz(const StageView *view, int px, int feet_y, int grown)
{
    if (view == 0 || view->map == 0)
        return 1;
    if (px < 0 || px >= (int)view->map->width || feet_y >= (int)view->map->height)
        return 1;
    if (feet_y >= 0 && stage_h_blocks_horiz(view, px, feet_y))
        return 1;
    if (grown && feet_y - 1 >= 0 && stage_h_blocks_horiz(view, px, feet_y - 1))
        return 1;
    return 0;
}

static int player_body_blocked_vert(const StageView *view, int px, int new_feet_y, int prev_feet_y, int dy,
    int grown)
{
    if (view == 0 || view->map == 0)
        return 1;
    if (px < 0 || px >= (int)view->map->width)
        return 1;
    /* Below the stage there is no floor. A real cell still blocks inside the map. */
    if (new_feet_y >= (int)view->map->height)
        return 0;
    if (new_feet_y >= 0 && stage_h_blocks_vert(view, px, new_feet_y, prev_feet_y, dy))
        return 1;
    if (grown) {
        int head_y = new_feet_y - 1;

        if (head_y < 0)
            return 0;
        /*
         * One-way ledges only catch the feet. The head must ignore semisolid or a tall body
         * stops one row early, and a body that jumped into the ledge cannot fall back out.
         */
        if (dy > 0) {
            if (stage_h_blocks_vert_hard(view, px, head_y))
                return 1;
        } else if (stage_h_blocks_vert(view, px, head_y, prev_feet_y - 1, dy))
            return 1;
    }
    return 0;
}

static int on_ground(const StageView *view, int px, int feet_y, int grown)
{
    (void)grown;
    if (view == 0 || view->map == 0)
        return 0;
    if (feet_y + 1 < 0 || feet_y + 1 >= (int)view->map->height)
        return 0;
    return stage_h_blocks_vert(view, px, feet_y + 1, feet_y, 1);
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
        if (!player_body_blocked_horiz(view, nx, player->py, player_grown_hitbox(player)))
            player->px = nx;
    }
}

static int player_grown_hitbox(const PlatformPlayer *player)
{
    if (player->growth_anim > 0)
        return player->growth_anim_show_two ? 1 : 0;
    if (player->crouch)
        return 0;
    return player->grown;
}

/* Head cell is hard terrain, so a crouched player cannot stand back up into it. */
static int crouch_ceiling(const StageView *view, int px, int feet_y)
{
    if (feet_y - 1 < 0)
        return 0;
    return stage_h_blocks_horiz(view, px, feet_y - 1);
}

static void update_crouch(PlatformPlayer *player, StageView *view, const Input *in)
{
    if (player == 0 || !player->grown) {
        if (player != 0)
            player->crouch = 0;
        return;
    }
    if (in != 0 && in->down) {
        player->crouch = 1;
        return;
    }
    if (player->crouch && !crouch_ceiling(view, player->px, player->py))
        player->crouch = 0;
}

/*
 * On an upward step, grown players test (px, ny) and (px, ny-1) for solid. The block bump
 * must use the cell that overlaps the 2x2 item — often the head hits first while feet are
 * still air, so we try the head row before the feet row.
 */
static void try_item_bump_upward(PlatformPlayer *player, StageView *view, int feet_y, int grown)
{
    StageBumpResult r;

    if (player == 0 || view == 0)
        return;
    if (grown) {
        r = stage_h_item_bump(view, player->px, feet_y - 1);
        if (r == STAGE_BUMP_COIN)
            player->item_pickups++;
        if (r != STAGE_BUMP_NONE)
            return;
    }
    r = stage_h_item_bump(view, player->px, feet_y);
    if (r == STAGE_BUMP_COIN)
        player->item_pickups++;
}

static void fall_free(PlatformPlayer *player, StageView *view)
{
    int ny;
    int terminal;

    if (on_ground(view, player->px, player->py, player_grown_hitbox(player))) {
        player->grounded = 1;
        player->vy = 0;
        player->fall_on = 0;
        player->fall_terminal = 0;
        return;
    }
    player->grounded = 0;
    terminal = player->fall_terminal;
    player->fall_terminal = 0;
    if (!plat_phys_fall_step(&player->fall_on, &player->fall_i, &player->fall_timer, terminal))
        return;
    ny = player->py + 1;
    if (player_body_blocked_vert(view, player->px, ny, player->py, 1, player_grown_hitbox(player))) {
        player->grounded = 1;
        player->vy = 0;
        player->fall_on = 0;
        return;
    }
    player->py = ny;
    player->grounded = on_ground(view, player->px, player->py, player_grown_hitbox(player));
    if (player->grounded) {
        player->vy = 0;
        player->fall_on = 0;
    }
}

/* Try one scripted jump step up (-1) or down (+1). Returns 0 if blocked (caller clears script). */
static int jump_script_try_step(PlatformPlayer *player, StageView *view, int dy)
{
    int ny = player->py + dy;

    if (player_body_blocked_vert(view, player->px, ny, player->py, dy, player_grown_hitbox(player))) {
        if (dy > 0)
            player->grounded = 1;
        else {
            try_item_bump_upward(player, view, ny, player_grown_hitbox(player));
        }
        player->vy = 0;
        return 0;
    }
    player->py = ny;
    player->grounded = on_ground(view, player->px, player->py, player_grown_hitbox(player));
    return 1;
}

/* rise = cells cleared above jump_base_py (>= 1 after first step). */
static int jump_rise_cells(const PlatformPlayer *player)
{
    return player->jump_base_py - player->py;
}

/*
 * Enter descent at the current apex. plat_phys_fall_waits[k] is the idle time on the row
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
    player->jump_down_i = PLAT_PHYS_FALL_STEPS - descent_steps;
    player->jump_script_timer = plat_phys_fall_waits[player->jump_down_i];
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
    if (player->jump_down_i >= PLAT_PHYS_FALL_STEPS) {
        player->jump_script = 0;
        player->fall_terminal = 1;
        return;
    }
    if (!jump_script_try_step(player, view, 1)) {
        player->jump_script = 0;
        return;
    }
    player->jump_down_i++;
    if (player->jump_down_i >= PLAT_PHYS_FALL_STEPS) {
        player->jump_script = 0;
        player->fall_terminal = 1;
    } else
        player->jump_script_timer = plat_phys_fall_waits[player->jump_down_i];
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

static void try_collect_growth(PlatformPlayer *player, StageView *view)
{
    int h;

    if (player == 0 || view == 0)
        return;
    if (player->grown || player->growth_anim > 0)
        return;
    h = player_grown_hitbox(player) ? 2 : 1;
    if (stage_h_collect_growth_pickup(view, player->px, player->py, h)) {
        player->growth_anim = GROWTH_ANIM_TOTAL_TICKS;
        player->growth_anim_phase_ticks = GROWTH_ANIM_PHASE_TICKS;
        player->growth_anim_show_two = 0;
    }
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
    player->grown = 0;
    player->crouch = 0;
    player->fall_on = 0;
    player->fall_i = 0;
    player->fall_timer = 0;
    player->fall_terminal = 0;
    player->growth_anim = 0;
    player->growth_anim_phase_ticks = 0;
    player->growth_anim_show_two = 0;
}

void platform_player_bind(PlatformPlayer *player, StageView *view, const StageMap *map)
{
    platform_player_spawn(player, map);
    if (view != 0) {
        player->grounded = on_ground(view, player->px, player->py, player_grown_hitbox(player));
        camera_follow(view, player->px);
    }
}

int platform_player_growth_freeze(const PlatformPlayer *player)
{
    if (player == 0)
        return 0;
    return player->growth_anim > 0;
}

int platform_player_tick(PlatformPlayer *player, StageView *view, const Input *in)
{
    int dx = 0;

    if (player == 0 || view == 0 || in == 0)
        return 0;
    player->item_pickups = 0;
    if (player->growth_anim > 0) {
        player->growth_anim--;
        if (player->growth_anim_phase_ticks > 0)
            player->growth_anim_phase_ticks--;
        if (player->growth_anim > 0 && player->growth_anim_phase_ticks == 0) {
            player->growth_anim_show_two = (uint8_t)(player->growth_anim_show_two ? 0 : 1);
            player->growth_anim_phase_ticks = GROWTH_ANIM_PHASE_TICKS;
        }
        if (player->growth_anim == 0) {
            player->grown = 1;
            player->growth_anim_phase_ticks = 0;
        }
        return player->item_pickups;
    }
    update_crouch(player, view, in);
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
        player->fall_on = 0;
        player->fall_terminal = 0;
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
    } else
        fall_free(player, view);
    if (!player->grounded)
        player->grounded = on_ground(view, player->px, player->py, player_grown_hitbox(player));
    try_collect_growth(player, view);
    camera_follow(view, player->px);
    return player->item_pickups;
}

static int paint_player_cells(const StageView *view, int world_x, int feet_y, int h)
{
    int sy;
    int sx;
    int k;

    sx = world_x - stage_h_camera_get(view);
    if (sx < 0 || sx > FIELD_LAST)
        return 0;
    for (k = 0; k < h; k++) {
        sy = feet_y - k;
        if (sy < 0 || sy > FIELD_LAST)
            continue;
        if (stage_h_suppress_player_led(view, world_x, sy))
            continue;
        pixel((uint8_t)sx, (uint8_t)sy, true);
    }
    return 1;
}

void platform_h_paint(const StageView *view, const PlatformPlayer *player)
{
    int hide_h;
    int draw_h;

    if (view == 0)
        return;
    if (player == 0 || view->map == 0) {
        stage_h_paint(view);
        return;
    }
    hide_h = player_grown_hitbox(player) ? 2 : 1;
    stage_h_paint_ex(view, player->px, player->py, 1, hide_h);
    draw_h = hide_h;
    paint_player_cells(view, player->px, player->py, draw_h);
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
