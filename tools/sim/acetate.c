#include "acetate.h"
#include "acetate_geom.h"
#include "../fw/text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef AcetateRect Island;

#define MAX_ISLANDS 128
#define HANDLE 5
#define ACETATE_MIN_ISLAND ACETATE_GEOM_MIN_ISLAND

typedef enum {
    HIT_NONE = 0,
    HIT_SWATCH,
    HIT_BODY,
    HIT_NW,
    HIT_N,
    HIT_NE,
    HIT_E,
    HIT_SE,
    HIT_S,
    HIT_SW,
    HIT_W,
} Hit;

typedef enum {
    DRAG_NONE = 0,
    DRAG_CREATE,
    DRAG_MOVE,
    DRAG_RESIZE,
} DragKind;

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

static Island islands[MAX_ISLANDS];
static int island_n;
static int selected = -1;
static int hover = -1;
static int active_color = 1;
static char cart_slug[64];
static int dirty;

static int win_w;
static int win_h;
static int present_scale = 1;
static int present_dx;
static int present_dy;

static DragKind drag_kind;
static Hit drag_hit;
static int drag_island;
static int drag_start_cx;
static int drag_start_cy;
static int drag_anchor_x;
static int drag_anchor_y;
static Island drag_snapshot;
static int drag_moved;
static int drag_deselect_on_release;

static SDL_Rect clock_hit;
static int clock_hit_active;
static SDL_Rect ctrl_stick_hit;
static SDL_Rect ctrl_button_hit;
static int ctrl_hits_active;
static int island_max_y = ACETATE_MAX_Y;
static SDL_Window *event_window;
static SDL_Renderer *map_renderer;

static Hit last_hit = HIT_NONE;
static SDL_Cursor *cur_default;
static SDL_Cursor *cur_hand;
static SDL_Cursor *cur_nwse;
static SDL_Cursor *cur_nesw;
static SDL_Cursor *cur_ns;
static SDL_Cursor *cur_ew;

static SDL_Texture *swatch_hint_tex;
static int swatch_hint_w;
static int swatch_hint_h;

static int swatch_x(void)
{
    return 2;
}

static int swatch_y(void)
{
    return ACETATE_CANVAS_H - ACETATE_SWATCH - 2;
}

static int in_swatch(int x, int y)
{
    return x >= swatch_x() && x < swatch_x() + ACETATE_SWATCH && y >= swatch_y() &&
           y < swatch_y() + ACETATE_SWATCH;
}

static int in_clock_hit(int x, int y)
{
    if (!clock_hit_active)
        return 0;
    return x >= clock_hit.x && x < clock_hit.x + clock_hit.w && y >= clock_hit.y &&
           y < clock_hit.y + clock_hit.h;
}

void acetate_set_clock_hit(int x, int y, int w, int h)
{
    clock_hit.x = x;
    clock_hit.y = y;
    clock_hit.w = w;
    clock_hit.h = h;
    clock_hit_active = w > 0 && h > 0;
}

void acetate_clear_clock_hit(void)
{
    clock_hit_active = 0;
}

void acetate_set_max_y(int max_y)
{
    if (max_y < 1)
        max_y = 1;
    if (max_y > ACETATE_CANVAS_H)
        max_y = ACETATE_CANVAS_H;
    island_max_y = max_y;
}

void acetate_set_control_hits(const SDL_Rect *stick, const SDL_Rect *button)
{
    if (stick != NULL) {
        ctrl_stick_hit = *stick;
        ctrl_hits_active = 1;
    }
    if (button != NULL)
        ctrl_button_hit = *button;
}

static int in_control_hit(int x, int y)
{
    if (!ctrl_hits_active)
        return 0;
    if (acetate_geom_point_in_rect(x, y, ctrl_stick_hit.x, ctrl_stick_hit.y, ctrl_stick_hit.w,
                                   ctrl_stick_hit.h))
        return 1;
    if (acetate_geom_point_in_rect(x, y, ctrl_button_hit.x, ctrl_button_hit.y, ctrl_button_hit.w,
                                   ctrl_button_hit.h))
        return 1;
    return 0;
}

static int point_in_island(const Island *is, int x, int y)
{
    return acetate_geom_point_in(is, x, y);
}

