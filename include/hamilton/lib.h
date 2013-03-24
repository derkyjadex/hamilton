/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_LIB_H
#define _HAMILTON_LIB_H

#include "synth.h"

typedef struct HmLib HmLib;

int hm_lib_init(HmLib **lib);
void hm_lib_free(HmLib *lib);

int hm_lib_add_synth(HmLib *lib, const char *name, HmSynth *(*init)(const HmSynthType *type));
const HmSynthType *hm_lib_get_synths(HmLib *lib, int *num);
const HmSynthType *hm_lib_get_synth(HmLib *lib, const char *name);

#endif
