#include "snake.h"

#include <string.h>

#include "frame.h"
#include "input.h"
#include "status.h"
#include "tests/report.h"

#define TICK_HZ 60
#define STEP_FRAMES (TICK_HZ / 2)
/*
 * A bounce cycle is three 60-frame pauses plus the out-and-back travel.
 * Large glyphs are 5px with the default 1px gap, plus 4px margins.
 * snake_reset already paints one frame of the first pause.
 */
#define TITLE_BOUNCE_CYCLE_FRAMES 539
#define TITLE_PRESS_CYCLE_FRAMES 684

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void feed(int up, int down, int left, int right, int button)
{
    Input in;

    input_tick(up, down, left, right, button, &in);
    snake_tick(&in);
}

static void feed_pressed(int up, int down, int left, int right)
{
    feed(up, down, left, right, 0);
    feed(up, down, left, right, 0);
}

static void feed_button(void)
{
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
}

static void leave_title(void)
{
    if (snake_on_title())
        feed_button();
}

static void reset_play(unsigned seed)
{
    snake_seed(seed);
    snake_reset();
    leave_title();
}

static void idle(int n)
{
    int i;

    for (i = 0; i < n; i++)
        feed(0, 0, 0, 0, 0);
}

static int food_clear_of_body(void)
{
    int slot;
    int fx;
    int fy;

    for (slot = 0; slot < 4; slot++) {
        if (!snake_food_xy(slot, &fx, &fy))
            continue;
        if (snake_body_at(fx, fy))
            return 0;
    }
    return 1;
}

static void test_food_not_on_body(void)
{
    int i;

    input_reset();
    reset_play(42);
    expect(food_clear_of_body(), "spawned food stays off the snake map");
    for (i = 0; i < 5; i++)
        idle(STEP_FRAMES);
    expect(food_clear_of_body(), "respawned food stays off the snake map");
}

static void test_title(void)
{
    int d;
    int i;

    input_reset();
    snake_seed(42);
    snake_reset();
    expect(snake_on_title(), "reset opens on the title screen");
    expect(strcmp(snake_title_message(), "S n a k e") == 0,
           "the title starts with spaced Snake scroll-bounce");
    for (d = 0; d < 8; d++)
        expect(status_get_digit(d) == 0, "title keeps status digits off");
    idle(TITLE_BOUNCE_CYCLE_FRAMES);
    expect(strcmp(snake_title_message(), "PRESS BUTTON") == 0,
           "a finished bounce switches to PRESS BUTTON");
    idle(TITLE_PRESS_CYCLE_FRAMES);
    expect(strcmp(snake_title_message(), "S n a k e") == 0,
           "the next finished bounce returns to spaced Snake");
    for (i = 0; i < 8; i++)
        expect(status_get_digit(i) == 0, "digits stay off while the title runs");
    feed_button();
    expect(!snake_on_title(), "the button leaves the title");
    expect(snake_length() == 3, "play starts with a length-three snake");
}

static void test_start(void)
{
    input_reset();
    reset_play(42);

    expect(snake_length() == 3, "the snake starts three cells long");
    expect(snake_score() == 0, "score starts at zero");
    expect(status_get_digit(0) == 0 && status_get_digit(1) == 0 &&
           status_get_digit(2) == 0 && status_get_digit(3) == 2,
           "the left four digits show starting speed 2 FPS");
    expect(status_get_digit(4) == 0 && status_get_digit(7) == 0,
           "the right four digits show zero score");
    expect(snake_food_count() == 4, "four food items are live");
    expect(snake_speed_fps() == 2, "the snake starts at 2 FPS");
}

static void test_eat_increments_score(void)
{
    int hx;
    int hy;

    input_reset();
    reset_play(1);
    hx = snake_head_x();
    hy = snake_head_y();
    snake_place_food(0, hx + 1, hy);
    idle(STEP_FRAMES + 5);
    expect(snake_score() == 1, "eating food adds to the score");
    expect(snake_speed_fps() == 2, "score food does not change speed");
    expect(status_get_digit(7) == 1 && status_get_digit(6) == 0,
           "the score uses the right four digits");
}

static void test_speed_food(void)
{
    int hx;
    int hy;

    input_reset();
    reset_play(1);
    hx = snake_head_x();
    hy = snake_head_y();
    snake_place_speed(hx + 1, hy);
    idle(STEP_FRAMES + 5);
    expect(snake_speed_fps() == 3, "a speed pickup raises movement by 1 FPS");
    expect(snake_score() == 1, "speed food adds to the score");
    expect(status_get_digit(3) == 3 && status_get_digit(7) == 1,
           "speed FPS and score update on the status row");
    expect(snake_length() == 3, "speed food does not lengthen the snake");
}

