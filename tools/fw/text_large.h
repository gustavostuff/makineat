#ifndef FW_TEXT_LARGE_H
#define FW_TEXT_LARGE_H

/*
 * 5x6 glyphs from img/fonts/larger_5x6_font.png (row 0: 0-9 - . , :;
 * row 1: A-Z). Lowercase letters use the capital glyph. Space is blank.
 */

#include <stdint.h>

#define TEXT_LARGE_W 5
#define TEXT_LARGE_H 6

const uint8_t *text_large_glyph(char c);

#endif