static Hit hit_island(const Island *is, int x, int y, int for_island_index)
{
    int left = is->x;
    int top = is->y;
    int right = is->x + is->w - 1;
    int bottom = is->y + is->h - 1;
    int on_l = x >= left - HANDLE / 2 && x <= left + HANDLE / 2;
    int on_r = x >= right - HANDLE / 2 && x <= right + HANDLE / 2;
    int on_t = y >= top - HANDLE / 2 && y <= top + HANDLE / 2;
    int on_b = y >= bottom - HANDLE / 2 && y <= bottom + HANDLE / 2;

    (void)for_island_index;
    if (on_t && on_l)
        return HIT_NW;
    if (on_t && on_r)
        return HIT_NE;
    if (on_b && on_l)
        return HIT_SW;
    if (on_b && on_r)
        return HIT_SE;
    if (on_t && x >= left && x <= right)
        return HIT_N;
    if (on_b && x >= left && x <= right)
        return HIT_S;
    if (on_l && y >= top && y <= bottom)
        return HIT_W;
    if (on_r && y >= top && y <= bottom)
        return HIT_E;
    if (point_in_island(is, x, y))
        return HIT_BODY;
    return HIT_NONE;
}

static int canvas_picks_islands(int y)
{
    /* Resize handles extend HANDLE/2 px past the island bottom. */
    return y <= island_max_y + HANDLE / 2;
}

static int topmost_at(int x, int y, Hit *out_hit)
{
    int i;

    if (!canvas_picks_islands(y)) {
        if (out_hit != NULL)
            *out_hit = HIT_NONE;
        return -1;
    }

    for (i = island_n - 1; i >= 0; i--) {
        Hit h = hit_island(&islands[i], x, y, i);

        if (h != HIT_NONE) {
            if (out_hit != NULL)
                *out_hit = h;
            return i;
        }
    }
    if (out_hit != NULL)
        *out_hit = HIT_NONE;
    return -1;
}

static int island_size_ok(const Island *is)
{
    return acetate_geom_size_ok(is, ACETATE_MIN_ISLAND);
}

static void normalize_rect(Island *is)
{
    acetate_geom_normalize(is);
}

static void clamp_island_move(Island *is)
{
    acetate_geom_clamp_move(is, ACETATE_CANVAS_W, island_max_y);
}

static void clamp_island_bounds(Island *is)
{
    acetate_geom_clamp_bounds(is, ACETATE_CANVAS_W, island_max_y);
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
    char *p;

    island_n = 0;
    selected = -1;
    path_for_cart(path, sizeof path);
    f = fopen(path, "rb");
    if (f == NULL)
        return;
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = '\0';

    p = strstr(buf, "\"active\"");
    if (p != NULL) {
        int v = atoi(p + 8);
        if (v >= 0 && v < ACETATE_COLORS)
            active_color = v;
    }
    for (p = buf; p != NULL && island_n < MAX_ISLANDS; p = strchr(p + 1, '{')) {
        {
            Island tmp;

            if (acetate_geom_parse_island(p, &tmp) != 0)
                continue;
            if (!island_size_ok(&tmp))
                continue;
            clamp_island_bounds(&tmp);
            if (!island_size_ok(&tmp))
                continue;
            islands[island_n] = tmp;
            island_n++;
        }
    }
    dirty = 0;
}

static void save_file(void)
{
    char path[512];
    char dir[512];
    FILE *f;
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
    fprintf(f, "{\"active\":%d,\"islands\":[", active_color);
    {
        int first = 1;

        for (i = 0; i < island_n; i++) {
            if (!island_size_ok(&islands[i]))
                continue;
            if (!first)
                fputc(',', f);
            first = 0;
            fprintf(f, "{\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d,\"c\":%d}", islands[i].x, islands[i].y,
                    islands[i].w, islands[i].h, (int)islands[i].color);
        }
    }
    fprintf(f, "]}\n");
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
    island_max_y = ACETATE_MAX_Y;
    event_window = NULL;
    cur_default = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    cur_hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    cur_nwse = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    cur_nesw = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    cur_ns = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    cur_ew = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    cart_slug[0] = '\0';
    island_n = 0;
    dirty = 0;
    drag_kind = DRAG_NONE;
}

