#ifndef FW_FRAME_H
#define FW_FRAME_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Sixteen rows, sixteen columns, one bit each.
 * frame[0] is the top row. Bit 0 is the leftmost pixel. A set bit is on.
 */

#define FIELD_SIZE 16
#define FIELD_LAST (FIELD_SIZE - 1)

extern uint16_t frame[FIELD_SIZE];

void pixel(uint8_t x, uint8_t y, bool on);
bool pixel_get(uint8_t x, uint8_t y);
void clear(void);
void frame_rect(int x, int y, int w, int h, bool on);
void frame_hline(int x, int y, int w, bool on);
void frame_vline(int x, int y, int h, bool on);

#endif
