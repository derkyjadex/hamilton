/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>

#include "hamilton/lib.h"

const int MAX_SYNTHS = 8;

struct HmLib {
	HmSynthType types[MAX_SYNTHS];
	int numTypes;
};

AlError hm_lib_init(HmLib **result)
{
	BEGIN()

	HmLib *lib = NULL;
	TRY(al_malloc(&lib, sizeof(HmLib), 1));

	lib->numTypes = 0;

	*result = lib;

	CATCH(
		hm_lib_free(lib);
	)
	FINALLY()
}

void hm_lib_free(HmLib *lib)
{
	if (lib) {
		free(lib);
	}
}

AlError hm_lib_add_synth(HmLib *lib, const char *name, HmSynth *(*init)(const HmSynthType *type))
{
	BEGIN()

	if (lib->numTypes == MAX_SYNTHS)
		THROW(AL_ERROR_GENERIC);

	lib->types[lib->numTypes] = (HmSynthType){name, init};
	lib->numTypes++;

	PASS()
}

const HmSynthType *hm_lib_get_synths(HmLib *lib, int *num)
{
	*num = lib->numTypes;
	return lib->types;
}

const HmSynthType *hm_lib_get_synth(HmLib *lib, const char *name)
{
	for (int i = 0; i < lib->numTypes; i++) {
		if (!strcmp(lib->types[i].name, name))
			return &lib->types[i];
	}

	return NULL;
}
