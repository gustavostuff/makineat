#include "game.h"

#include "entity.h"
#include "entity/bullet.h"
#include "entity/player.h"
#include "entity/rock.h"
#include "over.h"
#include "score.h"
#include "sound.h"
#include "status.h"

static int over;

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

static void status_sync_score(void)
{
    status_put4(0, score_get());
    status_put4(4, rock_score_get());
}

static void draw(void)
{
    status_sync_score();
    world_draw();
}

static void game_over_judge(void)
{
    over_start_shooter(score_get(), rock_score_get());
}

void game_reset(void)
{
    over = 0;
    over_clear();
    score_reset();
    status_clear();
    bullet_reset();
    rock_reset();
    world_reset();
    player_reset();
    status_sync_score();
    draw();
}

void game_tick(const Input *in)
{
    if (over) {
        if (over_tick(in))
            game_reset();
        return;
    }

    player_tick(in);
    bullet_tick();
    if (in->button_pressed &&
        bullet_fire(player_nose_x(), player_nose_y() - 1))
        sound(1);
    if (rock_tick()) {
        over = 1;
        game_over_judge();
        return;
    }
    draw();
}
