#include "entity.h"
#include "frame.h"
#include "tests/report.h"

static const Shape ship = SHAPE3(1, 0,
    "010",
    "111",
    "111");
static const Shape dot = SHAPE1(0, 0, "1");
static const Shape pair = SHAPE1(0, 0, "11");
static const Shape block = SHAPE2(0, 0, "11", "11");

static int pair_n;
static int pair_a;
static int pair_b;

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void on_pair(int a, int b, void *user)
{
    int *extra = user;

    pair_n++;
    pair_a = a;
    pair_b = b;
    if (extra != 0)
        (*extra)++;
}

static void test_shape(void)
{
    expect(ship.bits == 0x4EE0 && ship.w == 3 && ship.h == 3 &&
           ship.ox == 1 && ship.oy == 0,
           "the ship packs three rows and the nose is the origin");
    expect(dot.bits == 0x8000 && dot.w == 1 && dot.h == 1,
           "a one-pixel shape is the top bit");
    expect(block.bits == 0xCC00 && block.w == 2 && block.h == 2,
           "a solid 2x2 packs two nibbles");
    expect(entity_spawn(&(Shape)SHAPE1(0, 0, ""), 0, 0, 0, 0, 1, 0) < 0,
           "an empty shape is refused");
}

static void test_window(void)
{
    int id;

    world_reset();
    id = entity_spawn(&pair, 15, 0, 0, 0, 1, 0);
    world_draw();
    expect(pixel_get(15, 0) && !pixel_get(14, 0),
           "a pixel on the right edge is drawn");
    expect(entity_covers(id, 15, 0) && entity_covers(id, 16, 0),
           "the second pixel hangs past the screen");
    expect(world_lit(15, 0) && world_lit(16, 0) && !world_lit(14, 0),
           "vram keeps the pixel past the screen");
    expect(!entity_hidden(id), "one lit screen pixel keeps the entity visible");

    world_reset();
    id = entity_spawn(&dot, 16, 0, 0, 0, 1, 0);
    world_draw();
    expect(!pixel_get(15, 0) && entity_hidden(id) && entity_get(id) != 0,
           "an origin in the margin is hidden and still live");
    expect(world_lit(16, 0) && !world_lit(15, 0),
           "vram keeps a sprite that misses the screen");
}

static void test_hidden_dead(void)
{
    int id;

    world_reset();
    id = entity_spawn(&dot, -8, -8, 0, 0, 1, 0);
    expect(entity_hidden(id), "a sprite in the far margin is hidden");
    world_draw();
    expect(world_lit(-8, -8) && !world_lit(-9, -8) && !world_lit(24, 0),
           "vram holds the far corner and stops at the margin");
    entity_kill(id);
    expect(entity_get(id) == 0 && !entity_hidden(id),
           "a freed slot is dead");
}

static void test_motion(void)
{
    int id;
    Entity *e;

    world_reset();
    id = entity_spawn(&dot, 0, 0, 1, 0, 1, ENTITY_INACTIVE);
    world_tick();
    e = entity_get(id);
    expect(e != 0 && e->x == 0 && e->pace == 0,
           "an inactive entity keeps its place and its pace");
    e->flags = 0;
    world_tick();
    expect(entity_get(id)->x == 1, "every of 1 steps on the next tick");

    world_reset();
    id = entity_spawn(&dot, 4, 4, 0, 1, 2, 0);
    world_tick();
    expect(entity_get(id)->y == 4, "every of 2 waits on the first tick");
    world_tick();
    expect(entity_get(id)->y == 5, "every of 2 steps on the second tick");
}

static void test_death(void)
{
    int id;

    world_reset();
    id = entity_spawn(&dot, ENTITY_HI, 0, 1, 0, 1, 0);
    world_tick();
    expect(entity_get(id) == 0, "an origin past 23 frees the slot");

    id = entity_spawn(&dot, ENTITY_LO, 0, -1, 0, 1, 0);
    world_tick();
    expect(entity_get(id) == 0, "an origin past -8 frees the slot");

    id = entity_spawn(&dot, ENTITY_HI, 0, 0, 0, 1, 0);
    world_tick();
    expect(entity_get(id) != 0 && entity_get(id)->x == ENTITY_HI,
           "an origin on the edge stays live");
}

