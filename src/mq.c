/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "mq.h"
#include "pa_memorybarrier.h"

struct HmMQ {
	SDL_mutex *lock;
	size_t messageSize;
	size_t size, wrapMask, incrMask;

	volatile size_t start, end;
	char buffer[1];
};

AlError mq_init(HmMQ **result, size_t messageSize, size_t size)
{
	BEGIN()

	if (((size - 1) & size) != 0)
		THROW(AL_ERROR_GENERIC);

	HmMQ *mq = NULL;
	TRY(al_malloc(&mq, sizeof(HmMQ) - 1 + messageSize * size, 1));

	mq->lock = SDL_CreateMutex();
	if (!mq->lock)
		THROW(AL_ERROR_GENERIC);

	mq->messageSize = messageSize;
	mq->size = size;
	mq->wrapMask = size - 1;
	mq->incrMask = 2 * size - 1;
	mq->start = 0;
	mq->end = 0;

	*result = mq;

	CATCH(
		mq_free(mq);
	)
	FINALLY()
}

void mq_free(HmMQ *mq)
{
	if (mq) {
		SDL_DestroyMutex(mq->lock);
		free(mq);
	}
}

bool mq_push(HmMQ *mq, const void *message)
{
    if (mq->end == (mq->start ^ mq->size))
		return false;

	size_t index = mq->end & mq->wrapMask;
	void *ptr = mq->buffer + index * mq->messageSize;

	PaUtil_FullMemoryBarrier();

	memcpy(ptr, message, mq->messageSize);

	PaUtil_WriteMemoryBarrier();
	mq->end = (mq->end + 1) & mq->incrMask;

    return true;
}

bool mq_push_locked(HmMQ *mq, const void *message)
{
	SDL_LockMutex(mq->lock);
	bool result = mq_push(mq, message);
	SDL_UnlockMutex(mq->lock);

	return result;
}

bool mq_pop(HmMQ *mq, void *message)
{
    if (mq->end == mq->start)
        return false;

	size_t index = mq->start & mq->wrapMask;
    void *ptr = mq->buffer + index * mq->messageSize;

    PaUtil_ReadMemoryBarrier();

	memcpy(message, ptr, mq->messageSize);

    PaUtil_FullMemoryBarrier();
    mq->start = (mq->start + 1) & mq->incrMask;

    return true;
}
