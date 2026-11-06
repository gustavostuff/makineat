#include "stage.h"

int stage_map_get(const StageMap *map, int x, int y)
{
    int word;
    int bit;

    if (map == 0 || map->rows == 0)
        return 0;
    if (x < 0 || y < 0 || x >= (int)map->width || y >= (int)map->height)
        return 0;
    word = y * (int)map->row_words + x / 16;
    bit = x % 16;
    return ((map->rows[word] >> bit) & 1u) != 0;
}

void stage_view_init(StageView *view)
{
    if (view == 0)
        return;
    view->mode = STAGE_SCROLL_H;
    view->map = 0;
    view->camera = 0;
    view->scroll_accum = 0;
}

int stage_bind_h(StageView *view, const StageMap *map)
{
    if (view == 0 || map == 0)
        return STAGE_ERR_UNSUPPORTED;
    view->mode = STAGE_SCROLL_H;
    view->map = map;
    view->camera = 0;
    view->scroll_accum = 0;
    return 0;
}

int stage_bind_v(StageView *view, const StageMap *map)
{
    (void)view;
    (void)map;
    return STAGE_ERR_UNSUPPORTED;
}

int stage_h_camera_max(const StageView *view)
{
    int max;

    if (view == 0 || view->map == 0)
        return 0;
    max = (int)view->map->width - FIELD_SIZE;
    if (max < 0)
        max = 0;
    return max;
}

int stage_h_camera_get(const StageView *view)
{
    if (view == 0)
        return 0;
    return view->camera;
}

void stage_h_camera_set(StageView *view, int x)
{
    int max;

    if (view == 0)
        return;
    max = stage_h_camera_max(view);
    if (x < 0)
        x = 0;
    if (x > max)
        x = max;
    view->camera = x;
}

static void h_step_camera(StageView *view, int dx)
{
    stage_h_camera_set(view, view->camera + dx);
}

void stage_h_scroll_tick(StageView *view, int dx)
{
    if (view == 0 || view->mode != STAGE_SCROLL_H || dx == 0)
        return;
    if (dx < -1)
        dx = -1;
    if (dx > 1)
        dx = 1;
    view->scroll_accum += STAGE_SCROLL_FPS;
    while (view->scroll_accum >= STAGE_TICK_HZ) {
        view->scroll_accum -= STAGE_TICK_HZ;
        h_step_camera(view, dx);
    }
}

void stage_h_paint(const StageView *view)
{
    int sx;
    int sy;
    int wx;
    const StageMap *map;

    if (view == 0 || view->map == 0)
        return;
    map = view->map;
    clear();
    for (sy = 0; sy < FIELD_SIZE; sy++) {
        if (sy >= (int)map->height)
            continue;
        for (sx = 0; sx < FIELD_SIZE; sx++) {
            wx = view->camera + sx;
            if (stage_map_get(map, wx, sy))
                pixel((uint8_t)sx, (uint8_t)sy, true);
        }
    }
}

int stage_h_solid(const StageView *view, int world_x, int world_y)
{
    if (view == 0 || view->map == 0)
        return 0;
    return stage_map_get(view->map, world_x, world_y);
}

int stage_v_camera_max(const StageView *view)
{
    (void)view;
    return STAGE_ERR_UNSUPPORTED;
}

void stage_v_scroll_tick(StageView *view, int dy)
{
    (void)view;
    (void)dy;
}

void stage_v_paint(const StageView *view)
{
    (void)view;
}
