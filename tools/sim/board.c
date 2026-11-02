#include "board.h"

#include "wire.h"
#include <stdio.h>

/* 40 kHz on the listener. 100 us on the sender. 100 us is four samples. */
#include "clocks.h"

/* 40 kHz EVENT sample period (console AVR at 20 MHz). */
#define SAMPLE_NS (1000000000ULL / 40000ULL)
/* 100 us low time for a 0 bit (cart EVENT timer at 12 MHz). */
#define SEND_NS 100000ULL

static const int sw_pin[5] = {
    AVR_PD1, AVR_PD2, AVR_PD3, AVR_PD4, AVR_PD5,
};

static const char *sw_name[5] = {
    "UP", "DOWN", "LEFT", "RIGHT", "BUTTON",
};

static const char *spk_name[2] = {
    "GND", "SPK",
};

static void settle_all(Board *board)
{
    int i;
    int k;
    int c;

    net_settle(&board->vcc);
    net_settle(&board->gnd);
    net_settle(&board->din);
    net_settle(&board->clk);
    net_settle(&board->load);
    net_settle(&board->dout);
    net_settle(&board->reset);
    net_settle(&board->updi);
    net_settle(&board->event);
    net_settle(&board->spk);
    for (i = 0; i < NDRV - 1; i++)
        net_settle(&board->cascade[i]);
    for (i = 0; i < 5; i++)
        net_settle(&board->sw[i]);
    for (k = 0; k < NDRV; k++) {
        for (c = 0; c < 16; c++)
            net_settle(&board->ds_net[k][c]);
    }
}

void board_eval(Board *board)
{
    int i;

    settle_all(board);
    for (i = 0; i < NDRV; i++)
        max7219_edges(&board->drv[i], board->now_ns);
    settle_all(board);
    for (i = 0; i < NDRV; i++)
        max7219_sense(&board->drv[i], board->now_ns);
    for (i = 0; i < NDRV; i++)
        max7219_pins(&board->drv[i], board->now_ns);
    settle_all(board);
    status_digits_sample(&board->status);
    for (i = 0; i < NMAT; i++)
        matrix8x8_sample(&board->mat[i]);
}

static void event_level(Board *board, int low)
{
    avr_drive(&board->cpu, AVR_PA0, low ? SIG_L : SIG_H);
}

static void event_start_bit(Board *board)
{
    EventWire *wire = &board->event_wire;
    uint8_t one = (uint8_t)((wire->cur >> wire->bit) & 1u);

    wire->low_left = one ? 2 : 1;
    wire->phase = 1;
    event_level(board, 1);
}

/* One cart tick. A 0 is low for one tick, a 1 for two, then high for one. */
static void event_tick(Board *board)
{
    EventWire *wire = &board->event_wire;

    if (wire->phase == 1) {
        wire->low_left--;
        if (wire->low_left == 0) {
            event_level(board, 0);
            wire->phase = 2;
        }
        return;
    }
    if (wire->phase == 2) {
        wire->bit++;
        if (wire->bit < 8) {
            event_start_bit(board);
            return;
        }
        wire->phase = 0;
    }
    if (wire->n == 0)
        return;
    wire->cur = wire->q[wire->r];
    wire->r = (uint8_t)((wire->r + 1) % EVENT_Q);
    wire->n--;
    wire->bit = 0;
    event_start_bit(board);
}

static void event_listen(Board *board)
{
    int index;

    wire_sample(avr_sense(&board->audio, AVR_PA1) == SIG_H);
    index = wire_take();
    if (index >= 0 && board->event_wire.on_index != 0)
        board->event_wire.on_index((uint8_t)index);
}

void board_send(Board *board, uint8_t index)
{
    EventWire *wire = &board->event_wire;

    if (wire->n >= EVENT_Q)
        return;
    wire->q[wire->w] = index;
    wire->w = (uint8_t)((wire->w + 1) % EVENT_Q);
    wire->n++;
}

