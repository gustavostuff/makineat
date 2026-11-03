#ifndef TEXT_TEST_H
#define TEXT_TEST_H

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

void text_test_reset(void);
void text_test_tick(const Input *in);
int text_test_demo_index(void);
int text_test_demo_count(void);
int text_test_scroll_x(void);
int text_test_scroll_y(void);
const char *text_test_message(void);

#endif
