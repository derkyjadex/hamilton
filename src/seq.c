/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <stddef.h>
#include <math.h>

#include "hamilton/seq.h"
#include "albase/mq.h"

typedef struct EventNode EventNode;

struct EventNode {
	EventNode *prev, *next;
	HmEvent event;
};

#define GET_NOTE(node) ((HmNote *)((void *)(node) - offsetof(HmNote, on)))

struct HmNote {
	HmNoteData data;
	EventNode on, off;
};

typedef struct {
	enum {
		SWAP_ARRAY
	} type;

	union {
		struct {
			HmEvent *ptr;
			int length;
		} array;
	} data;
} ToAudioMessage;

typedef struct {
	enum {
		FREE_PTR
	} type;
	union {
		void *ptr;
	} data;
} FromAudioMessage;

struct HmSeq {
	AlMQ *toAudio;
	AlMQ *fromAudio;

	EventNode *head, *tail;
	int numEvents;
	HmEvent *array;
	int arrayLength;
};

AlError hm_seq_init(HmSeq **result)
{
	BEGIN()

	HmSeq *seq = NULL;
	TRY(al_malloc(&seq, sizeof(HmSeq), 1));

	seq->toAudio = NULL;
	seq->fromAudio = NULL;

	seq->head = NULL;
	seq->tail = NULL;
	seq->numEvents = 0;

	seq->array = NULL;
	seq->arrayLength = 0;

	TRY(al_mq_init(&seq->toAudio, sizeof(ToAudioMessage), 128));
	TRY(al_mq_init(&seq->fromAudio, sizeof(FromAudioMessage), 128));

	*result = seq;

	CATCH(
		hm_seq_free(seq);
	)
	FINALLY()
}

static void remove_node(HmSeq *seq, EventNode *node);
static void update_sequence(HmSeq *seq);

void hm_seq_free(HmSeq *seq)
{
	if (seq) {
		update_sequence(seq);
		hm_seq_process_messages(seq);

		EventNode *node = seq->head;
		while (node) {
			void *freePtr;

			if (node->event.type == HM_EV_NOTE_ON) {
				HmNote *note = GET_NOTE(node);
				remove_node(seq, &note->off);
				freePtr = note;

			} else {
				freePtr = node;
			}

			node = node->next;
			free(freePtr);
		}

		al_mq_free(seq->toAudio);
		al_mq_free(seq->fromAudio);
		free(seq->array);
		free(seq);
	}
}

static void insert_node(HmSeq *seq, EventNode *node)
{
	EventNode *next = seq->head;

	if (!next) {
		seq->head = node;
		seq->tail = node;

	} else {
		while (next && next->event.time <= node->event.time) {
			next = next->next;
		}

		if (!next) {
			seq->tail->next = node;
			node->prev = seq->tail;
			seq->tail = node;

		} else if (!next->prev) {
			seq->head = node;
			next->prev = node;
			node->next = next;

		} else {
			next->prev->next = node;
			node->prev = next->prev;
			next->prev = node;
			node->next = next;
		}
	}

	seq->numEvents++;
}

static void remove_node(HmSeq *seq, EventNode *node)
{
	if (node->prev) {
		node->prev->next = node->next;
	} else {
		seq->head = node->next;
	}

	if (node->next) {
		node->next->prev = node->prev;
	} else {
		seq->tail = node->prev;
	}

	node->prev = NULL;
	node->next = NULL;

	seq->numEvents--;
}

static EventNode *find_node(EventNode *start, uint32_t time, int channel, HmEventType type)
{
	for (EventNode *node = start; node && node->event.time <= time; node = node->next) {
		if (node->event.time == time && node->event.channel == channel && node->event.type == type)
			return node;
	}

	return NULL;
}

static EventNode *find_control_node(EventNode *start, uint32_t time, int channel, int control)
{
	EventNode *node = start;
	while ((node = find_node(node, time, channel, HM_EV_CONTROL))) {
		if (node->event.data.control.num == control)
			return node;

		node = node->next;
	}

	return NULL;
}

static EventNode *find_param_node(EventNode *start, uint32_t time, int channel, int param)
{
	EventNode *node = start;
	while ((node = find_node(node, time, channel, HM_EV_PARAM))) {
		if (node->event.data.param.num == param)
			return node;

		node = node->next;
	}

	return NULL;
}

static void free_from_audio(HmSeq *seq, void *ptr)
{
	FromAudioMessage message = {
		.type = FREE_PTR,
		.data = {
			.ptr = ptr
		}
	};

	al_mq_push(seq->fromAudio, &message);
}

static void update_sequence(HmSeq *seq)
{
	ToAudioMessage message;
	while (al_mq_pop(seq->toAudio, &message)) {
		switch (message.type) {
			case SWAP_ARRAY:
				free_from_audio(seq, seq->array);
				seq->array = message.data.array.ptr;
				seq->arrayLength = message.data.array.length;
				break;
		}
	}
}

