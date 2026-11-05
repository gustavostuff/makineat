#include "map.h"

#include "entity.h"
#include "frame.h"

static uint16_t map_rows[FIELD_SIZE];

void map_clear(void)
{
    int i;

    for (i = 0; i < FIELD_SIZE; i++)
        map_rows[i] = 0;
}

int map_get(int x, int y)
{
    if (x < 0 || y < 0 || x > FIELD_LAST || y > FIELD_LAST)
        return 0;
    return (map_rows[y] & (uint16_t)(1u << x)) != 0;
}

void map_set(int x, int y, int on)
{
    uint16_t bit;

    if (x < 0 || y < 0 || x > FIELD_LAST || y > FIELD_LAST)
        return;
    bit = (uint16_t)(1u << x);
    if (on)
        map_rows[y] |= bit;
    else
        map_rows[y] &= (uint16_t)~bit;
}

void map_toggle(int x, int y)
{
    map_set(x, y, !map_get(x, y));
}

static uint16_t span_mask(int x, int x1)
{
    int col;
    uint16_t mask = 0;

    for (col = x; col <= x1; col++)
        mask |= (uint16_t)(1u << col);
    return mask;
}

void map_hline(int x, int y, int w, int on)
{
    int x1;

    if (w <= 0)
        return;
    x1 = x + w - 1;
    if (y < 0 || y > FIELD_LAST || x > FIELD_LAST || x1 < 0)
        return;
    if (x < 0)
        x = 0;
    if (x1 > FIELD_LAST)
        x1 = FIELD_LAST;
    if (x > x1)
        return;
    if (on)
        map_rows[y] |= span_mask(x, x1);
    else
        map_rows[y] &= (uint16_t)~span_mask(x, x1);
}

void map_vline(int x, int y, int h, int on)
{
    int y1;
    int row;

    if (h <= 0)
        return;
    y1 = y + h - 1;
    if (x < 0 || x > FIELD_LAST || y > FIELD_LAST || y1 < 0)
        return;
    if (y < 0)
        y = 0;
    if (y1 > FIELD_LAST)
        y1 = FIELD_LAST;
    for (row = y; row <= y1; row++)
        map_set(x, row, on);
}

void map_rect(int x, int y, int w, int h, int on)
{
    if (h == 1) {
        map_hline(x, y, w, on);
        return;
    }
    if (w == 1) {
        map_vline(x, y, h, on);
        return;
    }
    {
        int row;
        int y1 = y + h - 1;

        if (w <= 0 || h <= 0)
            return;
        if (x > FIELD_LAST || y > FIELD_LAST || x + w - 1 < 0 || y1 < 0)
            return;
        if (y < 0)
            y = 0;
        if (y1 > FIELD_LAST)
            y1 = FIELD_LAST;
        for (row = y; row <= y1; row++)
            map_hline(x, row, w, on);
    }
}

void map_stamp_frame(void)
{
    int y;

    for (y = 0; y < FIELD_SIZE; y++)
        frame[y] |= map_rows[y];
}

int map_solid(int x, int y)
{
    return map_get(x, y);
}

int scene_blocked(int x, int y, unsigned mask)
{
    if ((mask & SCENE_MAP) != 0 && map_solid(x, y))
        return 1;
    if ((mask & SCENE_ENTITIES) != 0 && entity_at(x, y) >= 0)
        return 1;
    return 0;
}
