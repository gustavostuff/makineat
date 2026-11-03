#include "rock.h"

#include "bullet.h"
#include "entity.h"
#include "player.h"
#include "rand.h"
#include "score.h"
#include "sound.h"

#include <stdint.h>

#define FIELD 16
#define ROCK_MAX 32
#define SPAWN_EVERY 36
/* Screen row 13 counting from 1 is y=12; beyond that is bottom >= 13 (0-based). */
#define ROCK_SCORE_LINE 13
#define ROCK_SCORE_MAX 9999

#if ROCK_MAX + 8 + 1 > ENTITY_MAX
#error the shooter does not fit in the entity table
#endif

static const int period[4] = {4, 8, 12, 16};

static const Shape solid2 = SHAPE2(0, 0, "11", "11");
static const Shape solid3 = SHAPE3(0, 0, "111", "111", "111");

typedef struct Rock {
    int id;
    int size;
    int fall;
    int dx;
    int xevery;
    int xpace;
    int scored_line;
} Rock;

static Rock rock[ROCK_MAX];
static int spawn_wait;
static int age;
static int rock_score;

int rock_fall_period(int base, int age_ticks)
{
    int drop;

    if (base <= 1)
        return 1;
    if (age_ticks < 0)
        age_ticks = 0;
    if (age_ticks > ROCK_RAMP)
        age_ticks = ROCK_RAMP;
    drop = (base - 1) * age_ticks / ROCK_RAMP;
    return base - drop;
}

static const Shape *shape_of(int size)
{
    return size == 2 ? &solid2 : &solid3;
}

static uint16_t next_rand(void)
{
    return (uint16_t)rand_next();
}

void rock_reset(void)
{
    int i;

    for (i = 0; i < ROCK_MAX; i++) {
        entity_kill(rock[i].id);
        rock[i].id = -1;
        rock[i].scored_line = 0;
    }
    spawn_wait = 12;
    age = 0;
    rock_score = 0;
}

int rock_score_get(void)
{
    return rock_score;
}

static void rock_score_add(int points)
{
    if (points <= 0 || rock_score >= ROCK_SCORE_MAX)
        return;
    rock_score += points;
    if (rock_score > ROCK_SCORE_MAX)
        rock_score = ROCK_SCORE_MAX;
}

static int rock_bottom(const Entity *e, int size)
{
    return e->y + size - 1;
}

static void rock_try_score_line(Rock *r, const Entity *e)
{
    if (r->scored_line)
        return;
    if (rock_bottom(e, r->size) < ROCK_SCORE_LINE)
        return;
    r->scored_line = 1;
    rock_score_add(1);
}

void rock_add(int x, int y, int size, int every, int dx, int xevery)
{
    int i;
    const Shape *shape;

    if (size != 2 && size != 3)
        return;
    if (every != 4 && every != 8 && every != 12 && every != 16)
        return;
    if (dx < -1 || dx > 1)
        return;
    if (xevery != 15 && xevery != 30)
        return;
    if (x < 0 || x > FIELD - size)
        return;
    shape = shape_of(size);
    for (i = 0; i < ROCK_MAX; i++) {
        int id;

        if (rock[i].id >= 0 && entity_get(rock[i].id) != NULL)
            continue;
        id = entity_spawn(shape, x, y, 0, 1, every, 0);
        if (id < 0)
            return;
        rock[i].id = id;
        rock[i].size = size;
        rock[i].fall = every;
        rock[i].dx = dx;
        rock[i].xevery = xevery;
        rock[i].xpace = 0;
        rock[i].scored_line = 0;
        return;
    }
}

static int hits_player(const Entity *e, int size)
{
    int dx;
    int dy;

    for (dy = 0; dy < size; dy++) {
        for (dx = 0; dx < size; dx++) {
            if (player_covers(e->x + dx, e->y + dy))
                return 1;
        }
    }
    return 0;
}

/* The rock fits in the rows from the top of the 32 by 32 grid through
 * the top row of the screen. */
static int spawn_row(uint16_t r, int size)
{
    int band = 0 - ENTITY_LO + 1;
    int room = band - size + 1;

    return ENTITY_LO + (int)((r >> 1) % (uint16_t)room);
}

static void spawn(void)
{
    uint16_t r = next_rand();
    int size = (r & 1u) ? 2 : 3;
    /* Low bits of this generator count. Bit 0 picks the size. The margin
     * row uses the bits just above that. The fall period stays in bits
     * 13 and 14. The sideways step uses the bits just under those. */
    int every = period[(r >> 13) & 3u];
    int xevery = (r & 0x8000u) ? 15 : 30;
    int dx = (int)(((r >> 10) & 7u) % 3u) - 1;
    int x = (int)((r >> 3) % (uint16_t)(FIELD - size));

    rock_add(x, spawn_row(r, size), size, every, dx, xevery);
}

int rock_tick(void)
{
    int i;
    int crashed = 0;

    age++;
    spawn_wait--;
    if (spawn_wait <= 0) {
        spawn();
        spawn_wait = SPAWN_EVERY;
    }

    for (i = 0; i < ROCK_MAX; i++) {
        Rock *r = &rock[i];
        Entity *e;

        if (r->id < 0)
            continue;
        e = entity_get(r->id);
        if (e == NULL) {
            r->id = -1;
            continue;
        }
        e->every = rock_fall_period(r->fall, age);
        entity_step(r->id);
        e = entity_get(r->id);
        if (e == NULL) {
            r->id = -1;
            continue;
        }
        if (r->dx != 0) {
            r->xpace++;
            if (r->xpace >= r->xevery) {
                int nx = e->x + r->dx;

                r->xpace = 0;
                if (nx < 0)
                    nx = 0;
                if (nx > FIELD - r->size)
                    nx = FIELD - r->size;
                e->x = nx;
            }
        }
        rock_try_score_line(r, e);
        if (e->y >= FIELD) {
            entity_kill(r->id);
            r->id = -1;
            continue;
        }
        if (bullet_take_rect(e->x, e->y, r->size, r->size)) {
            entity_kill(r->id);
            r->id = -1;
            score_add(1);
            sound(2);
            continue;
        }
        if (hits_player(e, r->size))
            crashed = 1;
    }
    return crashed;
}
