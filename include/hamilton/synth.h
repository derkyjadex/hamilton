/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_SYNTH_H
#define _HAMILTON_SYNTH_H

struct HmSynth;
struct HmSynthType;

typedef struct HmSynth HmSynth;
typedef struct HmSynthType HmSynthType;

struct HmSynthType {
	const char *name;
	HmSynth *(*init)(const HmSynthType *type);
};

struct HmSynth {
	const HmSynthType *type;
	void (*free)(HmSynth *synth);

	const char **(*getControls)(HmSynth *synth, int *numControls);
	float (*getControl)(HmSynth *synth, int control);
	void (*setControl)(HmSynth *synth, int control, float value);

	int (*getNumPatches)(HmSynth *synth);
	void (*setPatch)(HmSynth *synth, int patch);
	int (*getPatch)(HmSynth *synth);

	void (*startNote)(HmSynth *synth, int note, float velocity);
	void (*stopNote)(HmSynth *synth, int note);

	void (*generate)(HmSynth *synth, float *buffer, int length);
};

#endif
