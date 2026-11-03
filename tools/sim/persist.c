#include "persist.h"

#include <stdio.h>
#include <string.h>

/* The AVR128DB28 EEPROM is 512 bytes. Erased cells read as 0xFF. */

#define EEPROM_BYTES 512

static uint8_t image[EEPROM_BYTES];
static int ready;
static int detached;
static const char *path;
static const char *bound;

static const char *const cand[] = {"tools/sim/makineat.eep", "makineat.eep"};

void persist_detach(void)
{
    bound = NULL;
    path = NULL;
    detached = 1;
    ready = 0;
}

void persist_bind(const char *stored)
{
    detached = 0;
    bound = stored;
    path = NULL;
    ready = 0;
}

static void load(void)
{
    int i;
    FILE *f;

    if (detached) {
        memset(image, 0xFF, sizeof image);
        ready = 1;
        return;
    }
    memset(image, 0xFF, sizeof image);
    path = NULL;
    ready = 1;
    if (bound != NULL) {
        path = bound;
        f = fopen(bound, "rb");
        if (f == NULL)
            return;
        fread(image, 1, EEPROM_BYTES, f);
        fclose(f);
        return;
    }
    for (i = 0; i < 2; i++) {
        FILE *f = fopen(cand[i], "rb");

        if (f == NULL)
            continue;
        fread(image, 1, EEPROM_BYTES, f);
        fclose(f);
        path = cand[i];
        break;
    }
    ready = 1;
}

static int store(void)
{
    int i;
    FILE *f;

    if (detached)
        return 1;
    if (path != NULL) {
        f = fopen(path, "wb");
        if (f != NULL) {
            fwrite(image, 1, EEPROM_BYTES, f);
            fclose(f);
            return 1;
        }
    }
    for (i = 0; i < 2; i++) {
        f = fopen(cand[i], "wb");
        if (f == NULL)
            continue;
        fwrite(image, 1, EEPROM_BYTES, f);
        fclose(f);
        path = cand[i];
        return 1;
    }
    return 0;
}

uint16_t persist_read(void)
{
    if (!ready)
        load();
    return (uint16_t)image[0] | ((uint16_t)image[1] << 8);
}

void persist_write(uint16_t value)
{
    if (!ready)
        load();
    image[0] = (uint8_t)value;
    image[1] = (uint8_t)(value >> 8);
    store();
}

void persist_reset(void)
{
    if (!ready)
        load();
    memset(image, 0xFF, sizeof image);
    store();
}
