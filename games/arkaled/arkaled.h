#ifndef ARKALED_H
#define ARKALED_H

#include "input.h"

/*
 * Forty-eight pieces on rows 2 through 5. Each piece is one LED,
 * in columns 2 through 13. Rows 0 and 1 are empty, and so are
 * the two columns on each side. Two three-LED paddles share the
 * bottom row. Reset opens a 1P/2P title. "arkaled" bounce-scrolls
 * in a 16x4 window at (0,2). 1P lets player one drive
 * both paddles in a relay: each stays in its 8-pixel half.
 * The paddle in the direction of travel runs to the edge of
 * that half; the next step that would leave the half moves
 * the other paddle instead. 2P keeps player one in columns
 * 0-7 and player two in 8-15. Player one uses left and
 * right; player two uses p2_left and p2_right. Hold a
 * player button for 60 FPS paddle motion, else 15 FPS.
 * Overlapping paddle cells blink 4 Hz.
 * Five random bricks anywhere in the block blink 2 Hz; hitting one
 * drops an item that blinks four times, then falls one pixel,
 * twice per second. Catching it seats another ball
 * on that paddle (player one wins a tie). That ball waits for
 * that paddle to move, then opens at the starting pace and ramps.
 * The first ball waits above player one until that paddle moves.
 * collide_step walks each live ball. A piece in the way turns off.
 * Missing the last ball blinks it for a second, then a new round.
 * Extra balls just drop. The last piece also starts a new round.
 * Later steps start at 15 ticks. Every 2 seconds that wait shrinks
 * by one tick, down to two (30 FPS). A ball on the row above a paddle moves
 * with that paddle. A bounce on a paddle adds one to that
 * player's score (player one in 1P), shown on the status digits.
 *
 * ARKALED_DEBUG_SLOW (compile with -DARKALED_DEBUG_SLOW=1, or
 * make -C tools/sim ARKALED_DEBUG=1 makineat-ui) holds the ball
 * at 0.25 FPS and the paddles at 1 FPS so the 1P relay is easy
 * to inspect. The button no longer speeds the paddles.
 * arkaled_begin_play(1) or (2) leaves the title.
 * arkaled_seed sets the shared RNG. Zero becomes 1.
 * arkaled_load places a live ball and leaves the pieces as they are.
 * arkaled_fill, arkaled_wipe, and arkaled_set edit those pieces.
 * arkaled_set addresses that one LED.
 */

void arkaled_reset(void);
void arkaled_begin_play(int players);
int arkaled_on_title(void);
int arkaled_title_scroll_x(void);
void arkaled_tick(const Input *in);
void arkaled_seed(unsigned n);
void arkaled_load(int pad_x, int x, int y, int vx, int vy);
void arkaled_fill(void);
void arkaled_wipe(void);
void arkaled_set(int x, int y, int on);
void arkaled_mark_special(int x, int y);
int arkaled_on(int x, int y);
int arkaled_special(int x, int y);
int arkaled_count(void);
int arkaled_pad_x(int player);
int arkaled_ball_count(void);
int arkaled_item_at(int *x, int *y);
int arkaled_held_ball(int *x, int *y);
void arkaled_ball(int *x, int *y, int *vx, int *vy);
void arkaled_place_pads(int x1, int x2);
int arkaled_period(int ticks);
int arkaled_score(int player);

#endif