static HmEvent *find_first_event(HmEvent *events, int length, uint32_t time)
{
	if (length == 0)
		return events;

	int a = 0;
	int b = length - 1;

	while (a < b) {
		int m = a + (b - a) / 2;

		if (events[m].time < time) {
			a = m + 1;
		} else {
			b = m;
		}
	}

	if (events[a].time < time) {
		a = length;
	}

	return events + a;
}

int hm_seq_get_events(HmSeq *seq, HmEvent *dest, int numEvents, uint64_t start, uint64_t end, double sampleRate)
{
	update_sequence(seq);

	uint32_t startTick = ceil((double)start * HM_SEQ_TICK_RATE / sampleRate);
	uint32_t endTick = ceil((double)end * HM_SEQ_TICK_RATE / sampleRate) - 1;

	HmEvent *src = find_first_event(seq->array, seq->arrayLength, startTick);
	HmEvent *srcEnd = seq->array + seq->arrayLength;

	int n = 0;
	while (src < srcEnd && src->time <= endTick && n < numEvents) {
		*dest = *src;
		dest->time = (dest->time - startTick) * sampleRate / HM_SEQ_TICK_RATE;

		src++;
		dest++;
		n++;
	}

	return n;
}

void hm_seq_process_messages(HmSeq *seq)
{
	FromAudioMessage audioMessage;
	while (al_mq_pop(seq->fromAudio, &audioMessage)) {
		switch (audioMessage.type) {
			case FREE_PTR:
				free(audioMessage.data.ptr);
				break;
		}
	}
}

AlError hm_seq_get_items(HmSeq *seq, HmSeqItem **result, int *resultLength)
{
	BEGIN()

	HmSeqItem *items = NULL;
	int numItems = 0;
	TRY(al_malloc(&items, sizeof(HmSeqItem), seq->numEvents));

	HmSeqItem *item = items;
	for (EventNode *node = seq->head; node; node = node->next, item++) {
		switch (node->event.type) {
			case HM_EV_NOTE_ON:
				*item = (HmSeqItem){
					.type = HM_SEQ_NOTE,
					.time = node->event.time,
					.channel = node->event.channel,
					.data = {
						.note = {
							.note = GET_NOTE(node),
							.data = GET_NOTE(node)->data
						}
					}
				};
				break;

			case HM_EV_NOTE_OFF:
				break;

			case HM_EV_PITCH:
				*item = (HmSeqItem){
					.type = HM_SEQ_PITCH,
					.time = node->event.time,
					.channel = node->event.channel,
					.data = {
						.pitch = node->event.data.pitch
					}
				};
				break;

			case HM_EV_CONTROL:
				*item = (HmSeqItem){
					.type = HM_SEQ_CONTROL,
					.time = node->event.time,
					.channel = node->event.channel,
					.data = {
						.control = {
							.num = node->event.data.control.num,
							.value = node->event.data.control.value
						}
					}
				};
				break;

			case HM_EV_PARAM:
				*item = (HmSeqItem){
					.type = HM_SEQ_PARAM,
					.time = node->event.time,
					.channel = node->event.channel,
					.data = {
						.param = {
							.num = node->event.data.param.num,
							.value = node->event.data.param.value
						}
					}
				};
				break;

			case HM_EV_PATCH:
				*item = (HmSeqItem){
					.type = HM_SEQ_PATCH,
					.time = node->event.time,
					.channel = node->event.channel,
					.data = {
						.patch = node->event.data.patch
					}
				};
				break;
		}
	}

	*result = items;
	*resultLength = numItems;

	CATCH(
		free(items);
	)
	FINALLY()
}

AlError hm_seq_add_note(HmSeq *seq, int channel, uint32_t time, HmNoteData *data)
{
	BEGIN()

	HmNote *note = NULL;
	TRY(al_malloc(&note, sizeof(HmNote), 1));

	note->data = *data;
	note->on = (EventNode){
		.prev = NULL,
		.next = NULL,
		.event = (HmEvent){
			.time = time,
			.channel = channel,
			.type = HM_EV_NOTE_ON,
			.data = {
				.note = {
					.num = data->num,
					.velocity = data->velocity
				}
			}
		}
	};
	note->off = (EventNode){
		.prev = NULL,
		.next = NULL,
		.event = (HmEvent){
			.time = data + data->length,
			.channel = channel,
			.type = HM_EV_NOTE_OFF,
			.data = {
				.note = {
					.num = data->num,
					.velocity = 0
				}
			}
		}
	};

	insert_node(seq, &note->on);
	insert_node(seq, &note->off);

	CATCH(
		free(note);
	)
	FINALLY()
}

AlError hm_seq_remove_note(HmSeq *seq, HmNote *note)
{
	BEGIN()

	remove_node(seq, &note->on);
	remove_node(seq, &note->off);
	free(note);

	PASS()
}

