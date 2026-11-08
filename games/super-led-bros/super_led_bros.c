#include "super_led_bros.h"

#include "frame.h"
#include "platform.h"
#include "stage.h"
#include "stage_01.h"
#include "stage_02.h"
#include "status.h"
#include "text.h"

#if SUPER_LED_BROS_DEATH_TICKS != STAGE_TICK_HZ * 3
#error "death wait must stay 3 seconds at the game tick rate"
#endif
#if SUPER_LED_BROS_OVER_TICKS != STAGE_TICK_HZ * 3
#error "game over hold must stay 3 seconds at the game tick rate"
#endif
#if SUPER_LED_BROS_CLEAR_TICKS != STAGE_TICK_HZ
#error "stage-clear freeze must stay 1 second at the game tick rate"
#endif

enum {
    PHASE_CARD = 0,
    PHASE_PLAY,
    PHASE_CLEAR,
    PHASE_DEAD,
    PHASE_OVER
};

static const StageMap *const stages[] = {
    &stage_01,
    &stage_02,
};

static StageView view;
static PlatformPlayer player;
static TextScroll over_scroll;
static char stage_title[12];
static int stage_index;
static int coins;
static int lives;
static int phase;
static int death_ticks;
static int over_ticks;
static int clear_ticks;

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

static const StageMap *current_stage(void)
{
    return stages[stage_index];
}

static void write_stage_title(int number)
{
    static const char prefix[] = "Stage ";
    int i;
    int tens;
    int ones;

    if (number < 1)
        number = 1;
    if (number > 99)
        number = 99;
    for (i = 0; prefix[i] != '\0'; i++)
        stage_title[i] = prefix[i];
    tens = number / 10;
    ones = number % 10;
    if (tens > 0)
        stage_title[i++] = (char)('0' + tens);
    stage_title[i++] = (char)('0' + ones);
    stage_title[i] = '\0';
}

static void stage_start(void)
{
    const StageMap *map = current_stage();

    stage_view_init(&view);
    stage_bind_h(&view, map);
#if !defined(__AVR__)
    /* Simulator leaves the background layer unpainted for now. */
    view.draw_bg = 0;
#endif
    platform_player_bind(&player, &view, map);
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

static void begin_card(void)
{
    phase = PHASE_CARD;
    write_stage_title(stage_index + 1);
    text_scroll_stop(&over_scroll);
    over_scroll.font = TEXT_FONT_LARGE;
    over_scroll.repeat = TEXT_SCROLL_BOUNCE;
    over_scroll.margin_left = 3;
    over_scroll.margin_right = 4;
    over_scroll.character_spacing = 0;
    text_scroll_start(&over_scroll, 0, (FIELD_SIZE - TEXT_LARGE_H) / 2, stage_title);
    clear();
    text_scroll_paint(&over_scroll);
    hud_show();
}

static void begin_clear(void)
{
    phase = PHASE_CLEAR;
    clear_ticks = SUPER_LED_BROS_CLEAR_TICKS;
    paint_play();
}

static void begin_death(void)
{
    phase = PHASE_DEAD;
    death_ticks = SUPER_LED_BROS_DEATH_TICKS;
    paint_stage_only();
}

static int body_on_goal(void)
{
    int h;
    int i;

    if (view.map == 0)
        return 0;
    h = (player.grown && !player.crouch) ? 2 : 1;
    for (i = 0; i < h; i++) {
        if (stage_map_goal(view.map, player.px, player.py - i))
            return 1;
    }
    return 0;
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
    stage_index = 0;
    death_ticks = 0;
    over_ticks = 0;
    clear_ticks = 0;
    text_scroll_stop(&over_scroll);
    status_clear();
    begin_card();
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
    player.py = view.map != 0 ? (int)view.map->height : 0;
    player.grounded = 0;
    player.jump_script = 0;
    player.fall_on = 0;
    player.fall_terminal = 0;
    player.vy = 0;
}

int super_led_bros_on_card(void)
{
    return phase == PHASE_CARD;
}

int super_led_bros_clearing(void)
{
    return phase == PHASE_CLEAR;
}

int super_led_bros_stage_number(void)
{
    return stage_index + 1;
}

void super_led_bros_test_place_on_goal(void)
{
    player.px = 250;
    player.py = 9;
    player.grounded = 1;
    player.jump_script = 0;
    player.fall_on = 0;
    player.fall_terminal = 0;
    player.vy = 0;
    player.grown = 0;
    player.crouch = 0;
    player.walk_accum = 0;
}
#endif

void super_led_bros_tick(const Input *in)
{
    int p;

    if (in == 0)
        return;
    if (phase == PHASE_CARD) {
        clear();
        print_scroll(&over_scroll);
        hud_show();
        if (text_scroll_take_far_done(&over_scroll)) {
            stage_start();
            phase = PHASE_PLAY;
            paint_play();
        }
        return;
    }
    if (phase == PHASE_CLEAR) {
        if (clear_ticks > 0)
            clear_ticks--;
        if (clear_ticks > 0) {
            paint_play();
            return;
        }
        stage_index++;
        if (stage_index >= (int)(sizeof stages / sizeof stages[0]))
            stage_index = 0;
        begin_card();
        return;
    }
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
    if (body_on_goal()) {
        begin_clear();
        return;
    }
    if (!platform_player_growth_freeze(&player))
        apply_pickups(p);
    paint_play();
}
