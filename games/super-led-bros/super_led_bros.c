#include "super_led_bros.h"

#include "frame.h"
#include "platform.h"
#include "stage.h"
#include "stage_01.h"
#include "status.h"
#include "text.h"

#if SUPER_LED_BROS_DEATH_TICKS != STAGE_TICK_HZ * 3
#error "death wait must stay 3 seconds at the game tick rate"
#endif
#if SUPER_LED_BROS_OVER_TICKS != STAGE_TICK_HZ * 3
#error "game over hold must stay 3 seconds at the game tick rate"
#endif

enum {
    PHASE_PLAY = 0,
    PHASE_DEAD,
    PHASE_OVER
};

static StageView view;
static PlatformPlayer player;
static TextScroll over_scroll;
static int coins;
static int lives;
static int phase;
static int death_ticks;
static int over_ticks;

static void apply_pickups(int n)
{
    if (n <= 0)
        return;
    coins += n;
    if (coins >= 100)
        coins %= 100;
}

static void hud_show(void)
{
    int n;

    n = lives;
    if (n < 0)
        n = 0;
    if (n > 99)
        n = 99;
    status_set_digit(0, 0);
    status_set_digit(1, 0);
    status_set_digit(2, 0);
    status_set_digit(3, 0);
    status_set_digit(4, (uint8_t)(coins / 10));
    status_set_digit(5, (uint8_t)(coins % 10));
    status_set_digit(6, (uint8_t)(n / 10));
    status_set_digit(7, (uint8_t)(n % 10));
}

static void stage_start(void)
{
    stage_view_init(&view);
    stage_bind_h(&view, &stage_01);
    platform_player_bind(&player, &view, &stage_01);
}

static void paint_play(void)
{
    hud_show();
    platform_h_paint(&view, &player);
}

static void paint_stage_only(void)
{
    hud_show();
    stage_h_paint(&view);
}

static void game_over_begin(void)
{
    phase = PHASE_OVER;
    over_ticks = SUPER_LED_BROS_OVER_TICKS;
    text_scroll_stop(&over_scroll);
    over_scroll.font = TEXT_FONT_LARGE;
    over_scroll.repeat = TEXT_SCROLL_BOUNCE;
    over_scroll.margin_left = 4;
    over_scroll.margin_right = 4;
    text_scroll_start(&over_scroll, 0, (FIELD_SIZE - TEXT_LARGE_H) / 2, "game over");
    clear();
    text_scroll_paint(&over_scroll);
    hud_show();
}

static void begin_death(void)
{
    phase = PHASE_DEAD;
    death_ticks = SUPER_LED_BROS_DEATH_TICKS;
    paint_stage_only();
}

static int feet_below_stage(void)
{
    if (view.map == 0)
        return 0;
    return platform_player_y(&player) >= (int)view.map->height;
}

void super_led_bros_reset(void)
{
    coins = 0;
    lives = SUPER_LED_BROS_LIVES;
    phase = PHASE_PLAY;
    death_ticks = 0;
    over_ticks = 0;
    text_scroll_stop(&over_scroll);
    status_clear();
    stage_start();
    paint_play();
}

#if !defined(__AVR__)
int super_led_bros_camera_x(void)
{
    return stage_h_camera_get(&view);
}

int super_led_bros_player_x(void)
{
    return platform_player_x(&player);
}

int super_led_bros_player_y(void)
{
    return platform_player_y(&player);
}

int super_led_bros_coins(void)
{
    return coins;
}

void super_led_bros_apply_pickups(int n)
{
    apply_pickups(n);
    hud_show();
}

void super_led_bros_test_place_below(void)
{
    player.py = (int)stage_01.height;
    player.grounded = 0;
    player.jump_script = 0;
    player.fall_on = 0;
    player.fall_terminal = 0;
    player.vy = 0;
}
#endif

void super_led_bros_tick(const Input *in)
{
    int p;

    if (in == 0)
        return;
    if (phase == PHASE_OVER) {
        clear();
        print_scroll(&over_scroll);
        hud_show();
        if (over_ticks > 0) {
            over_ticks--;
            return;
        }
        if (in->button_pressed)
            super_led_bros_reset();
        return;
    }
    if (phase == PHASE_DEAD) {
        if (death_ticks > 0)
            death_ticks--;
        if (death_ticks > 0) {
            paint_stage_only();
            return;
        }
        if (lives > 0) {
            lives--;
            phase = PHASE_PLAY;
            stage_start();
            paint_play();
        } else
            game_over_begin();
        return;
    }
    p = platform_player_tick(&player, &view, in);
    if (feet_below_stage()) {
        begin_death();
        return;
    }
    if (!platform_player_growth_freeze(&player))
        apply_pickups(p);
    paint_play();
}
