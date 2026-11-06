#ifndef SUPER_LED_BROS_H
#define SUPER_LED_BROS_H

#include "input.h"

/*
 * Platform cart. 1x1 player from the white spawn in stage_01. Walk at 15 FPS.
 * Button jumps when grounded. Status digits, left to right, are 4 score,
 * 2 coins, and 2 lives. Score stays at 0. Lives start at SUPER_LED_BROS_LIVES.
 * A fresh yellow block adds one coin. The count wraps from 99 to 0.
 * Falling off the bottom leaves the stage up for SUPER_LED_BROS_DEATH_TICKS.
 * The life counter drops when that wait restarts the stage, and it counts
 * through 0. Losing the life shown as 0 ends on "game over". The action
 * button can start a new game after SUPER_LED_BROS_OVER_TICKS.
 */

#define SUPER_LED_BROS_LIVES 4
/* 3 seconds at the 60 Hz game tick, held open for a later death sting. */
#define SUPER_LED_BROS_DEATH_TICKS 180
/* 3 seconds of game over before the action button can start a new game. */
#define SUPER_LED_BROS_OVER_TICKS 180

void super_led_bros_reset(void);
void super_led_bros_tick(const Input *in);

#if !defined(__AVR__)
int super_led_bros_camera_x(void);
int super_led_bros_player_x(void);
int super_led_bros_player_y(void);
int super_led_bros_coins(void);
/* Same coin grant a yellow-block hit reports through platform_player_tick. */
void super_led_bros_apply_pickups(int n);
/* Put the feet on the first row below the stage. The next tick is a fall death. */
void super_led_bros_test_place_below(void);
#endif

#endif
