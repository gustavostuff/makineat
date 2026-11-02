#ifndef FW_ROCK_H
#define FW_ROCK_H

/*
 * Solid 2x2 and 3x3 rocks. A new rock appears on a random row from
 * the top of the 32 by 32 grid through the top row of the screen, and
 * the whole rock fits in that band. It drifts down.
 * A new rock falls every 4, 8, 12, or 16 ticks. Over ROCK_RAMP ticks
 * of the round that period shortens until every rock falls once a tick.
 * A sideways step, if any, comes every 15 or 30 ticks and is one column
 * left or right.
 * rock_tick returns 1 when a rock meets the ship.
 */

/* 60 Hz ticks. 60 * 60 is one minute. */
#define ROCK_RAMP (60 * 60)

/* Fall period after `age` play ticks. `base` is 4, 8, 12, or 16. */
int rock_fall_period(int base, int age);

void rock_reset(void);
void rock_add(int x, int y, int size, int every, int dx, int xevery);
int rock_tick(void);
int rock_score_get(void);

#endif
