#include "entity/rock.h"
#include "frame.h"
#include "game.h"
#include "input.h"
#include "over.h"
#include "score.h"
#include "status.h"
#include "tests/report.h"
#include "tests/sound_log.h"
#include "text.h"

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void feed(int up, int down, int left, int right, int button)
{
    Input in;

    input_tick(up, down, left, right, button, &in);
    game_tick(&in);
}

static int ship_x(void)
{
    int x;

    for (x = 0; x < 16; x++) {
        if (pixel_get((uint8_t)x, 14))
            return x;
    }
    return -1;
}

static void test_frame(void)
{
    clear();
    pixel(0, 0, true);
    expect(pixel_get(0, 0), "bit 0 is the left pixel");
    expect((frame[0] & 1u) != 0, "the top row stores that bit");
    pixel(0, 0, false);
    expect(!pixel_get(0, 0), "a pixel can turn off");
    pixel(16, 0, true);
    expect(frame[0] == 0, "a column past 15 is ignored");
}

static void test_ship(void)
{
    int i;

    input_reset();
    game_reset();
    expect(pixel_get(7, 13) && !pixel_get(6, 13) && !pixel_get(8, 13),
           "the nose is the center LED of the top row");
    expect(pixel_get(6, 14) && pixel_get(7, 14) && pixel_get(8, 14),
           "the middle row is full");
    expect(pixel_get(6, 15) && pixel_get(7, 15) && pixel_get(8, 15),
           "the bottom row is full");
    expect(!pixel_get(15, 15), "the corner past the ship stays dark");
    expect(ship_x() == 6, "the ship starts near the middle of the bottom");

    feed(0, 0, 0, 1, 0);
    expect(ship_x() == 6, "one tick does not pass debounce");
    feed(0, 0, 0, 1, 0);
    expect(ship_x() == 7, "the next tick steps one LED");
    feed(0, 0, 0, 1, 0);
    expect(ship_x() == 7, "the ship waits one tick between steps");
    feed(0, 0, 0, 1, 0);
    expect(ship_x() == 8, "the following tick steps one LED");

    input_reset();
    game_reset();
    feed(1, 0, 0, 1, 0);
    feed(1, 0, 0, 1, 0);
    expect(pixel_get(8, 12) && pixel_get(7, 13) && !pixel_get(8, 15),
           "a diagonal steps both axes");

    input_reset();
    game_reset();
    feed(1, 0, 0, 0, 0);
    feed(1, 0, 0, 0, 0);
    expect(pixel_get(7, 12) && !pixel_get(6, 15), "the ship can leave the bottom row");
    feed(0, 1, 0, 0, 0);
    feed(0, 1, 0, 0, 0);
    expect(pixel_get(7, 13) && pixel_get(6, 15), "the ship can return to the bottom row");

    input_reset();
    game_reset();
    feed(0, 0, 1, 1, 0);
    feed(0, 0, 1, 1, 0);
    expect(ship_x() == 6, "opposite switches cancel that axis");

    input_reset();
    game_reset();
    for (i = 0; i < 12; i++)
        feed(0, 0, 1, 0, 0);
    expect(ship_x() == 0, "the ship stops at the left edge");
}

static void test_shot(void)
{
    int i;

    input_reset();
    game_reset();
    sound_log_reset();
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(sound_log_count() == 1 && sound_log_at(0) == 1,
           "a spawned bullet sends the laser");
    expect(pixel_get(7, 12), "the bullet leaves the nose");
    expect(!pixel_get(7, 11), "the bullet is one LED on the fire tick");
    feed(0, 0, 0, 0, 0);
    expect(pixel_get(7, 12) && !pixel_get(7, 11), "the bullet waits one tick");
    feed(0, 0, 0, 0, 0);
    expect(pixel_get(7, 11) && !pixel_get(7, 12), "the second tick climbs one LED");

    input_reset();
    game_reset();
    sound_log_reset();
    rock_add(7, 8, 2, 16, 0, 30);
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(pixel_get(7, 12) && pixel_get(7, 8), "the shot and the rock are both on screen");
    for (i = 0; i < 6; i++)
        feed(0, 0, 0, 0, 0);
    expect(!pixel_get(7, 8) && !pixel_get(8, 9) && !pixel_get(7, 9),
           "the rock is gone after the hit");
    expect(pixel_get(7, 13), "the ship is still there after a shot");
    expect(score_get() == 1, "the score shows one point");
    expect(sound_log_count() == 2 && sound_log_at(0) == 1 && sound_log_at(1) == 2,
           "the shot sends the laser and the hit sends the explosion");
}

static void advance(int n)
{
    int i;

    for (i = 0; i < n; i++)
        feed(0, 0, 0, 0, 0);
}

