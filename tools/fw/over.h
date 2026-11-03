#ifndef FW_OVER_H
#define FW_OVER_H

#include "input.h"

/*
 * The end picture shared by every cart. Paddle shows a face and the score.
 * Space shooter uses over_start_shooter instead.
 * over_start draws the end picture until the button starts the next round.
 * over_tick returns 1 on a button press.
 */

#define OVER_TEXT_DELAY 60 /* still time at start and at each bounce turn */
#define OVER_TEXT_STEP 8   /* scroll one column every N frames while moving */

void over_clear(void);
void over_start(void);
/* judge is 1 win, 0 tie, -1 lose (same as score_judge). */
void over_start_mark(int judge);
/* Space shooter: saved player score, duel vs rocks, no score on the matrix. */
void over_start_shooter(int player, int rocks);
void over_celebrate(void);
void over_paint(void);
int over_tick(const Input *in);

#endif
