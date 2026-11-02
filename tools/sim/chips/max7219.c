#include "chips/max7219.h"

#include <stdint.h>

typedef struct MaxPinDef {
    int number;
    const char *name;
    PinDir dir;
    Sig idle;
} MaxPinDef;

/* Digit drivers idle high (cathode off). Segment drivers idle low (anode off). */
static const MaxPinDef max_pins[MAX7219_PINS] = {
    {1, "DIN", PIN_IN, SIG_L},
    {2, "DIG0", PIN_OUT, SIG_H},
    {3, "DIG4", PIN_OUT, SIG_H},
    {4, "GND", PIN_PWR, SIG_L},
    {5, "DIG6", PIN_OUT, SIG_H},
    {6, "DIG2", PIN_OUT, SIG_H},
    {7, "DIG3", PIN_OUT, SIG_H},
    {8, "DIG7", PIN_OUT, SIG_H},
    {9, "GND", PIN_PWR, SIG_L},
    {10, "DIG5", PIN_OUT, SIG_H},
    {11, "DIG1", PIN_OUT, SIG_H},
    {12, "LOAD", PIN_IN, SIG_H},
    {13, "CLK", PIN_IN, SIG_L},
    {14, "SEGA", PIN_OUT, SIG_L},
    {15, "SEGF", PIN_OUT, SIG_L},
    {16, "SEGB", PIN_OUT, SIG_L},
    {17, "SEGG", PIN_OUT, SIG_L},
    {18, "ISET", PIN_IN, SIG_Z},
    {19, "V+", PIN_PWR, SIG_H},
    {20, "SEGC", PIN_OUT, SIG_L},
    {21, "SEGE", PIN_OUT, SIG_L},
    {22, "SEGDP", PIN_OUT, SIG_L},
    {23, "SEGD", PIN_OUT, SIG_L},
    {24, "DOUT", PIN_OUT, SIG_L},
};

/* Code B font. Bit 6 is segment A, bit 0 is segment G. DP is added from D7. */
static const uint8_t code_b[16] = {
    0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70,
    0x7F, 0x7B, 0x01, 0x4F, 0x37, 0x0E, 0x67, 0x00,
};

/* Column 0 is DP, then A B C D E F G. Row 0 is DIG0. */
static const int seg_pin[8] = {22, 14, 16, 20, 23, 21, 15, 17};
static const int dig_pin[8] = {2, 11, 6, 7, 3, 10, 5, 8};

static void fault(Max7219 *chip, int code)
{
    if (chip->fault == MAX7219_OK)
        chip->fault = code;
}

static void drive_out(Max7219 *chip, int number, Sig level)
{
    Pin *pin = &chip->pin[number - 1];

    pin->drive = level;
    if (pin->net == 0)
        pin->level = level;
}

static uint8_t segments_of(const Max7219 *chip, int row)
{
    uint8_t raw = chip->digit[row];

    if ((chip->decode >> row) & 1u) {
        uint8_t font = code_b[raw & 0x0F];

        if (raw & 0x80)
            font |= 0x80;
        return font;
    }
    return raw;
}

/*
 * Returns 1 when a digit is selected. *digit is that index.
 * *lit is 0 during the intensity blank at the end of the slot.
 * Display test scans all eight digits at 31/32 and ignores shutdown.
 */
static int scan_phase(const Max7219 *chip, uint64_t now, int *digit, int *lit)
{
    int n;
    int duty;
    uint64_t elapsed;
    uint64_t into;
    uint64_t on_ns;

    if (chip->display_test) {
        n = 8;
        duty = 31;
    } else if (chip->shutdown || now < chip->wake_ns) {
        return 0;
    } else {
        n = (chip->scan_limit & 7) + 1;
        duty = 2 * (chip->intensity & 15) + 1;
    }

    if (now < chip->scan_origin_ns)
        elapsed = 0;
    else
        elapsed = now - chip->scan_origin_ns;
    into = elapsed % (MAX7219_SLOT_NS * (uint64_t)n);
    *digit = (int)(into / MAX7219_SLOT_NS);
    on_ns = (MAX7219_SLOT_NS * (uint64_t)duty) / 32u;
    *lit = (into % MAX7219_SLOT_NS) < on_ns;
    return 1;
}

