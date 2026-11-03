#include "arkaled.h"

#include "collide.h"
#include "entity.h"
#include "frame.h"
#include "rand.h"
#include "sound.h"
#include "status.h"
#include "text.h"

#include <stdint.h>
#include <string.h>

#define FIELD 16
#define HALF 8
#define PAD_W 3
#define PAD_SLOW 4
#define PAD_FAST 1
#define BRICK_X 2
#define BRICK_Y 2
#define BRICK_COLS 12
#define BRICK_ROWS 4
#define MAX_BALLS 4
#define MAX_ITEMS 5
#define SPECIAL_N 5
#define BALL_EVERY 15
#define BALL_FAST 2 /* 30 FPS at 60 Hz */
#define BALL_DEBUG_PERIOD 240 /* 0.25 FPS at 60 Hz */
#define PAD_DEBUG_PERIOD 60 /* 1 FPS at 60 Hz */
#define ITEM_EVERY 30
#define RAMP (2 * 60)

#ifndef ARKALED_DEBUG_SLOW
#define ARKALED_DEBUG_SLOW 0
#endif
#define BLINK4_HALF 8
#define BLINK2_HALF 15
#define MISS_PAUSE 60
#define MISS_BLINK_HALF 8
#define PAD1_MIN 0
#define PAD1_MAX (HALF - PAD_W)
#define PAD2_MIN HALF
#define PAD2_MAX (FIELD - PAD_W)
#define PAD1_X ((HALF - PAD_W) / 2)
#define PAD2_X (HALF + (HALF - PAD_W) / 2)
#define ARROW_W 3
#define ARROW_H 3
#define MENU_GAP 1
#define MENU_TEXT_W (2 * TEXT_W)
#define MENU_TEXT_H (2 * TEXT_H)
#define MENU_W (ARROW_W + MENU_GAP + MENU_TEXT_W)
#define MENU_H MENU_TEXT_H
#define MENU_X ((FIELD - MENU_W) / 2)
#define MENU_Y ((FIELD - MENU_H) / 2 + 3)
#define TEXT_X (MENU_X + ARROW_W + MENU_GAP)
#define NAME_X 0
#define NAME_Y 2
#define NAME_W 16
#define NAME_H 4
#define NAME_TEXT "arkaled"

#if BRICK_COLS * BRICK_ROWS + 2 + MAX_BALLS + MAX_ITEMS > ENTITY_MAX
#error the arkaled block does not fit in the entity table
#endif

static const Shape brick_shape = SHAPE1(0, 0, "1");
static const Shape pad_shape = SHAPE1(0, 0, "111");
static const Shape dot = SHAPE1(0, 0, "1");

static int pad_id[2];
static int ppace[2];
static int ball_id[MAX_BALLS];
static int ball_age[MAX_BALLS];
static int ball_held[MAX_BALLS];
static int item_id[MAX_ITEMS];
static int piece[BRICK_ROWS][BRICK_COLS];
static uint8_t special[BRICK_ROWS][BRICK_COLS];
static int remain;
static int ticks;
static int pad_score[2];
static int on_title;
static int pick;
static int solo;
static int miss_pause;
static int miss_blink_phase;
static TextScroll name_scroll;

int arkaled_period(int ticks)
{
    int p;

    if (ARKALED_DEBUG_SLOW)
        return BALL_DEBUG_PERIOD;
    if (ticks < 0)
        ticks = 0;
    p = BALL_EVERY - ticks / RAMP;
    if (p < BALL_FAST)
        p = BALL_FAST;
    return p;
}

static int pad_every(int button)
{
    if (ARKALED_DEBUG_SLOW)
        return PAD_DEBUG_PERIOD;
    return button ? PAD_FAST : PAD_SLOW;
}

static int piece_at(int x, int y, int *col, int *row)
{
    if (y < BRICK_Y || y >= BRICK_Y + BRICK_ROWS)
        return 0;
    if (x < BRICK_X || x >= BRICK_X + BRICK_COLS)
        return 0;
    *row = y - BRICK_Y;
    *col = x - BRICK_X;
    return 1;
}

static int piece_live(int col, int row)
{
    return entity_get(piece[row][col]) != NULL;
}

int arkaled_on(int x, int y)
{
    int col;
    int row;

    if (!piece_at(x, y, &col, &row))
        return 0;
    return piece_live(col, row);
}

