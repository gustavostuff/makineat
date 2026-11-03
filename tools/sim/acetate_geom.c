#include "acetate_geom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int acetate_geom_field_px(int led, int pitch, int grid)
{
    if (grid < 1)
        return 0;
    if (grid == 1)
        return led > 0 ? led : 0;
    if (pitch < 1)
        pitch = led;
    return (grid - 1) * pitch + led;
}

void acetate_geom_led_rect(int field_x, int field_y, int led, int pitch, int grid, int col,
                           int row, int *x, int *y, int *w, int *h)
{
    int cell_w;
    int cell_h;

    if (pitch < led)
        pitch = led;
    cell_w = (col < grid - 1) ? pitch : led;
    cell_h = (row < grid - 1) ? pitch : led;
    if (x != NULL)
        *x = field_x + col * pitch;
    if (y != NULL)
        *y = field_y + row * pitch;
    if (w != NULL)
        *w = cell_w;
    if (h != NULL)
        *h = cell_h;
}

int acetate_geom_led_at(int field_x, int field_y, int led, int pitch, int grid, int x, int y,
                        int *col, int *row)
{
    int dx;
    int dy;
    int span;
    int c;
    int r;

    if (grid < 1 || led < 1)
        return 0;
    if (pitch < 1)
        pitch = led;
    if (x < field_x || y < field_y)
        return 0;
    dx = x - field_x;
    dy = y - field_y;
    span = acetate_geom_field_px(led, pitch, grid);
    if (dx >= span || dy >= span)
        return 0;
    c = dx / pitch;
    r = dy / pitch;
    if (c >= grid)
        c = grid - 1;
    if (r >= grid)
        r = grid - 1;
    if (col != NULL)
        *col = c;
    if (row != NULL)
        *row = r;
    return 1;
}

void acetate_geom_digit_rect(int origin_x, int origin_y, int digit_w, int digit_h, int index,
                             int *x, int *y, int *w, int *h)
{
    if (x != NULL)
        *x = origin_x + index * digit_w;
    if (y != NULL)
        *y = origin_y;
    if (w != NULL)
        *w = digit_w;
    if (h != NULL)
        *h = digit_h;
}

int acetate_geom_digit_at(int origin_x, int origin_y, int digit_w, int digit_h, int n, int x,
                          int y)
{
    int dx;

    if (n < 1 || digit_w < 1 || digit_h < 1)
        return -1;
    if (x < origin_x || y < origin_y || y >= origin_y + digit_h)
        return -1;
    dx = x - origin_x;
    if (dx < 0 || dx >= n * digit_w)
        return -1;
    return dx / digit_w;
}

int acetate_geom_color_at(const uint8_t led[ACETATE_GEOM_GRID][ACETATE_GEOM_GRID],
                          const uint8_t digit[ACETATE_GEOM_DIGITS], int field_x, int field_y,
                          int led_px, int pitch, int grid, int digit_x, int digit_y, int digit_w,
                          int digit_h, int digit_n, int x, int y)
{
    int col;
    int row;
    int d;

    if (led != NULL && acetate_geom_led_at(field_x, field_y, led_px, pitch, grid, x, y, &col, &row))
        return (int)led[row][col];
    if (digit != NULL) {
        d = acetate_geom_digit_at(digit_x, digit_y, digit_w, digit_h, digit_n, x, y);
        if (d >= 0)
            return (int)digit[d];
    }
    return 0;
}

static int parse_quoted(const char *json, const char *key, char *out, int cap)
{
    const char *p;
    char needle[32];
    int i;

    snprintf(needle, sizeof needle, "\"%s\"", key);
    p = strstr(json, needle);
    if (p == NULL)
        return -1;
    p = strchr(p + strlen(needle), '"');
    if (p == NULL)
        return -1;
    p++;
    for (i = 0; i < cap; i++) {
        if (p[i] == '\0' || p[i] == '"')
            return -1;
        out[i] = p[i];
    }
    return 0;
}

