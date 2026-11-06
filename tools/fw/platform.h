#ifndef FW_PLATFORM_H
#define FW_PLATFORM_H

/*
 * Horizontal platformer player (one or two cells tall). Tie to StageView after stage_bind_h.
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
    int item_pickups;      /* fresh containers used during the latest tick */
    uint8_t grown;         /* 1: two cells tall, feet at py, head at py-1 */
    uint8_t crouch;        /* 1: grown player held down, or still under a low ceiling */
    uint8_t fall_on;       /* 1: free-fall wait clock is running */
    uint8_t fall_i;        /* index into plat_phys_fall_waits */
    uint8_t fall_timer;    /* ticks left before the next downward cell */
    uint8_t fall_terminal; /* 1: next free-fall tick continues at the fastest wait */
    uint8_t growth_anim;   /* ticks left in grow cutscene, 0 when not playing */
    uint8_t growth_anim_phase_ticks; /* countdown in current 0.2s visual phase */
    uint8_t growth_anim_show_two;    /* 0 = one cell on screen, 1 = two */
} PlatformPlayer;

void platform_player_spawn(PlatformPlayer *player, const StageMap *map);
void platform_player_bind(PlatformPlayer *player, StageView *view, const StageMap *map);
/* Returns how many fresh item containers were used this tick. */
int platform_player_tick(PlatformPlayer *player, StageView *view, const Input *in);

void platform_h_paint(const StageView *view, const PlatformPlayer *player);
/* 1 while the growth cutscene freezes the world (no sim ticks for items or motion). */
int platform_player_growth_freeze(const PlatformPlayer *player);

#if !defined(__AVR__)
int platform_player_x(const PlatformPlayer *player);
int platform_player_y(const PlatformPlayer *player);
int platform_player_vy(const PlatformPlayer *player);
#endif

#endif
