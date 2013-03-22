/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdbool.h>

typedef enum {
	NOTE_OFF = 0,
	NOTE_ON = 1,
	PITCH,
	CONTROL,
	PATCH
} MessageType;

typedef struct {
	uint32_t time;
	int channel;
	MessageType type;
	union {
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
	} data;
} Message;

int mq_init(void);
void mq_free(void);

bool mq_push(Message *message);
Message *mq_pop(void);
