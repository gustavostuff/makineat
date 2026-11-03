#include "acetate.h"
#include "acetate_geom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define GRID ACETATE_GEOM_GRID
#define DIGITS ACETATE_GEOM_DIGITS

typedef enum {
    PAINT_NONE = 0,
    PAINT_SET,
    PAINT_CLEAR,
} PaintKind;

static const struct {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    const char *name;
} palette[ACETATE_COLORS] = {
    {255, 255, 255, "Clear"},
    {245, 55, 65, "Red"},
    {255, 140, 30, "Orange"},
    {255, 225, 45, "Yellow"},
    {60, 215, 85, "Green"},
    {40, 215, 245, "Cyan"},
    {55, 125, 255, "Blue"},
    {235, 70, 205, "Magenta"},
};

static uint8_t led_color[GRID][GRID];
static uint8_t digit_color[DIGITS];
static int active_color = 1;
static char cart_slug[64];
static int dirty;

static int field_x = 8;
static int field_y = 8;
static int field_led = 6;
static int field_pitch = 7;
static int field_grid = GRID;

static int digit_x;
static int digit_y;
static int digit_w;
static int digit_h;
static int digit_n;

static int present_scale = 1;
static int present_dx;
static int present_dy;

static PaintKind paint_kind;
static int start_x0;
static int start_y0;
static int start_x1;
static int start_y1;
static int cur_x0;
static int cur_y0;
static int cur_x1;
static int cur_y1;
static SDL_Window *event_window;
static SDL_Renderer *map_renderer;
static int cursor_hidden;

static int edit_mod_down(void)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    return keys[SDL_SCANCODE_LCTRL];
}

void acetate_set_field(int x, int y, int led, int pitch, int grid)
{
    field_x = x;
    field_y = y;
    field_led = led > 0 ? led : 6;
    field_pitch = pitch > 0 ? pitch : field_led;
    field_grid = grid > 0 ? grid : GRID;
    if (field_grid > GRID)
        field_grid = GRID;
}

void acetate_set_digits(int x, int y, int w, int h, int n)
{
    digit_x = x;
    digit_y = y;
    digit_w = w;
    digit_h = h;
    digit_n = n;
    if (digit_n > DIGITS)
        digit_n = DIGITS;
    if (digit_n < 0)
        digit_n = 0;
}

static void path_for_cart(char *out, size_t cap)
{
    const char *base = SDL_GetBasePath();
    const char *slug = cart_slug[0] != '\0' ? cart_slug : "default";

    if (base != NULL) {
        snprintf(out, cap, "%sacetate/%s.json", base, slug);
        SDL_free((void *)base);
    } else {
        snprintf(out, cap, "tools/sim/acetate/%s.json", slug);
    }
}

static void load_file(void)
{
    char path[512];
    FILE *f;
    char buf[8192];
    size_t n;
    int active;

    memset(led_color, 0, sizeof led_color);
    memset(digit_color, 0, sizeof digit_color);
    path_for_cart(path, sizeof path);
    f = fopen(path, "rb");
    if (f == NULL) {
        dirty = 0;
        return;
    }
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = '\0';
    if (acetate_geom_parse_layout(buf, led_color, digit_color, &active) == 0) {
        if (active > 0 && active < ACETATE_COLORS)
            active_color = active;
    } else {
        memset(led_color, 0, sizeof led_color);
        memset(digit_color, 0, sizeof digit_color);
    }
    dirty = 0;
}

