#include "board.h"
#include "cart.h"
#include "entity.h"
#include "flush.h"
#include "persist.h"
#include "sound.h"
#include "clocks.h"
#include "host.h"
#include "input.h"
#include "score.h"
#include "text.h"
#include "seven_seg.h"
#include "acetate.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <string.h>

#define GRID 16
#define LED 6 /* led_on/off.png native size; 1:1 blit into each cell */
#define FIELD_LED_MARGIN 1 /* px gap between adjacent LED sprites in the 16×16 grid */
#define LED_PITCH (LED + FIELD_LED_MARGIN)
#define FIELD_LED_PX ((GRID - 1) * LED_PITCH + LED)
#define FIELD_PX FIELD_LED_PX
#define CANVAS_W 128
#define CANVAS_H 192
#define CTRL_H 64
#define CTRL_Y (CANVAS_H - CTRL_H)
#define FIELD_X ((CANVAS_W - FIELD_PX) / 2)
#define FIELD_Y 8
#define SCALE 4
#define MARGIN 16
#define BG ((Uint8)(0.0 * 255)) // fully dark
#define VRAM_N 32
#define VRAM_CELL LED
#define VRAM_PX (VRAM_N * VRAM_CELL)
#define VRAM_GAP 12

static SDL_Texture *load_tex(SDL_Renderer *renderer, const char *file)
{
    char *base = SDL_GetBasePath();
    char path[1024];
    SDL_Surface *surface;
    SDL_Texture *tex;

    if (base != NULL) {
        snprintf(path, sizeof path, "%simg/%s", base, file);
        SDL_free(base);
        surface = IMG_Load(path);
    } else {
        surface = NULL;
    }
    if (surface == NULL) {
        snprintf(path, sizeof path, "tools/sim/img/%s", file);
        surface = IMG_Load(path);
    }
    if (surface == NULL) {
        snprintf(path, sizeof path, "img/%s", file);
        surface = IMG_Load(path);
    }
    if (surface == NULL) {
        SDL_Log("IMG_Load %s failed: %s", file, IMG_GetError());
        return NULL;
    }
    tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (tex == NULL)
        SDL_Log("texture %s failed: %s", file, SDL_GetError());
    return tex;
}

static void control_rects(int sx, int sy, SDL_Rect *stick, SDL_Rect *button)
{
    int travel = (sx != 0 || sy != 0) ? 6 : 8;
    int ctrl_rest = CTRL_Y + (CTRL_H - 32) / 2 + 8;

    stick->x = MARGIN + sx * travel;
    stick->y = ctrl_rest + sy * travel;
    stick->w = 32;
    stick->h = 32;
    button->x = 64 + MARGIN;
    button->y = ctrl_rest;
    button->w = 32;
    button->h = 32;
}

static void draw(SDL_Renderer *renderer, SDL_Texture *led_on, SDL_Texture *led_off,
                 SDL_Texture *stick_tex, SDL_Texture *button_tex,
                 SevenSegDisplay *status, const Board *board, int sx, int sy, int pressed,
                 unsigned ui_frame)
{
    int x;
    int y;
    SDL_Rect stick;
    SDL_Rect button;

    control_rects(sx, sy, &stick, &button);

    SDL_SetRenderDrawColor(renderer, BG, BG, BG, 255);
    SDL_RenderClear(renderer);

    for (y = 0; y < GRID; y++) {
        for (x = 0; x < GRID; x++) {
            SDL_Rect cell = {
                FIELD_X + x * LED_PITCH,
                FIELD_Y + y * LED_PITCH,
                LED,
                LED,
            };

            acetate_draw_led_tint(renderer, led_on, led_off, x, y, cell.x, cell.y, cell.w,
                                  cell.h, board_led_on(board, x, y));
        }
    }

    if (status != NULL) {
        const Max7219 *u7 = board_status_driver(board);

        if (u7 != NULL)
            seven_seg_sync_from_max7219(status, u7);
        seven_seg_draw_below_field(status, renderer, FIELD_X, FIELD_Y, FIELD_PX,
                                   SEVEN_SEG_GRID_MARGIN, CANVAS_W, CANVAS_H);
    }

    SDL_RenderCopy(renderer, stick_tex, NULL, &stick);
    if (pressed)
        SDL_SetTextureColorMod(button_tex, 150, 150, 150);
    else
        SDL_SetTextureColorMod(button_tex, 255, 255, 255);
    SDL_RenderCopy(renderer, button_tex, NULL, &button);

    acetate_draw_overlay(renderer, ui_frame);
    acetate_draw_swatch(renderer);
}