static void test_turn_applies_on_next_step(void)
{
    int hx;
    int hy;

    input_reset();
    reset_play(1);
    hx = snake_head_x();
    hy = snake_head_y();
    idle(STEP_FRAMES - 2);
    feed_pressed(1, 0, 0, 0);
    expect(snake_head_x() == hx && snake_head_y() == hy - 1,
           "a debounced turn lands on the next movement step");
}

static void test_turn_queue_u(void)
{
    int hx;
    int hy;

    input_reset();
    reset_play(1);
    hx = snake_head_x();
    hy = snake_head_y();
    feed_pressed(1, 0, 0, 0);
    feed_pressed(0, 0, 1, 0);
    expect(snake_turn_queue_depth() == 1, "the second turn waits in the queue");
    idle(STEP_FRAMES);
    expect(snake_head_x() == hx && snake_head_y() == hy - 1,
           "the first step uses the retargeted heading");
    idle(STEP_FRAMES);
    expect(snake_head_x() == hx - 1 && snake_head_y() == hy - 1,
           "the queued turn completes the U");
}

static void test_turn_queue_cap(void)
{
    input_reset();
    reset_play(1);
    feed_pressed(1, 0, 0, 0);
    feed_pressed(0, 0, 1, 0);
    feed_pressed(0, 1, 0, 0);
    expect(snake_turn_queue_depth() == 2, "two chained turns fill the queue");
    feed_pressed(1, 0, 0, 0);
    expect(snake_turn_queue_depth() == 2,
           "a third queued turn is ignored when the queue is full");
}

static void test_turn_opposite_rejected(void)
{
    int pdx;
    int pdy;

    input_reset();
    reset_play(1);
    feed_pressed(0, 0, 1, 0);
    snake_pending_dir(&pdx, &pdy);
    expect(pdx == 1 && pdy == 0,
           "a reverse turn does not change the next heading");
}

static void test_single_turn_after_move(void)
{
    int hx;
    int hy;
    int tdx;
    int tdy;

    input_reset();
    reset_play(1);
    idle(STEP_FRAMES);
    hx = snake_head_x();
    hy = snake_head_y();
    snake_travel_dir(&tdx, &tdy);
    expect(tdx == 1 && tdy == 0, "the snake starts moving right");
    feed_pressed(1, 0, 0, 0);
    snake_pending_dir(&tdx, &tdy);
    expect(tdx == 0 && tdy == -1, "the next heading updates before the queue");
    idle(STEP_FRAMES);
    expect(snake_head_x() == hx && snake_head_y() == hy - 1,
           "the following step uses that heading");
}

static void test_wall_death(void)
{
    int i;

    input_reset();
    reset_play(99);
    for (i = 0; i < 500; i++) {
        feed(0, 0, 0, 0, 0);
        if (snake_death_blinking())
            break;
    }
    expect(snake_death_blinking(), "the snake blinks after hitting the wall");
}

static void test_death_resets(void)
{
    int i;

    input_reset();
    reset_play(99);
    for (i = 0; i < 500; i++) {
        feed(0, 0, 0, 0, 0);
        if (snake_death_blinking())
            break;
    }
    idle(120);
    expect(!snake_death_blinking(), "death blink ends and play resumes");
    expect(snake_length() == 3, "a new round restores length three");
    expect(snake_score() == 0, "a new round clears the score");
    expect(snake_speed_fps() == 2, "a new round restores 2 FPS movement");
}

static void test_button_slows_speed(void)
{
    input_reset();
    reset_play(1);
    snake_place_speed(snake_head_x() + 1, snake_head_y());
    idle(STEP_FRAMES + 5);
    expect(snake_speed_fps() == 3, "setup: speed pickup raised FPS");
    feed_button();
    expect(snake_speed_fps() == 2, "button drops speed by 1 FPS");
    expect(status_get_digit(3) == 2, "left digits track the new FPS");
    feed_button();
    expect(snake_speed_fps() == 2, "button does not go below 2 FPS");
}

static void test_growth_after_food(void)
{
    int hx;
    int hy;

    input_reset();
    reset_play(1);
    hx = snake_head_x();
    hy = snake_head_y();
    snake_place_food(0, hx + 1, hy);
    idle(STEP_FRAMES + 5);
    expect(snake_length() == 4, "score food lengthens the snake");
}

int main(void)
{
    report_begin("snake");
    test_title();
    test_start();
    test_food_not_on_body();
    test_eat_increments_score();
    test_speed_food();
    test_button_slows_speed();
    test_turn_applies_on_next_step();
    test_single_turn_after_move();
    test_turn_queue_u();
    test_turn_queue_cap();
    test_turn_opposite_rejected();
    test_growth_after_food();
    test_wall_death();
    test_death_resets();
    return report_end();
}
