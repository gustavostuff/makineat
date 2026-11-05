#ifndef FW_MAP_H
#define FW_MAP_H

#include "frame.h"

#include <stdint.h>

/*
 * Screen playfield occupancy, columns and rows 0..FIELD_LAST (see frame.h).
 * Each row is a 16-bit span mask. Favor map_hline, map_vline, and map_rect
 * for 1xN and Nx1 blocks. map_stamp_frame ORs every row into frame[] in one
 * pass (no per-pixel scan). Use for long bodies, walls, and static art.
 */

#define SCENE_MAP 1u
#define SCENE_ENTITIES 2u

void map_clear(void);
int map_get(int x, int y);
void map_set(int x, int y, int on);
void map_toggle(int x, int y);
void map_hline(int x, int y, int w, int on);
void map_vline(int x, int y, int h, int on);
void map_rect(int x, int y, int w, int h, int on);
void map_stamp_frame(void);

/* Non-zero when the cell is lit on the map. */
int map_solid(int x, int y);

/* 1 when (x,y) is blocked per mask (SCENE_MAP and/or SCENE_ENTITIES). */
int scene_blocked(int x, int y, unsigned mask);

#endif