/* The canvas is 1:1 with the art. One integer scale of that whole picture
 * keeps every source pixel the same size. Per-sprite scaling does not. */
static void present_canvas(SDL_Renderer *renderer, SDL_Texture *canvas)
{
    int out_w;
    int out_h;
    int scale;
    int fit_h;
    SDL_Rect dst;

    SDL_SetRenderTarget(renderer, NULL);
    SDL_GetRendererOutputSize(renderer, &out_w, &out_h);
    scale = out_w / CANVAS_W;
    fit_h = out_h / CANVAS_H;
    if (fit_h < scale)
        scale = fit_h;
    if (scale < 1)
        scale = 1;
    dst.x = (out_w - CANVAS_W * scale) / 2;
    dst.y = (out_h - CANVAS_H * scale) / 2;
    dst.w = CANVAS_W * scale;
    dst.h = CANVAS_H * scale;
    acetate_notify_present(out_w, out_h, scale, dst.x, dst.y);
    SDL_SetRenderDrawColor(renderer, BG, BG, BG, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, canvas, NULL, &dst);
    SDL_RenderPresent(renderer);
}

/* The center 16 by 16 cells are the screen. The rest is entity margin in world space. */
static void draw_vram(SDL_Renderer *renderer, SDL_Texture *led_on, SDL_Texture *led_off)
{
    int col;
    int row;
    int origin = (VRAM_N - GRID) / 2;
    SDL_Rect screen = {
        origin * VRAM_CELL,
        origin * VRAM_CELL,
        GRID * VRAM_CELL,
        GRID * VRAM_CELL,
    };

    SDL_SetRenderDrawColor(renderer, BG, BG, BG, 255);
    SDL_RenderClear(renderer);
    for (row = 0; row < VRAM_N; row++) {
        for (col = 0; col < VRAM_N; col++) {
            int on = world_lit(col + ENTITY_LO, row + ENTITY_LO);
            SDL_Rect dot = {
                col * VRAM_CELL,
                row * VRAM_CELL,
                VRAM_CELL,
                VRAM_CELL,
            };

            SDL_RenderCopy(renderer, on ? led_on : led_off, NULL, &dot);
        }
    }
    SDL_SetRenderDrawColor(renderer, 102, 102, 102, 255);
    SDL_RenderDrawRect(renderer, &screen);
    SDL_RenderPresent(renderer);
}

static void place_vram(SDL_Window *parent, SDL_Window *vram)
{
    int x;
    int y;
    int w;
    int h;

    if (vram == NULL)
        return;
    SDL_GetWindowPosition(parent, &x, &y);
    SDL_GetWindowSize(parent, &w, &h);
    SDL_SetWindowPosition(vram, x + w + VRAM_GAP, y);
}

