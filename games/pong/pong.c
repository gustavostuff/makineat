#include "pong.h"

#include "entity.h"
#include "frame.h"
#include "over.h"
#include "rand.h"
#include "sound.h"
#include "status.h"
#include "title/title.h"

#include <stdint.h>

#define FIELD 16
#define PAD_H 4
#define PAD_SLOW 2
#define PAD_FAST 1
#define BALL_SLOW 7
#define BALL_DEBUG_PERIOD 60 /* one step per second at 60 Hz */
#define RAMP (10 * 60)
#define WIN_SCORE 100
#define GOAL_L 0
#define GOAL_R 15
#define PLAY_L 1
#define PLAY_R 14
#define SCORE_PAUSE 60
#define SCORE_BLINK_HALF 8 /* ~4 on/off cycles in one second at 60 Hz */

#ifndef PONG_DEBUG_SLOW_BALL
#define PONG_DEBUG_SLOW_BALL 0
#endif

static const Shape pad_shape = SHAPE4(0, 0, "1", "1", "1", "1");
static const Shape dot = SHAPE1(0, 0, "1");

static int over;
static int pad_l;
static int pad_r;
static int ball_id;
static int p1_score;
static int p2_score;
static int lpace;
static int rpace;
static int age;
static int live;
static int server;
static int last_scorer;
static int score_pause;
static int score_blink_phase;
static int demo;
static int hold_english_side; /* -1 left, 1 right: one Y nudge this approach */

int pong_period(int ticks)
{
    int p;

    if (PONG_DEBUG_SLOW_BALL)
        return BALL_DEBUG_PERIOD;
    if (ticks < 0)
        ticks = 0;
    p = BALL_SLOW - ticks / RAMP;
    if (p < 1)
        p = 1;
    return p;
}

static void status_put4(int start, int value)
{
    if (value < 0)
        value = 0;
    if (value > 9999)
        value = 9999;
    status_set_digit((uint8_t)start, (uint8_t)(value / 1000));
    status_set_digit((uint8_t)(start + 1), (uint8_t)((value / 100) % 10));
    status_set_digit((uint8_t)(start + 2), (uint8_t)((value / 10) % 10));
    status_set_digit((uint8_t)(start + 3), (uint8_t)(value % 10));
}

static void status_sync(void)
{
    status_put4(0, p1_score);
    status_put4(4, p2_score);
}

static int score_blink_lit(void)
{
    return (score_blink_phase / SCORE_BLINK_HALF) % 2 == 0;
}

int pong_pad_y(int player)
{
    Entity *pad = entity_get(player == 2 ? pad_r : pad_l);

    return pad == NULL ? (FIELD - PAD_H) / 2 : pad->y;
}

int pong_ball_y(void)
{
    Entity *ball = entity_get(ball_id);

    return ball == NULL ? FIELD / 2 : ball->y;
}

int pong_awaiting_serve(void)
{
    return !live && score_pause == 0;
}

#if !defined(__AVR__)
int pong_in_demo(void)
{
    return demo;
}
#endif

static void draw(void)
{
    Entity *ball;

    if (!demo)
        status_sync();
    world_draw();
    if (score_pause > 0) {
        ball = entity_get(ball_id);
        if (ball != NULL && !score_blink_lit())
            pixel((uint8_t)ball->x, (uint8_t)ball->y, false);
    }
}

void pong_seed(unsigned n)
{
    rand_seed(n == 0 ? 1u : (uint32_t)n);
}

static int on_pad(int py, int pad_y)
{
    return py >= pad_y && py < pad_y + PAD_H;
}

static int clamp_y(int y)
{
    if (y < 0)
        return 0;
    if (y >= FIELD)
        return FIELD - 1;
    return y;
}

static int hug_left(Entity *ball, Entity *l)
{
    return ball->vx < 0 && ball->x == PLAY_L && on_pad(ball->y, l->y);
}

static int hug_right(Entity *ball, Entity *r)
{
    return ball->vx > 0 && ball->x == PLAY_R && on_pad(ball->y, r->y);
}

