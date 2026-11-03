#include "text.h"

#include "frame.h"

#include <string.h>

/* Bit 0 of a row is the left pixel.
 * Each glyph has 1px margin on the right (col 3) and bottom (row 3). */
static const uint8_t digits[10][TEXT_H] = {
    {0x07, 0x05, 0x07, 0x00}, /* 0 */
    {0x03, 0x02, 0x07, 0x00}, /* 1 */
    {0x03, 0x02, 0x06, 0x00}, /* 2 */
    {0x07, 0x06, 0x07, 0x00}, /* 3 */
    {0x05, 0x07, 0x04, 0x00}, /* 4 */
    {0x06, 0x02, 0x03, 0x00}, /* 5 */
    {0x01, 0x07, 0x07, 0x00}, /* 6 */
    {0x07, 0x04, 0x04, 0x00}, /* 7 */
    {0x02, 0x07, 0x07, 0x00}, /* 8 */
    {0x07, 0x07, 0x04, 0x00}, /* 9 */
};

static const uint8_t letters[26][TEXT_H] = {
    {0x06, 0x05, 0x07, 0x00}, /* A */
    {0x01, 0x07, 0x07, 0x00}, /* B */
    {0x07, 0x01, 0x07, 0x00}, /* C */
    {0x04, 0x07, 0x07, 0x00}, /* D */
    {0x07, 0x03, 0x07, 0x00}, /* E */
    {0x07, 0x03, 0x01, 0x00}, /* F */
    {0x03, 0x05, 0x07, 0x00}, /* G */
    {0x05, 0x07, 0x05, 0x00}, /* H */
    {0x07, 0x02, 0x07, 0x00}, /* I */
    {0x07, 0x02, 0x03, 0x00}, /* J */
    {0x05, 0x03, 0x05, 0x00}, /* K */
    {0x01, 0x01, 0x07, 0x00}, /* L */
    {0x03, 0x06, 0x04, 0x00}, /* M */
    {0x07, 0x05, 0x05, 0x00}, /* N */
    {0x07, 0x05, 0x07, 0x00}, /* O */
    {0x07, 0x07, 0x01, 0x00}, /* P */
    {0x07, 0x07, 0x04, 0x00}, /* Q */
    {0x07, 0x03, 0x05, 0x00}, /* R */
    {0x06, 0x02, 0x03, 0x00}, /* S */
    {0x07, 0x02, 0x02, 0x00}, /* T */
    {0x05, 0x05, 0x07, 0x00}, /* U */
    {0x05, 0x05, 0x02, 0x00}, /* V */
    {0x01, 0x03, 0x06, 0x00}, /* W */
    {0x05, 0x02, 0x05, 0x00}, /* X */
    {0x05, 0x02, 0x02, 0x00}, /* Y */
    {0x03, 0x02, 0x06, 0x00}, /* Z */
};

static const uint8_t dot[TEXT_H] = {0x00, 0x00, 0x01, 0x00};
static const uint8_t comma[TEXT_H] = {0x00, 0x00, 0x02, 0x01};
static const uint8_t colon[TEXT_H] = {0x02, 0x00, 0x02, 0x00};
static const uint8_t semicolon[TEXT_H] = {0x02, 0x00, 0x02, 0x01};
static const uint8_t dash[TEXT_H] = {0x00, 0x07, 0x00, 0x00};
static const uint8_t plus[TEXT_H] = {0x02, 0x07, 0x02, 0x00};
static const uint8_t equals[TEXT_H] = {0x07, 0x00, 0x07, 0x00};
static const uint8_t slash[TEXT_H] = {0x04, 0x02, 0x01, 0x00};
static const uint8_t blank[TEXT_H] = {0x00, 0x00, 0x00, 0x00};

const uint8_t *text_glyph(char c)
{
    if (c >= '0' && c <= '9')
        return digits[c - '0'];
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z')
        return letters[c - 'A'];
    if (c == '.')
        return dot;
    if (c == ',')
        return comma;
    if (c == ':')
        return colon;
    if (c == ';')
        return semicolon;
    if (c == '-')
        return dash;
    if (c == '+')
        return plus;
    if (c == '=')
        return equals;
    if (c == '/')
        return slash;
    if (c == ' ')
        return blank;
    return 0;
}

static int text_pixel_w(const char *s)
{
    int len;

    if (s == 0)
        return 0;
    len = (int)strlen(s);
    return len * TEXT_W;
}

static void blit(int x, int y, const uint8_t *rows)
{
    int row;
    int col;

    for (row = 0; row < TEXT_H; row++) {
        for (col = 0; col < TEXT_W; col++) {
            int px;
            int py;

            if ((rows[row] & (uint8_t)(1u << col)) == 0)
                continue;
            px = x + col;
            py = y + row;
            if (px < 0 || py < 0 || px >= TEXT_FIELD_W || py >= TEXT_FIELD_W)
                continue;
            pixel((uint8_t)px, (uint8_t)py, true);
        }
    }
}

