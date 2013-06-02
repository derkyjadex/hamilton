/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdbool.h>

typedef struct HmMQ HmMQ;

typedef enum {
	NOTE_OFF = 0,
	NOTE_ON = 1,
	PITCH,
	CONTROL,
	PATCH,
	RESET_TIME
} MessageType;

typedef union {
	struct {
		int num;
		float velocity;
	} note;
	struct {
		float offset;
	} pitch;
	struct {
		int control;
		float value;
	} control;
	int patch;
	uint64_t time;
} MessageData;

typedef struct {
	uint64_t time;
	int channel;
	MessageType type;
	MessageData data;
} Message;

int mq_init(HmMQ **mq);
void mq_free(HmMQ *mq);

bool mq_push(HmMQ *mq, const Message *message);
bool mq_pop(HmMQ *mq, Message *message);
