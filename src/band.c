/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "band.h"
#include "lib.h"
#include "mq.h"

Synth *synths[NUM_CHANNELS];
static double time = 0;
static Message *message = NULL;

int band_init()
{
	int error;

	for (int i = 0; i < NUM_CHANNELS; i++) {
		synths[i] = NULL;
	}

	time = 0;
	message = NULL;

	error = lib_init();
	if (error) goto end;

	error = mq_init();
	if (error) goto end;

end:
	if (error) {
		band_free();
	}

	return error;
}

void band_free()
{
	lib_free();
	mq_free();
}

void band_get_channel_synths(const SynthType *types[NUM_CHANNELS])
{
	for (int i = 0; i < NUM_CHANNELS; i++) {
		Synth *synth = synths[i];
		if (synth) {
			types[i] = synth->type;
		} else {
			types[i] = NULL;
		}
	}
}

int band_set_channel_synth(int channel, const SynthType *type)
{
	Synth **synth = &synths[channel];

	if (*synth) {
		(*synth)->free(*synth);
	}

	*synth = type->init(type);
	if (!*synth)
		return 1;

	return 0;
}

const char **band_get_channel_controls(int channel, int *numControls)
{
	Synth *synth = synths[channel];
	return synth->getControls(synth, numControls);
}

float band_get_channel_control(int channel, int control)
{
	Synth *synth = synths[channel];
	return synth->getControl(synth, control);
}

static void process_message()
{
	Synth *synth = synths[message->channel];
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

void band_run(float *buffer, int length)
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
			samples = duration * (SAMPLE_RATE / 1000);
		}

		if (!message || samples > length) {
			samples = length;
			duration = (double)samples / (SAMPLE_RATE / 1000);
		}

		for (int i = 0; i < NUM_CHANNELS; i++) {
			Synth *synth = synths[i];
			if (synth) {
				synth->generate(synth, buffer, samples);
			}
		}

		time += duration;
		buffer += samples;
		length -= samples;

	} while (length);
}