static void test_speeds(void)
{
    input_reset();
    game_reset();
    rock_add(0, 0, 2, 4, 0, 30);
    rock_add(3, 0, 2, 8, 0, 30);
    rock_add(6, 0, 2, 12, 0, 30);
    rock_add(9, 0, 2, 16, 0, 30);
    advance(4);
    expect(!pixel_get(0, 0) && pixel_get(0, 1), "one rock steps every 4 ticks");
    expect(pixel_get(3, 0) && pixel_get(6, 0) && pixel_get(9, 0),
           "the slower rocks stay put");
    advance(4);
    expect(!pixel_get(3, 0) && pixel_get(3, 1), "one rock steps every 8 ticks");
    expect(pixel_get(6, 0) && pixel_get(9, 0), "the 12 and 16 rocks stay put");
    advance(4);
    expect(!pixel_get(6, 0) && pixel_get(6, 1), "one rock steps every 12 ticks");
    expect(pixel_get(9, 0), "the 16 rock stays put");
    advance(4);
    expect(!pixel_get(9, 0) && pixel_get(9, 1), "one rock steps every 16 ticks");

    input_reset();
    game_reset();
    rock_add(10, 4, 2, 16, 1, 15);
    advance(14);
    expect(pixel_get(10, 4) && pixel_get(11, 4) && !pixel_get(12, 4),
           "sideways drift waits out its period");
    advance(1);
    expect(pixel_get(11, 4) && pixel_get(12, 4) && !pixel_get(10, 4),
           "a rock steps one column every 15 ticks");
}

static void test_both_sizes(void)
{
    input_reset();
    game_reset();
    rock_add(1, 3, 2, 16, 0, 30);
    rock_add(1, -1, 3, 16, 0, 30);
    feed(0, 0, 0, 0, 0);
    expect(pixel_get(1, 3) && pixel_get(2, 3) && pixel_get(1, 4),
           "a spawned 2x2 rock is on screen");
    expect(!pixel_get(0, 3) && !pixel_get(3, 3), "that rock is two columns wide");
    expect(pixel_get(1, 0) && pixel_get(3, 0) && pixel_get(1, 1),
           "a spawned 3x3 rock reaches the top row");
    expect(!pixel_get(0, 0) && !pixel_get(4, 0) && !pixel_get(1, 2),
           "that rock is three columns wide and still crosses into the margin");
}

static void die(void)
{
    rock_add(6, 13, 2, 16, 0, 30);
    feed(0, 0, 0, 0, 0);
}

static int smile(void)
{
    return pixel_get(5, 4) && pixel_get(10, 4) && pixel_get(6, 5) && !pixel_get(6, 4);
}

static int frown(void)
{
    return pixel_get(7, 4) && pixel_get(8, 4) && pixel_get(6, 5) && !pixel_get(5, 4);
}

static void saved_high(int n)
{
    score_clear_saved();
    score_add(n);
    score_judge();
    score_reset();
}

static void test_crash(void)
{
    int i;

    input_reset();
    score_clear_saved();
    game_reset();
    die();
    expect(frown(), "zero player against one rock score is a loss");
    expect(pixel_get(4, 0) && !pixel_get(1, 7), "the face is on screen");
    expect(pixel_get(9, 10) || pixel_get(10, 10),
           "a loss shows the better luck message on the matrix");
    expect(!pixel_get(1, 8) && !pixel_get(2, 8),
           "the round score stays off the matrix");

    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(frown(), "the button cannot skip the game over hold");
    for (i = 0; i < 70; i++)
        feed(0, 0, 0, 0, 0);
    expect(frown(), "the face stays for 1.2 seconds");
    for (i = 0; i < 500; i++)
        feed(0, 0, 0, 0, 0);
    expect(frown(), "the end picture waits for the button");
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(pixel_get(7, 13) && !pixel_get(6, 13) && !frown(),
           "the button starts the next round");

    input_reset();
    saved_high(10);
    game_reset();
    score_add(2);
    die();
    expect(smile(), "beating the rocks without a record shows the smile");
    expect(!pixel_get(1, 8), "the matrix stays clear of the score");

    input_reset();
    saved_high(10);
    game_reset();
    score_add(1);
    die();
    expect(frown(), "a tied duel shows the frown");
    expect(!pixel_get(1, 8), "a tie still skips matrix score digits");

    input_reset();
    score_clear_saved();
    score_reset();
    score_add(3);
    over_start_shooter(3, 0);
    expect(smile(), "a new record shows the smile");
    expect(pixel_get(2, 7) && pixel_get(11, 9),
           "a new record bounce-scrolls new record on one line");

    input_reset();
    saved_high(10);
    game_reset();
    die();
    expect(frown(), "fewer player points than rocks draws the small frown");
}

