#include "platform.h"

#include "frame.h"
#include "input.h"
#include "stage.h"
#include "tests/report.h"

/*
 * Open 64x16 stage: solid floor on row 15 only, spawn at (10, 14). Enough room to walk
 * right and verify dead zone camera follow (defaults STAGE_H_DEAD_ZONE_X 5, width 6).
 */
static const uint16_t open_rows[16 * 4] = {
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0xffff,
    0xffff,
    0xffff,
    0xffff,
};

static const StageMap open_map = {
    64,
    16,
    4,
    10,
    14,
    open_rows,
    open_rows,
    0,
    0,
    0,
    0,
    0,
    0,
};

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void feed(PlatformPlayer *player, StageView *view, int left, int right)
{
    Input in;

    input_tick(0, 0, left, right, 0, &in);
    platform_player_tick(player, view, &in);
}

static void feed_stick(PlatformPlayer *player, StageView *view, int down, int left, int right)
{
    Input in;

    input_tick(0, down, left, right, 0, &in);
    platform_player_tick(player, view, &in);
}

static const uint16_t coin_rows[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xffff,
};
static const StageItem coin_item[] = {{4, 12, STAGE_ITEM_KIND_COIN}};
static const StageMap coin_map = {16, 16, 1, 4, 14, coin_rows, coin_rows, 0, 0, 0, 0, 1, coin_item};

/*
 * Same coin block y as coin_map, but spawn one row higher. A 1-tall player at py=13 would hit
 * with feet in the block; a grown player at py=14 hits with the head into y=12 first while feet
 * stay at y=13 (air), which used to skip stage_h_item_bump.
 */
static const StageMap coin_map_grown_probe = {16, 16, 1, 4, 14, coin_rows, coin_rows, 0, 0, 0, 0, 1, coin_item};

/* 16x16: solid floor row 15; #808080 ledge at row 9, x 10–12. Feet stand on row 8. */
static const uint16_t semi_only_floor_lit[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1c00, 0, 0, 0, 0, 0, 0xffff,
};
static const uint16_t semi_only_floor_sol[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xffff,
};
static const uint16_t semi_only_floor_plat[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1c00, 0, 0, 0, 0, 0, 0,
};
static const StageMap semi_platform_map = {
    16,
    16,
    1,
    11,
    8,
    semi_only_floor_lit,
    semi_only_floor_sol,
    semi_only_floor_plat,
    0,
    0,
    0,
    0,
    0,
};

static int tick_button(PlatformPlayer *player, StageView *view, int button)
{
    Input in;

    input_tick(0, 0, 0, 0, button, &in);
    return platform_player_tick(player, view, &in);
}

static void test_item_pickup(void)
{
    StageView view;
    PlatformPlayer player;
    int got;

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &coin_map);
    platform_player_bind(&player, &view, &coin_map);
    expect(tick_button(&player, &view, 0) == 0, "an idle tick collects nothing");
    expect(tick_button(&player, &view, 1) == 0, "the button debounce tick collects nothing");
    got = tick_button(&player, &view, 1);
    expect(got == 1, "a jump into an item block collects it");
    expect(platform_player_y(&player) == 14, "the player stays under the block");
    tick_button(&player, &view, 0);
    tick_button(&player, &view, 0);
    tick_button(&player, &view, 1);
    expect(tick_button(&player, &view, 1) == 0, "a used item block does not collect again");
}

static void run_ticks(PlatformPlayer *player, StageView *view, int n)
{
    int i;

    for (i = 0; i < n; i++)
        feed(player, view, 0, 0);
}

static void test_semisolid_walk_and_fall_through(void)
{
    StageView view;
    PlatformPlayer player;

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &semi_platform_map);
    platform_player_bind(&player, &view, &semi_platform_map);
    expect(player.grounded, "spawn one row above semisolid counts as grounded");
    expect(platform_player_y(&player) == 8, "spawn feet stand on the row above the ledge");
    feed(&player, &view, 0, 1);
    feed(&player, &view, 0, 1);
    feed(&player, &view, 0, 1);
    feed(&player, &view, 0, 1);
    feed(&player, &view, 0, 1);
    expect(platform_player_x(&player) == 12, "walk right across semisolid span");
    feed(&player, &view, 0, 1);
    feed(&player, &view, 0, 1);
    feed(&player, &view, 0, 1);
    feed(&player, &view, 0, 1);
    feed(&player, &view, 0, 1);
    expect(platform_player_x(&player) == 13, "walk off the right end of the bridge");
    run_ticks(&player, &view, 80);
    expect(platform_player_y(&player) > 8, "after stepping off semisolid gravity pulls the player down");
}

/* 16x16 floor row 15; semisolid ledge row 10 at x=8; spawn feet row 8 so a tall body can fall onto the ledge. */
static const uint16_t semi_tall_lit[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0100, 0, 0, 0, 0, 0xffff,
};
static const uint16_t semi_tall_sol[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xffff,
};
static const uint16_t semi_tall_plat[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0100, 0, 0, 0, 0, 0,
};
static const StageMap semi_tall_map = {
    16,
    16,
    1,
    8,
    8,
    semi_tall_lit,
    semi_tall_sol,
    semi_tall_plat,
    0,
    0,
    0,
    0,
    0,
};

