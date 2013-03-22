/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_LIB_H
#define _HAMILTON_LIB_H

#include "synth.h"

int hm_lib_init(void);
void hm_lib_free(void);

int hm_lib_add_synth(const char *name, HmSynth *(*init)(const HmSynthType *type));
const HmSynthType *hm_lib_get_synths(int *num);
const HmSynthType *hm_lib_get_synth(const char *name);

#endif
