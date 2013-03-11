#ifndef _HAMILTON_SYNTH_H
#define _HAMILTON_SYNTH_H

#include <stdint.h>

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
	float *(*getControl)(Synth *synth, const char *control);

	void (*startNote)(Synth *synth, int num, float velocity);
	void (*stopNote)(Synth *synth, int num);

	void (*generate)(Synth *synth, int16_t *buffer, int length);
};

#endif
