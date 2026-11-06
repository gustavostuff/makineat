#ifndef FW_PLATFORM_H
#define FW_PLATFORM_H

/*
 * Horizontal platformer player (1x1 cell). Tie to StageView after stage_bind_h.
 * Walk at 15 FPS. Jump schedule and fall behavior are under Platform in
 * docs/firmware.md. Tables in platform.c match that section.
 */

#include "input.h"
#include "stage.h"

typedef struct PlatformPlayer {
    int px;
    int py;
    int vy;
    int walk_accum;
    int vert_accum;
    int grounded;
    int jump_script;
    int jump_phase;        /* 0 = ascending, 1 = descending */
    int jump_script_timer; /* ticks left before next step; 60 Hz frames */
    int jump_up_i;         /* index into ascent waits (after initial step) */
    int jump_down_i;       /* index into descent waits */
    int jump_base_py;      /* ground row before jump (script uses rise from here) */
} PlatformPlayer;

void platform_player_spawn(PlatformPlayer *player, const StageMap *map);
void platform_player_bind(PlatformPlayer *player, StageView *view, const StageMap *map);
void platform_player_tick(PlatformPlayer *player, StageView *view, const Input *in);

void platform_h_paint(const StageView *view, const PlatformPlayer *player);

#if !defined(__AVR__)
int platform_player_x(const PlatformPlayer *player);
int platform_player_y(const PlatformPlayer *player);
int platform_player_vy(const PlatformPlayer *player);
#endif

#endif
