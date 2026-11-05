#ifndef SNAKE_H
#define SNAKE_H

#include "input.h"

/*
 * Classic snake on the 16x16 map. Four score food cells stay lit.
 * One speed pickup blinks at 4 FPS for eight seconds, then 8 FPS for two
 * seconds, then vanishes. The snake starts at 2 FPS; each speed pickup
 * adds 1 FPS up to 30. All eight status digits show the score. A wall or
 * self hit blinks the snake four times in about one second, then restarts.
 * Up to two turns can be queued for tight corners while moving slowly.
 */

void snake_reset(void);
void snake_tick(const Input *in);

int snake_score(void);
int snake_length(void);
int snake_head_x(void);
int snake_head_y(void);
int snake_food_count(void);
int snake_speed_fps(void);

#if !defined(__AVR__)
void snake_seed(unsigned n);
int snake_death_blinking(void);
void snake_place_food(int slot, int x, int y);
int snake_food_xy(int slot, int *x, int *y);
int snake_body_at(int x, int y);
void snake_place_speed(int x, int y);
int snake_turn_queue_depth(void);
void snake_pending_dir(int *out_dx, int *out_dy);
void snake_travel_dir(int *out_dx, int *out_dy);
#endif

#endif
