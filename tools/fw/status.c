#include "status.h"

#include <string.h>

static uint8_t status_ram[8];

void status_clear(void)
{
    memset(status_ram, 0, sizeof status_ram);
}

void status_set_digit(uint8_t digit, uint8_t value)
{
    if (digit >= 8)
        return;
    status_ram[digit] = (uint8_t)(value & 0x7Fu);
}

uint8_t status_get_digit(uint8_t digit)
{
    if (digit >= 8)
        return 0;
    return status_ram[digit];
}

const uint8_t *status_digits(void)
{
    return status_ram;
}
