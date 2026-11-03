#include "arkaled.h"
#include "collide.h"
#include "frame.h"
#include "input.h"
#include "sound.h"
#include "tests/report.h"
#include "tests/sound_log.h"

#include <stddef.h>

struct Grid {
    const char **row;
    int w;
    int h;
};

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void feed(int up, int down, int left, int right, int button)
{
    Input in;

    input_tick(up, down, left, right, button, &in);
    input_tick_p2(0, 0, 0, 0, 0, &in);
    arkaled_tick(&in);
}

static void start_2p(void)
{
    input_reset();
    arkaled_reset();
    arkaled_begin_play(2);
}

static void feed_p2(int left, int right, int button)
{
    Input in;

    input_tick(0, 0, 0, 0, 0, &in);
    input_tick_p2(0, 0, left, right, button, &in);
    arkaled_tick(&in);
}

static void at(int x, int y, int vx, int vy, const char *msg)
{
    int ax;
    int ay;
    int dx;
    int dy;

    arkaled_ball(&ax, &ay, &dx, &dy);
    expect(ax == x && ay == y && dx == vx && dy == vy, msg);
}

static int pump(int moves)
{
    int i;
    int x;
    int y;
    int vx;
    int vy;
    int px;
    int py;
    int seen = 0;

    arkaled_ball(&px, &py, &vx, &vy);
    for (i = 0; i < moves * 40 && seen < moves; i++) {
        feed(0, 0, 0, 0, 0);
        arkaled_ball(&x, &y, &vx, &vy);
        if (x != px || y != py) {
            seen++;
            px = x;
            py = y;
        }
    }
    return seen;
}

static int row_count(int y)
{
    int x;
    int n = 0;

    for (x = 0; x < 16; x++) {
        if (pixel_get((uint8_t)x, (uint8_t)y))
            n++;
    }
    return n;
}

static int pad_x(void)
{
    return arkaled_pad_x(1);
}

static int ended(void)
{
    return pixel_get(4, 0) && pixel_get(11, 0);
}

static int fresh(void)
{
    int x;
    int y;
    int vx;
    int vy;

    arkaled_ball(&x, &y, &vx, &vy);
    return x == 4 && y == 14 && vx == 0 && vy == 0 && pad_x() == 2 && arkaled_count() == 48;
}

static int grid_solid(int x, int y, void *ctx)
{
    struct Grid *g = ctx;

    if (x < 0 || y < 0 || x >= g->w || y >= g->h)
        return 1;
    return g->row[y][x] == '#';
}

static int open_solid(int x, int y, void *ctx)
{
    (void)x;
    (void)y;
    (void)ctx;
    return 0;
}

static int checker_solid(int x, int y, void *ctx)
{
    (void)ctx;
    if (x < 0 || y < 0 || x >= 8 || y >= 8)
        return 1;
    return ((x + y) & 1) == 0;
}

