/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_AUDIO_H
#define _HAMILTON_AUDIO_H

#include "albase/common.h"
#include "hamilton/band.h"

AlError hm_audio_init(HmBand *band);
void hm_audio_free(void);
void hm_audio_start(void);
void hm_audio_pause(void);

#endif
