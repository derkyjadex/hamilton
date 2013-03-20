/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_SYNTH_H
#define _HAMILTON_SYNTH_H

struct Synth;
struct SynthType;

typedef struct Synth Synth;
typedef struct SynthType SynthType;

struct SynthType {
	const char *name;
	Synth *(*init)(const SynthType *type);
};

struct Synth {
	const SynthType *type;
	void (*free)(Synth *synth);

	const char **(*getControls)(Synth *synth, int *numControls);
	float (*getControl)(Synth *synth, int control);
	void (*setControl)(Synth *synth, int control, float value);

	void (*startNote)(Synth *synth, int num, float velocity);
	void (*stopNote)(Synth *synth, int num);

	void (*generate)(Synth *synth, float *buffer, int length);
};

#endif