static void test_step(void)
{
    static const char *both[] = {".#.", "#..", "..."};
    static const char *diag[] = {"...", "...", "..#"};
    static const char *tunnel[] = {"#.#", "#.#", "#.#"};
    static const char *hole[] = {".....", "##.##", "....."};
    static const char *cave[] = {
        "######",
        "#....#",
        "#.##.#",
        "#....#",
        "######",
    };
    struct Grid g;
    CollideHit hit[2];
    int x;
    int y;
    int dx;
    int dy;
    int n;
    int i;
    int bad;
    int seen_n;
    int seen[32];

    x = 1;
    y = 1;
    dx = 1;
    dy = 0;
    n = collide_step(&x, &y, &dx, &dy, open_solid, NULL, hit, 2);
    expect(n == 0 && x == 2 && y == 1 && dx == 1 && dy == 0, "an open cell takes the step");

    g.row = both;
    g.w = 3;
    g.h = 3;
    x = 1;
    y = 1;
    dx = -1;
    dy = -1;
    n = collide_step(&x, &y, &dx, &dy, grid_solid, &g, hit, 2);
    expect(n == 2 && hit[0].x == 0 && hit[0].y == 1 && hit[1].x == 1 && hit[1].y == 0,
           "two neighbors are both hits");
    expect(x == 2 && y == 2 && dx == 1 && dy == 1, "two neighbors reverse both steps");

    g.row = diag;
    x = 1;
    y = 1;
    dx = 1;
    dy = 1;
    n = collide_step(&x, &y, &dx, &dy, grid_solid, &g, hit, 2);
    expect(n == 1 && hit[0].x == 2 && hit[0].y == 2, "a diagonal cell is one hit");
    expect(x == 0 && y == 0 && dx == -1 && dy == -1, "a diagonal cell reverses both steps");

    g.row = tunnel;
    g.w = 3;
    g.h = 3;
    x = 1;
    y = 1;
    dx = 1;
    dy = -1;
    bad = 0;
    for (i = 0; i < 6; i++) {
        collide_step(&x, &y, &dx, &dy, grid_solid, &g, NULL, 0);
        if (x != 1 || grid_solid(x, y, &g))
            bad = 1;
    }
    expect(!bad && y == 1 && dx == 1 && dy == 1, "a one-cell tunnel keeps the ball off the walls");

    g.row = hole;
    g.w = 5;
    g.h = 3;
    x = 2;
    y = 2;
    dx = 0;
    dy = -1;
    collide_step(&x, &y, &dx, &dy, grid_solid, &g, NULL, 0);
    expect(x == 2 && y == 1 && dx == 0 && dy == -1, "a one-cell hole lets the ball through");
    collide_step(&x, &y, &dx, &dy, grid_solid, &g, NULL, 0);
    expect(x == 2 && y == 0 && dx == 0 && dy == -1, "the ball comes out on the far side of the hole");

    g.row = cave;
    g.w = 6;
    g.h = 5;
    x = 1;
    y = 1;
    dx = 1;
    dy = 1;
    bad = 0;
    seen_n = 0;
    for (i = 0; i < 60; i++) {
        int px = x;
        int py = y;
        int k;
        int ddx;
        int ddy;

        collide_step(&x, &y, &dx, &dy, grid_solid, &g, NULL, 0);
        ddx = x - px;
        ddy = y - py;
        if (ddx < 0)
            ddx = -ddx;
        if (ddy < 0)
            ddy = -ddy;
        if (grid_solid(x, y, &g) || ddx > 1 || ddy > 1)
            bad = 1;
        for (k = 0; k < seen_n; k++) {
            if (seen[k] == y * 6 + x)
                break;
        }
        if (k == seen_n && seen_n < 32)
            seen[seen_n++] = y * 6 + x;
    }
    expect(!bad && seen_n >= 4 && x == 4 && y == 2 && dx == 1 && dy == 1,
           "a closed cave keeps the ball in open cells");

    x = 1;
    y = 0;
    dx = 1;
    dy = 1;
    bad = 0;
    for (i = 0; i < 40; i++) {
        collide_step(&x, &y, &dx, &dy, checker_solid, NULL, NULL, 0);
        if (checker_solid(x, y, NULL))
            bad = 1;
    }
    expect(!bad && x == 1 && y == 0 && dx == 1 && dy == 1,
           "a checkerboard never lets the ball enter a cell");
}

static void test_period(void)
{
    expect(arkaled_period(0) == 15 && arkaled_period(119) == 15,
           "the opening pace waits 15 ticks");
    expect(arkaled_period(120) == 14, "two seconds trims one tick from the wait");
    expect(arkaled_period(240) == 13, "four seconds trims the wait to 13 ticks");
    expect(arkaled_period(13 * 120) == 2 && arkaled_period(14 * 120) == 2 &&
               arkaled_period(14 * 120 + 50) == 2,
           "the wait reaches two ticks and stays there");
}

