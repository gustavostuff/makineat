#ifndef SEVEN_SEG_FONT_H
#define SEVEN_SEG_FONT_H

#include <stdint.h>

/* Segment bits. A is bit 0 through G at bit 6, then the decimal point. */
#define SEVEN_SEG_A (1u << 0)
#define SEVEN_SEG_B (1u << 1)
#define SEVEN_SEG_C (1u << 2)
#define SEVEN_SEG_D (1u << 3)
#define SEVEN_SEG_E (1u << 4)
#define SEVEN_SEG_F (1u << 5)
#define SEVEN_SEG_G (1u << 6)
#define SEVEN_SEG_DP (1u << 7)

typedef struct SevenSegGlyph {
    char symbol;
    uint8_t mask;
} SevenSegGlyph;

/* Glyph list saved by tools/seven-seg-font. */
extern const SevenSegGlyph seven_seg_font[];
extern const int seven_seg_font_count;

/* Exact symbol first. A letter with no row uses the other case. Still missing is blank. */
uint8_t seven_seg_font_mask(char c);

/* Same glyph in MAX7219 raw order: D6=A ... D0=G, D7=DP. */
uint8_t seven_seg_font_max7219(char c);

#endif
