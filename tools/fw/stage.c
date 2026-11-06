#include "stage.h"

#include "plat_physics.h"

#include <stddef.h>

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

int stage_map_semisolid(const StageMap *map, int x, int y)
{
    if (map == 0 || map->fg_semisolid == 0)
        return 0;
    return map_bit(map->fg_semisolid, (int)map->row_words, (int)map->width, (int)map->height, x, y);
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

static int item_cell_in_block(const StageItem *it, int x, int y)
{
    return x >= (int)it->x && x < (int)it->x + 2 && y >= (int)it->y && y < (int)it->y + 2;
}

static const StageItem *item_at_cell(const StageMap *map, int x, int y, int kind_filter)
{
    int i;
    int n;

    n = item_limit(map);
    for (i = 0; i < n; i++) {
        const StageItem *it = &map->items[i];

        if (!item_cell_in_block(it, x, y))
            continue;
        if (kind_filter < 0 || it->kind == (uint8_t)kind_filter)
            return it;
    }
    return 0;
}

int stage_item_covers(const StageMap *map, int x, int y)
{
    return item_at_cell(map, x, y, STAGE_ITEM_KIND_COIN) != 0;
}

int stage_growth_covers(const StageMap *map, int x, int y)
{
    return item_at_cell(map, x, y, STAGE_ITEM_KIND_GROWTH) != 0;
}

static int item_blocks_cell(const StageMap *map, int x, int y)
{
    return item_at_cell(map, x, y, -1) != 0;
}

static int pickup_at(const StageView *view, int x, int y, int ignore_slot)
{
    int s;

    if (view == 0)
        return 0;
    for (s = 0; s < STAGE_PICKUP_MAX; s++) {
        if (s == ignore_slot)
            continue;
        if (!view->pickups[s].active)
            continue;
        if ((int)view->pickups[s].px == x && (int)view->pickups[s].py == y)
            return 1;
    }
    return 0;
}

static void clear_pickups(StageView *view)
{
    int s;

    if (view == 0)
        return;
    for (s = 0; s < STAGE_PICKUP_MAX; s++)
        view->pickups[s].active = 0;
}

static void clear_item_state(StageView *view)
{
    int i;

    view->item_shine = 0;
    view->item_shine_age = 0;
    clear_pickups(view);
    for (i = 0; i < STAGE_ITEM_MAX; i++) {
        view->item_bounce[i] = 0;
        view->item_used[i] = 0;
        view->growth_pickup_spawned[i] = 0;
    }
}

/*
 * Shine patterns, top row then bottom row, 1 is ON.
 *   1 1     0 1     1 0     1 1
 *   1 1     1 1     0 1     1 0
 * Holds are 1 second, then 0.1 second for each of the other three.
 */
static const uint8_t item_shine_pat[STAGE_ITEM_SHINE_FRAMES][2][2] = {
    {{1, 1}, {1, 1}},
    {{0, 1}, {1, 1}},
    {{1, 0}, {0, 1}},
    {{1, 1}, {1, 0}},
};

static const uint8_t item_shine_hold[STAGE_ITEM_SHINE_FRAMES] = {
    STAGE_ITEM_SHINE_HOLD_LONG,
    STAGE_ITEM_SHINE_HOLD_SHORT,
    STAGE_ITEM_SHINE_HOLD_SHORT,
    STAGE_ITEM_SHINE_HOLD_SHORT,
};

struct pickup_block_ctx {
    StageView *view;
    int slot;
};

static int pickup_blocked_cb(void *p, int x, int y, int prev_feet_y, int dy)
{
    struct pickup_block_ctx *c = (struct pickup_block_ctx *)p;

    (void)prev_feet_y;
    (void)dy;
    if (c == 0 || c->view == 0)
        return 1;
    return stage_h_pickup_blocked(c->view, x, y, c->slot);
}

static void tick_pickups(StageView *view);

static void try_spawn_growth_pickup(StageView *view, int idx, const StageItem *it)
{
    int s;
    int px;
    int py;

    if (view == 0 || it == 0)
        return;
    if (it->kind != STAGE_ITEM_KIND_GROWTH)
        return;
    if (view->growth_pickup_spawned[idx])
        return;
    px = (int)it->x;
    py = (int)it->y - 1;
    if (py < 0)
        return;
    if (stage_h_pickup_blocked(view, px, py, -1))
        return;
    for (s = 0; s < STAGE_PICKUP_MAX; s++) {
        if (view->pickups[s].active)
            continue;
        view->pickups[s].active = 1;
        view->pickups[s].px = (int16_t)px;
        view->pickups[s].py = (int16_t)py;
        view->pickups[s].vy = 0;
        view->pickups[s].walk_accum = 0;
        view->pickups[s].vert_accum = 0;
        view->pickups[s].grounded = 0;
        view->pickups[s].dir = 1;
        view->pickups[s].fall_on = 0;
        view->pickups[s].fall_i = 0;
        view->pickups[s].fall_timer = 0;
        view->growth_pickup_spawned[idx] = 1;
        return;
    }
}

static void paint_pickups(const StageView *view)
{
    int s;
    int sx;
    int sy;

    if (view == 0)
        return;
    for (s = 0; s < STAGE_PICKUP_MAX; s++) {
        if (!view->pickups[s].active)
            continue;
        sx = (int)view->pickups[s].px - view->camera;
        sy = (int)view->pickups[s].py;
        if (sx < 0 || sx > FIELD_LAST || sy < 0 || sy > FIELD_LAST)
            continue;
        pixel((uint8_t)sx, (uint8_t)sy, true);
    }
}

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

void stage_h_paint_ex(const StageView *view, int hide_world_x, int hide_world_y, int hide_player_fg,
    int hide_player_h)
{
    int sx;
    int sy;
    int wx;
    int bx;
    int player_cell;
    int fg_on;
    int bg_on;
    const StageMap *map;
    int ph;

    if (view == 0 || view->map == 0)
        return;
    map = view->map;
    ph = hide_player_h;
    if (ph < 1)
        ph = 1;
    if (ph > 2)
        ph = 2;
    clear();
    for (sy = 0; sy < FIELD_SIZE; sy++) {
        if (sy >= (int)map->height)
            continue;
        for (sx = 0; sx < FIELD_SIZE; sx++) {
            wx = view->camera + sx;
            bx = view->bg_camera + sx;
            player_cell = hide_player_fg != 0 && wx == hide_world_x && sy == hide_world_y;
            if (!player_cell && hide_player_fg != 0 && ph == 2 &&
                wx == hide_world_x && sy == hide_world_y - 1)
                player_cell = 1;
            /*
             * One pass per cell. Empty foreground lets the background show. A foreground LED
             * that is on covers the background. Item blocks are not in fg_lit; paint_item_blocks
             * forces their dark shine cells off so a lit background cannot fill those holes.
             *
             * Player hide: on the player cell, skip ON for green fg decor and for bg when fg
             * is not solid red (empty or green), so the sim uses dim led_off acetate green.
             */
            fg_on = stage_map_lit(map, wx, sy);
            if (player_cell && fg_on && !stage_map_solid(map, wx, sy) && !stage_map_semisolid(map, wx, sy))
                fg_on = 0;
            if (fg_on) {
                pixel((uint8_t)sx, (uint8_t)sy, true);
                continue;
            }
            bg_on = map_bg_lit(map, bx, sy);
            if (bg_on && !(player_cell && !stage_map_solid(map, wx, sy) && !stage_map_semisolid(map, wx, sy)))
                pixel((uint8_t)sx, (uint8_t)sy, true);
        }
    }
    paint_item_blocks(view);
    paint_pickups(view);
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
    int lit;
    const StageItem *it;

    n = item_limit(view->map);
    for (i = 0; i < n; i++) {
        it = &view->map->items[i];
        lift = view->item_bounce[i] > 0 ? 1 : 0;
        for (dy = 0; dy < 2; dy++) {
            for (dx = 0; dx < 2; dx++) {
                sx = (int)it->x + dx - view->camera;
                sy = (int)it->y + dy - lift;
                if (sx < 0 || sx > FIELD_LAST || sy < 0 || sy > FIELD_LAST)
                    continue;
                /*
                 * Every cell of the block is foreground. A dark shine cell stays off
                 * and hides a background LED behind it. Empty cells outside the block
                 * are left alone so the background still shows there.
                 */
                lit = view->item_used[i] || item_shine_pat[view->item_shine][dy][dx];
                pixel((uint8_t)sx, (uint8_t)sy, lit != 0);
            }
        }
    }
}

void stage_h_paint(const StageView *view)
{
    stage_h_paint_ex(view, 0, 0, 0, 1);
}

int stage_h_suppress_player_led(const StageView *view, int world_x, int world_y)
{
    int sx;
    int bx;

    if (view == 0 || view->map == 0)
        return 0;
    if (stage_map_solid(view->map, world_x, world_y))
        return 0;
    if (stage_map_semisolid(view->map, world_x, world_y))
        return 0;
    if (stage_map_lit(view->map, world_x, world_y))
        return 1;
    sx = world_x - view->camera;
    if (sx < 0 || sx > FIELD_LAST || world_y < 0 || world_y > FIELD_LAST)
        return 0;
    bx = view->bg_camera + sx;
    return map_bg_lit(view->map, bx, world_y);
}

int stage_h_pickup_blocked(const StageView *view, int world_x, int world_y,
    int ignore_pickup_index)
{
    if (view == 0 || view->map == 0)
        return 1;
    if (world_x < 0 || world_y < 0 || world_x >= (int)view->map->width ||
        world_y >= (int)view->map->height)
        return 1;
    if (stage_map_solid(view->map, world_x, world_y))
        return 1;
    if (stage_map_semisolid(view->map, world_x, world_y))
        return 1;
    if (item_blocks_cell(view->map, world_x, world_y))
        return 1;
    if (pickup_at(view, world_x, world_y, ignore_pickup_index))
        return 1;
    return 0;
}

int stage_h_solid(const StageView *view, int world_x, int world_y)
{
    if (view == 0 || view->map == 0)
        return 0;
    if (stage_map_solid(view->map, world_x, world_y))
        return 1;
    if (stage_map_semisolid(view->map, world_x, world_y))
        return 1;
    return item_blocks_cell(view->map, world_x, world_y);
}

int stage_h_blocks_horiz(const StageView *view, int world_x, int world_y)
{
    if (view == 0 || view->map == 0)
        return 1;
    if (world_x < 0 || world_y < 0 || world_x >= (int)view->map->width ||
        world_y >= (int)view->map->height)
        return 1;
    if (stage_map_solid(view->map, world_x, world_y))
        return 1;
    return item_blocks_cell(view->map, world_x, world_y);
}

int stage_h_blocks_vert(const StageView *view, int world_x, int world_y, int prev_feet_y, int dy)
{
    const StageMap *map;
    int semi;

    (void)prev_feet_y;
    if (view == 0 || view->map == 0)
        return 1;
    map = view->map;
    if (world_x < 0 || world_y < 0 || world_x >= (int)map->width || world_y >= (int)map->height)
        return 1;
    if (item_blocks_cell(map, world_x, world_y))
        return 1;
    if (stage_map_solid(map, world_x, world_y))
        return 1;
    semi = stage_map_semisolid(map, world_x, world_y);
    if (!semi)
        return 0;
    /*
     * One-way ledge: the body stands in the cell above the gray row, same as red ground.
     * Falling into the cell is blocked. Jumping up into it from below is not.
     */
    if (dy > 0)
        return 1;
    return 0;
}

int stage_h_blocks_vert_hard(const StageView *view, int world_x, int world_y)
{
    if (view == 0 || view->map == 0)
        return 1;
    if (world_x < 0 || world_y < 0 || world_x >= (int)view->map->width ||
        world_y >= (int)view->map->height)
        return 1;
    if (item_blocks_cell(view->map, world_x, world_y))
        return 1;
    if (stage_map_solid(view->map, world_x, world_y))
        return 1;
    return 0;
}

void stage_h_items_tick(StageView *view)
{
    int i;
    int n;

    if (view == 0 || view->map == 0)
        return;
    /*
     * Age counts ticks already shown for this pattern, including this tick.
     * The next tick after the hold shows the next pattern, so a 60-tick
     * pattern stays up for 60 paints in the tick-then-paint loop.
     */
    view->item_shine_age++;
    if (view->item_shine_age > item_shine_hold[view->item_shine]) {
        view->item_shine++;
        if (view->item_shine >= STAGE_ITEM_SHINE_FRAMES)
            view->item_shine = 0;
        view->item_shine_age = 1;
    }
    n = item_limit(view->map);
    for (i = 0; i < n; i++) {
        if (view->item_bounce[i] > 0) {
            view->item_bounce[i]--;
            if (view->item_bounce[i] == 0)
                try_spawn_growth_pickup(view, i, &view->map->items[i]);
        }
    }
    tick_pickups(view);
}

static void tick_pickups(StageView *view)
{
    int s;
    PlatBody b;
    struct pickup_block_ctx ctx;
    int px;
    int py;

    if (view == 0)
        return;
    ctx.view = view;
    for (s = 0; s < STAGE_PICKUP_MAX; s++) {
        StagePickup *p = &view->pickups[s];

        if (!p->active)
            continue;
        ctx.slot = s;
        px = (int)p->px;
        py = (int)p->py;
        b.px = px;
        b.py = py;
        b.vy = p->vy;
        b.walk_accum = 0;
        b.vert_accum = p->vert_accum;
        b.grounded = p->grounded;
        b.fall_on = p->fall_on;
        b.fall_i = p->fall_i;
        b.fall_timer = p->fall_timer;
        plat_phys_move_vertical(&b, view, NULL, NULL, NULL, pickup_blocked_cb, &ctx);
        px = b.px;
        py = b.py;
        p->vy = b.vy;
        p->vert_accum = b.vert_accum;
        p->grounded = b.grounded;
        p->fall_on = b.fall_on;
        p->fall_i = b.fall_i;
        p->fall_timer = b.fall_timer;
        plat_phys_horiz_bounce(&px, &p->walk_accum, &p->dir, py, view,
            STAGE_PICKUP_HORIZ_FPS, pickup_blocked_cb, &ctx);
        p->px = (int16_t)px;
        p->py = (int16_t)py;
    }
}

StageBumpResult stage_h_item_bump(StageView *view, int x, int y)
{
    int i;
    int n;
    const StageItem *it;
    StageBumpResult kind;

    if (view == 0 || view->map == 0)
        return STAGE_BUMP_NONE;
    n = item_limit(view->map);
    for (i = 0; i < n; i++) {
        it = &view->map->items[i];
        if (x < (int)it->x || x >= (int)it->x + 2 || y < (int)it->y || y >= (int)it->y + 2)
            continue;
        /* The first hit uses the container. Later hits leave it as a solid cell. */
        if (view->item_used[i])
            return STAGE_BUMP_NONE;
        view->item_used[i] = 1;
        view->item_bounce[i] = STAGE_ITEM_BOUNCE_TICKS;
        kind = (it->kind == STAGE_ITEM_KIND_GROWTH) ? STAGE_BUMP_GROWTH : STAGE_BUMP_COIN;
        return kind;
    }
    return STAGE_BUMP_NONE;
}

int stage_h_collect_growth_pickup(StageView *view, int px, int py, int player_h)
{
    int s;
    int hit_py;

    if (view == 0)
        return 0;
    if (player_h < 1)
        player_h = 1;
    if (player_h > 2)
        player_h = 2;
    for (s = 0; s < STAGE_PICKUP_MAX; s++) {
        StagePickup *p = &view->pickups[s];

        if (!p->active)
            continue;
        for (hit_py = py; hit_py > py - player_h; hit_py--) {
            if (hit_py == (int)p->py && px == (int)p->px) {
                p->active = 0;
                p->vy = 0;
                p->walk_accum = 0;
                p->vert_accum = 0;
                p->grounded = 0;
                return 1;
            }
        }
    }
    return 0;
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