void max7219_init(Max7219 *chip, const char *refdes)
{
    int i;

    chip->refdes = refdes;
    for (i = 0; i < MAX7219_PINS; i++) {
        const MaxPinDef *def = &max_pins[i];
        Pin *pin = &chip->pin[def->number - 1];

        pin->name = def->name;
        pin->number = def->number;
        pin->dir = def->dir;
        pin->net = 0;
        if (def->dir == PIN_IN) {
            pin->drive = SIG_Z;
            pin->level = def->idle;
        } else {
            pin->drive = def->idle;
            pin->level = def->idle;
        }
    }

    chip->shift = 0;
    for (i = 0; i < 8; i++)
        chip->digit[i] = 0;
    chip->decode = 0;
    chip->intensity = 0;
    chip->scan_limit = 0;
    chip->shutdown = 1;
    chip->display_test = 0;
    chip->wake_ns = UINT64_MAX;
    chip->scan_origin_ns = 0;
    chip->prev_clk = SIG_L;
    chip->prev_load = SIG_H;
    chip->prev_din = SIG_L;
    chip->last_din_ns = 0;
    chip->last_clk_rise_ns = 0;
    chip->last_clk_fall_ns = 0;
    chip->last_load_rise_ns = 0;
    chip->saw_clk_rise = 0;
    chip->saw_clk_fall = 0;
    chip->saw_load_rise = 0;
    chip->fault = MAX7219_OK;
}

Pin *max7219_pin(Max7219 *chip, int number)
{
    if (number < 1 || number > MAX7219_PINS)
        return 0;
    return &chip->pin[number - 1];
}

void max7219_sense(Max7219 *chip, uint64_t now_ns)
{
    Sig din = chip->pin[MAX7219_DIN - 1].level;

    if (din != chip->prev_din) {
        chip->prev_din = din;
        chip->last_din_ns = now_ns;
    }
}

static void latch(Max7219 *chip, uint64_t now_ns)
{
    unsigned addr = (chip->shift >> 8) & 0x0F;
    uint8_t data = (uint8_t)(chip->shift & 0xFF);

    if (addr >= 1 && addr <= 8) {
        chip->digit[addr - 1] = data;
        return;
    }
    if (addr == 0x9) {
        chip->decode = data;
        return;
    }
    if (addr == 0xA) {
        chip->intensity = (uint8_t)(data & 0x0F);
        return;
    }
    if (addr == 0xB) {
        chip->scan_limit = (uint8_t)(data & 0x07);
        return;
    }
    if (addr == 0xC) {
        int normal = (data & 1) != 0;

        if (normal && chip->shutdown) {
            chip->wake_ns = now_ns + MAX7219_WAKE_NS;
            chip->scan_origin_ns = chip->wake_ns;
        }
        chip->shutdown = normal ? 0 : 1;
        return;
    }
    if (addr == 0xF)
        chip->display_test = (data & 1) != 0;
}

