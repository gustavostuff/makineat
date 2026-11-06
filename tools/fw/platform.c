#include "platform.h"

#define PLAT_MOVE_FPS 15
#define PLAT_GRAVITY 1
#define PLAT_MAX_FALL 8
#define PLAT_JUMP_HEIGHT 5
#define PLAT_JUMP_UP_TICKS (PLAT_JUMP_HEIGHT * (STAGE_TICK_HZ / PLAT_MOVE_FPS))
#define PLAT_JUMP_VY (-(PLAT_JUMP_UP_TICKS + 1))
#define PLAT_CAMERA_LEAD (FIELD_SIZE / 2)

static int body_blocked(const StageView *view, int x, int y)
{
    if (view == 0 || view->map == 0)
        return 1;
    if (x < 0 || y < 0 || x >= (int)view->map->width || y >= (int)view->map->height)
        return 1;
    return stage_map_get(view->map, x, y);
}

static int on_ground(const StageView *view, int px, int py)
{
    if (py + 1 >= (int)view->map->height)
        return 1;
    return body_blocked(view, px, py + 1);
}

static void camera_follow(StageView *view, int px)
{
    stage_h_camera_set(view, px - PLAT_CAMERA_LEAD);
}

static void try_walk(PlatformPlayer *player, StageView *view, int dx)
{
    int nx;

    if (player == 0 || view == 0 || dx == 0)
        return;
    player->walk_accum += PLAT_MOVE_FPS;
    while (player->walk_accum >= STAGE_TICK_HZ) {
        player->walk_accum -= STAGE_TICK_HZ;
        nx = player->px + dx;
        if (!body_blocked(view, nx, player->py))
            player->px = nx;
    }
}

static void apply_gravity(PlatformPlayer *player)
{
    player->vy += PLAT_GRAVITY;
    if (player->vy > PLAT_MAX_FALL)
        player->vy = PLAT_MAX_FALL;
}

static void move_vertical(PlatformPlayer *player, StageView *view)
{
    int ny;
    int step;

    if (player->vy > 0)
        step = 1;
    else if (player->vy < 0)
        step = -1;
    else {
        player->vert_accum = 0;
        return;
    }
    player->vert_accum += PLAT_MOVE_FPS;
    while (player->vert_accum >= STAGE_TICK_HZ) {
        player->vert_accum -= STAGE_TICK_HZ;
        ny = player->py + step;
        if (body_blocked(view, player->px, ny)) {
            if (step > 0) {
                player->grounded = 1;
                player->vy = 0;
            } else
                player->vy = 0;
            player->vert_accum = 0;
            return;
        }
        player->py = ny;
        player->grounded = on_ground(view, player->px, player->py);
    }
}

void platform_player_spawn(PlatformPlayer *player, const StageMap *map)
{
    if (player == 0 || map == 0)
        return;
    player->px = (int)map->spawn_x;
    player->py = (int)map->spawn_y;
    player->vy = 0;
    player->walk_accum = 0;
    player->vert_accum = 0;
    player->grounded = 0;
}

void platform_player_bind(PlatformPlayer *player, StageView *view, const StageMap *map)
{
    platform_player_spawn(player, map);
    if (view != 0)
        camera_follow(view, player->px);
}

void platform_player_tick(PlatformPlayer *player, StageView *view, const Input *in)
{
    int dx = 0;

    if (player == 0 || view == 0 || in == 0)
        return;
    if (in->left && !in->right)
        dx = -1;
    else if (in->right && !in->left)
        dx = 1;
    else
        player->walk_accum = 0;
    try_walk(player, view, dx);
    if (in->button_pressed && player->grounded) {
        player->vy = PLAT_JUMP_VY;
        player->grounded = 0;
    }
    apply_gravity(player);
    move_vertical(player, view);
    if (!player->grounded)
        player->grounded = on_ground(view, player->px, player->py);
    camera_follow(view, player->px);
}

void platform_h_paint(const StageView *view, const PlatformPlayer *player)
{
    int sx;
    int sy;

    stage_h_paint(view);
    if (player == 0 || view == 0)
        return;
    sx = player->px - stage_h_camera_get(view);
    sy = player->py;
    if (sx >= 0 && sx <= FIELD_LAST && sy >= 0 && sy <= FIELD_LAST)
        pixel((uint8_t)sx, (uint8_t)sy, true);
}

#if !defined(__AVR__)
int platform_player_x(const PlatformPlayer *player)
{
    if (player == 0)
        return 0;
    return player->px;
}

int platform_player_y(const PlatformPlayer *player)
{
    if (player == 0)
        return 0;
    return player->py;
}

int platform_player_vy(const PlatformPlayer *player)
{
    if (player == 0)
        return 0;
    return player->vy;
}
#endif
