#ifndef PONG_H
#define PONG_H

#include "input.h"

/*
 * Two-player pong. Player one moves the left paddle with up and down.
 * Player two moves the right paddle with p2_up and p2_down.
 * Hold a player button for 60 FPS paddle motion, else 30 FPS.
 * The ball ramps up to 60 FPS. First to 100 on the status digits wins.
 * Play is columns 1..14; 0 and 15 are paddles. A point is scored only when
 * the ball reaches column 0 or 15 off the paddle. A face hit flips vx.
 * Arriving down onto the top pixel or up onto the bottom pixel is a
 * corner hit and flips both vx and vy. After a point the ball blinks
 * on the goal cell for one second, then paddles reset and the ball sits
 * on the conceded player's paddle until that player moves.
 *
 * PONG_DEBUG_SLOW_BALL (compile with -DPONG_DEBUG_SLOW_BALL=1, or
 * make -C tools/sim PONG_DEBUG=1 makineat-ui) holds the ball at 1 FPS
 * so paddle bounces are easy to inspect. Paddle speed is unchanged.
 */

void pong_reset(void);
void pong_tick(const Input *in);
void pong_seed(unsigned n);
int pong_period(int ticks);
int pong_pad_y(int player);
int pong_ball_y(void);
int pong_awaiting_serve(void);
void pong_play_tick(const Input *in);
void pong_begin_play(int player);

#if !defined(__AVR__)
void pong_place_ball(int x, int y, int vx, int vy);
void pong_place_paddles(int ly, int ry);
int pong_in_demo(void);
#endif

#endif
