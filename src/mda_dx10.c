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

typedef int32_t VstInt32;

#define NPARAMS 16       //number of parameters
#define NPROGS  32       //number of programs
#define NOUTS    2       //number of outputs
#define NVOICES  8       //max polyphony
#define SILENCE 0.0003f  //voice choking

typedef struct mdaDX10Program {
	float param[NPARAMS];
	char  name[24];
} mdaDX10Program;

//voice state
typedef struct VOICE  {
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
	VstInt32 note; //remember what note triggered this
} VOICE;

typedef struct Dx10 {
	Synth base;

	int curProgram;
	mdaDX10Program programs[NPROGS];
	float Fs;

	#define EVENTBUFFER 120
	#define EVENTS_DONE 99999999
	VstInt32 notes[EVENTBUFFER + 8];  //list of delta|note|velocity for current block

	VOICE voice[NVOICES];
	#define SUSTAIN 128
	VstInt32 sustain, activevoices, K;

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

static void fill_program(Dx10 *this, int i, float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12, float p13, float p14, float p15)
{
	float *param = this->programs[i].param;
	param[0] = p0;
	param[1] = p1;
	param[2] = p2;
	param[3] = p3;
	param[4] = p4;
	param[5] = p5;
	param[6] = p6;
	param[7] = p7;
	param[8] = p8;
	param[9] = p9;
	param[10] = p10;
	param[11] = p11;
	param[12] = p12;
	param[13] = p13;
	param[14] = p14;
	param[15] = p15;
}

//parameter change //if multitimbral would have to move all this...
void update(Dx10 *this)
{
	float ifs = 1.0f / this->Fs;
	float *param = this->programs[this->curProgram].param;

	this->tune = (float)(8.175798915644 * ifs * pow(2.0, floor(param[11] * 6.9) - 2.0));

	this->rati = param[3];
	this->rati = (float)floor(40.1f * this->rati * this->rati);

	if(param[4] < 0.5f) {
		this->ratf = 0.2f * param[4] * param[4];

	} else {
		switch((VstInt32)(8.9f * param[4])) {
			case  4: this->ratf = 0.25f;       break;
			case  5: this->ratf = 0.33333333f; break;
			case  6: this->ratf = 0.50f;       break;
			case  7: this->ratf = 0.66666667f; break;
			default: this->ratf = 0.75f;
		}
	}

	this->ratio = 1.570796326795f * (this->rati + this->ratf);

	this->depth = 0.0002f * param[5] * param[5];
	this->dept2 = 0.0002f * param[7] * param[7];

	this->velsens = param[9];
	this->vibrato = 0.001f * param[10] * param[10];

	this->catt = 1.0f - (float)exp(-ifs * exp(8.0 - 8.0 * param[0]));

	if(param[1]>0.98f) {
		this->cdec = 1.0f;
	} else {
		this->cdec = (float)exp(-ifs * exp(5.0 - 8.0 * param[1]));
	}

	this->crel =        (float)exp(-ifs * exp(5.0 - 5.0 * param[2]));
	this->mdec = 1.0f - (float)exp(-ifs * exp(6.0 - 7.0 * param[6]));
	this->mrel = 1.0f - (float)exp(-ifs * exp(5.0 - 8.0 * param[8]));

	this->rich = 0.50f - 3.0f * param[13] * param[13];
	this->modmix = 0.25f * param[14] * param[14];
	this->dlfo = 628.3f * ifs * 25.0f * param[15] * param[15]; //these params not in original DX10
}

static void noteOn(Dx10 *this, VstInt32 note, VstInt32 velocity)
{
	float *param = this->programs[this->curProgram].param;
	float l = 1.0f;
	VstInt32 v, vl = 0;

	if(velocity > 0) {
		//find quietest voice
		for (v = 0; v < NVOICES; v++) {
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
		for(v = 0; v < NVOICES; v++) {
			if(this->voice[v].note == note) {
				if(this->sustain==0) {
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

	VstInt32 npos = 0;
	this->notes[npos++] = 0;
	this->notes[npos++] = num;
	this->notes[npos++] = velocity * 127;
	this->notes[npos] = EVENTS_DONE;
}

static void stop_note(Synth *base, int num)
{
	Dx10 *this = (Dx10 *)base;

	VstInt32 npos = 0;
	this->notes[npos++] = 0;
	this->notes[npos++] = num;
	this->notes[npos++] = 0;
}

static void generate(Synth *base, float *buffer, int length)
{
	Dx10 *this = (Dx10 *)base;
	float **outputs = &buffer;
	VstInt32 sampleFrames = length;

	float* out1 = outputs[0];
//	float* out2 = outputs[1];
	VstInt32 event = 0, frame = 0, frames, v;
	float o, x, e, mw = this->MW, w = this->rich, m = this->modmix;
	VstInt32 k = this->K;

	//detect & bypass completely empty blocks
	if(this->activevoices > 0 || this->notes[event] < sampleFrames) {
		while (frame < sampleFrames) {
			frames = this->notes[event++];
			if (frames > sampleFrames) {
				frames = sampleFrames;
			}
			frames -= frame;
			frame += frames;

			//would be faster with voice loop outside frame loop!
			//but then each voice would need it's own LFO...
			while(--frames >= 0) {
				VOICE *V = this->voice;
				o = 0.0f;

				if(--k < 0) {
					this->lfo0 += this->dlfo * this->lfo1; //sine LFO
					this->lfo1 -= this->dlfo * this->lfo0;
					mw = this->lfo1 * (this->modwhl + this->vibrato);
					k = 100;
				}

				//for each voice
				for(v = 0; v < NVOICES; v++) {
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

				*out1++ += o;
//				*out2++ += o;
			}

			//next note on/off
			if (frame < sampleFrames) {
				VstInt32 note = this->notes[event++];
				VstInt32 vel  = this->notes[event++];
				noteOn(this, note, vel);
			}
		}

		this->activevoices = NVOICES;
		for(v = 0; v < NVOICES; v++) {
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

	int i = 0;
	fill_program(this, i++, 0.000f, 0.650f, 0.441f, 0.842f, 0.329f, 0.230f, 0.800f, 0.050f, 0.800f, 0.900f, 0.000f, 0.500f, 0.500f, 0.447f, 0.000f, 0.414f);
	fill_program(this, i++, 0.000f, 0.500f, 0.100f, 0.671f, 0.000f, 0.441f, 0.336f, 0.243f, 0.800f, 0.500f, 0.000f, 0.500f, 0.500f, 0.178f, 0.000f, 0.500f);
	fill_program(this, i++, 0.000f, 0.700f, 0.400f, 0.230f, 0.184f, 0.270f, 0.474f, 0.224f, 0.800f, 0.974f, 0.250f, 0.500f, 0.500f, 0.428f, 0.836f, 0.500f);
	fill_program(this, i++, 0.000f, 0.700f, 0.400f, 0.320f, 0.217f, 0.599f, 0.670f, 0.309f, 0.800f, 0.500f, 0.263f, 0.507f, 0.500f, 0.276f, 0.638f, 0.526f);
	fill_program(this, i++, 0.400f, 0.600f, 0.650f, 0.760f, 0.000f, 0.390f, 0.250f, 0.160f, 0.900f, 0.500f, 0.362f, 0.500f, 0.500f, 0.401f, 0.296f, 0.493f);
	fill_program(this, i++, 0.000f, 0.342f, 0.000f, 0.280f, 0.000f, 0.880f, 0.100f, 0.408f, 0.740f, 0.000f, 0.000f, 0.600f, 0.500f, 0.842f, 0.651f, 0.500f);
	fill_program(this, i++, 0.000f, 0.400f, 0.100f, 0.360f, 0.000f, 0.875f, 0.160f, 0.592f, 0.800f, 0.500f, 0.000f, 0.500f, 0.500f, 0.303f, 0.868f, 0.500f);
	fill_program(this, i++, 0.000f, 0.500f, 0.704f, 0.230f, 0.000f, 0.151f, 0.750f, 0.493f, 0.770f, 0.500f, 0.000f, 0.400f, 0.500f, 0.421f, 0.632f, 0.500f);
	fill_program(this, i++, 0.600f, 0.990f, 0.400f, 0.320f, 0.283f, 0.570f, 0.300f, 0.050f, 0.240f, 0.500f, 0.138f, 0.500f, 0.500f, 0.283f, 0.822f, 0.500f);
	fill_program(this, i++, 0.000f, 0.500f, 0.650f, 0.368f, 0.651f, 0.395f, 0.550f, 0.257f, 0.900f, 0.500f, 0.300f, 0.800f, 0.500f, 0.000f, 0.414f, 0.500f);
	fill_program(this, i++, 0.000f, 0.700f, 0.520f, 0.230f, 0.197f, 0.520f, 0.720f, 0.280f, 0.730f, 0.500f, 0.250f, 0.500f, 0.500f, 0.336f, 0.428f, 0.500f);
	fill_program(this, i++, 0.000f, 0.240f, 0.000f, 0.390f, 0.000f, 0.880f, 0.100f, 0.600f, 0.740f, 0.500f, 0.000f, 0.500f, 0.500f, 0.526f, 0.480f, 0.500f);
	fill_program(this, i++, 0.000f, 0.500f, 0.700f, 0.160f, 0.000f, 0.158f, 0.349f, 0.000f, 0.280f, 0.900f, 0.000f, 0.618f, 0.500f, 0.401f, 0.000f, 0.500f);
	fill_program(this, i++, 0.000f, 0.500f, 0.100f, 0.390f, 0.000f, 0.490f, 0.250f, 0.250f, 0.800f, 0.500f, 0.000f, 0.500f, 0.500f, 0.263f, 0.145f, 0.500f);
	fill_program(this, i++, 0.000f, 0.300f, 0.507f, 0.480f, 0.730f, 0.000f, 0.100f, 0.303f, 0.730f, 1.000f, 0.000f, 0.600f, 0.500f, 0.579f, 0.000f, 0.500f);
	fill_program(this, i++, 0.000f, 0.300f, 0.500f, 0.320f, 0.000f, 0.467f, 0.079f, 0.158f, 0.500f, 0.500f, 0.000f, 0.400f, 0.500f, 0.151f, 0.020f, 0.500f);
	fill_program(this, i++, 0.000f, 0.990f, 0.100f, 0.230f, 0.000f, 0.000f, 0.200f, 0.450f, 0.800f, 0.000f, 0.112f, 0.600f, 0.500f, 0.711f, 0.000f, 0.401f);
	fill_program(this, i++, 0.280f, 0.990f, 0.280f, 0.230f, 0.000f, 0.180f, 0.400f, 0.300f, 0.800f, 0.500f, 0.000f, 0.400f, 0.500f, 0.217f, 0.480f, 0.500f);
	fill_program(this, i++, 0.220f, 0.990f, 0.250f, 0.170f, 0.000f, 0.240f, 0.310f, 0.257f, 0.900f, 0.757f, 0.000f, 0.500f, 0.500f, 0.697f, 0.803f, 0.500f);
	fill_program(this, i++, 0.220f, 0.990f, 0.250f, 0.450f, 0.070f, 0.240f, 0.310f, 0.360f, 0.900f, 0.500f, 0.211f, 0.500f, 0.500f, 0.184f, 0.000f, 0.414f);
	fill_program(this, i++, 0.697f, 0.990f, 0.421f, 0.230f, 0.138f, 0.750f, 0.390f, 0.513f, 0.800f, 0.316f, 0.467f, 0.678f, 0.500f, 0.743f, 0.757f, 0.487f);
	fill_program(this, i++, 0.000f, 0.400f, 0.000f, 0.280f, 0.125f, 0.474f, 0.250f, 0.100f, 0.500f, 0.500f, 0.000f, 0.400f, 0.500f, 0.579f, 0.592f, 0.500f);
	fill_program(this, i++, 0.230f, 0.500f, 0.100f, 0.395f, 0.000f, 0.388f, 0.092f, 0.250f, 0.150f, 0.500f, 0.200f, 0.200f, 0.500f, 0.178f, 0.822f, 0.500f);
	fill_program(this, i++, 0.000f, 0.600f, 0.400f, 0.230f, 0.000f, 0.450f, 0.320f, 0.050f, 0.900f, 0.500f, 0.000f, 0.200f, 0.500f, 0.520f, 0.105f, 0.500f);
	fill_program(this, i++, 0.000f, 0.600f, 0.400f, 0.170f, 0.145f, 0.290f, 0.350f, 0.100f, 0.900f, 0.500f, 0.000f, 0.400f, 0.500f, 0.441f, 0.309f, 0.500f);
	fill_program(this, i++, 0.000f, 0.600f, 0.490f, 0.170f, 0.151f, 0.099f, 0.400f, 0.000f, 0.900f, 0.500f, 0.000f, 0.400f, 0.500f, 0.118f, 0.013f, 0.500f);
	fill_program(this, i++, 0.000f, 0.600f, 0.100f, 0.320f, 0.000f, 0.350f, 0.670f, 0.100f, 0.150f, 0.500f, 0.000f, 0.200f, 0.500f, 0.303f, 0.730f, 0.500f);
	fill_program(this, i++, 0.300f, 0.500f, 0.400f, 0.280f, 0.000f, 0.180f, 0.540f, 0.000f, 0.700f, 0.500f, 0.000f, 0.400f, 0.500f, 0.296f, 0.033f, 0.500f);
	fill_program(this, i++, 0.300f, 0.500f, 0.400f, 0.360f, 0.000f, 0.461f, 0.070f, 0.070f, 0.700f, 0.500f, 0.000f, 0.400f, 0.500f, 0.546f, 0.467f, 0.500f);
	fill_program(this, i++, 0.000f, 0.500f, 0.500f, 0.280f, 0.000f, 0.330f, 0.200f, 0.000f, 0.700f, 0.500f, 0.000f, 0.500f, 0.500f, 0.151f, 0.079f, 0.500f);
	fill_program(this, i++, 0.000f, 0.500f, 0.000f, 0.000f, 0.240f, 0.580f, 0.630f, 0.000f, 0.000f, 0.500f, 0.000f, 0.600f, 0.500f, 0.816f, 0.243f, 0.500f);
	fill_program(this, i++, 0.000f, 0.355f, 0.350f, 0.000f, 0.105f, 0.000f, 0.000f, 0.200f, 0.500f, 0.500f, 0.000f, 0.645f, 0.500f, 1.000f, 0.296f, 0.500f);

	this->Fs = SAMPLE_RATE;
	this->curProgram = -1 + 5;

	for (int i = 0; i < NVOICES; i++) {
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

	update(this);

	return &this->base;
}

int mda_dx10_register()
{
	return lib_add_synth(name, init);
}
