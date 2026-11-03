#include "text.h"

#include "frame.h"

/* Bit 0 of a row is the left pixel.
 * Each glyph has 1px margin on the right (col 3) and bottom (row 3). */
static const uint8_t digits[10][TEXT_H] = {
    {0x07, 0x05, 0x07, 0x00}, /* 0 */
    {0x03, 0x02, 0x07, 0x00}, /* 1 */
    {0x03, 0x02, 0x06, 0x00}, /* 2 */
    {0x07, 0x06, 0x07, 0x00}, /* 3 */
    {0x05, 0x07, 0x04, 0x00}, /* 4 */
    {0x06, 0x02, 0x03, 0x00}, /* 5 */
    {0x01, 0x07, 0x07, 0x00}, /* 6 */
    {0x07, 0x04, 0x04, 0x00}, /* 7 */
    {0x02, 0x07, 0x07, 0x00}, /* 8 */
    {0x07, 0x07, 0x04, 0x00}, /* 9 */
};

static const uint8_t letters[26][TEXT_H] = {
    {0x06, 0x05, 0x07, 0x00}, /* A */
    {0x01, 0x07, 0x07, 0x00}, /* B */
    {0x07, 0x01, 0x07, 0x00}, /* C */
    {0x04, 0x07, 0x07, 0x00}, /* D */
    {0x07, 0x03, 0x07, 0x00}, /* E */
    {0x07, 0x03, 0x01, 0x00}, /* F */
    {0x03, 0x05, 0x07, 0x00}, /* G */
    {0x05, 0x07, 0x05, 0x00}, /* H */
    {0x07, 0x02, 0x07, 0x00}, /* I */
    {0x07, 0x02, 0x03, 0x00}, /* J */
    {0x05, 0x03, 0x05, 0x00}, /* K */
    {0x01, 0x01, 0x07, 0x00}, /* L */
    {0x03, 0x06, 0x04, 0x00}, /* M */
    {0x07, 0x05, 0x05, 0x00}, /* N */
    {0x07, 0x05, 0x07, 0x00}, /* O */
    {0x07, 0x07, 0x01, 0x00}, /* P */
    {0x07, 0x07, 0x04, 0x00}, /* Q */
    {0x07, 0x03, 0x05, 0x00}, /* R */
    {0x06, 0x02, 0x03, 0x00}, /* S */
    {0x07, 0x02, 0x02, 0x00}, /* T */
    {0x05, 0x05, 0x07, 0x00}, /* U */
    {0x05, 0x05, 0x02, 0x00}, /* V */
    {0x01, 0x03, 0x06, 0x00}, /* W */
    {0x05, 0x02, 0x05, 0x00}, /* X */
    {0x05, 0x02, 0x02, 0x00}, /* Y */
    {0x03, 0x02, 0x06, 0x00}, /* Z */
};

static const uint8_t dot[TEXT_H] = {0x00, 0x00, 0x01, 0x00};
static const uint8_t comma[TEXT_H] = {0x00, 0x00, 0x02, 0x01};
static const uint8_t colon[TEXT_H] = {0x02, 0x00, 0x02, 0x00};
static const uint8_t semicolon[TEXT_H] = {0x02, 0x00, 0x02, 0x01};
static const uint8_t dash[TEXT_H] = {0x00, 0x07, 0x00, 0x00};
static const uint8_t plus[TEXT_H] = {0x02, 0x07, 0x02, 0x00};
static const uint8_t equals[TEXT_H] = {0x07, 0x00, 0x07, 0x00};
static const uint8_t slash[TEXT_H] = {0x04, 0x02, 0x01, 0x00};
static const uint8_t blank[TEXT_H] = {0x00, 0x00, 0x00, 0x00};

const uint8_t *text_glyph(char c)
{
    if (c >= '0' && c <= '9')
        return digits[c - '0'];
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z')
        return letters[c - 'A'];
    if (c == '.')
        return dot;
    if (c == ',')
        return comma;
    if (c == ':')
        return colon;
    if (c == ';')
        return semicolon;
    if (c == '-')
        return dash;
    if (c == '+')
        return plus;
    if (c == '=')
        return equals;
    if (c == '/')
        return slash;
    if (c == ' ')
        return blank;
    return 0;
}

static void blit(int x, int y, const uint8_t *rows)
{
    int row;
    int col;

    for (row = 0; row < TEXT_H; row++) {
        for (col = 0; col < TEXT_W; col++) {
            int px;
            int py;

            if ((rows[row] & (uint8_t)(1u << col)) == 0)
                continue;
            px = x + col;
            py = y + row;
            if (px < 0 || py < 0 || px >= 16 || py >= 16)
                continue;
            pixel((uint8_t)px, (uint8_t)py, true);
        }
    }
}

void text_draw(int x, int y, const char *s)
{
    if (s == 0)
        return;
    for (; *s != '\0'; s++) {
        const uint8_t *rows = text_glyph(*s);

        if (rows == 0)
            continue;
        if (x >= 16)
            break;
        if (x + TEXT_W > 0)
            blit(x, y, rows);
        x += TEXT_W;
    }
}