static void test_open(void)
{
    int x;
    int y;
    int vx;
    int vy;

    start_2p();
    expect(arkaled_count() == 48, "the block starts with 48 pieces");
    {
        int n = 0;

        for (y = 0; y < 16; y++) {
            for (x = 0; x < 16; x++) {
                if (arkaled_special(x, y))
                    n++;
            }
        }
        expect(n == 5, "five bricks are special");
    }
    expect(row_count(0) == 0 && row_count(1) == 0, "two rows above the block stay empty");
    expect(row_count(2) == 12 && row_count(3) == 12 && row_count(4) == 12 && row_count(5) == 12,
           "rows 2 through 5 hold twelve LEDs each");
    expect(!pixel_get(0, 2) && !pixel_get(1, 2) && !pixel_get(14, 2) && !pixel_get(15, 2),
           "two columns on each side stay empty");
    expect(row_count(6) == 0 && row_count(13) == 0, "the playfield under the block starts empty");
    expect(pad_x() == 2 && arkaled_pad_x(2) == 10,
           "two paddles share the bottom row");
    expect(row_count(15) == 6, "each paddle is 3 LEDs");
    expect(row_count(14) == 1, "the ball is the only LED on its row");
    arkaled_ball(&x, &y, &vx, &vy);
    expect(x == 4 && y == 14 && vx == 0 && vy == 0, "the ball waits above the paddle");
    expect(!pixel_get(1, 8), "the score stays off the field");
    for (x = 0; x < 20; x++)
        feed(0, 0, 0, 0, 0);
    at(4, 14, 0, 0, "the ball waits until the paddle moves");
    expect(!ended(), "a waiting ball does not end the round");
    arkaled_set(0, 0, 1);
    arkaled_set(0, 2, 1);
    arkaled_set(1, 3, 1);
    arkaled_set(14, 4, 1);
    arkaled_set(15, 5, 1);
    arkaled_set(4, 1, 1);
    arkaled_set(2, 6, 1);
    feed(0, 0, 0, 0, 0);
    expect(arkaled_count() == 48 && row_count(0) == 0 && row_count(1) == 0,
           "pieces stay inside the 12 by 4 block");
}

static void serve(unsigned seed, int left)
{
    input_reset();
    arkaled_seed(seed);
    arkaled_reset();
    arkaled_begin_play(2);
    if (left) {
        feed(0, 0, 1, 0, 0);
        feed(0, 0, 1, 0, 0);
    } else {
        feed(0, 0, 0, 1, 0);
        feed(0, 0, 0, 1, 0);
    }
}

static void test_serve(void)
{
    int i;

    serve(8, 1);
    at(3, 13, -1, -1, "the first paddle step sends the ball up and left");
    expect(pad_x() == 1, "that step moves the paddle left");
    for (i = 0; i < 14; i++)
        feed(0, 0, 0, 0, 0);
    at(3, 13, -1, -1, "the ball waits 15 ticks between steps");
    feed(0, 0, 0, 0, 0);
    at(2, 12, -1, -1, "the next tick steps the ball");

    serve(1, 0);
    at(5, 13, 1, -1, "the other opening sends the ball up and right");
    expect(pad_x() == 3, "that step moves the paddle right");
}

static void test_move(void)
{
    start_2p();
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 2, "one tick does not pass debounce");
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 3, "the paddle steps right");
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 3, "the paddle waits after a step");
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 3, "the paddle waits a second tick");
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 3, "the paddle waits a third tick");
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 4, "the next tick steps right again");

    start_2p();
    feed(1, 0, 0, 0, 0);
    feed(1, 0, 0, 0, 0);
    expect(pad_x() == 2, "up and down do not move the paddle");
    at(4, 14, 0, 0, "up and down do not serve");

    start_2p();
    feed(0, 0, 1, 1, 0);
    feed(0, 0, 1, 1, 0);
    expect(pad_x() == 2 && !ended(), "opposite directions cancel");
    at(4, 14, 0, 0, "a cancelled step does not serve");

    input_reset();
    arkaled_wipe();
    arkaled_load(5, 4, 10, 0, 0);
    arkaled_place_pads(5, 8);
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 5, "player one stops at the midline");
    feed_p2(1, 0, 0);
    feed_p2(1, 0, 0);
    expect(arkaled_pad_x(2) == 8, "player two stops at the midline");
    input_reset();
    arkaled_place_pads(5, 13);
    feed_p2(0, 1, 0);
    feed_p2(0, 1, 0);
    expect(arkaled_pad_x(2) == 13, "player two stops on the right edge");
    input_reset();
    arkaled_load(0, 4, 10, 0, 0);
    arkaled_place_pads(0, 8);
    feed(0, 0, 1, 0, 0);
    feed(0, 0, 1, 0, 0);
    expect(pad_x() == 0, "player one stops on the left edge");
}