void acetate_quit(void)
{
    acetate_save();
    if (swatch_hint_tex != NULL) {
        SDL_DestroyTexture(swatch_hint_tex);
        swatch_hint_tex = NULL;
    }
    SDL_FreeCursor(cur_default);
    SDL_FreeCursor(cur_hand);
    SDL_FreeCursor(cur_nwse);
    SDL_FreeCursor(cur_nesw);
    SDL_FreeCursor(cur_ns);
    SDL_FreeCursor(cur_ew);
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
    if (index >= 0 && index < ACETATE_COLORS) {
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
    return acetate_geom_color_at(islands, island_n, x, y);
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

    acetate_render_copy(renderer, tex, NULL, &cell, on);
    (void)cell_x;
    (void)cell_y;
}

static void draw_checker(SDL_Renderer *renderer, SDL_Rect r)
{
    for (int py = 0; py < r.h; py++) {
        for (int px = 0; px < r.w; px++) {
            if (((px / 2) + (py / 2)) % 2 == 0)
                SDL_SetRenderDrawColor(renderer, 205, 205, 210, 255);
            else
                SDL_SetRenderDrawColor(renderer, 95, 95, 105, 255);
            SDL_RenderDrawPoint(renderer, r.x + px, r.y + py);
        }
    }
}

static void draw_ants(SDL_Renderer *renderer, const Island *is, unsigned tick, int selected_flag)
{
    int x0 = is->x;
    int y0 = is->y;
    int x1 = is->x + is->w - 1;
    int y1 = is->y + is->h - 1;
    int phase = (int)(tick / 2) % 8;
    Uint8 r = selected_flag ? 255 : 180;
    Uint8 g = selected_flag ? 255 : 180;
    Uint8 b = selected_flag ? 255 : 180;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int x = x0; x <= x1; x++) {
        if (((x - x0 + phase) / 2) % 2 == 0) {
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, x, y0);
            SDL_RenderDrawPoint(renderer, x, y1);
        }
    }
    for (int y = y0; y <= y1; y++) {
        if (((y - y0 + phase) / 2) % 2 == 0) {
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, x0, y);
            SDL_RenderDrawPoint(renderer, x1, y);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void acetate_draw_overlay(SDL_Renderer *renderer, unsigned tick)
{
    int i;

    for (i = 0; i < island_n; i++) {
        const Island *is = &islands[i];
        SDL_Rect r = {is->x, is->y, is->w, is->h};

        if (is->color <= 0 || is->color >= ACETATE_COLORS)
            continue;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, palette[is->color].r, palette[is->color].g,
                               palette[is->color].b, 36);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    for (i = 0; i < island_n; i++) {
        if (i == hover || i == selected)
            draw_ants(renderer, &islands[i], tick, i == selected);
    }
}

static void blit_string(SDL_Surface *surface, int x0, int y0, const char *s)
{
    Uint32 *pixels = (Uint32 *)surface->pixels;
    int surf_w = surface->w;
    Uint32 fg = SDL_MapRGBA(surface->format, 180, 180, 185, 255);

    for (int i = 0; s[i] != '\0'; i++) {
        const uint8_t *glyph = text_glyph(s[i]);
        int base_x = x0 + i * TEXT_W;

        if (glyph == NULL)
            continue;
        for (int row = 0; row < TEXT_H; row++) {
            for (int col = 0; col < TEXT_W; col++) {
                if (((glyph[row] >> col) & 1) != 0)
                    pixels[(y0 + row) * surf_w + (base_x + col)] = fg;
            }
        }
    }
}

static void ensure_swatch_hint(SDL_Renderer *renderer)
{
    static const char *line0 = "click drag =";
    static const char *line1 = "acetate rectangle";
    SDL_Surface *surface;
    Uint32 *pixels;
    Uint32 trans;
    int w;
    int h;

    if (swatch_hint_tex != NULL)
        return;

    w = 17 * TEXT_W;
    h = 2 * TEXT_H + 1;
    surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL)
        return;
    pixels = (Uint32 *)surface->pixels;
    trans = SDL_MapRGBA(surface->format, 0, 0, 0, 0);
    for (int i = 0; i < w * h; i++)
        pixels[i] = trans;
    blit_string(surface, 0, 0, line0);
    blit_string(surface, 0, TEXT_H + 1, line1);
    swatch_hint_tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (swatch_hint_tex != NULL) {
        SDL_SetTextureScaleMode(swatch_hint_tex, SDL_ScaleModeNearest);
        swatch_hint_w = w;
        swatch_hint_h = h;
    }
    SDL_FreeSurface(surface);
}

