#ifndef FW_PERSIST_H
#define FW_PERSIST_H

#include <stdint.h>

/*
 * One 16-bit word at the start of the EEPROM image.
 * An erased image reads as 0xFFFF.
 */

/* NULL keeps the shooter's tools/sim/makineat.eep image. */
void persist_bind(const char *stored);
/* RAM-only image: no file read or write (e.g. pong). */
void persist_detach(void);
uint16_t persist_read(void);
void persist_write(uint16_t value);
void persist_reset(void);

#endif