void max7219_edges(Max7219 *chip, uint64_t now_ns)
{
    Sig clk = chip->pin[MAX7219_CLK - 1].level;
    Sig din = chip->pin[MAX7219_DIN - 1].level;
    Sig load = chip->pin[MAX7219_LOAD - 1].level;

    if (din != chip->prev_din) {
        chip->prev_din = din;
        chip->last_din_ns = now_ns;
    }

    if (chip->prev_clk == SIG_L && clk == SIG_H) {
        if (chip->saw_clk_fall &&
            now_ns - chip->last_clk_fall_ns < MAX7219_TCL_MIN_NS)
            fault(chip, MAX7219_FAULT_TCL);
        if (chip->saw_clk_rise &&
            now_ns - chip->last_clk_rise_ns < MAX7219_TCP_MIN_NS)
            fault(chip, MAX7219_FAULT_TCP);
        if (now_ns - chip->last_din_ns < MAX7219_TDS_MIN_NS)
            fault(chip, MAX7219_FAULT_TDS);
        if (chip->saw_load_rise &&
            chip->last_load_rise_ns > chip->last_clk_rise_ns &&
            now_ns - chip->last_load_rise_ns < MAX7219_TLOAD_TO_CLK_NS)
            fault(chip, MAX7219_FAULT_TLOAD_CLK);

        chip->shift = (uint16_t)((chip->shift << 1) | (din == SIG_H ? 1 : 0));
        chip->last_clk_rise_ns = now_ns;
        chip->saw_clk_rise = 1;
    }

    if (chip->prev_clk == SIG_H && clk == SIG_L) {
        if (chip->saw_clk_rise &&
            now_ns - chip->last_clk_rise_ns < MAX7219_TCH_MIN_NS)
            fault(chip, MAX7219_FAULT_TCH);
        drive_out(chip, MAX7219_DOUT, (chip->shift & 0x8000) ? SIG_H : SIG_L);
        chip->last_clk_fall_ns = now_ns;
        chip->saw_clk_fall = 1;
    }

    if (chip->prev_load == SIG_L && load == SIG_H) {
        if (chip->saw_clk_rise &&
            now_ns - chip->last_clk_rise_ns < MAX7219_TCLK_TO_LOAD_NS)
            fault(chip, MAX7219_FAULT_TCLK_LOAD);
        latch(chip, now_ns);
        chip->last_load_rise_ns = now_ns;
        chip->saw_load_rise = 1;
    }

    if (chip->prev_load == SIG_H && load == SIG_L) {
        if (chip->saw_load_rise &&
            now_ns - chip->last_load_rise_ns < MAX7219_TCSW_MIN_NS)
            fault(chip, MAX7219_FAULT_TCSW);
    }

    chip->prev_clk = clk;
    chip->prev_load = load;
}

void max7219_pins(Max7219 *chip, uint64_t now_ns)
{
    int digit = 0;
    int lit = 0;
    int scanning = scan_phase(chip, now_ns, &digit, &lit);
    uint8_t bits = 0;
    int i;

    if (scanning && lit) {
        if (chip->display_test)
            bits = 0xFF;
        else
            bits = segments_of(chip, digit);
    }

    for (i = 0; i < 8; i++) {
        Sig seg = (scanning && lit && ((bits >> (7 - i)) & 1)) ? SIG_H : SIG_L;
        Sig dig = (scanning && lit && i == digit) ? SIG_L : SIG_H;

        drive_out(chip, seg_pin[i], seg);
        drive_out(chip, dig_pin[i], dig);
    }
}

uint8_t max7219_segment_byte(const Max7219 *chip, int digit)
{
    if (digit < 0 || digit > 7)
        return 0;
    return segments_of(chip, digit);
}

int max7219_led_on(const Max7219 *chip, int col, int row, uint64_t now_ns)
{
    uint8_t bits;

    if (col < 0 || col > 7 || row < 0 || row > 7)
        return 0;
    if (chip->display_test)
        return 1;
    if (chip->shutdown || now_ns < chip->wake_ns)
        return 0;
    if (row > (chip->scan_limit & 7))
        return 0;
    bits = segments_of(chip, row);
    return (bits >> (7 - col)) & 1u;
}

int max7219_active_digit(const Max7219 *chip, uint64_t now_ns)
{
    int digit = 0;
    int lit = 0;

    if (!scan_phase(chip, now_ns, &digit, &lit) || !lit)
        return -1;
    return digit;
}
