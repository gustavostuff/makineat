#include "chips/max7219.h"
#include "chips/status_digits.h"
#include "engine/signal.h"
#include "seven_seg.h"
#include "seven_seg_hitmap.h"

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

static void test_font_seed(void)
{
    expect(seven_seg_font_mask('0') ==
               (SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_E | SEVEN_SEG_F),
           "font 0 is segments A through F");
    expect(seven_seg_font_mask('1') == (SEVEN_SEG_B | SEVEN_SEG_C), "font 1 is segments B and C");
    expect(seven_seg_font_mask('2') ==
               (SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_D | SEVEN_SEG_E | SEVEN_SEG_G),
           "font 2 is segments A B D E G");
    expect(seven_seg_font_mask('3') ==
               (SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_G),
           "font 3 is segments A B C D G");
    expect(seven_seg_font_mask('4') == (SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_F | SEVEN_SEG_G),
           "font 4 is segments B C F G");
    expect(seven_seg_font_mask('5') ==
               (SEVEN_SEG_A | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_F | SEVEN_SEG_G),
           "font 5 is segments A C D F G");
    expect(seven_seg_font_mask('6') == (SEVEN_SEG_A | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_E |
                                        SEVEN_SEG_F | SEVEN_SEG_G),
           "font 6 is segments A C D E F G");
    expect(seven_seg_font_mask('7') == (SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C),
           "font 7 is segments A B C");
    expect(seven_seg_font_mask('8') == (SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_D |
                                        SEVEN_SEG_E | SEVEN_SEG_F | SEVEN_SEG_G),
           "font 8 lights every bar");
    expect(seven_seg_font_mask('9') ==
               (SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_F | SEVEN_SEG_G),
           "font 9 is segments A B C D F G");
    expect(seven_seg_font_mask(' ') == 0, "font space is blank");
    expect(seven_seg_encode_char('4') == seven_seg_font_mask('4'),
           "the simulator lookup uses the shared font");
    expect(seven_seg_font_mask('@') == 0, "a missing symbol stays blank");
    expect(seven_seg_font_mask('F') == seven_seg_font_mask('f'),
           "F uses f when that is the letter in the font");
    expect(seven_seg_font_mask('a') == seven_seg_font_mask('A'),
           "a uses A when that is the letter in the font");
    expect(seven_seg_hitmap[0 * SEVEN_SEG_HIT_W + 2] == 1, "the top red pixels are segment A");
    expect(seven_seg_hitmap[16 * SEVEN_SEG_HIT_W + 10] == 8,
           "the lower-right decimal point is a hit target");
    expect(seven_seg_hitmap[0] == 0, "empty pixels are not a segment");
}

int main(void)
{
    report_begin("status_digits");
    test_max7219_mask_map();
    test_font_seed();
    test_netlist_code_b();
    return report_end();
}