void acetate_draw_swatch(SDL_Renderer *renderer)
{
    SDL_Rect r = {swatch_x(), swatch_y(), ACETATE_SWATCH, ACETATE_SWATCH};
    int c = active_color;

    if (c == 0)
        draw_checker(renderer, r);
    else {
        SDL_SetRenderDrawColor(renderer, palette[c].r, palette[c].g, palette[c].b, 255);
        SDL_RenderFillRect(renderer, &r);
    }
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &r);

    //ensure_swatch_hint(renderer);
    if (swatch_hint_tex != NULL) {
        SDL_Rect dst = {
            swatch_x() + ACETATE_SWATCH + 4,
            swatch_y(),
            swatch_hint_w,
            swatch_hint_h,
        };

        SDL_RenderCopy(renderer, swatch_hint_tex, NULL, &dst);
    }
}

void acetate_notify_present(int w, int h, int scale, int dx, int dy)
{
    win_w = w;
    win_h = h;
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

static void cycle_color(int delta)
{
    int c = active_color + delta;

    while (c < 0)
        c += ACETATE_COLORS;
    c %= ACETATE_COLORS;
    acetate_set_active_color(c);
}

static void cycle_island_color(int index, int delta)
{
    int c;

    if (index < 0 || index >= island_n)
        return;
    c = (int)islands[index].color + delta;
    while (c < 0)
        c += ACETATE_COLORS;
    c %= ACETATE_COLORS;
    islands[index].color = (uint8_t)c;
    dirty = 1;
}

static void start_capture(void)
{
    SDL_CaptureMouse(SDL_TRUE);
}

static void stop_capture(void)
{
    SDL_CaptureMouse(SDL_FALSE);
}

static void resize_island(Island *is, Hit hit, int cx, int cy);

static void try_commit_drag_island(const Island *trial)
{
    if (selected < 0 || selected >= island_n)
        return;
    islands[selected] = *trial;
    dirty = 1;
}

static void apply_drag(int cx, int cy)
{
    if (drag_kind == DRAG_CREATE && selected >= 0) {
        Island trial;

        trial.x = drag_start_cx;
        trial.y = drag_start_cy;
        trial.w = cx - drag_start_cx + 1;
        trial.h = cy - drag_start_cy + 1;
        trial.color = islands[selected].color;
        normalize_rect(&trial);
        clamp_island_bounds(&trial);
        try_commit_drag_island(&trial);
    } else if (drag_kind == DRAG_MOVE && selected >= 0) {
        Island trial = islands[selected];

        trial.x = cx - drag_anchor_x;
        trial.y = cy - drag_anchor_y;
        clamp_island_move(&trial);
        try_commit_drag_island(&trial);
    } else if (drag_kind == DRAG_RESIZE && selected >= 0) {
        Island trial = drag_snapshot;

        resize_island(&trial, drag_hit, cx, cy);
        try_commit_drag_island(&trial);
    }
}

static void delete_selected(void)
{
    if (selected < 0 || selected >= island_n)
        return;
    memmove(&islands[selected], &islands[selected + 1],
            (size_t)(island_n - selected - 1) * sizeof(Island));
    island_n--;
    if (selected >= island_n)
        selected = island_n - 1;
    dirty = 1;
}

static void resize_island(Island *is, Hit hit, int cx, int cy)
{
    int right = drag_snapshot.x + drag_snapshot.w - 1;
    int bottom = drag_snapshot.y + drag_snapshot.h - 1;

    switch (hit) {
    case HIT_NW:
        is->x = cx;
        is->y = cy;
        is->w = right - cx + 1;
        is->h = bottom - cy + 1;
        break;
    case HIT_N:
        is->y = cy;
        is->h = bottom - cy + 1;
        break;
    case HIT_NE:
        is->y = cy;
        is->w = cx - is->x + 1;
        is->h = bottom - cy + 1;
        break;
    case HIT_E:
        is->w = cx - is->x + 1;
        break;
    case HIT_SE:
        is->w = cx - is->x + 1;
        is->h = cy - is->y + 1;
        break;
    case HIT_S:
        is->h = cy - is->y + 1;
        break;
    case HIT_SW:
        is->x = cx;
        is->w = right - cx + 1;
        is->h = cy - is->y + 1;
        break;
    case HIT_W:
        is->x = cx;
        is->w = right - cx + 1;
        break;
    default:
        break;
    }
    normalize_rect(is);
    clamp_island_bounds(is);
}

static int pointer_canvas_coords(int *cx, int *cy)
{
    int mx;
    int my;

    /* Client coords; with capture this keeps updating outside the window. */
    (void)event_window;
    SDL_GetMouseState(&mx, &my);
    return window_point_to_canvas(mx, my, cx, cy, 1);
}

static int edit_mod_down(void)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    return keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
}