static void save_file(void)
{
    char path[512];
    char dir[512];
    FILE *f;
    int x;
    int y;
    int i;
    char *slash;

    if (!dirty)
        return;
    path_for_cart(path, sizeof path);
    strncpy(dir, path, sizeof dir);
    dir[sizeof dir - 1] = '\0';
    slash = strrchr(dir, '/');
    if (slash != NULL) {
        *slash = '\0';
        mkdir(dir, 0755);
    }
    f = fopen(path, "w");
    if (f == NULL)
        return;
    fprintf(f, "{\"active\":%d,\"led\":\"", active_color);
    for (y = 0; y < GRID; y++) {
        for (x = 0; x < GRID; x++)
            fputc('0' + led_color[y][x], f);
    }
    fprintf(f, "\",\"digit\":\"");
    for (i = 0; i < DIGITS; i++)
        fputc('0' + digit_color[i], f);
    fprintf(f, "\"}\n");
    fclose(f);
    dirty = 0;
}

void acetate_set_window(SDL_Window *window)
{
    event_window = window;
}

void acetate_set_renderer(SDL_Renderer *renderer)
{
    map_renderer = renderer;
}

void acetate_init(void)
{
    event_window = NULL;
    cart_slug[0] = '\0';
    memset(led_color, 0, sizeof led_color);
    memset(digit_color, 0, sizeof digit_color);
    dirty = 0;
    paint_kind = PAINT_NONE;
    active_color = 1;
    cursor_hidden = 0;
}

void acetate_quit(void)
{
    acetate_save();
    if (cursor_hidden) {
        SDL_ShowCursor(SDL_ENABLE);
        cursor_hidden = 0;
    }
}

void acetate_set_cart(const char *cart_id)
{
    if (cart_id == NULL || cart_id[0] == '\0') {
        strncpy(cart_slug, "default", sizeof cart_slug);
    } else {
        strncpy(cart_slug, cart_id, sizeof cart_slug - 1);
        cart_slug[sizeof cart_slug - 1] = '\0';
    }
    load_file();
}

void acetate_save(void)
{
    save_file();
}

void acetate_set_active_color(int index)
{
    if (index >= 1 && index < ACETATE_COLORS) {
        active_color = index;
        dirty = 1;
    }
}

int acetate_active_color(void)
{
    return active_color;
}

int acetate_color_at(int x, int y)
{
    return acetate_geom_color_at(led_color, digit_color, field_x, field_y, field_led, field_pitch,
                                 field_grid, digit_x, digit_y, digit_w, digit_h, digit_n, x, y);
}

static void set_texture_acetate_mod(SDL_Texture *tex, int c, int lit)
{
    if (c <= 0 || c >= ACETATE_COLORS) {
        SDL_SetTextureColorMod(tex, 255, 255, 255);
        return;
    }
    if (lit) {
        SDL_SetTextureColorMod(tex, palette[c].r, palette[c].g, palette[c].b);
    } else {
        SDL_SetTextureColorMod(tex, (Uint8)(160 + palette[c].r / 3),
                               (Uint8)(160 + palette[c].g / 3), (Uint8)(160 + palette[c].b / 3));
    }
}

void acetate_render_copy(SDL_Renderer *renderer, SDL_Texture *tex, const SDL_Rect *src,
                         const SDL_Rect *dst, int lit)
{
    int cx;
    int cy;
    int c;

    if (tex == NULL || dst == NULL)
        return;
    cx = dst->x + dst->w / 2;
    cy = dst->y + dst->h / 2;
    c = acetate_color_at(cx, cy);
    set_texture_acetate_mod(tex, c, lit != 0);
    if (src != NULL)
        SDL_RenderCopy(renderer, tex, src, dst);
    else
        SDL_RenderCopy(renderer, tex, NULL, dst);
    SDL_SetTextureColorMod(tex, 255, 255, 255);
}

void acetate_draw_led_tint(SDL_Renderer *renderer, SDL_Texture *led_on, SDL_Texture *led_off,
                           int cell_x, int cell_y, int dst_x, int dst_y, int dst_w, int dst_h,
                           int on)
{
    SDL_Rect cell = {dst_x, dst_y, dst_w, dst_h};
    SDL_Texture *tex = on ? led_on : led_off;
    int c;

    if (cell_x >= 0 && cell_x < field_grid && cell_y >= 0 && cell_y < field_grid)
        c = led_color[cell_y][cell_x];
    else
        c = acetate_color_at(dst_x + dst_w / 2, dst_y + dst_h / 2);
    set_texture_acetate_mod(tex, c, on);
    SDL_RenderCopy(renderer, tex, NULL, &cell);
    SDL_SetTextureColorMod(tex, 255, 255, 255);
}

