#include "chips/matrix8x8.h"
#include "chips/max7219.h"
#include "engine/signal.h"
#include "tests/report.h"

#include <stdio.h>
#include <string.h>

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

/*
 * Replicates the bench tests performed on actual 1088AW hardware
 * using a +5V source and 220 ohm resistor to ground (source: docs/driver_to_matrix_pinout.md).
 */
static void test_bench_pairs(void)
{
    int col = -1;
    int row = -1;

    /* Pin 3 is Column 0 (Anode). Testing with all 8 row cathodes: */
    expect(matrix8x8_test_pair(0x3, 0x7, &col, &row) && col == 0 && row == 0,
           "Pin 3 (+5V, C0) and Pin 7 (GND, R0) lights LED (0, 0)");
    expect(matrix8x8_test_pair(0x3, 0x2, &col, &row) && col == 0 && row == 1,
           "Pin 3 (+5V, C0) and Pin 2 (GND, R1) lights LED (0, 1)");
    expect(matrix8x8_test_pair(0x3, 0xF, &col, &row) && col == 0 && row == 2,
           "Pin 3 (+5V, C0) and Pin F (GND, R2) lights LED (0, 2)");
    expect(matrix8x8_test_pair(0x3, 0x4, &col, &row) && col == 0 && row == 3,
           "Pin 3 (+5V, C0) and Pin 4 (GND, R3) lights LED (0, 3)");
    expect(matrix8x8_test_pair(0x3, 0x8, &col, &row) && col == 0 && row == 4,
           "Pin 3 (+5V, C0) and Pin 8 (GND, R4) lights LED (0, 4)");
    expect(matrix8x8_test_pair(0x3, 0xE, &col, &row) && col == 0 && row == 5,
           "Pin 3 (+5V, C0) and Pin E (GND, R5) lights LED (0, 5)");
    expect(matrix8x8_test_pair(0x3, 0x9, &col, &row) && col == 0 && row == 6,
           "Pin 3 (+5V, C0) and Pin 9 (GND, R6) lights LED (0, 6)");
    expect(matrix8x8_test_pair(0x3, 0xC, &col, &row) && col == 0 && row == 7,
           "Pin 3 (+5V, C0) and Pin C (GND, R7) lights LED (0, 7)");

    /* Diagonal test across all 8 columns and rows: */
    expect(matrix8x8_test_pair(0x3, 0x7, &col, &row) && col == 0 && row == 0,
           "Pin 3 (+5V, C0) and Pin 7 (GND, R0) lights LED (0, 0)");
    expect(matrix8x8_test_pair(0xA, 0x2, &col, &row) && col == 1 && row == 1,
           "Pin A (+5V, C1) and Pin 2 (GND, R1) lights LED (1, 1)");
    expect(matrix8x8_test_pair(0xB, 0xF, &col, &row) && col == 2 && row == 2,
           "Pin B (+5V, C2) and Pin F (GND, R2) lights LED (2, 2)");
    expect(matrix8x8_test_pair(0x6, 0x4, &col, &row) && col == 3 && row == 3,
           "Pin 6 (+5V, C3) and Pin 4 (GND, R3) lights LED (3, 3)");
    expect(matrix8x8_test_pair(0xD, 0x8, &col, &row) && col == 4 && row == 4,
           "Pin D (+5V, C4) and Pin 8 (GND, R4) lights LED (4, 4)");
    expect(matrix8x8_test_pair(0x5, 0xE, &col, &row) && col == 5 && row == 5,
           "Pin 5 (+5V, C5) and Pin E (GND, R5) lights LED (5, 5)");
    expect(matrix8x8_test_pair(0x1, 0x9, &col, &row) && col == 6 && row == 6,
           "Pin 1 (+5V, C6) and Pin 9 (GND, R6) lights LED (6, 6)");
    expect(matrix8x8_test_pair(0x0, 0xC, &col, &row) && col == 7 && row == 7,
           "Pin 0 (+5V, C7) and Pin C (GND, R7) lights LED (7, 7)");
}

