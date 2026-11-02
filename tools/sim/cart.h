#ifndef SIM_CART_H
#define SIM_CART_H

#include "input.h"

/*
 * Which game image is in the socket. cart_id reads a JSON object
 * and returns "paddle", "space-shooter", "arkaled", or NULL.
 * cart_read_file returns 1 for a known cart, 0 when the file has
 * no usable cart, and -1 when the file is missing.
 */

const char *cart_id(const char *json);
int cart_read_file(const char *path);
int cart_loaded(void);
const char *cart_name(void);
const char *cart_eeprom(void);
void cart_reset(void);
void cart_tick(const Input *in);

#endif
