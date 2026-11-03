#include "acetate_geom.h"

#include "tests/report.h"

#include <stdio.h>
#include <string.h>

#define FX 8
#define FY 8
#define LED 6
#define PITCH 7
#define GRID 16

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void test_field_px(void)
{
    expect(acetate_geom_field_px(LED, PITCH, GRID) == 111, "16 LEDs of 6 with 1px gaps span 111");
}

static void test_led_rect_includes_margin(void)
{
    int x;
    int y;
    int w;
    int h;

    acetate_geom_led_rect(FX, FY, LED, PITCH, GRID, 0, 0, &x, &y, &w, &h);
    expect(x == FX && y == FY && w == PITCH && h == PITCH, "first LED owns 6x6 plus 1px margin");
    acetate_geom_led_rect(FX, FY, LED, PITCH, GRID, 15, 15, &x, &y, &w, &h);
    expect(w == LED && h == LED, "last LED has no trailing margin");
    expect(x == FX + 15 * PITCH && y == FY + 15 * PITCH, "last LED origin is at pitch * 15");
}

static void test_led_at_sprite_and_gap(void)
{
    int col = -1;
    int row = -1;

    expect(acetate_geom_led_at(FX, FY, LED, PITCH, GRID, FX, FY, &col, &row),
           "top-left LED sprite hits");
    expect(col == 0 && row == 0, "top-left is LED 0,0");
    expect(acetate_geom_led_at(FX, FY, LED, PITCH, GRID, FX + 6, FY, &col, &row),
           "1px gap after first LED still belongs to it");
    expect(col == 0 && row == 0, "gap maps to LED 0,0");
    expect(acetate_geom_led_at(FX, FY, LED, PITCH, GRID, FX + PITCH, FY, &col, &row),
           "next pitch starts LED 1");
    expect(col == 1 && row == 0, "second column is LED 1,0");
}

static void test_led_at_last_and_miss(void)
{
    int col = -1;
    int row = -1;
    int last = FX + 15 * PITCH;

    expect(acetate_geom_led_at(FX, FY, LED, PITCH, GRID, last + 5, FY, &col, &row),
           "last LED includes its 6px sprite");
    expect(col == 15 && row == 0, "last column is LED 15");
    expect(!acetate_geom_led_at(FX, FY, LED, PITCH, GRID, last + 6, FY, &col, &row),
           "one pixel past last LED misses");
    expect(!acetate_geom_led_at(FX, FY, LED, PITCH, GRID, FX - 1, FY, &col, &row),
           "left of field misses");
}

static void test_digit_at_whole_glyph(void)
{
    int origin_x = 15;
    int origin_y = 123;
    int dw = 14;
    int dh = 18;

    expect(acetate_geom_digit_at(origin_x, origin_y, dw, dh, 8, origin_x, origin_y) == 0,
           "first pixel of digit 0");
    expect(acetate_geom_digit_at(origin_x, origin_y, dw, dh, 8, origin_x + dw - 1,
                                origin_y + dh - 1) == 0,
           "last pixel of digit 0 still paints that digit");
    expect(acetate_geom_digit_at(origin_x, origin_y, dw, dh, 8, origin_x + dw, origin_y) == 1,
           "next glyph is digit 1");
    expect(acetate_geom_digit_at(origin_x, origin_y, dw, dh, 8, origin_x, origin_y - 1) == -1,
           "above digits misses");
}

static void test_color_at_led_and_digit(void)
{
    uint8_t led[GRID][GRID];
    uint8_t digit[8];

    memset(led, 0, sizeof led);
    memset(digit, 0, sizeof digit);
    led[0][1] = 4;
    digit[2] = 6;
    expect(acetate_geom_color_at(led, digit, FX, FY, LED, PITCH, GRID, 15, 123, 14, 18, 8, FX + PITCH,
                                 FY) == 4,
           "color_at reads painted LED");
    expect(acetate_geom_color_at(led, digit, FX, FY, LED, PITCH, GRID, 15, 123, 14, 18, 8, 15 + 2 * 14,
                                 123) == 6,
           "color_at reads painted digit");
    expect(acetate_geom_color_at(led, digit, FX, FY, LED, PITCH, GRID, 15, 123, 14, 18, 8, 0, 0) == 0,
           "unpainted canvas is clear");
}

static void test_parse_layout(void)
{
    uint8_t led[GRID][GRID];
    uint8_t digit[8];
    int active = 0;
    char led_s[GRID * GRID + 1];
    char json[512];

    memset(led_s, '0', GRID * GRID);
    led_s[0] = '4';
    led_s[GRID * GRID] = '\0';
    snprintf(json, sizeof json, "{\"active\":3,\"led\":\"%s\",\"digit\":\"16000000\"}", led_s);
    expect(acetate_geom_parse_layout(json, led, digit, &active) == 0, "parse cell layout JSON");
    expect(active == 3, "parsed active color");
    expect(led[0][0] == 4, "first LED color from led string");
    expect(digit[0] == 1 && digit[1] == 6, "digit colors from digit string");
    expect(acetate_geom_parse_layout("{\"islands\":[]}", led, digit, &active) != 0,
           "reject old island JSON");
}

static void test_fill_overlap_rectangle(void)
{
    uint8_t led[GRID][GRID];
    uint8_t digit[8];
    int x;
    int y;
    int w;
    int h;
    int ux;
    int uy;
    int uw;
    int uh;

    memset(led, 0, sizeof led);
    memset(digit, 0, sizeof digit);
    acetate_geom_led_rect(FX, FY, LED, PITCH, GRID, 1, 2, &x, &y, &w, &h);
    acetate_geom_led_rect(FX, FY, LED, PITCH, GRID, 3, 4, &ux, &uy, &uw, &uh);
    expect(acetate_geom_fill_overlap(led, digit, FX, FY, LED, PITCH, GRID, 15, 123, 14, 18, 8, x, y,
                                     (ux + uw) - x, (uy + uh) - y, 2) == 1,
           "fill overlap paints a LED rectangle");
    expect(led[2][1] == 2 && led[4][3] == 2 && led[3][2] == 2, "interior LEDs of the box are painted");
    expect(led[2][0] == 0 && led[1][1] == 0, "LEDs outside the box stay clear");
    expect(acetate_geom_union_overlap(FX, FY, LED, PITCH, GRID, 15, 123, 14, 18, 8, x, y,
                                      (ux + uw) - x, (uy + uh) - y, &ux, &uy, &uw, &uh),
           "union covers the painted LED block");
}

static void test_fill_overlap_digit(void)
{
    uint8_t led[GRID][GRID];
    uint8_t digit[8];

    memset(led, 0, sizeof led);
    memset(digit, 0, sizeof digit);
    expect(acetate_geom_fill_overlap(led, digit, FX, FY, LED, PITCH, GRID, 15, 123, 14, 18, 8, 15,
                                     123, 14 * 3, 18, 6) == 1,
           "fill overlap paints whole digits in the box");
    expect(digit[0] == 6 && digit[1] == 6 && digit[2] == 6, "first three digits are painted");
    expect(digit[3] == 0, "digit outside the box stays clear");
}

int main(void)
{
    report_begin("acetate");
    test_field_px();
    test_led_rect_includes_margin();
    test_led_at_sprite_and_gap();
    test_led_at_last_and_miss();
    test_digit_at_whole_glyph();
    test_color_at_led_and_digit();
    test_parse_layout();
    test_fill_overlap_rectangle();
    test_fill_overlap_digit();
    return report_end();
}
