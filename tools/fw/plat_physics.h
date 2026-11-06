#ifndef FW_PLAT_PHYSICS_H
#define FW_PLAT_PHYSICS_H

/*
 * Shared vertical fall and horizontal bounce for platform entities.
 * Downward waits match docs/firmware.md Platform (jump descent and fall).
 */

#include "stage.h"

#include <stdint.h>

#define PLAT_PHYS_GRAVITY 1
#define PLAT_PHYS_MAX_FALL 8
/*
 * Downward cells use the same idle waits as a jump descent: 4, 3, 2, 1, 1 ticks
 * between steps. The last wait repeats, so the fastest fall is one cell every
 * two ticks (30 cells per second). Jump code indexes this table too.
 */
#define PLAT_PHYS_FALL_STEPS 5
extern const uint8_t plat_phys_fall_waits[PLAT_PHYS_FALL_STEPS];

typedef struct PlatBody {
    int px;
    int py;
    int vy;
    int walk_accum;
    int vert_accum;
    int grounded;
    uint8_t fall_on;
    uint8_t fall_i;
    uint8_t fall_timer;
} PlatBody;

/* dy: -1 up, +1 down for vertical moves; 0 for horizontal (prev_feet_y ignored). */
typedef int (*PlatBlockedFn)(void *ctx, int x, int y, int prev_feet_y, int dy);

void plat_phys_apply_gravity(int *vy);

/*
 * One tick of a free fall. The first airborne tick arms the jump-descent waits and does
 * not step. Later ticks count those waits down. terminal continues at the last wait,
 * and that arm tick spends one count of it. Returns 1 when the body should move down
 * one cell.
 */
int plat_phys_fall_step(uint8_t *fall_on, uint8_t *fall_i, uint8_t *fall_timer, int terminal);

/*
 * Vertical move. Downward motion uses plat_phys_fall_step. blocked returns 1 if (x,y)
 * is blocked. If blocked is null, uses stage_h_blocks_vert / stage_h_blocks_horiz.
 * ceiling_bump: optional for upward hits; adds its return value to bump_sum_out.
 */
void plat_phys_move_vertical(PlatBody *body, StageView *view,
        int (*ceiling_bump)(StageView *view, int x, int y, void *ctx), void *ceiling_ctx,
        int *bump_sum_out, PlatBlockedFn blocked, void *blocked_ctx);

void plat_phys_horiz_bounce(int *px, int *walk_accum, int8_t *dir, int py, StageView *view,
        int horiz_fps, PlatBlockedFn blocked, void *blocked_ctx);

#endif
