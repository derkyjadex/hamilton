#include <stdlib.h>
#include <math.h>

#include "synth.h"
#include "band.h"
#include "lib.h"

int sine_wave_register();
static const char *name = "Sine Wave";
static const char *controls[] = { };

typedef struct SineSynth {
	Synth base;

	float time;
	int note;
	float freq;
} SineSynth;

static const char **get_controls(Synth *synth, int *numControls)
{
	*numControls = 0;
	return controls;
}

static float *get_control(Synth *synth, const char *control)
{
	return NULL;
}

static float midi_to_freq(int note)
{
	return (note <= 0) ? 0 : 440.0 * powf(2.0, (note - 69.0) / 12.0);
}

static void start_note(Synth *synth, int num, float velocity)
{
	SineSynth *sine = (SineSynth *)synth;
	sine->time = 0;
	sine->note = num;
	sine->freq = midi_to_freq(num);
}

static void stop_note(Synth *synth, int num)
{
	SineSynth *sine = (SineSynth *)synth;

	if (num == sine->note) {
		sine->time = 0;
		sine->note = 0;
		sine->freq = 0;
	}
}

static void generate(Synth *synth, float *buffer, int length)
{
	SineSynth *sine = (SineSynth *)synth;

	float t = sine->time;
	float f = sine->freq;

	for (int i = 0; i < length; i++) {
		float x = sinf(f * t * M_PI * 2);

		buffer[i] = x;

		t += 1.0 / SAMPLE_RATE;
	}

	sine->time = t;
}

static void free_synth(Synth *synth)
{
	free(synth);
}

static Synth *init(const SynthType *type)
{
	SineSynth *synth = malloc(sizeof(SineSynth));
	if (!synth)
		return NULL;

	synth->base = (Synth){
		.type = type,
		.free = free_synth,
		.getControls = get_controls,
		.getControl = get_control,
		.startNote = start_note,
		.stopNote = stop_note,
		.generate = generate
	};
	synth->time = 0;
	synth->note = 0;
	synth->freq = 0;

	return &synth->base;
}

int sine_wave_register()
{
	return lib_add_synth(name, init);
}
