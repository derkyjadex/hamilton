/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_SEQ_H
#define _HAMILTON_SEQ_H

typedef struct HmSeq HmSeq;

int hm_seq_init(HmSeq **seq);
void hm_seq_free(HmSeq *seq);

#endif