static void test_grown_lands_on_semisolid(void)
{
    StageView view;
    PlatformPlayer player;

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &semi_tall_map);
    platform_player_bind(&player, &view, &semi_tall_map);
    player.grown = 1;
    expect(!player.grounded, "spawn in air above ledge");
    run_ticks(&player, &view, 120);
    expect(platform_player_y(&player) == 9, "tall player feet stop on the row above semisolid");
    expect(player.grounded, "tall player is grounded on semisolid");
}

static void test_grown_coin_block_from_below(void)
{
    StageView view;
    PlatformPlayer player;
    int got;

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &coin_map_grown_probe);
    platform_player_bind(&player, &view, &coin_map_grown_probe);
    player.grown = 1;
    expect(tick_button(&player, &view, 0) == 0, "grown idle tick collects nothing");
    expect(tick_button(&player, &view, 1) == 0, "grown button debounce collects nothing");
    got = tick_button(&player, &view, 1);
    expect(got == 1, "grown player jump bumps coin when head hits first");
    expect(platform_player_y(&player) == 14, "grown player stays under the block");
}

/* Row 13 is solid only at x=6, so a 2-tall body on row 14 cannot enter that column. */
static const uint16_t crouch_rows[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0040, 0, 0xffff,
};
static const StageMap crouch_map = {
    16, 16, 1, 4, 14, crouch_rows, crouch_rows, 0, 0, 0, 0, 0, 0,
};

static void hold_down(PlatformPlayer *player, StageView *view, int n)
{
    int i;

    for (i = 0; i < n; i++)
        feed_stick(player, view, 1, 0, 0);
}

static void test_crouch(void)
{
    StageView view;
    PlatformPlayer player;
    int i;

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &crouch_map);
    platform_player_bind(&player, &view, &crouch_map);
    hold_down(&player, &view, 4);
    expect(!player.crouch, "a small player does not crouch");

    input_reset();
    platform_player_bind(&player, &view, &crouch_map);
    player.grown = 1;
    for (i = 0; i < 12; i++)
        feed(&player, &view, 0, 1);
    expect(platform_player_x(&player) == 5, "a tall player stops before the low ceiling");

    input_reset();
    platform_player_bind(&player, &view, &crouch_map);
    player.grown = 1;
    hold_down(&player, &view, 2);
    expect(player.crouch, "holding down crouches a tall player");
    clear();
    platform_h_paint(&view, &player);
    expect(pixel_get(4, 14) && !pixel_get(4, 13), "a crouch draws one cell");
    for (i = 0; i < 16; i++)
        feed_stick(&player, &view, 1, 0, 1);
    expect(platform_player_x(&player) >= 6, "a crouch fits under the low ceiling");

    input_reset();
    platform_player_bind(&player, &view, &crouch_map);
    player.grown = 1;
    player.px = 6;
    hold_down(&player, &view, 2);
    feed_stick(&player, &view, 0, 0, 0);
    feed_stick(&player, &view, 0, 0, 0);
    expect(player.crouch, "releasing down under a ceiling keeps the crouch");
    feed(&player, &view, 1, 0);
    feed(&player, &view, 1, 0);
    feed(&player, &view, 1, 0);
    feed(&player, &view, 1, 0);
    feed(&player, &view, 1, 0);
    expect(platform_player_x(&player) < 6, "leaving the ceiling lets the player stand");
    feed(&player, &view, 0, 0);
    expect(!player.crouch, "the player stands once the head cell is clear");
}

/* Open shaft: spawn in the air at row 2, floor on row 15. */
static const uint16_t pit_rows[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xffff,
};
static const StageMap pit_map = {16, 16, 1, 4, 2, pit_rows, pit_rows, 0, 0, 0, 0, 0, 0};

static void test_ledge_fall_accelerates(void)
{
    StageView view;
    PlatformPlayer player;
    int i;

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &pit_map);
    platform_player_bind(&player, &view, &pit_map);
    expect(platform_player_y(&player) == 2, "fall test starts in the air");
    for (i = 0; i < 5; i++)
        feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 2, "arming plus four idle ticks do not drop a cell");
    feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 3, "the first fall cell matches the 4-tick jump descent wait");
    for (i = 0; i < 3; i++)
        feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 3, "the next wait is 3 idle ticks");
    feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 4, "the second fall cell follows the jump descent curve");
    for (i = 0; i < 2; i++)
        feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 4, "the next wait is 2 idle ticks");
    feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 5, "the fall steps after that wait");
    feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 5, "the fastest wait is one idle tick");
    feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 6, "the next cell arrives on the following tick");
    feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 6, "terminal speed stays at one cell every two ticks");
    feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 7, "the fall does not speed up to one cell per tick");
}

