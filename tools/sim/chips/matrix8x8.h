#ifndef RD_MATRIX8X8_H
#define RD_MATRIX8X8_H

/*
 * 1088AW 8x8 LED Matrix module model.
 * 16-pin package (two 1x08 socket strips, 25.4 mm row spacing).
 *
 * Source of truth: docs/driver_to_matrix_pinout.md
 *
 * Physical Orientation (viewed from front display face, notch at bottom edge):
 * Top row (opposite notch, left-to-right, Col 0 to Col 7): Pins 0 to 7.
 * Bottom row (notch side, left-to-right, Col 0 to Col 7): Pins 8 to F.
 *
 * Polarity: Row Cathode / Column Anode (1088AW).
 * An LED at (col, row) illuminates if and only if its column anode pin is driven
 * HIGH (SIG_H) and its row cathode pin is pulled LOW (SIG_L).
 *
 * Column Anodes (apply +V):
 *   Col 0 (MSB): Pin 0x3 (3)
 *   Col 1:       Pin 0xA (A)
 *   Col 2:       Pin 0xB (B)
 *   Col 3:       Pin 0x6 (6)
 *   Col 4:       Pin 0xD (D)
 *   Col 5:       Pin 0x5 (5)
 *   Col 6:       Pin 0x1 (1)
 *   Col 7 (LSB): Pin 0x0 (0)
 *
 * Row Cathodes (pull to GND):
 *   Row 0 (Top): Pin 0x7 (7)
 *   Row 1:       Pin 0x2 (2)
 *   Row 2:       Pin 0xF (F)
 *   Row 3:       Pin 0x4 (4)
 *   Row 4:       Pin 0x8 (8)
 *   Row 5:       Pin 0xE (E)
 *   Row 6:       Pin 0x9 (9)
 *   Row 7:       Pin 0xC (C)
 */

#include "engine/signal.h"

#include <stdint.h>

#define MATRIX8X8_PINS 16

/* Physical pin numbers (0 to 15 / 0x0 to 0xF) for column C0..C7 and row R0..R7 */
extern const int matrix8x8_col_pins[8];
extern const int matrix8x8_row_pins[8];

/* Aliases for compatibility */
#define matrix8x8_col_pads matrix8x8_col_pins
#define matrix8x8_row_pads matrix8x8_row_pins

typedef struct Matrix8x8 {
    const char *refdes;
    Pin pin[MATRIX8X8_PINS];
    /* Accumulated persistence frame over a scan cycle:
     * bit c of frame[r] is 1 if LED (c, r) was forward-biased. */
    uint8_t frame[8];
} Matrix8x8;

void matrix8x8_init(Matrix8x8 *mat, const char *refdes);
Pin *matrix8x8_pin(Matrix8x8 *mat, int pin_number);

/* Returns 1 if LED at (col, row) is currently forward-biased (anode H, cathode L).
 * Returns 0 if reverse-biased, unpowered, floating, or out of range. */
int matrix8x8_led_active(const Matrix8x8 *mat, int col, int row);

/* Updates frame persistence by latching currently active LEDs */
void matrix8x8_sample(Matrix8x8 *mat);

/* Clears frame persistence buffer */
void matrix8x8_clear_frame(Matrix8x8 *mat);

/* Returns 1 if LED at (col, row) is lit in the persistent frame buffer */
int matrix8x8_led_frame(const Matrix8x8 *mat, int col, int row);

/* Returns 1 if applying HIGH to anode_pin and LOW to cathode_pin lights an LED.
 * Sets *out_col and *out_row to the 0-based column and row coordinates.
 * Returns 0 if reverse-biased, invalid pins, or no LED lights. */
int matrix8x8_test_pair(int anode_pin, int cathode_pin, int *out_col, int *out_row);

#endif
