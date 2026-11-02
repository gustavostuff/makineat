#include "status_digits.h"

#include <string.h>

/* Segment anodes (A,B,C,D,E,F,G,DP) on DS5 pin suffixes. */
static const int status_seg_pins[8] = {0xA, 0xB, 0x6, 0xD, 0x5, 0x1, 0x0, 0x3};
/* Digit cathodes DIG0..DIG7 on DS5 pin suffixes (J9 map). */
static const int status_dig_pins[8] = {0x7, 0x2, 0xF, 0x4, 0x8, 0xE, 0x9, 0xC};

static const char *pin_names[STATUS_DIGIT_PINS] = {
    "0", "1", "2", "3", "4", "5", "6", "7",
    "8", "9", "A", "B", "C", "D", "E", "F",
};

void status_digits_init(StatusDigits *mod, const char *refdes)
{
    int i;

    mod->refdes = refdes;
    for (i = 0; i < STATUS_DIGIT_PINS; i++) {
        Pin *p = &mod->pin[i];

        p->name = pin_names[i];
        p->number = i;
        p->dir = PIN_IN;
        p->drive = SIG_Z;
        p->level = SIG_Z;
        p->net = 0;
    }
    status_digits_clear_frame(mod);
}

Pin *status_digits_pin(StatusDigits *mod, int pin_number)
{
    if (pin_number < 0 || pin_number >= STATUS_DIGIT_PINS)
        return 0;
    return &mod->pin[pin_number];
}

void status_digits_clear_frame(StatusDigits *mod)
{
    memset(mod->frame, 0, sizeof mod->frame);
}

static int lit(const StatusDigits *mod, int anode_pin, int cathode_pin)
{
    if (anode_pin < 0 || anode_pin >= STATUS_DIGIT_PINS ||
        cathode_pin < 0 || cathode_pin >= STATUS_DIGIT_PINS)
        return 0;
    return mod->pin[anode_pin].level == SIG_H && mod->pin[cathode_pin].level == SIG_L;
}

void status_digits_sample(StatusDigits *mod)
{
    int d;
    int s;

    for (d = 0; d < STATUS_DIGITS; d++) {
        for (s = 0; s < 8; s++) {
            if (lit(mod, status_seg_pins[s], status_dig_pins[d]))
                mod->frame[d] |= (uint8_t)(1u << s);
        }
    }
}

uint8_t status_digits_frame_mask(const StatusDigits *mod, int digit)
{
    if (digit < 0 || digit >= STATUS_DIGITS)
        return 0;
    return mod->frame[digit];
}
