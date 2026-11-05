#include "snake.h"

#include "frame.h"
#include "input.h"
#include "status.h"
#include "tests/report.h"

#define TICK_HZ 60
#define STEP_FRAMES (TICK_HZ / 2)

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
    snake_seed(42);
    snake_reset();
    expect(food_clear_of_body(), "spawned food stays off the snake map");
    for (i = 0; i < 5; i++)
        idle(STEP_FRAMES);
    expect(food_clear_of_body(), "respawned food stays off the snake map");
}

static void test_start(void)
{
    input_reset();
    snake_seed(42);
    snake_reset();

    expect(snake_length() == 3, "the snake starts three cells long");
    expect(snake_score() == 0, "score starts at zero");
    expect(status_get_digit(0) == 0 && status_get_digit(7) == 0,
           "all eight status digits show zero");
    expect(snake_food_count() == 4, "four food items are live");
    expect(snake_speed_fps() == 2, "the snake starts at 2 FPS");
}

static void test_eat_increments_score(void)
{
    int hx;
    int hy;

    input_reset();
    snake_seed(1);
    snake_reset();
    hx = snake_head_x();
    hy = snake_head_y();
    snake_place_food(0, hx + 1, hy);
    idle(STEP_FRAMES + 5);
    expect(snake_score() == 1, "eating food adds to the score");
    expect(snake_speed_fps() == 2, "score food does not change speed");
    expect(status_get_digit(7) == 1 && status_get_digit(6) == 0,
           "the score uses all eight digits");
}

static void test_speed_food(void)
{
    int hx;
    int hy;

    input_reset();
    snake_seed(1);
    snake_reset();
    hx = snake_head_x();
    hy = snake_head_y();
    snake_place_speed(hx + 1, hy);
    idle(STEP_FRAMES + 5);
    expect(snake_speed_fps() == 3, "a speed pickup raises movement by 1 FPS");
    expect(snake_score() == 0, "speed food does not add score");
    expect(snake_length() == 3, "speed food does not lengthen the snake");
}

static void test_turn_applies_on_next_step(void)
{
    int hx;
    int hy;

    input_reset();
    snake_seed(1);
    snake_reset();
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
    snake_seed(1);
    snake_reset();
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
    snake_seed(1);
    snake_reset();
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
    snake_seed(1);
    snake_reset();
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
    snake_seed(1);
    snake_reset();
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
    snake_seed(99);
    snake_reset();
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
    snake_seed(99);
    snake_reset();
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

static void test_growth_after_food(void)
{
    int hx;
    int hy;

    input_reset();
    snake_seed(1);
    snake_reset();
    hx = snake_head_x();
    hy = snake_head_y();
    snake_place_food(0, hx + 1, hy);
    idle(STEP_FRAMES + 5);
    expect(snake_length() == 4, "score food lengthens the snake");
}

int main(void)
{
    report_begin("snake");
    test_start();
    test_food_not_on_body();
    test_eat_increments_score();
    test_speed_food();
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
