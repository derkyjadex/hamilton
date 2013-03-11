#ifndef _HAMILTON_LIB_H
#define _HAMILTON_LIB_H

#include "synth.h"

int lib_init(void);
void lib_free(void);

int lib_add_synth(const char *name, Synth *(*init)(const SynthType *type));
const SynthType *lib_get_synths(int *num);
const SynthType *lib_get_synth(const char *name);

#endif
