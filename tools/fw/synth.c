#include "synth.h"

/*
 * Index 1 is the laser, a falling triangle.
 * Index 2 is the explosion, noise that thins out.
 * Index 3 is the pop, a short square blip.
 * Index 0 and index 4 upward are free.
 * A step is a period in samples, a level from 0 to 255, and a duration
 * in samples. For noise, the period is how often the shift register clocks.
 */

#include <stddef.h>

#define CHANS 3
#define FULL 765

typedef struct {
    uint16_t period;
    uint8_t level;
    uint16_t duration;
} Step;

typedef struct {
    uint8_t channel;
    uint8_t hold;
    const Step *steps;
    uint8_t nsteps;
} Row;

typedef struct {
    const Step *steps;
    uint8_t nsteps;
    uint8_t step;
    uint8_t hold;
    uint8_t on;
    uint16_t left;
    uint16_t phase;
    uint16_t lfsr;
    uint16_t noise_div;
} Chan;

static const Step laser_steps[] = {
    {12, 220, 500},
    {22, 180, 600},
    {40, 130, 700},
    {80, 70, 800},
    {140, 30, 900},
};

static const Step boom_steps[] = {
    {1, 240, 1000},
    {2, 190, 1800},
    {4, 120, 2600},
    {8, 50, 3600},
};

static const Step pop_steps[] = {
    {20, 210, 160},
    {48, 90, 240},
};

static const Row laser_row = {SYNTH_TRIANGLE, 0, laser_steps, 5};
static const Row boom_row = {SYNTH_NOISE, 0, boom_steps, 4};
static const Row pop_row = {SYNTH_SQUARE, 0, pop_steps, 2};

static Chan chan[CHANS];
static int volume_pct = SYNTH_VOLUME_DEFAULT;

static const Row *row_of(uint8_t index)
{
    if (index == 1)
        return &laser_row;
    if (index == 2)
        return &boom_row;
    if (index == 3)
        return &pop_row;
    return NULL;
}

static uint16_t period_of(const Step *step, int noise)
{
    if (noise)
        return step->period == 0 ? 1 : step->period;
    return step->period < 2 ? 2 : step->period;
}

static void load_step(Chan *c)
{
    const Step *step = &c->steps[c->step];

    c->left = step->duration;
    c->phase = (uint16_t)(period_of(step, 0) / 4);
    c->noise_div = period_of(step, 1);
}

static void begin(Chan *c, const Row *row)
{
    c->steps = row->steps;
    c->nsteps = row->nsteps;
    c->step = 0;
    c->hold = row->hold;
    c->on = 1;
    c->lfsr = 0x51u;
    load_step(c);
}

static void advance(Chan *c)
{
    c->step++;
    if (c->step >= c->nsteps) {
        if (!c->hold) {
            c->on = 0;
            return;
        }
        c->step = 0;
    }
    load_step(c);
}

static int16_t tonal(Chan *c, const Step *step, int triangle)
{
    uint16_t period = period_of(step, 0);
    uint16_t half = (uint16_t)(period / 2);
    int16_t level = step->level;
    int16_t out;

    if (half == 0)
        half = 1;
    if (triangle) {
        uint16_t up = c->phase < half ? c->phase : (uint16_t)(period - c->phase);

        out = (int16_t)(((int32_t)up * 2 * level) / half - level);
    } else {
        out = c->phase < half ? level : (int16_t)-level;
    }
    c->phase++;
    if (c->phase >= period)
        c->phase = 0;
    return out;
}

static int16_t noise(Chan *c, const Step *step)
{
    int16_t out = (c->lfsr & 1u) ? step->level : (int16_t)-step->level;

    if (c->noise_div > 0)
        c->noise_div--;
    if (c->noise_div == 0) {
        uint16_t bit = (uint16_t)((c->lfsr ^ (c->lfsr >> 1)) & 1u);

        c->lfsr = (uint16_t)((c->lfsr >> 1) | (bit << 14));
        c->noise_div = period_of(step, 1);
    }
    return out;
}

static int16_t voice(Chan *c, int kind)
{
    const Step *step;
    int16_t out;

    if (!c->on)
        return 0;
    step = &c->steps[c->step];
    if (kind == SYNTH_NOISE)
        out = noise(c, step);
    else
        out = tonal(c, step, kind == SYNTH_TRIANGLE);
    if (c->left > 0)
        c->left--;
    if (c->left == 0)
        advance(c);
    return out;
}

void synth_volume_set(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;
    volume_pct = percent;
}

int synth_volume_get(void)
{
    return volume_pct;
}

void synth_reset(void)
{
    int i;

    for (i = 0; i < CHANS; i++)
        chan[i].on = 0;
}

void synth_play(uint8_t index)
{
    const Row *row = row_of(index);

    if (row == NULL || row->channel >= CHANS || row->nsteps == 0)
        return;
    begin(&chan[row->channel], row);
}

uint16_t synth_sample(void)
{
    int32_t sum = 0;
    int i;
    int32_t dac;

    for (i = 0; i < CHANS; i++)
        sum += voice(&chan[i], i);
    dac = 512 + (sum * 511 * volume_pct) / (FULL * 100);
    if (dac < 0)
        dac = 0;
    if (dac > 1023)
        dac = 1023;
    return (uint16_t)dac;
}

int synth_on(int channel)
{
    if (channel < 0 || channel >= CHANS)
        return 0;
    return chan[channel].on;
}
