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

const size_t QUEUE_SIZE = 128;

struct HmMQ {
	SDL_mutex *lock;
	Message *reading, *writing;
	int readNext, readLength, writeNext;
	Message _buffer[QUEUE_SIZE * 2];
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

	mq->reading = mq->_buffer;
	mq->writing = mq->_buffer + QUEUE_SIZE;
	mq->readNext = 0;
	mq->readLength = 0;
	mq->writeNext = 0;

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

bool mq_push(HmMQ *mq, Message *message) {
	SDL_LockMutex(mq->lock);

	bool result = false;

	if (mq->writeNext < QUEUE_SIZE) {
		mq->writing[mq->writeNext] = *message;
		mq->writeNext++;
		result = true;
	}

	SDL_UnlockMutex(mq->lock);

	return result;
}

static bool swap_buffers(HmMQ *mq)
{
	SDL_LockMutex(mq->lock);

	bool result = false;

	if (mq->writeNext != 0) {
		Message *temp = mq->reading;
		mq->reading = mq->writing;
		mq->writing = temp;

		mq->readNext = 0;
		mq->readLength = mq->writeNext;
		mq->writeNext = 0;

		result = true;
	}

	SDL_UnlockMutex(mq->lock);

	return result;
}

Message *mq_pop(HmMQ *mq)
{
	if (mq->readNext == mq->readLength) {
		if (!swap_buffers(mq))
			return NULL;
	}

	Message *message = &mq->reading[mq->readNext];
	mq->readNext++;

	return message;
}
