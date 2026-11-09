#include "chips/matrix8x8.h"

#include <string.h>

/*
 * Column anodes C0..C7 mapping to 1088AW pins (0x0 to 0xF).
 * C0 is leftmost pixel (bit 7, DP), C7 is rightmost pixel (bit 0, G).
 * Source: docs/driver_to_matrix_pinout.md
 */
const int matrix8x8_col_pins[8] = {0x3, 0xA, 0xB, 0x6, 0xD, 0x5, 0x1, 0x0};

/*
 * Row cathodes R0..R7 mapping to 1088AW pins (0x0 to 0xF).
 * R0 is top row (DIG0), R7 is bottom row (DIG7).
 * Source: docs/driver_to_matrix_pinout.md
 */
const int matrix8x8_row_pins[8] = {0x7, 0x2, 0xF, 0x4, 0x8, 0xE, 0x9, 0xC};

static const char *pin_names[MATRIX8X8_PINS] = {
    "0", "1", "2", "3", "4", "5", "6", "7",
    "8", "9", "A", "B", "C", "D", "E", "F",
};

void matrix8x8_init(Matrix8x8 *mat, const char *refdes)
{
    int i;

    mat->refdes = refdes;
    for (i = 0; i < MATRIX8X8_PINS; i++) {
        Pin *p = &mat->pin[i];

        p->name = pin_names[i];
        p->number = i;
        p->dir = PIN_IN;
        p->drive = SIG_Z;
        p->level = SIG_Z;
        p->net = 0;
    }
    matrix8x8_clear_frame(mat);
}

Pin *matrix8x8_pin(Matrix8x8 *mat, int pin_number)
{
    if (pin_number < 0 || pin_number >= MATRIX8X8_PINS)
        return 0;
    return &mat->pin[pin_number];
}

int matrix8x8_led_active(const Matrix8x8 *mat, int col, int row)
{
    int anode_pin;
    int cathode_pin;

    if (col < 0 || col > 7 || row < 0 || row > 7)
        return 0;
    anode_pin = matrix8x8_col_pins[col];
    cathode_pin = matrix8x8_row_pins[row];
    return (mat->pin[anode_pin].level == SIG_H &&
            mat->pin[cathode_pin].level == SIG_L);
}

void matrix8x8_sample(Matrix8x8 *mat)
{
    int col;
    int row;

    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            if (matrix8x8_led_active(mat, col, row))
                mat->frame[row] |= (uint8_t)(1u << col);
        }
    }
}

void matrix8x8_clear_frame(Matrix8x8 *mat)
{
    memset(mat->frame, 0, sizeof mat->frame);
}

int matrix8x8_led_frame(const Matrix8x8 *mat, int col, int row)
{
    if (col < 0 || col > 7 || row < 0 || row > 7)
        return 0;
    return (mat->frame[row] >> col) & 1u;
}

int matrix8x8_test_pair(int anode_pin, int cathode_pin, int *out_col, int *out_row)
{
    int c;
    int r;
    int found_col = -1;
    int found_row = -1;

    if (anode_pin < 0 || anode_pin >= MATRIX8X8_PINS ||
        cathode_pin < 0 || cathode_pin >= MATRIX8X8_PINS ||
        anode_pin == cathode_pin)
        return 0;

    for (c = 0; c < 8; c++) {
        if (matrix8x8_col_pins[c] == anode_pin) {
            found_col = c;
            break;
        }
    }
    for (r = 0; r < 8; r++) {
        if (matrix8x8_row_pins[r] == cathode_pin) {
            found_row = r;
            break;
        }
    }

    if (found_col >= 0 && found_row >= 0) {
        if (out_col)
            *out_col = found_col;
        if (out_row)
            *out_row = found_row;
        return 1;
    }
    return 0;
}
