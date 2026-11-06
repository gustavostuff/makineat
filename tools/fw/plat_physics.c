#include "plat_physics.h"

#include <stddef.h>

#include "stage.h"

const uint8_t plat_phys_fall_waits[PLAT_PHYS_FALL_STEPS] = {4, 3, 2, 1, 1};

int plat_phys_fall_step(uint8_t *fall_on, uint8_t *fall_i, uint8_t *fall_timer, int terminal)
{
    int i;

    if (fall_on == 0 || fall_i == 0 || fall_timer == 0)
        return 0;
    if (!*fall_on) {
        if (terminal)
            *fall_i = PLAT_PHYS_FALL_STEPS - 1;
        else
            *fall_i = 0;
        *fall_timer = plat_phys_fall_waits[*fall_i];
        *fall_on = 1;
        /* A finished jump already stepped last tick, so this tick spends one count. */
        if (terminal && *fall_timer > 0)
            (*fall_timer)--;
        if (*fall_timer > 0)
            return 0;
    }
    if (*fall_timer > 0) {
        (*fall_timer)--;
        return 0;
    }
    i = (int)*fall_i;
    if (i < PLAT_PHYS_FALL_STEPS - 1)
        i++;
    *fall_i = (uint8_t)i;
    *fall_timer = plat_phys_fall_waits[i];
    return 1;
}

void plat_phys_apply_gravity(int *vy)
{
    if (vy == 0)
        return;
    *vy += PLAT_PHYS_GRAVITY;
    if (*vy > PLAT_PHYS_MAX_FALL)
        *vy = PLAT_PHYS_MAX_FALL;
}

static int default_blocked(void *ctx, int x, int y, int prev_feet_y, int dy)
{
    StageView *view = (StageView *)ctx;

    if (view == 0)
        return 1;
    if (dy == 0)
        return stage_h_blocks_horiz(view, x, y);
    return stage_h_blocks_vert(view, x, y, prev_feet_y, dy);
}

static int on_ground_blocked(StageView *view, PlatBlockedFn blocked, void *blocked_ctx, int px, int py)
{
    if (view == 0 || view->map == 0)
        return 0;
    if (py + 1 >= (int)view->map->height)
        return 1;
    return blocked(blocked_ctx, px, py + 1, py, 1);
}

void plat_phys_move_vertical(PlatBody *body, StageView *view,
    int (*ceiling_bump)(StageView *view, int x, int y, void *ctx), void *ctx,
    int *bump_count_out, PlatBlockedFn blocked, void *blocked_ctx)
{
    int ny;

    (void)ceiling_bump;
    (void)ctx;
    (void)bump_count_out;
    if (body == 0 || view == 0)
        return;
    if (blocked == 0) {
        blocked = default_blocked;
        blocked_ctx = view;
    }
    if (on_ground_blocked(view, blocked, blocked_ctx, body->px, body->py)) {
        body->grounded = 1;
        body->vy = 0;
        body->fall_on = 0;
        return;
    }
    body->grounded = 0;
    if (!plat_phys_fall_step(&body->fall_on, &body->fall_i, &body->fall_timer, 0))
        return;
    ny = body->py + 1;
    if (blocked(blocked_ctx, body->px, ny, body->py, 1)) {
        body->grounded = 1;
        body->vy = 0;
        body->fall_on = 0;
        return;
    }
    body->py = ny;
    body->grounded = on_ground_blocked(view, blocked, blocked_ctx, body->px, body->py);
    if (body->grounded)
        body->fall_on = 0;
}

void plat_phys_horiz_bounce(int *px, int *walk_accum, int8_t *dir, int py, StageView *view,
    int horiz_fps, PlatBlockedFn blocked, void *blocked_ctx)
{
    int nx;

    if (px == 0 || walk_accum == 0 || dir == 0 || view == 0)
        return;
    if (blocked == 0) {
        blocked = default_blocked;
        blocked_ctx = view;
    }
    if (*dir != -1 && *dir != 1)
        *dir = 1;
    *walk_accum += horiz_fps;
    while (*walk_accum >= STAGE_TICK_HZ) {
        *walk_accum -= STAGE_TICK_HZ;
        nx = *px + (int)*dir;
        if (!blocked(blocked_ctx, nx, py, py, 0))
            *px = nx;
        else
            *dir = (int8_t)(-(int)*dir);
    }
}