static void test_button(void)
{
    input_reset();
    arkaled_seed(1);
    arkaled_reset();
    arkaled_begin_play(2);
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(pad_x() == 2, "the button does not move the paddle");
    at(4, 14, 0, 0, "the button does not serve");

    start_2p();
    feed(0, 0, 0, 1, 1);
    expect(pad_x() == 2, "a held button still waits for debounce");
    feed(0, 0, 0, 1, 1);
    expect(pad_x() == 3, "the first button step is one cell");
    feed(0, 0, 0, 1, 1);
    expect(pad_x() == 4, "holding the button steps the paddle every tick");

    start_2p();
    feed_p2(1, 0, 1);
    feed_p2(1, 0, 1);
    expect(arkaled_pad_x(2) == 9, "player two's first button step is one cell");
    feed_p2(1, 0, 1);
    expect(arkaled_pad_x(2) == 8, "player two also steps every tick with the button");
}

static void test_bricks(void)
{
    int x;

    input_reset();
    arkaled_wipe();
    arkaled_set(2, 2, 1);
    arkaled_set(8, 5, 1);
    arkaled_load(6, 8, 6, 0, -1);
    feed(0, 0, 0, 0, 0);
    at(8, 7, 0, 1, "a piece overhead sends the ball back down");
    expect(!arkaled_on(8, 5) && arkaled_on(2, 2) && !ended(),
           "that piece turns off");

    arkaled_wipe();
    arkaled_set(2, 2, 1);
    arkaled_set(5, 4, 1);
    arkaled_set(6, 3, 1);
    arkaled_load(6, 6, 4, -1, -1);
    feed(0, 0, 0, 0, 0);
    at(7, 5, 1, 1, "a corner of two pieces reverses both steps");
    expect(!arkaled_on(5, 4) && !arkaled_on(6, 3) && arkaled_on(2, 2) && !ended(),
           "both pieces of that corner turn off");

    arkaled_wipe();
    arkaled_set(2, 2, 1);
    arkaled_set(8, 5, 1);
    arkaled_load(6, 7, 6, 1, -1);
    feed(0, 0, 0, 0, 0);
    at(6, 7, -1, 1, "a lone diagonal piece reverses both steps");
    expect(!arkaled_on(8, 5) && arkaled_on(2, 2), "the diagonal piece turns off");

    arkaled_wipe();
    for (x = 0; x < 16; x++)
        arkaled_set(x, 5, 1);
    arkaled_load(6, 8, 6, 1, -1);
    feed(0, 0, 0, 0, 0);
    at(9, 7, 1, 1, "a ceiling keeps the horizontal step");
    expect(!arkaled_on(8, 5) && arkaled_on(9, 5) && arkaled_on(10, 5),
           "the piece overhead turns off and the next one stays");

    arkaled_reset();
    arkaled_begin_play(2);
    arkaled_load(6, 4, 0, 1, -1);
    feed(0, 0, 0, 0, 0);
    at(5, 1, 1, 1, "the top edge sends the ball back down");
    expect(arkaled_count() == 48, "the top edge leaves the block in place");

    arkaled_load(6, 0, 8, -1, 1);
    feed(0, 0, 0, 0, 0);
    at(1, 9, 1, 1, "the left edge sends the ball back");

    arkaled_load(6, 15, 8, 1, -1);
    feed(0, 0, 0, 0, 0);
    at(14, 7, -1, -1, "the right edge sends the ball back");

    arkaled_wipe();
    for (x = 0; x < 16; x++)
        arkaled_set(x, 2, 1);
    arkaled_set(8, 2, 0);
    arkaled_load(6, 8, 4, 0, -1);
    expect(pump(5) == 5, "the ball reaches the gap above the block");
    at(8, 1, 0, 1, "a hole lets the ball into the two open rows");
    expect(!arkaled_on(8, 2) && arkaled_on(9, 2) && arkaled_on(7, 2) && arkaled_on(10, 2),
           "the hole stays open and the pieces beside it stay");

    serve(8, 1);
    expect(pump(8) == 8, "the left opening reaches the block");
    at(5, 7, 1, 1, "the gutter wall and the block turn the ball");
    expect(!arkaled_on(4, 5) && arkaled_on(5, 5) && arkaled_on(4, 4),
           "that piece turns off");
    expect(!ended(), "the rally continues");

    serve(1, 0);
    expect(pump(8) == 8, "the right opening reaches the block");
    at(13, 7, 1, 1, "the right side of the block sends the ball down");
    expect(!arkaled_on(12, 5) && arkaled_on(13, 5) && arkaled_on(10, 5),
           "that piece turns off");
}

