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

int hm_lib_init(HmLib **result)
{
	int error = 0;

	HmLib *lib = malloc(sizeof(HmLib));
	if (!lib) {
		error = 1;
		goto end;
	}

	lib->numTypes = 0;

	*result = lib;

end:
	if (error) {
		hm_lib_free(lib);
	}

	return error;
}

void hm_lib_free(HmLib *lib)
{
	if (lib) {
		free(lib);
	}
}

int hm_lib_add_synth(HmLib *lib, const char *name, HmSynth *(*init)(const HmSynthType *type))
{
	if (lib->numTypes == MAX_SYNTHS)
		return 1;

	lib->types[lib->numTypes] = (HmSynthType){name, init};
	lib->numTypes++;

	return 0;
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
