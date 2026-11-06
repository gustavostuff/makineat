#include "stage.h"

#include "frame.h"
#include "tests/report.h"

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static const uint16_t tiny_rows[] = {
    0x0001, 0x0001,
};

static const StageMap tiny = {32, 1, 2, 0, 0, tiny_rows};

static void test_map_get(void)
{
    expect(stage_map_get(&tiny, 0, 0), "bit 0 of row 0 is set");
    expect(!stage_map_get(&tiny, 1, 0), "neighbor cell is clear");
    expect(stage_map_get(&tiny, 16, 0), "second word column is addressable");
    expect(!stage_map_get(&tiny, -1, 0), "out of bounds reads empty");
}

static void test_h_paint_and_camera(void)
{
    StageView view;

    stage_view_init(&view);
    stage_bind_h(&view, &tiny);
    stage_h_paint(&view);
    expect(pixel_get(0, 0), "home camera shows the left marker");
    stage_h_camera_set(&view, 1);
    stage_h_paint(&view);
    expect(!pixel_get(0, 0) && pixel_get(15, 0), "camera offset shifts visible art");
}

int main(void)
{
    report_begin("stage");
    test_map_get();
    test_h_paint_and_camera();
    return report_end();
}
