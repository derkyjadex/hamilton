/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_BAND_H
#define _HAMILTON_BAND_H

#include <stdint.h>
#include <stdbool.h>

#include "synth.h"

static const int HM_SAMPLE_RATE = 48000;
static const int NUM_CHANNELS = 4;

int hm_band_init(void);
void hm_band_free(void);

void hm_band_get_channel_synths(const HmSynthType *types[NUM_CHANNELS]);
int hm_band_set_channel_synth(int channel, const HmSynthType *type);

const char **hm_band_get_channel_controls(int channel, int *numControls);
float hm_band_get_channel_control(int channel, int control);
void hm_band_run(float *buffer, int length);

bool hm_band_send_note(uint32_t time, int channel, bool state, int num, float velocity);
bool hm_band_send_pitch(uint32_t time, int channel, float offset);
bool hm_band_send_cc(uint32_t time, int channel, int control, float value);
bool hm_band_send_patch(uint32_t time, int channel, int patch);

#endif
