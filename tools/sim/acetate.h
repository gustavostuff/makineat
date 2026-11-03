#ifndef SIM_ACETATE_H
#define SIM_ACETATE_H

#include <SDL.h>
#include <stdint.h>

#define ACETATE_COLORS 8
#define ACETATE_CANVAS_W 128
#define ACETATE_CANVAS_H 192

void acetate_init(void);
void acetate_quit(void);
void acetate_set_window(SDL_Window *window);
void acetate_set_renderer(SDL_Renderer *renderer);

void acetate_set_field(int x, int y, int led, int pitch, int grid);
void acetate_set_digits(int x, int y, int digit_w, int digit_h, int n);

void acetate_set_cart(const char *cart_id);
void acetate_save(void);

void acetate_set_active_color(int index);
int acetate_active_color(void);

/* Color index at canvas pixel (0 = none). */
int acetate_color_at(int x, int y);

void acetate_draw_led_tint(SDL_Renderer *renderer, SDL_Texture *led_on, SDL_Texture *led_off,
                           int cell_x, int cell_y, int dst_x, int dst_y, int dst_w, int dst_h,
                           int led_on_state);

/* Texture blit with acetate color mod; only non-transparent texels show tint (no cell wash). */
void acetate_render_copy(SDL_Renderer *renderer, SDL_Texture *tex, const SDL_Rect *src,
                         const SDL_Rect *dst, int lit);

void acetate_draw_overlay(SDL_Renderer *renderer);

void acetate_notify_present(int win_w, int win_h, int scale, int dst_x, int dst_y);
int acetate_window_to_canvas(int wx, int wy, int *cx, int *cy);

void acetate_event(const SDL_Event *event, int *cursor_changed);
void acetate_drag_tick(void);
void acetate_apply_cursor(SDL_Renderer *renderer);

const char *acetate_color_name(int index);

#endif
