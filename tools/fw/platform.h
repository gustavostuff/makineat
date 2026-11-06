#ifndef FW_PLATFORM_H
#define FW_PLATFORM_H

/*
 * Horizontal platformer player (1x1 cell). Tie to StageView after stage_bind_h.
 * Walk and vertical motion at 15 FPS; jump on debounced button_pressed when grounded.
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