static void test_paddle(void)
{
    start_2p();
    arkaled_load(6, 7, 14, 1, 1);
    feed(0, 0, 0, 0, 0);
    at(8, 13, 1, -1, "the body of the paddle keeps the horizontal step");
    expect(arkaled_score(1) == 1 && arkaled_score(2) == 0,
           "a paddle bounce scores for that player");
    expect(arkaled_count() == 48 && pad_x() == 6 && !ended(),
           "the paddle leaves the block in place");

    arkaled_load(6, 8, 14, 1, 1);
    feed(0, 0, 0, 0, 0);
    at(9, 13, 1, -1, "the LED at the end of the paddle keeps the horizontal step");

    arkaled_load(6, 9, 14, -1, 1);
    feed(0, 0, 0, 0, 0);
    at(10, 13, 1, -1, "meeting the end from outside reverses both steps");
    expect(arkaled_count() == 48 && !ended(), "that corner leaves the block in place");

    arkaled_load(6, 8, 12, 0, -1);
    feed(0, 0, 0, 0, 0);
    at(8, 11, 0, -1, "a ball moving away from the paddle does not bounce");
}

static void test_carry(void)
{
    int i;

    input_reset();
    arkaled_wipe();
    arkaled_load(2, 4, 14, 0, 0);
    feed(0, 0, 0, 0, 0);
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    at(5, 14, 0, 0, "a moving paddle carries a ball on contact");
    expect(pad_x() == 3, "that step moves the paddle under the ball");

    input_reset();
    arkaled_wipe();
    arkaled_load(2, 4, 14, 0, 0);
    feed(0, 0, 0, 0, 0);
    feed(0, 0, 1, 0, 0);
    feed(0, 0, 1, 0, 0);
    at(3, 14, 0, 0, "the paddle carries the ball to the left");
    expect(pad_x() == 1, "that step moves the paddle left under the ball");

    input_reset();
    arkaled_wipe();
    arkaled_load(2, 5, 14, 0, 0);
    feed(0, 0, 0, 0, 0);
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    at(5, 14, 0, 0, "a ball beside the paddle stays in its column");

    input_reset();
    arkaled_wipe();
    arkaled_load(2, 4, 12, 0, 0);
    feed(0, 0, 0, 0, 0);
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    at(4, 12, 0, 0, "a ball above the paddle stays in its column");

    input_reset();
    arkaled_wipe();
    arkaled_load(2, 4, 13, 0, 1);
    feed(0, 0, 0, 0, 0);
    at(4, 14, 0, 1, "the ball arrives on the paddle");
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    at(5, 14, 0, 1, "the paddle carries the ball before the bounce");
    for (i = 0; i < 13; i++)
        feed(0, 0, 0, 0, 0);
    at(5, 13, 0, -1, "the bounce leaves from the carried column");
}

static void test_miss(void)
{
    input_reset();
    arkaled_wipe();
    arkaled_load(6, 0, 14, 0, 1);
    feed(0, 0, 0, 0, 0);
    at(0, 15, 0, 0, "a miss leaves the ball on the bottom row");
    expect(pixel_get(0, 15) && !fresh(), "the dying ball starts lit");
    {
        int i;
        int dark = 0;

        for (i = 0; i < 8; i++)
            feed(0, 0, 0, 0, 0);
        expect(!pixel_get(0, 15), "the dying ball blinks off");
        for (i = 0; i < 52; i++) {
            feed(0, 0, 0, 0, 0);
            if (!pixel_get(0, 15) && !fresh())
                dark = 1;
        }
        expect(dark && fresh() && !ended(), "a miss starts a new round after the blink");
    }
    expect(!pixel_get(7, 13), "the ship is not part of this cart");
}