static void fill_tint(SDL_Renderer *renderer, int x, int y, int w, int h, int c)
{
    SDL_Rect r = {x, y, w, h};

    if (c <= 0 || c >= ACETATE_COLORS || w < 1 || h < 1)
        return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, palette[c].r, palette[c].g, palette[c].b, 36);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void acetate_notify_present(int w, int h, int scale, int dx, int dy)
{
    (void)w;
    (void)h;
    present_scale = scale > 0 ? scale : 1;
    present_dx = dx;
    present_dy = dy;
}

static int window_point_to_canvas(int wx, int wy, int *cx, int *cy, int clamp_to_canvas)
{
    int lx;
    int ly;
    int max_lx;
    int max_ly;
    int rx;
    int ry;
    int scale = present_scale > 0 ? present_scale : 1;

    rx = wx;
    ry = wy;
    if (map_renderer != NULL && event_window != NULL) {
        int ww;
        int wh;
        int ow;
        int oh;

        SDL_GetWindowSize(event_window, &ww, &wh);
        SDL_GetRendererOutputSize(map_renderer, &ow, &oh);
        if (ww > 0 && wh > 0) {
            rx = wx * ow / ww;
            ry = wy * oh / wh;
        }
    }
    lx = rx - present_dx;
    ly = ry - present_dy;
    max_lx = ACETATE_CANVAS_W * scale - 1;
    max_ly = ACETATE_CANVAS_H * scale - 1;
    if (!clamp_to_canvas) {
        if (lx < 0 || ly < 0 || lx > max_lx || ly > max_ly)
            return 0;
    } else {
        if (lx < 0)
            lx = 0;
        if (ly < 0)
            ly = 0;
        if (lx > max_lx)
            lx = max_lx;
        if (ly > max_ly)
            ly = max_ly;
    }
    *cx = lx / scale;
    *cy = ly / scale;
    if (*cx >= ACETATE_CANVAS_W)
        *cx = ACETATE_CANVAS_W - 1;
    if (*cy >= ACETATE_CANVAS_H)
        *cy = ACETATE_CANVAS_H - 1;
    if (*cx < 0)
        *cx = 0;
    if (*cy < 0)
        *cy = 0;
    return 1;
}

int acetate_window_to_canvas(int wx, int wy, int *cx, int *cy)
{
    return window_point_to_canvas(wx, wy, cx, cy, 0);
}

static int pointer_canvas_coords(int *cx, int *cy, int clamp)
{
    int mx;
    int my;

    SDL_GetMouseState(&mx, &my);
    return window_point_to_canvas(mx, my, cx, cy, clamp);
}

static void cell_box_at(int cx, int cy, int *x0, int *y0, int *x1, int *y1)
{
    int col;
    int row;
    int d;
    int x;
    int y;
    int w;
    int h;

    if (acetate_geom_led_at(field_x, field_y, field_led, field_pitch, field_grid, cx, cy, &col,
                            &row)) {
        acetate_geom_led_rect(field_x, field_y, field_led, field_pitch, field_grid, col, row, &x, &y,
                              &w, &h);
        *x0 = x;
        *y0 = y;
        *x1 = x + w - 1;
        *y1 = y + h - 1;
        return;
    }
    d = acetate_geom_digit_at(digit_x, digit_y, digit_w, digit_h, digit_n, cx, cy);
    if (d >= 0) {
        acetate_geom_digit_rect(digit_x, digit_y, digit_w, digit_h, d, &x, &y, &w, &h);
        *x0 = x;
        *y0 = y;
        *x1 = x + w - 1;
        *y1 = y + h - 1;
        return;
    }
    *x0 = cx;
    *y0 = cy;
    *x1 = cx;
    *y1 = cy;
}

