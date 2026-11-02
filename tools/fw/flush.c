#include "flush.h"

#include "frame.h"
#include "hal.h"
#include "status.h"

#define CHAIN 5

static void shift_bit(int bit)
{
    hal_din(bit ? 1 : 0);
    hal_step();
    hal_clk(1);
    hal_step();
    hal_clk(0);
    hal_step();
}

static void send_word(unsigned word)
{
    int i;

    for (i = 15; i >= 0; i--)
        shift_bit((word >> i) & 1);
}

/* The first word lands in the last driver. CLK is left low, DIN low, LOAD high. */
static void send_words(const unsigned *word, int n)
{
    int i;

    hal_load(0);
    hal_step();
    for (i = n - 1; i >= 0; i--)
        send_word(word[i]);
    hal_din(0);
    hal_step();
    hal_load(1);
    hal_step();
}

static unsigned reg_word(unsigned addr, unsigned data)
{
    return ((addr & 0x0Fu) << 8) | (data & 0xFFu);
}

/* Bit 0 of the row is the left pixel. Bit 7 of the driver byte is that pixel. */
static unsigned pack_byte(uint16_t row, int origin)
{
    unsigned byte = 0;
    int i;

    for (i = 0; i < 8; i++) {
        if ((row >> (origin + i)) & 1u)
            byte |= 1u << (7 - i);
    }
    return byte;
}

static void send_mixed(unsigned addr, unsigned matrix_data, unsigned status_data)
{
    unsigned word[CHAIN];
    int i;

    for (i = 0; i < CHAIN - 1; i++)
        word[i] = reg_word(addr, matrix_data);
    word[CHAIN - 1] = reg_word(addr, status_data);
    send_words(word, CHAIN);
}

void drivers_init(void)
{
    int d;
    const uint8_t *status = status_digits();

    send_mixed(0x0F, 0x00, 0x00);
    send_mixed(0x09, 0x00, 0xFF);
    send_mixed(0x0A, 0x0F, 0x0F);
    send_mixed(0x0B, 0x07, 0x07);
    for (d = 0; d < 8; d++)
        send_mixed((unsigned)(d + 1), 0x00, status[d]);
    send_mixed(0x0C, 0x01, 0x01);
}

void drivers_flush(void)
{
    int d;
    const uint8_t *status = status_digits();

    for (d = 0; d < 8; d++) {
        unsigned word[CHAIN];
        int b;

        for (b = 0; b < CHAIN - 1; b++) {
            int br = b / 2;
            int bc = b % 2;

            word[b] = reg_word((unsigned)(d + 1),
                               pack_byte(frame[br * 8 + d], bc * 8));
        }
        word[CHAIN - 1] = reg_word((unsigned)(d + 1), status[d]);
        send_words(word, CHAIN);
    }
}
