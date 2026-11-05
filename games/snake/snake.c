#include "snake.h"

#include "frame.h"
#include "map.h"
#include "rand.h"
#include "sound.h"
#include "status.h"

#include <stddef.h>
#include <stdint.h>

#define TICK_HZ 60
#define MAX_SNAKE (FIELD_SIZE * FIELD_SIZE)
#define FOOD_N 4
#define SPEED_BLINK_SLOW_FPS 4
#define SPEED_BLINK_FAST_FPS 8
#define SPEED_BLINK_SLOW_HALF (TICK_HZ / (SPEED_BLINK_SLOW_FPS * 2))
#define SPEED_BLINK_FAST_HALF (TICK_HZ / (SPEED_BLINK_FAST_FPS * 2))
#define SPEED_PHASE1_FRAMES (8 * TICK_HZ)
#define SPEED_PHASE2_FRAMES (2 * TICK_HZ)
#define SPEED_LIFE_FRAMES (SPEED_PHASE1_FRAMES + SPEED_PHASE2_FRAMES)
#define SNAKE_SPEED_START 2
#define SNAKE_SPEED_MAX 30
#define DEATH_BLINK_CYCLES 4
#define DEATH_BLINK_HALF (TICK_HZ / (DEATH_BLINK_CYCLES * 2))
#define TURN_QUEUE_MAX 2

typedef enum {
    SNAKE_PLAY,
    SNAKE_DEATH,
} SnakePhase;

static uint8_t seg_x[MAX_SNAKE];
static uint8_t seg_y[MAX_SNAKE];
static int len;
static int dx;
static int dy;
static int pending_dx;
static int pending_dy;
static int queue_dx[TURN_QUEUE_MAX];
static int queue_dy[TURN_QUEUE_MAX];
static int queue_n;
static int move_accum;
static int speed_fps;
static int score;
static int food_x[FOOD_N];
static int food_y[FOOD_N];
static uint8_t food_live[FOOD_N];
static int speed_x;
static int speed_y;
static uint8_t speed_live;
static int speed_age;
static int blink_phase;
static SnakePhase phase;
static int death_phase;

static void status_put8(int value)
{
    int i;
    int v;

    if (value < 0)
        value = 0;
    if (value > 99999999)
        value = 99999999;
    v = value;
    for (i = 7; i >= 0; i--) {
        status_set_digit((uint8_t)i, (uint8_t)(v % 10));
        v /= 10;
    }
}

static void sync_status(void)
{
    status_put8(score);
}

static int cell_occupied(int x, int y)
{
    int i;

    if (map_get(x, y))
        return 1;
    for (i = 0; i < FOOD_N; i++) {
        if (!food_live[i])
            continue;
        if (food_x[i] == x && food_y[i] == y)
            return 1;
    }
    if (speed_live && speed_x == x && speed_y == y)
        return 1;
    return 0;
}

static void place_food(int slot)
{
    int tries;
    int x;
    int y;

    for (tries = 0; tries < 256; tries++) {
        x = (int)(rand_next() % FIELD_SIZE);
        y = (int)(rand_next() % FIELD_SIZE);
        if (!cell_occupied(x, y)) {
            food_x[slot] = x;
            food_y[slot] = y;
            food_live[slot] = 1;
            return;
        }
    }
    food_live[slot] = 0;
}

static void spawn_food(void)
{
    int i;

    for (i = 0; i < FOOD_N; i++)
        place_food(i);
}

static void spawn_speed(void)
{
    int tries;
    int x;
    int y;

    speed_live = 0;
    for (tries = 0; tries < 256; tries++) {
        x = (int)(rand_next() % FIELD_SIZE);
        y = (int)(rand_next() % FIELD_SIZE);
        if (!cell_occupied(x, y)) {
            speed_x = x;
            speed_y = y;
            speed_live = 1;
            speed_age = 0;
            return;
        }
    }
}

/*
 * Body cells live in seg_x/y for movement. Occupancy is a row bitmap (map).
 * Full sync uses hline/vline when the body is one straight segment; each step
 * only clears the tail cell and sets the new head (not a full-body repaint).
 */