static void test_pop(void)
{
    sound_log_reset();
    input_reset();
    arkaled_wipe();
    arkaled_load(6, 8, 10, 1, 0);
    feed(0, 0, 0, 0, 0);
    expect(sound_log_count() == 0, "an open step stays quiet");

    sound_log_reset();
    arkaled_set(2, 2, 1);
    arkaled_set(8, 5, 1);
    arkaled_load(6, 8, 6, 0, -1);
    feed(0, 0, 0, 0, 0);
    expect(sound_log_count() == 1 && sound_log_at(0) == SOUND_POP,
           "a piece sends the pop");

    sound_log_reset();
    arkaled_wipe();
    arkaled_set(2, 2, 1);
    arkaled_set(5, 4, 1);
    arkaled_set(6, 3, 1);
    arkaled_load(6, 6, 4, -1, -1);
    feed(0, 0, 0, 0, 0);
    expect(sound_log_count() == 1 && sound_log_at(0) == SOUND_POP,
           "two pieces in one step send one pop");

    sound_log_reset();
    arkaled_load(6, 0, 8, -1, 0);
    feed(0, 0, 0, 0, 0);
    expect(sound_log_count() == 1 && sound_log_at(0) == SOUND_POP,
           "the left edge sends the pop");

    sound_log_reset();
    arkaled_load(6, 8, 14, 0, 1);
    feed(0, 0, 0, 0, 0);
    expect(sound_log_count() == 1 && sound_log_at(0) == SOUND_POP, "the paddle sends the pop");

    sound_log_reset();
    arkaled_wipe();
    arkaled_load(6, 0, 14, 0, 1);
    feed(0, 0, 0, 0, 0);
    {
        int i;

        for (i = 0; i < 60; i++)
            feed(0, 0, 0, 0, 0);
    }
    expect(fresh() && sound_log_count() == 0, "a miss stays quiet");

    sound_log_reset();
    arkaled_wipe();
    arkaled_set(8, 5, 1);
    arkaled_load(6, 8, 6, 0, -1);
    feed(0, 0, 0, 0, 0);
    expect(fresh() && sound_log_count() == 1 && sound_log_at(0) == SOUND_POP,
           "the last piece still sends the pop");
}

static void test_clear(void)
{
    input_reset();
    arkaled_wipe();
    arkaled_set(8, 5, 1);
    arkaled_load(6, 8, 6, 0, -1);
    feed(0, 0, 0, 0, 0);
    expect(fresh() && !ended(), "the last piece starts a new round");
}

static void test_cave(void)
{
    int i;
    int x;
    int y;
    int vx;
    int vy;
    int px;
    int py;
    int moves = 0;
    int clean = 1;
    int left = -1;
    int restarted = 0;

    input_reset();
    arkaled_wipe();
    for (x = 4; x <= 10; x++) {
        arkaled_set(x, 2, 1);
        arkaled_set(x, 5, 1);
    }
    arkaled_set(4, 3, 1);
    arkaled_set(4, 4, 1);
    arkaled_set(10, 3, 1);
    arkaled_set(10, 4, 1);
    arkaled_load(6, 6, 4, 1, -1);
    for (i = 0; i < 500 && !restarted; i++) {
        int ddx;
        int ddy;
        int before = arkaled_count();

        arkaled_ball(&px, &py, &vx, &vy);
        feed(0, 0, 0, 0, 0);
        arkaled_ball(&x, &y, &vx, &vy);
        if (before < 48 && fresh()) {
            left = before;
            restarted = 1;
            break;
        }
        ddx = x - px;
        ddy = y - py;
        if (ddx < 0)
            ddx = -ddx;
        if (ddy < 0)
            ddy = -ddy;
        if (arkaled_on(x, y) || x < 0 || x >= 16 || y < 0 || y >= 16 || ddx > 1 || ddy > 1)
            clean = 0;
        if (x != px || y != py)
            moves++;
    }
    expect(clean && moves >= 8 && restarted && left == 11,
           "the ball crosses the cave and then the miss starts over");
}

