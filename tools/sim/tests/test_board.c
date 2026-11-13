#include "board.h"
#include "clocks.h"
#include "flush.h"
#include "frame.h"
#include "game.h"
#include "host.h"
#include "input.h"
#include "chips/max7219.h"
#include "status.h"

#include "tests/report.h"

#include <stdio.h>
#include <string.h>

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void draw_border(void)
{
    int x;
    int y;

    clear();
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            int edge = (x == 0 || x == 15 || y == 0 || y == 15);

            pixel((uint8_t)x, (uint8_t)y, edge);
        }
    }
}

static void show(Board *board)
{
    sim_bind(board);
    drivers_init();
    drivers_flush();
    board_advance(board, MAX7219_WAKE_NS);
}

static void test_switches(void)
{
    Board board;

    board_init(&board);
    expect(!board_led_on(&board, 0, 0), "power-up leaves the field dark");

    board_set_switches(&board, 1, 0, 0, 0, 1);
    expect(board_sense(&board, AVR_PD1) == SIG_L, "a closed switch pulls the input low");
    expect(board_sense(&board, AVR_PD2) == SIG_H, "an open switch stays high");
    expect(board_sense(&board, AVR_PD5) == SIG_L, "the button is package PD5");
    expect(board_sense(&board, AVR_PF6) == SIG_H, "RESET sits on its pull-up");
    board_set_switches(&board, 0, 0, 0, 0, 0);
    expect(board_sense(&board, AVR_PD1) == SIG_H, "releasing the switch returns the input high");
}

static void test_flush_border(void)
{
    Board board;
    int i;

    board_init(&board);
    draw_border();
    show(&board);
    if (board_fault(&board) != MAX7219_OK) {
        char line[64];

        for (i = 0; i < NDRV; i++) {
            if (board.drv[i].fault != MAX7219_OK) {
                snprintf(line, sizeof line, "%s timing fault %d",
                         board.drv[i].refdes, board.drv[i].fault);
                expect(0, line);
            }
        }
        return;
    }

    expect(board.drv[0].digit[0] == 0xFF, "top-left top row is full");
    expect(board.drv[0].digit[1] == 0x80, "top-left second row keeps the left edge");
    expect(board.drv[1].digit[1] == 0x01, "top-right second row keeps the right edge");
    expect(board.drv[2].digit[1] == 0x80, "bottom-left follows the same edge");
    expect(board.drv[3].digit[1] == 0x01, "the first word reached the last driver");
    expect(board.drv[0].decode == 0, "decode stays off");
    expect(board.drv[1].intensity == 0x0F, "intensity is full scale");
    expect(board.drv[2].scan_limit == 7, "scan limit is all eight digits");
    expect(board.drv[3].shutdown == 0, "the drivers have left shutdown");

    expect(board_led_on(&board, 0, 0), "top-left pixel is on");
    expect(board_led_on(&board, 15, 0), "top-right pixel is on");
    expect(board_led_on(&board, 0, 15), "bottom-left pixel is on");
    expect(board_led_on(&board, 15, 15), "bottom-right pixel is on");
    expect(board_led_on(&board, 0, 1), "left edge stays on");
    expect(!board_led_on(&board, 1, 1), "the inside of the border is off");
    expect(!board_led_on(&board, 8, 1), "the inside of the right block is off");
    expect(board_led_on(&board, 15, 1), "right edge stays on");

    board_advance(&board, 8 * MAX7219_SLOT_NS);
    expect(board_matrix_led_frame(&board, 0, 0), "matrix hardware sees top-left border pixel");
    expect(board_matrix_led_frame(&board, 15, 0), "matrix hardware sees top-right border pixel");
    expect(board_matrix_led_frame(&board, 0, 15), "matrix hardware sees bottom-left border pixel");
    expect(board_matrix_led_frame(&board, 15, 15), "matrix hardware sees bottom-right border pixel");
    expect(!board_matrix_led_frame(&board, 1, 1), "matrix hardware leaves inside border dark");

    expect(board.drv[4].decode == 0xFF, "U7 runs Code-B decode");
    expect(board.drv[4].shutdown == 0, "U7 is out of shutdown");
}

