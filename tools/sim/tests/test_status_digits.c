#include "chips/max7219.h"
#include "chips/status_digits.h"
#include "engine/signal.h"
#include "seven_seg.h"

#include "tests/report.h"

#include <stdio.h>
#include <string.h>

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static const int matrix_drv_pins[16] = {
    17, 15, 11, 22, 7, 21, 20, 2, 3, 5, 14, 16, 8, 23, 10, 6,
};

static Net nets[16];

static void tie_status_net(Max7219 *drv, StatusDigits *mod, int pin_index)
{
    net_init(&nets[pin_index], "DS5", SIG_L);
    net_tie(&nets[pin_index], max7219_pin(drv, matrix_drv_pins[pin_index]));
    net_tie(&nets[pin_index], status_digits_pin(mod, pin_index));
}

static void settle_nets(void)
{
    int p;

    for (p = 0; p < 16; p++)
        net_settle(&nets[p]);
}

static void test_netlist_code_b(void)
{
    Max7219 u7;
    StatusDigits ds5;
    int p;

    max7219_init(&u7, "U7");
    status_digits_init(&ds5, "DS5");
    for (p = 0; p < 16; p++)
        tie_status_net(&u7, &ds5, p);

    u7.shutdown = 0;
    u7.wake_ns = 0;
    u7.scan_limit = 7;
    u7.decode = 0xFF;
    u7.intensity = 15;
    u7.digit[7] = 0x07;

    status_digits_clear_frame(&ds5);
    for (p = 0; p < 8; p++) {
        uint64_t t = (uint64_t)p * MAX7219_SLOT_NS + MAX7219_SLOT_NS / 4;

        max7219_pins(&u7, t);
        settle_nets();
        status_digits_sample(&ds5);
    }

    expect(status_digits_frame_mask(&ds5, 7) ==
               (SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C),
           "DS5 netlist shows Code-B 7 on the rightmost digit");
}

static void test_max7219_mask_map(void)
{
    expect(seven_seg_mask_from_max7219(0x30) == (SEVEN_SEG_B | SEVEN_SEG_C),
           "Code-B 1 maps to segments B and C");
}

int main(void)
{
    report_begin("status_digits");
    test_max7219_mask_map();
    test_netlist_code_b();
    return report_end();
}
