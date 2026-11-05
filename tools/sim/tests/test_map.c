#include "entity.h"
#include "frame.h"
#include "map.h"
#include "tests/report.h"

static const Shape dot = SHAPE1(0, 0, "1");

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static void test_rect_clip(void)
{
    map_clear();
    map_rect(FIELD_LAST - 1, FIELD_LAST - 1, 4, 4, 1);
    expect(map_get(FIELD_LAST, FIELD_LAST) && map_get(FIELD_LAST - 1, FIELD_LAST - 1) &&
               !map_get(FIELD_LAST - 2, FIELD_LAST - 2),
           "map_rect clips to the playfield");
    map_clear();
    map_rect(-2, 0, 3, 1, 1);
    expect(map_get(0, 0) && !map_get(1, 0),
           "map_rect clips a negative origin to visible columns");
}

static void test_stamp_or(void)
{
    map_clear();
    map_set(1, 1, 1);
    pixel(0, 0, true);
    pixel(2, 2, true);
    map_stamp_frame();
    expect(pixel_get(0, 0) && pixel_get(1, 1) && pixel_get(2, 2),
           "map_stamp_frame ORs into the frame without clearing");
}

static void test_scene_blocked(void)
{
    int id;

    map_clear();
    world_reset();
    map_set(5, 5, 1);
    expect(scene_blocked(5, 5, SCENE_MAP) && !scene_blocked(5, 5, SCENE_ENTITIES),
           "scene_blocked respects SCENE_MAP");
    expect(!scene_blocked(0, 0, SCENE_MAP), "an empty map cell is open");

    id = entity_spawn(&dot, 3, 3, 0, 0, 1, 0);
    expect(id >= 0, "spawn for scene test");
    expect(scene_blocked(3, 3, SCENE_ENTITIES) && !scene_blocked(3, 3, SCENE_MAP),
           "scene_blocked respects SCENE_ENTITIES");
    expect(scene_blocked(3, 3, SCENE_MAP | SCENE_ENTITIES),
           "scene_blocked ORs map and entities");
    expect(!scene_blocked(4, 4, SCENE_MAP | SCENE_ENTITIES),
           "a clear cell is not blocked");
}

static void test_compose_paint(void)
{
    int id;

    map_clear();
    world_reset();
    map_set(2, 2, 1);
    id = entity_spawn(&dot, 2, 2, 0, 0, 1, 0);
    expect(id >= 0, "spawn on map cell");
    clear();
    map_stamp_frame();
    world_paint();
    expect(pixel_get(2, 2), "entity paints over a map cell at the same coordinate");
}

static void test_snake_ready(void)
{
    int hx = 8;
    int hy = 8;
    int nx = hx + 1;
    int ny = hy;

    map_clear();
    map_set(hx, hy, 1);
    map_set(hx - 1, hy, 1);
    map_set(hx - 2, hy, 1);
    expect(scene_blocked(nx, ny, SCENE_MAP) == 0, "head can move into an open cell");
    map_set(nx, ny, 1);
    expect(scene_blocked(nx, ny, SCENE_MAP), "head cannot move into map body");
    clear();
    map_stamp_frame();
    expect(pixel_get(hx, hy) && pixel_get(hx - 1, hy),
           "snake body draws from the map");
}

int main(void)
{
    report_begin("test_map");
    test_rect_clip();
    test_stamp_or();
    test_scene_blocked();
    test_compose_paint();
    test_snake_ready();
    return report_end();
}