int arkaled_special(int x, int y)
{
    int col;
    int row;

    if (!piece_at(x, y, &col, &row))
        return 0;
    return special[row][col] && piece_live(col, row);
}

int arkaled_count(void)
{
    return remain;
}

int arkaled_pad_x(int player)
{
    Entity *pad = entity_get(pad_id[player == 2 ? 1 : 0]);

    return pad == NULL ? -1 : pad->x;
}

static int first_ball(void)
{
    int i;

    for (i = 0; i < MAX_BALLS; i++) {
        if (entity_get(ball_id[i]) != NULL)
            return i;
    }
    return -1;
}

int arkaled_ball_count(void)
{
    int i;
    int n = 0;

    for (i = 0; i < MAX_BALLS; i++) {
        if (entity_get(ball_id[i]) != NULL)
            n++;
    }
    return n;
}

int arkaled_item_at(int *x, int *y)
{
    int i;

    for (i = 0; i < MAX_ITEMS; i++) {
        Entity *it = entity_get(item_id[i]);

        if (it == NULL)
            continue;
        if (x != NULL)
            *x = it->x;
        if (y != NULL)
            *y = it->y;
        return 1;
    }
    return 0;
}

static int spawn_piece(int col, int row)
{
    int x = BRICK_X + col;
    int y = BRICK_Y + row;

    piece[row][col] = entity_spawn(&brick_shape, x, y, 0, 0, 1,
                                   ENTITY_INACTIVE | ENTITY_CAN_BE_COLLIDED);
    return piece[row][col] >= 0;
}

static void pick_specials(void)
{
    int n = 0;
    int guard = 0;
    int cells = BRICK_COLS * BRICK_ROWS;

    memset(special, 0, sizeof special);
    while (n < SPECIAL_N && guard < cells * 4) {
        int cell = (int)(rand_next() % (uint32_t)cells);
        int col = cell % BRICK_COLS;
        int row = cell / BRICK_COLS;

        if (piece_live(col, row) && !special[row][col]) {
            special[row][col] = 1;
            n++;
        }
        guard++;
    }
}

void arkaled_fill(void)
{
    int col;
    int row;

    remain = 0;
    memset(special, 0, sizeof special);
    for (row = 0; row < BRICK_ROWS; row++) {
        for (col = 0; col < BRICK_COLS; col++) {
            if (!piece_live(col, row) && !spawn_piece(col, row))
                continue;
            remain++;
        }
    }
    pick_specials();
}

void arkaled_wipe(void)
{
    int col;
    int row;

    for (row = 0; row < BRICK_ROWS; row++) {
        for (col = 0; col < BRICK_COLS; col++) {
            entity_kill(piece[row][col]);
            piece[row][col] = -1;
            special[row][col] = 0;
        }
    }
    remain = 0;
}

void arkaled_set(int x, int y, int on)
{
    int col;
    int row;

    if (!piece_at(x, y, &col, &row))
        return;
    if (on) {
        if (piece_live(col, row))
            return;
        if (!spawn_piece(col, row))
            return;
        remain++;
        return;
    }
    if (!piece_live(col, row))
        return;
    entity_kill(piece[row][col]);
    piece[row][col] = -1;
    special[row][col] = 0;
    remain--;
}

void arkaled_mark_special(int x, int y)
{
    int col;
    int row;

    if (!piece_at(x, y, &col, &row) || !piece_live(col, row))
        return;
    special[row][col] = 1;
}

static int spawn_item_at(int x, int y)
{
    int i;

    for (i = 0; i < MAX_ITEMS; i++) {
        if (entity_get(item_id[i]) != NULL)
            continue;
        item_id[i] = entity_spawn(&dot, x, y, 0, 1, ITEM_EVERY, ENTITY_INACTIVE);
        return item_id[i] >= 0;
    }
    return 0;
}

static int clear_brick(int x, int y)
{
    int col;
    int row;
    int was_special;

    if (!piece_at(x, y, &col, &row) || !piece_live(col, row))
        return 0;
    was_special = special[row][col];
    entity_kill(piece[row][col]);
    piece[row][col] = -1;
    special[row][col] = 0;
    remain--;
    if (was_special)
        spawn_item_at(x, y);
    return 1;
}

void arkaled_ball(int *x, int *y, int *vx, int *vy)
{
    int i = first_ball();
    Entity *ball = i < 0 ? NULL : entity_get(ball_id[i]);

    if (ball == NULL) {
        *x = 0;
        *y = 0;
        *vx = 0;
        *vy = 0;
        return;
    }
    *x = ball->x;
    *y = ball->y;
    *vx = ball->vx;
    *vy = ball->vy;
}

