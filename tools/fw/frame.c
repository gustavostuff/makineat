#include "frame.h"

uint16_t frame[FIELD_SIZE];

void pixel(uint8_t x, uint8_t y, bool on)
{
    uint16_t bit;

    if (x > FIELD_LAST || y > FIELD_LAST)
        return;
    bit = (uint16_t)(1u << x);
    if (on)
        frame[y] |= bit;
    else
        frame[y] &= (uint16_t)~bit;
}

bool pixel_get(uint8_t x, uint8_t y)
{
    if (x > FIELD_LAST || y > FIELD_LAST)
        return false;
    return (frame[y] & (uint16_t)(1u << x)) != 0;
}

void clear(void)
{
    int i;

    for (i = 0; i < FIELD_SIZE; i++)
        frame[i] = 0;
}

void frame_rect(int x, int y, int w, int h, bool on)
{
    int row;
    int col;
    int x1;
    int y1;

    if (w <= 0 || h <= 0)
        return;
    x1 = x + w - 1;
    y1 = y + h - 1;
    if (x > FIELD_LAST || y > FIELD_LAST || x1 < 0 || y1 < 0)
        return;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x1 > FIELD_LAST)
        x1 = FIELD_LAST;
    if (y1 > FIELD_LAST)
        y1 = FIELD_LAST;
    for (row = y; row <= y1; row++) {
        for (col = x; col <= x1; col++)
            pixel((uint8_t)col, (uint8_t)row, on);
    }
}

void frame_hline(int x, int y, int w, bool on)
{
    frame_rect(x, y, w, 1, on);
}

void frame_vline(int x, int y, int h, bool on)
{
    frame_rect(x, y, 1, h, on);
}
