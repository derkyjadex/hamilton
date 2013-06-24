/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_BAND_H
#define _HAMILTON_BAND_H

#include <stdint.h>
#include <stdbool.h>

#include "albase/common.h"
#include "hamilton/synth.h"
#include "hamilton/lib.h"
#include "hamilton/seq.h"

static const int HM_SAMPLE_RATE = 48000;
static const int NUM_CHANNELS = 4;

typedef struct HmBand HmBand;

typedef struct {
	bool playing;
	uint32_t position;
	bool looping;
	uint32_t loopStart;
	uint32_t loopEnd;
} HmBandState;

AlError hm_band_init(HmBand **band);
void hm_band_free(HmBand *band);

HmLib *hm_band_get_lib(HmBand *band);
HmSeq *hm_band_get_seq(HmBand *band);

void hm_band_get_channel_synths(HmBand *band, const HmSynthType *types[NUM_CHANNELS]);
AlError hm_band_set_channel_synth(HmBand *band, int channel, const HmSynthType *type);

const char **hm_band_get_channel_controls(HmBand *band, int channel, int *numControls);
float hm_band_get_channel_control(HmBand *band, int channel, int control);
void hm_band_run(HmBand *band, float *buffer, int length);

AlError hm_band_play(HmBand *band);
AlError hm_band_pause(HmBand *band);
AlError hm_band_seek(HmBand *band, uint32_t position);

AlError hm_band_set_looping(HmBand *band, bool looping);
AlError hm_band_set_loop(HmBand *band, uint32_t start, uint32_t end);

void hm_band_get_state(HmBand *band, HmBandState *state);

bool hm_band_send_note(HmBand *band, int channel, bool state, int num, float velocity);
bool hm_band_send_pitch(HmBand *band, int channel, float pitch);
bool hm_band_send_cc(HmBand *band, int channel, int control, float value);
bool hm_band_send_patch(HmBand *band, int channel, int patch);

#endif