static void test_two_player(void)
{
    start_2p();
    feed_p2(1, 0, 0);
    feed_p2(1, 0, 0);
    expect(arkaled_pad_x(2) == 9, "player two moves the right paddle");
    expect(pad_x() == 2, "player one stays put");
    at(4, 14, 0, 0, "player two does not serve player one's ball");

    start_2p();
    arkaled_wipe();
    arkaled_place_pads(6, 6);
    expect(pixel_get(6, 15) && pixel_get(8, 15), "overlapping paddles start lit");
    {
        int i;

        for (i = 0; i < 8; i++)
            feed(0, 0, 0, 0, 0);
    }
    expect(!pixel_get(6, 15) && !pixel_get(8, 15),
           "the overlap blinks off four times a second");
}

static void test_special_drop(void)
{
    int i;
    int x;
    int y;
    int ix;
    int iy;

    input_reset();
    arkaled_wipe();
    arkaled_set(2, 2, 1);
    arkaled_set(8, 5, 1);
    arkaled_mark_special(8, 5);
    expect(arkaled_special(8, 5), "the marked piece is special");
    arkaled_load(6, 8, 6, 0, -1);
    feed(0, 0, 0, 0, 0);
    expect(!arkaled_on(8, 5) && arkaled_item_at(&ix, &iy) && ix == 8 && iy == 5,
           "a special piece drops an item");
    {
        int dark = 0;

        for (i = 0; i < 28; i++) {
            feed(0, 0, 0, 0, 0);
            if (arkaled_item_at(&ix, &iy) && !pixel_get((uint8_t)ix, (uint8_t)iy))
                dark = 1;
        }
        expect(arkaled_item_at(&ix, &iy) && iy == 5 && dark,
               "the falling item blinks four times before it drops");
    }
    feed(0, 0, 0, 0, 0);
    expect(arkaled_item_at(&ix, &iy) && iy == 6, "the item falls one row at two FPS");
    for (i = 0; i < 800 && arkaled_item_at(&ix, &iy) && iy < 15; i++)
        feed(0, 0, 0, 0, 0);
    for (i = 0; i < 16 && arkaled_item_at(&ix, &iy); i++)
        feed(0, 0, 0, 0, 0);
    expect(!arkaled_item_at(&ix, &iy) && arkaled_ball_count() == 2 &&
               arkaled_held_ball(&x, &y) == 1 && y == 14,
           "catching the item seats a second ball");
    feed(0, 0, 1, 0, 0);
    feed(0, 0, 1, 0, 0);
    expect(arkaled_ball_count() == 2 && arkaled_held_ball(&x, &y) == 0,
           "the extra ball launches when the paddle moves");
    expect(arkaled_period(0) == 15, "the extra ball opens at the starting pace");
}

static void test_item_prefers_p1(void)
{
    int i;
    int ix;
    int iy;

    input_reset();
    arkaled_wipe();
    arkaled_set(2, 2, 1);
    arkaled_set(8, 5, 1);
    arkaled_mark_special(8, 5);
    arkaled_load(6, 8, 6, 0, -1);
    arkaled_place_pads(6, 6);
    feed(0, 0, 0, 0, 0);
    for (i = 0; i < 800 && arkaled_item_at(&ix, &iy) && iy < 15; i++)
        feed(0, 0, 0, 0, 0);
    for (i = 0; i < 16 && arkaled_item_at(&ix, &iy); i++)
        feed(0, 0, 0, 0, 0);
    expect(arkaled_ball_count() == 2, "overlapping paddles give the extra ball to player one");
}

