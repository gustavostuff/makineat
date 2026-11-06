#ifndef FW_STAGE_H
#define FW_STAGE_H

/*
 * Platform stages: wide/tall bitmaps with a scrolling camera over the 16x16
 * field. PNG import (tools/fw/gen_stage.py): pure colors on fg (see docs/firmware.md
 * Stage section). Optional bg parallax maps scroll spans so both layers start and
 * end together: bg moves (bg_width - 16) / (fg_width - 16) per fg camera step.
 * Horizontal mode is implemented. Vertical
 * mode is stubbed (STAGE_ERR_UNSUPPORTED).
 */

#include "frame.h"

#include <stdint.h>

#define STAGE_TICK_HZ 60
#define STAGE_SCROLL_FPS 30

/* Horizontal camera dead zone in screen space (see stage_h_camera_follow_dead_zone). */
#define STAGE_H_DEAD_ZONE_X 5
#define STAGE_H_DEAD_ZONE_W 6

typedef enum {
    STAGE_SCROLL_H,
    STAGE_SCROLL_V,
} StageScrollMode;

#define STAGE_ERR_UNSUPPORTED (-1)

typedef struct StageMap {
    uint16_t width;
    uint16_t height;
    uint16_t row_words;
    uint16_t spawn_x;
    uint16_t spawn_y;
    const uint16_t *fg_lit;
    const uint16_t *fg_solid;
    uint16_t bg_width;
    uint16_t bg_row_words;
    const uint16_t *bg_lit;
} StageMap;

typedef struct StageView {
    StageScrollMode mode;
    const StageMap *map;
    int camera;
    int bg_camera;
    int scroll_accum;
    int dead_zone_x; /* screen column of left edge (0-based), full FIELD_SIZE height */
    int dead_zone_w; /* width in screen columns */
} StageView;

int stage_map_get(const StageMap *map, int x, int y);
int stage_map_lit(const StageMap *map, int x, int y);
int stage_map_solid(const StageMap *map, int x, int y);

void stage_view_init(StageView *view);

int stage_bind_h(StageView *view, const StageMap *map);
int stage_bind_v(StageView *view, const StageMap *map);

int stage_h_camera_max(const StageView *view);
int stage_h_bg_camera_max(const StageView *view);
int stage_h_camera_get(const StageView *view);
int stage_h_bg_camera_get(const StageView *view);
void stage_h_camera_set(StageView *view, int x);
void stage_h_camera_follow_dead_zone(StageView *view, int world_x);

/* dx is -1, 0, or +1 (world moves opposite stick). Steps at STAGE_SCROLL_FPS. */
void stage_h_scroll_tick(StageView *view, int dx);

void stage_h_paint(const StageView *view);
/* 1 when the player LED must stay off: green fg decor, or bg art with no solid fg. */
int stage_h_suppress_player_led(const StageView *view, int world_x, int world_y);
/*
 * Like stage_h_paint, but when hide_player_fg is non-zero the cell at world
 * (hide_world_x, hide_world_y) skips ON for green fg decor and for bg when fg is not solid
 * red. Foreground always wins over background for the ON bit in one pass so bg never stacks
 * under fg on the same LED. Red solid fg can still sit over bg without double brightness.
 */
void stage_h_paint_ex(const StageView *view, int hide_world_x, int hide_world_y, int hide_player_fg);
int stage_h_solid(const StageView *view, int world_x, int world_y);

int stage_v_camera_max(const StageView *view);
void stage_v_scroll_tick(StageView *view, int dy);
void stage_v_paint(const StageView *view);

#endif
