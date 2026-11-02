#ifndef FW_OVER_H
#define FW_OVER_H

#include "input.h"

/*
 * The end picture shared by every cart. A face, then the round's score.
 * over_start draws it and holds the button for one second.
 * over_tick returns 1 when the button may start the next round.
 */

#define OVER_HOLD 60

void over_clear(void);
void over_start(void);
/* judge is 1 win, 0 tie, -1 lose (same as score_judge). */
void over_start_mark(int judge);
void over_celebrate(void);
void over_paint(void);
int over_tick(const Input *in);

#endif
