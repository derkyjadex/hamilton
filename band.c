#include <stdlib.h>

#include "private.h"

Synth *synths[NUM_CHANNELS];

int band_init()
{
	for (int i = 0; i < NUM_CHANNELS; i++) {
		synths[i] = NULL;
	}

	return lib_init();
}

void band_free()
{
	lib_free();
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

int band_set_synth(int channel, const SynthType *type)
{
	Synth **synth = &synths[channel];

	if (*synth) {
		(*synth)->free(*synth);
	}

	*synth = type->init();

	return *synth != NULL;
}

const char *band_get_channel_controls(int channel, int *numControls)
{
	Synth *synth = synths[channel];
	return synth->getControls(synth, numControls);
}

float *band_get_channel_control(int channel, const char *control)
{
	Synth *synth = synths[channel];
	return synth->getControl(synth, control);
}
