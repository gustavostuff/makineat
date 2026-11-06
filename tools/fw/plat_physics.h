#ifndef FW_PLAT_PHYSICS_H
#define FW_PLAT_PHYSICS_H

/*
 * Shared vertical fall and horizontal bounce for platform entities.
 * Gravity and vertical step rate match docs/firmware.md Platform (fall).
 */

#include "stage.h"

#include <stdint.h>

#define PLAT_PHYS_GRAVITY 1
#define PLAT_PHYS_MAX_FALL 8
#define PLAT_PHYS_VERT_STEP_FPS 15

typedef struct PlatBody {
    int px;
    int py;
    int vy;
    int walk_accum;
    int vert_accum;
    int grounded;
} PlatBody;

typedef int (*PlatBlockedFn)(void *ctx, int x, int y);

void plat_phys_apply_gravity(int *vy);

/*
 * Vertical move when vy is non-zero. blocked returns 1 if (x,y) is blocked.
 * If blocked is null, uses stage_h_solid(view, x, y) with ctx = view.
 * ceiling_bump: optional for upward hits; adds its return value to bump_sum_out.
 */
void plat_phys_move_vertical(PlatBody *body, StageView *view,
        int (*ceiling_bump)(StageView *view, int x, int y, void *ctx), void *ceiling_ctx,
        int *bump_sum_out, PlatBlockedFn blocked, void *blocked_ctx);

void plat_phys_horiz_bounce(int *px, int *walk_accum, int8_t *dir, int py, StageView *view,
        int horiz_fps, PlatBlockedFn blocked, void *blocked_ctx);

#endif
