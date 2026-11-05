#ifndef FW_ENTITY_H
#define FW_ENTITY_H

#include <stddef.h>
#include <stdint.h>

/*
 * Up to 64 sprites. The screen is columns 0..15 and rows 0..15.
 * A one-screen margin around it makes the live area -8..23.
 * That 32 by 32 grid is where an entity may sit.
 * The picture copied into the frame is the center window.
 *
 * x and y are the origin, one cell of the shape box.
 * vx and vy are the step, each -1, 0, or 1.
 * every is how many 60 Hz ticks pass between steps.
 * When it elapses, the entity moves by vx and vy together.
 * An inactive entity keeps its speed and does not step.
 * An origin outside -8..23 frees the slot.
 *
 * ENTITY_CAN_COLLIDE and ENTITY_CAN_BE_COLLIDED mark a pair.
 * world_pairs reports each such pair once when lit pixels share a cell.
 * The report does not bounce, score, or delete.
 *
 * SHAPE1 through SHAPE4 pack string rows into one flash word.
 * Width and height come from the strings and are 1..4.
 * An instance does not copy pixels.
 */

#define ENTITY_MAX 64
#define ENTITY_LO (-8)
#define ENTITY_HI 23

#define ENTITY_INACTIVE 1u
#define ENTITY_CAN_COLLIDE 2u
#define ENTITY_CAN_BE_COLLIDED 4u

typedef struct Shape {
    uint16_t bits;
    uint8_t w;
    uint8_t h;
    uint8_t ox;
    uint8_t oy;
} Shape;

#define SHAPE_BIT(s, i) \
    (((int)(i) < (int)sizeof(s) - 1 && (s)[i] == '1') ? 1 : 0)
#define SHAPE_NIBBLE(s) \
    ((SHAPE_BIT(s, 0) << 3) | (SHAPE_BIT(s, 1) << 2) | \
     (SHAPE_BIT(s, 2) << 1) | SHAPE_BIT(s, 3))
#define SHAPE_BITS(a, b, c, d) \
    ((uint16_t)((SHAPE_NIBBLE(a) << 12) | (SHAPE_NIBBLE(b) << 8) | \
                (SHAPE_NIBBLE(c) << 4) | SHAPE_NIBBLE(d)))
#define SHAPE_W(a) ((uint8_t)(sizeof(a) - 1))

#define SHAPE1(ox, oy, a) \
    { SHAPE_BITS(a, "", "", ""), SHAPE_W(a), 1, (uint8_t)(ox), (uint8_t)(oy) }
#define SHAPE2(ox, oy, a, b) \
    { SHAPE_BITS(a, b, "", ""), SHAPE_W(a), 2, (uint8_t)(ox), (uint8_t)(oy) }
#define SHAPE3(ox, oy, a, b, c) \
    { SHAPE_BITS(a, b, c, ""), SHAPE_W(a), 3, (uint8_t)(ox), (uint8_t)(oy) }
#define SHAPE4(ox, oy, a, b, c, d) \
    { SHAPE_BITS(a, b, c, d), SHAPE_W(a), 4, (uint8_t)(ox), (uint8_t)(oy) }

typedef struct Entity {
    const Shape *shape;
    int x;
    int y;
    int vx;
    int vy;
    int every;
    int pace;
    unsigned flags;
} Entity;

void world_reset(void);

/* Returns a slot index, or -1 when every slot is live or the shape is unusable. */
int entity_spawn(const Shape *shape, int x, int y, int vx, int vy,
                 int every, unsigned flags);

void entity_kill(int id);

/* Null when the slot is free or the index is out of range. */
Entity *entity_get(int id);

/* Steps one active entity. Returns 1 when it moved. */
int entity_step(int id);

/* True when the entity is live and none of its lit pixels lie on the screen. */
int entity_hidden(int id);

int entity_covers(int id, int x, int y);
int entity_overlaps(int a, int b);

void world_tick(void);
void world_paint(void);
void world_draw(void);

/* First live entity covering screen cell (x,y) in 0..15, or -1. */
int entity_at(int x, int y);

void entity_set_pos(int id, int x, int y);
void entity_set_motion(int id, int vx, int vy, int every);
void entity_set_flags(int id, unsigned flags);
void entity_clear_flags(int id, unsigned flags);

/* 1 when that cell of the 32 by 32 grid is lit. x and y run from -8 to 23. */
int world_lit(int x, int y);

/* Simulator: force one world cell on in VRAM until cleared (x,y) with on=0. */
void world_debug_pixel(int x, int y, int on);

void world_pairs(void (*fn)(int a, int b, void *user), void *user);

#endif
