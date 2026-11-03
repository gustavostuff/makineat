#include "text_test.h"

#include "frame.h"
#include "rand.h"
#include "text.h"

#include <string.h>

#define TITLE_Y 2
#define BODY_Y 7
#define FOOTER_Y 11

/* Last five playfield rows (bit 0 is the left column). */
static const uint16_t footer_rows[5] = {
    0x2004u,
    0x4002u,
    0x8001u,
    0x4002u,
    0x2004u,
};

static const char sample[] = "this is a test text";

static const char message[] =
    "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG - 0123456789 - . , : ; - + = / ";

typedef enum {
    DEMO_PRINT,
    DEMO_SCROLL_BOUNCE,
    DEMO_SCROLL_RESET,
    DEMO_SCROLL_VIEW8,
    DEMO_SCROLL_FAST,
    DEMO_OUTLINE,
    DEMO_BLINK,
    DEMO_COUNT
} DemoKind;

typedef struct {
    const char *title;
    DemoKind kind;
} Demo;

static const Demo demos[DEMO_COUNT] = {
    {"PRINT", DEMO_PRINT},
    {"BOUNCE", DEMO_SCROLL_BOUNCE},
    {"RESET", DEMO_SCROLL_RESET},
    {"VIEW 8", DEMO_SCROLL_VIEW8},
    {"FAST", DEMO_SCROLL_FAST},
    {"OUTLINE", DEMO_OUTLINE},
    {"BLINK", DEMO_BLINK},
};

static int demo_index;
static TextScroll title_scroll;
static TextScroll body_scroll;
static TextBlink body_blink;

static void title_scroll_begin(void)
{
    text_scroll_stop(&title_scroll);
    memset(&title_scroll, 0, sizeof title_scroll);
    title_scroll.repeat = TEXT_SCROLL_BOUNCE;
    text_scroll_start(&title_scroll, 0, TITLE_Y, demos[demo_index].title);
}

static void body_scroll_stop(void)
{
    text_scroll_stop(&body_scroll);
}

static void demo_begin(void)
{
    DemoKind kind = demos[demo_index].kind;

    title_scroll_begin();
    body_scroll_stop();
    memset(&body_scroll, 0, sizeof body_scroll);

    switch (kind) {
    case DEMO_PRINT:
        break;
    case DEMO_SCROLL_BOUNCE:
        body_scroll.repeat = TEXT_SCROLL_BOUNCE;
        text_scroll_start(&body_scroll, 0, BODY_Y, sample);
        break;
    case DEMO_SCROLL_RESET:
        body_scroll.repeat = TEXT_SCROLL_RESET;
        text_scroll_start(&body_scroll, 0, BODY_Y, sample);
        break;
    case DEMO_SCROLL_VIEW8:
        body_scroll.repeat = TEXT_SCROLL_BOUNCE;
        body_scroll.viewport_w = 8;
        text_scroll_start(&body_scroll, 4, BODY_Y, sample);
        break;
    case DEMO_SCROLL_FAST:
        body_scroll.repeat = TEXT_SCROLL_BOUNCE;
        body_scroll.speed_fps = 60;
        text_scroll_start(&body_scroll, 0, BODY_Y, sample);
        break;
    case DEMO_OUTLINE:
        body_scroll.repeat = TEXT_SCROLL_BOUNCE;
        text_scroll_start(&body_scroll, 0, BODY_Y, sample);
        break;
    case DEMO_BLINK:
        body_scroll.repeat = TEXT_SCROLL_BOUNCE;
        text_scroll_start(&body_scroll, 0, BODY_Y, sample);
        memset(&body_blink, 0, sizeof body_blink);
        text_blink_start(&body_blink);
        break;
    default:
        break;
    }
}

static void noise_draw(void)
{
    int x;
    int y;

    for (y = 5; y <= 11; y++) {
        for (x = 0; x < 16; x++) {
            uint8_t n = (uint8_t)((x * 7) ^ (y * 13) ^ (x * y));

            if (n & 1u)
                pixel((uint8_t)x, (uint8_t)y, true);
        }
    }
}

static void footer_draw(void)
{
    int row;
    int y;

    for (row = 0; row < 5; row++) {
        uint16_t bits = footer_rows[row];
        int x;

        y = FOOTER_Y + row;
        for (x = 0; x < 16; x++) {
            if ((bits & (uint16_t)(1u << x)) != 0)
                pixel((uint8_t)x, (uint8_t)y, true);
        }
    }
}

static void frame_draw(void)
{
    DemoKind kind = demos[demo_index].kind;

    clear();
    if (kind == DEMO_OUTLINE || kind == DEMO_BLINK)
        noise_draw();
    else
        footer_draw();
    print_scroll(&title_scroll);
    switch (kind) {
    case DEMO_PRINT:
        text_print(0, BODY_Y, sample);
        break;
    case DEMO_SCROLL_BOUNCE:
    case DEMO_SCROLL_RESET:
    case DEMO_SCROLL_VIEW8:
    case DEMO_SCROLL_FAST:
        print_scroll(&body_scroll);
        break;
    case DEMO_OUTLINE:
        print_scroll_outline(&body_scroll);
        break;
    case DEMO_BLINK:
        print_scroll_outline_blink(&body_scroll, &body_blink);
        break;
    default:
        break;
    }
}

void text_test_reset(void)
{
    rand_reseed();
    demo_index = 0;
    demo_begin();
    frame_draw();
}

int text_test_demo_index(void)
{
    return demo_index;
}

int text_test_demo_count(void)
{
    return DEMO_COUNT;
}

int text_test_scroll_x(void)
{
    if (demos[demo_index].kind == DEMO_PRINT)
        return 0;
    return body_scroll.scroll_x;
}

int text_test_scroll_y(void)
{
    return BODY_Y;
}

const char *text_test_message(void)
{
    return message;
}

void text_test_tick(const Input *in)
{
    rand_feed(in);
    if (in == 0) {
        frame_draw();
        return;
    }

    if (in->button_pressed)
        demo_begin();

    if (in->left_pressed) {
        demo_index = (demo_index + DEMO_COUNT - 1) % DEMO_COUNT;
        demo_begin();
    } else if (in->right_pressed) {
        demo_index = (demo_index + 1) % DEMO_COUNT;
        demo_begin();
    }

    frame_draw();
}