static void paint_box(int *x, int *y, int *w, int *h)
{
    int left = start_x0 < cur_x0 ? start_x0 : cur_x0;
    int top = start_y0 < cur_y0 ? start_y0 : cur_y0;
    int right = start_x1 > cur_x1 ? start_x1 : cur_x1;
    int bottom = start_y1 > cur_y1 ? start_y1 : cur_y1;

    *x = left;
    *y = top;
    *w = right - left + 1;
    *h = bottom - top + 1;
}

static int snapped_paint_box(int *x, int *y, int *w, int *h)
{
    int rx;
    int ry;
    int rw;
    int rh;

    paint_box(&rx, &ry, &rw, &rh);
    if (acetate_geom_union_overlap(field_x, field_y, field_led, field_pitch, field_grid, digit_x,
                                   digit_y, digit_w, digit_h, digit_n, rx, ry, rw, rh, x, y, w, h))
        return 1;
    *x = rx;
    *y = ry;
    *w = rw;
    *h = rh;
    return 0;
}

static void set_paint_end(int cx, int cy)
{
    cell_box_at(cx, cy, &cur_x0, &cur_y0, &cur_x1, &cur_y1);
}

static void begin_paint(int cx, int cy, PaintKind kind)
{
    cell_box_at(cx, cy, &start_x0, &start_y0, &start_x1, &start_y1);
    cur_x0 = start_x0;
    cur_y0 = start_y0;
    cur_x1 = start_x1;
    cur_y1 = start_y1;
    paint_kind = kind;
    SDL_CaptureMouse(SDL_TRUE);
}

static void commit_paint(void)
{
    int x;
    int y;
    int w;
    int h;
    uint8_t color;

    if (paint_kind == PAINT_NONE)
        return;
    paint_box(&x, &y, &w, &h);
    color = paint_kind == PAINT_CLEAR ? 0 : (uint8_t)active_color;
    if (acetate_geom_fill_overlap(led_color, digit_color, field_x, field_y, field_led, field_pitch,
                                  field_grid, digit_x, digit_y, digit_w, digit_h, digit_n, x, y, w,
                                  h, color))
        dirty = 1;
}

static void cycle_color(int delta)
{
    int c = active_color + delta;

    while (c < 1)
        c += ACETATE_COLORS - 1;
    while (c >= ACETATE_COLORS)
        c -= ACETATE_COLORS - 1;
    acetate_set_active_color(c);
}

static void end_paint(int commit)
{
    if (paint_kind == PAINT_NONE)
        return;
    if (commit)
        commit_paint();
    paint_kind = PAINT_NONE;
    SDL_CaptureMouse(SDL_FALSE);
    acetate_save();
}

