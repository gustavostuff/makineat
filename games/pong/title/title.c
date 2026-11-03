#include "title/title.h"

#include "pong.h"
#include "status.h"

#include <string.h>

#define DEBOUNCE 2
#define WAVE_STEP 8
#define SERVE_WAIT 30 /* 0.5 s at 60 Hz */
#define PAD_H 4

/* Raw MAX7219 bits with decode off: A = top, G = middle, D = bottom. */
#define SEG_A 0x40
#define SEG_G 0x01
#define SEG_D 0x08

static const uint8_t wave[8] = {
    SEG_A, SEG_A, SEG_G, SEG_G, SEG_D, SEG_D, SEG_G, SEG_G,
};

typedef struct {
    uint8_t count[2];
    uint8_t stable[2];
} Npc;

static Npc npc[2];
static int wave_off;
static int wave_pace;
static int serve_hold;
static int waiting;

static void npc_reset(Npc *n)
{
    n->count[0] = 0;
    n->count[1] = 0;
    n->stable[0] = 0;
    n->stable[1] = 0;
}

static int debounce(Npc *n, int raw, int idx)
{
    if (raw == n->stable[idx]) {
        n->count[idx] = 0;
        return n->stable[idx];
    }
    n->count[idx]++;
    if (n->count[idx] >= DEBOUNCE) {
        n->stable[idx] = (uint8_t)raw;
        n->count[idx] = 0;
    }
    return n->stable[idx];
}

static void follow(int pad_y, int ball_y, int *up, int *down)
{
    *up = 0;
    *down = 0;
    if (ball_y < pad_y + 1)
        *up = 1;
    else if (ball_y > pad_y + PAD_H - 2)
        *down = 1;
}

static void think(Npc *n, int pad_y, int *up, int *down)
{
    int want_up;
    int want_down;

    follow(pad_y, pong_ball_y(), &want_up, &want_down);
    *up = debounce(n, want_up, 0);
    *down = debounce(n, want_down, 1);
}

static void paint_sine(void)
{
    int i;

    status_set_decode(0);
    for (i = 0; i < 8; i++)
        status_set_digit((uint8_t)i, wave[(wave_off + i) & 7]);
}

static void serve_nudge(Input *npc_in)
{
    int y = pong_pad_y(1);
    int up = y > 0;
    int down = !up;

    npc_in->up = debounce(&npc[0], up, 0) != 0;
    npc_in->down = debounce(&npc[0], down, 1) != 0;
}

void title_reset(void)
{
    npc_reset(&npc[0]);
    npc_reset(&npc[1]);
    wave_off = 0;
    wave_pace = 0;
    serve_hold = SERVE_WAIT;
    waiting = 1;
    paint_sine();
}

void title_tick(const Input *real)
{
    Input npc_in;
    int u1;
    int d1;
    int u2;
    int d2;

    if (real->button_pressed) {
        pong_begin_play(1);
        return;
    }
    if (real->p2_button_pressed) {
        pong_begin_play(2);
        return;
    }

    memset(&npc_in, 0, sizeof npc_in);
    if (pong_awaiting_serve()) {
        if (!waiting) {
            npc_reset(&npc[0]);
            npc_reset(&npc[1]);
            serve_hold = SERVE_WAIT;
            waiting = 1;
        }
        if (serve_hold > DEBOUNCE)
            serve_hold--;
        else {
            serve_nudge(&npc_in);
            if (serve_hold > 0)
                serve_hold--;
        }
    } else {
        waiting = 0;
        think(&npc[0], pong_pad_y(1), &u1, &d1);
        think(&npc[1], pong_pad_y(2), &u2, &d2);
        npc_in.up = u1 != 0;
        npc_in.down = d1 != 0;
        npc_in.p2_up = u2 != 0;
        npc_in.p2_down = d2 != 0;
    }
    pong_play_tick(&npc_in);

    wave_pace++;
    if (wave_pace >= WAVE_STEP) {
        wave_pace = 0;
        wave_off = (wave_off + 1) & 7;
    }
    paint_sine();
}
