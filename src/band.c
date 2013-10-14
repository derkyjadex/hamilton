/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "hamilton/band.h"
#include "hamilton/lib.h"
#include "albase/mq.h"

#define TICKS_TO_SAMPLES(t) (t) * (band->sampleRate / HM_SEQ_TICK_RATE)
#define SAMPLES_TO_TICKS(n) (n) * (HM_SEQ_TICK_RATE / band->sampleRate)

typedef struct {
	enum {
		PLAY,
		PAUSE,
		SEEK,
		SET_LOOPING,
		SET_LOOP
	} type;
	union {
		uint32_t position;
		bool looping;
		struct {
			uint32_t start, end;
		} loop;
	} data;
} ToAudioMessage;

typedef struct {
	enum {
		PLAYING,
		PAUSED,
		POSITION,
		LOOPING_SET,
		LOOP_SET
	} type;
	union {
		uint32_t position;
		bool looping;
		struct {
			uint32_t start, end;
		} loop;
	} data;
} FromAudioMessage;

struct HmBand {
	HmSynth *synths[NUM_CHANNELS];
	double sampleRate;
	uint64_t time;
	bool playing;
	bool looping;
	uint64_t loopStart;
	uint64_t loopEnd;

	HmLib *lib;
	HmSeq *seq;
	AlMQ *toAudio;
	AlMQ *fromAudio;

	HmBandState lastState;
};

AlError hm_band_init(HmBand **result)
{
	BEGIN()

	HmBand *band = NULL;
	TRY(al_malloc(&band, sizeof(HmBand)));

	for (int i = 0; i < NUM_CHANNELS; i++) {
		band->synths[i] = NULL;
	}

	band->time = 0;
	band->sampleRate = 48000;
	band->playing = false;
	band->looping = false;
	band->loopStart = 0;
	band->loopEnd = 0;
	band->lib = NULL;
	band->seq = NULL;
	band->toAudio = NULL;
	band->fromAudio = NULL;

	band->lastState = (HmBandState){
		.playing = false,
		.position = 0,
		.looping = false,
		.loopStart = 0,
		.loopEnd = 0
	};

	TRY(hm_lib_init(&band->lib));
	TRY(hm_seq_init(&band->seq));
	TRY(al_mq_init(&band->toAudio, sizeof(ToAudioMessage), 256));
	TRY(al_mq_init(&band->fromAudio, sizeof(FromAudioMessage), 256));

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
		al_mq_free(band->toAudio);
		al_mq_free(band->fromAudio);
		free(band);
	}
}

