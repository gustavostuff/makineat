#ifndef FW_STAGE_H
#define FW_STAGE_H

/*
 * Platform stages: wide/tall bitmaps with a scrolling camera over the 16x16
 * field. PNG import (tools/fw/gen_stage.py): RGB (0,0,0) is empty; pure red
 * (255,0,0) is the player spawn (one pixel, not solid); any other pixel is
 * solid and drawn. Horizontal scrolling is implemented; vertical
 * mode is stubbed (STAGE_ERR_UNSUPPORTED).
 */

#include "frame.h"

#include <stdint.h>

#define STAGE_TICK_HZ 60
#define STAGE_SCROLL_FPS 30

typedef enum {
    STAGE_SCROLL_H,
    STAGE_SCROLL_V,
} StageScrollMode;

#define STAGE_ERR_UNSUPPORTED (-1)

typedef struct StageMap {
    uint8_t width;
    uint8_t height;
    uint8_t row_words;
    uint8_t spawn_x;
    uint8_t spawn_y;
    const uint16_t *rows;
} StageMap;

typedef struct StageView {
    StageScrollMode mode;
    const StageMap *map;
    int camera;
    int scroll_accum;
} StageView;

int stage_map_get(const StageMap *map, int x, int y);

void stage_view_init(StageView *view);

int stage_bind_h(StageView *view, const StageMap *map);
int stage_bind_v(StageView *view, const StageMap *map);

int stage_h_camera_max(const StageView *view);
int stage_h_camera_get(const StageView *view);
void stage_h_camera_set(StageView *view, int x);

/* dx is -1, 0, or +1 (world moves opposite stick). Steps at STAGE_SCROLL_FPS. */
void stage_h_scroll_tick(StageView *view, int dx);

void stage_h_paint(const StageView *view);
int stage_h_solid(const StageView *view, int world_x, int world_y);

int stage_v_camera_max(const StageView *view);
void stage_v_scroll_tick(StageView *view, int dy);
void stage_v_paint(const StageView *view);

#endif
