#include "pong.h"
#include "test_grids.h"

#include "frame.h"
#include "input.h"
#include "status.h"
#include "tests/report.h"

#include <stdio.h>
#include <string.h>

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void feed_p1(int up, int down, int button)
{
    Input in;

    input_tick(up, down, 0, 0, button, &in);
    input_tick_p2(0, 0, 0, 0, 0, &in);
    pong_tick(&in);
}

static void feed_p2(int up, int down, int button)
{
    Input in;

    input_tick(0, 0, 0, 0, 0, &in);
    input_tick_p2(up, down, 0, 0, button, &in);
    pong_tick(&in);
}

static int col_top_from(int x, int y0)
{
    int y;

    for (y = y0; y < FIELD; y++) {
        if (pixel_get((uint8_t)x, (uint8_t)y))
            return y;
    }
    return -1;
}

static int col_top(int x)
{
    return col_top_from(x, 0);
}

static void step_idle(void)
{
    Input in;

    input_tick(0, 0, 0, 0, 0, &in);
    input_tick_p2(0, 0, 0, 0, 0, &in);
    pong_tick(&in);
}

static void capture(uint8_t out[FIELD][FIELD])
{
    int x;
    int y;

    for (y = 0; y < FIELD; y++) {
        for (x = 0; x < FIELD; x++)
            out[y][x] = pixel_get((uint8_t)x, (uint8_t)y) ? 1 : 0;
    }
}

static void print_grid(const uint8_t m[FIELD][FIELD])
{
    int x;
    int y;

    for (y = 0; y < FIELD; y++) {
        for (x = 0; x < FIELD; x++)
            fputc(m[y][x] ? '1' : '.', stderr);
        fputc('\n', stderr);
    }
}

static int grids_equal(const uint8_t got[FIELD][FIELD], const uint8_t want[FIELD][FIELD])
{
    int x;
    int y;

    for (y = 0; y < FIELD; y++) {
        for (x = 0; x < FIELD; x++) {
            if (got[y][x] != want[y][x])
                return 0;
        }
    }
    return 1;
}

static void setup_scene(const uint8_t m[FIELD][FIELD], int vx, int vy)
{
    int ly = -1;
    int ry = -1;
    int bx = -1;
    int by = -1;
    int x;
    int y;

    for (y = 0; y < FIELD; y++) {
        if (m[y][0] && ly < 0)
            ly = y;
        if (m[y][15] && ry < 0)
            ry = y;
        for (x = 1; x < 15; x++) {
            if (m[y][x]) {
                bx = x;
                by = y;
            }
        }
    }
    input_reset();
    pong_reset();
    pong_place_paddles(ly, ry);
    pong_place_ball(bx, by, vx, vy);
}

static void expect_grid(const uint8_t want[FIELD][FIELD], const char *msg)
{
    uint8_t got[FIELD][FIELD];
    int ok;

    capture(got);
    ok = grids_equal(got, want);
    if (!ok) {
        fputs("got:\n", stderr);
        print_grid(got);
        fputs("want:\n", stderr);
        print_grid(want);
    }
    expect(ok, msg);
}

static void bounce_case(const uint8_t before[FIELD][FIELD],
                        const uint8_t after[FIELD][FIELD],
                        int vx, int vy, const char *msg)
{
    setup_scene(before, vx, vy);
    step_idle();
    expect_grid(after, msg);
}

static void start_play(void)
{
    input_reset();
    pong_reset();
    pong_begin_play(0);
}

static void feed_buttons(int p1, int p2)
{
    Input in;

    input_tick(0, 0, 0, 0, p1, &in);
    input_tick_p2(0, 0, 0, 0, p2, &in);
    pong_tick(&in);
}

static void test_open(void)
{
    start_play();
    expect(col_top(0) == 6 && col_top(15) == 6, "both paddles start centered");
    expect(status_get_digit(7) == 0 && status_get_digit(3) == 0,
           "scores start at zero on both sides");
    expect(!pong_in_demo(), "begin play leaves the title screen");
}

static void test_p1_moves(void)
{
    start_play();
    feed_p1(1, 0, 0);
    feed_p1(1, 0, 0);
    expect(col_top(0) == 5, "player one moves the left paddle");
    expect(col_top(15) == 6, "player two paddle stays put");
}

static void test_p2_moves(void)
{
    start_play();
    feed_p2(0, 1, 0);
    feed_p2(0, 1, 0);
    expect(col_top(15) == 7, "player two moves the right paddle");
    expect(col_top(0) == 6, "player one paddle stays put");
}

