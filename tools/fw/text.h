#ifndef FW_TEXT_H
#define FW_TEXT_H

/*
 * Shared 4x4 glyphs (3x3 font with 1px right and bottom margin).
 * Digits 0-9, letters A-Z, dot, comma, colon, semicolon, -, +, =, and /.
 * A lowercase letter uses the capital glyph. A space is a blank cell.
 *
 * text_print paints one line from column x, row y. Each glyph takes
 * the next four columns. Pixels past the field are ignored.
 *
 * print_scroll advances and draws clipped scrolling text. Configure the
 * TextScroll fields before text_scroll_start, then call print_scroll
 * once per 60 Hz frame.
 */

#include <stdint.h>

#define TEXT_W 4
#define TEXT_H 4
#define TEXT_FIELD_W 16
#define TEXT_FRAME_HZ 60
#define TEXT_SCROLL_SPEED_FPS_DEFAULT 15
#define TEXT_SCROLL_PAUSE_DEFAULT 60
#define TEXT_SCROLL_VIEW_W_DEFAULT TEXT_FIELD_W
#define TEXT_SCROLL_VIEW_H_DEFAULT TEXT_H

typedef enum {
    TEXT_SCROLL_BOUNCE,
    TEXT_SCROLL_RESET,
} TextScrollRepeat;

typedef struct TextScroll {
    const char *text;
    int x;
    int y;
    int viewport_w;
    int viewport_h;
    int speed_fps;
    int pause_frames;
    TextScrollRepeat repeat;
    int scroll_x;
    int dir;
    int wait;
    int step;
    int pending_reset;
} TextScroll;

const uint8_t *text_glyph(char c);

void text_draw(int x, int y, const char *s);
void text_print(int x, int y, const char *s);

void text_scroll_start(TextScroll *ts, int x, int y, const char *text);
void text_scroll_stop(TextScroll *ts);
void text_scroll_advance(TextScroll *ts);
void text_scroll_paint(const TextScroll *ts);
void print_scroll(TextScroll *ts);

#endif
