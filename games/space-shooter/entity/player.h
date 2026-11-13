#ifndef FW_PLAYER_H
#define FW_PLAYER_H

#include "input.h"

/*
 * The ship. Three columns, two rows:
 *   010
 *   111
 * The stick steps it one LED every two ticks.
 * It can reach every edge of the field.
 */

void player_reset(void);
void player_tick(const Input *in);
int player_nose_x(void);
int player_nose_y(void);
int player_covers(int x, int y);

#endif