static void clear_hold_english(int hl, int hr)
{
    if (!hl && hold_english_side == -1)
        hold_english_side = 0;
    if (!hr && hold_english_side == 1)
        hold_english_side = 0;
}

/*
 * While the ball waits between slow steps on the play column, a moving
 * paddle may nudge Y once and force the bounce step this tick.
 */
static void hold_english(Entity *ball, int side, int pad_step, int *pace, int every)
{
    if (pad_step == 0 || every <= 1 || *pace <= 0 || *pace >= every)
        return;
    if (hold_english_side == side)
        return;
    ball->y = (int8_t)clamp_y(ball->y + pad_step);
    hold_english_side = side;
    *pace = every - 1;
}

/*
 * Corner bounce only when the ball arrives onto the facing end:
 * down onto the top pixel, or up onto the bottom pixel.
 * Leaving the far end is a normal face bounce (vx only).
 */
static int pad_corner(int py, int ny, int pad_y, int vy)
{
    int top = pad_y;
    int bot = pad_y + PAD_H - 1;

    if (on_pad(py, pad_y))
        return 0;
    if (vy > 0 && ny == top)
        return 1;
    if (vy < 0 && ny == bot)
        return 1;
    return 0;
}

static int step_pad(Entity *pad, int dir, int *pace, int every)
{
    int next;

    if (dir == 0) {
        *pace = 0;
        return 0;
    }
    if (*pace > 0) {
        (*pace)--;
        return 0;
    }
    *pace = every - 1;
    next = pad->y + dir;
    if (next < 0)
        next = 0;
    if (next > FIELD - PAD_H)
        next = FIELD - PAD_H;
    if (next == pad->y)
        return 0;
    pad->y = next;
    return 1;
}

static void center_paddles(Entity *l, Entity *r)
{
    int y = (FIELD - PAD_H) / 2;

    if (l != NULL)
        l->y = y;
    if (r != NULL)
        r->y = y;
    lpace = 0;
    rpace = 0;
}

static void stick_serve(Entity *l, Entity *r, Entity *ball)
{
    if (server == 2) {
        ball->x = (int8_t)PLAY_R;
        ball->y = (int8_t)(r->y + 1);
    } else {
        ball->x = (int8_t)PLAY_L;
        ball->y = (int8_t)(l->y + 1);
    }
}

/*
 * The ball plays in columns 1..14. Columns 0 and 15 are the paddle/goal line.
 * A step into column 0 or 15 on a paddle row bounces back to 1 or 14.
 * The same step on an open row is a point (ball stays on the goal column).
 */
static int left_pad_block(Entity *l, Entity *ball, int nx, int ny)
{
    if (ball->vx >= 0 || nx > GOAL_L)
        return 0;
    return on_pad(ball->y, l->y) || on_pad(ny, l->y);
}

static int right_pad_block(Entity *r, Entity *ball, int nx, int ny)
{
    if (ball->vx <= 0 || nx < GOAL_R)
        return 0;
    return on_pad(ball->y, r->y) || on_pad(ny, r->y);
}

static void bounce_off_pad(Entity *ball, int *nx, int *ny, int pad_y, int xmin, int xmax,
                           int pad_step)
{
    int corner = pad_corner(ball->y, *ny, pad_y, ball->vy);

    ball->vx = (int8_t)(-ball->vx);
    *nx = ball->x + ball->vx;
    if (*nx < xmin)
        *nx = xmin;
    if (*nx > xmax)
        *nx = xmax;
    if (corner) {
        ball->vy = (int8_t)(-ball->vy);
        *ny = ball->y + ball->vy;
        if (*ny < 0 || *ny >= FIELD) {
            ball->vy = (int8_t)(-ball->vy);
            *ny = ball->y + ball->vy;
        }
    }
    if (pad_step != 0) {
        *ny = clamp_y(*ny + pad_step);
        ball->y = (int8_t)*ny;
    }
    sound(SOUND_POP);
}

