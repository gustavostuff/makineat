#include "stage.h"

static int map_bit(const uint16_t *rows, int row_words, int w, int h, int x, int y)
{
    int word;
    int bit;

    if (rows == 0 || w <= 0 || h <= 0)
        return 0;
    if (x < 0 || y < 0 || x >= w || y >= h)
        return 0;
    word = y * row_words + x / 16;
    bit = x % 16;
    return ((rows[word] >> bit) & 1u) != 0;
}

int stage_map_lit(const StageMap *map, int x, int y)
{
    if (map == 0 || map->fg_lit == 0)
        return 0;
    return map_bit(map->fg_lit, (int)map->row_words, (int)map->width, (int)map->height, x, y);
}

int stage_map_solid(const StageMap *map, int x, int y)
{
    if (map == 0 || map->fg_solid == 0)
        return 0;
    return map_bit(map->fg_solid, (int)map->row_words, (int)map->width, (int)map->height, x, y);
}

int stage_map_get(const StageMap *map, int x, int y)
{
    return stage_map_lit(map, x, y);
}

static int item_limit(const StageMap *map)
{
    int n;

    if (map == 0 || map->items == 0)
        return 0;
    n = (int)map->item_count;
    if (n > STAGE_ITEM_MAX)
        n = STAGE_ITEM_MAX;
    return n;
}

int stage_item_covers(const StageMap *map, int x, int y)
{
    int i;
    int n;
    const StageItem *it;

    n = item_limit(map);
    for (i = 0; i < n; i++) {
        it = &map->items[i];
        if (x >= (int)it->x && x < (int)it->x + 2 && y >= (int)it->y && y < (int)it->y + 2)
            return 1;
    }
    return 0;
}

static void clear_item_state(StageView *view)
{
    int i;

    view->item_shine = 0;
    view->item_shine_accum = 0;
    for (i = 0; i < STAGE_ITEM_MAX; i++) {
        view->item_bounce[i] = 0;
        view->item_used[i] = 0;
    }
}

/*
 * Shine patterns, top row then bottom row, 1 is ON.
 *   1 0     0 1     0 0
 *   0 0     1 0     0 1
 */
static const uint8_t item_shine_pat[STAGE_ITEM_SHINE_FRAMES][2][2] = {
    {{1, 0}, {0, 0}},
    {{0, 1}, {1, 0}},
    {{0, 0}, {0, 1}},
};

static void paint_item_blocks(const StageView *view);

static int map_bg_lit(const StageMap *map, int x, int y)
{
    if (map == 0 || map->bg_lit == 0 || map->bg_width == 0)
        return 0;
    return map_bit(map->bg_lit, (int)map->bg_row_words, (int)map->bg_width, (int)map->height, x, y);
}

static void sync_bg_camera(StageView *view)
{
    int bg;
    int max_bg;

    if (view == 0 || view->map == 0)
        return;
    if (view->map->bg_lit == 0 || view->map->bg_width == 0 || view->map->width == 0) {
        view->bg_camera = 0;
        return;
    }
    /*
     * Map the scrollable spans, not the raw widths. Both windows are FIELD_SIZE
     * wide, so fg can travel (width - FIELD_SIZE) and bg can travel
     * (bg_width - FIELD_SIZE). Scaling by full widths hits the bg clamp before
     * the fg camera reaches the end of a wider stage.
     */
    {
        int fg_span = (int)view->map->width - FIELD_SIZE;
        int bg_span = (int)view->map->bg_width - FIELD_SIZE;

        if (fg_span < 1 || bg_span < 1)
            bg = 0;
        else
            bg = view->camera * bg_span / fg_span;
    }
    max_bg = (int)view->map->bg_width - FIELD_SIZE;
    if (max_bg < 0)
        max_bg = 0;
    if (bg < 0)
        bg = 0;
    if (bg > max_bg)
        bg = max_bg;
    view->bg_camera = bg;
}

void stage_view_init(StageView *view)
{
    if (view == 0)
        return;
    view->mode = STAGE_SCROLL_H;
    view->map = 0;
    view->camera = 0;
    view->bg_camera = 0;
    view->scroll_accum = 0;
    view->dead_zone_x = STAGE_H_DEAD_ZONE_X;
    view->dead_zone_w = STAGE_H_DEAD_ZONE_W;
    clear_item_state(view);
}

