#ifndef TEXT_DEMO_H
#define TEXT_DEMO_H

#include "input.h"

/*
 * Text test cart: cycles through shared text_print, outline, blink, and
 * print_scroll flows.
 *
 * Row 2: demo name (bounce scroll). Row 7: sample body for each flow.
 *
 * Stick Left / Right: previous / next demo
 * Button: restart the current demo
 */

void text_demo_reset(void);
void text_demo_tick(const Input *in);
int text_demo_demo_index(void);
int text_demo_demo_count(void);
int text_demo_scroll_x(void);
int text_demo_scroll_y(void);
const char *text_demo_message(void);

#endif
