#ifndef ACETATE_GEOM_H
#define ACETATE_GEOM_H

#include <stdint.h>

#define ACETATE_GEOM_MIN_ISLAND 4

typedef struct AcetateRect {
    int x;
    int y;
    int w;
    int h;
    uint8_t color;
} AcetateRect;

void acetate_geom_normalize(AcetateRect *r);
void acetate_geom_clamp_move(AcetateRect *r, int canvas_w, int max_y);
void acetate_geom_clamp_bounds(AcetateRect *r, int canvas_w, int max_y);
int acetate_geom_size_ok(const AcetateRect *r, int min_size);

int acetate_geom_point_in(const AcetateRect *r, int x, int y);
int acetate_geom_point_in_rect(int x, int y, int rx, int ry, int rw, int rh);

/* Last array entry is topmost; color 0 is transparent. */
int acetate_geom_color_at(const AcetateRect *list, int n, int x, int y);

void acetate_geom_send_to_front(AcetateRect *list, int n, int index, int *selected, int *hover);

int acetate_geom_parse_island(const char *json_obj, AcetateRect *out);

#endif
