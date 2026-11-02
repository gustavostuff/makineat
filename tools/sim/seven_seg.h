#ifndef SEVEN_SEG_H
#define SEVEN_SEG_H

#include <SDL.h>
#include <stdint.h>

#define SEVEN_SEG_DIGITS 8
#define SEVEN_SEG_DIGIT_W 17
#define SEVEN_SEG_DIGIT_H 23
/* Abutting 1:1 blits; row is eight digit widths with no horizontal gap. */
#define SEVEN_SEG_DIGIT_PITCH SEVEN_SEG_DIGIT_W
#define SEVEN_SEG_ROW_W (SEVEN_SEG_DIGITS * SEVEN_SEG_DIGIT_W)
#define SEVEN_SEG_CENTER_X(canvas_w) (((canvas_w) - (SEVEN_SEG_ROW_W)) / 2)

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
    uint8_t mask[SEVEN_SEG_DIGITS];
} SevenSegDisplay;

int seven_seg_init(SevenSegDisplay *disp, SDL_Renderer *renderer);
void seven_seg_quit(SevenSegDisplay *disp);

void seven_seg_clear(SevenSegDisplay *disp);
void seven_seg_set_mask(SevenSegDisplay *disp, int digit, uint8_t mask);
uint8_t seven_seg_get_mask(const SevenSegDisplay *disp, int digit);

uint8_t seven_seg_encode_char(char c);
void seven_seg_put_char(SevenSegDisplay *disp, int digit, char c);

/* Writes up to SEVEN_SEG_DIGITS characters. '.' sets DP on the previous digit. */
int seven_seg_print(SevenSegDisplay *disp, const char *text);

void seven_seg_draw(const SevenSegDisplay *disp, SDL_Renderer *renderer, int x, int y,
                    int canvas_w, int canvas_h);

#endif
