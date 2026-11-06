#include "stage.h"

#include "frame.h"
#include "tests/report.h"

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static const uint16_t tiny_rows[] = {
    0x0001, 0x0001,
};

static const StageMap tiny = {32, 1, 2, 0, 0, tiny_rows, tiny_rows, 0, 0, 0, 0, 0, 0};

static void test_map_get(void)
{
    expect(stage_map_get(&tiny, 0, 0), "bit 0 of row 0 is set");
    expect(!stage_map_get(&tiny, 1, 0), "neighbor cell is clear");
    expect(stage_map_get(&tiny, 16, 0), "second word column is addressable");
    expect(!stage_map_get(&tiny, -1, 0), "out of bounds reads empty");
}

static void test_h_paint_and_camera(void)
{
    StageView view;

    stage_view_init(&view);
    stage_bind_h(&view, &tiny);
    stage_h_paint(&view);
    expect(pixel_get(0, 0), "home camera shows the left marker");
    stage_h_camera_set(&view, 1);
    stage_h_paint(&view);
    expect(!pixel_get(0, 0) && pixel_get(15, 0), "camera offset shifts visible art");
}

static const uint16_t para_fg_lit[] = {
    0xffff,
    0xffff,
    0xffff,
    0xffff,
};
static const uint16_t para_fg_sol[] = {0x0000, 0x0000, 0x0000, 0x0000};
static const uint16_t para_bg_same[] = {
    0xffff,
    0xffff,
};

static void bind_scroll_map(StageView *view, StageMap *map, int fg_w, int bg_w)
{
    map->width = (uint16_t)fg_w;
    map->height = 1;
    map->row_words = 1;
    map->spawn_x = 0;
    map->spawn_y = 0;
    map->fg_lit = para_fg_lit;
    map->fg_solid = para_fg_sol;
    map->fg_semisolid = 0;
    map->bg_width = (uint16_t)bg_w;
    map->bg_row_words = 1;
    map->bg_lit = bg_w > 0 ? para_bg_same : 0;
    map->item_count = 0;
    map->items = 0;
    stage_view_init(view);
    stage_bind_h(view, map);
}

static void test_h_scroll_math(void)
{
    StageView view;
    StageMap map;

    /* Equal widths: spans match, so the cameras stay locked. */
    bind_scroll_map(&view, &map, 32, 32);
    expect(stage_h_camera_get(&view) == 0 && stage_h_bg_camera_get(&view) == 0,
           "both cameras start at 0");
    stage_h_camera_set(&view, 10);
    expect(stage_h_bg_camera_get(&view) == 10, "equal widths keep bg locked to fg");
    stage_h_camera_set(&view, 16);
    expect(stage_h_camera_get(&view) == 16 && stage_h_bg_camera_get(&view) == 16,
           "equal widths end together");

    /* fg 64, bg 32: spans 48 and 16. */
    bind_scroll_map(&view, &map, 64, 32);
    stage_h_camera_set(&view, 20);
    expect(stage_h_bg_camera_get(&view) == 6, "mid scroll uses bg span over fg span");
    stage_h_camera_set(&view, 48);
    expect(stage_h_bg_camera_get(&view) == 16, "narrower bg reaches its end with the fg");
    stage_h_camera_set(&view, 1);
    expect(stage_h_bg_camera_get(&view) == 0, "integer span math can stay at 0 for one fg step");

    /* Stage 01 shape: fg 256 (span 240), bg 64 (span 48). */
    bind_scroll_map(&view, &map, 256, 64);
    stage_h_camera_set(&view, 120);
    expect(stage_h_bg_camera_get(&view) == 24, "256 by 64 mid point is one fifth of the fg camera");
    stage_h_camera_set(&view, 192);
    expect(stage_h_bg_camera_get(&view) == 38,
           "256 by 64 still scrolls after the old width-ratio clamp");
    stage_h_camera_set(&view, 999);
    expect(stage_h_camera_get(&view) == 240 && stage_h_bg_camera_get(&view) == 48,
           "fg clamp at 240 lands bg on its last column");
    stage_h_camera_set(&view, -4);
    expect(stage_h_camera_get(&view) == 0 && stage_h_bg_camera_get(&view) == 0,
           "a negative camera request clamps both layers to 0");

    /* Background wider than foreground: fg span 16, bg span 48. */
    bind_scroll_map(&view, &map, 32, 64);
    stage_h_camera_set(&view, 8);
    expect(stage_h_bg_camera_get(&view) == 24, "a wider bg moves farther per fg step");
    stage_h_camera_set(&view, 16);
    expect(stage_h_bg_camera_get(&view) == 48, "a wider bg also ends with the fg");

    /* A layer that is only one screen wide has no travel. */
    bind_scroll_map(&view, &map, 64, 16);
    stage_h_camera_set(&view, 20);
    expect(stage_h_bg_camera_get(&view) == 0, "a 16-wide bg stays pinned");
    bind_scroll_map(&view, &map, 16, 64);
    stage_h_camera_set(&view, 8);
    expect(stage_h_camera_get(&view) == 0 && stage_h_bg_camera_get(&view) == 0,
           "a 16-wide fg cannot scroll the bg");

    /* No background layer. */
    bind_scroll_map(&view, &map, 64, 0);
    stage_h_camera_set(&view, 20);
    expect(stage_h_bg_camera_get(&view) == 0, "missing bg keeps the bg camera at 0");
}