void board_on_index(Board *board, void (*fn)(uint8_t index))
{
    board->event_wire.on_index = fn;
}

void board_advance(Board *board, uint64_t ns)
{
    uint64_t end = board->now_ns + ns;

    for (;;) {
        uint64_t next = board->event_wire.next_sample_ns;

        if (board->event_wire.next_send_ns < next)
            next = board->event_wire.next_send_ns;
        if (next > end)
            break;
        board->now_ns = next;
        if (board->now_ns == board->event_wire.next_send_ns) {
            event_tick(board);
            board->event_wire.next_send_ns += SEND_NS;
        }
        board_eval(board);
        if (board->now_ns == board->event_wire.next_sample_ns) {
            event_listen(board);
            board->event_wire.next_sample_ns += SAMPLE_NS;
        }
    }
    board->now_ns = end;
    board_eval(board);
}

void board_drive(Board *board, int avr_pin, Sig level)
{
    avr_drive(&board->cpu, avr_pin, level);
}

Sig board_sense(const Board *board, int avr_pin)
{
    return avr_sense(&board->cpu, avr_pin);
}

static void tie_chip_rails(Board *board, Max7219 *chip)
{
    net_tie(&board->vcc, max7219_pin(chip, MAX7219_VCC));
    net_tie(&board->gnd, max7219_pin(chip, MAX7219_GND1));
    net_tie(&board->gnd, max7219_pin(chip, MAX7219_GND2));
    net_tie(&board->clk, max7219_pin(chip, MAX7219_CLK));
    net_tie(&board->load, max7219_pin(chip, MAX7219_LOAD));
}

