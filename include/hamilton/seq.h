/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_SEQ_H
#define _HAMILTON_SEQ_H

#include "albase/common.h"

typedef struct HmSeq HmSeq;

AlError hm_seq_init(HmSeq **seq);
void hm_seq_free(HmSeq *seq);

#endif