static void abort_drag(void)
{
    if (drag_kind == DRAG_NONE)
        return;
    if (drag_kind == DRAG_CREATE && selected >= 0)
        delete_selected();
    else if (selected >= 0 &&
             (drag_kind == DRAG_RESIZE || drag_kind == DRAG_MOVE))
        islands[selected] = drag_snapshot;
    drag_kind = DRAG_NONE;
    drag_deselect_on_release = 0;
    stop_capture();
}

void acetate_drag_tick(void)
{
    int cx;
    int cy;
    Uint32 buttons;

    if (!edit_mod_down()) {
        abort_drag();
        return;
    }
    if (drag_kind == DRAG_NONE)
        return;
    buttons = SDL_GetMouseState(NULL, NULL);
    if ((buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) == 0)
        return;
    if (!pointer_canvas_coords(&cx, &cy))
        return;
    if (cx != drag_start_cx || cy != drag_start_cy)
        drag_moved = 1;
    apply_drag(cx, cy);
}

void acetate_update_hover(int cx, int cy)
{
    Hit h;

    if (!edit_mod_down()) {
        hover = -1;
        last_hit = HIT_NONE;
        return;
    }
    if (drag_kind != DRAG_NONE)
        return;
    if (in_clock_hit(cx, cy) || in_control_hit(cx, cy)) {
        hover = -1;
        last_hit = HIT_NONE;
        return;
    }
    hover = topmost_at(cx, cy, &h);
    if (hover >= 0) {
        last_hit = h;
        return;
    }
    last_hit = HIT_NONE;
    if (in_swatch(cx, cy))
        last_hit = HIT_SWATCH;
}

void acetate_apply_cursor(SDL_Renderer *renderer)
{
    Hit h = HIT_NONE;

    (void)renderer;
    if (!edit_mod_down()) {
        SDL_SetCursor(cur_default);
        return;
    }
    if (drag_kind == DRAG_MOVE)
        h = HIT_BODY;
    else if (drag_kind == DRAG_RESIZE)
        h = drag_hit;
    else if (last_hit != HIT_SWATCH && last_hit != HIT_NONE)
        h = last_hit;

    if (h == HIT_BODY) {
        SDL_SetCursor(cur_hand);
        return;
    }
    if (h == HIT_NW || h == HIT_SE) {
        SDL_SetCursor(cur_nwse);
        return;
    }
    if (h == HIT_NE || h == HIT_SW) {
        SDL_SetCursor(cur_nesw);
        return;
    }
    if (h == HIT_N || h == HIT_S) {
        SDL_SetCursor(cur_ns);
        return;
    }
    if (h == HIT_E || h == HIT_W) {
        SDL_SetCursor(cur_ew);
        return;
    }
    SDL_SetCursor(cur_default);
}

