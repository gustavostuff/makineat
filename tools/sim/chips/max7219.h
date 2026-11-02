#ifndef RD_MAX7219_H
#define RD_MAX7219_H

/*
 * MAX7219ENG+, 24-pin narrow DIP.
 * Package pins follow the datasheet pin diagram.
 * DIN is sampled on the rising edge of CLK. DOUT follows D15 and changes
 * on the falling edge. The datasheet calls the DIN-to-DOUT path 16.5 clock
 * cycles. Here the bit sampled on a rising edge sits in D15 on the 16th
 * rising edge, and DOUT takes that level on the falling edge. The next
 * driver samples it on the following rising edge, so a 16-bit word moves
 * one driver down the chain every 16 clocks.
 * LOAD's rising edge latches the shift register into the addressed register.
 */

#include "engine/signal.h"

#include <stdint.h>

#define MAX7219_PINS 24

#define MAX7219_DIN 1
#define MAX7219_DIG0 2
#define MAX7219_DIG4 3
#define MAX7219_GND1 4
#define MAX7219_DIG6 5
#define MAX7219_DIG2 6
#define MAX7219_DIG3 7
#define MAX7219_DIG7 8
#define MAX7219_GND2 9
#define MAX7219_DIG5 10
#define MAX7219_DIG1 11
#define MAX7219_LOAD 12
#define MAX7219_CLK 13
#define MAX7219_SEGA 14
#define MAX7219_SEGF 15
#define MAX7219_SEGB 16
#define MAX7219_SEGG 17
#define MAX7219_ISET 18
#define MAX7219_VCC 19
#define MAX7219_SEGC 20
#define MAX7219_SEGE 21
#define MAX7219_SEGDP 22
#define MAX7219_SEGD 23
#define MAX7219_DOUT 24

/* Datasheet minimums, nanoseconds, V+ from 4.0 V to 5.5 V. */
#define MAX7219_TCP_MIN_NS 100u
#define MAX7219_TCH_MIN_NS 50u
#define MAX7219_TCL_MIN_NS 50u
#define MAX7219_TDS_MIN_NS 25u
#define MAX7219_TCLK_TO_LOAD_NS 50u
#define MAX7219_TLOAD_TO_CLK_NS 50u
#define MAX7219_TCSW_MIN_NS 50u

/* Typical time from leaving shutdown until the scan runs. */
#define MAX7219_WAKE_NS 250000u

/*
 * Scan oscillator is 800 Hz. Eight digit slots fit in one period when the
 * scan limit is 7, so each slot is 1/6400 s. Fewer digits raise the visit
 * rate and keep this slot length.
 */
#define MAX7219_SLOT_NS (1000000000ull / 6400ull)

enum {
    MAX7219_OK = 0,
    MAX7219_FAULT_TCH = 1,
    MAX7219_FAULT_TCL = 2,
    MAX7219_FAULT_TCP = 3,
    MAX7219_FAULT_TDS = 4,
    MAX7219_FAULT_TCLK_LOAD = 5,
    MAX7219_FAULT_TLOAD_CLK = 6,
    MAX7219_FAULT_TCSW = 7
};

typedef struct Max7219 {
    const char *refdes;
    Pin pin[MAX7219_PINS];
    uint16_t shift;
    uint8_t digit[8];
    uint8_t decode;
    uint8_t intensity;
    uint8_t scan_limit;
    int shutdown;
    int display_test;
    uint64_t wake_ns;
    uint64_t scan_origin_ns;
    Sig prev_clk;
    Sig prev_load;
    Sig prev_din;
    uint64_t last_din_ns;
    uint64_t last_clk_rise_ns;
    uint64_t last_clk_fall_ns;
    uint64_t last_load_rise_ns;
    int saw_clk_rise;
    int saw_clk_fall;
    int saw_load_rise;
    int fault;
} Max7219;

void max7219_init(Max7219 *chip, const char *refdes);
void max7219_edges(Max7219 *chip, uint64_t now_ns);
void max7219_sense(Max7219 *chip, uint64_t now_ns);
void max7219_pins(Max7219 *chip, uint64_t now_ns);
int max7219_led_on(const Max7219 *chip, int col, int row, uint64_t now_ns);
int max7219_active_digit(const Max7219 *chip, uint64_t now_ns);
/* Latched segment byte for one digit row (bit 7 DP, bit 6 A, .. bit 0 G). */
uint8_t max7219_segment_byte(const Max7219 *chip, int digit);
Pin *max7219_pin(Max7219 *chip, int number);

#endif
