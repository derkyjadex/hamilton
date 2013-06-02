/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdbool.h>

typedef struct HmMQ HmMQ;

int mq_init(HmMQ **result, size_t messageSize, size_t size);
void mq_free(HmMQ *mq);

bool mq_push(HmMQ *mq, const void *message);
bool mq_push_locked(HmMQ *mq, const void *message);
bool mq_pop(HmMQ *mq, void *message);
