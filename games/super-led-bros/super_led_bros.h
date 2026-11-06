#ifndef SUPER_LED_BROS_H
#define SUPER_LED_BROS_H

#include "input.h"

/*
 * Platform cart. 1x1 player from the white spawn in stage_01. Walk at 15 FPS.
 * Button jumps when grounded. Status digits, left to right, are 4 score,
 * 2 coins, and 2 lives. Score and lives stay at 0. A fresh yellow block adds
 * one coin. The count wraps from 99 to 0.
 */

void super_led_bros_reset(void);
void super_led_bros_tick(const Input *in);

#if !defined(__AVR__)
int super_led_bros_camera_x(void);
int super_led_bros_player_x(void);
int super_led_bros_player_y(void);
int super_led_bros_coins(void);
/* Same coin grant a yellow-block hit reports through platform_player_tick. */
void super_led_bros_apply_pickups(int n);
#endif

#endif
