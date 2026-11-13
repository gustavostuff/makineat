#ifndef FW_ROCK_H
#define FW_ROCK_H

/*
 * Solid 2x2 and 3x3 rocks. A new rock appears on a random row from
 * the top of the 32 by 32 grid through the top row of the screen, and
 * the whole rock fits in that band. It drifts down.
 * Vertical speed is chosen when the rock appears. For the first 10
 * seconds the choice is 4, 6, or 8 FPS. The next 10 seconds it is 8,
 * 10, or 12. The next 10 seconds it is 12, 14, or 16. After that it
 * is 16, 18, or 20. Horizontal speed is -4, 0, or 4 FPS.
 * rock_tick returns 1 when a rock meets the ship.
 */

/* 60 Hz ticks. 60 * 10 is ten seconds. */
#define ROCK_BAND_TICKS (60 * 10)

/* Vertical FPS for slot 0, 1, or 2 after age_ticks of the round. */
int rock_vert_fps(int slot, int age_ticks);

/* Horizontal FPS for pick 0, 1, or 2. */
int rock_horiz_fps(int pick);

void rock_reset(void);
void rock_add(int x, int y, int size, int vert_fps, int horiz_fps);
int rock_tick(void);
int rock_score_get(void);

#endif