static void test_title(void)
{
    input_reset();
    arkaled_reset();
    expect(arkaled_on_title(), "reset opens the 1P 2P title");
    expect(pixel_get(1, 2) && pixel_get(0, 3), "the title banner starts with A");
    expect(!pixel_get(0, 1) && !pixel_get(0, 6),
           "the banner stays in the 16 by 4 window");
    expect(arkaled_title_scroll_x() == 0, "the banner starts at the left");
    {
        int i;

        for (i = 0; i < 80; i++)
            feed(0, 0, 0, 0, 0);
    }
    expect(arkaled_on_title() && arkaled_title_scroll_x() < 0,
           "the banner bounces left");
    expect(pixel_get(6, 7) && pixel_get(6, 11), "1P sits above 2P");
    expect(pixel_get(2, 7) && pixel_get(3, 8) && pixel_get(2, 9),
           "the arrow starts on 1P");
    expect(!pixel_get(2, 11) && !pixel_get(0, 7) && !pixel_get(14, 7),
           "the 1P 2P block is centered");
    expect(row_count(15) == 0, "the title has no paddles");
    feed(0, 1, 0, 0, 0);
    feed(0, 1, 0, 0, 0);
    expect(arkaled_on_title() && pixel_get(2, 11) && pixel_get(3, 12) && pixel_get(2, 13),
           "down moves the arrow to 2P");
    expect(!pixel_get(2, 7), "the arrow leaves 1P");
    feed(1, 0, 0, 0, 0);
    feed(1, 0, 0, 0, 0);
    expect(pixel_get(2, 7) && !pixel_get(2, 11), "up returns the arrow to 1P");
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(!arkaled_on_title() && pad_x() == 2 && arkaled_count() == 48,
           "the button starts 1P from the title");
}

static void test_title_two_player(void)
{
    input_reset();
    arkaled_reset();
    feed(0, 1, 0, 0, 0);
    feed(0, 1, 0, 0, 0);
    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(!arkaled_on_title(), "choosing 2P leaves the title");
    feed_p2(1, 0, 0);
    feed_p2(1, 0, 0);
    expect(arkaled_pad_x(2) == 9 && pad_x() == 2, "2P still splits the paddles");
}

static void test_solo_relay(void)
{
    input_reset();
    arkaled_reset();
    arkaled_begin_play(1);
    expect(pad_x() == 2 && arkaled_pad_x(2) == 10, "1P still seats both paddles");
    feed_p2(0, 1, 0);
    feed_p2(0, 1, 0);
    expect(arkaled_pad_x(2) == 10, "player two stick is idle in 1P");
    feed(0, 0, 0, 1, 1);
    expect(pad_x() == 2, "relay still waits for debounce");
    feed(0, 0, 0, 1, 1);
    feed(0, 0, 0, 1, 1);
    feed(0, 0, 0, 1, 1);
    expect(pad_x() == 5 && arkaled_pad_x(2) == 10,
           "the left paddle takes the rightward run");
    feed(0, 0, 0, 1, 1);
    expect(pad_x() == 5 && arkaled_pad_x(2) == 11,
           "the right paddle continues the relay");
    input_reset();
    feed(0, 0, 1, 0, 1);
    feed(0, 0, 1, 0, 1);
    expect(pad_x() == 5 && arkaled_pad_x(2) == 10,
           "coming back moves the right paddle first");
    feed(0, 0, 1, 0, 1);
    feed(0, 0, 1, 0, 1);
    expect(pad_x() == 5 && arkaled_pad_x(2) == 8,
           "the right paddle runs to its left limit");
    feed(0, 0, 1, 0, 1);
    expect(pad_x() == 4 && arkaled_pad_x(2) == 8,
           "the left paddle continues the return relay");

    arkaled_place_pads(4, 8);
    input_reset();
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 5 && arkaled_pad_x(2) == 8,
           "the paddles sit adjacent at the half");
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 5 && arkaled_pad_x(2) == 8,
           "the neighbor waits for the next paddle beat");
    feed(0, 0, 0, 1, 0);
    expect(pad_x() == 5 && arkaled_pad_x(2) == 9,
           "the next step past the half moves the right paddle");
}

int main(void)
{
    report_begin("arkaled");
    test_step();
    test_period();
    test_open();
    test_serve();
    test_move();
    test_button();
    test_bricks();
    test_paddle();
    test_carry();
    test_miss();
    test_clear();
    test_pop();
    test_cave();
    test_two_player();
    test_special_drop();
    test_item_prefers_p1();
    test_title();
    test_title_two_player();
    test_solo_relay();
    return report_end();
}