static void test_period(void)
{
    expect(pong_period(0) == 7 && pong_period(599) == 7, "the ball opens at 7 ticks");
    expect(pong_period(3600) == 1, "the ball reaches one tick per step");
}

static int ball_x(void)
{
    int x;
    int y;

    for (y = 0; y < FIELD; y++) {
        for (x = 1; x < 15; x++) {
            if (pixel_get((uint8_t)x, (uint8_t)y))
                return x;
        }
    }
    return -1;
}

static void play_p1(int up, int down, int button)
{
    Input in;

    memset(&in, 0, sizeof in);
    in.up = up != 0;
    in.down = down != 0;
    in.button = button != 0;
    pong_play_tick(&in);
}

static void test_paddle_english(void)
{
    setup_scene(PAD_CENTER_BALL_L_FACE, -1, -1);
    play_p1(0, 0, 0);
    expect(ball_x() == 2 && pong_ball_y() == 6,
           "a still paddle leaves the face bounce unchanged");
    expect(col_top(0) == 6, "the paddle stays put without input");

    setup_scene(PAD_CENTER_BALL_L_FACE, -1, -1);
    play_p1(1, 0, 1);
    expect(ball_x() == 2 && pong_ball_y() == 5,
           "an upward paddle step nudges the ball one row on bounce");
    expect(col_top(0) == 5, "the paddle moved up with the hit");

    setup_scene(PAD_CENTER_BALL_L_FACE, -1, -1);
    play_p1(0, 1, 1);
    expect(ball_x() == 2 && pong_ball_y() == 7,
           "a downward paddle step nudges the ball down on bounce");

    input_reset();
    pong_reset();
    pong_place_paddles(6, 6);
    pong_place_ball_timed(1, 7, -1, 0, 60, 30);
    play_p1(1, 0, 1);
    expect(ball_x() == 2 && pong_ball_y() == 6,
           "slow hold english forces the bounce and shifts Y once");
}

static void test_bounces(void)
{
    bounce_case(PAD_CENTER_BALL_L_APPROACH, AFTER_L_APPROACH, -1, -1,
                "from column 2 the ball reaches column 1 without bouncing in air");
    bounce_case(PAD_CENTER_BALL_L_FACE, AFTER_L_FACE_UP, -1, -1,
                "a left face hit bounces diagonally, not a vertical slide");
    bounce_case(PAD_CENTER_BALL_L_FACE_DOWN, AFTER_L_FACE_DOWN, -1, 1,
                "a left face hit downward bounces diagonally");
    bounce_case(PAD_CENTER_BALL_L_TOP, AFTER_L_TOP_UP, -1, -1,
                "leaving the left paddle top going up is a face bounce");
    bounce_case(PAD_CENTER_BALL_L_BOT, AFTER_L_BOT_DOWN, -1, 1,
                "leaving the left paddle bottom going down is a face bounce");
    bounce_case(PAD_CENTER_BALL_L_ARRIVE_TOP, AFTER_L_ARRIVE_TOP, -1, 1,
                "down onto the left paddle top inverts both vx and vy");
    bounce_case(PAD_CENTER_BALL_L_ARRIVE_BOT, AFTER_L_ARRIVE_BOT, -1, -1,
                "up onto the left paddle bottom inverts both vx and vy");
    bounce_case(PAD_CENTER_BALL_L_MISS, AFTER_L_MISS, -1, -1,
                "column 0 off the paddle is a point and keeps the ball there");
    bounce_case(PAD_CENTER_BALL_R_APPROACH, AFTER_R_APPROACH, 1, -1,
                "from column 13 the ball reaches column 14 without bouncing in air");
    bounce_case(PAD_CENTER_BALL_R_FACE, AFTER_R_FACE_UP, 1, -1,
                "a right face hit bounces diagonally, not a vertical slide");
    bounce_case(PAD_CENTER_BALL_R_TOP, AFTER_R_TOP_UP, 1, -1,
                "leaving the right paddle top going up is a face bounce");
    bounce_case(PAD_CENTER_BALL_R_BOT, AFTER_R_BOT_DOWN, 1, 1,
                "leaving the right paddle bottom going down is a face bounce");
    bounce_case(PAD_CENTER_BALL_R_MISS, AFTER_R_MISS, 1, -1,
                "column 15 off the paddle is a point and keeps the ball there");
    bounce_case(PAD_CENTER_BALL_WALL, AFTER_TOP_WALL, 1, -1,
                "the top wall reflects vy and still steps x");
}

