#include <stdbool.h>
#include <SDL/SDL.h>

#include "private.h"

static SDL_mutex *lock;

const size_t QUEUE_SIZE = 128;
static Message _a[QUEUE_SIZE],
			   _b[QUEUE_SIZE];

static Message *reading = _a,
			   *writing = _b;

static int readNext = 0,
		   readLength = 0,
		   writeNext = 0;

int mq_init()
{
	lock = SDL_CreateMutex();

	return 0;
}

void mq_free()
{
	SDL_DestroyMutex(lock);
}

bool mq_push(Message *message) {
	SDL_LockMutex(lock);

	bool result = false;

	if (writeNext < QUEUE_SIZE) {
		writing[writeNext] = *message;
		writeNext++;
		result = true;
	}

	SDL_UnlockMutex(lock);

	return result;
}

static bool swap_buffers()
{
	SDL_LockMutex(lock);

	bool result = false;

	if (writeNext != 0) {
		Message *temp = reading;
		reading = writing;
		writing = temp;

		readNext = 0;
		readLength = writeNext;
		writeNext = 0;

		result = true;
	}

	SDL_UnlockMutex(lock);

	return result;
}

Message *mq_pop()
{
	if (readNext == readLength) {
		if (!swap_buffers())
			return NULL;
	}

	Message *message = &reading[readNext];
	readNext++;

	return message;
}

bool mq_note(int time, int channel, bool state, int num, float velocity)
{
	Message message = {
		.time = time,
		.type = (state) ? NOTE_ON : NOTE_OFF,
		.channel = channel,
		.data = {
			.note = {
				.num = num,
				.velocity = velocity
			}
		}
	};

	return mq_push(&message);
}

bool mq_pitch(int time, int channel, float offset)
{
	Message message = {
		.time = time,
		.channel = channel,
		.type = PITCH,
		.data = {
			.pitch = {
				.offset = offset
			}
		}
	};

	return mq_push(&message);
}

bool mq_cc(int time, int channel, float *control, float value)
{
	Message message = {
		.time = time,
		.channel = channel,
		.type = CONTROL,
		.data = {
			.control = {
				.ptr = control,
				.value = value
			}
		}
	};

	return mq_push(&message);
}
