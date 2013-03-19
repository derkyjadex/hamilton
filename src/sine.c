#include <stdlib.h>
#include <math.h>

#include "synth.h"
#include "band.h"
#include "lib.h"

int sine_wave_register();
static const char *name = "Sine Wave";
static const char *controls[] = { };

enum EnvState {
	OFF, ATTACK, DECAY, SUSTAIN, RELEASE
};

struct Env {
	float a, d, s, r;
	enum EnvState state;
	float gain, releaseGain;
	uint32_t t;
};

struct Osc {
	float t, step;
};

struct LP2 {
	float cutoff;
	float a[3];
	float b[3];
	float in[3];
	float out[3];
};

typedef struct SineSynth {
	Synth base;

	int note;

	struct Osc osc;
	struct Env env;
	struct Env fenv;
	struct LP2 filter;
} SineSynth;

typedef float (*OscFunc)(float t);

static void osc_init(struct Osc *osc)
{
	osc->t = 0;
	osc->step = 0;
}

static void osc_set_freq(struct Osc *osc, float freq)
{
	osc->step = freq / SAMPLE_RATE;
}

static float osc_step(struct Osc *osc, OscFunc func)
{
	osc->t += osc->step;
	while (osc->t > 1) osc->t -= 1;

	return func(osc->t);
}

static float osc_sine(float t)
{
	return sinf(2 * M_PI * t);
}

static float osc_tri(float t)
{
	return (t < 0.5) ?
		 4 * t - 1 :
		-4 * t + 3;
}

static float osc_saw(float t)
{
	return 2 * t - 1;
}

static float osc_square(float t)
{
	return (t < 0.5) ? 1 : -1;
}

static void env_init(struct Env *env)
{
	env->a = 0;
	env->d = 0;
	env->s = 1;
	env->r = 0;
	env->state = OFF;
	env->t = 0;
}

static void env_start(struct Env *env)
{
	env->state = ATTACK;
	env->t = 0;
}

static void env_stop(struct Env *env)
{
	env->state = RELEASE;
	env->t = 0;
	env->releaseGain = env->gain;
}

static float env_step(struct Env *env)
{
	uint32_t t = env->t++;

	switch (env->state) {
		case OFF:
			env->gain = 0.0;
			break;

		case ATTACK:
			if (t == env->a) {
				env->state = DECAY;
				env->t = 0;
				env->gain = 1.0;
			} else {
				env->gain = t / env->a;
			}
			break;

		case DECAY:
			if (t == env->d) {
				env->state = SUSTAIN;
				env->t = 0;
				env->gain = env->s;
			} else {
				env->gain = (1.0 - (t / env->d)) * (1.0 - env->s) + env->s;
			}
			break;

		case SUSTAIN:
			env->gain = env->s;
			break;

		case RELEASE:
			if (t == env->r) {
				env->state = OFF;
				env->gain = 0.0;
			} else {
				env->gain = (1.0 - (t / env->r)) * env->releaseGain;
			}
	}

	return env->gain;
}

static void lp2_recalc(struct LP2 *lp)
{
	float alpha = tanf(2 * M_PI * lp->cutoff / SAMPLE_RATE);
	float alphaSq = alpha * alpha;
	float c = 1 / (1 + 2 * M_SQRT1_2 * alpha + alphaSq);

	lp->a[0] = alphaSq * c;
	lp->a[1] = 2 * lp->a[0];
	lp->a[2] = lp->a[0];

	lp->b[1] = 2 * (alphaSq - 1) * c;
//	lp->b[2] = (1 - 2 * M_SQRT1_2 * alpha + alphaSq) / c;
	lp->b[2] = lp->a[0] + lp->a[1] + lp->a[2] - lp->b[1] - 1;
}

static void lp2_init(struct LP2 *lp)
{
//	lp->cutoff = 16715;
	lp->cutoff = 1000;
	lp->in[0] = 0;
	lp->in[1] = 0;
	lp->in[2] = 0;
	lp->out[0] = 0;
	lp->out[1] = 0;
	lp->out[2] = 0;

	lp2_recalc(lp);
}

static float lp2_step(struct LP2 *lp, float input)
{
	lp->in[2] = lp->in[1];
	lp->in[1] = lp->in[0];
	lp->in[0] = input;
	lp->out[2] = lp->out[1];
	lp->out[1] = lp->out[0];

	lp->out[0] = lp->a[0] * lp->in[0] +
				 lp->a[1] * lp->in[1] +
				 lp->a[2] * lp->in[2] -
				 lp->b[1] * lp->out[0] -
				 lp->b[2] * lp->out[1];

	return lp->out[0];
}

static float mix(float a, float b, float factor)
{
	return (1.0 - factor) * a + factor * b;
}

static const char **get_controls(Synth *base, int *numControls)
{
	*numControls = 0;
	return controls;
}

static float get_control(Synth *base, int control)
{
	return 0;
}

static void set_control(Synth *base, int control, float value)
{ }

static float midi_to_freq(int note)
{
	return (note <= 0) ? 0 : 440.0 * powf(2.0, (note - 69.0) / 12.0);
}

static void start_note(Synth *base, int num, float velocity)
{
	SineSynth *synth = (SineSynth *)base;

	synth->note = num;

	float freq = midi_to_freq(num);
	osc_set_freq(&synth->osc, freq);
	env_start(&synth->env);
	env_start(&synth->fenv);
	synth->filter.cutoff = freq * 2;
	lp2_recalc(&synth->filter);
}

static void stop_note(Synth *base, int num)
{
	SineSynth *synth = (SineSynth *)base;

	if (num == synth->note) {
		synth->note = 0;

		env_stop(&synth->env);
		env_stop(&synth->fenv);
	}
}

static void generate(Synth *base, float *buffer, int length)
{
	SineSynth *synth = (SineSynth *)base;

	for (int i = 0; i < length; i++) {
		float x;

		x = osc_step(&synth->osc, osc_saw);
		x *= env_step(&synth->env);
		x = mix(x, lp2_step(&synth->filter, x), 1 - env_step(&synth->fenv));

        buffer[i] = 0.4 * x;
	}
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
		.setControl = set_control,
		.startNote = start_note,
		.stopNote = stop_note,
		.generate = generate
	};

	synth->note = 0;

	osc_init(&synth->osc);
	env_init(&synth->env);
	env_init(&synth->fenv);
	lp2_init(&synth->filter);

	synth->env.a = 48 * 10;
	synth->env.d = 48 * 5000;
	synth->env.s = 0.0;
	synth->env.r = 48 * 100;

	synth->fenv.a = 48 * 100;
	synth->fenv.d = 48 * 0;
	synth->fenv.s = 0.0;
	synth->fenv.r = 48 * 100;

	return &synth->base;
}

int sine_wave_register()
{
	return lib_add_synth(name, init);
}