void board_init(Board *board)
{
    static const char *refdes[NDRV] = {
        "U1", "U2", "U3", "U4", "U7",
    };
    static const char *mat_refdes[NMAT] = {
        "DS1", "DS2", "DS3", "DS4",
    };
    /*
     * MAX7219 driver pin connected to each matrix pin (0 to F).
     * Exact match for MATRIX_DRV_PINS in tools/kicad/main-pcb/skidl/makineat.py,
     * makineat.net, and docs/driver_to_matrix_pinout.md.
     */
    static const int matrix_drv_pins[16] = {
        17, /* Pin 0: COL 7 (LSB) -> SEGG (pin 17) */
        15, /* Pin 1: COL 6       -> SEGF (pin 15) */
        11, /* Pin 2: ROW 1       -> DIG1 (pin 11) */
        22, /* Pin 3: COL 0 (MSB) -> SEGDP (pin 22) */
        7,  /* Pin 4: ROW 3       -> DIG3 (pin 7) */
        21, /* Pin 5: COL 5       -> SEGE (pin 21) */
        20, /* Pin 6: COL 3       -> SEGC (pin 20) */
        2,  /* Pin 7: ROW 0 (Top) -> DIG0 (pin 2) */
        3,  /* Pin 8: ROW 4       -> DIG4 (pin 3) */
        5,  /* Pin 9: ROW 6       -> DIG6 (pin 5) */
        14, /* Pin A: COL 1       -> SEGA (pin 14) */
        16, /* Pin B: COL 2       -> SEGB (pin 16) */
        8,  /* Pin C: ROW 7 (Btm) -> DIG7 (pin 8) */
        23, /* Pin D: COL 4       -> SEGD (pin 23) */
        10, /* Pin E: ROW 5       -> DIG5 (pin 10) */
        6,  /* Pin F: ROW 2       -> DIG2 (pin 6) */
    };
    static const char *mat_pin_hex[16] = {
        "0", "1", "2", "3", "4", "5", "6", "7",
        "8", "9", "A", "B", "C", "D", "E", "F"
    };
    int i;

    avr_init(&board->cpu, "U1");
    avr_init(&board->audio, "U5");
    avr_output(&board->cpu, AVR_PA0, SIG_H);
    for (i = 0; i < NDRV; i++)
        max7219_init(&board->drv[i], refdes[i]);
    status_digits_init(&board->status, "DS5");
    for (i = 0; i < NMAT; i++) {
        int p;

        matrix8x8_init(&board->mat[i], mat_refdes[i]);
        for (p = 0; p < 16; p++) {
            char net_name[16];

            snprintf(net_name, sizeof net_name, "DS%d_%s", i + 1, mat_pin_hex[p]);
            net_init(&board->ds_net[i][p], net_name, SIG_L);
            net_tie(&board->ds_net[i][p], max7219_pin(&board->drv[i], matrix_drv_pins[p]));
            net_tie(&board->ds_net[i][p], matrix8x8_pin(&board->mat[i], p));
        }
    }
    {
        int p;

        for (p = 0; p < 16; p++) {
            char net_name[16];

            snprintf(net_name, sizeof net_name, "DS5_%s", mat_pin_hex[p]);
            net_init(&board->ds_net[4][p], net_name, SIG_L);
            net_tie(&board->ds_net[4][p], max7219_pin(&board->drv[4], matrix_drv_pins[p]));
            net_tie(&board->ds_net[4][p], status_digits_pin(&board->status, p));
        }
    }

    for (i = 0; i < 5; i++) {
        board->contact[i].name = sw_name[i];
        board->contact[i].number = i + 1;
        board->contact[i].dir = PIN_OUT;
        board->contact[i].drive = SIG_Z;
        board->contact[i].level = SIG_H;
        board->contact[i].net = 0;
    }

    net_init(&board->vcc, "VCC", SIG_H);
    net_init(&board->gnd, "GND", SIG_L);
    net_init(&board->din, "DIN", SIG_L);
    net_init(&board->clk, "CLK", SIG_L);
    net_init(&board->load, "LOAD", SIG_H);
    net_init(&board->dout, "DOUT", SIG_L);
    net_init(&board->reset, "RESET", SIG_H);
    net_init(&board->updi, "UPDI", SIG_H);
    net_init(&board->event, "EVENT", SIG_H);
    net_init(&board->spk, "SPK", SIG_Z);
    for (i = 0; i < NDRV - 1; i++)
        net_init(&board->cascade[i], "CASCADE", SIG_L);
    for (i = 0; i < 5; i++)
        net_init(&board->sw[i], sw_name[i], SIG_H);

    net_tie(&board->vcc, avr_pin(&board->cpu, AVR_VDD));
    net_tie(&board->vcc, avr_pin(&board->cpu, AVR_AVDD));
    net_tie(&board->vcc, avr_pin(&board->cpu, AVR_VDDIO2));
    net_tie(&board->gnd, avr_pin(&board->cpu, AVR_GND1));
    net_tie(&board->gnd, avr_pin(&board->cpu, AVR_GND2));
    net_tie(&board->din, avr_pin(&board->cpu, AVR_PA4));
    net_tie(&board->clk, avr_pin(&board->cpu, AVR_PA6));
    net_tie(&board->load, avr_pin(&board->cpu, AVR_PA7));
    net_tie(&board->reset, avr_pin(&board->cpu, AVR_PF6));
    net_tie(&board->updi, avr_pin(&board->cpu, AVR_UPDI));
    net_tie(&board->event, avr_pin(&board->cpu, AVR_PA0));
    net_tie(&board->event, avr_pin(&board->audio, AVR_PA1));
    net_tie(&board->vcc, avr_pin(&board->audio, AVR_VDD));
    net_tie(&board->vcc, avr_pin(&board->audio, AVR_AVDD));
    net_tie(&board->vcc, avr_pin(&board->audio, AVR_VDDIO2));
    net_tie(&board->gnd, avr_pin(&board->audio, AVR_GND1));
    net_tie(&board->gnd, avr_pin(&board->audio, AVR_GND2));
    net_tie(&board->reset, avr_pin(&board->audio, AVR_PF6));

    net_tie(&board->din, max7219_pin(&board->drv[0], MAX7219_DIN));
    for (i = 0; i < NDRV; i++)
        tie_chip_rails(board, &board->drv[i]);
    for (i = 0; i < NMAT - 1; i++) {
        net_tie(&board->cascade[i], max7219_pin(&board->drv[i], MAX7219_DOUT));
        net_tie(&board->cascade[i], max7219_pin(&board->drv[i + 1], MAX7219_DIN));
    }
    net_tie(&board->cascade[NMAT - 1], max7219_pin(&board->drv[NMAT - 1], MAX7219_DOUT));
    net_tie(&board->cascade[NMAT - 1], max7219_pin(&board->drv[4], MAX7219_DIN));
    net_tie(&board->dout, max7219_pin(&board->drv[4], MAX7219_DOUT));

    for (i = 0; i < 5; i++) {
        net_tie(&board->sw[i], avr_pin(&board->cpu, sw_pin[i]));
        net_tie(&board->sw[i], &board->contact[i]);
    }

    for (i = 0; i < 2; i++) {
        board->speaker[i].name = spk_name[i];
        board->speaker[i].number = i + 1;
        board->speaker[i].dir = PIN_IN;
        board->speaker[i].drive = SIG_Z;
        board->speaker[i].level = SIG_Z;
        board->speaker[i].net = 0;
    }
    net_tie(&board->gnd, &board->speaker[0]);
    net_tie(&board->spk, &board->speaker[1]);

    board->event_wire.n = 0;
    board->event_wire.r = 0;
    board->event_wire.w = 0;
    board->event_wire.phase = 0;
    board->event_wire.on_index = 0;
    board->event_wire.next_sample_ns = SAMPLE_NS;
    board->event_wire.next_send_ns = SEND_NS;
    board->now_ns = 0;
    wire_reset();
    board_eval(board);
}

