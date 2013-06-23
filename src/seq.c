/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>
#include <stddef.h>

#include "hamilton/seq.h"
#include "mq.h"

typedef struct EventNode EventNode;

struct EventNode {
	EventNode *prev, *next;
	HmEvent event;
};

struct HmNote {
	HmNoteData data;
	EventNode on, off;
};

typedef struct {
	enum {
		ADD_NOTE,
		REMOVE_NOTE,
		UPDATE_NOTE,
		SET_PITCH,
		CLEAR_PITCH,
		SET_CONTROL,
		CLEAR_CONTROL,
		SET_PATCH,
		CLEAR_PATCH
	} type;

	union {
		HmNote *addNote, *removeNote;
		struct {
			HmNote *note;
			HmNoteData data;
		} updateNote;

		EventNode *setPitch, *setControl, *setPatch;

		struct {
			int channel;
			uint32_t time;
		} clearPitch, clearPatch;

		struct {
			int channel;
			uint32_t time;
			int control;
		} clearControl;

	} data;
} ToAudioMessage;

typedef struct {
	enum {
		FREE_PTR,
		SEQ_MESSAGE
	} type;
	union {
		void *ptr;
		HmSeqMessage message;
	} data;
} FromAudioMessage;

struct HmSeq {
	HmMQ *toAudio;
	HmMQ *fromAudio;

	EventNode *head, *tail;
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

	TRY(mq_init(&seq->toAudio, sizeof(ToAudioMessage), 128));
	TRY(mq_init(&seq->fromAudio, sizeof(FromAudioMessage), 128));

	*result = seq;

	CATCH(
		hm_seq_free(seq);
	)
	FINALLY()
}

static void remove_event(HmSeq *seq, EventNode *event);
static void update_sequence(HmSeq *seq);

void hm_seq_free(HmSeq *seq)
{
	if (seq) {
		update_sequence(seq);
		HmSeqMessage message;
		while (hm_seq_pop_message(seq, &message)) { }

		EventNode *event = seq->head;
		while (event) {
			void *freePtr;

			if (event->event.type == HM_EV_NOTE_ON) {
				HmNote *note = (void *)event - offsetof(HmNote, on);
				remove_event(seq, &note->off);
				freePtr = note;

			} else {
				freePtr = event;
			}

			event = event->next;
			free(freePtr);
		}

		mq_free(seq->toAudio);
		mq_free(seq->fromAudio);
		free(seq);
	}
}

static void insert_event(HmSeq *seq, EventNode *event)
{
	EventNode *next = seq->head;

	if (!next) {
		seq->head = event;
		seq->tail = event;

	} else {
		while (next && next->event.time <= event->event.time) {
			next = next->next;
		}

		if (!next) {
			seq->tail->next = event;
			event->prev = seq->tail;
			seq->tail = event;

		} else if (!next->prev) {
			seq->head = event;
			next->prev = event;
			event->next = next;

		} else {
			next->prev->next = event;
			event->prev = next->prev;
			next->prev = event;
			event->next = next;
		}
	}
}

static void remove_event(HmSeq *seq, EventNode *event)
{
	if (event->prev) {
		event->prev->next = event->next;
	} else {
		seq->head = event->next;
	}

	if (event->next) {
		event->next->prev = event->prev;
	} else {
		seq->tail = event->prev;
	}

	event->prev = NULL;
	event->next = NULL;
}

static EventNode *find_event(EventNode *start, uint32_t time, int channel, HmEventType type)
{
	for (EventNode *event = start; event && event->event.time <= time; event = event->next) {
		if (event->event.time == time && event->event.channel == channel && event->event.type == type)
			return event;
	}

	return NULL;
}

