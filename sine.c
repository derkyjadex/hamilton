#include <stdlib.h>
#include <math.h>

#include "public.h"

int sine_wave_register();
static const char *name = "Sine Wave";
static const char *controls[] = { };

typedef struct SineSynth {
	Synth base;

	double t;
	double f;
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

static double midi_to_freq(int note)
{
	return (note <= 0) ? 0 : 440.0 * pow(2.0, (note - 69.0) / 12.0);
}

static void start_note(Synth *synth, int num, float velocity)
{
	SineSynth *sine = (SineSynth *)synth;
	sine->t = 0;
	sine->f = midi_to_freq(num);
}

static void stop_note(Synth *synth, int num)
{
	SineSynth *sine = (SineSynth *)synth;
	sine->t = 0;
	sine->f = 0;
}

static void generate(Synth *synth, int16_t *buffer, int length)
{
	SineSynth *sine = (SineSynth *)synth;

	double t = sine->t;
	double f = sine->f;

	for (int i = 0; i < length; i++) {
		double x = sin(f * t * M_PI * 2);

		buffer[i] = x * 2000;

		t += 1.0 / SAMPLE_RATE;
	}

	sine->t = t;
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
	synth->t = 0;
	synth->f = 0;

	return &synth->base;
}

int sine_wave_register()
{
	return lib_add_synth(name, init);
}