void acetate_event(const SDL_Event *event, int *cursor_changed)
{
    int cx;
    int cy;

    if (cursor_changed != NULL)
        *cursor_changed = 0;

    if (!edit_mod_down()) {
        if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT &&
            drag_kind != DRAG_NONE)
            abort_drag();
        return;
    }

    if (event->type == SDL_KEYDOWN &&
        (event->key.keysym.scancode == SDL_SCANCODE_DELETE ||
         event->key.keysym.scancode == SDL_SCANCODE_BACKSPACE)) {
        delete_selected();
        return;
    }

    if (event->type == SDL_MOUSEWHEEL) {
        int mx;
        int my;
        int delta;
        int idx;

        if (event->wheel.y == 0)
            return;
        delta = event->wheel.y > 0 ? 1 : -1;
        SDL_GetMouseState(&mx, &my);
        if (!window_point_to_canvas(mx, my, &cx, &cy, 0))
            return;
        if (in_clock_hit(cx, cy) || in_control_hit(cx, cy))
            return;
        idx = hover >= 0 ? hover : selected;
        if (idx < 0)
            idx = topmost_at(cx, cy, NULL);
        if (idx >= 0) {
            cycle_island_color(idx, delta);
            return;
        }
        if (in_swatch(cx, cy))
            cycle_color(delta);
        return;
    }

    if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        if (!acetate_window_to_canvas(event->button.x, event->button.y, &cx, &cy))
            return;
        if (in_clock_hit(cx, cy) || in_control_hit(cx, cy))
            return;
        drag_start_cx = cx;
        drag_start_cy = cy;
        drag_moved = 0;
        drag_deselect_on_release = 0;
        Hit h;
        int idx = topmost_at(cx, cy, &h);

        if (idx >= 0 && h != HIT_BODY) {
            selected = idx;
            drag_kind = DRAG_RESIZE;
            drag_hit = h;
            drag_island = selected;
            drag_snapshot = islands[selected];
            dirty = 1;
            start_capture();
        } else if (idx >= 0) {
            if (idx == selected)
                drag_deselect_on_release = 1;
            else
                selected = idx;
            drag_kind = DRAG_MOVE;
            drag_island = selected;
            drag_snapshot = islands[selected];
            drag_anchor_x = cx - islands[selected].x;
            drag_anchor_y = cy - islands[selected].y;
            dirty = 1;
            start_capture();
        } else {
            selected = -1;
            dirty = 1;
            if (in_swatch(cx, cy)) {
                cycle_color(1);
            } else if (active_color != 0 && island_n < MAX_ISLANDS) {
                drag_kind = DRAG_CREATE;
                drag_island = island_n;
                islands[island_n].x = cx;
                islands[island_n].y = cy;
                islands[island_n].w = 1;
                islands[island_n].h = 1;
                islands[island_n].color = (uint8_t)active_color;
                island_n++;
                selected = drag_island;
                dirty = 1;
                start_capture();
            }
        }
        return;
    }

    if (event->type == SDL_MOUSEMOTION) {
        if (drag_kind != DRAG_NONE) {
            if (!window_point_to_canvas(event->motion.x, event->motion.y, &cx, &cy, 1))
                return;
        } else if (!window_point_to_canvas(event->motion.x, event->motion.y, &cx, &cy, 0))
            return;
        if (drag_kind != DRAG_NONE &&
            (cx != drag_start_cx || cy != drag_start_cy))
            drag_moved = 1;
        if (drag_kind == DRAG_NONE)
            acetate_update_hover(cx, cy);
        if (cursor_changed != NULL)
            *cursor_changed = 1;
        apply_drag(cx, cy);
        return;
    }

    if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT) {
        DragKind finished = drag_kind;

        if (finished == DRAG_MOVE && drag_deselect_on_release && !drag_moved) {
            selected = -1;
            dirty = 1;
        }
        drag_deselect_on_release = 0;
        if (finished == DRAG_CREATE && selected >= 0) {
            if (islands[selected].color == 0 || !island_size_ok(&islands[selected]))
                delete_selected();
        } else if (finished == DRAG_RESIZE && selected >= 0) {
            if (!island_size_ok(&islands[selected])) {
                islands[selected] = drag_snapshot;
                dirty = 1;
            }
        }
        drag_kind = DRAG_NONE;
        stop_capture();
        acetate_save();
    }
}

const char *acetate_color_name(int index)
{
    if (index < 0 || index >= ACETATE_COLORS)
        return "";
    return palette[index].name;
}
