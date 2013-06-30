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
	uint32_t time;
	uint32_t length;
	uint32_t num;
	float velocity;
} HmNoteData;

typedef struct {
	enum {
		HM_SEQ_NOTE_ADDED,
		HM_SEQ_NOTE_REMOVED,
		HM_SEQ_NOTE_UPDATED,
		HM_SEQ_PITCH_SET,
		HM_SEQ_PITCH_CLEARED,
		HM_SEQ_CONTROL_SET,
		HM_SEQ_CONTROL_CLEARED,
		HM_SEQ_PARAM_SET,
		HM_SEQ_PARAM_CLEARED,
		HM_SEQ_PATCH_SET,
		HM_SEQ_PATCH_CLEARED
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
} HmSeqMessage;

AlError hm_seq_init(HmSeq **seq);
void hm_seq_free(HmSeq *seq);

int hm_seq_get_events(HmSeq *seq, HmEvent *events, int numEvents, uint32_t start, uint32_t end);

bool hm_seq_pop_message(HmSeq *seq, HmSeqMessage *message);

AlError hm_seq_add_note(HmSeq *seq, int channel, HmNoteData *data);
AlError hm_seq_remove_note(HmSeq *seq, HmNote *note);
AlError hm_seq_update_note(HmSeq *seq, HmNote *note, HmNoteData *data);

AlError hm_seq_set_pitch(HmSeq *seq, int channel, uint32_t time, float pitch);
AlError hm_seq_clear_pitch(HmSeq *seq, int channel, uint32_t time);

AlError hm_seq_set_control(HmSeq *seq, int channel, uint32_t time, int control, float value);
AlError hm_seq_clear_control(HmSeq *seq, int channel, uint32_t time, int control);

AlError hm_seq_set_param(HmSeq *seq, int channel, uint32_t time, int control, float value);
AlError hm_seq_clear_param(HmSeq *seq, int channel, uint32_t time, int control);

AlError hm_seq_set_patch(HmSeq *seq, int channel, uint32_t time, int patch);
AlError hm_seq_clear_patch(HmSeq *seq, int channel, uint32_t time);

#endif
