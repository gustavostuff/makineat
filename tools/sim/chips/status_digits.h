#ifndef RD_STATUS_DIGITS_H
#define RD_STATUS_DIGITS_H

#include "engine/signal.h"

#include <stdint.h>

#define STATUS_DIGITS 8
#define STATUS_DIGIT_PINS 16

/*
 * Passive 8-digit module on DS5_* nets (J7 segments, J9 digit cathodes).
 * Pin names 0-F match makineat.py / KiCad DS5 net suffixes.
 */
typedef struct StatusDigits {
    const char *refdes;
    Pin pin[STATUS_DIGIT_PINS];
    /* Per-digit segment mask: SEVEN_SEG_A..DP (see seven_seg.h). */
    uint8_t frame[STATUS_DIGITS];
} StatusDigits;

void status_digits_init(StatusDigits *mod, const char *refdes);
Pin *status_digits_pin(StatusDigits *mod, int pin_number);

void status_digits_clear_frame(StatusDigits *mod);
void status_digits_sample(StatusDigits *mod);
uint8_t status_digits_frame_mask(const StatusDigits *mod, int digit);

#endif
