#include "pong.h"
#include "test_grids.h"

#include "frame.h"
#include "input.h"
#include "status.h"
#include "tests/report.h"

#include <stdio.h>

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

static int col_top(int x)
{
    int y;

    for (y = 0; y < FIELD; y++) {
        if (pixel_get((uint8_t)x, (uint8_t)y))
            return y;
    }
    return -1;
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

static void test_open(void)
{
    input_reset();
    pong_reset();
    expect(col_top(0) == 6 && col_top(15) == 6, "both paddles start centered");
    expect(status_get_digit(7) == 0 && status_get_digit(3) == 0,
           "scores start at zero on both sides");
}

static void test_p1_moves(void)
{
    input_reset();
    pong_reset();
    feed_p1(1, 0, 0);
    feed_p1(1, 0, 0);
    expect(col_top(0) == 5, "player one moves the left paddle");
    expect(col_top(15) == 6, "player two paddle stays put");
}

static void test_p2_moves(void)
{
    input_reset();
    pong_reset();
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
    feed_p1(1, 0, 0);
    feed_p1(1, 0, 0);
    expect(col_top(0) == 5, "play resumes after a paddle move");
}

int main(void)
{
    report_begin("pong");
    test_open();
    test_p1_moves();
    test_p2_moves();
    test_period();
    test_bounces();
    test_score_pause_resets();
    return report_end();
}
