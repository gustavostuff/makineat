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
} OverStyle;

static int over_frame;
static int mark;
static OverStyle style;
static int text_scroll;
static int text_dir;
static int text_wait;
static int text_step;
static const char *shooter_msg;

static void text_scroll_reset(void)
{
    text_scroll = 0;
    text_dir = -1;
    text_wait = OVER_TEXT_DELAY;
    text_step = 0;
}

static void text_scroll_tick(void)
{
    int min_x;
    int len;

    if (shooter_msg == 0)
        return;
    if (text_wait > 0) {
        text_wait--;
        return;
    }
    text_step++;
    if (text_step < OVER_TEXT_STEP)
        return;
    text_step = 0;

    len = (int)strlen(shooter_msg);
    min_x = 16 - len * TEXT_W;
    if (min_x > 0)
        min_x = 0;

    text_scroll += text_dir;
    if (text_scroll <= min_x) {
        text_scroll = min_x;
        text_dir = 1;
        text_wait = OVER_TEXT_DELAY;
    } else if (text_scroll >= 0) {
        text_scroll = 0;
        text_dir = -1;
        text_wait = OVER_TEXT_DELAY;
    }
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
    text_draw(1, 8, buf + i);
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
        text_draw(2, 5, "new");
        text_draw(-4, 10, "record");
        return;
    }
    if (style == OVER_STYLE_SHOOTER_WIN) {
        draw_face();
        return;
    }
    if (style == OVER_STYLE_SHOOTER_LOSE) {
        draw_face();
        if (shooter_msg != 0)
            text_draw(text_scroll, 9, shooter_msg);
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
    text_scroll_reset();
    shooter_msg = 0;
}

void over_start_mark(int judge)
{
    style = OVER_STYLE_DEFAULT;
    shooter_msg = 0;
    text_scroll_reset();
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

    shooter_msg = 0;
    text_scroll_reset();
    if (cmp > 0) {
        style = OVER_STYLE_SHOOTER_RECORD;
        mark = 0;
    } else if (player > rocks) {
        style = OVER_STYLE_SHOOTER_WIN;
        mark = 1;
    } else {
        style = OVER_STYLE_SHOOTER_LOSE;
        mark = -1;
        shooter_msg = "better luck next time";
    }
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

int over_tick(const Input *in)
{
    over_frame++;
    if (style == OVER_STYLE_SHOOTER_LOSE)
        text_scroll_tick();
    draw_over();
    return in->button_pressed ? 1 : 0;
}