static SDL_Window *open_vram(SDL_Window *parent, SDL_Renderer **out)
{
    SDL_Window *vram;
    SDL_Renderer *renderer;

    vram = SDL_CreateWindow("VRAM",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            VRAM_PX, VRAM_PX,
                            SDL_WINDOW_UTILITY | SDL_WINDOW_SKIP_TASKBAR |
                            SDL_WINDOW_HIDDEN);
    if (vram == NULL) {
        SDL_Log("VRAM window failed: %s", SDL_GetError());
        return NULL;
    }
    SDL_SetWindowModalFor(vram, parent);
    place_vram(parent, vram);
    renderer = SDL_CreateRenderer(vram, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
        renderer = SDL_CreateRenderer(vram, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == NULL) {
        SDL_Log("VRAM renderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(vram);
        return NULL;
    }
    *out = renderer;
    return vram;
}

static TTF_Font *open_font(void)
{
    const char *rels[] = {
        "tools/assets/fonts/proggy-tiny.ttf",
        "../assets/fonts/proggy-tiny.ttf",
        NULL,
    };
    char *base = SDL_GetBasePath();
    char path[1024];
    int i;
    TTF_Font *font = NULL;

    if (base != NULL) {
        snprintf(path, sizeof path, "%s../assets/fonts/proggy-tiny.ttf", base);
        SDL_free(base);
        font = TTF_OpenFont(path, 16);
    }
    for (i = 0; font == NULL && rels[i] != NULL; i++)
        font = TTF_OpenFont(rels[i], 16);
    if (font == NULL) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
        return NULL;
    }
    TTF_SetFontHinting(font, TTF_HINTING_MONO);
    TTF_SetFontKerning(font, 0);
    return font;
}

static SDL_Texture *fps_text(SDL_Renderer *renderer, TTF_Font *font, int fps,
                             int *w, int *h)
{
    char buf[16];
    int len = snprintf(buf, sizeof buf, "%d FPS", fps);
    if (len <= 0)
        return NULL;

    (void)font;
    int surf_w = len * TEXT_W;
    int surf_h = TEXT_H;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, surf_w, surf_h, 32, SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL)
        return NULL;

    Uint32 *pixels = (Uint32 *)surface->pixels;
    Uint32 trans = SDL_MapRGBA(surface->format, 0, 0, 0, 0); // black is transparent
    Uint32 fg = SDL_MapRGBA(surface->format, 204, 204, 204, 255);

    for (int i = 0; i < surf_w * surf_h; i++)
        pixels[i] = trans;

    for (int i = 0; i < len; i++) {
        const uint8_t *glyph = text_glyph(buf[i]);
        if (!glyph)
            continue;
        int base_x = i * TEXT_W;
        for (int r = 0; r < TEXT_H; r++) {
            for (int c = 0; c < TEXT_W; c++) {
                if ((glyph[r] >> c) & 1) {
                    pixels[r * surf_w + (base_x + c)] = fg;
                }
            }
        }
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (tex != NULL) {
        *w = surf_w;
        *h = surf_h;
    }
    SDL_FreeSurface(surface);
    return tex;
}

/* Block until the next 1/TICK_HZ slot. Uses a fixed schedule so the UI and
 * game stay at 60 Hz without drifting fast like a per-frame "start + sleep". */
static void wait_next_frame(Uint64 *next_frame, Uint64 freq, Uint64 period)
{
    Uint64 now;

    if (*next_frame == 0) {
        *next_frame = SDL_GetPerformanceCounter();
        return;
    }
    *next_frame += period;
    for (;;) {
        now = SDL_GetPerformanceCounter();
        if ((Sint64)(now - *next_frame) >= 0)
            break;
        {
            double left = (double)(*next_frame - now) / (double)freq;

            if (left > 0.002) {
                Uint32 ms = (Uint32)((left - 0.001) * 1000.0);

                if (ms > 0)
                    SDL_Delay(ms);
            }
        }
    }
    for (;;) {
        now = SDL_GetPerformanceCounter();
        if ((Sint64)(now - *next_frame) >= 0)
            break;
    }
    now = SDL_GetPerformanceCounter();
    if ((Sint64)(now - *next_frame) > (Sint64)(period * 2))
        *next_frame = now;
}

static int open_cart(void)
{
    char *base;
    char path[1024];
    int rc;

    rc = cart_read_file("cart_placed.json");
    if (rc >= 0)
        return rc;
    base = SDL_GetBasePath();
    if (base == NULL)
        return 0;
    snprintf(path, sizeof path, "%s../../cart_placed.json", base);
    SDL_free(base);
    rc = cart_read_file(path);
    return rc > 0;
}

int main(void)
{
    static Board board;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Window *vram = NULL;
    SDL_Renderer *vram_renderer = NULL;
    SDL_Texture *vram_led_on = NULL;
    SDL_Texture *vram_led_off = NULL;
    SDL_Texture *led_on;
    SDL_Texture *led_off;
    SDL_Texture *stick_tex;
    SDL_Texture *button_tex;
    SevenSegDisplay status_disp;
    int running = 1;
    int loaded;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    /* The drivers latch the first picture before a window exists, so the
     * open does not flash an empty frame. An empty socket leaves them
     * in reset, which is a dark field. */
    loaded = open_cart();
    board_init(&board);
    sim_bind(&board);
    sound_bind(&board);
    input_reset();
    if (loaded) {
        persist_bind(cart_eeprom());
        if (cart_name() == NULL || strcmp(cart_name(), "space-shooter") != 0)
            score_load();
        cart_reset();
        drivers_init();
        drivers_flush();
        board_advance(&board, MAX7219_WAKE_NS);
        if (board_fault(&board) != 0) {
            SDL_Log("display bring-up saw a timing fault");
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    window = SDL_CreateWindow("MakiNeat simulator",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              CANVAS_W * SCALE,
                              CANVAS_H * SCALE,
                              SDL_WINDOW_HIDDEN);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1,
                                 SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (renderer == NULL)
        renderer = SDL_CreateRenderer(window, -1,
                                     SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    led_on = load_tex(renderer, "led_on.png");
    led_off = load_tex(renderer, "led_off.png");
    stick_tex = load_tex(renderer, "stick.png");
    button_tex = load_tex(renderer, "button.png");
    if (led_on == NULL || led_off == NULL || stick_tex == NULL || button_tex == NULL) {
        SDL_DestroyTexture(led_on);
        SDL_DestroyTexture(led_off);
        SDL_DestroyTexture(stick_tex);
        SDL_DestroyTexture(button_tex);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    if (seven_seg_init(&status_disp, renderer) != 0) {
        SDL_Log("7-segment layer load failed");
        seven_seg_quit(&status_disp);
        SDL_DestroyTexture(led_on);
        SDL_DestroyTexture(led_off);
        SDL_DestroyTexture(stick_tex);
        SDL_DestroyTexture(button_tex);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture *canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET,
                                            CANVAS_W, CANVAS_H);
    if (canvas == NULL) {
        SDL_Log("canvas texture failed: %s", SDL_GetError());
        SDL_DestroyTexture(led_on);
        SDL_DestroyTexture(led_off);
        SDL_DestroyTexture(stick_tex);
        SDL_DestroyTexture(button_tex);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureScaleMode(canvas, SDL_ScaleModeNearest);
    SDL_SetTextureScaleMode(led_on, SDL_ScaleModeNearest);
    SDL_SetTextureScaleMode(led_off, SDL_ScaleModeNearest);

    TTF_Font *font = open_font();

    SDL_SetRenderTarget(renderer, canvas);
    draw(renderer, led_on, led_off, stick_tex, button_tex, &status_disp, &board, 0, 0, 0, 0);
    present_canvas(renderer, canvas);
    vram = open_vram(window, &vram_renderer);
    if (vram != NULL) {
        vram_led_on = load_tex(vram_renderer, "led_on.png");
        vram_led_off = load_tex(vram_renderer, "led_off.png");
        if (vram_led_on != NULL)
            SDL_SetTextureScaleMode(vram_led_on, SDL_ScaleModeNearest);
        if (vram_led_off != NULL)
            SDL_SetTextureScaleMode(vram_led_off, SDL_ScaleModeNearest);
        draw_vram(vram_renderer, vram_led_on, vram_led_off);
    }
    SDL_ShowWindow(window);
    if (vram != NULL)
        SDL_ShowWindow(vram);
    SDL_RaiseWindow(window);
    present_canvas(renderer, canvas);
    place_vram(window, vram);

    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 frame_period = freq / TICK_HZ;
    Uint64 next_frame = 0;
    Uint64 last_frame = 0;
    unsigned ui_frame = 0;
    SDL_Texture *fps_tex = NULL;
    int fps_show = -1;
    int fps_w = 0;
    int fps_h = 0;

    acetate_init();
    acetate_set_window(window);
    acetate_set_renderer(renderer);
    if (cart_name() != NULL)
        acetate_set_cart(cart_name());
    else
        acetate_set_cart("default");
    seven_seg_set_layer_blit(acetate_render_copy);

    sound_open();
    sound_render();

    while (running) {
        SDL_Event event;
        const Uint8 *keys;
        int up;
        int down;
        int left;
        int right;
        Input in;
        int sx = 0;
        int sy = 0;
        int pressed;
        Uint64 now;

        wait_next_frame(&next_frame, freq, frame_period);
        now = SDL_GetPerformanceCounter();
        if (last_frame != 0) {
            double dt = (double)(now - last_frame) / (double)freq;
            int fps = (int)(1.0 / dt + 0.5);

            if (fps > 999)
                fps = 999;
            if (fps != fps_show) {
                SDL_Texture *next = fps_text(renderer, font, fps, &fps_w, &fps_h);

                if (next != NULL) {
                    SDL_SetTextureScaleMode(next, SDL_ScaleModeNearest);
                    SDL_DestroyTexture(fps_tex);
                    fps_tex = next;
                    fps_show = fps;
                    acetate_set_clock_hit(CANVAS_W - fps_w - 2, 2, fps_w, fps_h);
                }
            }
        }
        last_frame = now;
        ui_frame++;

        keys = SDL_GetKeyboardState(NULL);
        up = (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) ? 1 : 0;
        down = (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) ? 1 : 0;
        left = (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) ? 1 : 0;
        right = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) ? 1 : 0;
        sx = 0;
        sy = 0;
        if (up && !down)
            sy = -1;
        if (down && !up)
            sy = 1;
        if (left && !right)
            sx = -1;
        if (right && !left)
            sx = 1;
        {
            SDL_Rect stick_hit;
            SDL_Rect button_hit;

            control_rects(sx, sy, &stick_hit, &button_hit);
            acetate_set_control_hits(&stick_hit, &button_hit);
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE) {
                if (event.window.windowID == SDL_GetWindowID(window))
                    running = 0;
                else if (vram != NULL &&
                         event.window.windowID == SDL_GetWindowID(vram))
                    SDL_HideWindow(vram);
            }
            if (event.type == SDL_WINDOWEVENT &&
                event.window.windowID == SDL_GetWindowID(window) &&
                (event.window.event == SDL_WINDOWEVENT_MOVED ||
                 event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
                place_vram(window, vram);
            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                running = 0;
            {
                int cur = 0;
                acetate_event(&event, &cur);
            }
            if (event.type == SDL_MOUSEMOTION) {
                int cx;
                int cy;
                if (acetate_window_to_canvas(event.motion.x, event.motion.y, &cx, &cy))
                    acetate_update_hover(cx, cy);
            }
            if (event.type == SDL_KEYDOWN && !event.key.repeat &&
                event.key.keysym.scancode == SDL_SCANCODE_V &&
                (event.key.keysym.mod & (KMOD_CTRL | KMOD_ALT)) == 0 &&
                vram != NULL) {
                if (SDL_GetWindowFlags(vram) & SDL_WINDOW_SHOWN)
                    SDL_HideWindow(vram);
                else {
                    place_vram(window, vram);
                    SDL_ShowWindow(vram);
                    SDL_RaiseWindow(window);
                }
            }
            if (event.type == SDL_KEYDOWN &&
                (event.key.keysym.mod & KMOD_CTRL) &&
                !event.key.repeat) {
                int scale = 0;

                if (event.key.keysym.scancode == SDL_SCANCODE_1)
                    scale = 1;
                if (event.key.keysym.scancode == SDL_SCANCODE_2)
                    scale = 2;
                if (event.key.keysym.scancode == SDL_SCANCODE_3)
                    scale = 3;
                if (event.key.keysym.scancode == SDL_SCANCODE_4)
                    scale = 4;
                if (scale != 0) {
                    SDL_SetWindowSize(window, CANVAS_W * scale, CANVAS_H * scale);
                    place_vram(window, vram);
                }
            }
        }
        pressed = keys[SDL_SCANCODE_SPACE] ? 1 : 0;
        board_set_switches(&board, up, down, left, right, pressed);
        input_tick(board_sense(&board, AVR_PD1) == SIG_L,
                   board_sense(&board, AVR_PD2) == SIG_L,
                   board_sense(&board, AVR_PD3) == SIG_L,
                   board_sense(&board, AVR_PD4) == SIG_L,
                   board_sense(&board, AVR_PD5) == SIG_L,
                   &in);
        if (loaded) {
            cart_tick(&in);
            drivers_flush();
        }
        board_advance(&board, TICK_NS);
        acetate_drag_tick();

        SDL_SetRenderTarget(renderer, canvas);
        draw(renderer, led_on, led_off, stick_tex, button_tex, &status_disp, &board,
             sx, sy, pressed, ui_frame);
        if (fps_tex != NULL) {
            SDL_Rect dst = {
                CANVAS_W - fps_w - 2,
                2,
                fps_w,
                fps_h,
            };

            SDL_RenderCopy(renderer, fps_tex, NULL, &dst);
        }
        present_canvas(renderer, canvas);
        acetate_apply_cursor(renderer);
        if (vram != NULL && (SDL_GetWindowFlags(vram) & SDL_WINDOW_SHOWN))
            draw_vram(vram_renderer, vram_led_on, vram_led_off);
        sound_render();
    }

    sound_close();
    acetate_quit();

    SDL_DestroyTexture(fps_tex);
    if (vram_led_on != NULL)
        SDL_DestroyTexture(vram_led_on);
    if (vram_led_off != NULL)
        SDL_DestroyTexture(vram_led_off);
    if (vram_renderer != NULL)
        SDL_DestroyRenderer(vram_renderer);
    if (vram != NULL)
        SDL_DestroyWindow(vram);
    SDL_DestroyTexture(canvas);
    seven_seg_quit(&status_disp);
    SDL_DestroyTexture(led_on);
    SDL_DestroyTexture(led_off);
    SDL_DestroyTexture(stick_tex);
    SDL_DestroyTexture(button_tex);
    if (font != NULL)
        TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