static void test_h_suppress_player_on_bg(void)
{
    StageView view;
    static const uint16_t fg_empty[] = {0x0000};
    static const uint16_t fg_solid[] = {0x0001};
    static const uint16_t bg_lit[] = {0x0001};
    static const StageMap bg_only = {16, 1, 1, 0, 0, fg_empty, fg_empty, 0, 16, 1, bg_lit, 0, 0};
    static const StageMap solid_fg = {16, 1, 1, 0, 0, fg_solid, fg_solid, 0, 16, 1, bg_lit, 0, 0};

    stage_view_init(&view);
    stage_bind_h(&view, &bg_only);
    stage_h_paint_ex(&view, 0, 0, 1, 1);
    expect(!pixel_get(0, 0), "background under the player stays off in the framebuffer");
    expect(stage_h_suppress_player_led(&view, 0, 0),
           "player LED stays off on background art with empty foreground");
    expect(!stage_h_suppress_player_led(&view, 1, 0),
           "player LED draws on an empty cell");
    stage_bind_h(&view, &solid_fg);
    expect(!stage_h_suppress_player_led(&view, 0, 0),
           "player LED draws on solid foreground even when background is lit");
}

static void test_h_dead_zone_follow(void)
{
    StageView view;

    stage_view_init(&view);
    stage_bind_h(&view, &tiny);
    /* Default zone is screen x 5..10. Focus left of zone: new camera clips at 0. */
    stage_h_camera_set(&view, 5);
    stage_h_camera_follow_dead_zone(&view, 2);
    expect(stage_h_camera_get(&view) == 0, "dead zone pans left and clips at map edge");
    /* Focus inside zone 5..10 at screen x 7, camera unchanged. */
    stage_h_camera_set(&view, 5);
    stage_h_camera_follow_dead_zone(&view, 12);
    expect(stage_h_camera_get(&view) == 5, "dead zone does nothing when focus is inside zone");
    /* Focus right of zone at screen x 11, camera moves right by one. */
    stage_h_camera_set(&view, 0);
    stage_h_camera_follow_dead_zone(&view, 11);
    expect(stage_h_camera_get(&view) == 1, "dead zone pans right when focus exits zone right edge");
}