int stage_bind_h(StageView *view, const StageMap *map)
{
    if (view == 0 || map == 0)
        return STAGE_ERR_UNSUPPORTED;
    view->mode = STAGE_SCROLL_H;
    view->map = map;
    view->camera = 0;
    view->scroll_accum = 0;
    view->dead_zone_x = STAGE_H_DEAD_ZONE_X;
    view->dead_zone_w = STAGE_H_DEAD_ZONE_W;
    clear_item_state(view);
    sync_bg_camera(view);
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

int stage_h_bg_camera_max(const StageView *view)
{
    int max;

    if (view == 0 || view->map == 0 || view->map->bg_lit == 0 || view->map->bg_width == 0)
        return 0;
    max = (int)view->map->bg_width - FIELD_SIZE;
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

int stage_h_bg_camera_get(const StageView *view)
{
    if (view == 0)
        return 0;
    return view->bg_camera;
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
    sync_bg_camera(view);
}

void stage_h_camera_follow_dead_zone(StageView *view, int world_x)
{
    int sx;
    int left;
    int right;

    if (view == 0 || view->mode != STAGE_SCROLL_H || view->map == 0)
        return;
    sx = world_x - view->camera;
    left = view->dead_zone_x;
    right = view->dead_zone_x + view->dead_zone_w - 1;
    if (sx < left)
        stage_h_camera_set(view, world_x - left);
    else if (sx > right)
        stage_h_camera_set(view, world_x - right);
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

void stage_h_paint_ex(const StageView *view, int hide_world_x, int hide_world_y, int hide_player_fg)
{
    int sx;
    int sy;
    int wx;
    int bx;
    int player_cell;
    int fg_on;
    int bg_on;
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
            bx = view->bg_camera + sx;
            player_cell = hide_player_fg != 0 && wx == hide_world_x && sy == hide_world_y;
            /*
             * One pass per cell. Foreground wins over background for the ON bit so BG+FG
             * never stack to a double-bright LED when both layers have art. Background only
             * contributes when foreground does not draw ON at this cell.
             *
             * Player hide: on the player cell, skip ON for green fg decor and for bg when fg
             * is not solid red (empty or green), so the sim uses dim led_off acetate green.
             */
            fg_on = stage_map_lit(map, wx, sy);
            if (player_cell && fg_on && !stage_map_solid(map, wx, sy))
                fg_on = 0;
            if (fg_on) {
                pixel((uint8_t)sx, (uint8_t)sy, true);
                continue;
            }
            bg_on = map_bg_lit(map, bx, sy);
            if (bg_on && !(player_cell && !stage_map_solid(map, wx, sy)))
                pixel((uint8_t)sx, (uint8_t)sy, true);
        }
    }
    paint_item_blocks(view);
}

static void paint_item_blocks(const StageView *view)
{
    int i;
    int n;
    int dx;
    int dy;
    int lift;
    int sx;
    int sy;
    const StageItem *it;

    n = item_limit(view->map);
    for (i = 0; i < n; i++) {
        it = &view->map->items[i];
        lift = view->item_bounce[i] > 0 ? 1 : 0;
        for (dy = 0; dy < 2; dy++) {
            for (dx = 0; dx < 2; dx++) {
                /* Shine is draw-only. A used block stays fully lit. */
                if (!view->item_used[i] && !item_shine_pat[view->item_shine][dy][dx])
                    continue;
                sx = (int)it->x + dx - view->camera;
                sy = (int)it->y + dy - lift;
                if (sx < 0 || sx > FIELD_LAST || sy < 0 || sy > FIELD_LAST)
                    continue;
                pixel((uint8_t)sx, (uint8_t)sy, true);
            }
        }
    }
}

void stage_h_paint(const StageView *view)
{
    stage_h_paint_ex(view, 0, 0, 0);
}

int stage_h_suppress_player_led(const StageView *view, int world_x, int world_y)
{
    int sx;
    int bx;

    if (view == 0 || view->map == 0)
        return 0;
    if (stage_map_solid(view->map, world_x, world_y))
        return 0;
    if (stage_map_lit(view->map, world_x, world_y))
        return 1;
    sx = world_x - view->camera;
    if (sx < 0 || sx > FIELD_LAST || world_y < 0 || world_y > FIELD_LAST)
        return 0;
    bx = view->bg_camera + sx;
    return map_bg_lit(view->map, bx, world_y);
}

int stage_h_solid(const StageView *view, int world_x, int world_y)
{
    if (view == 0 || view->map == 0)
        return 0;
    if (stage_map_solid(view->map, world_x, world_y))
        return 1;
    return stage_item_covers(view->map, world_x, world_y);
}

void stage_h_items_tick(StageView *view)
{
    int i;
    int n;

    if (view == 0 || view->map == 0)
        return;
    /* 8 does not divide 60, so the shine steps with the same accumulator as walk speed. */
    view->item_shine_accum += STAGE_ITEM_SHINE_FPS;
    if (view->item_shine_accum >= STAGE_TICK_HZ) {
        view->item_shine_accum -= STAGE_TICK_HZ;
        view->item_shine++;
        if (view->item_shine >= STAGE_ITEM_SHINE_FRAMES)
            view->item_shine = 0;
    }
    n = item_limit(view->map);
    for (i = 0; i < n; i++) {
        if (view->item_bounce[i] > 0)
            view->item_bounce[i]--;
    }
}

void stage_h_item_bump(StageView *view, int x, int y)
{
    int i;
    int n;
    const StageItem *it;

    if (view == 0 || view->map == 0)
        return;
    n = item_limit(view->map);
    for (i = 0; i < n; i++) {
        it = &view->map->items[i];
        if (x < (int)it->x || x >= (int)it->x + 2 || y < (int)it->y || y >= (int)it->y + 2)
            continue;
        /* The first hit uses the container. Later hits leave it as a solid cell. */
        if (view->item_used[i])
            return;
        view->item_used[i] = 1;
        view->item_bounce[i] = STAGE_ITEM_BOUNCE_TICKS;
        return;
    }
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