static void test_status_flush(void)
{
    Board board;

    board_init(&board);
    sim_bind(&board);
    status_set_digit(6, 4);
    status_set_digit(7, 2);
    drivers_init();
    drivers_flush();
    board_advance(&board, MAX7219_WAKE_NS);
    expect(board.drv[4].digit[6] == 4, "status tens lands in U7 digit 6");
    expect(board.drv[4].digit[7] == 2, "status ones lands in U7 digit 7");
    expect(max7219_segment_byte(&board.drv[4], 6) == 0x33u,
           "U7 Code-B tens pattern for digit 4");
    expect(max7219_segment_byte(&board.drv[4], 7) == 0x6Du,
           "U7 Code-B ones pattern for digit 2");

}

static void test_square_on_leds(void)
{
    Board board;
    Input in;

    board_init(&board);
    input_reset();
    game_reset();
    show(&board);
    expect(board_fault(&board) == MAX7219_OK, "the ship flush stays inside the timings");
    expect(board_led_on(&board, 7, 14), "the nose is lit");
    expect(!board_led_on(&board, 6, 14), "the shoulder beside the nose is dark");
    expect(board_led_on(&board, 6, 15) && board_led_on(&board, 8, 15),
           "the body row of the ship is lit");

    board_set_switches(&board, 0, 0, 0, 1, 0);
    input_tick(0, 0, 0, 1, 0, &in);
    game_tick(&in);
    drivers_flush();
    expect(board_led_on(&board, 6, 15), "debounce holds the ship for the first tick");

    input_tick(0, 0, 0, 1, 0, &in);
    game_tick(&in);
    drivers_flush();
    expect(!board_led_on(&board, 6, 15), "the old left column turns off");
    expect(board_led_on(&board, 9, 15), "the new right column turns on");
}

static uint8_t heard[4];
static int nheard;

static void hear(uint8_t index)
{
    if (nheard < 4)
        heard[nheard++] = index;
}

static void test_event(void)
{
    Board board;

    board_init(&board);
    expect(avr_sense(&board.audio, AVR_PA1) == SIG_H, "EVENT idles high at the audio pin");
    expect(strcmp(board.audio.refdes, "U5") == 0, "the audio AVR is U5");
    expect(board.cpu.pin[AVR_PA0 - 1].net == &board.event &&
               board.audio.pin[AVR_PA1 - 1].net == &board.event,
           "pin 22 and pin 23 share EVENT");
    expect(board.speaker[0].net == &board.gnd && board.speaker[0].level == SIG_L,
           "J5 pin 1 is ground");
    expect(board.speaker[1].net == &board.spk,
           "J5 pin 2 is speaker");

    board_drive(&board, AVR_PA0, SIG_Z);
    board_eval(&board);
    expect(avr_sense(&board.audio, AVR_PA1) == SIG_H, "the pull-up holds EVENT with PA0 released");
    expect(avr_sense(&board.cpu, AVR_PA0) == SIG_H, "the game pin sees that same high");

    nheard = 0;
    board_on_index(&board, hear);
    board_send(&board, 0xA5);
    board_advance(&board, 1000);
    expect(nheard == 0, "a queued index waits for the pulse ticks");
    board_advance(&board, 5000000);
    expect(nheard == 1 && heard[0] == 0xA5, "pin 23 decodes the byte on EVENT");

    nheard = 0;
    board_send(&board, 1);
    board_send(&board, 2);
    board_advance(&board, TICK_NS);
    expect(nheard == 2 && heard[0] == 1 && heard[1] == 2,
           "a laser and an explosion both cross the net");
}

int main(void)
{
    report_begin("board");
    test_switches();
    test_flush_border();
    test_status_flush();
    test_square_on_leds();
    test_event();
    return report_end();
}
