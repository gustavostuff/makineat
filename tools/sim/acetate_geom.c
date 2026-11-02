#include "acetate_geom.h"

#include <stdio.h>
#include <string.h>

void acetate_geom_normalize(AcetateRect *r)
{
    if (r->w < 0) {
        r->x += r->w;
        r->w = -r->w;
    }
    if (r->h < 0) {
        r->y += r->h;
        r->h = -r->h;
    }
    if (r->w < 1)
        r->w = 1;
    if (r->h < 1)
        r->h = 1;
}

void acetate_geom_clamp_move(AcetateRect *r, int canvas_w, int max_y)
{
    int w = r->w;
    int h = r->h;

    if (w > canvas_w)
        w = canvas_w;
    if (h > max_y)
        h = max_y;
    r->w = w;
    r->h = h;
    if (r->x < 0)
        r->x = 0;
    if (r->y < 0)
        r->y = 0;
    if (r->x + r->w > canvas_w)
        r->x = canvas_w - r->w;
    if (r->y + r->h > max_y)
        r->y = max_y - r->h;
}

void acetate_geom_clamp_bounds(AcetateRect *r, int canvas_w, int max_y)
{
    if (r->x < 0) {
        r->w += r->x;
        r->x = 0;
    }
    if (r->y < 0) {
        r->h += r->y;
        r->y = 0;
    }
    if (r->x + r->w > canvas_w)
        r->w = canvas_w - r->x;
    if (r->y + r->h > max_y)
        r->h = max_y - r->y;
    if (r->w < 1)
        r->w = 1;
    if (r->h < 1)
        r->h = 1;
}

int acetate_geom_size_ok(const AcetateRect *r, int min_size)
{
    return r->w >= min_size && r->h >= min_size;
}

int acetate_geom_point_in(const AcetateRect *r, int x, int y)
{
    return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

int acetate_geom_point_in_rect(int x, int y, int rx, int ry, int rw, int rh)
{
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

int acetate_geom_color_at(const AcetateRect *list, int n, int x, int y)
{
    int i;

    for (i = n - 1; i >= 0; i--) {
        if (list[i].color == 0)
            continue;
        if (acetate_geom_point_in(&list[i], x, y))
            return (int)list[i].color;
    }
    return 0;
}

void acetate_geom_send_to_front(AcetateRect *list, int n, int index, int *selected, int *hover)
{
    AcetateRect tmp;

    if (index < 0 || index >= n || index == n - 1)
        return;
    tmp = list[index];
    memmove(&list[index], &list[index + 1], (size_t)(n - index - 1) * sizeof(AcetateRect));
    list[n - 1] = tmp;
    if (selected != NULL && *selected == index)
        *selected = n - 1;
    if (hover != NULL && *hover == index)
        *hover = n - 1;
}

int acetate_geom_parse_island(const char *json_obj, AcetateRect *out)
{
    int x;
    int y;
    int w;
    int h;
    int c;

    if (out == NULL || json_obj == NULL)
        return -1;
    if (sscanf(json_obj, "{\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d,\"c\":%d}", &x, &y, &w, &h, &c) != 5)
        return -1;
    out->x = x;
    out->y = y;
    out->w = w;
    out->h = h;
    out->color = (uint8_t)c;
    return 0;
}
