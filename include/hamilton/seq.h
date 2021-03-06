/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_SEQ_H
#define _HAMILTON_SEQ_H

#include <stdint.h>
#include <stdbool.h>

#include "albase/common.h"

static const float HM_SEQ_TICK_RATE = 1000.0f;

typedef struct HmSeq HmSeq;

typedef enum {
	HM_EV_NOTE_ON,
	HM_EV_NOTE_OFF,
	HM_EV_PITCH,
	HM_EV_CONTROL,
	HM_EV_PARAM,
	HM_EV_PATCH
} HmEventType;

typedef struct {
	uint32_t time;
	int channel;
	HmEventType type;
	union {
		struct {
			int num;
			float velocity;
		} note;
		float pitch;
		struct {
			int num;
			float value;
		} control, param;
		int patch;
	} data;
} HmEvent;

typedef struct HmNote HmNote;

typedef struct {
	uint32_t length;
	uint32_t num;
	float velocity;
} HmNoteData;

typedef struct {
	enum {
		HM_SEQ_NOTE,
		HM_SEQ_PITCH,
		HM_SEQ_CONTROL,
		HM_SEQ_PARAM,
		HM_SEQ_PATCH
	} type;
	uint32_t time;
	int channel;
	union {
		struct {
			HmNote *note;
			HmNoteData data;
		} note;
		float pitch;
		struct {
			int num;
			float value;
		} control, param;
		int patch;
	} data;
} HmSeqItem;

AlError hm_seq_init(HmSeq **seq);
void hm_seq_free(HmSeq *seq);

int hm_seq_get_events(HmSeq *seq, HmEvent *events, int numEvents, uint64_t start, uint64_t end, double sampleRate);

void hm_seq_process_messages(HmSeq *seq);

AlError hm_seq_get_items(HmSeq *seq, HmSeqItem **items, int *numItems);

AlError hm_seq_add_note(HmSeq *seq, int channel, uint32_t time, HmNoteData *data);
AlError hm_seq_remove_note(HmSeq *seq, HmNote *note);
AlError hm_seq_update_note(HmSeq *seq, HmNote *note, uint32_t time, HmNoteData *data);

AlError hm_seq_set_pitch(HmSeq *seq, int channel, uint32_t time, float pitch);
AlError hm_seq_clear_pitch(HmSeq *seq, int channel, uint32_t time);

AlError hm_seq_set_control(HmSeq *seq, int channel, uint32_t time, int control, float value);
AlError hm_seq_clear_control(HmSeq *seq, int channel, uint32_t time, int control);

AlError hm_seq_set_param(HmSeq *seq, int channel, uint32_t time, int param, float value);
AlError hm_seq_clear_param(HmSeq *seq, int channel, uint32_t time, int param);

AlError hm_seq_set_patch(HmSeq *seq, int channel, uint32_t time, int patch);
AlError hm_seq_clear_patch(HmSeq *seq, int channel, uint32_t time);

AlError hm_seq_commit(HmSeq *seq);

#endif
