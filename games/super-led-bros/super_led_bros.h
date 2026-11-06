#ifndef SUPER_LED_BROS_H
#define SUPER_LED_BROS_H

#include "input.h"

/*
 * Platform demo: 1x1 player from the white spawn in stage_01. Left/right
 * walk at 30 FPS; button jumps when grounded. Camera follows the player.
 */

void super_led_bros_reset(void);
void super_led_bros_tick(const Input *in);

#if !defined(__AVR__)
int super_led_bros_camera_x(void);
int super_led_bros_player_x(void);
int super_led_bros_player_y(void);
#endif

#endif
