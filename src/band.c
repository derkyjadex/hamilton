/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "hamilton/band.h"
#include "hamilton/lib.h"
#include "mq.h"

HmSynth *synths[NUM_CHANNELS];
static double time = 0;
static Message *message = NULL;

int hm_band_init()
{
	int error;

	for (int i = 0; i < NUM_CHANNELS; i++) {
		synths[i] = NULL;
	}

	time = 0;
	message = NULL;

	error = hm_lib_init();
	if (error) goto end;

	error = mq_init();
	if (error) goto end;

end:
	if (error) {
		hm_band_free();
	}

	return error;
}

void hm_band_free()
{
	hm_lib_free();
	mq_free();
}

void hm_band_get_channel_synths(const HmSynthType *types[NUM_CHANNELS])
{
	for (int i = 0; i < NUM_CHANNELS; i++) {
		HmSynth *synth = synths[i];
		if (synth) {
			types[i] = synth->type;
		} else {
			types[i] = NULL;
		}
	}
}

int hm_band_set_channel_synth(int channel, const HmSynthType *type)
{
	HmSynth **synth = &synths[channel];

	if (*synth) {
		(*synth)->free(*synth);
	}

	*synth = type->init(type);
	if (!*synth)
		return 1;

	return 0;
}

const char **hm_band_get_channel_controls(int channel, int *numControls)
{
	HmSynth *synth = synths[channel];
	return synth->getControls(synth, numControls);
}

float hm_band_get_channel_control(int channel, int control)
{
	HmSynth *synth = synths[channel];
	return synth->getControl(synth, control);
}

static void process_message()
{
	HmSynth *synth = synths[message->channel];
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

void hm_band_run(float *buffer, int length)
{
	for (int i = 0; i < length ; i++) {
		buffer[i] = 0;
	}

	do {
		if (!message) {
			message = mq_pop();
		}

		while (message && message->time <= time) {
			process_message();
			message = mq_pop();
		}

		double duration;
		int samples;
		if (message) {
			duration = message->time - time;
			samples = duration * (HM_SAMPLE_RATE / 1000);
		}

		if (!message || samples > length) {
			samples = length;
			duration = (double)samples / (HM_SAMPLE_RATE / 1000);
		}

		for (int i = 0; i < NUM_CHANNELS; i++) {
			HmSynth *synth = synths[i];
			if (synth) {
				synth->generate(synth, buffer, samples);
			}
		}

		time += duration;
		buffer += samples;
		length -= samples;

	} while (length);
}
