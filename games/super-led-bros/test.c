#include "super_led_bros.h"

#include "frame.h"
#include "input.h"
#include "stage.h"
#include "stage_01.h"
#include "stage_02.h"
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

static void idle(int n)
{
    int i;

    for (i = 0; i < n; i++)
        feed(0, 0, 0, 0, 0);
}

/* "Stage N" is 7 glyphs (41px) plus a 3px start inset and a 4px right margin. The strip is 48.
 * Travel across a 16-wide view is 32px at 15 FPS (4 ticks each), plus the 60-tick intro pause
 * and the 60-tick pause at the far edge. reset paints once without advancing. */
#define STAGE_CARD_TICKS 248

static void finish_card(void)
{
    int guard;

    for (guard = 0; guard < STAGE_CARD_TICKS && super_led_bros_on_card(); guard++)
        feed(0, 0, 0, 0, 0);
}

static void play_from_reset(void)
{
    input_reset();
    super_led_bros_reset();
    finish_card();
}

static void test_stage_spawn(void)
{
    expect(stage_01.spawn_x == 2 && stage_01.spawn_y == 12,
           "the white pixel defines spawn at (2, 12)");
    expect(!stage_map_solid(&stage_01, 2, 12), "spawn is not solid terrain");
    expect(stage_map_solid(&stage_01, 2, 13), "ground below spawn is solid");
    expect(stage_map_goal(&stage_01, 250, 9) && !stage_map_solid(&stage_01, 250, 9),
           "stage 1 goal is the magenta flag and is not solid");
    expect(stage_map_goal(&stage_02, 250, 9) && !stage_map_solid(&stage_02, 250, 9),
           "stage 2 goal is the magenta flag and is not solid");
}

static void test_player_start(void)
{
    play_from_reset();
    expect(super_led_bros_player_x() == 2 && super_led_bros_player_y() == 12,
           "the player starts on the spawn cell");
    expect(pixel_get(2, 12), "the player draws on the field");
}

static void test_walk_and_camera(void)
{
    play_from_reset();
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

    play_from_reset();
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

    play_from_reset();
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
    expect(status_get_digit(6) == 0 && status_get_digit(7) == SUPER_LED_BROS_LIVES, "lives start at 04");
    super_led_bros_apply_pickups(1);
    expect(status_get_digit(4) == 0 && status_get_digit(5) == 1, "one coin shows on digits 4 and 5");
    expect(status_get_digit(0) == 0 && status_get_digit(3) == 0, "score stays at zero when a coin is added");
    expect(status_get_digit(6) == 0 && status_get_digit(7) == SUPER_LED_BROS_LIVES,
           "lives stay at 04 when a coin is added");
    for (i = 0; i < 98; i++)
        super_led_bros_apply_pickups(1);
    expect(super_led_bros_coins() == 99 && status_get_digit(4) == 9 && status_get_digit(5) == 9,
           "coins reach 99");
    super_led_bros_apply_pickups(1);
    expect(super_led_bros_coins() == 0 && status_get_digit(4) == 0 && status_get_digit(5) == 0,
           "coins wrap from 99 to 00");
    expect(status_get_digit(0) == 0 && status_get_digit(7) == SUPER_LED_BROS_LIVES,
           "score stays at zero and lives stay at 04 after the coin wrap");
}

static void fall_off(void)
{
    super_led_bros_test_place_below();
    feed(0, 0, 0, 0, 0);
}

static void wait_death(int n)
{
    int i;

    for (i = 0; i < n; i++)
        feed(0, 0, 0, 0, 0);
}

static int lives_digit(void)
{
    return status_get_digit(6) * 10 + status_get_digit(7);
}

static void test_death_and_game_over(void)
{
    int i;

    play_from_reset();
    super_led_bros_apply_pickups(1);
    fall_off();
    expect(lives_digit() == SUPER_LED_BROS_LIVES, "the life counter stays through the death wait");
    expect(super_led_bros_player_y() >= stage_01.height, "the player is below the stage");
    wait_death(10);
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(super_led_bros_player_y() >= stage_01.height, "a button press does not skip the death wait");
    expect(lives_digit() == SUPER_LED_BROS_LIVES, "the life counter stays while the button is pressed");
    wait_death(SUPER_LED_BROS_DEATH_TICKS - 13);
    expect(super_led_bros_player_y() >= stage_01.height, "the stage stays down until the death wait ends");
    feed(0, 0, 0, 0, 0);
    expect(super_led_bros_player_x() == 2 && super_led_bros_player_y() == 12,
           "the stage restarts at spawn after 3 seconds");
    expect(lives_digit() == 3, "the life counter drops when the stage restarts");
    expect(super_led_bros_coins() == 1, "coins survive a death restart");

    for (i = 0; i < 4; i++) {
        fall_off();
        wait_death(SUPER_LED_BROS_DEATH_TICKS);
    }
    expect(lives_digit() == 0, "life 0 is lost into game over");
    expect(pixel_get(5, 5) && !pixel_get(4, 5), "game over draws the large G after 4 pixels");
    expect(!pixel_get(9, 5) && pixel_get(11, 5),
           "A follows G after the default one pixel gap");
    expect(!pixel_get(2, 12), "game over replaces the stage");
    feed_button();
    expect(lives_digit() == 0, "a button press does not skip the game over hold");
    expect(super_led_bros_player_y() >= stage_01.height, "game over stays up while the button is pressed");
    wait_death(SUPER_LED_BROS_OVER_TICKS - 2);
    expect(lives_digit() == 0, "game over lasts 3 seconds");
    feed_button();
    expect(lives_digit() == SUPER_LED_BROS_LIVES, "the button starts a new game with 4 lives");
    expect(super_led_bros_coins() == 0, "a new game clears the coins");
    expect(super_led_bros_on_card() && super_led_bros_stage_number() == 1,
           "a new game opens on the Stage 1 title");
    finish_card();
    expect(super_led_bros_player_x() == 2 && super_led_bros_player_y() == 12,
           "a new game starts on the spawn cell");
}