static int blink_on(int half)
{
    return (ticks / half) % 2 == 0;
}

static int item_lit(int pace)
{
    return ((pace * 8) / ITEM_EVERY) % 2 == 0;
}

static int pad_covers(const Entity *pad, int x)
{
    return pad != NULL && x >= pad->x && x < pad->x + PAD_W;
}

int arkaled_score(int player)
{
    if (player != 1 && player != 2)
        return 0;
    return pad_score[player - 1];
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

static void draw(void)
{
    Entity *p1 = entity_get(pad_id[0]);
    Entity *p2 = entity_get(pad_id[1]);
    int x;
    int row;
    int col;
    int i;

    world_draw();
    status_put4(0, pad_score[0]);
    status_put4(4, pad_score[1]);
    if (!blink_on(BLINK2_HALF)) {
        for (row = 0; row < BRICK_ROWS; row++) {
            for (col = 0; col < BRICK_COLS; col++) {
                if (!special[row][col] || !piece_live(col, row))
                    continue;
                pixel((uint8_t)(BRICK_X + col), (uint8_t)(BRICK_Y + row), false);
            }
        }
    }
    if (!blink_on(BLINK4_HALF)) {
        for (x = 0; x < FIELD; x++) {
            if (pad_covers(p1, x) && pad_covers(p2, x))
                pixel((uint8_t)x, (uint8_t)(FIELD - 1), false);
        }
    }
    for (i = 0; i < MAX_ITEMS; i++) {
        Entity *it = entity_get(item_id[i]);

        if (it != NULL && !item_lit(it->pace))
            pixel((uint8_t)it->x, (uint8_t)it->y, false);
    }
    if (miss_pause > 0 && (miss_blink_phase / MISS_BLINK_HALF) % 2 != 0) {
        for (i = 0; i < MAX_BALLS; i++) {
            Entity *ball = entity_get(ball_id[i]);

            if (ball != NULL)
                pixel((uint8_t)ball->x, (uint8_t)ball->y, false);
        }
    }
}

static void name_scroll_begin(void)
{
    text_scroll_stop(&name_scroll);
    memset(&name_scroll, 0, sizeof name_scroll);
    name_scroll.repeat = TEXT_SCROLL_BOUNCE;
    name_scroll.viewport_w = NAME_W;
    name_scroll.viewport_h = NAME_H;
    text_scroll_start(&name_scroll, NAME_X, NAME_Y, NAME_TEXT);
}

static void paint_title(void)
{
    static const uint8_t tip[ARROW_H][ARROW_W] = {
        {1, 0, 0},
        {0, 1, 0},
        {1, 0, 0},
    };
    int r;
    int c;
    int ay;

    clear();
    print_scroll(&name_scroll);
    text_print(TEXT_X, MENU_Y, "1P");
    text_print(TEXT_X, MENU_Y + TEXT_H, "2P");
    ay = pick == 2 ? MENU_Y + TEXT_H : MENU_Y;
    for (r = 0; r < ARROW_H; r++) {
        for (c = 0; c < ARROW_W; c++) {
            if (tip[r][c])
                pixel((uint8_t)(MENU_X + c), (uint8_t)(ay + r), true);
        }
    }
}

void arkaled_seed(unsigned n)
{
    rand_seed(n == 0 ? 1u : (uint32_t)n);
}

static void aim(Entity *ball)
{
    ball->vx = ((rand_next() >> 30) & 1u) ? 1 : -1;
    ball->vy = -1;
}

static int on_paddle(const Entity *ball, int pad_x)
{
    return ball->y == FIELD - 2 && ball->x >= pad_x && ball->x < pad_x + PAD_W;
}

static void carry(Entity *pad, Entity *ball, int from_x)
{
    int dx = pad->x - from_x;
    int nx;

    if (!on_paddle(ball, from_x))
        return;
    nx = ball->x + dx;
    if (nx < 0)
        nx = 0;
    if (nx >= FIELD)
        nx = FIELD - 1;
    ball->x = nx;
}

static int pad_can_move(const Entity *pad, int dir, int xmin, int xmax)
{
    if (pad == NULL || dir == 0)
        return 0;
    if (dir > 0)
        return pad->x < xmax;
    return pad->x > xmin;
}

static int step_pad(Entity *pad, int dir, int *pace, int every, int xmin, int xmax)
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
    next = pad->x + dir;
    if (next < xmin)
        next = xmin;
    if (next > xmax)
        next = xmax;
    if (next == pad->x)
        return 0;
    if ((dir > 0 && next < pad->x) || (dir < 0 && next > pad->x))
        return 0;
    pad->x = next;
    return 1;
}