AlError hm_seq_update_note(HmSeq *seq, HmNote *note, uint32_t time, HmNoteData *data)
{
	BEGIN()

	if (note->on.event.time != time || note->data.length != data->length) {
		remove_node(seq, &note->on);
		remove_node(seq, &note->off);

		note->on.event.time = time;
		note->off.event.time = time + data->length;

		insert_node(seq, &note->on);
		insert_node(seq, &note->off);
	}

	note->data = *data;
	note->on.event.data.note.num = data->num;
	note->on.event.data.note.velocity = data->velocity;
	note->off.event.data.note.num = data->num;

	PASS()
}

AlError hm_seq_set_pitch(HmSeq *seq, int channel, uint32_t time, float pitch)
{
	BEGIN()

	EventNode *node = find_node(seq->head, time, channel, HM_EV_PITCH);

	if (node) {
		node->event.data.pitch = pitch;

	} else {
		TRY(al_malloc(&node, sizeof(EventNode), 1));

		node->prev = NULL;
		node->next = NULL;
		node->event = (HmEvent){
			.time = time,
			.channel = channel,
			.type = HM_EV_PITCH,
			.data = {
				.pitch = pitch
			}
		};

		insert_node(seq, node);
	}

	PASS()
}

AlError hm_seq_clear_pitch(HmSeq *seq, int channel, uint32_t time)
{
	BEGIN()

	EventNode *node = find_node(seq->head, time, channel, HM_EV_PITCH);
	if (node) {
		remove_node(seq, node);
		free(node);
	}

	PASS()
}

AlError hm_seq_set_control(HmSeq *seq, int channel, uint32_t time, int control, float value)
{
	BEGIN()

	EventNode *node = find_control_node(seq->head, time, channel, control);

	if (node) {
		node->event.data.control.value = value;

	} else {
		TRY(al_malloc(&node, sizeof(EventNode), 1));

		node->prev = NULL;
		node->next = NULL;
		node->event = (HmEvent){
			.time = time,
			.channel = channel,
			.type = HM_EV_CONTROL,
			.data = {
				.control = {
					.num = control,
					.value = value
				}
			}
		};

		insert_node(seq, node);
	}

	PASS()
}

AlError hm_seq_clear_control(HmSeq *seq, int channel, uint32_t time, int control)
{
	BEGIN()

	EventNode *node = find_control_node(seq->head, time, channel, control);
	if (node) {
		remove_node(seq, node);
		free(node);
	}

	PASS()
}

AlError hm_seq_set_param(HmSeq *seq, int channel, uint32_t time, int param, float value)
{
	BEGIN()

	EventNode *node = find_param_node(seq->head, time, channel, param);

	if (node) {
		node->event.data.param.value = value;

	} else {
		TRY(al_malloc(&node, sizeof(EventNode), 1));

		node->prev = NULL;
		node->next = NULL;
		node->event = (HmEvent){
			.time = time,
			.channel = channel,
			.type = HM_EV_PARAM,
			.data = {
				.param = {
					.num = param,
					.value = value
				}
			}
		};

		insert_node(seq, node);
	}

	PASS()
}

AlError hm_seq_clear_param(HmSeq *seq, int channel, uint32_t time, int param)
{
	BEGIN()

	EventNode *node = find_param_node(seq->head, time, channel, param);
	if (node) {
		remove_node(seq, node);
		free(node);
	}

	PASS()
}

AlError hm_seq_set_patch(HmSeq *seq, int channel, uint32_t time, int patch)
{
	BEGIN()

	EventNode *node = find_node(seq->head, time, channel, HM_EV_PATCH);

	if (node) {
		node->event.data.patch = patch;

	} else {
		TRY(al_malloc(&node, sizeof(EventNode), 1));

		node->prev = NULL;
		node->next = NULL;
		node->event = (HmEvent){
			.time = time,
			.channel = channel,
			.type = HM_EV_PATCH,
			.data = {
				.patch = patch
			}
		};

		insert_node(seq, node);
	}

	PASS()
}

AlError hm_seq_clear_patch(HmSeq *seq, int channel, uint32_t time)
{
	BEGIN()

	EventNode *node = find_node(seq->head, time, channel, HM_EV_PATCH);
	if (node) {
		remove_node(seq, node);
		free(node);
	}

	PASS()
}

AlError hm_seq_commit(HmSeq *seq)
{
	BEGIN()

	HmEvent *array = NULL;
	TRY(al_malloc(&array, sizeof(HmEvent), seq->numEvents));

	HmEvent *event = array;
	for (EventNode *node = seq->head; node; node = node->next, event++) {
		*event = node->event;
	}

	ToAudioMessage message = {
		.type = SWAP_ARRAY,
		.data = {
			.array = {
				.ptr = array,
				.length = seq->numEvents
			}
		}
	};

	if (!al_mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	CATCH(
		free(array);
	)
	FINALLY()
}