static void map_sync_body(void)
{
    int i;
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int y0;
    int x0;

    map_clear();
    if (len <= 0)
        return;
    min_x = max_x = seg_x[0];
    min_y = max_y = seg_y[0];
    y0 = seg_y[0];
    x0 = seg_x[0];
    for (i = 1; i < len; i++) {
        if (seg_y[i] != y0)
            y0 = -1;
        if (seg_x[i] != x0)
            x0 = -1;
        if (seg_x[i] < min_x)
            min_x = seg_x[i];
        if (seg_x[i] > max_x)
            max_x = seg_x[i];
        if (seg_y[i] < min_y)
            min_y = seg_y[i];
        if (seg_y[i] > max_y)
            max_y = seg_y[i];
    }
    if (y0 >= 0 && max_x - min_x + 1 == len) {
        map_hline(min_x, seg_y[0], len, 1);
        return;
    }
    if (x0 >= 0 && max_y - min_y + 1 == len) {
        map_vline(seg_x[0], min_y, len, 1);
        return;
    }
    for (i = 0; i < len; i++)
        map_set(seg_x[i], seg_y[i], 1);
}

static void seed_snake(void)
{
    int start_x = FIELD_SIZE / 2 - 1;
    int start_y = FIELD_SIZE / 2;
    int i;

    len = 3;
    dx = 1;
    dy = 0;
    pending_dx = 1;
    pending_dy = 0;
    queue_n = 0;
    move_accum = 0;
    speed_fps = SNAKE_SPEED_START;
    score = 0;
    phase = SNAKE_PLAY;
    death_phase = 0;
    blink_phase = 0;
    speed_live = 0;
    speed_age = 0;
    for (i = 0; i < FOOD_N; i++)
        food_live[i] = 0;
    for (i = 0; i < len; i++) {
        seg_x[i] = (uint8_t)(start_x + (len - 1 - i));
        seg_y[i] = (uint8_t)start_y;
    }
    map_sync_body();
    spawn_food();
    spawn_speed();
    status_clear();
    sync_status();
}

void snake_reset(void)
{
    rand_reseed();
    seed_snake();
}

#if !defined(__AVR__)
void snake_seed(unsigned n)
{
    rand_seed(n == 0 ? 1u : (uint32_t)n);
}
#endif

int snake_score(void)
{
    return score;
}

int snake_length(void)
{
    return len;
}

int snake_head_x(void)
{
    return seg_x[0];
}

int snake_head_y(void)
{
    return seg_y[0];
}

int snake_food_count(void)
{
    int i;
    int n = 0;

    for (i = 0; i < FOOD_N; i++) {
        if (food_live[i])
            n++;
    }
    return n;
}

int snake_speed_fps(void)
{
    return speed_fps;
}

#if !defined(__AVR__)
int snake_death_blinking(void)
{
    return phase == SNAKE_DEATH;
}

void snake_place_food(int slot, int x, int y)
{
    if (slot < 0 || slot >= FOOD_N)
        return;
    food_x[slot] = x;
    food_y[slot] = y;
    food_live[slot] = 1;
}

int snake_food_xy(int slot, int *x, int *y)
{
    if (slot < 0 || slot >= FOOD_N || !food_live[slot])
        return 0;
    if (x != NULL)
        *x = food_x[slot];
    if (y != NULL)
        *y = food_y[slot];
    return 1;
}

int snake_body_at(int x, int y)
{
    return map_get(x, y);
}

void snake_place_speed(int x, int y)
{
    speed_x = x;
    speed_y = y;
    speed_live = 1;
    speed_age = 0;
}

int snake_turn_queue_depth(void)
{
    return queue_n;
}

void snake_pending_dir(int *out_dx, int *out_dy)
{
    if (out_dx != NULL)
        *out_dx = pending_dx;
    if (out_dy != NULL)
        *out_dy = pending_dy;
}

void snake_travel_dir(int *out_dx, int *out_dy)
{
    if (out_dx != NULL)
        *out_dx = dx;
    if (out_dy != NULL)
        *out_dy = dy;
}
#endif

static int food_lit(int slot)
{
    return food_live[slot] != 0;
}

static int speed_lit(void)
{
    int half;

    if (!speed_live)
        return 0;
    if (speed_age >= SPEED_LIFE_FRAMES)
        return 0;
    half = speed_age < SPEED_PHASE1_FRAMES ? SPEED_BLINK_SLOW_HALF
                                         : SPEED_BLINK_FAST_HALF;
    return (blink_phase / half) % 2 == 0;
}

static void tick_speed_life(void)
{
    if (!speed_live)
        return;
    speed_age++;
    if (speed_age >= SPEED_LIFE_FRAMES) {
        speed_live = 0;
        spawn_speed();
    }
}

static void draw(void)
{
    int i;

    clear();
    if (phase != SNAKE_DEATH || (death_phase / DEATH_BLINK_HALF) % 2 == 0)
        map_stamp_frame();
    for (i = 0; i < FOOD_N; i++) {
        if (food_lit(i))
            pixel((uint8_t)food_x[i], (uint8_t)food_y[i], true);
    }
    if (speed_lit())
        pixel((uint8_t)speed_x, (uint8_t)speed_y, true);
}