static int blocked(int x, int y, void *ctx)
{
    Entity **pads = ctx;

    if (x < 0 || x >= FIELD || y < 0)
        return 1;
    if (y >= FIELD)
        return 0;
    if (y == FIELD - 1)
        return pad_covers(pads[0], x) || pad_covers(pads[1], x);
    return arkaled_on(x, y);
}

static int ball_step(Entity *pads[2], Entity *ball)
{
    CollideHit hit[2];
    int n;
    int i;
    int broke = 0;
    int bounce_who = 0;
    int x = ball->x;
    int y = ball->y;
    int vx = ball->vx;
    int vy = ball->vy;

    n = collide_step(&x, &y, &vx, &vy, blocked, pads, hit, 2);
    ball->x = x;
    ball->y = y;
    ball->vx = vx;
    ball->vy = vy;
    for (i = 0; i < n; i++) {
        if (hit[i].y == FIELD - 1) {
            int who = 0;

            if (pad_covers(pads[0], hit[i].x))
                who = 1;
            else if (pad_covers(pads[1], hit[i].x))
                who = 2;
            if (who != 0 && bounce_who == 0)
                bounce_who = solo ? 1 : who;
            continue;
        }
        if (!clear_brick(hit[i].x, hit[i].y))
            continue;
        broke = 1;
    }
    if (bounce_who != 0 && pad_score[bounce_who - 1] < 9999)
        pad_score[bounce_who - 1]++;
    if (n > 0)
        sound(SOUND_POP);
    if (y >= FIELD - 1)
        return 1;
    if (broke && remain == 0)
        return 2;
    return 0;
}

static void kill_ball(int i)
{
    entity_kill(ball_id[i]);
    ball_id[i] = -1;
    ball_held[i] = 0;
    ball_age[i] = 0;
}

static void kill_items(void)
{
    int i;

    for (i = 0; i < MAX_ITEMS; i++) {
        entity_kill(item_id[i]);
        item_id[i] = -1;
    }
}

static int spawn_held(int player)
{
    Entity *pad = entity_get(pad_id[player == 2 ? 1 : 0]);
    int i;
    int x;

    if (pad == NULL)
        return 0;
    for (i = 0; i < MAX_BALLS; i++) {
        if (entity_get(ball_id[i]) != NULL)
            continue;
        x = pad->x + 2;
        if (x >= pad->x + PAD_W)
            x = pad->x + PAD_W - 1;
        ball_id[i] = entity_spawn(&dot, x, FIELD - 2, 0, 0, BALL_EVERY,
                                  ENTITY_INACTIVE | ENTITY_CAN_COLLIDE);
        if (ball_id[i] < 0)
            return 0;
        ball_held[i] = player;
        ball_age[i] = 0;
        return 1;
    }
    return 0;
}

static void spawn_actors(int x1, int x2, int bx, int by)
{
    int i;

    for (i = 0; i < 2; i++) {
        if (pad_id[i] >= 0)
            entity_kill(pad_id[i]);
        pad_id[i] = -1;
    }
    for (i = 0; i < MAX_BALLS; i++) {
        if (ball_id[i] >= 0)
            entity_kill(ball_id[i]);
        ball_id[i] = -1;
        ball_held[i] = 0;
        ball_age[i] = 0;
    }
    kill_items();
    pad_id[0] = entity_spawn(&pad_shape, x1, FIELD - 1, 0, 0, PAD_SLOW,
                             ENTITY_INACTIVE | ENTITY_CAN_BE_COLLIDED);
    pad_id[1] = entity_spawn(&pad_shape, x2, FIELD - 1, 0, 0, PAD_SLOW,
                             ENTITY_INACTIVE | ENTITY_CAN_BE_COLLIDED);
    ball_id[0] = entity_spawn(&dot, bx, by, 0, 0, BALL_EVERY,
                              ENTITY_INACTIVE | ENTITY_CAN_COLLIDE);
    ball_held[0] = 1;
}

