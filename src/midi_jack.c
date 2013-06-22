/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <stdio.h>

#include "hamilton/midi.h"

AlError hm_midi_init()
{
	return AL_NO_ERROR;
}

void hm_midi_free(void)
{
}

int hm_midi_read(uint8_t *status, uint8_t *data1, uint8_t *data2)
{
	return 0;
}