static void test_stage_card_and_clear(void)
{
    input_reset();
    super_led_bros_reset();
    expect(super_led_bros_on_card() && super_led_bros_stage_number() == 1,
           "stage 1 opens on its title");
    expect(pixel_get(4, 5) && !pixel_get(3, 5), "the title starts at column 3");
    expect(!pixel_get(2, 12), "the title replaces the stage");
    feed_button();
    expect(super_led_bros_on_card(), "the title ignores the action button");
    idle(186);
    expect(pixel_get(7, 5) && pixel_get(8, 5) && !pixel_get(12, 5) && !pixel_get(15, 5),
           "the leftward scroll stops with 4 blank pixels after the last letter");
    idle(STAGE_CARD_TICKS - 189);
    expect(super_led_bros_on_card(), "the title stays through the left-edge pause");
    feed(0, 0, 0, 0, 0);
    expect(!super_led_bros_on_card() && super_led_bros_stage_number() == 1,
           "the title ends when the left-edge pause ends");
    expect(super_led_bros_player_x() == 2 && super_led_bros_player_y() == 12,
           "stage 1 starts at spawn after the title");

    super_led_bros_apply_pickups(1);
    super_led_bros_test_place_on_goal();
    feed(0, 0, 0, 0, 0);
    expect(super_led_bros_clearing() && super_led_bros_player_x() == 250 &&
               super_led_bros_player_y() == 9,
           "the magenta flag freezes the player");
    feed(0, 0, 0, 1, 1);
    feed(0, 0, 0, 1, 1);
    expect(super_led_bros_clearing() && super_led_bros_player_x() == 250,
           "input does not move the player during the freeze");
    idle(SUPER_LED_BROS_CLEAR_TICKS - 3);
    expect(super_led_bros_clearing(), "the freeze lasts one second");
    feed(0, 0, 0, 0, 0);
    expect(super_led_bros_on_card() && super_led_bros_stage_number() == 2,
           "the freeze ends on the Stage 2 title");
    expect(pixel_get(4, 5) && !pixel_get(3, 5) && !pixel_get(2, 12),
           "Stage 2 uses the same large title at column 3");
    idle(STAGE_CARD_TICKS - 1);
    expect(super_led_bros_on_card(), "Stage 2 stays through the left-edge pause");
    feed(0, 0, 0, 0, 0);
    expect(!super_led_bros_on_card() && super_led_bros_stage_number() == 2,
           "Stage 2 starts when its title finishes");
    expect(super_led_bros_player_x() == (int)stage_02.spawn_x &&
               super_led_bros_player_y() == (int)stage_02.spawn_y,
           "stage 2 starts at its spawn");
    expect(super_led_bros_coins() == 1 && lives_digit() == SUPER_LED_BROS_LIVES,
           "coins and lives carry onto stage 2");

    fall_off();
    expect(!super_led_bros_on_card() && super_led_bros_stage_number() == 2,
           "a death on stage 2 does not show a title");
    idle(SUPER_LED_BROS_DEATH_TICKS);
    expect(!super_led_bros_on_card() && super_led_bros_stage_number() == 2,
           "stage 2 restarts without a title");
    expect(super_led_bros_player_x() == (int)stage_02.spawn_x && lives_digit() == 3,
           "the life counter drops on the stage 2 restart");
    expect(super_led_bros_coins() == 1, "coins survive the stage 2 death");

    super_led_bros_test_place_on_goal();
    feed(0, 0, 0, 0, 0);
    expect(super_led_bros_clearing(), "stage 2's flag starts the freeze");
    idle(SUPER_LED_BROS_CLEAR_TICKS);
    expect(super_led_bros_on_card() && super_led_bros_stage_number() == 1,
           "clearing the last stage returns to the Stage 1 title");
    finish_card();
    expect(!super_led_bros_on_card() && super_led_bros_stage_number() == 1,
           "the return lands on stage 1");
    expect(super_led_bros_player_x() == 2 && super_led_bros_player_y() == 12,
           "stage 1 starts at spawn again");
    expect(super_led_bros_coins() == 1 && lives_digit() == 3,
           "the return keeps coins and lives");
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
    test_death_and_game_over();
    test_stage_card_and_clear();
    return report_end();
}
