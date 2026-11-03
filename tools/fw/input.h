#ifndef FW_INPUT_H
#define FW_INPUT_H

#include <stdbool.h>

/*
 * Player one uses five switches. Player two uses the same layout in p2_*.
 * A change sticks after the new raw level has lasted two ticks.
 */

typedef struct Input {
    bool up, down, left, right, button;
    bool up_pressed, down_pressed, left_pressed, right_pressed, button_pressed;
    bool up_released, down_released, left_released, right_released, button_released;
    bool p2_up, p2_down, p2_left, p2_right, p2_button;
    bool p2_up_pressed, p2_down_pressed, p2_left_pressed, p2_right_pressed,
        p2_button_pressed;
    bool p2_up_released, p2_down_released, p2_left_released, p2_right_released,
        p2_button_released;
} Input;

void input_reset(void);

/* Raw arguments are 1 when the switch is closed. */
void input_tick(bool up, bool down, bool left, bool right, bool button, Input *out);
void input_tick_p2(bool up, bool down, bool left, bool right, bool button, Input *out);

#endif
