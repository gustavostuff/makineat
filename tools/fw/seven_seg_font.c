/* seven-seg font. Saved by tools/seven-seg-font. */
#include "seven_seg_font.h"

const SevenSegGlyph seven_seg_font[] = {
    { '0', 0x3F },
    { '1', 0x06 },
    { '2', 0x5B },
    { '3', 0x4F },
    { '4', 0x66 },
    { '5', 0x6D },
    { '6', 0x7D },
    { '7', 0x07 },
    { '8', 0x7F },
    { '9', 0x6F },
    { 'A', 0x77 },
    { 'b', 0x7C },
    { 'C', 0x39 },
    { 'D', 0x5E },
    { 'E', 0x79 },
    { 'f', 0x71 },
    { 'g', 0x3D },
    { 'h', 0x76 },
    { 'i', 0x06 },
    { 'J', 0x0E },
    { 'l', 0x38 },
    { 'n', 0x37 },
    { 'o', 0x3F },
    { 'p', 0x73 },
    { 'q', 0x67 },
    { 's', 0x6D },
    { 'u', 0x3E },
    { 'z', 0x5B },
    { '-', 0x40 },
    { '.', 0x80 },
};

const int seven_seg_font_count = 30;

static int seven_seg_font_find(char c, uint8_t *mask)
{
    int i;

    if (c == '\0')
        return 0;
    for (i = 0; i < seven_seg_font_count; i++) {
        if (seven_seg_font[i].symbol == c) {
            *mask = seven_seg_font[i].mask;
            return 1;
        }
    }
    return 0;
}

uint8_t seven_seg_font_mask(char c)
{
    uint8_t mask = 0;
    char other = 0;

    if (seven_seg_font_find(c, &mask))
        return mask;
    if (c >= 'a' && c <= 'z')
        other = (char)(c - 'a' + 'A');
    else if (c >= 'A' && c <= 'Z')
        other = (char)(c - 'A' + 'a');
    if (other != 0 && seven_seg_font_find(other, &mask))
        return mask;
    return 0;
}

uint8_t seven_seg_font_max7219(char c)
{
    uint8_t mask = seven_seg_font_mask(c);
    uint8_t out = 0;

    if ((mask & SEVEN_SEG_A) != 0)
        out |= 0x40;
    if ((mask & SEVEN_SEG_B) != 0)
        out |= 0x20;
    if ((mask & SEVEN_SEG_C) != 0)
        out |= 0x10;
    if ((mask & SEVEN_SEG_D) != 0)
        out |= 0x08;
    if ((mask & SEVEN_SEG_E) != 0)
        out |= 0x04;
    if ((mask & SEVEN_SEG_F) != 0)
        out |= 0x02;
    if ((mask & SEVEN_SEG_G) != 0)
        out |= 0x01;
    if ((mask & SEVEN_SEG_DP) != 0)
        out |= 0x80;
    return out;
}
