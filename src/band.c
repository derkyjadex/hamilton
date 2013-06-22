/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "hamilton/band.h"
#include "hamilton/lib.h"
#include "mq.h"

typedef enum {
	NOTE_OFF = 0,
	NOTE_ON = 1,
	PITCH,
	CONTROL,
	PATCH,
	RESET_TIME
} MessageType;

typedef union {
	struct {
		int num;
		float velocity;
	} note;
	struct {
		float offset;
	} pitch;
	struct {
		int control;
		float value;
	} control;
	int patch;
	uint32_t time;
} MessageData;

typedef struct {
	uint32_t time;
	int channel;
	MessageType type;
	MessageData data;
} Message;


struct HmBand {
	HmSynth *synths[NUM_CHANNELS];
	uint64_t time;
	HmLib *lib;
	HmMQ *mq;
	bool hasMessage;
	Message message;
};

AlError hm_band_init(HmBand **result)
{
	BEGIN()

	HmBand *band = NULL;
	TRY(al_malloc(&band, sizeof(HmBand), 1));

	for (int i = 0; i < NUM_CHANNELS; i++) {
		band->synths[i] = NULL;
	}

	band->time = 0;
	band->lib = NULL;
	band->mq = NULL;
	band->hasMessage = false;

	TRY(mq_init(&band->mq, sizeof(Message), 256));
	TRY(hm_lib_init(&band->lib));

	*result = band;

	CATCH(
		hm_band_free(band);
	)
	FINALLY()
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

AlError hm_band_set_channel_synth(HmBand *band, int channel, const HmSynthType *type)
{
	BEGIN()

	HmSynth **synth = &band->synths[channel];

	if (*synth) {
		(*synth)->free(*synth);
	}

	*synth = type->init(type);
	if (!*synth)
		THROW(AL_ERROR_GENERIC);

	PASS()
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
	MessageData *data = &band->message.data;
	HmSynth *synth = band->synths[band->message.channel];
	if (!synth)
		return;

	switch (band->message.type) {
		case RESET_TIME:
			band->time = data->time;
			break;

		case NOTE_OFF:
			synth->stopNote(synth, data->note.num);
			break;

		case NOTE_ON:
			synth->startNote(synth, data->note.num, data->note.velocity);
			break;

		case PITCH:
			break;

		case CONTROL:
			synth->setControl(synth, data->control.control, data->control.value);
			break;

		case PATCH:
			synth->setPatch(synth, data->patch);
			break;
	}
}

void hm_band_run(HmBand *band, float *buffer, int length)
{
	for (int i = 0; i < length; i++) {
		buffer[i] = 0;
	}

	do {
		if (!band->hasMessage) {
			band->hasMessage = mq_pop(band->mq, &band->message);
		}

		while (band->hasMessage && band->message.time <= band->time) {
			process_message(band);
			band->hasMessage = mq_pop(band->mq, &band->message);
		}

		int samples;
		if (band->hasMessage) {
			samples = (int)(band->message.time - band->time);
		}

		if (!band->hasMessage || samples > length) {
			samples = length;
		}

		for (int i = 0; i < NUM_CHANNELS; i++) {
			HmSynth *synth = band->synths[i];
			if (synth) {
				synth->generate(synth, buffer, samples);
			}
		}

		band->time += samples;
		buffer += samples;
		length -= samples;

	} while (length);
}

bool hm_band_reset_time(HmBand *band, uint32_t time)
{
	Message message = {
		.time = 0,
		.type = RESET_TIME,
		.data = {
			.time = time
		}
	};

	return mq_push(band->mq, &message);
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
