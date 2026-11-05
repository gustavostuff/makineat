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

static int text_scale_norm(int scale)
{
    if (scale <= 1)
        return TEXT_SCALE_1;
    if (scale >= TEXT_SCALE_2)
        return TEXT_SCALE_2;
    return scale;
}

static int text_glyph_advance(int scale)
{
    return TEXT_W * text_scale_norm(scale);
}

static int text_glyph_height(int scale)
{
    return TEXT_H * text_scale_norm(scale);
}

static int text_pixel_w(const char *s, int scale)
{
    int len;

    if (s == 0)
        return 0;
    len = (int)strlen(s);
    return len * text_glyph_advance(scale);
}

/* Left, right, up, down, then the four corners. */
static const int8_t outline_dx[8] = { -1, 1, 0, 0, -1, 1, -1, 1 };
static const int8_t outline_dy[8] = { 0, 0, -1, 1, -1, -1, 1, 1 };

static void stamp(int x, int y, const uint8_t *rows, int scale, bool on)
{
    int row;
    int col;
    int s;

    scale = text_scale_norm(scale);
    for (row = 0; row < TEXT_H; row++) {
        for (col = 0; col < TEXT_W; col++) {
            int px;
            int py;

            if ((rows[row] & (uint8_t)(1u << col)) == 0)
                continue;
            px = x + col * scale;
            py = y + row * scale;
            for (s = 0; s < scale * scale; s++) {
                int sx = s % scale;
                int sy = s / scale;
                int fx = px + sx;
                int fy = py + sy;

                if (fx < 0 || fy < 0 || fx >= TEXT_FIELD_W || fy >= TEXT_FIELD_W)
                    continue;
                pixel((uint8_t)fx, (uint8_t)fy, on);
            }
        }
    }
}

static void blit(int x, int y, const uint8_t *rows, int scale)
{
    stamp(x, y, rows, scale, true);
}

void text_draw_outline(int x, int y, const char *s, int invert)
{
    const char *p;
    int gx;
    int i;
    bool ring = invert != 0;
    bool fill = invert == 0;

    if (s == 0)
        return;

    for (i = 0; i < 8; i++) {
        gx = x;
        for (p = s; *p != '\0'; p++) {
            const uint8_t *rows = text_glyph(*p);

            if (rows == 0)
                continue;
            if (gx >= TEXT_FIELD_W)
                break;
            if (gx + TEXT_W > 0)
                stamp(gx + outline_dx[i], y + outline_dy[i], rows, TEXT_SCALE_1, ring);
            gx += TEXT_W;
        }
    }

    gx = x;
    for (p = s; *p != '\0'; p++) {
        const uint8_t *rows = text_glyph(*p);

        if (rows == 0)
            continue;
        if (gx >= TEXT_FIELD_W)
            break;
        if (gx + TEXT_W > 0)
            stamp(gx, y, rows, TEXT_SCALE_1, fill);
        gx += TEXT_W;
    }
}

void text_print_outline(int x, int y, const char *s)
{
    text_draw_outline(x, y, s, 0);
}

static int blink_period(const TextBlink *tb)
{
    int fps = tb->fps;

    if (fps <= 0)
        fps = TEXT_BLINK_FPS_DEFAULT;
    if (fps > TEXT_FRAME_HZ)
        fps = TEXT_FRAME_HZ;
    return TEXT_FRAME_HZ / fps;
}

void text_blink_start(TextBlink *tb)
{
    if (tb == 0)
        return;
    if (tb->fps <= 0)
        tb->fps = TEXT_BLINK_FPS_DEFAULT;
    tb->step = 0;
    tb->invert = 0;
}

void text_blink_advance(TextBlink *tb)
{
    int period;

    if (tb == 0)
        return;
    period = blink_period(tb);
    tb->step++;
    if (tb->step >= period) {
        tb->step = 0;
        tb->invert = tb->invert ? 0 : 1;
    }
}

void text_print_outline_blink(int x, int y, const char *s, TextBlink *tb)
{
    text_blink_advance(tb);
    text_draw_outline(x, y, s, tb != 0 ? tb->invert : 0);
}

static void stamp_clip(int x, int y, const uint8_t *rows, int scale, int clip_x, int clip_y,
                       int clip_w, int clip_h, bool on)
{
    int row;
    int col;
    int s;

    scale = text_scale_norm(scale);
    for (row = 0; row < TEXT_H; row++) {
        for (col = 0; col < TEXT_W; col++) {
            int px;
            int py;

            if ((rows[row] & (uint8_t)(1u << col)) == 0)
                continue;
            px = x + col * scale;
            py = y + row * scale;
            for (s = 0; s < scale * scale; s++) {
                int sx = s % scale;
                int sy = s / scale;
                int fx = px + sx;
                int fy = py + sy;

                if (fx < clip_x || fy < clip_y || fx >= clip_x + clip_w ||
                    fy >= clip_y + clip_h)
                    continue;
                if (fx < 0 || fy < 0 || fx >= TEXT_FIELD_W || fy >= TEXT_FIELD_W)
                    continue;
                pixel((uint8_t)fx, (uint8_t)fy, on);
            }
        }
    }
}

