#ifndef SEVEN_SEG_H
#define SEVEN_SEG_H

#include <SDL.h>
#include <stdint.h>

struct Max7219;
typedef struct Max7219 Max7219;

#define SEVEN_SEG_DIGITS 8
/* Gap between the 16×16 LED matrix edge and the digit row (horizontal inset / vertical). */
#define SEVEN_SEG_GRID_MARGIN 4

/* Segment bits (A through G, then decimal point). */
#define SEVEN_SEG_A (1u << 0)
#define SEVEN_SEG_B (1u << 1)
#define SEVEN_SEG_C (1u << 2)
#define SEVEN_SEG_D (1u << 3)
#define SEVEN_SEG_E (1u << 4)
#define SEVEN_SEG_F (1u << 5)
#define SEVEN_SEG_G (1u << 6)
#define SEVEN_SEG_DP (1u << 7)

typedef struct SevenSegDisplay {
    SDL_Texture *base;
    SDL_Texture *seg[7];
    SDL_Texture *dp;
    int digit_w;
    int digit_h;
    uint8_t mask[SEVEN_SEG_DIGITS];
} SevenSegDisplay;

int seven_seg_init(SevenSegDisplay *disp, SDL_Renderer *renderer);
void seven_seg_quit(SevenSegDisplay *disp);

void seven_seg_clear(SevenSegDisplay *disp);
void seven_seg_set_mask(SevenSegDisplay *disp, int digit, uint8_t mask);
uint8_t seven_seg_get_mask(const SevenSegDisplay *disp, int digit);

uint8_t seven_seg_encode_char(char c);
uint8_t seven_seg_mask_from_max7219(uint8_t segment_byte);

void seven_seg_sync_from_max7219(SevenSegDisplay *disp, const Max7219 *chip);
void seven_seg_sync_from_status_frame(SevenSegDisplay *disp, const uint8_t *frame, int n);
void seven_seg_put_char(SevenSegDisplay *disp, int digit, char c);

/* Writes up to SEVEN_SEG_DIGITS characters. '.' sets DP on the previous digit. */
int seven_seg_print(SevenSegDisplay *disp, const char *text);

int seven_seg_row_width(const SevenSegDisplay *disp);

void seven_seg_draw(const SevenSegDisplay *disp, SDL_Renderer *renderer, int x, int y,
                    int canvas_w, int canvas_h);

/* Abutting digits, centered on the matrix; grid_margin px from the 16×16 field. */
void seven_seg_draw_below_field(const SevenSegDisplay *disp, SDL_Renderer *renderer,
                                int field_x, int field_y, int field_px, int grid_margin,
                                int canvas_w, int canvas_h);

#endif
