#include "rand.h"

static uint32_t pool = 1;
static uint32_t state = 1;
static uint32_t frames;

static uint32_t step(uint32_t x)
{
    if (x == 0)
        x = 1;
    return x * 1664525u + 1013904223u;
}

void rand_seed(uint32_t n)
{
    if (n == 0)
        n = 1;
    pool = n;
    state = n;
}

void rand_reseed(void)
{
    state = pool;
    if (state == 0)
        state = 1;
}

uint32_t rand_next(void)
{
    state = step(state);
    return state;
}

static uint32_t pack_input(const Input *in)
{
    uint32_t b = 0;

    if (in->up)
        b |= 1u;
    if (in->down)
        b |= 2u;
    if (in->left)
        b |= 4u;
    if (in->right)
        b |= 8u;
    if (in->button)
        b |= 16u;
    if (in->p2_up)
        b |= 32u;
    if (in->p2_down)
        b |= 64u;
    if (in->p2_left)
        b |= 128u;
    if (in->p2_right)
        b |= 256u;
    if (in->p2_button)
        b |= 512u;
    if (in->up_pressed)
        b |= 1u << 10;
    if (in->down_pressed)
        b |= 1u << 11;
    if (in->left_pressed)
        b |= 1u << 12;
    if (in->right_pressed)
        b |= 1u << 13;
    if (in->button_pressed)
        b |= 1u << 14;
    if (in->p2_up_pressed)
        b |= 1u << 15;
    if (in->p2_down_pressed)
        b |= 1u << 16;
    if (in->p2_left_pressed)
        b |= 1u << 17;
    if (in->p2_right_pressed)
        b |= 1u << 18;
    if (in->p2_button_pressed)
        b |= 1u << 19;
    return b;
}

void rand_feed(const Input *in)
{
    frames++;
    if (in == 0)
        return;
    pool ^= pack_input(in) * 0x9E3779B9u;
    pool ^= frames;
    pool = step(pool);
}
