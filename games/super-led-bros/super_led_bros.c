#include "super_led_bros.h"

#include "platform.h"
#include "stage.h"
#include "stage_01_test.h"

static StageView view;
static PlatformPlayer player;

void super_led_bros_reset(void)
{
    stage_view_init(&view);
    stage_bind_h(&view, &stage_01_test);
    platform_player_bind(&player, &view, &stage_01_test);
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
#endif

void super_led_bros_tick(const Input *in)
{
    platform_player_tick(&player, &view, in);
    platform_h_paint(&view, &player);
}