static void test_dead_zone_camera(void)
{
    StageView view;
    PlatformPlayer player;
    int i;

    input_reset();
    clear();
    stage_view_init(&view);
    stage_bind_h(&view, &open_map);
    platform_player_bind(&player, &view, &open_map);
    expect(platform_player_x(&player) == 10, "spawn world x 10");
    expect(stage_h_camera_get(&view) == 0, "camera starts at 0");
    /* Screen x = 10 is inside zone 5..10 inclusive. One walk step needs debounce then 4 ticks
     * at 15/60 accum per tick (5 feeds cover debounce + one step). */
    for (i = 0; i < 5; i++)
        feed(&player, &view, 0, 1);
    expect(platform_player_x(&player) == 11, "one cell right at 15 FPS");
    expect(stage_h_camera_get(&view) == 1, "camera moves when focus exits zone right edge");
}

/* Floor on row 2, spawn standing on it. A full jump rises through the top of the screen. */
static const uint16_t sky_rows[16] = {
    0, 0, 0xffff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const StageMap sky_map = {16, 16, 1, 4, 1, sky_rows, sky_rows, 0, 0, 0, 0, 0, 0};

/* Solid top row and a floor on row 3. The ceiling is terrain, not the screen edge. */
static const uint16_t low_ceiling_rows[16] = {
    0xffff, 0, 0, 0xffff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const StageMap low_ceiling_map = {16, 16, 1, 4, 2, low_ceiling_rows, low_ceiling_rows, 0, 0, 0, 0, 0, 0};

/* Floor on row 1 so a tall player's head sits above the screen. */
static const uint16_t top_rows[16] = {
    0, 0xffff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const StageMap top_map = {16, 16, 1, 4, 0, top_rows, top_rows, 0, 0, 0, 0, 0, 0};

static void test_above_screen_keeps_moving(void)
{
    StageView view;
    PlatformPlayer player;
    int i;
    int peak;

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &sky_map);
    platform_player_bind(&player, &view, &sky_map);
    peak = platform_player_y(&player);
    for (i = 0; i < 120; i++) {
        tick_button(&player, &view, 1);
        if (platform_player_y(&player) < peak)
            peak = platform_player_y(&player);
    }
    expect(peak == -4, "a full jump rises through the top of the screen");
    expect(platform_player_y(&player) == 1 && player.grounded, "the player lands back on the floor");

    input_reset();
    platform_player_bind(&player, &view, &sky_map);
    player.py = -3;
    player.grounded = 0;
    for (i = 0; i < 5; i++)
        feed(&player, &view, 0, 1);
    expect(platform_player_x(&player) == 5, "the player walks while above the screen");
    expect(platform_player_y(&player) < 0, "walking above the screen does not snap back into view");
    clear();
    platform_h_paint(&view, &player);
    expect(!pixel_get(5, 0), "a player above the screen is not drawn");

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &top_map);
    platform_player_bind(&player, &view, &top_map);
    player.grown = 1;
    for (i = 0; i < 5; i++)
        feed(&player, &view, 0, 1);
    expect(platform_player_x(&player) == 5 && platform_player_y(&player) == 0,
           "a tall player walks on the top row with the head off screen");

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &low_ceiling_map);
    platform_player_bind(&player, &view, &low_ceiling_map);
    peak = platform_player_y(&player);
    for (i = 0; i < 80; i++) {
        tick_button(&player, &view, 1);
        if (platform_player_y(&player) < peak)
            peak = platform_player_y(&player);
    }
    expect(peak == 1, "a solid cell on the top row still blocks a jump");
}

/* No floor at all. The bottom edge must not invent one. */
static const uint16_t hole_rows[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static const StageMap hole_map = {16, 16, 1, 4, 14, hole_rows, hole_rows, 0, 0, 0, 0, 0, 0};

static void test_fall_through_bottom(void)
{
    StageView view;
    PlatformPlayer player;
    int i;

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &hole_map);
    platform_player_bind(&player, &view, &hole_map);
    for (i = 0; i < 40; i++)
        feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) >= 16, "an open bottom lets the player fall off the stage");
    expect(!player.grounded, "the row below the stage is not ground");

    input_reset();
    stage_view_init(&view);
    stage_bind_h(&view, &pit_map);
    platform_player_bind(&player, &view, &pit_map);
    for (i = 0; i < 80; i++)
        feed(&player, &view, 0, 0);
    expect(platform_player_y(&player) == 14 && player.grounded, "a solid bottom row still catches the player");
}

int main(void)
{
    report_begin("platform");
    test_dead_zone_camera();
    test_item_pickup();
    test_grown_coin_block_from_below();
    test_semisolid_walk_and_fall_through();
    test_grown_lands_on_semisolid();
    test_crouch();
    test_ledge_fall_accelerates();
    test_above_screen_keeps_moving();
    test_fall_through_bottom();
    return report_end();
}