static void test_glyphs(void)
{
    clear();
    text_draw(0, 0, "A", TEXT_CHAR_SPACING_DEFAULT);
    expect(pixel_get(1, 0) && !pixel_get(0, 0) && pixel_get(0, 2) && pixel_get(2, 2),
           "A is the 4x4 capital");
    clear();
    text_draw(0, 0, "a", TEXT_CHAR_SPACING_DEFAULT);
    expect(pixel_get(1, 0) && pixel_get(0, 2), "a lowercase letter uses the capital");
    clear();
    text_draw(0, 0, "M", TEXT_CHAR_SPACING_DEFAULT);
    expect(pixel_get(0, 0) && pixel_get(1, 1) && pixel_get(2, 2),
           "M is drawn in 4x4");
    clear();
    text_draw(0, 0, ".", TEXT_CHAR_SPACING_DEFAULT);
    expect(pixel_get(0, 2) && !pixel_get(0, 1) && !pixel_get(1, 2),
           "the dot sits on row 2");
    clear();
    text_draw(0, 0, "-", TEXT_CHAR_SPACING_DEFAULT);
    expect(pixel_get(0, 1) && pixel_get(1, 1) && pixel_get(2, 1) && !pixel_get(0, 0),
           "the dash is the middle row");
    clear();
    text_draw(0, 0, ":", TEXT_CHAR_SPACING_DEFAULT);
    expect(pixel_get(1, 0) && pixel_get(1, 2) && !pixel_get(1, 1),
           "the colon is two dots");
    clear();
    text_draw(0, 0, "Z", TEXT_CHAR_SPACING_DEFAULT);
    expect(pixel_get(0, 0) && pixel_get(1, 1) && pixel_get(2, 2),
           "Z is the last letter");
    clear();
    text_draw(1, 8, "10", TEXT_CHAR_SPACING_DEFAULT);
    expect(pixel_get(1, 8) && pixel_get(2, 8) && !pixel_get(3, 8),
           "the digit 1 keeps its narrow top");
    expect(!pixel_get(5, 8) && pixel_get(6, 8) && pixel_get(7, 8) && pixel_get(8, 8) &&
               !pixel_get(9, 8),
           "the next glyph starts after a one pixel gap");
}

static void test_score(void)
{
    int i;

    score_reset();
    for (i = 0; i < 9999; i++)
        score_add(1);
    expect(score_get() == 9999, "the score reaches 9999");
    score_add(1);
    expect(score_get() == 9999, "the score stops at 9999");

    input_reset();
    game_reset();
    for (i = 0; i < 99; i++)
        score_add(1);
    feed(0, 0, 0, 0, 0);
    expect(status_get_digit(0) == 0 && status_get_digit(1) == 0 &&
           status_get_digit(2) == 9 && status_get_digit(3) == 9,
           "the left four status digits show player 99");
    expect(status_get_digit(4) == 0 && status_get_digit(5) == 0 &&
           status_get_digit(6) == 0 && status_get_digit(7) == 0,
           "the right four status digits start at zero rocks");
    score_add(1);
    feed(0, 0, 0, 0, 0);
    expect(score_get() == 100, "the score can pass 99");
    expect(status_get_digit(1) == 1 && status_get_digit(2) == 0 && status_get_digit(3) == 0,
           "player score 100 uses the hundreds digit");
}

static void test_rock_line_score(void)
{
    int i;

    input_reset();
    game_reset();
    rock_add(0, 11, 2, 4, 0, 30);
    for (i = 0; i < 8 && rock_score_get() == 0; i++)
        feed(0, 0, 0, 0, 0);
    expect(rock_score_get() == 1, "a rock past row 13 scores for the rocks");
    expect(status_get_digit(7) == 1, "the rocks score shows in the right digit");
    expect(pixel_get(7, 13), "the ship survives a rock that only crossed the line");
}

static void test_ramp(void)
{
    int mid = ROCK_RAMP * 2 / 3;

    expect(rock_fall_period(4, 0) == 4 && rock_fall_period(16, 0) == 16,
           "a new round keeps the slow fall");
    expect(rock_fall_period(4, mid) == 2, "the fast rocks reach 30 FPS during the minute");
    expect(rock_fall_period(16, mid) > 2, "the slow rocks are still slower then");
    expect(rock_fall_period(4, ROCK_RAMP) == 1 && rock_fall_period(8, ROCK_RAMP) == 1 &&
           rock_fall_period(12, ROCK_RAMP) == 1 && rock_fall_period(16, ROCK_RAMP) == 1,
           "every rock falls once a tick by the end of one minute");
    expect(rock_fall_period(8, ROCK_RAMP + 50) == 1, "the pace stays at one tick");
}

int main(void)
{
    report_begin("space-shooter");
    test_frame();
    test_ship();
    test_shot();
    test_speeds();
    test_both_sizes();
    test_crash();
    test_score();
    test_rock_line_score();
    test_ramp();
    test_glyphs();
    return report_end();
}
