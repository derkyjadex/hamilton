/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "hamilton/band.h"
#include "hamilton/lib.h"
#include "mq.h"

#define SEQ_TO_SAMPLES(t) (t) * ((double)HM_SAMPLE_RATE / 10000.0)
#define SAMPLES_TO_SEQ(n) (n) * (10000.0 / (double)HM_SAMPLE_RATE)

typedef struct {
	enum {
		PLAY,
		PAUSE,
		SEEK
	} type;
	union {
		uint32_t position;
	} data;
} ToAudioMessage;

typedef struct {
	enum {
		PLAYING,
		PAUSED,
		POSITION
	} type;
	union {
		uint32_t position;
	} data;
} FromAudioMessage;

struct HmBand {
	HmSynth *synths[NUM_CHANNELS];
	uint64_t time;
	bool playing;
	HmLib *lib;
	HmSeq *seq;
	HmMQ *toAudio;
	HmMQ *fromAudio;

	HmBandState lastState;
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
	band->playing = false;
	band->lib = NULL;
	band->seq = NULL;
	band->toAudio = NULL;
	band->fromAudio = NULL;

	band->lastState = (HmBandState){
		.playing = false,
		.position = 0
	};

	TRY(hm_lib_init(&band->lib));
	TRY(hm_seq_init(&band->seq));
	TRY(mq_init(&band->toAudio, sizeof(ToAudioMessage), 256));
	TRY(mq_init(&band->fromAudio, sizeof(FromAudioMessage), 256));

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
		hm_seq_free(band->seq);
		mq_free(band->toAudio);
		mq_free(band->fromAudio);
		free(band);
	}
}

HmLib *hm_band_get_lib(HmBand *band)
{
	return band->lib;
}

HmSeq *hm_band_get_seq(HmBand *band)
{
	return band->seq;
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

static void process_messages(HmBand *band)
{
	FromAudioMessage outMessage;
	ToAudioMessage message;
	while (mq_pop(band->toAudio, &message)) {
		switch (message.type) {
			case PLAY:
				band->playing = true;
				outMessage = (FromAudioMessage){
					.type = PLAYING,
				};
				mq_push(band->fromAudio, &outMessage);
				break;

			case PAUSE:
				band->playing = false;
				outMessage = (FromAudioMessage){
					.type = PAUSED
				};
				mq_push(band->fromAudio, &outMessage);
				break;

			case SEEK:
				band->time = SEQ_TO_SAMPLES(message.data.position);
				break;
		}
	}
}

static void process_event(HmBand *band, HmEvent *event)
{
	HmSynth *synth = band->synths[event->channel];
	if (!synth)
		return;

	switch (event->type) {
		case HM_EV_NOTE_OFF:
			synth->stopNote(synth, event->data.note.num);
			break;

		case HM_EV_NOTE_ON:
			synth->startNote(synth, event->data.note.num, event->data.note.velocity);
			break;

		case HM_EV_PITCH:
			break;

		case HM_EV_CONTROL:
			synth->setControl(synth, event->data.control.control, event->data.control.value);
			break;

		case HM_EV_PATCH:
			synth->setPatch(synth, event->data.patch);
			break;
	}
}

void hm_band_run(HmBand *band, float *buffer, int length)
{
	for (int i = 0; i < length; i++) {
		buffer[i] = 0;
	}

	process_messages(band);

	uint32_t time = SAMPLES_TO_SEQ(band->time);
	uint32_t start = time;
	uint32_t end = start + SAMPLES_TO_SEQ(length);

	HmEvent events[128];
	int numEvents;
	numEvents = hm_seq_get_events(band->seq, events, sizeof(events), start, end);
	int event = 0;

	if (!band->playing) {
		numEvents = 0;
	}

	bool eventUpcoming = numEvents > 0;

	do {
		while (eventUpcoming && events[event].time <= time) {
			process_event(band, &events[event]);
			event++;
			eventUpcoming = event < numEvents;
		}

		int samples;
		if (eventUpcoming) {
			samples = SEQ_TO_SAMPLES(events[event].time - time);
		}

		if (!eventUpcoming || samples > length) {
			samples = length;
		}

		for (int i = 0; i < NUM_CHANNELS; i++) {
			HmSynth *synth = band->synths[i];
			if (synth) {
				synth->generate(synth, buffer, samples);
			}
		}

		if (eventUpcoming) {
			time = events[event].time;
		}

		if (band->playing) {
			band->time += samples;
		}
		buffer += samples;
		length -= samples;

	} while (length);

	FromAudioMessage message = {
		.type = POSITION,
		.data = {
			.position = time
		}
	};
	mq_push(band->fromAudio, &message);
}

AlError hm_band_play(HmBand *band)
{
	BEGIN()

	ToAudioMessage message = {
		.type = PLAY
	};

	if (!mq_push(band->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_band_pause(HmBand *band)
{
	BEGIN()

	ToAudioMessage message = {
		.type = PAUSE
	};

	if (!mq_push(band->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_band_seek(HmBand *band, uint32_t	position)
{
	BEGIN()

	ToAudioMessage message = {
		.type = SEEK,
		.data = {
			.position = position
		}
	};

	if (!mq_push(band->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

void hm_band_get_state(HmBand *band, HmBandState *state)
{
	HmBandState newState = band->lastState;

	FromAudioMessage message;
	while (mq_pop(band->fromAudio, &message)) {
		switch (message.type) {
			case PLAYING:
				newState.playing = true;
				break;

			case PAUSED:
				newState.playing = false;
				break;

			case POSITION:
				newState.position = message.data.position;
				break;
		}
	}

	band->lastState = newState;
	*state = newState;
}

bool hm_band_send_note(HmBand *band, int channel, bool state, int num, float velocity)
{
	HmEvent event = {
		.time = 0,
		.channel = channel,
		.type = (state) ? HM_EV_NOTE_ON : HM_EV_NOTE_OFF,
		.data = {
			.note = {
				.num = num,
				.velocity = velocity
			}
		}
	};

	return false;
}

bool hm_band_send_pitch(HmBand *band, int channel, float pitch)
{
	HmEvent event = {
		.time = 0,
		.channel = channel,
		.type = HM_EV_PITCH,
		.data = {
			.pitch = pitch
		}
	};

	return false;
}

bool hm_band_send_cc(HmBand *band, int channel, int control, float value)
{
	HmEvent event = {
		.time = 0,
		.channel = channel,
		.type = HM_EV_PITCH,
		.data = {
			.control = {
				.control = control,
				.value = value
			}
		}
	};

	return false;
}

bool hm_band_send_patch(HmBand *band, int channel, int patch)
{
	HmEvent event = {
		.time = 0,
		.channel = channel,
		.type = HM_EV_PITCH,
		.data = {
			.patch = patch
		}
	};

	return false;
}