static EventNode *find_control_event(EventNode *start, uint32_t time, int channel, int control)
{
	EventNode *event;
	while ((event = find_event(start, time, channel, HM_EV_CONTROL))) {
		if (event->event.data.control.control == control)
			return event;
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

	mq_push(seq->fromAudio, &message);
}

static void add_note(HmSeq *seq, HmNote *note)
{
	insert_event(seq, &note->on);
	insert_event(seq, &note->off);

	FromAudioMessage message = {
		.type = SEQ_MESSAGE,
		.data = {
			.message = {
				.type = HM_SEQ_NOTE_ADDED,
				.time = note->data.time,
				.channel = note->on.event.channel,
				.data = {
					.note = {
						.note = note,
						.data = note->data
					}
				}
			}
		}
	};
	mq_push(seq->fromAudio, &message);
}

static void remove_note(HmSeq *seq, HmNote *note)
{
	remove_event(seq, &note->on);
	remove_event(seq, &note->off);

	FromAudioMessage message = {
		.type = SEQ_MESSAGE,
		.data = {
			.message = {
				.type = HM_SEQ_NOTE_REMOVED,
				.time = note->data.time,
				.channel = note->on.event.channel,
				.data = {
					.note = {
						.note = note,
						.data = note->data
					}
				}
			}
		}
	};
	mq_push(seq->fromAudio, &message);
}

static void update_note(HmSeq *seq, HmNote *note, HmNoteData *noteData)
{
	if (note->data.time != noteData->time || note->data.length != noteData->length) {
		remove_event(seq, &note->on);
		remove_event(seq, &note->off);
		insert_event(seq, &note->on);
		insert_event(seq, &note->off);
	}

	note->data = *noteData;

	FromAudioMessage message = {
		.type = SEQ_MESSAGE,
		.data = {
			.message = {
				.type = HM_SEQ_NOTE_UPDATED,
				.time = note->data.time,
				.channel = note->on.event.channel,
				.data = {
					.note = {
						.note = note,
						.data = note->data
					}
				}
			}
		}
	};
	mq_push(seq->fromAudio, &message);
}

static void set_pitch(HmSeq *seq, EventNode *event)
{
	EventNode *foundEvent = find_event(seq->head, event->event.time, event->event.channel, HM_EV_PITCH);

	FromAudioMessage message = {
		.type = SEQ_MESSAGE,
		.data = {
			.message = {
				.type = HM_SEQ_PITCH_SET,
				.time = event->event.time,
				.channel = event->event.channel,
				.data = {
					.pitch = event->event.data.pitch
				}
			}
		}
	};
	mq_push(seq->fromAudio, &message);

	if (!foundEvent) {
		insert_event(seq, event);

	} else {
		foundEvent->event.data.pitch = event->event.data.pitch;
		free_from_audio(seq, event);
	}
}

static void clear_pitch(HmSeq *seq, int channel, uint32_t time)
{
	EventNode *event = find_event(seq->head, time, channel, HM_EV_PITCH);

	if (event) {
		FromAudioMessage message = {
			.type = SEQ_MESSAGE,
			.data = {
				.message = {
					.type = HM_SEQ_PITCH_CLEARED,
					.time = event->event.time,
					.channel = event->event.channel,
					.data = {
						.pitch = event->event.data.pitch
					}
				}
			}
		};
		mq_push(seq->fromAudio, &message);

		remove_event(seq, event);
		free_from_audio(seq, event);
	}
}

static void set_control(HmSeq *seq, EventNode *event)
{
	EventNode *foundEvent = find_control_event(foundEvent, event->event.time, event->event.channel, event->event.data.control.control);

	FromAudioMessage message = {
		.type = SEQ_MESSAGE,
		.data = {
			.message = {
				.type = HM_SEQ_CONTROL_SET,
				.time = event->event.time,
				.channel = event->event.channel,
				.data = {
					.control = {
						.control = event->event.data.control.control,
						.value = event->event.data.control.value
					}
				}
			}
		}
	};
	mq_push(seq->fromAudio, &message);

	if (!foundEvent) {
		insert_event(seq, event);

	} else {
		foundEvent->event.data.control.value = event->event.data.control.value;
		free_from_audio(seq, event);
	}
}

static void clear_control(HmSeq *seq, int channel, uint32_t time, int control)
{
	EventNode *event = find_control_event(seq->head, time, channel, control);

	if (event) {
		FromAudioMessage message = {
			.type = SEQ_MESSAGE,
			.data = {
				.message = {
					.type = HM_SEQ_CONTROL_CLEARED,
					.time = event->event.time,
					.channel = event->event.channel,
					.data = {
						.control = {
							.control = event->event.data.control.control,
							.value = event->event.data.control.value
						}
					}
				}
			}
		};
		mq_push(seq->fromAudio, &message);

		remove_event(seq, event);
		free_from_audio(seq, event);
	}
}

static void set_patch(HmSeq *seq, EventNode *event)
{
	EventNode *foundEvent = find_event(seq->head, event->event.time, event->event.channel, HM_EV_PATCH);

	FromAudioMessage message = {
		.type = SEQ_MESSAGE,
		.data = {
			.message = {
				.type = HM_SEQ_PATCH_SET,
				.time = event->event.time,
				.channel = event->event.channel,
				.data = {
					.patch = event->event.data.patch
				}
			}
		}
	};
	mq_push(seq->fromAudio, &message);

	if (!foundEvent) {
		insert_event(seq, event);

	} else {
		foundEvent->event.data.patch = event->event.data.patch;
		free_from_audio(seq, event);
	}
}

static void clear_patch(HmSeq *seq, int channel, uint32_t time)
{
	EventNode *event = find_event(seq->head, time, channel, HM_EV_PATCH);

	if (event) {
		FromAudioMessage message = {
			.type = SEQ_MESSAGE,
			.data = {
				.message = {
					.type = HM_SEQ_PATCH_CLEARED,
					.time = event->event.time,
					.channel = event->event.channel,
					.data = {
						.patch = event->event.data.patch
					}
				}
			}
		};
		mq_push(seq->fromAudio, &message);

		remove_event(seq, event);
		free_from_audio(seq, event);
	}
}

static void update_sequence(HmSeq *seq)
{
	ToAudioMessage message;
	while (mq_pop(seq->toAudio, &message)) {
		switch (message.type) {
			case ADD_NOTE:
				add_note(seq, message.data.addNote);
				break;

			case REMOVE_NOTE:
				remove_note(seq, message.data.removeNote);
				break;

			case UPDATE_NOTE:
				update_note(seq, message.data.updateNote.note, &message.data.updateNote.data);
				break;

			case SET_PITCH:
				set_pitch(seq, message.data.setPitch);
				break;

			case CLEAR_PITCH:
				clear_pitch(seq, message.data.clearPitch.time, message.data.clearPitch.channel);
				break;

			case SET_CONTROL:
				set_control(seq, message.data.setControl);
				break;

			case CLEAR_CONTROL:
				clear_control(seq, message.data.clearControl.channel, message.data.clearControl.time, message.data.clearControl.control);
				break;

			case SET_PATCH:
				set_pitch(seq, message.data.setPatch);
				break;

			case CLEAR_PATCH:
				clear_patch(seq, message.data.clearPatch.time, message.data.clearPatch.channel);
				break;
		}
	}
}

int hm_seq_get_events(HmSeq *seq, HmEvent *events, int numEvents, uint32_t start, uint32_t end)
{
	update_sequence(seq);

	EventNode *event = seq->head;
	while (event && event->event.time < start) {
		event = event->next;
	}

	int numWritten = 0;
	while (event && event->event.time <= end && numWritten < numEvents) {
		*(events++) = event->event;
		numWritten++;

		event = event->next;
	}

	return numWritten;
}

bool hm_seq_pop_message(HmSeq *seq, HmSeqMessage *message)
{
	FromAudioMessage audioMessage;
	while (mq_pop(seq->fromAudio, &audioMessage)) {
		switch (audioMessage.type) {
			case FREE_PTR:
				free(audioMessage.data.ptr);
				break;

			case SEQ_MESSAGE:
				*message = audioMessage.data.message;
				return true;
		}
	}

	return false;
}

AlError hm_seq_add_note(HmSeq *seq, int channel, HmNoteData *data)
{
	BEGIN()

	HmNote *note = NULL;
	TRY(al_malloc(&note, sizeof(HmNote), 1));

	note->data = *data;
	note->on = (EventNode){
		.prev = NULL,
		.next = NULL,
		.event = (HmEvent){
			.time = data->time,
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
			.time = data->time + data->length,
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

	ToAudioMessage message = {
		.type = ADD_NOTE,
		.data = {
			.addNote = note
		}
	};

	if (!mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	CATCH(
		free(note);
	)
	FINALLY()
}

AlError hm_seq_remove_note(HmSeq *seq, HmNote *note)
{
	BEGIN()

	ToAudioMessage message = {
		.type = REMOVE_NOTE,
		.data = {
			.removeNote = note
		}
	};

	if (!mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_seq_update_note(HmSeq *seq, HmNote *note, HmNoteData *data)
{
	BEGIN()

	ToAudioMessage message = {
		.type = UPDATE_NOTE,
		.data = {
			.updateNote = {
				.note = note,
				.data = *data
			}
		}
	};

	if (!mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_seq_set_pitch(HmSeq *seq, int channel, uint32_t time, float pitch)
{
	BEGIN()

	EventNode *event = NULL;
	TRY(al_malloc(&event, sizeof(EventNode), 1));

	event->prev = NULL;
	event->next = NULL;
	event->event = (HmEvent){
		.time = time,
		.channel = channel,
		.type = HM_EV_PITCH,
		.data = {
			.pitch = pitch
		}
	};

	ToAudioMessage message = {
		.type = SET_PITCH,
		.data = {
			.setPitch = event
		}
	};

	if (!mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_seq_clear_pitch(HmSeq *seq, int channel, uint32_t time)
{
	BEGIN()

	ToAudioMessage message = {
		.type = CLEAR_PITCH,
		.data = {
			.clearPitch = {
				.channel = channel,
				.time = time
			}
		}
	};

	if (!mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_seq_set_control(HmSeq *seq, int channel, uint32_t time, int control, float value)
{
	BEGIN()

	EventNode *event = NULL;
	TRY(al_malloc(&event, sizeof(EventNode), 1));

	event->prev = NULL;
	event->next = NULL;
	event->event = (HmEvent){
		.time = time,
		.channel = channel,
		.type = HM_EV_CONTROL,
		.data = {
			.control = {
				.control = control,
				.value = value
			}
		}
	};

	ToAudioMessage message = {
		.type = SET_CONTROL,
		.data = {
			.setPitch = event
		}
	};

	if (!mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_seq_clear_control(HmSeq *seq, int channel, uint32_t time, int control)
{
	BEGIN()

	ToAudioMessage message = {
		.type = CLEAR_CONTROL,
		.data = {
			.clearControl = {
				.channel = channel,
				.time = time,
				.control = control
			}
		}
	};

	if (!mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_seq_set_patch(HmSeq *seq, int channel, uint32_t time, int patch)
{
	BEGIN()

	EventNode *event = NULL;
	TRY(al_malloc(&event, sizeof(EventNode), 1));

	event->prev = NULL;
	event->next = NULL;
	event->event = (HmEvent){
		.time = time,
		.channel = channel,
		.type = HM_EV_PATCH,
		.data = {
			.patch = patch
		}
	};

	ToAudioMessage message = {
		.type = SET_PATCH,
		.data = {
			.setPitch = event
		}
	};

	if (!mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}

AlError hm_seq_clear_patch(HmSeq *seq, int channel, uint32_t time)
{
	BEGIN()

	ToAudioMessage message = {
		.type = CLEAR_PATCH,
		.data = {
			.clearPatch = {
				.channel = channel,
				.time = time
			}
		}
	};

	if (!mq_push(seq->toAudio, &message))
		THROW(AL_ERROR_MEMORY);

	PASS()
}
