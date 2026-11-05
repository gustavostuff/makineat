#ifndef FW_SYNTH_H
#define FW_SYNTH_H

#include <stdint.h>

/*
 * The console player. Three channels, one waveform each.
 * A sample is a DAC code, 0 to 1023. 512 is silence.
 * SYNTH_RATE is the sample rate the audio AVR and the simulator share.
 */

#define SYNTH_SQUARE 0
#define SYNTH_TRIANGLE 1
#define SYNTH_NOISE 2
#define SYNTH_RATE 20000
#define SYNTH_FRAME (SYNTH_RATE / 60)
#define SYNTH_VOLUME_DEFAULT 20

void synth_reset(void);
void synth_volume_set(int percent);
int synth_volume_get(void);
void synth_play(uint8_t index);
uint16_t synth_sample(void);
int synth_on(int channel);

#endif
