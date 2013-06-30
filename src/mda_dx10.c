/*
 * Copyright (c) 2013 James Deery
 * Based on "mda DX10", copyright (c) 1999-2000, 2008 Paul Kellett
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "hamilton/synth.h"
#include "hamilton/band.h"
#include "hamilton/lib.h"
#include "hamilton/core_synths.h"

static const char *name = "mda DX10";
static const char *params[] = {
	"Attack",
	"Decay",
	"Release",
	"Coarse",
	"Fine",
	"Mod Init",
	"Mod Dec",
	"Mod Sus",
	"Mod Rel",
	"Mod Vel",
	"Vibrato",
	"Octave",
	"FineTune",
	"Waveform",
	"Mod Thru",
	"LFO Rate"
};

const int NUM_PARAMS = 16;
const int NUM_PATCHES = 32;
const int NUM_VOICES = 8;
const float SILENCE = 0.0003f;
const int SUSTAIN_NOTE = 128;

const float MIDI_TO_FREQ_1 = 8.175798915644f;
const float MIDI_TO_FREQ_2 = 0.05776226505f;
const float HALF_PI = 1.570796326795f;

struct Patch {
	float params[NUM_PARAMS];
};

struct Voice {
	int note;

	struct {
		float phase, delta;
	} carrier;

	struct {
		float d, x0, x1;
	} mod;

	struct {
		float level;
		float target;
		float decay;
	} modEnv;

	struct {
		float attack, decay;
		float decayLevel, level;
	} env;
};

typedef struct Dx10 {
	HmSynth base;

	struct Patch patches[NUM_PATCHES];
	int currentPatch;

	struct Voice voices[NUM_VOICES];
	int activeVoices;

	bool sustain;

	struct {
		float attack, decay, release;
	} env;

	struct {
		float ratio;
		float initDepth, susDepth;
		float decay, release;
		float mix;
	} mod;

	struct {
		float d, x0, x1;
		int t;
		float vibrato, mw;
	} lfo;

	float tune, pitchBend;
	float modWheel, velSens, volume;
	float waveform;
} Dx10;

static void update_params(Dx10 *this)
{
	const float SAMPLE_TIME = 1.0f / HM_SAMPLE_RATE;
	float *params = this->patches[this->currentPatch].params;

	this->tune = MIDI_TO_FREQ_1 * SAMPLE_TIME * powf(2.0f, floorf(params[11] * 6.9f) - 2.0f);

	float ratio = floorf(40.1f * params[3] * params[3]);

	if (params[4] < 0.5f) {
		ratio += 0.2f * params[4] * params[4];

	} else {
		switch((int)(8.9f * params[4])) {
			case  4: ratio += 3.0 / 12.0; break;
			case  5: ratio += 4.0 / 12.0; break;
			case  6: ratio += 6.0 / 12.0; break;
			case  7: ratio += 8.0 / 12.0; break;
			default: ratio += 9.0 / 12.0;
		}
	}

	this->mod.ratio = HALF_PI * ratio;
	this->mod.initDepth = 0.0002f * params[5] * params[5];
	this->mod.susDepth = 0.0002f * params[7] * params[7];

	this->velSens = params[9];
	this->lfo.vibrato = 0.001f * params[10] * params[10];

	this->env.attack = 1.0f - expf(-SAMPLE_TIME * expf(8.0f - 8.0f * params[0]));

	if(params[1] > 0.98f) {
		this->env.decay = 1.0f;
	} else {
		this->env.decay = expf(-SAMPLE_TIME * expf(5.0f - 8.0f * params[1]));
	}

	this->env.release =        expf(-SAMPLE_TIME * expf(5.0f - 5.0f * params[2]));
	this->mod.decay =   1.0f - expf(-SAMPLE_TIME * expf(6.0f - 7.0f * params[6]));
	this->mod.release = 1.0f - expf(-SAMPLE_TIME * expf(5.0f - 8.0f * params[8]));

	this->waveform = 0.50f - 3.0f * params[13] * params[13];
	this->mod.mix = 0.25f * params[14] * params[14];
	this->lfo.d = 628.3f * SAMPLE_TIME * 25.0f * params[15] * params[15]; //these params not in original DX10
}

static struct Voice *find_next_voice(Dx10 *this)
{
	float level = 1;
	struct Voice *voice;

	for (int i = 0; i < NUM_VOICES; i++) {
		if (this->voices[i].env.decayLevel < level) {
			level = this->voices[i].env.decayLevel;
			voice = &this->voices[i];
		}
	}

	return voice;
}

static void start_note(HmSynth *base, int note, float velocity)
{
	Dx10 *this = (Dx10 *)base;

	float *params = this->patches[this->currentPatch].params;
	struct Voice *voice = find_next_voice(this);

	float delta = expf(MIDI_TO_FREQ_2 * ((float)note + 2.0f * params[12] - 1.0f));
	voice->note = note;
	voice->carrier.phase = 0.0f;
	voice->carrier.delta = this->tune * this->pitchBend * delta;

	if (delta > 50.0f) delta = 50.0f; //key tracking

	float depth = delta * (64.0f + this->velSens * (velocity * 127.0f - 64.0f));
	voice->modEnv.level = this->mod.initDepth * depth;
	voice->modEnv.target = this->mod.susDepth * depth;
	voice->modEnv.decay = this->mod.decay;

	float modDelta = this->mod.ratio * voice->carrier.delta;
	voice->mod.x0 = 0.0f;
	voice->mod.x1 = sinf(modDelta);
	voice->mod.d = 2.0f * cosf(modDelta);

	//scale volume with richness
	voice->env.decayLevel = (1.5f - params[13]) * this->volume * (velocity * 127.0f + 10.0f);
	voice->env.attack = this->env.attack;
	voice->env.level = 0.0f;
	voice->env.decay = this->env.decay;
}

static void stop_note(HmSynth *base, int note)
{
	Dx10 *this = (Dx10 *)base;

	for (int i = 0; i < NUM_VOICES; i++) {
		struct Voice *voice = &this->voices[i];

		if (voice->note == note) {
			if (!this->sustain) {
				voice->env.decay = this->env.release; //release phase
				voice->env.decayLevel = voice->env.level;
				voice->env.attack = 1.0f;
				voice->modEnv.target = 0.0f;
				voice->modEnv.decay = this->mod.release;

			} else {
				voice->note = SUSTAIN_NOTE;
			}
		}
	}
}

static void set_control(HmSynth *base, int control, float value)
{

}

static const char **get_params(HmSynth *base, int *numParams)
{
	*numParams = NUM_PARAMS;
	return params;
}

static float get_param(HmSynth *base, int param)
{
	Dx10 *this = (Dx10 *)base;

	return this->patches[this->currentPatch].params[param];
}

static void set_param(HmSynth *base, int param, float value)
{
	Dx10 *this = (Dx10 *)base;

	this->patches[this->currentPatch].params[param] = value;
	update_params(this);
}

static int get_num_patches(HmSynth *base)
{
	return NUM_PATCHES;
}

static void set_patch(HmSynth *base, int patch)
{
	Dx10 *this = (Dx10 *)base;

	this->currentPatch = patch;
	update_params(this);
}

static int get_patch(HmSynth *base)
{
	Dx10 *this = (Dx10 *)base;

	return this->currentPatch;
}

static void update_voices(Dx10 *this)
{
	this->activeVoices = NUM_VOICES;

	for (int i = 0; i < NUM_VOICES; i++) {
		if (this->voices[i].env.decayLevel < SILENCE) {
			this->voices[i].env.decayLevel = 0.0f;
			this->voices[i].env.level = 0.0f;
			this->activeVoices--;
		}

		if (this->voices[i].modEnv.level < SILENCE) {
			this->voices[i].modEnv.level = 0.0f;
			this->voices[i].modEnv.target = 0.0f;
		}
	}
}

static void generate(HmSynth *base, float *output, int samples)
{
	Dx10 *this = (Dx10 *)base;

	int lfoT = this->lfo.t;
	float mw = this->lfo.mw;

	update_voices(this);

	if (this->activeVoices == 0)
		return;

	while (--samples >= 0) {
		float out = 0.0f;

		if(--lfoT < 0) {
			this->lfo.x0 += this->lfo.d * this->lfo.x1; //sine LFO
			this->lfo.x1 -= this->lfo.d * this->lfo.x0;
			mw = this->lfo.x1 * (this->modWheel + this->lfo.vibrato);
			lfoT = 100;
		}

		for (int i = 0; i < NUM_VOICES; i++) {
			struct Voice *voice = &this->voices[i];

			float env = voice->env.decayLevel;
			if (env < SILENCE)
				continue;

			voice->env.decayLevel = env * voice->env.decay;
			voice->env.level += voice->env.attack * (env - voice->env.level);

			float mod = voice->mod.d * voice->mod.x0 - voice->mod.x1;
			voice->mod.x1 = voice->mod.x0;
			voice->mod.x0 = mod;
			voice->modEnv.level += voice->modEnv.decay * (voice->modEnv.target - voice->modEnv.level);

			float phase = voice->carrier.phase + voice->carrier.delta + mod * voice->modEnv.level + mw;
			while (phase >  1.0f) phase -= 2.0f;
			while (phase < -1.0f) phase += 2.0f;
			voice->carrier.phase = phase;

			float x = phase;
			float wave = x + x * x * x * (this->waveform * x * x - this->waveform - 1.0f);

			out += voice->env.level * (this->mod.mix * voice->mod.x1 + wave);
		}

		*output++ += out;
	}

	this->lfo.t = lfoT;
	this->lfo.mw = mw;
}

static void free_synth(HmSynth *synth)
{
	free(synth);
}

static void set_patch_params(Dx10 *this, int patch, float params[NUM_PARAMS])
{
	float *patch_params = this->patches[patch].params;

	for (int i = 0; i < NUM_PARAMS; i++) {
		patch_params[i] = params[i];
	}
}

static void fill_patches(Dx10 *this)
{
	set_patch_params(this,  0, (float[]){0.000, 0.650, 0.441, 0.842, 0.329, 0.230, 0.800, 0.050, 0.800, 0.900, 0.000, 0.500, 0.500, 0.447, 0.000, 0.414});
	set_patch_params(this,  1, (float[]){0.000, 0.500, 0.100, 0.671, 0.000, 0.441, 0.336, 0.243, 0.800, 0.500, 0.000, 0.500, 0.500, 0.178, 0.000, 0.500});
	set_patch_params(this,  2, (float[]){0.000, 0.700, 0.400, 0.230, 0.184, 0.270, 0.474, 0.224, 0.800, 0.974, 0.250, 0.500, 0.500, 0.428, 0.836, 0.500});
	set_patch_params(this,  3, (float[]){0.000, 0.700, 0.400, 0.320, 0.217, 0.599, 0.670, 0.309, 0.800, 0.500, 0.263, 0.507, 0.500, 0.276, 0.638, 0.526});
	set_patch_params(this,  4, (float[]){0.400, 0.600, 0.650, 0.760, 0.000, 0.390, 0.250, 0.160, 0.900, 0.500, 0.362, 0.500, 0.500, 0.401, 0.296, 0.493});
	set_patch_params(this,  5, (float[]){0.000, 0.342, 0.000, 0.280, 0.000, 0.880, 0.100, 0.408, 0.740, 0.000, 0.000, 0.600, 0.500, 0.842, 0.651, 0.500});
	set_patch_params(this,  6, (float[]){0.000, 0.400, 0.100, 0.360, 0.000, 0.875, 0.160, 0.592, 0.800, 0.500, 0.000, 0.500, 0.500, 0.303, 0.868, 0.500});
	set_patch_params(this,  7, (float[]){0.000, 0.500, 0.704, 0.230, 0.000, 0.151, 0.750, 0.493, 0.770, 0.500, 0.000, 0.400, 0.500, 0.421, 0.632, 0.500});
	set_patch_params(this,  8, (float[]){0.600, 0.990, 0.400, 0.320, 0.283, 0.570, 0.300, 0.050, 0.240, 0.500, 0.138, 0.500, 0.500, 0.283, 0.822, 0.500});
	set_patch_params(this,  9, (float[]){0.000, 0.500, 0.650, 0.368, 0.651, 0.395, 0.550, 0.257, 0.900, 0.500, 0.300, 0.800, 0.500, 0.000, 0.414, 0.500});
	set_patch_params(this, 10, (float[]){0.000, 0.700, 0.520, 0.230, 0.197, 0.520, 0.720, 0.280, 0.730, 0.500, 0.250, 0.500, 0.500, 0.336, 0.428, 0.500});
	set_patch_params(this, 11, (float[]){0.000, 0.240, 0.000, 0.390, 0.000, 0.880, 0.100, 0.600, 0.740, 0.500, 0.000, 0.500, 0.500, 0.526, 0.480, 0.500});
	set_patch_params(this, 12, (float[]){0.000, 0.500, 0.700, 0.160, 0.000, 0.158, 0.349, 0.000, 0.280, 0.900, 0.000, 0.618, 0.500, 0.401, 0.000, 0.500});
	set_patch_params(this, 13, (float[]){0.000, 0.500, 0.100, 0.390, 0.000, 0.490, 0.250, 0.250, 0.800, 0.500, 0.000, 0.500, 0.500, 0.263, 0.145, 0.500});
	set_patch_params(this, 14, (float[]){0.000, 0.300, 0.507, 0.480, 0.730, 0.000, 0.100, 0.303, 0.730, 1.000, 0.000, 0.600, 0.500, 0.579, 0.000, 0.500});
	set_patch_params(this, 15, (float[]){0.000, 0.300, 0.500, 0.320, 0.000, 0.467, 0.079, 0.158, 0.500, 0.500, 0.000, 0.400, 0.500, 0.151, 0.020, 0.500});
	set_patch_params(this, 16, (float[]){0.000, 0.990, 0.100, 0.230, 0.000, 0.000, 0.200, 0.450, 0.800, 0.000, 0.112, 0.600, 0.500, 0.711, 0.000, 0.401});
	set_patch_params(this, 17, (float[]){0.280, 0.990, 0.280, 0.230, 0.000, 0.180, 0.400, 0.300, 0.800, 0.500, 0.000, 0.400, 0.500, 0.217, 0.480, 0.500});
	set_patch_params(this, 18, (float[]){0.220, 0.990, 0.250, 0.170, 0.000, 0.240, 0.310, 0.257, 0.900, 0.757, 0.000, 0.500, 0.500, 0.697, 0.803, 0.500});
	set_patch_params(this, 19, (float[]){0.220, 0.990, 0.250, 0.450, 0.070, 0.240, 0.310, 0.360, 0.900, 0.500, 0.211, 0.500, 0.500, 0.184, 0.000, 0.414});
	set_patch_params(this, 20, (float[]){0.697, 0.990, 0.421, 0.230, 0.138, 0.750, 0.390, 0.513, 0.800, 0.316, 0.467, 0.678, 0.500, 0.743, 0.757, 0.487});
	set_patch_params(this, 21, (float[]){0.000, 0.400, 0.000, 0.280, 0.125, 0.474, 0.250, 0.100, 0.500, 0.500, 0.000, 0.400, 0.500, 0.579, 0.592, 0.500});
	set_patch_params(this, 22, (float[]){0.230, 0.500, 0.100, 0.395, 0.000, 0.388, 0.092, 0.250, 0.150, 0.500, 0.200, 0.200, 0.500, 0.178, 0.822, 0.500});
	set_patch_params(this, 23, (float[]){0.000, 0.600, 0.400, 0.230, 0.000, 0.450, 0.320, 0.050, 0.900, 0.500, 0.000, 0.200, 0.500, 0.520, 0.105, 0.500});
	set_patch_params(this, 24, (float[]){0.000, 0.600, 0.400, 0.170, 0.145, 0.290, 0.350, 0.100, 0.900, 0.500, 0.000, 0.400, 0.500, 0.441, 0.309, 0.500});
	set_patch_params(this, 25, (float[]){0.000, 0.600, 0.490, 0.170, 0.151, 0.099, 0.400, 0.000, 0.900, 0.500, 0.000, 0.400, 0.500, 0.118, 0.013, 0.500});
	set_patch_params(this, 26, (float[]){0.000, 0.600, 0.100, 0.320, 0.000, 0.350, 0.670, 0.100, 0.150, 0.500, 0.000, 0.200, 0.500, 0.303, 0.730, 0.500});
	set_patch_params(this, 27, (float[]){0.300, 0.500, 0.400, 0.280, 0.000, 0.180, 0.540, 0.000, 0.700, 0.500, 0.000, 0.400, 0.500, 0.296, 0.033, 0.500});
	set_patch_params(this, 28, (float[]){0.300, 0.500, 0.400, 0.360, 0.000, 0.461, 0.070, 0.070, 0.700, 0.500, 0.000, 0.400, 0.500, 0.546, 0.467, 0.500});
	set_patch_params(this, 29, (float[]){0.000, 0.500, 0.500, 0.280, 0.000, 0.330, 0.200, 0.000, 0.700, 0.500, 0.000, 0.500, 0.500, 0.151, 0.079, 0.500});
	set_patch_params(this, 30, (float[]){0.000, 0.500, 0.000, 0.000, 0.240, 0.580, 0.630, 0.000, 0.000, 0.500, 0.000, 0.600, 0.500, 0.816, 0.243, 0.500});
	set_patch_params(this, 31, (float[]){0.000, 0.355, 0.350, 0.000, 0.105, 0.000, 0.000, 0.200, 0.500, 0.500, 0.000, 0.645, 0.500, 1.000, 0.296, 0.500});
}

static HmSynth *init(const HmSynthType *type)
{
	Dx10 *this = malloc(sizeof(Dx10));
	if (!this)
		return NULL;

	this->base = (HmSynth){
		.type = type,
		.free = free_synth,
		.getNumPatches = get_num_patches,
		.getPatch = get_patch,
		.setPatch = set_patch,
		.getParams = get_params,
		.getParam = get_param,
		.setParam = set_param,
		.startNote = start_note,
		.stopNote = stop_note,
		.setControl = set_control,
		.generate = generate
	};

	fill_patches(this);

	this->currentPatch = 0;

	for (int i = 0; i < NUM_VOICES; i++) {
		this->voices[i] = (struct Voice){
			.env = 0,
			.carrier = {
				.phase = 0,
				.delta = 0
			},
			.mod = {
				.d = 0,
				.x0 = 0,
				.x1 = 0
			},
			.env = {
				.decay = 0.99f
			}
		};
	}

	this->activeVoices = 0;
	this->sustain = false;
	this->lfo.t = 0;

	this->lfo.x0 = this->lfo.d = this->modWheel = 0.0f;
	this->lfo.x1 = this->pitchBend = 1.0f;
	this->volume = 0.0035f;

	update_params(this);

	return &this->base;
}

AlError mda_dx10_register(HmBand *band)
{
	return hm_lib_add_synth(hm_band_get_lib(band), name, init);
}
