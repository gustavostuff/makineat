#ifndef FW_RAND_H
#define FW_RAND_H

#include "input.h"

#include <stdint.h>

/*
 * One LCG for every cart. rand_feed mixes both players' sticks into
 * a pool on each tick. rand_reseed copies that pool into the play
 * stream; call it when a game starts and again on reset.
 * rand_seed sets both the pool and the stream (tests).
 */

void rand_feed(const Input *in);
void rand_reseed(void);
void rand_seed(uint32_t n);
uint32_t rand_next(void);

#endif
