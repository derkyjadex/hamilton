/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "hamilton/seq.h"
#include "mq.h"

typedef enum {
	ADD_NOTE,
	DEL_NOTE,
	SET_NOTE_TIME,
	SET_NOTE_LENGTH,
	SET_NOTE_NUM,
	SET_NOTE_VEL
} InMessageType;

typedef struct {
	InMessageType type;
} InMessage;

typedef enum {
	FREE_PTR
} OutMessageType;

typedef struct {
	OutMessageType type;
} OutMessage;

struct HmSeq {
	HmMQ *in;
	HmMQ *out;
};

int hm_seq_init(HmSeq **result)
{
	int error = 0;

	HmSeq *seq = malloc(sizeof(HmSeq));
	if (!seq) {
		error = 1;
		goto end;
	}

	seq->in = NULL;
	seq->out = NULL;

	error = mq_init(&seq->in, sizeof(InMessage), 128);
	if (error) goto end;

	error = mq_init(&seq->out, sizeof(OutMessage), 128);
	if (error) goto end;

	*result = seq;

end:
	if (error) {
		hm_seq_free(seq);
	}

	return error;
}

void hm_seq_free(HmSeq *seq)
{
	if (seq) {
		mq_free(seq->in);
		mq_free(seq->out);
		free(seq);
	}
}
