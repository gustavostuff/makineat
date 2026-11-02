#ifndef FW_STATUS_H
#define FW_STATUS_H

#include <stdint.h>

void status_clear(void);
void status_set_digit(uint8_t digit, uint8_t value);
uint8_t status_get_digit(uint8_t digit);

/* Digit register bytes for drivers_flush (Code-B data, no address). */
const uint8_t *status_digits(void);

#endif
