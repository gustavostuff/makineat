#include "super_led_bros.h"

#include "frame.h"
#include "input.h"
#include "stage.h"
#include "stage_01_test.h"
#include "tests/report.h"

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void feed(int up, int down, int left, int right, int button)
{
    Input in;

    input_tick(up, down, left, right, button, &in);
    super_led_bros_tick(&in);
}

static void feed_button(void)
{
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
}

static void test_stage_spawn(void)
{
    expect(stage_01_test.spawn_x == 2 && stage_01_test.spawn_y == 12,
           "the red pixel defines spawn at (2, 12)");
    expect(!stage_map_get(&stage_01_test, 2, 12), "spawn is not solid terrain");
    expect(stage_map_get(&stage_01_test, 2, 13), "ground below spawn is solid");
}

static void test_player_start(void)
{
    input_reset();
    super_led_bros_reset();
    expect(super_led_bros_player_x() == 2 && super_led_bros_player_y() == 12,
           "the player starts on the spawn cell");
    expect(pixel_get(2, 12), "the player draws on the field");
}

static void test_walk_and_camera(void)
{
    input_reset();
    super_led_bros_reset();
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    expect(super_led_bros_player_x() == 3, "holding right walks at 15 FPS");
    expect(super_led_bros_camera_x() == 0, "camera stays pinned at the left edge early on");
}

static void test_jump(void)
{
    int i;
    int y;
    int peak_y = 12;

    input_reset();
    super_led_bros_reset();
    feed_button();
    for (i = 0; i < 40; i++) {
        feed(0, 0, 0, 0, 0);
        y = super_led_bros_player_y();
        if (y < peak_y)
            peak_y = y;
    }
    expect(peak_y == 7, "jump rises 5 cells at 15 FPS");
}

static void test_vertical_stub(void)
{
    StageView view;

    stage_view_init(&view);
    expect(stage_bind_v(&view, &stage_01_test) == STAGE_ERR_UNSUPPORTED,
           "vertical scroll mode is not implemented yet");
}

int main(void)
{
    report_begin("super-led-bros");
    test_stage_spawn();
    test_vertical_stub();
    test_player_start();
    test_walk_and_camera();
    test_jump();
    return report_end();
}