void hm_band_set_sample_rate(HmBand *band, int sampleRate)
{
	band->sampleRate = sampleRate;

	for (int i = 0; i < NUM_CHANNELS; i++) {
		if (band->synths[i]) {
			band->synths[i]->setSampleRate(band->synths[i], sampleRate);
		}
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

	(*synth)->setSampleRate(*synth, band->sampleRate);

	PASS()
}

const char **hm_band_get_channel_params(HmBand *band, int channel, int *numParams)
{
	HmSynth *synth = band->synths[channel];
	return synth->getParams(synth, numParams);
}

float hm_band_get_channel_param(HmBand *band, int channel, int param)
{
	HmSynth *synth = band->synths[channel];
	return synth->getParam(synth, param);
}

static void process_messages(HmBand *band)
{
	FromAudioMessage outMessage;
	ToAudioMessage message;
	while (al_mq_pop(band->toAudio, &message)) {
		switch (message.type) {
			case PLAY:
				band->playing = true;
				outMessage = (FromAudioMessage){
					.type = PLAYING,
				};
				al_mq_push(band->fromAudio, &outMessage);
				break;

			case PAUSE:
				band->playing = false;
				outMessage = (FromAudioMessage){
					.type = PAUSED
				};
				al_mq_push(band->fromAudio, &outMessage);
				break;

			case SEEK:
				band->time = TICKS_TO_SAMPLES(message.data.position);
				break;

			case SET_LOOPING:
				band->looping = message.data.looping;
				outMessage = (FromAudioMessage){
					.type = LOOPING_SET,
					.data = {
						.looping = message.data.looping
					}
				};
				al_mq_push(band->fromAudio, &outMessage);
				break;

			case SET_LOOP:
				band->loopStart = TICKS_TO_SAMPLES(message.data.loop.start);
				band->loopEnd = TICKS_TO_SAMPLES(message.data.loop.end);
				outMessage = (FromAudioMessage){
					.type = LOOP_SET,
					.data = {
						.loop = {
							.start = message.data.loop.start,
							.end = message.data.loop.end
						}
					}
				};
				al_mq_push(band->fromAudio, &outMessage);
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
			synth->setPitch(synth, event->data.pitch);
			break;

		case HM_EV_CONTROL:
			synth->setControl(synth, event->data.control.num, event->data.control.value);
			break;

		case HM_EV_PARAM:
			synth->setParam(synth, event->data.param.num, event->data.param.value);
			break;

		case HM_EV_PATCH:
			synth->setPatch(synth, event->data.patch);
			break;
	}
}

static void run(HmBand *band, float *buffer, uint64_t numSamples)
{
	uint64_t time = band->time;

	HmEvent events[128];
	int numEvents;
	int event = 0;

	if (band->playing) {
		numEvents = hm_seq_get_events(band->seq, events, sizeof(events), time, time + numSamples, band->sampleRate);
	} else {
		numEvents = 0;
	}

	bool eventUpcoming = numEvents > 0;

	do {
		while (eventUpcoming && events[event].time <= time) {
			process_event(band, &events[event]);
			event++;
			eventUpcoming = event < numEvents;
		}

		uint64_t intervalSamples;
		if (eventUpcoming) {
			intervalSamples = TICKS_TO_SAMPLES(events[event].time - time);
		}

		if (!eventUpcoming || intervalSamples > numSamples) {
			intervalSamples = numSamples;
		}

		for (int i = 0; i < NUM_CHANNELS; i++) {
			HmSynth *synth = band->synths[i];
			if (synth) {
				synth->generate(synth, buffer, (int)intervalSamples);
			}
		}

		if (eventUpcoming) {
			time = events[event].time;
		}

		if (band->playing) {
			band->time += intervalSamples;
		}
		buffer += intervalSamples;
		numSamples -= intervalSamples;

	} while (numSamples);
}

void hm_band_run(HmBand *band, float *buffer, uint64_t numSamples)
{
	for (int i = 0; i < numSamples; i++) {
		buffer[i] = 0;
	}

	process_messages(band);

	while (true) {
		if (band->playing &&
			band->looping &&
			band->time <= band->loopEnd &&
			band->time + numSamples > band->loopEnd) {

			uint64_t intervalSamples = band->loopEnd - band->time;

			run(band, buffer, intervalSamples);

			band->time = band->loopStart;
			buffer += intervalSamples;
			numSamples -= intervalSamples;

		} else {
			run(band, buffer, numSamples);
			break;
		}
	}

	FromAudioMessage message = {
		.type = POSITION,
		.data = {
			.position = SAMPLES_TO_TICKS(band->time)
		}
	};
	al_mq_push(band->fromAudio, &message);
}

AlError hm_band_play(HmBand *band)
{
	BEGIN()

	ToAudioMessage message = {
		.type = PLAY
	};

	if (!al_mq_push(band->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_band_pause(HmBand *band)
{
	BEGIN()

	ToAudioMessage message = {
		.type = PAUSE
	};

	if (!al_mq_push(band->toAudio, &message))
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

	if (!al_mq_push(band->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_band_set_looping(HmBand *band, bool looping)
{
	BEGIN()

	ToAudioMessage message = {
		.type = SET_LOOPING,
		.data = {
			.looping = looping
		}
	};

	if (!al_mq_push(band->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_band_set_loop(HmBand *band, uint32_t start, uint32_t end)
{
	BEGIN()

	ToAudioMessage message = {
		.type = SET_LOOP,
		.data = {
			.loop = {
				.start = start,
				.end = end
			}
		}
	};

	if (!al_mq_push(band->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

void hm_band_get_state(HmBand *band, HmBandState *state)
{
	HmBandState newState = band->lastState;

	FromAudioMessage message;
	while (al_mq_pop(band->fromAudio, &message)) {
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

			case LOOPING_SET:
				newState.looping = message.data.looping;
				break;

			case LOOP_SET:
				newState.loopStart = message.data.loop.start;
				newState.loopEnd = message.data.loop.end;
				break;
		}
	}

	band->lastState = newState;
	*state = newState;
}

bool hm_band_send_note(HmBand *band, int channel, bool state, int num, float velocity)
{
/*	HmEvent event = {
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
*/
	return false;
}

bool hm_band_send_pitch(HmBand *band, int channel, float pitch)
{
/*	HmEvent event = {
		.time = 0,
		.channel = channel,
		.type = HM_EV_PITCH,
		.data = {
			.pitch = pitch
		}
	};
*/
	return false;
}

bool hm_band_send_cc(HmBand *band, int channel, int control, float value)
{
/*	HmEvent event = {
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
*/
	return false;
}

bool hm_band_send_patch(HmBand *band, int channel, int patch)
{
/*	HmEvent event = {
		.time = 0,
		.channel = channel,
		.type = HM_EV_PITCH,
		.data = {
			.patch = patch
		}
	};
*/
	return false;
}
