#include "over.h"

#include "frame.h"
#include "score.h"
#include "text.h"

#include <string.h>

/* Rows of the end picture. Bit 0 is the left pixel.
 * The mouth is the only part that changes. */
static const uint16_t face_top[4] = {0x0810, 0x1248, 0x1008, 0x1008};
static const uint16_t mouth[3][2] = {
    {0x1188, 0x0240},
    {0x13c8, 0x0000},
    {0x1428, 0x03c0},
};

typedef enum {
    OVER_STYLE_DEFAULT,
    OVER_STYLE_SHOOTER_RECORD,
    OVER_STYLE_SHOOTER_WIN,
    OVER_STYLE_SHOOTER_LOSE,
    OVER_STYLE_PONG_WIN,
} OverStyle;

static int over_frame;
static int mark;
static OverStyle style;
static int pong_winner;
static TextScroll shooter_scroll;

#define SHOOTER_MSG_Y_CENTER 7
#define SHOOTER_MSG_Y_BELOW_FACE 10
/* Game tick runs at 60 Hz (see docs/firmware.md). */
#define OVER_SHOOTER_MIN_FRAMES 72

static void shooter_scroll_begin(const char *msg, int y)
{
    text_scroll_stop(&shooter_scroll);
    memset(&shooter_scroll, 0, sizeof shooter_scroll);
    shooter_scroll.repeat = TEXT_SCROLL_BOUNCE;
    text_scroll_start(&shooter_scroll, 0, y, msg);
}

static void paint(int y, uint16_t bits)
{
    int x;

    for (x = 0; x < 16; x++) {
        if ((bits & (uint16_t)(1u << x)) != 0)
            pixel((uint8_t)x, (uint8_t)y, true);
    }
}

static void draw_points(void)
{
    char buf[5];
    int n = score_get();
    int i = 4;

    buf[4] = '\0';
    if (n < 0)
        n = 0;
    do {
        buf[--i] = (char)('0' + (n % 10));
        n /= 10;
    } while (n > 0 && i > 0);
    text_print(1, 8, buf + i, TEXT_CHAR_SPACING_DEFAULT);
}

static void draw_face(void)
{
    const uint16_t *rows = mouth[mark + 1];
    int i;

    for (i = 0; i < 4; i++)
        paint(i, face_top[i]);
    paint(4, rows[0]);
    paint(5, rows[1]);
}

static void draw_over(void)
{
    clear();
    if (style == OVER_STYLE_SHOOTER_RECORD) {
        draw_face();
        text_scroll_paint(&shooter_scroll);
        return;
    }
    if (style == OVER_STYLE_SHOOTER_WIN) {
        draw_face();
        return;
    }
    if (style == OVER_STYLE_SHOOTER_LOSE) {
        draw_face();
        text_scroll_paint(&shooter_scroll);
        return;
    }
    if (style == OVER_STYLE_PONG_WIN) {
        text_print(1, 1, pong_winner == 1 ? "P1" : "P2", TEXT_CHAR_SPACING_DEFAULT);
        text_print(1, 5, "wins", TEXT_CHAR_SPACING_DEFAULT);
        return;
    }
    draw_face();
    draw_points();
}

void over_clear(void)
{
    over_frame = 0;
    mark = 0;
    style = OVER_STYLE_DEFAULT;
    text_scroll_stop(&shooter_scroll);
}

void over_start_mark(int judge)
{
    style = OVER_STYLE_DEFAULT;
    text_scroll_stop(&shooter_scroll);
    if (judge > 0)
        mark = 1;
    else if (judge < 0)
        mark = -1;
    else
        mark = 0;
    over_frame = 0;
    draw_over();
}

void over_start_shooter(int player, int rocks)
{
    int cmp = score_judge();

    text_scroll_stop(&shooter_scroll);
    if (cmp > 0) {
        style = OVER_STYLE_SHOOTER_RECORD;
        mark = 1;
        shooter_scroll_begin("new record", SHOOTER_MSG_Y_CENTER);
    } else if (player > rocks) {
        style = OVER_STYLE_SHOOTER_WIN;
        mark = 1;
    } else {
        style = OVER_STYLE_SHOOTER_LOSE;
        mark = -1;
        shooter_scroll_begin("better luck next time", SHOOTER_MSG_Y_BELOW_FACE);
    }
    over_frame = 0;
    draw_over();
}

void over_start_pong(int winner)
{
    style = OVER_STYLE_PONG_WIN;
    pong_winner = winner == 2 ? 2 : 1;
    over_frame = 0;
    draw_over();
}

void over_start(void)
{
    over_start_mark(score_judge());
}

void over_celebrate(void)
{
    mark = 1;
    over_frame = 0;
    draw_over();
}

void over_paint(void)
{
    draw_over();
}

static int shooter_over(void)
{
    return style == OVER_STYLE_SHOOTER_RECORD || style == OVER_STYLE_SHOOTER_WIN ||
           style == OVER_STYLE_SHOOTER_LOSE;
}

int over_tick(const Input *in)
{
    int pressed;

    over_frame++;
    if (style == OVER_STYLE_SHOOTER_RECORD || style == OVER_STYLE_SHOOTER_LOSE)
        text_scroll_advance(&shooter_scroll);
    draw_over();
    if (style == OVER_STYLE_PONG_WIN)
        pressed = in->button_pressed || in->p2_button_pressed;
    else
        pressed = in->button_pressed;
    if (!pressed)
        return 0;
    if (shooter_over() && over_frame < OVER_SHOOTER_MIN_FRAMES)
        return 0;
    return 1;
}
