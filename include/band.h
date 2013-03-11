#ifndef _HAMILTON_BAND_H
#define _HAMILTON_BAND_H

#include <stdint.h>
#include <stdbool.h>

#include "synth.h"

static const int SAMPLE_RATE = 48000;
static const int NUM_CHANNELS = 4;

int band_init(void);
void band_free(void);

void band_get_channel_synths(const SynthType *types[NUM_CHANNELS]);
int band_set_channel_synth(int channel, const SynthType *type);

const char **band_get_channel_controls(int channel, int *numControls);
float *band_get_channel_control(int channel, const char *control);
void band_run(float *buffer, int length);

bool band_send_note(uint32_t time, int channel, bool state, int num, float velocity);
bool band_send_pitch(uint32_t time, int channel, float offset);
bool band_send_cc(uint32_t time, int channel, float *control, float value);

#endif