static void blank_pieces(void)
{
    int col;
    int row;

    for (row = 0; row < BRICK_ROWS; row++) {
        for (col = 0; col < BRICK_COLS; col++) {
            piece[row][col] = -1;
            special[row][col] = 0;
        }
    }
    remain = 0;
}

static void ready(int fill)
{
    Entity *ball;

    ppace[0] = 0;
    ppace[1] = 0;
    ticks = 0;
    spawn_actors(PAD1_X, PAD2_X, PAD1_X + 2, FIELD - 2);
    ball = entity_get(ball_id[0]);
    if (ball != NULL) {
        ball->vx = 0;
        ball->vy = 0;
        ball->pace = 0;
    }
    ball_held[0] = 1;
    ball_age[0] = 0;
    if (fill)
        arkaled_fill();
}

static void clear_actors(void)
{
    int i;

    world_reset();
    pad_id[0] = -1;
    pad_id[1] = -1;
    for (i = 0; i < MAX_BALLS; i++) {
        ball_id[i] = -1;
        ball_held[i] = 0;
        ball_age[i] = 0;
    }
    for (i = 0; i < MAX_ITEMS; i++)
        item_id[i] = -1;
    blank_pieces();
}

static void start_round(void)
{
    miss_pause = 0;
    miss_blink_phase = 0;
    clear_actors();
    ready(1);
    draw();
}

static int last_ball_miss(int i, Entity *ball)
{
    if (arkaled_ball_count() != 1)
        return 0;
    ball->vx = 0;
    ball->vy = 0;
    ball_held[i] = 0;
    miss_pause = MISS_PAUSE;
    miss_blink_phase = 0;
    return 1;
}

void arkaled_reset(void)
{
    on_title = 1;
    pick = 1;
    solo = 0;
    miss_pause = 0;
    miss_blink_phase = 0;
    pad_score[0] = 0;
    pad_score[1] = 0;
    rand_reseed();
    clear_actors();
    name_scroll_begin();
    status_clear();
    paint_title();
}

int arkaled_title_scroll_x(void)
{
    return name_scroll.scroll_x;
}

void arkaled_begin_play(int players)
{
    on_title = 0;
    solo = players == 1;
    pad_score[0] = 0;
    pad_score[1] = 0;
    rand_reseed();
    start_round();
}

int arkaled_on_title(void)
{
    return on_title;
}

static void title_tick(const Input *in)
{
    int up = in->up_pressed || in->p2_up_pressed;
    int down = in->down_pressed || in->p2_down_pressed;

    if (up && !down)
        pick = 1;
    if (down && !up)
        pick = 2;
    if (in->button_pressed || in->p2_button_pressed) {
        arkaled_begin_play(pick);
        return;
    }
    paint_title();
}

void arkaled_load(int pad_x, int x, int y, int vx, int vy)
{
    Entity *ball;
    Entity *p2;

    on_title = 0;
    miss_pause = 0;
    miss_blink_phase = 0;
    ready(0);
    entity_get(pad_id[0])->x = pad_x;
    p2 = entity_get(pad_id[1]);
    if (p2 != NULL)
        p2->x = pad_x;
    ball = entity_get(ball_id[0]);
    ball->x = x;
    ball->y = y;
    ball->vx = vx;
    ball->vy = vy;
    ball_held[0] = 0;
    ball_age[0] = 0;
    ball->pace = arkaled_period(0) - 1;
    if (ball->pace < 0)
        ball->pace = 0;
    draw();
}

int arkaled_held_ball(int *x, int *y)
{
    int i;

    for (i = 0; i < MAX_BALLS; i++) {
        Entity *ball;

        if (ball_held[i] == 0)
            continue;
        ball = entity_get(ball_id[i]);
        if (ball == NULL)
            continue;
        if (x != NULL)
            *x = ball->x;
        if (y != NULL)
            *y = ball->y;
        return ball_held[i];
    }
    return 0;
}

void arkaled_place_pads(int x1, int x2)
{
    Entity *p1 = entity_get(pad_id[0]);
    Entity *p2 = entity_get(pad_id[1]);

    if (p1 != NULL)
        p1->x = x1;
    if (p2 != NULL)
        p2->x = x2;
    ppace[0] = 0;
    ppace[1] = 0;
    draw();
}

static int any_balls(void)
{
    return first_ball() >= 0;
}

