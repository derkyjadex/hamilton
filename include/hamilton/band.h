/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_BAND_H
#define _HAMILTON_BAND_H

#include <stdint.h>
#include <stdbool.h>

#include "hamilton/synth.h"
#include "hamilton/lib.h"

static const int HM_SAMPLE_RATE = 48000;
static const int NUM_CHANNELS = 4;

typedef struct HmBand HmBand;

int hm_band_init(HmBand **band);
void hm_band_free(HmBand *band);

HmLib *hm_band_get_lib(HmBand *band);

void hm_band_get_channel_synths(HmBand *band, const HmSynthType *types[NUM_CHANNELS]);
int hm_band_set_channel_synth(HmBand *band, int channel, const HmSynthType *type);

const char **hm_band_get_channel_controls(HmBand *band, int channel, int *numControls);
float hm_band_get_channel_control(HmBand *band, int channel, int control);
void hm_band_run(HmBand *band, float *buffer, int length);

bool hm_band_reset_time(HmBand *band, uint32_t time);
bool hm_band_send_note(HmBand *band, uint32_t time, int channel, bool state, int num, float velocity);
bool hm_band_send_pitch(HmBand *band, uint32_t time, int channel, float offset);
bool hm_band_send_cc(HmBand *band, uint32_t time, int channel, int control, float value);
bool hm_band_send_patch(HmBand *band, uint32_t time, int channel, int patch);

#endif