static void test_reverse_bias_and_invalid(void)
{
    int col = -1;
    int row = -1;

    /* Inverted polarity (+5V on cathode, GND on anode): */
    expect(!matrix8x8_test_pair(0x7, 0x3, &col, &row),
           "Pin 7 (+5V, R0) and Pin 3 (GND, C0) is reverse-biased and dark");
    expect(!matrix8x8_test_pair(0x4, 0x6, &col, &row),
           "Pin 4 (+5V, R3) and Pin 6 (GND, C3) is reverse-biased and dark");

    /* Two anodes (+5V and GND on columns): */
    expect(!matrix8x8_test_pair(0x3, 0xA, &col, &row),
           "two column anode pins (3 and A) produce no light");

    /* Two cathodes (+5V and GND on rows): */
    expect(!matrix8x8_test_pair(0x7, 0x2, &col, &row),
           "two row cathode pins (7 and 2) produce no light");

    /* Same pin or out of range: */
    expect(!matrix8x8_test_pair(0x3, 0x3, &col, &row),
           "same pin for +5V and GND produces no light");
    expect(!matrix8x8_test_pair(-1, 0x3, &col, &row),
           "pin -1 is invalid");
    expect(!matrix8x8_test_pair(0x3, 16, &col, &row),
           "pin 16 is invalid");
}

static void test_exhaustive_64_leds(void)
{
    Matrix8x8 mat;
    int target_c;
    int target_r;
    int all_ok = 1;

    matrix8x8_init(&mat, "DS1");

    for (target_r = 0; target_r < 8; target_r++) {
        for (target_c = 0; target_c < 8; target_c++) {
            int c;
            int r;
            int anode_pin = matrix8x8_col_pins[target_c];
            int cathode_pin = matrix8x8_row_pins[target_r];

            /* Drive target anode HIGH and target cathode LOW */
            for (c = 0; c < 16; c++)
                mat.pin[c].level = SIG_Z;
            mat.pin[anode_pin].level = SIG_H;
            mat.pin[cathode_pin].level = SIG_L;

            if (!matrix8x8_led_active(&mat, target_c, target_r))
                all_ok = 0;

            /* Check that all other 63 LEDs remain dark */
            for (r = 0; r < 8; r++) {
                for (c = 0; c < 8; c++) {
                    if (c == target_c && r == target_r)
                        continue;
                    if (matrix8x8_led_active(&mat, c, r))
                        all_ok = 0;
                }
            }
        }
    }
    expect(all_ok, "all 64 LEDs light exclusively when their CX and RX are active");
}

