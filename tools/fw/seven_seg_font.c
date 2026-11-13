#include "seven_seg_font.h"
#include "seven_seg_font_table.h"

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
