#include "player.h"

#include "entity.h"

#define FIELD 16
#define STEP_EVERY 2

static const Shape ship_shape = SHAPE2(1, 0,
    "010",
    "111");

static int ship_id = -1;

static int hold(Entity *e, int moving)
{
    if (!moving) {
        e->pace = 0;
        return 0;
    }
    if (e->pace > 0) {
        e->pace--;
        return 0;
    }
    e->pace = e->every - 1;
    return 1;
}

static void clamp_box(Entity *e)
{
    int left = e->x - e->shape->ox;
    int top = e->y - e->shape->oy;

    if (left < 0)
        left = 0;
    if (top < 0)
        top = 0;
    if (left > FIELD - e->shape->w)
        left = FIELD - e->shape->w;
    if (top > FIELD - e->shape->h)
        top = FIELD - e->shape->h;
    e->x = left + e->shape->ox;
    e->y = top + e->shape->oy;
}

void player_reset(void)
{
    if (ship_id >= 0)
        entity_kill(ship_id);
    ship_id = entity_spawn(&ship_shape, 7, 13, 0, 0, STEP_EVERY, ENTITY_INACTIVE);
}

void player_tick(const Input *in)
{
    Entity *ship = entity_get(ship_id);
    int dx = 0;
    int dy = 0;

    if (ship == NULL)
        return;
    if (in->left && !in->right)
        dx = -1;
    if (in->right && !in->left)
        dx = 1;
    if (in->up && !in->down)
        dy = -1;
    if (in->down && !in->up)
        dy = 1;
    if (!hold(ship, dx || dy))
        return;
    ship->x += dx;
    ship->y += dy;
    clamp_box(ship);
}

int player_nose_x(void)
{
    Entity *ship = entity_get(ship_id);

    return ship == NULL ? 0 : ship->x;
}

int player_nose_y(void)
{
    Entity *ship = entity_get(ship_id);

    return ship == NULL ? 0 : ship->y;
}

int player_covers(int x, int y)
{
    return entity_covers(ship_id, x, y);
}
