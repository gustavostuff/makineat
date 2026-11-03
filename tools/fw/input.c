#include "input.h"

#include <stdint.h>

#define SWITCHES 10
#define DEBOUNCE 2

static bool stable[SWITCHES];
static uint8_t count[SWITCHES];

void input_reset(void)
{
    int i;

    for (i = 0; i < SWITCHES; i++) {
        stable[i] = false;
        count[i] = 0;
    }
}

static void one(bool raw, int index, bool *held, bool *pressed, bool *released)
{
    *pressed = false;
    *released = false;
    if (raw == stable[index]) {
        count[index] = 0;
    } else {
        count[index]++;
        if (count[index] >= DEBOUNCE) {
            stable[index] = raw;
            count[index] = 0;
            if (raw)
                *pressed = true;
            else
                *released = true;
        }
    }
    *held = stable[index];
}

void input_tick(bool up, bool down, bool left, bool right, bool button, Input *out)
{
    one(up, 0, &out->up, &out->up_pressed, &out->up_released);
    one(down, 1, &out->down, &out->down_pressed, &out->down_released);
    one(left, 2, &out->left, &out->left_pressed, &out->left_released);
    one(right, 3, &out->right, &out->right_pressed, &out->right_released);
    one(button, 4, &out->button, &out->button_pressed, &out->button_released);
}

void input_tick_p2(bool up, bool down, bool left, bool right, bool button, Input *out)
{
    one(up, 5, &out->p2_up, &out->p2_up_pressed, &out->p2_up_released);
    one(down, 6, &out->p2_down, &out->p2_down_pressed, &out->p2_down_released);
    one(left, 7, &out->p2_left, &out->p2_left_pressed, &out->p2_left_released);
    one(right, 8, &out->p2_right, &out->p2_right_pressed, &out->p2_right_released);
    one(button, 9, &out->p2_button, &out->p2_button_pressed, &out->p2_button_released);
}
