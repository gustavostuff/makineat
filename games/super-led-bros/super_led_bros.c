#include "super_led_bros.h"

#include "platform.h"
#include "stage.h"
#include "stage_01.h"
#include "status.h"

static StageView view;
static PlatformPlayer player;
static int coins;

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
    status_set_digit(0, 0);
    status_set_digit(1, 0);
    status_set_digit(2, 0);
    status_set_digit(3, 0);
    status_set_digit(4, (uint8_t)(coins / 10));
    status_set_digit(5, (uint8_t)(coins % 10));
    status_set_digit(6, 0);
    status_set_digit(7, 0);
}

void super_led_bros_reset(void)
{
    coins = 0;
    status_clear();
    hud_show();
    stage_view_init(&view);
    stage_bind_h(&view, &stage_01);
    platform_player_bind(&player, &view, &stage_01);
    platform_h_paint(&view, &player);
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
#endif

void super_led_bros_tick(const Input *in)
{
    int p;

    p = platform_player_tick(&player, &view, in);
    if (!platform_player_growth_freeze(&player))
        apply_pickups(p);
    hud_show();
    platform_h_paint(&view, &player);
}
