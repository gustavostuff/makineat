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

int main(void)
{
    report_begin("platform");
    test_dead_zone_camera();
    return report_end();
}