static void test_netlist_with_max7219(void)
{
    Max7219 drv;
    Matrix8x8 mat;
    Net ds_net[16];
    static const int matrix_drv_pins[16] = {
        17, 15, 11, 22, 7, 21, 20, 2, 3, 5, 14, 16, 8, 23, 10, 6
    };
    int p;
    int r;
    uint64_t now_ns = 0;

    max7219_init(&drv, "U1");
    matrix8x8_init(&mat, "DS1");

    /* Wire according to the updated netlist: */
    for (p = 0; p < 16; p++) {
        char net_name[16];
        snprintf(net_name, sizeof net_name, "DS1_%X", p);
        net_init(&ds_net[p], net_name, SIG_L);
        net_tie(&ds_net[p], max7219_pin(&drv, matrix_drv_pins[p]));
        net_tie(&ds_net[p], matrix8x8_pin(&mat, p));
    }

    /* Configure MAX7219: wake from shutdown, full scan limit */
    drv.shutdown = 0;
    drv.scan_limit = 7;
    drv.intensity = 15;
    drv.wake_ns = 0;
    drv.scan_origin_ns = 0;

    /*
     * Load an 'X' pattern into the driver registers:
     * Row 0: 0x81 (pixels 0, 7)
     * Row 1: 0x42 (pixels 1, 6)
     * Row 2: 0x24 (pixels 2, 5)
     * Row 3: 0x18 (pixels 3, 4)
     * Row 4: 0x18 (pixels 3, 4)
     * Row 5: 0x24 (pixels 2, 5)
     * Row 6: 0x42 (pixels 1, 6)
     * Row 7: 0x81 (pixels 0, 7)
     */
    drv.digit[0] = 0x81;
    drv.digit[1] = 0x42;
    drv.digit[2] = 0x24;
    drv.digit[3] = 0x18;
    drv.digit[4] = 0x18;
    drv.digit[5] = 0x24;
    drv.digit[6] = 0x42;
    drv.digit[7] = 0x81;

    matrix8x8_clear_frame(&mat);

    /* Step through one full 8-digit scan period (1.25 ms) in 10 us increments */
    for (now_ns = 1000; now_ns <= 8 * MAX7219_SLOT_NS; now_ns += 10000) {
        max7219_pins(&drv, now_ns);
        for (p = 0; p < 16; p++) {
            net_settle(&ds_net[p]);
        }
        matrix8x8_sample(&mat);
    }

    /* Verify that the accumulated frame on the matrix matches the pattern */
    for (r = 0; r < 8; r++) {
        char msg[64];
        uint8_t expected_row = 0;

        switch (r) {
        case 0: case 7: expected_row = (1u << 0) | (1u << 7); break;
        case 1: case 6: expected_row = (1u << 1) | (1u << 6); break;
        case 2: case 5: expected_row = (1u << 2) | (1u << 5); break;
        case 3: case 4: expected_row = (1u << 3) | (1u << 4); break;
        }

        snprintf(msg, sizeof msg, "netlist matrix frame matches expected row %d pattern", r);
        expect(mat.frame[r] == expected_row, msg);
    }
}

static void test_old_netlist_fails(void)
{
    /*
     * If an incorrect / swapped netlist is wired:
     * Segment DP (pin 22) -> Pin 7 (which is 1088AW Row 0 Cathode)
     * Digit 0 (pin 2) -> Pin 2 (which is 1088AW Row 1 Cathode)
     * Both pins are cathodes, so driving them produces zero forward current.
     */
    Max7219 drv;
    Matrix8x8 mat;
    Net wrong_col0;
    Net wrong_row0;

    max7219_init(&drv, "U1");
    matrix8x8_init(&mat, "DS1");

    net_init(&wrong_col0, "WRONG_C0", SIG_L);
    net_tie(&wrong_col0, max7219_pin(&drv, 22)); /* SEGDP */
    net_tie(&wrong_col0, matrix8x8_pin(&mat, 0x7)); /* Pin 7 is cathode! */

    net_init(&wrong_row0, "WRONG_R0", SIG_H);
    net_tie(&wrong_row0, max7219_pin(&drv, 2));   /* DIG0 */
    net_tie(&wrong_row0, matrix8x8_pin(&mat, 0x2)); /* Pin 2 is cathode! */

    drv.shutdown = 0;
    drv.scan_limit = 0;
    drv.intensity = 15;
    drv.wake_ns = 0;
    drv.scan_origin_ns = 0;
    drv.digit[0] = 0x80; /* Col 0 bit active */

    /* Run during digit 0 slot */
    max7219_pins(&drv, 1000);
    net_settle(&wrong_col0);
    net_settle(&wrong_row0);

    /* Under incorrect netlist, LED (0, 0) fails to illuminate */
    expect(!matrix8x8_led_active(&mat, 0, 0),
           "incorrect cathode-cathode netlist connection fails to light LED (0,0)");
}

int main(void)
{
    report_begin("matrix");
    test_bench_pairs();
    test_reverse_bias_and_invalid();
    test_exhaustive_64_leds();
    test_netlist_with_max7219();
    test_old_netlist_fails();
    report_end();
    return 0;
}