int acetate_geom_rects_overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    if (aw < 1 || ah < 1 || bw < 1 || bh < 1)
        return 0;
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static int visit_overlap(int field_x, int field_y, int led_px, int pitch, int grid, int digit_x,
                         int digit_y, int digit_w, int digit_h, int digit_n, int rx, int ry, int rw,
                         int rh, uint8_t led[ACETATE_GEOM_GRID][ACETATE_GEOM_GRID],
                         uint8_t digit[ACETATE_GEOM_DIGITS], uint8_t color, int paint, int *ox,
                         int *oy, int *ow, int *oh)
{
    int col;
    int row;
    int i;
    int x;
    int y;
    int w;
    int h;
    int hit = 0;
    int minx = 0;
    int miny = 0;
    int maxx = 0;
    int maxy = 0;
    int changed = 0;

    if (grid > ACETATE_GEOM_GRID)
        grid = ACETATE_GEOM_GRID;
    if (digit_n > ACETATE_GEOM_DIGITS)
        digit_n = ACETATE_GEOM_DIGITS;

    for (row = 0; row < grid; row++) {
        for (col = 0; col < grid; col++) {
            acetate_geom_led_rect(field_x, field_y, led_px, pitch, grid, col, row, &x, &y, &w, &h);
            if (!acetate_geom_rects_overlap(rx, ry, rw, rh, x, y, w, h))
                continue;
            if (!hit) {
                minx = x;
                miny = y;
                maxx = x + w;
                maxy = y + h;
                hit = 1;
            } else {
                if (x < minx)
                    minx = x;
                if (y < miny)
                    miny = y;
                if (x + w > maxx)
                    maxx = x + w;
                if (y + h > maxy)
                    maxy = y + h;
            }
            if (paint && led != NULL && led[row][col] != color) {
                led[row][col] = color;
                changed = 1;
            }
        }
    }
    for (i = 0; i < digit_n; i++) {
        acetate_geom_digit_rect(digit_x, digit_y, digit_w, digit_h, i, &x, &y, &w, &h);
        if (!acetate_geom_rects_overlap(rx, ry, rw, rh, x, y, w, h))
            continue;
        if (!hit) {
            minx = x;
            miny = y;
            maxx = x + w;
            maxy = y + h;
            hit = 1;
        } else {
            if (x < minx)
                minx = x;
            if (y < miny)
                miny = y;
            if (x + w > maxx)
                maxx = x + w;
            if (y + h > maxy)
                maxy = y + h;
        }
        if (paint && digit != NULL && digit[i] != color) {
            digit[i] = color;
            changed = 1;
        }
    }
    if (hit && ox != NULL && oy != NULL && ow != NULL && oh != NULL) {
        *ox = minx;
        *oy = miny;
        *ow = maxx - minx;
        *oh = maxy - miny;
    }
    return paint ? changed : hit;
}

int acetate_geom_fill_overlap(uint8_t led[ACETATE_GEOM_GRID][ACETATE_GEOM_GRID],
                              uint8_t digit[ACETATE_GEOM_DIGITS], int field_x, int field_y,
                              int led_px, int pitch, int grid, int digit_x, int digit_y,
                              int digit_w, int digit_h, int digit_n, int rx, int ry, int rw, int rh,
                              uint8_t color)
{
    return visit_overlap(field_x, field_y, led_px, pitch, grid, digit_x, digit_y, digit_w, digit_h,
                         digit_n, rx, ry, rw, rh, led, digit, color, 1, NULL, NULL, NULL, NULL);
}

int acetate_geom_union_overlap(int field_x, int field_y, int led_px, int pitch, int grid,
                               int digit_x, int digit_y, int digit_w, int digit_h, int digit_n,
                               int rx, int ry, int rw, int rh, int *ox, int *oy, int *ow, int *oh)
{
    return visit_overlap(field_x, field_y, led_px, pitch, grid, digit_x, digit_y, digit_w, digit_h,
                         digit_n, rx, ry, rw, rh, NULL, NULL, 0, 0, ox, oy, ow, oh);
}

int acetate_geom_parse_layout(const char *json, uint8_t led[ACETATE_GEOM_GRID][ACETATE_GEOM_GRID],
                              uint8_t digit[ACETATE_GEOM_DIGITS], int *active)
{
    char led_s[ACETATE_GEOM_GRID * ACETATE_GEOM_GRID];
    char digit_s[ACETATE_GEOM_DIGITS];
    const char *p;
    int i;
    int v;

    if (led != NULL)
        memset(led, 0, ACETATE_GEOM_GRID * ACETATE_GEOM_GRID);
    if (digit != NULL)
        memset(digit, 0, ACETATE_GEOM_DIGITS);
    if (active != NULL)
        *active = 1;
    if (json == NULL)
        return -1;
    p = strstr(json, "\"active\"");
    if (p != NULL && active != NULL) {
        p = strchr(p, ':');
        if (p != NULL) {
            v = atoi(p + 1);
            if (v >= 0)
                *active = v;
        }
    }
    if (parse_quoted(json, "led", led_s, (int)sizeof led_s) != 0)
        return -1;
    if (parse_quoted(json, "digit", digit_s, (int)sizeof digit_s) != 0)
        return -1;
    if (led != NULL) {
        for (i = 0; i < ACETATE_GEOM_GRID * ACETATE_GEOM_GRID; i++) {
            if (led_s[i] < '0' || led_s[i] > '7')
                return -1;
            led[i / ACETATE_GEOM_GRID][i % ACETATE_GEOM_GRID] = (uint8_t)(led_s[i] - '0');
        }
    }
    if (digit != NULL) {
        for (i = 0; i < ACETATE_GEOM_DIGITS; i++) {
            if (digit_s[i] < '0' || digit_s[i] > '7')
                return -1;
            digit[i] = (uint8_t)(digit_s[i] - '0');
        }
    }
    return 0;
}
