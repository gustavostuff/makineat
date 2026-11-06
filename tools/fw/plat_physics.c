#include "plat_physics.h"

#include <stddef.h>

#include "stage.h"

void plat_phys_apply_gravity(int *vy)
{
    if (vy == 0)
        return;
    *vy += PLAT_PHYS_GRAVITY;
    if (*vy > PLAT_PHYS_MAX_FALL)
        *vy = PLAT_PHYS_MAX_FALL;
}

static int default_blocked(void *ctx, int x, int y)
{
    StageView *view = (StageView *)ctx;

    if (view == 0)
        return 1;
    return stage_h_solid(view, x, y);
}

static int on_ground_blocked(StageView *view, PlatBlockedFn blocked, void *blocked_ctx, int px, int py)
{
    if (view == 0 || view->map == 0)
        return 0;
    if (py + 1 >= (int)view->map->height)
        return 1;
    return blocked(blocked_ctx, px, py + 1);
}

void plat_phys_move_vertical(PlatBody *body, StageView *view,
    int (*ceiling_bump)(StageView *view, int x, int y, void *ctx), void *ctx,
    int *bump_count_out, PlatBlockedFn blocked, void *blocked_ctx)
{
    int ny;
    int step;
    int n;

    if (body == 0 || view == 0)
        return;
    if (blocked == 0) {
        blocked = default_blocked;
        blocked_ctx = view;
    }
    if (body->vy > 0)
        step = 1;
    else if (body->vy < 0)
        step = -1;
    else {
        body->vert_accum = 0;
        return;
    }
    body->vert_accum += PLAT_PHYS_VERT_STEP_FPS;
    while (body->vert_accum >= STAGE_TICK_HZ) {
        body->vert_accum -= STAGE_TICK_HZ;
        ny = body->py + step;
        if (blocked(blocked_ctx, body->px, ny)) {
            if (step > 0) {
                body->grounded = 1;
                body->vy = 0;
            } else {
                n = 0;
                if (ceiling_bump != 0)
                    n = ceiling_bump(view, body->px, ny, ctx);
                if (bump_count_out != 0)
                    *bump_count_out += n;
                body->vy = 0;
            }
            body->vert_accum = 0;
            return;
        }
        body->py = ny;
        body->grounded = on_ground_blocked(view, blocked, blocked_ctx, body->px, body->py);
    }
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
        if (!blocked(blocked_ctx, nx, py))
            *px = nx;
        else
            *dir = (int8_t)(-(int)*dir);
    }
}
