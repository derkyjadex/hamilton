/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "seq_cmds.h"
#include "hamilton/band.h"

int cmd_add_note(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1);

	uint32_t time = (int)luaL_checkinteger(L, 2);

	HmNoteData note = {
		.length = (int)luaL_checkinteger(L, 3),
		.num = (int)luaL_checkinteger(L, 4),
		.velocity = luaL_checknumber(L, 5)
	};

	TRY(hm_seq_add_note(seq, channel, time, &note));

	CATCH_LUA(, "error adding note")
	FINALLY_LUA(, 0)
}

int cmd_remove_note(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	HmNote *note = lua_touserdata(L, 1);

	TRY(hm_seq_remove_note(seq, note));

	CATCH_LUA(, "error removing note")
	FINALLY_LUA(, 0)
}

int cmd_update_note(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	HmNote *note = lua_touserdata(L, 1);

	uint32_t time = (int)luaL_checkinteger(L, 2);
	HmNoteData data = {
		.length = (int)luaL_checkinteger(L, 3),
		.num = (int)luaL_checkinteger(L, 4),
		.velocity = luaL_checknumber(L, 5)
	};

	TRY(hm_seq_update_note(seq, note, time, &data));

	CATCH_LUA(, "error updating note")
	FINALLY_LUA(, 0)
}

int cmd_add_set_pitch(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1);
	int time = (int)luaL_checkinteger(L, 2);
	float pitch = luaL_checknumber(L, 3);

	TRY(hm_seq_set_pitch(seq, channel, time, pitch));

	CATCH_LUA(, "error setting pitch")
	FINALLY_LUA(, 0)
}

int cmd_clear_set_pitch(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1);
	int time = (int)luaL_checkinteger(L, 2);

	TRY(hm_seq_clear_pitch(seq, channel, time));

	CATCH_LUA(, "error clearing pitch")
	FINALLY_LUA(, 0)
}

int cmd_add_set_control(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1);
	int time = (int)luaL_checkinteger(L, 2);
	int control = (int)luaL_checkinteger(L, 3);
	float value = luaL_checknumber(L, 4);

	TRY(hm_seq_set_control(seq, channel, time, control, value));

	CATCH_LUA(, "error setting control")
	FINALLY_LUA(, 0)
}

int cmd_clear_set_control(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1);
	int time = (int)luaL_checkinteger(L, 2);
	int control = (int)luaL_checkinteger(L, 3);

	TRY(hm_seq_clear_control(seq, channel, time, control));

	CATCH_LUA(, "error clearing control")
	FINALLY_LUA(, 0)
}

int cmd_add_set_patch(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1);
	int time = (int)luaL_checkinteger(L, 2);
	int patch = (int)luaL_checkinteger(L, 3);

	TRY(hm_seq_set_patch(seq, channel, time, patch));

	CATCH_LUA(, "error setting patch")
	FINALLY_LUA(, 0)
}

int cmd_clear_set_patch(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1);
	int time = (int)luaL_checkinteger(L, 2);

	TRY(hm_seq_clear_patch(seq, channel, time));

	CATCH_LUA(, "error clearing patch")
	FINALLY_LUA(, 0)
}

static void push_type(lua_State *L, HmSeqMessage *message)
{
	switch (message->type) {
		case HM_SEQ_NOTE_ADDED:
			lua_pushliteral(L, "note_added");
			break;
		case HM_SEQ_NOTE_REMOVED:
			lua_pushliteral(L, "note_removed");
			break;
		case HM_SEQ_NOTE_UPDATED:
			lua_pushliteral(L, "note_updated");
			break;
		case HM_SEQ_PITCH_SET:
			lua_pushliteral(L, "pitch_set");
			break;
		case HM_SEQ_PITCH_CLEARED:
			lua_pushliteral(L, "pitch_cleared");
			break;
		case HM_SEQ_CONTROL_SET:
			lua_pushliteral(L, "control_set");
			break;
		case HM_SEQ_CONTROL_CLEARED:
			lua_pushliteral(L, "control_cleared");
			break;
		case HM_SEQ_PARAM_SET:
			lua_pushliteral(L, "param_set");
			break;
		case HM_SEQ_PARAM_CLEARED:
			lua_pushliteral(L, "param_cleared");
			break;
		case HM_SEQ_PATCH_SET:
			lua_pushliteral(L, "patch_set");
			break;
		case HM_SEQ_PATCH_CLEARED:
			lua_pushliteral(L, "patch_cleared");
			break;
	}
}

int cmd_get_seq_messages(lua_State *L)
{
	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	lua_newtable(L);
	int n = 1;

	HmSeqMessage message;
	while (hm_seq_pop_message(seq, &message)) {
		lua_pushinteger(L, n++);
		lua_newtable(L);

		lua_pushliteral(L, "type");
		push_type(L, &message);
		lua_settable(L, -3);

		lua_pushliteral(L, "time");
		lua_pushinteger(L, message.time);
		lua_settable(L, -3);

		lua_pushliteral(L, "channel");
		lua_pushinteger(L, message.channel);
		lua_settable(L, -3);

		switch (message.type) {
			case HM_SEQ_NOTE_ADDED:
			case HM_SEQ_NOTE_REMOVED:
			case HM_SEQ_NOTE_UPDATED:
				lua_pushliteral(L, "note");
				lua_pushlightuserdata(L, message.data.note.note);
				lua_settable(L, -3);

				lua_pushliteral(L, "length");
				lua_pushinteger(L, message.data.note.data.length);
				lua_settable(L, -3);

				lua_pushliteral(L, "num");
				lua_pushinteger(L, message.data.note.data.num);
				lua_settable(L, -3);

				lua_pushliteral(L, "velocity");
				lua_pushnumber(L, message.data.note.data.velocity);
				lua_settable(L, -3);
				break;

			case HM_SEQ_PITCH_SET:
			case HM_SEQ_PITCH_CLEARED:
				lua_pushliteral(L, "pitch");
				lua_pushnumber(L, message.data.pitch);
				lua_settable(L, -3);

			case HM_SEQ_CONTROL_SET:
			case HM_SEQ_CONTROL_CLEARED:
				lua_pushliteral(L, "control");
				lua_pushinteger(L, message.data.control.num);
				lua_settable(L, -3);

				lua_pushliteral(L, "value");
				lua_pushnumber(L, message.data.control.value);
				lua_settable(L, -3);
				break;

			case HM_SEQ_PARAM_SET:
			case HM_SEQ_PARAM_CLEARED:
				lua_pushliteral(L, "param");
				lua_pushinteger(L, message.data.param.num);
				lua_settable(L, -3);

				lua_pushliteral(L, "value");
				lua_pushnumber(L, message.data.param.value);
				lua_settable(L, -3);
				break;

			case HM_SEQ_PATCH_SET:
			case HM_SEQ_PATCH_CLEARED:
				lua_pushliteral(L, "patch");
				lua_pushinteger(L, message.data.patch);
				lua_settable(L, -3);
				break;
		}

		lua_settable(L, -3);
	}

	return 1;
}