void board_set_switches(Board *board, int up, int down, int left, int right, int button)
{
    int closed[5];
    int i;

    closed[0] = up;
    closed[1] = down;
    closed[2] = left;
    closed[3] = right;
    closed[4] = button;
    for (i = 0; i < 5; i++)
        board->contact[i].drive = closed[i] ? SIG_L : SIG_Z;
    board_eval(board);
}

int board_led_on(const Board *board, int x, int y)
{
    int block;

    if (x < 0 || x > 15 || y < 0 || y > 15)
        return 0;
    block = (y / 8) * 2 + (x / 8);
    return max7219_led_on(&board->drv[block], x & 7, y & 7, board->now_ns);
}

int board_matrix_led_active(const Board *board, int x, int y)
{
    int block;

    if (x < 0 || x > 15 || y < 0 || y > 15)
        return 0;
    block = (y / 8) * 2 + (x / 8);
    return matrix8x8_led_active(&board->mat[block], x & 7, y & 7);
}

int board_matrix_led_frame(const Board *board, int x, int y)
{
    int block;

    if (x < 0 || x > 15 || y < 0 || y > 15)
        return 0;
    block = (y / 8) * 2 + (x / 8);
    return matrix8x8_led_frame(&board->mat[block], x & 7, y & 7);
}

void board_matrix_sample(Board *board)
{
    int i;

    for (i = 0; i < NMAT; i++)
        matrix8x8_sample(&board->mat[i]);
}

void board_matrix_clear_frame(Board *board)
{
    int i;

    for (i = 0; i < NMAT; i++)
        matrix8x8_clear_frame(&board->mat[i]);
}

const Max7219 *board_status_driver(const Board *board)
{
    if (board == NULL)
        return NULL;
    return &board->drv[4];
}

uint8_t board_status_digit_mask(const Board *board, int digit)
{
    if (board == NULL)
        return 0;
    return status_digits_frame_mask(&board->status, digit);
}

int board_fault(const Board *board)
{
    int i;

    for (i = 0; i < NDRV; i++) {
        if (board->drv[i].fault != MAX7219_OK)
            return board->drv[i].fault;
    }
    return MAX7219_OK;
}