static void blit_clip(int x, int y, const uint8_t *rows, int clip_x, int clip_y, int clip_w,
                      int clip_h)
{
    int row;
    int col;

    for (row = 0; row < TEXT_H; row++) {
        for (col = 0; col < TEXT_W; col++) {
            int px;
            int py;

            if ((rows[row] & (uint8_t)(1u << col)) == 0)
                continue;
            px = x + col;
            py = y + row;
            if (px < clip_x || py < clip_y || px >= clip_x + clip_w || py >= clip_y + clip_h)
                continue;
            if (px < 0 || py < 0 || px >= TEXT_FIELD_W || py >= TEXT_FIELD_W)
                continue;
            pixel((uint8_t)px, (uint8_t)py, true);
        }
    }
}

void text_draw(int x, int y, const char *s)
{
    if (s == 0)
        return;
    for (; *s != '\0'; s++) {
        const uint8_t *rows = text_glyph(*s);

        if (rows == 0)
            continue;
        if (x >= TEXT_FIELD_W)
            break;
        if (x + TEXT_W > 0)
            blit(x, y, rows);
        x += TEXT_W;
    }
}

void text_print(int x, int y, const char *s)
{
    text_draw(x, y, s);
}

static int scroll_step_period(const TextScroll *ts)
{
    int fps = ts->speed_fps;

    if (fps <= 0)
        fps = TEXT_SCROLL_SPEED_FPS_DEFAULT;
    if (fps > TEXT_FRAME_HZ)
        fps = TEXT_FRAME_HZ;
    return TEXT_FRAME_HZ / fps;
}

static int scroll_pause_frames(const TextScroll *ts)
{
    if (ts->pause_frames > 0)
        return ts->pause_frames;
    return TEXT_SCROLL_PAUSE_DEFAULT;
}

static int scroll_view_w(const TextScroll *ts)
{
    if (ts->viewport_w > 0)
        return ts->viewport_w;
    return TEXT_SCROLL_VIEW_W_DEFAULT;
}

static int scroll_view_h(const TextScroll *ts)
{
    if (ts->viewport_h > 0)
        return ts->viewport_h;
    return TEXT_SCROLL_VIEW_H_DEFAULT;
}

static void scroll_apply_defaults(TextScroll *ts)
{
    if (ts->viewport_w <= 0)
        ts->viewport_w = TEXT_SCROLL_VIEW_W_DEFAULT;
    if (ts->viewport_h <= 0)
        ts->viewport_h = TEXT_SCROLL_VIEW_H_DEFAULT;
    if (ts->speed_fps <= 0)
        ts->speed_fps = TEXT_SCROLL_SPEED_FPS_DEFAULT;
    if (ts->pause_frames <= 0)
        ts->pause_frames = TEXT_SCROLL_PAUSE_DEFAULT;
}

void text_scroll_start(TextScroll *ts, int x, int y, const char *text)
{
    if (ts == 0)
        return;
    scroll_apply_defaults(ts);
    ts->text = text;
    ts->x = x;
    ts->y = y;
    ts->scroll_x = 0;
    ts->dir = -1;
    ts->wait = scroll_pause_frames(ts);
    ts->step = 0;
    ts->pending_reset = 0;
}

void text_scroll_stop(TextScroll *ts)
{
    if (ts == 0)
        return;
    ts->text = 0;
    ts->pending_reset = 0;
}

static void scroll_advance(TextScroll *ts)
{
    int min_x;
    int period;
    int pause;
    int view_w;

    if (ts == 0 || ts->text == 0)
        return;

    pause = scroll_pause_frames(ts);
    if (ts->wait > 0) {
        ts->wait--;
        if (ts->wait == 0 && ts->pending_reset) {
            ts->scroll_x = 0;
            ts->pending_reset = 0;
            ts->dir = -1;
            ts->wait = pause;
        }
        return;
    }

    period = scroll_step_period(ts);
    ts->step++;
    if (ts->step < period)
        return;
    ts->step = 0;

    view_w = scroll_view_w(ts);
    min_x = view_w - text_pixel_w(ts->text);
    if (min_x > 0)
        min_x = 0;

    ts->scroll_x += ts->dir;
    if (ts->scroll_x <= min_x) {
        ts->scroll_x = min_x;
        ts->wait = pause;
        if (ts->repeat == TEXT_SCROLL_RESET)
            ts->pending_reset = 1;
        else
            ts->dir = 1;
    } else if (ts->scroll_x >= 0) {
        ts->scroll_x = 0;
        ts->wait = pause;
        ts->dir = -1;
    }
}

static void scroll_paint(const TextScroll *ts)
{
    int clip_w;
    int clip_h;
    int x;
    const char *s;

    if (ts == 0 || ts->text == 0)
        return;

    clip_w = scroll_view_w(ts);
    clip_h = scroll_view_h(ts);
    x = ts->x + ts->scroll_x;
    s = ts->text;
    for (; *s != '\0'; s++) {
        const uint8_t *rows = text_glyph(*s);

        if (rows == 0)
            continue;
        if (x >= TEXT_FIELD_W)
            break;
        if (x + TEXT_W > ts->x)
            blit_clip(x, ts->y, rows, ts->x, ts->y, clip_w, clip_h);
        x += TEXT_W;
    }
}

void text_scroll_advance(TextScroll *ts)
{
    scroll_advance(ts);
}

void text_scroll_paint(const TextScroll *ts)
{
    scroll_paint(ts);
}

void print_scroll(TextScroll *ts)
{
    scroll_advance(ts);
    scroll_paint(ts);
}