static void test_score_pause_resets(void)
{
    int i;

    bounce_case(PAD_CENTER_BALL_L_MISS, AFTER_L_MISS, -1, -1,
                "a miss still shows the ball on the goal for the pause");
    expect(status_get_digit(7) == 1, "player two scores the miss");
    for (i = 0; i < 60; i++)
        step_idle();
    expect(col_top(0) == 6 && col_top(15) == 6, "paddles return to center after the pause");
    expect(pixel_get(1, 7) != 0, "the conceded player holds the ball");
    feed_p2(0, 1, 0);
    feed_p2(0, 1, 0);
    expect(pixel_get(1, 7) != 0, "the other paddle cannot launch the serve");
    expect(col_top(15) == 7, "player two can still line up while waiting");
    feed_p1(1, 0, 0);
    feed_p1(1, 0, 0);
    expect(col_top(0) == 5, "play resumes after the holder moves");
    expect(pixel_get(1, 7) == 0, "the holder's move sends the ball");
}

#define SEG_A 0x40
#define SEG_B 0x20
#define SEG_C 0x10
#define SEG_D 0x08
#define SEG_E 0x04
#define SEG_F 0x02
#define SEG_G 0x01
#define DIGIT_P (SEG_A | SEG_B | SEG_E | SEG_F | SEG_G)
#define DIGIT_O (SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F)
#define DIGIT_N (SEG_C | SEG_E | SEG_G)
#define DIGIT_G6 (SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G)

static int digits_pong_at(int off)
{
    int i;

    for (i = 0; i < 8; i++) {
        uint8_t want = 0;

        if (i == off)
            want = DIGIT_P;
        else if (i == off + 1)
            want = DIGIT_O;
        else if (i == off + 2)
            want = DIGIT_N;
        else if (i == off + 3)
            want = DIGIT_G6;
        if (status_get_digit((uint8_t)i) != want)
            return 0;
    }
    return 1;
}

static void test_title_digits(void)
{
    int i;

    input_reset();
    pong_reset();
    expect(pong_in_demo(), "reset opens on the title screen");
    expect(status_decode() == 0, "title digits are raw segments");
    expect(digits_pong_at(0), "PONG starts on the left four digits");
    for (i = 0; i < 60; i++)
        step_idle();
    expect(digits_pong_at(0), "PONG pauses before it scrolls");
    for (i = 0; i < 15; i++)
        step_idle();
    expect(digits_pong_at(1), "PONG then steps right at 4 FPS");
}

static void test_title_button_serves(void)
{
    int py;

    input_reset();
    pong_reset();
    feed_buttons(1, 0);
    feed_buttons(1, 0);
    py = col_top(0);
    expect(!pong_in_demo(), "player one button leaves the title");
    expect(status_decode() == 0xFF, "play restores Code-B digits");
    expect(pixel_get(1, (uint8_t)(py + 1)) != 0, "the ball sits on player one's paddle");

    input_reset();
    pong_reset();
    feed_buttons(0, 1);
    feed_buttons(0, 1);
    py = col_top(15);
    expect(!pong_in_demo(), "player two button leaves the title");
    expect(pixel_get(14, (uint8_t)(py + 1)) != 0, "the ball sits on player two's paddle");
}

static void test_title_serve(void)
{
    int i;

    pong_seed(1);
    input_reset();
    pong_reset();
    expect(pong_ball_y() == 7 && pong_pad_y(1) == 6,
           "demo opens with the ball on player one");
    for (i = 0; i < 28; i++)
        step_idle();
    expect(pong_pad_y(1) == 6 && pong_ball_y() == 7,
           "the demo serve waits half a second");
    for (i = 0; i < 8; i++)
        step_idle();
    expect(pong_pad_y(1) != 6, "player one then moves and launches");
}

static void test_title_npcs_play(void)
{
    int i;
    int moved = 0;

    pong_seed(1);
    input_reset();
    pong_reset();
    for (i = 0; i < 180; i++)
        step_idle();
    if (pong_pad_y(1) != 6 || pong_pad_y(2) != 6)
        moved = 1;
    expect(pong_in_demo(), "npcs keep the title screen running");
    expect(moved, "npc paddles move through the real game");
}

int main(void)
{
    report_begin("pong");
    test_open();
    test_p1_moves();
    test_p2_moves();
    test_period();
    test_paddle_english();
    test_bounces();
    test_score_pause_resets();
    test_title_digits();
    test_title_button_serves();
    test_title_serve();
    test_title_npcs_play();
    return report_end();
}
