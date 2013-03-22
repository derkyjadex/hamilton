/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <string.h>

#include "hamilton/lib.h"

const int MAX_SYNTHS = 8;
static HmSynthType types[MAX_SYNTHS];
static int numTypes;

int hm_lib_init()
{
	return 0;
}

void hm_lib_free()
{ }

int hm_lib_add_synth(const char *name, HmSynth *(*init)(const HmSynthType *type))
{
	if (numTypes == MAX_SYNTHS)
		return 1;

	types[numTypes] = (HmSynthType){name, init};
	numTypes++;

	return 0;
}

const HmSynthType *hm_lib_get_synths(int *num)
{
	*num = numTypes;
	return types;
}

const HmSynthType *hm_lib_get_synth(const char *name)
{
	for (int i = 0; i < numTypes; i++) {
		if (!strcmp(types[i].name, name))
			return &types[i];
	}

	return NULL;
}
