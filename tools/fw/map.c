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

void map_rect(int x, int y, int w, int h, int on)
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
            map_set(col, row, on);
    }
}

void map_stamp_frame(void)
{
    int y;
    int x;

    for (y = 0; y < FIELD_SIZE; y++) {
        for (x = 0; x < FIELD_SIZE; x++) {
            if (map_get(x, y))
                pixel((uint8_t)x, (uint8_t)y, true);
        }
    }
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
