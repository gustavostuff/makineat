#include "persist.h"

/* Tests keep the word in RAM. An empty image reads as erased. */

static uint16_t word = 0xFFFF;

void persist_bind(const char *stored)
{
    (void)stored;
}

void persist_detach(void)
{
}

uint16_t persist_read(void)
{
    return word;
}

void persist_write(uint16_t value)
{
    word = value;
}

void persist_reset(void)
{
    word = 0xFFFF;
}
