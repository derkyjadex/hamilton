/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_CORE_SYNTHS_H
#define _HAMILTON_CORE_SYNTHS_H

#include "hamilton/band.h"

AlError sine_wave_register(HmBand *band);
AlError mda_dx10_register(HmBand *band);

#endif
