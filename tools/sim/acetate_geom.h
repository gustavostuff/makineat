#ifndef ACETATE_GEOM_H
#define ACETATE_GEOM_H

#include <stdint.h>

#define ACETATE_GEOM_GRID 16
#define ACETATE_GEOM_DIGITS 8

int acetate_geom_field_px(int led, int pitch, int grid);

void acetate_geom_led_rect(int field_x, int field_y, int led, int pitch, int grid, int col,
                           int row, int *x, int *y, int *w, int *h);

/* Returns 1 and writes col/row when (x,y) hits a cell (LED plus trailing pitch margin). */
int acetate_geom_led_at(int field_x, int field_y, int led, int pitch, int grid, int x, int y,
                        int *col, int *row);

void acetate_geom_digit_rect(int origin_x, int origin_y, int digit_w, int digit_h, int index,
                             int *x, int *y, int *w, int *h);

/* Returns digit index, or -1. */
int acetate_geom_digit_at(int origin_x, int origin_y, int digit_w, int digit_h, int n, int x,
                          int y);

int acetate_geom_color_at(const uint8_t led[ACETATE_GEOM_GRID][ACETATE_GEOM_GRID],
                          const uint8_t digit[ACETATE_GEOM_DIGITS], int field_x, int field_y,
                          int led_px, int pitch, int grid, int digit_x, int digit_y, int digit_w,
                          int digit_h, int digit_n, int x, int y);

int acetate_geom_parse_layout(const char *json, uint8_t led[ACETATE_GEOM_GRID][ACETATE_GEOM_GRID],
                              uint8_t digit[ACETATE_GEOM_DIGITS], int *active);

int acetate_geom_rects_overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh);

/* Paint every LED/digit whose cell intersects the canvas box. Returns 1 if anything changed. */
int acetate_geom_fill_overlap(uint8_t led[ACETATE_GEOM_GRID][ACETATE_GEOM_GRID],
                              uint8_t digit[ACETATE_GEOM_DIGITS], int field_x, int field_y,
                              int led_px, int pitch, int grid, int digit_x, int digit_y,
                              int digit_w, int digit_h, int digit_n, int rx, int ry, int rw, int rh,
                              uint8_t color);

/* Bounding box of cells that intersect the canvas box. Returns 1 if any cell hits. */
int acetate_geom_union_overlap(int field_x, int field_y, int led_px, int pitch, int grid,
                               int digit_x, int digit_y, int digit_w, int digit_h, int digit_n,
                               int rx, int ry, int rw, int rh, int *ox, int *oy, int *ow, int *oh);

#endif
