#include "text_test.h"

#include "input.h"
#include "tests/report.h"
#include "text.h"

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void feed(int up, int down, int left, int right, int button)
{
    Input in;

    input_tick(up, down, left, right, button, &in);
    text_test_tick(&in);
}

static void test_initial_state(void)
{
    input_reset();
    text_test_reset();

    expect(text_test_demo_index() == 0, "starts on the print demo");
    expect(text_test_scroll_x() == 0, "print demo keeps scroll at zero");
    expect(text_test_scroll_y() == 7, "body text starts on row 7");
}

static void test_print_idle(void)
{
    int i;

    input_reset();
    text_test_reset();
    for (i = 0; i < 60; i++)
        feed(0, 0, 0, 0, 0);
    expect(text_test_scroll_x() == 0, "print demo stays still without input");
}

static void test_demo_switch(void)
{
    input_reset();
    text_test_reset();

    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    expect(text_test_demo_index() == 1, "stick right selects bounce scroll");

    feed(0, 0, 1, 0, 0);
    feed(0, 0, 1, 0, 0);
    expect(text_test_demo_index() == 0, "stick left returns to print");

    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    expect(text_test_demo_index() == 1, "bounce scroll is selected again");
}

static void test_bounce_scroll_moves(void)
{
    int i;
    int x0;

    input_reset();
    text_test_reset();
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    x0 = text_test_scroll_x();
    for (i = 0; i < 120; i++)
        feed(0, 0, 0, 0, 0);
    expect(text_test_scroll_x() < x0, "bounce scroll moves the string left");
}

static void test_button_restarts_demo(void)
{
    int i;

    input_reset();
    text_test_reset();
    feed(0, 0, 0, 1, 0);
    feed(0, 0, 0, 1, 0);
    for (i = 0; i < 120; i++)
        feed(0, 0, 0, 0, 0);
    expect(text_test_scroll_x() < 0, "bounce scroll moved before reset");

    feed(0, 0, 0, 0, 1);
    feed(0, 0, 0, 0, 1);
    expect(text_test_scroll_x() == 0, "button restarts the active scroll demo");
}

static void test_all_demos_exist(void)
{
    expect(text_test_demo_count() == 5, "five text flows are available");
}

static void test_font_coverage(void)
{
    const char *msg = text_test_message();
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
    report_begin("text-test");
    test_initial_state();
    test_print_idle();
    test_demo_switch();
    test_bounce_scroll_moves();
    test_button_restarts_demo();
    test_all_demos_exist();
    test_font_coverage();
    return report_end();
}
