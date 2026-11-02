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
static int celebrate;

static void status_sync_score(void)
{
    int s = score_get();

    if (s > 99)
        s = 99;
    status_set_digit(6, (uint8_t)(s / 10));
    status_set_digit(7, (uint8_t)(s % 10));
}

static void draw(void)
{
    if (!celebrate)
        status_sync_score();
    world_draw();
}

void game_reset(void)
{
    over = 0;
    celebrate = 0;
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
    if (celebrate) {
        over_paint();
        celebrate--;
        if (celebrate <= 0)
            game_reset();
        return;
    }
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
        over_start();
        return;
    }
    if (score_get() > 99) {
        celebrate = OVER_HOLD;
        over_celebrate();
        return;
    }
    draw();
}
