#include "over.h"

#include "frame.h"
#include "score.h"
#include "text.h"

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
static TextScroll lose_scroll;

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
    text_print(1, 8, buf + i);
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
        text_print(2, 5, "new");
        text_print(-4, 10, "record");
        return;
    }
    if (style == OVER_STYLE_SHOOTER_WIN) {
        draw_face();
        return;
    }
    if (style == OVER_STYLE_SHOOTER_LOSE) {
        draw_face();
        text_scroll_paint(&lose_scroll);
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
    text_scroll_stop(&lose_scroll);
}

void over_start_mark(int judge)
{
    style = OVER_STYLE_DEFAULT;
    text_scroll_stop(&lose_scroll);
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

    text_scroll_stop(&lose_scroll);
    if (cmp > 0) {
        style = OVER_STYLE_SHOOTER_RECORD;
        mark = 0;
    } else if (player > rocks) {
        style = OVER_STYLE_SHOOTER_WIN;
        mark = 1;
    } else {
        style = OVER_STYLE_SHOOTER_LOSE;
        mark = -1;
        lose_scroll.repeat = TEXT_SCROLL_BOUNCE;
        text_scroll_start(&lose_scroll, 0, 9, "better luck next time");
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
        text_scroll_advance(&lose_scroll);
    draw_over();
    return in->button_pressed ? 1 : 0;
}
