#include "acetate_geom.h"

#include "tests/report.h"

#define TEST_CANVAS_W 128
#define TEST_MAX_Y 141

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void test_min_size(void)
{
    AcetateRect small = {0, 0, 3, 4, 1};
    AcetateRect ok = {0, 0, 4, 4, 1};

    expect(!acetate_geom_size_ok(&small, ACETATE_GEOM_MIN_ISLAND),
           "3x4 island fails minimum size");
    expect(acetate_geom_size_ok(&ok, ACETATE_GEOM_MIN_ISLAND), "4x4 island meets minimum size");
}

static void test_max_y_clamp_bounds(void)
{
    AcetateRect r = {0, 122, 128, 30, 4};

    acetate_geom_clamp_bounds(&r, TEST_CANVAS_W, TEST_MAX_Y);
    expect(r.y + r.h == TEST_MAX_Y, "bounds clamp trims height to max Y");
    expect(r.h == TEST_MAX_Y - 122, "green band height capped at max Y");
}

static void test_move_keeps_size_at_max_y(void)
{
    AcetateRect r = {0, 100, 40, 20, 1};
    int h_before = r.h;

    acetate_geom_clamp_move(&r, TEST_CANVAS_W, TEST_MAX_Y);
    expect(r.h == h_before, "move clamp does not shrink height at bottom edge");
    expect(r.y + r.h <= TEST_MAX_Y, "move clamp slides island below max Y");
}

static void test_color_at_topmost(void)
{
    AcetateRect stack[2] = {
        {0, 0, 10, 10, 1},
        {5, 5, 10, 10, 4},
    };

    expect(acetate_geom_color_at(stack, 2, 7, 7) == 4, "topmost island color wins in overlap");
    expect(acetate_geom_color_at(stack, 2, 1, 1) == 1, "non-overlap uses lower island");
    expect(acetate_geom_color_at(stack, 2, 50, 50) == 0, "outside islands is clear");
}

static void test_color_at_skips_clear(void)
{
    AcetateRect stack[2] = {
        {0, 0, 20, 20, 0},
        {0, 0, 20, 20, 3},
    };

    expect(acetate_geom_color_at(stack, 2, 5, 5) == 3, "transparent island is skipped for tint");
}

static void test_send_to_front(void)
{
    AcetateRect stack[2] = {
        {0, 0, 10, 10, 1},
        {5, 5, 10, 10, 4},
    };
    int selected = 0;
    int hover = 0;

    acetate_geom_send_to_front(stack, 2, 0, &selected, &hover);
    expect(acetate_geom_color_at(stack, 2, 7, 7) == 1, "send_to_front promotes moved island");
    expect(selected == 1, "selected index tracks send_to_front");
    expect(hover == 1, "hover index tracks send_to_front");
}

static void test_parse_island(void)
{
    AcetateRect r;
    const char *obj = "{\"x\":10,\"y\":20,\"w\":30,\"h\":40,\"c\":5}";

    expect(acetate_geom_parse_island(obj, &r) == 0, "parse island JSON object");
    expect(r.x == 10 && r.y == 20 && r.w == 30 && r.h == 40 && r.color == 5,
           "parsed fields match JSON");
    expect(acetate_geom_parse_island("{\"x\":bad}", &r) != 0, "reject malformed island JSON");
}

static void test_normalize_negative_size(void)
{
    AcetateRect r = {10, 10, -5, -8, 2};

    acetate_geom_normalize(&r);
    expect(r.x == 5 && r.y == 2 && r.w == 5 && r.h == 8, "normalize flips negative width and height");
}

static void test_control_rect_hit(void)
{
    expect(acetate_geom_point_in_rect(16, 152, 16, 152, 32, 32), "stick rect contains interior point");
    expect(!acetate_geom_point_in_rect(0, 0, 16, 152, 32, 32), "stick rect excludes far point");
}

static void test_load_filter_small_island(void)
{
    AcetateRect r;

    expect(acetate_geom_parse_island("{\"x\":0,\"y\":0,\"w\":1,\"h\":1,\"c\":1}", &r) == 0,
           "parse tiny island for load filter");
    expect(!acetate_geom_size_ok(&r, ACETATE_GEOM_MIN_ISLAND),
           "load rejects island below minimum size");
}

int main(void)
{
    report_begin("acetate");
    test_min_size();
    test_max_y_clamp_bounds();
    test_move_keeps_size_at_max_y();
    test_color_at_topmost();
    test_color_at_skips_clear();
    test_send_to_front();
    test_parse_island();
    test_normalize_negative_size();
    test_control_rect_hit();
    test_load_filter_small_island();
    return report_end();
}
