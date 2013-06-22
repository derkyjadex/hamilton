/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_MIDI_H
#define _HAMILTON_MIDI_H

#include <stdint.h>

#include "albase/common.h"

AlError hm_midi_init(void);
void hm_midi_free(void);
int hm_midi_read(uint8_t *status, uint8_t *data1, uint8_t *data2);

#endif
