#include "entity.h"

#include "frame.h"

#include <stddef.h>
#include <stdint.h>

#define GRID 32

typedef struct Slot {
    Entity e;
    uint8_t live;
} Slot;

static Slot slot[ENTITY_MAX];
static uint8_t grid[GRID * GRID / 8];
static int debug_x;
static int debug_y;
static int debug_on;

static int shape_ok(const Shape *shape)
{
    if (shape == NULL)
        return 0;
    if (shape->w < 1 || shape->w > 4 || shape->h < 1 || shape->h > 4)
        return 0;
    if (shape->ox >= shape->w || shape->oy >= shape->h)
        return 0;
    return 1;
}

static int bit_on(const Shape *shape, int col, int row)
{
    int shift;

    if (col < 0 || row < 0 || col >= shape->w || row >= shape->h)
        return 0;
    shift = 15 - (row * 4 + col);
    return (shape->bits >> shift) & 1;
}

static void cell_of(const Entity *e, int col, int row, int *x, int *y)
{
    *x = e->x + col - e->shape->ox;
    *y = e->y + row - e->shape->oy;
}

void world_reset(void)
{
    int i;

    for (i = 0; i < ENTITY_MAX; i++)
        slot[i].live = 0;
}

int entity_spawn(const Shape *shape, int x, int y, int vx, int vy,
                 int every, unsigned flags)
{
    int i;

    if (!shape_ok(shape))
        return -1;
    for (i = 0; i < ENTITY_MAX; i++) {
        Entity *e;

        if (slot[i].live)
            continue;
        e = &slot[i].e;
        e->shape = shape;
        e->x = x;
        e->y = y;
        e->vx = vx;
        e->vy = vy;
        e->every = every;
        e->pace = 0;
        e->flags = flags;
        slot[i].live = 1;
        return i;
    }
    return -1;
}

void entity_kill(int id)
{
    if (id < 0 || id >= ENTITY_MAX)
        return;
    slot[id].live = 0;
}

Entity *entity_get(int id)
{
    if (id < 0 || id >= ENTITY_MAX || !slot[id].live)
        return NULL;
    return &slot[id].e;
}

int entity_step(int id)
{
    Entity *e = entity_get(id);

    if (e == NULL)
        return 0;
    if (e->flags & ENTITY_INACTIVE)
        return 0;
    e->pace++;
    if (e->pace < e->every)
        return 0;
    e->pace = 0;
    e->x += e->vx;
    e->y += e->vy;
    if (e->x < ENTITY_LO || e->x > ENTITY_HI || e->y < ENTITY_LO || e->y > ENTITY_HI)
        entity_kill(id);
    return 1;
}

int entity_covers(int id, int x, int y)
{
    Entity *e = entity_get(id);
    int col;
    int row;

    if (e == NULL)
        return 0;
    col = x - e->x + e->shape->ox;
    row = y - e->y + e->shape->oy;
    return bit_on(e->shape, col, row);
}

int entity_hidden(int id)
{
    Entity *e = entity_get(id);
    int col;
    int row;

    if (e == NULL)
        return 0;
    for (row = 0; row < e->shape->h; row++) {
        for (col = 0; col < e->shape->w; col++) {
            int x;
            int y;

            if (!bit_on(e->shape, col, row))
                continue;
            cell_of(e, col, row, &x, &y);
            if (x >= 0 && x < 16 && y >= 0 && y < 16)
                return 0;
        }
    }
    return 1;
}

static int box_miss(const Entity *a, const Entity *b)
{
    int ax0 = a->x - a->shape->ox;
    int ay0 = a->y - a->shape->oy;
    int ax1 = ax0 + a->shape->w - 1;
    int ay1 = ay0 + a->shape->h - 1;
    int bx0 = b->x - b->shape->ox;
    int by0 = b->y - b->shape->oy;
    int bx1 = bx0 + b->shape->w - 1;
    int by1 = by0 + b->shape->h - 1;

    if (ax1 < bx0 || bx1 < ax0 || ay1 < by0 || by1 < ay0)
        return 1;
    return 0;
}

int entity_overlaps(int a, int b)
{
    Entity *ea = entity_get(a);
    Entity *eb = entity_get(b);
    int col;
    int row;

    if (ea == NULL || eb == NULL)
        return 0;
    if (box_miss(ea, eb))
        return 0;
    for (row = 0; row < ea->shape->h; row++) {
        for (col = 0; col < ea->shape->w; col++) {
            int x;
            int y;

            if (!bit_on(ea->shape, col, row))
                continue;
            cell_of(ea, col, row, &x, &y);
            if (entity_covers(b, x, y))
                return 1;
        }
    }
    return 0;
}

void world_tick(void)
{
    int i;

    for (i = 0; i < ENTITY_MAX; i++)
        entity_step(i);
}

static void grid_clear(void)
{
    int i;

    for (i = 0; i < (int)sizeof grid; i++)
        grid[i] = 0;
}

static void grid_set(int x, int y)
{
    int col = x - ENTITY_LO;
    int row = y - ENTITY_LO;
    int i;

    if (col < 0 || row < 0 || col >= GRID || row >= GRID)
        return;
    i = row * GRID + col;
    grid[i >> 3] = (uint8_t)(grid[i >> 3] | (uint8_t)(1u << (i & 7)));
}

static int grid_on(int x, int y)
{
    int col = x - ENTITY_LO;
    int row = y - ENTITY_LO;
    int i;

    if (col < 0 || row < 0 || col >= GRID || row >= GRID)
        return 0;
    i = row * GRID + col;
    return (grid[i >> 3] >> (i & 7)) & 1;
}

static void stamp(const Entity *e)
{
    int col;
    int row;

    for (row = 0; row < e->shape->h; row++) {
        for (col = 0; col < e->shape->w; col++) {
            int x;
            int y;

            if (!bit_on(e->shape, col, row))
                continue;
            cell_of(e, col, row, &x, &y);
            grid_set(x, y);
        }
    }
}

void world_draw(void)
{
    int i;
    int x;
    int y;

    grid_clear();
    for (i = 0; i < ENTITY_MAX; i++) {
        if (slot[i].live)
            stamp(&slot[i].e);
    }
    clear();
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            if (grid_on(x, y))
                pixel((uint8_t)x, (uint8_t)y, true);
        }
    }
}

void world_debug_pixel(int x, int y, int on)
{
    if (on) {
        debug_x = x;
        debug_y = y;
        debug_on = 1;
    } else {
        debug_on = 0;
    }
}

int world_lit(int x, int y)
{
    if (debug_on && x == debug_x && y == debug_y)
        return 1;
    return grid_on(x, y);
}

void world_pairs(void (*fn)(int a, int b, void *user), void *user)
{
    int a;
    int b;

    if (fn == NULL)
        return;
    for (a = 0; a < ENTITY_MAX; a++) {
        if (!slot[a].live || (slot[a].e.flags & ENTITY_CAN_COLLIDE) == 0)
            continue;
        for (b = 0; b < ENTITY_MAX; b++) {
            if (a == b || !slot[b].live)
                continue;
            if ((slot[b].e.flags & ENTITY_CAN_BE_COLLIDED) == 0)
                continue;
            if (!entity_overlaps(a, b))
                continue;
            fn(a, b, user);
        }
    }
}