static int hits_food(int x, int y, int *slot_out)
{
    int i;

    for (i = 0; i < FOOD_N; i++) {
        if (!food_live[i])
            continue;
        if (food_x[i] == x && food_y[i] == y) {
            if (slot_out != NULL)
                *slot_out = i;
            return 1;
        }
    }
    return 0;
}

static int hits_speed(int x, int y)
{
    return speed_live && speed_age < SPEED_LIFE_FRAMES && speed_x == x &&
           speed_y == y;
}

static void begin_death(void)
{
    phase = SNAKE_DEATH;
    death_phase = 0;
}

static int opposite_dir(int ax, int ay, int bx, int by)
{
    return ax == -bx && ay == -by && (ax != 0 || ay != 0);
}

static void queue_turn(int ndx, int ndy)
{
    int ref_dx;
    int ref_dy;

    if (ndx == 0 && ndy == 0)
        return;
    if (queue_n > 0) {
        ref_dx = queue_dx[queue_n - 1];
        ref_dy = queue_dy[queue_n - 1];
    } else {
        ref_dx = pending_dx;
        ref_dy = pending_dy;
    }
    if (opposite_dir(ref_dx, ref_dy, ndx, ndy))
        return;
    if (ndx == ref_dx && ndy == ref_dy)
        return;
    if (queue_n >= TURN_QUEUE_MAX)
        return;
    if (queue_n == 0 && pending_dx == dx && pending_dy == dy) {
        pending_dx = ndx;
        pending_dy = ndy;
        return;
    }
    queue_dx[queue_n] = ndx;
    queue_dy[queue_n] = ndy;
    queue_n++;
}

static void apply_direction(const Input *in)
{
    if (in->left_pressed)
        queue_turn(-1, 0);
    else if (in->right_pressed)
        queue_turn(1, 0);
    else if (in->up_pressed)
        queue_turn(0, -1);
    else if (in->down_pressed)
        queue_turn(0, 1);
}

static void commit_next_turn(void)
{
    if (queue_n <= 0)
        return;
    pending_dx = queue_dx[0];
    pending_dy = queue_dy[0];
    queue_n--;
    if (queue_n > 0) {
        queue_dx[0] = queue_dx[1];
        queue_dy[0] = queue_dy[1];
    }
}

static void shift_body(int grow)
{
    int i;

    if (!grow)
        map_set(seg_x[len - 1], seg_y[len - 1], 0);
    if (grow && len < MAX_SNAKE)
        len++;
    for (i = len - 1; i > 0; i--) {
        seg_x[i] = seg_x[i - 1];
        seg_y[i] = seg_y[i - 1];
    }
}

static void eat_speed(void)
{
    if (speed_fps < SNAKE_SPEED_MAX)
        speed_fps++;
    speed_live = 0;
    spawn_speed();
    sound(SOUND_POP);
}

static void step_snake(void)
{
    int nx;
    int ny;
    int food_slot = 0;
    int ate_food = 0;
    int ate_speed = 0;

    dx = pending_dx;
    dy = pending_dy;
    nx = (int)seg_x[0] + dx;
    ny = (int)seg_y[0] + dy;
    if (nx < 0 || ny < 0 || nx > FIELD_LAST || ny > FIELD_LAST) {
        begin_death();
        return;
    }
    if (map_get(nx, ny)) {
        begin_death();
        return;
    }
    ate_speed = hits_speed(nx, ny);
    ate_food = hits_food(nx, ny, &food_slot);
    shift_body(ate_food);
    seg_x[0] = (uint8_t)nx;
    seg_y[0] = (uint8_t)ny;
    map_set(nx, ny, 1);
    if (ate_food) {
        score++;
        sync_status();
        sound(SOUND_POP);
        food_live[food_slot] = 0;
        place_food(food_slot);
    }
    if (ate_speed)
        eat_speed();
    commit_next_turn();
}

void snake_tick(const Input *in)
{
    rand_feed(in);
    blink_phase++;
    tick_speed_life();
    if (phase == SNAKE_DEATH) {
        death_phase++;
        if (death_phase >= DEATH_BLINK_HALF * 2 * DEATH_BLINK_CYCLES)
            seed_snake();
        draw();
        return;
    }
    apply_direction(in);
    move_accum += speed_fps;
    while (move_accum >= TICK_HZ) {
        move_accum -= TICK_HZ;
        step_snake();
        if (phase == SNAKE_DEATH) {
            draw();
            return;
        }
    }
    draw();
}