static void test_pairs(void)
{
    int a;
    int b;
    int extra = 0;

    world_reset();
    a = entity_spawn(&dot, 3, 3, 0, 0, 1, ENTITY_CAN_COLLIDE);
    b = entity_spawn(&dot, 3, 3, 0, 0, 1, ENTITY_CAN_BE_COLLIDED);
    pair_n = 0;
    world_pairs(on_pair, &extra);
    expect(pair_n == 1 && pair_a == a && pair_b == b && extra == 1,
           "one collide flag and one solid flag report one pair");

    entity_get(a)->x = 0;
    pair_n = 0;
    world_pairs(on_pair, 0);
    expect(pair_n == 0, "separated sprites do not pair");

    entity_get(a)->x = 3;
    entity_get(a)->flags = ENTITY_CAN_BE_COLLIDED;
    entity_get(b)->flags = ENTITY_CAN_COLLIDE;
    pair_n = 0;
    world_pairs(on_pair, 0);
    expect(pair_n == 1 && pair_a == b && pair_b == a,
           "the pair follows the collide flag");

    entity_get(a)->flags = 0;
    entity_get(b)->flags = 0;
    pair_n = 0;
    world_pairs(on_pair, 0);
    expect(pair_n == 0, "sprites with neither flag do not pair");
}

static void test_entity_at(void)
{
    int a;
    int b;

    world_reset();
    expect(entity_at(0, 0) < 0, "entity_at on an empty field");
    a = entity_spawn(&dot, 4, 5, 0, 0, 1, 0);
    b = entity_spawn(&dot, 7, 7, 0, 0, 1, 0);
    expect(a >= 0 && b >= 0, "spawns for entity_at");
    expect(entity_at(4, 5) == a && entity_at(7, 7) == b,
           "entity_at returns the covering slot");
    expect(entity_at(0, 0) < 0, "entity_at misses empty cells");
}

static void test_entity_set(void)
{
    int id;
    Entity *e;

    world_reset();
    id = entity_spawn(&dot, 0, 0, 0, 1, 4, ENTITY_INACTIVE);
    expect(id >= 0, "spawn for entity_set");
    entity_set_pos(id, 9, 10);
    e = entity_get(id);
    expect(e != NULL && e->x == 9 && e->y == 10, "entity_set_pos");
    entity_set_motion(id, 1, 0, 1);
    entity_clear_flags(id, ENTITY_INACTIVE);
    world_tick();
    e = entity_get(id);
    expect(e != NULL && e->x == 10 && e->y == 10, "entity_set_motion and clear_flags");
    entity_set_flags(id, ENTITY_INACTIVE);
    e = entity_get(id);
    expect(e != NULL && (e->flags & ENTITY_INACTIVE) != 0, "entity_set_flags");
}

static void test_cap(void)
{
    int i;
    int id;
    int again;

    world_reset();
    for (i = 0; i < ENTITY_MAX; i++) {
        id = entity_spawn(&dot, i - 8, 0, 0, 0, 1, 0);
        if (id != i)
            break;
    }
    expect(i == ENTITY_MAX, "sixty-four spawns fill the table");
    expect(entity_spawn(&dot, 0, 0, 0, 0, 1, 0) < 0,
           "a sixty-fifth spawn is refused");
    entity_kill(7);
    again = entity_spawn(&dot, 1, 1, 0, 0, 1, 0);
    expect(again == 7, "a freed slot is used again");
}

int main(void)
{
    report_begin("entity");
    test_shape();
    test_window();
    test_hidden_dead();
    test_motion();
    test_pairs();
    test_death();
    test_entity_at();
    test_entity_set();
    test_cap();
    return report_end();
}