static const uint16_t item_rows[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static const StageItem one_item[] = {{4, 6, STAGE_ITEM_KIND_COIN}};
static const StageMap item_map = {16, 8, 1, 0, 0, item_rows, item_rows, 0, 0, 0, 0, 1, one_item};

static void test_item_block_bump(void)
{
    int i;
    StageView view;

    expect(stage_item_covers(&item_map, 4, 6), "item block covers its top-left cell");
    expect(stage_item_covers(&item_map, 5, 7), "item block covers its bottom-right cell");
    expect(!stage_item_covers(&item_map, 6, 6), "the cell beside an item block is open");
    stage_view_init(&view);
    stage_bind_h(&view, &item_map);
    stage_h_paint(&view);
    expect(pixel_get(4, 6) && pixel_get(5, 6) && pixel_get(4, 7) && pixel_get(5, 7),
           "a resting item block shows the first shine pattern");
    expect(!pixel_get(4, 5), "a resting item block does not draw above itself");
    expect(stage_h_item_bump(&view, 0, 0) == STAGE_BUMP_NONE, "a miss does not use the item block");
    stage_h_paint(&view);
    expect(pixel_get(4, 6) && !pixel_get(4, 5), "a miss does not bump the item block");
    expect(stage_h_item_bump(&view, 5, 7) == STAGE_BUMP_COIN, "the first hit uses the item block");
    stage_h_paint(&view);
    expect(pixel_get(4, 5) && pixel_get(5, 5) && pixel_get(4, 6) && pixel_get(5, 6), "a hit draws the used block one cell higher");
    expect(!pixel_get(4, 7) && !pixel_get(5, 7), "a bumped item block leaves its old bottom row clear");
    stage_h_items_tick(&view);
    expect(stage_h_item_bump(&view, 5, 7) == STAGE_BUMP_NONE,
           "a used item block does not award another pickup");
    for (i = 0; i < STAGE_ITEM_BOUNCE_TICKS - 1; i++)
        stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(pixel_get(4, 6) && pixel_get(5, 7) && !pixel_get(4, 5), "the item block returns after one bounce");
}

static int shine_block_full(void)
{
    return pixel_get(4, 6) && pixel_get(5, 6) && pixel_get(4, 7) && pixel_get(5, 7);
}

static const uint16_t growth_rows[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xffff, 0xffff, 0xffff,
};
static const StageItem growth_item[] = {{4, 12, STAGE_ITEM_KIND_GROWTH}};
static const StageMap growth_map = {16, 16, 1, 0, 0, growth_rows, growth_rows, 0, 0, 0, 0, 1, growth_item};

/* Row 10: one-pixel semisolid platform at x=4; row 15: full red floor */
static const uint16_t semi_lit[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0010, 0, 0, 0, 0, 0xffff,
};
static const uint16_t semi_sol[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xffff,
};
static const uint16_t semi_plat[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0010, 0, 0, 0, 0, 0,
};
static const StageMap semisolid_map = {
    16,
    16,
    1,
    4,
    8,
    semi_lit,
    semi_sol,
    semi_plat,
    0,
    0,
    0,
    0,
    0,
};

static void test_semisolid_platform(void)
{
    StageView view;

    stage_view_init(&view);
    stage_bind_h(&view, &semisolid_map);
    expect(stage_map_semisolid(&semisolid_map, 4, 10), "semisolid cell is in fg_semisolid");
    expect(!stage_map_solid(&semisolid_map, 4, 10), "semisolid is not red solid");
    expect(!stage_h_blocks_horiz(&view, 4, 10), "semisolid does not block horizontal walk");
    expect(stage_h_blocks_vert(&view, 4, 10, 9, 1), "falling into a semisolid cell is blocked");
    expect(!stage_h_blocks_vert(&view, 4, 10, 11, -1), "jumping up into a semisolid cell passes through");
}

static void test_growth_pickup(void)
{
    int i;
    StageView view;

    stage_view_init(&view);
    stage_bind_h(&view, &growth_map);
    expect(stage_h_item_bump(&view, 4, 13) == STAGE_BUMP_GROWTH, "growth block bump from below");
    for (i = 0; i < STAGE_ITEM_BOUNCE_TICKS - 1; i++)
        stage_h_items_tick(&view);
    stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(pixel_get(4, 11), "growth pickup appears one row above after the bounce");
    for (i = 0; i < 14; i++)
        stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(pixel_get(5, 11), "pickup moves right at 4 FPS after fifteen ticks");
}

static void test_pickup_fall_waits(void)
{
    int i;
    int born = -1;
    int off = -1;
    int dropped = -1;
    StageView view;

    stage_view_init(&view);
    stage_bind_h(&view, &growth_map);
    expect(stage_h_item_bump(&view, 4, 13) == STAGE_BUMP_GROWTH, "pickup fall starts from a growth bump");
    for (i = 0; i < 80; i++) {
        stage_h_items_tick(&view);
        stage_h_paint(&view);
        if (born < 0 && pixel_get(4, 11))
            born = i;
        if (born >= 0 && off < 0 && pixel_get(6, 11))
            off = i;
        if (off >= 0 && dropped < 0 && pixel_get(6, 12))
            dropped = i;
    }
    expect(born >= 0 && off > born, "the pickup walks off the block");
    expect(dropped - off == 6, "the pickup's first fall cell uses the jump descent wait");
}

static void test_item_block_shine(void)
{
    int i;
    StageView view;

    stage_view_init(&view);
    stage_bind_h(&view, &item_map);
    stage_h_paint(&view);
    expect(shine_block_full(), "shine pattern 1 lights every cell");
    for (i = 0; i < STAGE_ITEM_SHINE_HOLD_LONG; i++)
        stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(shine_block_full(), "the first shine pattern holds for 1 second");
    stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(!pixel_get(4, 6) && pixel_get(5, 6) && pixel_get(4, 7) && pixel_get(5, 7),
           "shine pattern 2 leaves the top-left cell off");
    expect(stage_h_solid(&view, 4, 6), "an item block stays solid on a dark shine cell");
    for (i = 0; i < STAGE_ITEM_SHINE_HOLD_SHORT - 1; i++)
        stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(!pixel_get(4, 6) && pixel_get(5, 7), "shine pattern 2 holds for 6 frames");
    stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(pixel_get(4, 6) && !pixel_get(5, 6) && !pixel_get(4, 7) && pixel_get(5, 7),
           "shine pattern 3 lights the top-left and bottom-right cells");
    for (i = 0; i < STAGE_ITEM_SHINE_HOLD_SHORT - 1; i++)
        stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(pixel_get(4, 6) && !pixel_get(5, 6), "shine pattern 3 holds for 6 frames");
    stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(pixel_get(4, 6) && pixel_get(5, 6) && pixel_get(4, 7) && !pixel_get(5, 7),
           "shine pattern 4 leaves the bottom-right cell off");
    for (i = 0; i < STAGE_ITEM_SHINE_HOLD_SHORT - 1; i++)
        stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(pixel_get(4, 6) && !pixel_get(5, 7), "shine pattern 4 holds for 6 frames");
    stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(shine_block_full(), "the shine returns to the full block");
    for (i = 0; i < STAGE_ITEM_SHINE_HOLD_LONG; i++)
        stage_h_items_tick(&view);
    stage_h_item_bump(&view, 5, 7);
    stage_h_paint(&view);
    expect(pixel_get(4, 5) && pixel_get(5, 5) && pixel_get(4, 6) && pixel_get(5, 6),
           "a used block lifts as a solid 2x2");
    for (i = 0; i < STAGE_ITEM_BOUNCE_TICKS + STAGE_TICK_HZ; i++)
        stage_h_items_tick(&view);
    stage_h_paint(&view);
    expect(pixel_get(4, 6) && pixel_get(5, 6) && pixel_get(4, 7) && pixel_get(5, 7),
           "a used block stays fully lit after the shine would have moved");
    expect(!pixel_get(4, 5), "a used block does not stay lifted");
    stage_h_item_bump(&view, 4, 6);
    stage_h_paint(&view);
    expect(!pixel_get(4, 5) && pixel_get(4, 6), "a used item block does not bounce again");
}

int main(void)
{
    report_begin("stage");
    test_map_get();
    test_h_paint_and_camera();
    test_h_scroll_math();
    test_h_suppress_player_on_bg();
    test_h_dead_zone_follow();
    test_item_block_bump();
    test_growth_pickup();
    test_pickup_fall_waits();
    test_item_block_shine();
    test_semisolid_platform();
    return report_end();
}
