#include <string.h>

#include "private.h"

const int MAX_SYNTHS = 8;
static SynthType types[MAX_SYNTHS];
static int numTypes;

int lib_init()
{
	return 0;
}

void lib_free()
{ }

int lib_add_synth(const char *name, Synth *(*init)(const SynthType *type))
{
	if (numTypes == MAX_SYNTHS)
		return 1;

	types[numTypes] = (SynthType){name, init};
	numTypes++;

	return 0;
}

const SynthType *lib_get_synths(int *num)
{
	*num = numTypes;
	return types;
}

const SynthType *lib_get_synth(const char *name)
{
	for (int i = 0; i < numTypes; i++) {
		if (!strcmp(types[i].name, name))
			return &types[i];
	}

	return NULL;
}
