/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "hamilton/band.h"
#include "mq.h"
#include "pa_memorybarrier.h"

#define QUEUE_SIZE 256
#define SMALL_MASK (QUEUE_SIZE - 1)
#define BIG_MASK (2 * QUEUE_SIZE - 1)

struct HmMQ {
	SDL_mutex *lock;
	int start, end;
	Message buffer[QUEUE_SIZE];
};

int mq_init(HmMQ **result)
{
	int error = 0;

	HmMQ *mq = malloc(sizeof(HmMQ));
	if (!mq) {
		error = 1;
		goto end;
	}

	mq->lock = SDL_CreateMutex();
	if (!mq->lock) {
		error = 2;
		goto end;
	}

	mq->start = 0;
	mq->end = 0;

	*result = mq;

end:
	if (error) {
		mq_free(mq);
	}

	return 0;
}

void mq_free(HmMQ *mq)
{
	if (mq) {
		SDL_DestroyMutex(mq->lock);
		free(mq);
	}
}

static bool mq_push_(HmMQ *mq, const Message *message)
{
    if (mq->end == (mq->start ^ QUEUE_SIZE))
		return false;

	Message *ptr = &mq->buffer[mq->end & SMALL_MASK];

	PaUtil_FullMemoryBarrier();

	*ptr = *message;

	PaUtil_WriteMemoryBarrier();
	mq->end = (mq->end + 1) & BIG_MASK;

    return true;
}

bool mq_push(HmMQ *mq, const Message *message)
{
	SDL_LockMutex(mq->lock);
	bool result = mq_push_(mq, message);
	SDL_UnlockMutex(mq->lock);

	return result;
}

bool mq_pop(HmMQ *mq, Message *message)
{
    if (mq->end == mq->start)
        return false;

    Message *ptr = &mq->buffer[mq->start & SMALL_MASK];

    PaUtil_ReadMemoryBarrier();

    *message = *ptr;

    PaUtil_FullMemoryBarrier();
    mq->start = (mq->start + 1) & BIG_MASK;

    return true;
}