static void draw_rect_cursor(SDL_Renderer *renderer, int x, int y, int w, int h, int c, int solid)
{
    SDL_Rect r = {x, y, w, h};
    Uint8 cr = 220;
    Uint8 cg = 220;
    Uint8 cb = 220;

    if (w < 1 || h < 1)
        return;
    if (c > 0 && c < ACETATE_COLORS) {
        cr = palette[c].r;
        cg = palette[c].g;
        cb = palette[c].b;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (solid) {
        SDL_SetRenderDrawColor(renderer, cr, cg, cb, 255);
        SDL_RenderFillRect(renderer, &r);
    } else {
        SDL_SetRenderDrawColor(renderer, cr, cg, cb, 96);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, cr, cg, cb, 255);
        SDL_RenderDrawRect(renderer, &r);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static void draw_brush(SDL_Renderer *renderer)
{
    int cx;
    int cy;
    int x;
    int y;
    int w;
    int h;
    int c = active_color;
    int size = field_led > 0 ? field_led : 6;

    if (!edit_mod_down())
        return;
    if (c <= 0 || c >= ACETATE_COLORS)
        c = 1;
    if (paint_kind != PAINT_NONE) {
        snapped_paint_box(&x, &y, &w, &h);
        draw_rect_cursor(renderer, x, y, w, h, paint_kind == PAINT_CLEAR ? 0 : c, 0);
        return;
    }
    if (!pointer_canvas_coords(&cx, &cy, 0))
        return;
    cell_box_at(cx, cy, &x, &y, &w, &h);
    w = w - x + 1;
    h = h - y + 1;
    if (w == 1 && h == 1) {
        x = cx - size / 2;
        y = cy - size / 2;
        w = size;
        h = size;
        draw_rect_cursor(renderer, x, y, w, h, c, 1);
        return;
    }
    draw_rect_cursor(renderer, x, y, w, h, c, 0);
}

void acetate_draw_overlay(SDL_Renderer *renderer)
{
    int col;
    int row;
    int i;
    int x;
    int y;
    int w;
    int h;

    for (row = 0; row < field_grid; row++) {
        for (col = 0; col < field_grid; col++) {
            acetate_geom_led_rect(field_x, field_y, field_led, field_pitch, field_grid, col, row,
                                  &x, &y, &w, &h);
            fill_tint(renderer, x, y, w, h, led_color[row][col]);
        }
    }
    for (i = 0; i < digit_n; i++) {
        acetate_geom_digit_rect(digit_x, digit_y, digit_w, digit_h, i, &x, &y, &w, &h);
        fill_tint(renderer, x, y, w, h, digit_color[i]);
    }
    draw_brush(renderer);
}

void acetate_drag_tick(void)
{
    int cx;
    int cy;

    if (!edit_mod_down()) {
        end_paint(0);
        return;
    }
    if (paint_kind == PAINT_NONE)
        return;
    if (!pointer_canvas_coords(&cx, &cy, 1))
        return;
    set_paint_end(cx, cy);
}

void acetate_apply_cursor(SDL_Renderer *renderer)
{
    (void)renderer;
    if (edit_mod_down()) {
        if (!cursor_hidden) {
            SDL_ShowCursor(SDL_DISABLE);
            cursor_hidden = 1;
        }
        return;
    }
    if (cursor_hidden) {
        SDL_ShowCursor(SDL_ENABLE);
        cursor_hidden = 0;
    }
}

void acetate_event(const SDL_Event *event, int *cursor_changed)
{
    int cx;
    int cy;

    if (cursor_changed != NULL)
        *cursor_changed = 0;

    if (!edit_mod_down()) {
        if (event->type == SDL_MOUSEBUTTONUP)
            end_paint(0);
        return;
    }

    if (event->type == SDL_MOUSEWHEEL) {
        if (event->wheel.y == 0)
            return;
        cycle_color(event->wheel.y > 0 ? 1 : -1);
        return;
    }

    if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (!acetate_window_to_canvas(event->button.x, event->button.y, &cx, &cy))
            return;
        if (event->button.button == SDL_BUTTON_LEFT)
            begin_paint(cx, cy, PAINT_SET);
        else if (event->button.button == SDL_BUTTON_RIGHT)
            begin_paint(cx, cy, PAINT_CLEAR);
        return;
    }

    if (event->type == SDL_MOUSEMOTION) {
        if (paint_kind != PAINT_NONE) {
            if (!window_point_to_canvas(event->motion.x, event->motion.y, &cx, &cy, 1))
                return;
            set_paint_end(cx, cy);
        }
        if (cursor_changed != NULL)
            *cursor_changed = 1;
        return;
    }

    if (event->type == SDL_MOUSEBUTTONUP) {
        if (event->button.button == SDL_BUTTON_LEFT || event->button.button == SDL_BUTTON_RIGHT)
            end_paint(1);
    }
}

const char *acetate_color_name(int index)
{
    if (index < 0 || index >= ACETATE_COLORS)
        return "";
    return palette[index].name;
}