static void blit_clip(int x, int y, const uint8_t *rows, int scale, int clip_x, int clip_y,
                      int clip_w, int clip_h)
{
    stamp_clip(x, y, rows, scale, clip_x, clip_y, clip_w, clip_h, true);
}

void text_draw_scaled(int x, int y, int scale, const char *s)
{
    int advance;

    if (s == 0)
        return;
    scale = text_scale_norm(scale);
    advance = text_glyph_advance(scale);
    for (; *s != '\0'; s++) {
        const uint8_t *rows = text_glyph(*s);

        if (rows == 0)
            continue;
        if (x >= TEXT_FIELD_W)
            break;
        if (x + advance > 0)
            blit(x, y, rows, scale);
        x += advance;
    }
}

void text_print_scaled(int x, int y, int scale, const char *s)
{
    text_draw_scaled(x, y, scale, s);
}

void text_draw(int x, int y, const char *s)
{
    text_draw_scaled(x, y, TEXT_SCALE_1, s);
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

static int scroll_scale(const TextScroll *ts)
{
    if (ts == 0)
        return TEXT_SCALE_1;
    return text_scale_norm(ts->scale);
}

static int scroll_view_h(const TextScroll *ts)
{
    if (ts->viewport_h > 0)
        return ts->viewport_h;
    return text_glyph_height(scroll_scale(ts));
}

static void scroll_apply_defaults(TextScroll *ts)
{
    if (ts->viewport_w <= 0)
        ts->viewport_w = TEXT_SCROLL_VIEW_W_DEFAULT;
    if (ts->viewport_h <= 0)
        ts->viewport_h = text_glyph_height(scroll_scale(ts));
    if (ts->speed_fps <= 0)
        ts->speed_fps = TEXT_SCROLL_SPEED_FPS_DEFAULT;
    if (ts->pause_frames <= 0)
        ts->pause_frames = TEXT_SCROLL_PAUSE_DEFAULT;
    if (ts->scale <= 0)
        ts->scale = TEXT_SCALE_1;
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
    min_x = view_w - text_pixel_w(ts->text, scroll_scale(ts));
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
    int advance;
    int scale;
    const char *s;

    if (ts == 0 || ts->text == 0)
        return;

    scale = scroll_scale(ts);
    advance = text_glyph_advance(scale);
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
        if (x + advance > ts->x)
            blit_clip(x, ts->y, rows, scale, ts->x, ts->y, clip_w, clip_h);
        x += advance;
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

static void scroll_paint_outline(const TextScroll *ts, int invert)
{
    int clip_w;
    int clip_h;
    int ring_x;
    int ring_y;
    int ring_w;
    int ring_h;
    int x;
    int i;
    int advance;
    int scale;
    const char *s;
    bool ring = invert != 0;
    bool fill = invert == 0;

    if (ts == 0 || ts->text == 0)
        return;

    scale = scroll_scale(ts);
    advance = text_glyph_advance(scale);
    clip_w = scroll_view_w(ts);
    clip_h = scroll_view_h(ts);
    ring_x = ts->x - 1;
    ring_y = ts->y - 1;
    ring_w = clip_w + 2;
    ring_h = clip_h + 2;

    for (i = 0; i < 8; i++) {
        x = ts->x + ts->scroll_x;
        for (s = ts->text; *s != '\0'; s++) {
            const uint8_t *rows = text_glyph(*s);

            if (rows == 0)
                continue;
            if (x >= TEXT_FIELD_W + 1)
                break;
            if (x + advance + 1 > ts->x)
                stamp_clip(x + outline_dx[i], ts->y + outline_dy[i], rows, scale, ring_x, ring_y,
                           ring_w, ring_h, ring);
            x += advance;
        }
    }

    x = ts->x + ts->scroll_x;
    for (s = ts->text; *s != '\0'; s++) {
        const uint8_t *rows = text_glyph(*s);

        if (rows == 0)
            continue;
        if (x >= TEXT_FIELD_W)
            break;
        if (x + advance > ts->x)
            stamp_clip(x, ts->y, rows, scale, ts->x, ts->y, clip_w, clip_h, fill);
        x += advance;
    }
}

void text_scroll_paint_outline(const TextScroll *ts, int invert)
{
    scroll_paint_outline(ts, invert);
}

void print_scroll(TextScroll *ts)
{
    scroll_advance(ts);
    scroll_paint(ts);
}

void print_scroll_outline(TextScroll *ts)
{
    scroll_advance(ts);
    scroll_paint_outline(ts, 0);
}

void print_scroll_outline_blink(TextScroll *ts, TextBlink *tb)
{
    scroll_advance(ts);
    text_blink_advance(tb);
    scroll_paint_outline(ts, tb != 0 ? tb->invert : 0);
}