static int ball_step(Entity *l, Entity *r, Entity *ball, int l_step, int r_step)
{
    int nx = ball->x + ball->vx;
    int ty = ball->y + ball->vy;
    int ny;
    int scorer = 0;
    int left_eng;
    int right_eng;

    ny = ty;
    if (ny < 0 || ny >= FIELD) {
        ball->vy = (int8_t)(-ball->vy);
        ny = ball->y + ball->vy;
        sound(SOUND_POP);
    }

    left_eng = hold_english_side == -1 ? 0 : l_step;
    right_eng = hold_english_side == 1 ? 0 : r_step;

    if (left_pad_block(l, ball, nx, ny))
        bounce_off_pad(ball, &nx, &ny, l->y, PLAY_L, PLAY_R, left_eng);
    else if (ball->vx < 0 && nx <= GOAL_L) {
        nx = GOAL_L;
        scorer = 2;
    } else if (right_pad_block(r, ball, nx, ny))
        bounce_off_pad(ball, &nx, &ny, r->y, PLAY_L, PLAY_R, right_eng);
    else if (ball->vx > 0 && nx >= GOAL_R) {
        nx = GOAL_R;
        scorer = 1;
    }

    ball->x = (int8_t)nx;
    ball->y = (int8_t)ny;
    return scorer;
}

static void spawn_actors(void)
{
    world_reset();
    pad_l = entity_spawn(&pad_shape, GOAL_L, (FIELD - PAD_H) / 2, 0, 0, PAD_SLOW, ENTITY_INACTIVE);
    pad_r = entity_spawn(&pad_shape, (int8_t)GOAL_R, (FIELD - PAD_H) / 2, 0, 0, PAD_SLOW,
                         ENTITY_INACTIVE);
    ball_id = entity_spawn(&dot, (int8_t)(FIELD / 2), (int8_t)(FIELD / 2), 0, 0, BALL_SLOW,
                            ENTITY_INACTIVE | ENTITY_CAN_COLLIDE);
}

static void serve_from_paddle(int player)
{
    Entity *l = entity_get(pad_l);
    Entity *r = entity_get(pad_r);
    Entity *ball = entity_get(ball_id);
    uint32_t bits;

    if (l == NULL || r == NULL || ball == NULL)
        return;
    bits = rand_next() >> 30;
    live = 0;
    server = player == 2 ? 2 : 1;
    ball->vy = (bits & 2u) ? 1 : -1;
    ball->pace = pong_period(age) - 1;
    if (ball->pace < 0)
        ball->pace = 0;
    if (player == 2) {
        ball->x = (int8_t)PLAY_R;
        ball->y = (int8_t)(r->y + 1);
        ball->vx = -1;
    } else {
        ball->x = (int8_t)PLAY_L;
        ball->y = (int8_t)(l->y + 1);
        ball->vx = 1;
    }
}

void pong_begin_play(int player)
{
    rand_reseed();
    demo = 0;
    over = 0;
    over_clear();
    score_pause = 0;
    score_blink_phase = 0;
    p1_score = 0;
    p2_score = 0;
    lpace = 0;
    rpace = 0;
    hold_english_side = 0;
    age = 0;
    live = 0;
    status_clear();
    if (player != 1 && player != 2) {
        center_paddles(entity_get(pad_l), entity_get(pad_r));
        player = 1;
    }
    serve_from_paddle(player);
    draw();
}

void pong_reset(void)
{
    rand_reseed();
    over = 0;
    over_clear();
    score_pause = 0;
    score_blink_phase = 0;
    p1_score = 0;
    p2_score = 0;
    lpace = 0;
    rpace = 0;
    hold_english_side = 0;
    age = 0;
    live = 0;
    demo = 1;
    status_clear();
    spawn_actors();
    center_paddles(entity_get(pad_l), entity_get(pad_r));
    serve_from_paddle(1);
    status_set_decode(0);
    title_reset();
    draw();
    title_paint();
}

static void finish_score_pause(Entity *l, Entity *r)
{
    score_pause = 0;
    score_blink_phase = 0;
    if (!demo && (p1_score >= WIN_SCORE || p2_score >= WIN_SCORE)) {
        over = 1;
        over_start_pong(p1_score >= WIN_SCORE ? 1 : 2);
        return;
    }
    center_paddles(l, r);
    if (demo) {
        age = 0;
        serve_from_paddle(1);
    } else
        serve_from_paddle(last_scorer == 1 ? 2 : 1);
    draw();
}

