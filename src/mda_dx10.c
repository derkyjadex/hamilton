//
// Taken from VST2 plug-in: "mda mdaDX10" v1.0
//
// Copyright(c)1999-2000 Paul Kellett (maxim digital audio)
// Based on VST2 SDK (c)1996-1999 Steinberg Soft und Hardware GmbH, All Rights Reserved
//
// Copyright (c) 2008 Paul Kellett
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "synth.h"
#include "band.h"
#include "lib.h"

int mda_dx10_register();
static const char *name = "mda DX10";
static const char *controls[] = {
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

const int NUM_CONTROLS = 16;
const int NUM_PATCHES = 32;
const int NUM_VOICES = 8;
const int SILENCE = 0.0003f;  //voice choking

const int EVENTBUFFER = 120;
const int EVENTS_DONE = 99999999;
const int SUSTAIN = 128;

struct Patch {
	float controls[NUM_CONTROLS];
	char name[24];
};

struct Voice {
	int32_t note; //remember what note triggered this
	float env;  //carrier envelope
	float dmod; //modulator oscillator
	float mod0;
	float mod1;
	float menv; //modulator envelope
	float mlev; //modulator target level
	float mdec; //modulator envelope decay
	float car;  //carrier oscillator
	float dcar;
	float cenv; //smoothed env
	float catt; //smoothing
	float cdec; //carrier envelope decay
};

typedef struct Dx10 {
	Synth base;

	struct Patch patches[NUM_PATCHES];
	int currentPatch;

	int32_t notes[EVENTBUFFER + 8];  //list of delta|note|velocity for current block

	struct Voice voice[NUM_VOICES];
	int32_t sustain, activevoices, K;

	float tune, rati, ratf, ratio; //modulator ratio
	float catt, cdec, crel;        //carrier envelope
	float depth, dept2, mdec, mrel; //modulator envelope
	float lfo0, lfo1, dlfo, modwhl, MW, pbend, velsens, volume, vibrato; //LFO and CC
	float rich, modmix;
} Dx10;

static const char **get_controls(Synth *base, int *numControls)
{
	*numControls = 0;
	return controls;
}

static float *get_control(Synth *base, const char *control)
{
	return NULL;
}

static float midi_to_freq(int note)
{
	return (note <= 0) ? 0 : 440.0 * powf(2.0, (note - 69.0) / 12.0);
}

//parameter change //if multitimbral would have to move all this...
void update_params(Dx10 *this)
{
	float ifs = 1.0f / SAMPLE_RATE;
	float *controls = this->patches[this->currentPatch].controls;

	this->tune = (float)(8.175798915644 * ifs * pow(2.0, floor(controls[11] * 6.9) - 2.0));

	this->rati = controls[3];
	this->rati = (float)floor(40.1f * this->rati * this->rati);

	if(controls[4] < 0.5f) {
		this->ratf = 0.2f * controls[4] * controls[4];

	} else {
		switch((int32_t)(8.9f * controls[4])) {
			case  4: this->ratf = 0.25f;       break;
			case  5: this->ratf = 0.33333333f; break;
			case  6: this->ratf = 0.50f;       break;
			case  7: this->ratf = 0.66666667f; break;
			default: this->ratf = 0.75f;
		}
	}

	this->ratio = 1.570796326795f * (this->rati + this->ratf);

	this->depth = 0.0002f * controls[5] * controls[5];
	this->dept2 = 0.0002f * controls[7] * controls[7];

	this->velsens = controls[9];
	this->vibrato = 0.001f * controls[10] * controls[10];

	this->catt = 1.0f - (float)exp(-ifs * exp(8.0 - 8.0 * controls[0]));

	if(controls[1]>0.98f) {
		this->cdec = 1.0f;
	} else {
		this->cdec = (float)exp(-ifs * exp(5.0 - 8.0 * controls[1]));
	}

	this->crel =        (float)exp(-ifs * exp(5.0 - 5.0 * controls[2]));
	this->mdec = 1.0f - (float)exp(-ifs * exp(6.0 - 7.0 * controls[6]));
	this->mrel = 1.0f - (float)exp(-ifs * exp(5.0 - 8.0 * controls[8]));

	this->rich = 0.50f - 3.0f * controls[13] * controls[13];
	this->modmix = 0.25f * controls[14] * controls[14];
	this->dlfo = 628.3f * ifs * 25.0f * controls[15] * controls[15]; //these params not in original DX10
}

static void note_on(Dx10 *this, int32_t note, int32_t velocity)
{
	float *param = this->patches[this->currentPatch].controls;
	float l = 1.0f;
	int32_t v, vl = 0;

	if(velocity > 0) {
		//find quietest voice
		for (v = 0; v < NUM_VOICES; v++) {
			if(this->voice[v].env < l) {
				l = this->voice[v].env;
				vl = v;
			}
		}

		l = (float)exp(0.05776226505f * ((float)note + param[12] + param[12] - 1.0f));
		this->voice[vl].note = note;                         //fine tuning
		this->voice[vl].car  = 0.0f;
		this->voice[vl].dcar = this->tune * this->pbend * l; //pitch bend not updated during note as a bit tricky...

		if (l > 50.0f) l = 50.0f; //key tracking

		l *= (64.0f + this->velsens * (velocity - 64)); //vel sens
		this->voice[vl].menv = this->depth * l;
		this->voice[vl].mlev = this->dept2 * l;
		this->voice[vl].mdec = this->mdec;

		this->voice[vl].dmod = this->ratio * this->voice[vl].dcar; //sine oscillator
		this->voice[vl].mod0 = 0.0f;
		this->voice[vl].mod1 = (float)sin(this->voice[vl].dmod);
		this->voice[vl].dmod = 2.0f * (float)cos(this->voice[vl].dmod);
		//scale volume with richness
		this->voice[vl].env  = (1.5f - param[13]) * this->volume * (velocity + 10);
		this->voice[vl].catt = this->catt;
		this->voice[vl].cenv = 0.0f;
		this->voice[vl].cdec = this->cdec;

	} else {
		//note off
		//any voices playing that note?
		for (v = 0; v < NUM_VOICES; v++) {
			if (this->voice[v].note == note) {
				if (this->sustain == 0) {
					this->voice[v].cdec = this->crel; //release phase
					this->voice[v].env  = this->voice[v].cenv;
					this->voice[v].catt = 1.0f;
					this->voice[v].mlev = 0.0f;
					this->voice[v].mdec = this->mrel;

				} else {
					this->voice[v].note = SUSTAIN;
				}
			}
		}
	}
}

static void start_note(Synth *base, int num, float velocity)
{
	Dx10 *this = (Dx10 *)base;

	int32_t npos = 0;
	this->notes[npos++] = 0;
	this->notes[npos++] = num;
	this->notes[npos++] = velocity * 127;
	this->notes[npos] = EVENTS_DONE;
}

static void stop_note(Synth *base, int num)
{
	Dx10 *this = (Dx10 *)base;

	int32_t npos = 0;
	this->notes[npos++] = 0;
	this->notes[npos++] = num;
	this->notes[npos++] = 0;
}

static void generate(Synth *base, float *output, int samples)
{
	Dx10 *this = (Dx10 *)base;

	int32_t event = 0, frame = 0, frames, v;
	float o, x, e, mw = this->MW, w = this->rich, m = this->modmix;
	int32_t k = this->K;

	//detect & bypass completely empty blocks
	if(this->activevoices > 0 || this->notes[event] < samples) {
		while (frame < samples) {
			frames = this->notes[event++];
			if (frames > samples) {
				frames = samples;
			}
			frames -= frame;
			frame += frames;

			//would be faster with voice loop outside frame loop!
			//but then each voice would need it's own LFO...
			while(--frames >= 0) {
				struct Voice *V = this->voice;
				o = 0.0f;

				if(--k < 0) {
					this->lfo0 += this->dlfo * this->lfo1; //sine LFO
					this->lfo1 -= this->dlfo * this->lfo0;
					mw = this->lfo1 * (this->modwhl + this->vibrato);
					k = 100;
				}

				//for each voice
				for(v = 0; v < NUM_VOICES; v++) {
					e = V->env;

					//**** this is the synth ****
					if(e > SILENCE) {
						V->env = e * V->cdec; //decay & release
						V->cenv += V->catt * (e - V->cenv); //attack

						x = V->dmod * V->mod0 - V->mod1; //could add more modulator blocks like
						V->mod1 = V->mod0;               //this for a wider range of FM sounds
						V->mod0 = x;
						V->menv += V->mdec * (V->mlev - V->menv);

						x = V->car + V->dcar + x * V->menv + mw; //carrier phase
						while(x >  1.0f) x -= 2.0f;  //wrap phase
						while(x < -1.0f) x += 2.0f;
						V->car = x;
						o += V->cenv * (m * V->mod1 + (x + x * x * x * (w * x * x - 1.0f - w)));
					}        //amp env //mod thru-mix //5th-order sine approximation

					V++;
				}

				*output++ += o;
			}

			//next note on/off
			if (frame < samples) {
				int32_t note = this->notes[event++];
				int32_t vel  = this->notes[event++];
				note_on(this, note, vel);
			}
		}

		this->activevoices = NUM_VOICES;
		for(v = 0; v < NUM_VOICES; v++) {
			//choke voices that have finished
			if(this->voice[v].env < SILENCE) {
				this->voice[v].env = this->voice[v].cenv = 0.0f;
				this->activevoices--;
			}

			if(this->voice[v].menv < SILENCE) {
				this->voice[v].menv = this->voice[v].mlev = 0.0f;
			}
		}
	}

	this->K = k;
	this->MW = mw;
	this->notes[0] = EVENTS_DONE;
}

static void free_synth(Synth *synth)
{
	free(synth);
}

static void set_patch(Dx10 *this, int patch, float controls[NUM_CONTROLS])
{
	float *patch_controls = this->patches[patch].controls;

	for (int i = 0; i < NUM_CONTROLS; i++) {
		patch_controls[i] = controls[i];
	}
}

static void set_patches(Dx10 *this)
{
	set_patch(this,  0, (float[]){0.000, 0.650, 0.441, 0.842, 0.329, 0.230, 0.800, 0.050, 0.800, 0.900, 0.000, 0.500, 0.500, 0.447, 0.000, 0.414});
	set_patch(this,  1, (float[]){0.000, 0.500, 0.100, 0.671, 0.000, 0.441, 0.336, 0.243, 0.800, 0.500, 0.000, 0.500, 0.500, 0.178, 0.000, 0.500});
	set_patch(this,  2, (float[]){0.000, 0.700, 0.400, 0.230, 0.184, 0.270, 0.474, 0.224, 0.800, 0.974, 0.250, 0.500, 0.500, 0.428, 0.836, 0.500});
	set_patch(this,  3, (float[]){0.000, 0.700, 0.400, 0.320, 0.217, 0.599, 0.670, 0.309, 0.800, 0.500, 0.263, 0.507, 0.500, 0.276, 0.638, 0.526});
	set_patch(this,  4, (float[]){0.400, 0.600, 0.650, 0.760, 0.000, 0.390, 0.250, 0.160, 0.900, 0.500, 0.362, 0.500, 0.500, 0.401, 0.296, 0.493});
	set_patch(this,  5, (float[]){0.000, 0.342, 0.000, 0.280, 0.000, 0.880, 0.100, 0.408, 0.740, 0.000, 0.000, 0.600, 0.500, 0.842, 0.651, 0.500});
	set_patch(this,  6, (float[]){0.000, 0.400, 0.100, 0.360, 0.000, 0.875, 0.160, 0.592, 0.800, 0.500, 0.000, 0.500, 0.500, 0.303, 0.868, 0.500});
	set_patch(this,  7, (float[]){0.000, 0.500, 0.704, 0.230, 0.000, 0.151, 0.750, 0.493, 0.770, 0.500, 0.000, 0.400, 0.500, 0.421, 0.632, 0.500});
	set_patch(this,  8, (float[]){0.600, 0.990, 0.400, 0.320, 0.283, 0.570, 0.300, 0.050, 0.240, 0.500, 0.138, 0.500, 0.500, 0.283, 0.822, 0.500});
	set_patch(this,  9, (float[]){0.000, 0.500, 0.650, 0.368, 0.651, 0.395, 0.550, 0.257, 0.900, 0.500, 0.300, 0.800, 0.500, 0.000, 0.414, 0.500});
	set_patch(this, 10, (float[]){0.000, 0.700, 0.520, 0.230, 0.197, 0.520, 0.720, 0.280, 0.730, 0.500, 0.250, 0.500, 0.500, 0.336, 0.428, 0.500});
	set_patch(this, 11, (float[]){0.000, 0.240, 0.000, 0.390, 0.000, 0.880, 0.100, 0.600, 0.740, 0.500, 0.000, 0.500, 0.500, 0.526, 0.480, 0.500});
	set_patch(this, 12, (float[]){0.000, 0.500, 0.700, 0.160, 0.000, 0.158, 0.349, 0.000, 0.280, 0.900, 0.000, 0.618, 0.500, 0.401, 0.000, 0.500});
	set_patch(this, 13, (float[]){0.000, 0.500, 0.100, 0.390, 0.000, 0.490, 0.250, 0.250, 0.800, 0.500, 0.000, 0.500, 0.500, 0.263, 0.145, 0.500});
	set_patch(this, 14, (float[]){0.000, 0.300, 0.507, 0.480, 0.730, 0.000, 0.100, 0.303, 0.730, 1.000, 0.000, 0.600, 0.500, 0.579, 0.000, 0.500});
	set_patch(this, 15, (float[]){0.000, 0.300, 0.500, 0.320, 0.000, 0.467, 0.079, 0.158, 0.500, 0.500, 0.000, 0.400, 0.500, 0.151, 0.020, 0.500});
	set_patch(this, 16, (float[]){0.000, 0.990, 0.100, 0.230, 0.000, 0.000, 0.200, 0.450, 0.800, 0.000, 0.112, 0.600, 0.500, 0.711, 0.000, 0.401});
	set_patch(this, 17, (float[]){0.280, 0.990, 0.280, 0.230, 0.000, 0.180, 0.400, 0.300, 0.800, 0.500, 0.000, 0.400, 0.500, 0.217, 0.480, 0.500});
	set_patch(this, 18, (float[]){0.220, 0.990, 0.250, 0.170, 0.000, 0.240, 0.310, 0.257, 0.900, 0.757, 0.000, 0.500, 0.500, 0.697, 0.803, 0.500});
	set_patch(this, 19, (float[]){0.220, 0.990, 0.250, 0.450, 0.070, 0.240, 0.310, 0.360, 0.900, 0.500, 0.211, 0.500, 0.500, 0.184, 0.000, 0.414});
	set_patch(this, 20, (float[]){0.697, 0.990, 0.421, 0.230, 0.138, 0.750, 0.390, 0.513, 0.800, 0.316, 0.467, 0.678, 0.500, 0.743, 0.757, 0.487});
	set_patch(this, 21, (float[]){0.000, 0.400, 0.000, 0.280, 0.125, 0.474, 0.250, 0.100, 0.500, 0.500, 0.000, 0.400, 0.500, 0.579, 0.592, 0.500});
	set_patch(this, 22, (float[]){0.230, 0.500, 0.100, 0.395, 0.000, 0.388, 0.092, 0.250, 0.150, 0.500, 0.200, 0.200, 0.500, 0.178, 0.822, 0.500});
	set_patch(this, 23, (float[]){0.000, 0.600, 0.400, 0.230, 0.000, 0.450, 0.320, 0.050, 0.900, 0.500, 0.000, 0.200, 0.500, 0.520, 0.105, 0.500});
	set_patch(this, 24, (float[]){0.000, 0.600, 0.400, 0.170, 0.145, 0.290, 0.350, 0.100, 0.900, 0.500, 0.000, 0.400, 0.500, 0.441, 0.309, 0.500});
	set_patch(this, 25, (float[]){0.000, 0.600, 0.490, 0.170, 0.151, 0.099, 0.400, 0.000, 0.900, 0.500, 0.000, 0.400, 0.500, 0.118, 0.013, 0.500});
	set_patch(this, 26, (float[]){0.000, 0.600, 0.100, 0.320, 0.000, 0.350, 0.670, 0.100, 0.150, 0.500, 0.000, 0.200, 0.500, 0.303, 0.730, 0.500});
	set_patch(this, 27, (float[]){0.300, 0.500, 0.400, 0.280, 0.000, 0.180, 0.540, 0.000, 0.700, 0.500, 0.000, 0.400, 0.500, 0.296, 0.033, 0.500});
	set_patch(this, 28, (float[]){0.300, 0.500, 0.400, 0.360, 0.000, 0.461, 0.070, 0.070, 0.700, 0.500, 0.000, 0.400, 0.500, 0.546, 0.467, 0.500});
	set_patch(this, 29, (float[]){0.000, 0.500, 0.500, 0.280, 0.000, 0.330, 0.200, 0.000, 0.700, 0.500, 0.000, 0.500, 0.500, 0.151, 0.079, 0.500});
	set_patch(this, 30, (float[]){0.000, 0.500, 0.000, 0.000, 0.240, 0.580, 0.630, 0.000, 0.000, 0.500, 0.000, 0.600, 0.500, 0.816, 0.243, 0.500});
	set_patch(this, 31, (float[]){0.000, 0.355, 0.350, 0.000, 0.105, 0.000, 0.000, 0.200, 0.500, 0.500, 0.000, 0.645, 0.500, 1.000, 0.296, 0.500});
}

static Synth *init(const SynthType *type)
{
	Dx10 *this = malloc(sizeof(Dx10));
	if (!this)
		return NULL;

	this->base = (Synth){
		.type = type,
		.free = free_synth,
		.getControls = get_controls,
		.getControl = get_control,
		.startNote = start_note,
		.stopNote = stop_note,
		.generate = generate
	};

	set_patches(this);

	this->currentPatch = 0;

	for (int i = 0; i < NUM_VOICES; i++) {
		this->voice[i].env = 0.0f;
		this->voice[i].car = this->voice[i].dcar = 0.0f;
		this->voice[i].mod0 = this->voice[i].mod1 = this->voice[i].dmod = 0.0f;
		this->voice[i].cdec = 0.99f;
	}

	this->notes[0] = EVENTS_DONE;
	this->lfo0 = this->dlfo = this->modwhl = 0.0f;
	this->lfo1 = this->pbend = 1.0f;
	this->volume = 0.0035f;
	this->sustain = this->activevoices = 0;
	this->K = 0;

	update_params(this);

	return &this->base;
}

int mda_dx10_register()
{
	return lib_add_synth(name, init);
}
