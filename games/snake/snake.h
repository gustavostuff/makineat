#ifndef SNAKE_H
#define SNAKE_H

#include "input.h"

/*
 * Reset opens a title in the large 5x6 font: "S n a k e" scroll-bounces,
 * then alternates with "PRESS BUTTON" after each full bounce cycle until the
 * action button starts play (status digits stay off on the title). Classic snake on
 * the 16x16 map. Four score food cells stay lit until eaten.
 * One speed pickup blinks at 2 FPS for eight seconds, then 4 FPS for two
 * seconds, then vanishes. The snake starts at 2 FPS; each speed pickup
 * adds 1 FPS up to 30 and adds one to the score. The left four status digits
 * show movement FPS; the right four show the score. The action button drops
 * speed by 1 FPS down to the 2 FPS minimum. A wall or
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
int snake_on_title(void);
const char *snake_title_message(void);
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