static void begin_score_pause(int scorer, Entity *ball)
{
    last_scorer = scorer;
    if (scorer == 1)
        p1_score++;
    else
        p2_score++;
    score_pause = SCORE_PAUSE;
    score_blink_phase = 0;
    ball->vx = 0;
    ball->vy = 0;
    live = 0;
    draw();
}

void pong_play_tick(const Input *in)
{
    Entity *l;
    Entity *r;
    Entity *ball;
    int ldir = 0;
    int rdir = 0;
    int l_moved;
    int r_moved;
    int l_step;
    int r_step;
    int launched;
    int levery;
    int revery;
    int scorer;

    l = entity_get(pad_l);
    r = entity_get(pad_r);
    ball = entity_get(ball_id);
    if (l == NULL || r == NULL || ball == NULL)
        return;

    if (score_pause > 0) {
        score_pause--;
        score_blink_phase++;
        if (score_pause == 0)
            finish_score_pause(l, r);
        else
            draw();
        return;
    }

    if (in->up && !in->down)
        ldir = -1;
    if (in->down && !in->up)
        ldir = 1;
    if (in->p2_up && !in->p2_down)
        rdir = -1;
    if (in->p2_down && !in->p2_up)
        rdir = 1;

    levery = in->button ? PAD_FAST : PAD_SLOW;
    revery = in->p2_button ? PAD_FAST : PAD_SLOW;
    l_moved = step_pad(l, ldir, &lpace, levery);
    r_moved = step_pad(r, rdir, &rpace, revery);
    if (l_moved)
        l_step = ldir;
    else
        l_step = 0;
    if (r_moved)
        r_step = rdir;
    else
        r_step = 0;

    if (!live) {
        stick_serve(l, r, ball);
        launched = server == 2 ? r_moved : l_moved;
        if (!launched) {
            draw();
            return;
        }
        live = 1;
    }

    age++;
    ball->every = (uint8_t)pong_period(age);
    if (live) {
        int hl = hug_left(ball, l);
        int hr = hug_right(ball, r);

        clear_hold_english(hl, hr);
        if (hl)
            hold_english(ball, -1, l_step, &ball->pace, (int)ball->every);
        if (hr)
            hold_english(ball, 1, r_step, &ball->pace, (int)ball->every);
    }
    ball->pace++;
    if (ball->pace >= ball->every) {
        ball->pace = 0;
        scorer = ball_step(l, r, ball, l_step, r_step);
        if (scorer != 0) {
            begin_score_pause(scorer, ball);
            return;
        }
    }
    draw();
}

void pong_tick(const Input *in)
{
    rand_feed(in);
    if (demo) {
        title_tick(in);
        return;
    }
    if (over) {
        int who = in->button_pressed ? 1 : 2;

        if (over_tick(in))
            pong_begin_play(who);
        return;
    }
    pong_play_tick(in);
}

#if !defined(__AVR__)
void pong_place_paddles(int ly, int ry)
{
    Entity *l = entity_get(pad_l);
    Entity *r = entity_get(pad_r);

    if (l != NULL)
        l->y = ly;
    if (r != NULL)
        r->y = ry;
    lpace = 0;
    rpace = 0;
}

void pong_place_ball(int x, int y, int vx, int vy)
{
    pong_place_ball_timed(x, y, vx, vy, 1, 0);
}

void pong_place_ball_timed(int x, int y, int vx, int vy, int every, int pace)
{
    Entity *ball = entity_get(ball_id);

    if (ball == NULL)
        return;
    demo = 0;
    status_set_decode(0xFF);
    score_pause = 0;
    score_blink_phase = 0;
    hold_english_side = 0;
    age = RAMP * BALL_SLOW;
    ball->x = (int8_t)x;
    ball->y = (int8_t)y;
    ball->vx = (int8_t)vx;
    ball->vy = (int8_t)vy;
    if (every < 1)
        every = 1;
    if (pace < 0)
        pace = 0;
    if (pace >= every)
        pace = every - 1;
    ball->every = (uint8_t)every;
    ball->pace = (uint8_t)pace;
    live = 1;
}
#endif
