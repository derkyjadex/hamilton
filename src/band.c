/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "hamilton/band.h"
#include "hamilton/lib.h"
#include "mq.h"

struct HmBand {
	HmSynth *synths[NUM_CHANNELS];
	double time;
	HmLib *lib;
	HmMQ *mq;
	Message *message;
};

int hm_band_init(HmBand **result)
{
	int error;

	HmBand *band = malloc(sizeof(HmBand));
	if (!band) {
		error = 1;
		goto end;
	}

	for (int i = 0; i < NUM_CHANNELS; i++) {
		band->synths[i] = NULL;
	}

	band->time = 0;
	band->lib = NULL;
	band->mq = NULL;
	band->message = NULL;

	error = mq_init(&band->mq);
	if (error) goto end;

	error = hm_lib_init(&band->lib);
	if (error) goto end;

	*result = band;

end:
	if (error) {
		hm_band_free(band);
	}

	return error;
}

void hm_band_free(HmBand *band)
{
	if (band) {
		hm_lib_free(band->lib);
		mq_free(band->mq);
		free(band);
	}
}

HmLib *hm_band_get_lib(HmBand *band)
{
	return band->lib;
}

void hm_band_get_channel_synths(HmBand *band, const HmSynthType *types[NUM_CHANNELS])
{
	for (int i = 0; i < NUM_CHANNELS; i++) {
		HmSynth *synth = band->synths[i];
		if (synth) {
			types[i] = synth->type;
		} else {
			types[i] = NULL;
		}
	}
}

int hm_band_set_channel_synth(HmBand *band, int channel, const HmSynthType *type)
{
	HmSynth **synth = &band->synths[channel];

	if (*synth) {
		(*synth)->free(*synth);
	}

	*synth = type->init(type);
	if (!*synth)
		return 1;

	return 0;
}

const char **hm_band_get_channel_controls(HmBand *band, int channel, int *numControls)
{
	HmSynth *synth = band->synths[channel];
	return synth->getControls(synth, numControls);
}

float hm_band_get_channel_control(HmBand *band, int channel, int control)
{
	HmSynth *synth = band->synths[channel];
	return synth->getControl(synth, control);
}

static void process_message(HmBand *band)
{
	Message *message = band->message;
	HmSynth *synth = band->synths[message->channel];
	if (!synth)
		return;

	switch (message->type) {
		case NOTE_OFF:
			synth->stopNote(synth, message->data.note.num);
			break;

		case NOTE_ON:
			synth->startNote(synth, message->data.note.num, message->data.note.velocity);
			break;

		case PITCH:
			break;

		case CONTROL:
			synth->setControl(synth, message->data.control.control, message->data.control.value);
			break;

		case PATCH:
			synth->setPatch(synth, message->data.patch);
			break;
	}
}

void hm_band_run(HmBand *band, float *buffer, int length)
{
	for (int i = 0; i < length ; i++) {
		buffer[i] = 0;
	}

	do {
		if (!band->message) {
			band->message = mq_pop(band->mq);
		}

		while (band->message && band->message->time <= band->time) {
			process_message(band);
			band->message = mq_pop(band->mq);
		}

		double duration;
		int samples;
		if (band->message) {
			duration = band->message->time - band->time;
			samples = duration * (HM_SAMPLE_RATE / 1000);
		}

		if (!band->message || samples > length) {
			samples = length;
			duration = (double)samples / (HM_SAMPLE_RATE / 1000);
		}

		for (int i = 0; i < NUM_CHANNELS; i++) {
			HmSynth *synth = band->synths[i];
			if (synth) {
				synth->generate(synth, buffer, samples);
			}
		}

		band->time += duration;
		buffer += samples;
		length -= samples;

	} while (length);
}

bool hm_band_send_note(HmBand *band, uint32_t time, int channel, bool state, int num, float velocity)
{
	Message message = {
		.time = time,
		.type = (state) ? NOTE_ON : NOTE_OFF,
		.channel = channel,
		.data = {
			.note = {
				.num = num,
				.velocity = velocity
			}
		}
	};

	return mq_push(band->mq, &message);
}

bool hm_band_send_pitch(HmBand *band, uint32_t time, int channel, float offset)
{
	Message message = {
		.time = time,
		.channel = channel,
		.type = PITCH,
		.data = {
			.pitch = {
				.offset = offset
			}
		}
	};

	return mq_push(band->mq, &message);
}

bool hm_band_send_cc(HmBand *band, uint32_t time, int channel, int control, float value)
{
	Message message = {
		.time = time,
		.channel = channel,
		.type = CONTROL,
		.data = {
			.control = {
				.control = control,
				.value = value
			}
		}
	};

	return mq_push(band->mq, &message);
}

bool hm_band_send_patch(HmBand *band, uint32_t time, int channel, int patch)
{
	Message message = {
		.time = time,
		.channel = channel,
		.type = PATCH,
		.data = {
			.patch = patch
		}
	};

	return mq_push(band->mq, &message);
}
