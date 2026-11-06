#include "super_led_bros.h"

#include "frame.h"
#include "input.h"
#include "stage.h"
#include "stage_01.h"
#include "status.h"
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
    expect(stage_01.spawn_x == 2 && stage_01.spawn_y == 12,
           "the white pixel defines spawn at (2, 12)");
    expect(!stage_map_solid(&stage_01, 2, 12), "spawn is not solid terrain");
    expect(stage_map_solid(&stage_01, 2, 13), "ground below spawn is solid");
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
    expect(super_led_bros_camera_x() == 0, "dead zone keeps camera still while focus stays inside");
}

static void test_jump_full_hold(void)
{
    int i;
    int y;
    int peak_y = 12;

    input_reset();
    super_led_bros_reset();
    feed_button();
    for (i = 0; i < 80; i++) {
        feed(0, 0, 0, 0, 1);
        y = super_led_bros_player_y();
        if (y < peak_y)
            peak_y = y;
    }
    expect(peak_y == 7, "full jump rises 5 cells with button held");
}

static void test_jump_short_release(void)
{
    int i;
    int y;
    int peak_y = 12;

    input_reset();
    super_led_bros_reset();
    feed_button();
    feed(0, 0, 0, 0, 0);
    feed(0, 0, 0, 0, 0);
    for (i = 0; i < 80; i++) {
        feed(0, 0, 0, 0, 0);
        y = super_led_bros_player_y();
        if (y < peak_y)
            peak_y = y;
    }
    expect(peak_y == 10, "minimum jump rises 2 cells after early button release");
}

static void test_coins(void)
{
    int i;

    input_reset();
    super_led_bros_reset();
    expect(status_get_digit(0) == 0 && status_get_digit(1) == 0 && status_get_digit(2) == 0 &&
               status_get_digit(3) == 0,
           "score starts at 0000");
    expect(status_get_digit(4) == 0 && status_get_digit(5) == 0, "coins start at 00");
    expect(status_get_digit(6) == 0 && status_get_digit(7) == 0, "lives start at 00");
    super_led_bros_apply_pickups(1);
    expect(status_get_digit(4) == 0 && status_get_digit(5) == 1, "one coin shows on digits 4 and 5");
    expect(status_get_digit(0) == 0 && status_get_digit(3) == 0, "score stays at zero when a coin is added");
    expect(status_get_digit(6) == 0 && status_get_digit(7) == 0, "lives stay at zero when a coin is added");
    for (i = 0; i < 98; i++)
        super_led_bros_apply_pickups(1);
    expect(super_led_bros_coins() == 99 && status_get_digit(4) == 9 && status_get_digit(5) == 9,
           "coins reach 99");
    super_led_bros_apply_pickups(1);
    expect(super_led_bros_coins() == 0 && status_get_digit(4) == 0 && status_get_digit(5) == 0,
           "coins wrap from 99 to 00");
    expect(status_get_digit(0) == 0 && status_get_digit(6) == 0, "score and lives stay at zero after wrap");
}

static void test_vertical_stub(void)
{
    StageView view;

    stage_view_init(&view);
    expect(stage_bind_v(&view, &stage_01) == STAGE_ERR_UNSUPPORTED,
           "vertical scroll mode is not implemented yet");
}

int main(void)
{
    report_begin("super-led-bros");
    test_stage_spawn();
    test_vertical_stub();
    test_player_start();
    test_walk_and_camera();
    test_jump_full_hold();
    test_jump_short_release();
    test_coins();
    return report_end();
}