static void collect_item(int i, Entity *pads[2], int x)
{
    int who = 2;

    if (pad_covers(pads[0], x))
        who = 1;
    else if (!pad_covers(pads[1], x))
        return;
    entity_kill(item_id[i]);
    item_id[i] = -1;
    spawn_held(who);
}

static void step_items(Entity *pads[2])
{
    int i;

    for (i = 0; i < MAX_ITEMS; i++) {
        Entity *it = entity_get(item_id[i]);

        if (it == NULL)
            continue;
        it->pace++;
        if (it->pace < ITEM_EVERY)
            continue;
        it->pace = 0;
        it->y++;
        if (it->y >= FIELD) {
            entity_kill(item_id[i]);
            item_id[i] = -1;
            continue;
        }
        if (it->y == FIELD - 1)
            collect_item(i, pads, it->x);
    }
}

void arkaled_tick(const Input *in)
{
    Entity *pads[2];
    int dir[2];
    int from_x[2];
    int moved[2];
    int i;
    int ended = 0;

    rand_feed(in);
    if (on_title) {
        title_tick(in);
        return;
    }
    if (miss_pause > 0) {
        miss_pause--;
        miss_blink_phase++;
        if (miss_pause == 0)
            start_round();
        else
            draw();
        return;
    }

    pads[0] = entity_get(pad_id[0]);
    pads[1] = entity_get(pad_id[1]);
    if (pads[0] == NULL || pads[1] == NULL)
        return;

    ticks++;
    dir[0] = 0;
    dir[1] = 0;
    from_x[0] = pads[0]->x;
    from_x[1] = pads[1]->x;
    moved[0] = 0;
    moved[1] = 0;
    if (solo) {
        int d = 0;
        int every = pad_every(in->button);
        int xmin[2];
        int xmax[2];

        if (in->left && !in->right)
            d = -1;
        if (in->right && !in->left)
            d = 1;
        xmin[0] = PAD1_MIN;
        xmin[1] = PAD2_MIN;
        xmax[0] = PAD1_MAX;
        xmax[1] = PAD2_MAX;
        if (d == 0) {
            ppace[0] = 0;
            ppace[1] = 0;
        } else {
            int first = d > 0 ? 0 : 1;
            int who = first;

            if (!pad_can_move(pads[first], d, xmin[first], xmax[first]))
                who = 1 - first;
            moved[who] = step_pad(pads[who], d, &ppace[0], every,
                                  xmin[who], xmax[who]);
        }
    } else {
        if (in->left && !in->right)
            dir[0] = -1;
        if (in->right && !in->left)
            dir[0] = 1;
        if (in->p2_left && !in->p2_right)
            dir[1] = -1;
        if (in->p2_right && !in->p2_left)
            dir[1] = 1;
        moved[0] = step_pad(pads[0], dir[0], &ppace[0],
                            pad_every(in->button),
                            PAD1_MIN, PAD1_MAX);
        moved[1] = step_pad(pads[1], dir[1], &ppace[1],
                            pad_every(in->p2_button),
                            PAD2_MIN, PAD2_MAX);
    }

    for (i = 0; i < MAX_BALLS; i++) {
        Entity *ball = entity_get(ball_id[i]);
        int who;
        int r;

        if (ball == NULL)
            continue;
        who = ball_held[i];
        if (who == 1 || who == 2) {
            if (!moved[who - 1])
                continue;
            ball_held[i] = 0;
            ball_age[i] = 0;
            ball->every = arkaled_period(0);
            ball->pace = 0;
            aim(ball);
            r = ball_step(pads, ball);
            if (r == 2) {
                ended = 1;
                break;
            }
            if (r == 1) {
                if (last_ball_miss(i, ball)) {
                    draw();
                    return;
                }
                kill_ball(i);
            }
            continue;
        }
        if (moved[0])
            carry(pads[0], ball, from_x[0]);
        if (moved[1])
            carry(pads[1], ball, from_x[1]);
        ball_age[i]++;
        ball->every = arkaled_period(ball_age[i]);
        ball->pace++;
        if (ball->pace < ball->every)
            continue;
        ball->pace = 0;
        r = ball_step(pads, ball);
        if (r == 2) {
            ended = 1;
            break;
        }
        if (r == 1) {
            if (last_ball_miss(i, ball)) {
                draw();
                return;
            }
            kill_ball(i);
        }
    }
    if (ended || !any_balls()) {
        start_round();
        return;
    }
    step_items(pads);
    draw();
}
