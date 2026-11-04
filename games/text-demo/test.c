#include "text_demo.h"

#include "frame.h"
#include "input.h"
#include "tests/report.h"
#include "text.h"

#include <string.h>

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void feed(int up, int down, int left, int right, int button)
{
    Input in;

    input_tick(up, down, left, right, button, &in);
    text_demo_tick(&in);
}

static void test_initial_state(void)
{
    input_reset();
    text_demo_reset();

    expect(text_demo_demo_index() == 0, "starts on the print demo");
    expect(text_demo_scroll_x() == 0, "print demo keeps scroll at zero");
    expect(text_demo_scroll_y() == 7, "body text starts on row 7");
}

static void test_print_idle(void)
{
    int i;

    input_reset();
    text_demo_reset();
    for (i = 0; i < 60; i++)
        feed(0, 0, 0, 0, 0);
    expect(text_demo_scroll_x() == 0, "print demo stays still without input");
}

static void test_demo_switch(void)
{
    input_reset();
    text_demo_reset();

    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    expect(text_demo_demo_index() == 1, "stick right selects bounce scroll");

    feed(0, 0, 1, 0, 0);
    feed(0, 0, 1, 0, 0);
    expect(text_demo_demo_index() == 0, "stick left returns to print");

    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    expect(text_demo_demo_index() == 1, "bounce scroll is selected again");
}

static void test_bounce_scroll_moves(void)
{
    int i;
    int x0;

    input_reset();
    text_demo_reset();
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    x0 = text_demo_scroll_x();
    for (i = 0; i < 120; i++)
        feed(0, 0, 0, 0, 0);
    expect(text_demo_scroll_x() < x0, "bounce scroll moves the string left");
}

static void test_button_restarts_demo(void)
{
    int i;

    input_reset();
    text_demo_reset();
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    for (i = 0; i < 120; i++)
        feed(0, 0, 0, 0, 0);
    expect(text_demo_scroll_x() < 0, "bounce scroll moved before reset");

    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(text_demo_scroll_x() == 0, "button restarts the active scroll demo");
}

static void test_all_demos_exist(void)
{
    expect(text_demo_demo_count() == 7, "seven text flows are available");
}

static void select_demo(int index)
{
    input_reset();
    text_demo_reset();
    while (text_demo_demo_index() != index) {
        feed(0, 0, 0, 1, 0);
        feed(0, 0, 0, 1, 0);
        feed(0, 0, 0, 0, 0);
        feed(0, 0, 0, 0, 0);
    }
}

static void test_outline_scrolls(void)
{
    int i;
    int x0;

    select_demo(5);
    expect(text_demo_demo_index() == 5, "outline demo is selected");
    x0 = text_demo_scroll_x();
    for (i = 0; i < 120; i++)
        feed(0, 0, 0, 0, 0);
    expect(text_demo_scroll_x() < x0, "outline demo scrolls the sample text");
}

static void fill_on(void)
{
    int x;
    int y;

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++)
            pixel((uint8_t)x, (uint8_t)y, true);
    }
}

static void test_outline_punches(void)
{
    clear();
    fill_on();
    text_print_outline(0, 0, "I");
    expect(pixel_get(0, 0) && pixel_get(1, 0) && pixel_get(2, 0),
           "outline keeps the glyph pixels on");
    expect(!pixel_get(0, 1) && pixel_get(1, 1) && !pixel_get(2, 1),
           "outline punches the sides of I");
    expect(!pixel_get(3, 0) && !pixel_get(1, 3),
           "outline punches the 1px ring around I");
}

static void test_outline_blink(void)
{
    TextBlink tb;
    int i;

    clear();
    fill_on();
    memset(&tb, 0, sizeof tb);
    text_blink_start(&tb);
    text_print_outline_blink(0, 0, "I", &tb);
    expect(pixel_get(1, 0) && !pixel_get(0, 1),
           "blink starts with a normal outline");

    for (i = 0; i < 29; i++) {
        clear();
        fill_on();
        text_print_outline_blink(0, 0, "I", &tb);
    }
    expect(!pixel_get(1, 0) && pixel_get(0, 1),
           "at 2 FPS the fill and ring swap after half a second");
}

static void test_font_coverage(void)
{
    const char *msg = text_demo_message();
    int has_letter[26] = {0};
    int has_digit[10] = {0};
    int has_dot = 0, has_comma = 0, has_colon = 0, has_semi = 0;
    int has_dash = 0, has_plus = 0, has_equals = 0, has_slash = 0;
    int i;

    for (i = 0; msg[i] != '\0'; i++) {
        char c = msg[i];

        if (c >= 'A' && c <= 'Z')
            has_letter[c - 'A'] = 1;
        if (c >= '0' && c <= '9')
            has_digit[c - '0'] = 1;
        if (c == '.')
            has_dot = 1;
        if (c == ',')
            has_comma = 1;
        if (c == ':')
            has_colon = 1;
        if (c == ';')
            has_semi = 1;
        if (c == '-')
            has_dash = 1;
        if (c == '+')
            has_plus = 1;
        if (c == '=')
            has_equals = 1;
        if (c == '/')
            has_slash = 1;

        expect(text_glyph(c) != 0, "glyph exists for message character");
    }

    for (i = 0; i < 26; i++)
        expect(has_letter[i], "pangram covers all 26 letters A-Z");

    for (i = 0; i < 10; i++)
        expect(has_digit[i], "message covers all 10 digits 0-9");

    expect(has_dot, "message includes dot");
    expect(has_comma, "message includes comma");
    expect(has_colon, "message includes colon");
    expect(has_semi, "message includes semicolon");
    expect(has_dash, "message includes dash");
    expect(has_plus, "message includes plus");
    expect(has_equals, "message includes equals");
    expect(has_slash, "message includes slash");
}

int main(void)
{
    report_begin("text-demo");
    test_initial_state();
    test_print_idle();
    test_demo_switch();
    test_bounce_scroll_moves();
    test_button_restarts_demo();
    test_all_demos_exist();
    test_outline_scrolls();
    test_outline_punches();
    test_outline_blink();
    test_font_coverage();
    return report_end();
}
