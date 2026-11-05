#include "synth.h"
#include "tests/report.h"
#include "wire.h"

static void expect(int cond, const char *msg)
{
    report_check(cond, msg);
}

static int remember(uint16_t *seen, int n, int cap, uint16_t sample)
{
    int i;

    for (i = 0; i < n; i++) {
        if (seen[i] == sample)
            return n;
    }
    if (n < cap)
        seen[n++] = sample;
    return n;
}

static void pulse(int samples, int high)
{
    int i;

    for (i = 0; i < samples; i++)
        wire_sample(high);
}

static void send(uint8_t byte)
{
    int bit;

    for (bit = 0; bit < 8; bit++) {
        pulse(((byte >> bit) & 1) ? 8 : 4, 0);
        pulse(4, 1);
    }
}

static void test_catalog(void)
{
    uint16_t seen[32];
    int n = 0;
    int i;
    int guard;

    synth_reset();
    synth_play(0);
    synth_play(4);
    expect(!synth_on(SYNTH_SQUARE) && !synth_on(SYNTH_TRIANGLE) &&
               !synth_on(SYNTH_NOISE),
           "an empty index leaves every channel quiet");
    expect(synth_sample() == 512, "silence sits at midscale");
    expect(synth_volume_get() == SYNTH_VOLUME_DEFAULT, "volume starts at the default percent");

    synth_play(1);
    expect(synth_on(SYNTH_TRIANGLE) && !synth_on(SYNTH_SQUARE) &&
               !synth_on(SYNTH_NOISE),
           "the laser uses the triangle");
    for (i = 0; i < 24; i++)
        n = remember(seen, n, 32, synth_sample());
    expect(n >= 4, "the triangle moves through several levels");
    guard = 24;
    while (synth_on(SYNTH_TRIANGLE) && guard < 4000) {
        synth_sample();
        guard++;
    }
    expect(guard == 3500, "the laser lasts 3500 samples");
    expect(!synth_on(SYNTH_TRIANGLE), "the laser ends on its own");
    expect(synth_sample() == 512, "the mix is quiet after the laser");

    synth_reset();
    synth_play(2);
    expect(synth_on(SYNTH_NOISE) && !synth_on(SYNTH_SQUARE) &&
               !synth_on(SYNTH_TRIANGLE),
           "the explosion uses noise");
    n = 0;
    for (i = 0; i < 32; i++)
        n = remember(seen, n, 32, synth_sample());
    expect(n == 2, "one noise step is two levels");
    guard = 32;
    while (synth_on(SYNTH_NOISE) && guard < 10000) {
        synth_sample();
        guard++;
    }
    expect(guard == 9000, "the explosion lasts 9000 samples");

    synth_reset();
    synth_play(1);
    synth_play(2);
    expect(synth_on(SYNTH_TRIANGLE) && synth_on(SYNTH_NOISE) &&
               !synth_on(SYNTH_SQUARE),
           "a shot and an explosion share the mix");

    synth_reset();
    synth_play(3);
    expect(synth_on(SYNTH_SQUARE) && !synth_on(SYNTH_TRIANGLE) &&
               !synth_on(SYNTH_NOISE),
           "the pop uses the square");
    n = 0;
    for (i = 0; i < 200; i++)
        n = remember(seen, n, 32, synth_sample());
    expect(n >= 4, "the square drops through more than one level");
    guard = 200;
    while (synth_on(SYNTH_SQUARE) && guard < 1000) {
        synth_sample();
        guard++;
    }
    expect(guard == 400, "the pop lasts 400 samples");
    expect(!synth_on(SYNTH_SQUARE), "the pop ends on its own");
}

static void test_wire(void)
{
    wire_reset();
    send(0xA5);
    expect(wire_take() == 0xA5, "a byte survives the pulse widths");
    expect(wire_take() == -1, "the queue is empty after that byte");

    pulse(2, 0);
    pulse(4, 1);
    send(0x3C);
    expect(wire_take() == 0x3C, "a short glitch drops the partial byte");

    send(1);
    send(2);
    expect(wire_take() == 1 && wire_take() == 2, "two bytes stay in order");
    expect(wire_take() == -1, "nothing follows the pair");
}

int main(void)
{
    report_begin("synth");
    test_catalog();
    test_wire();
    return report_end();
}
