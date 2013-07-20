/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "seq_cmds.h"
#include "hamilton/band.h"

int cmd_add_note(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1) - 1;

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

	int channel = (int)luaL_checkinteger(L, 1) - 1;
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

	int channel = (int)luaL_checkinteger(L, 1) - 1;
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

	int channel = (int)luaL_checkinteger(L, 1) - 1;
	int time = (int)luaL_checkinteger(L, 2);
	int control = (int)luaL_checkinteger(L, 3) - 1;
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

	int channel = (int)luaL_checkinteger(L, 1) - 1;
	int time = (int)luaL_checkinteger(L, 2);
	int control = (int)luaL_checkinteger(L, 3) - 1;

	TRY(hm_seq_clear_control(seq, channel, time, control));

	CATCH_LUA(, "error clearing control")
	FINALLY_LUA(, 0)
}

int cmd_add_set_param(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1) - 1;
	int time = (int)luaL_checkinteger(L, 2);
	int param = (int)luaL_checkinteger(L, 3) - 1;
	float value = luaL_checknumber(L, 4);

	TRY(hm_seq_set_param(seq, channel, time, param, value));

	CATCH_LUA(, "error setting param")
	FINALLY_LUA(, 0)
}

int cmd_clear_set_param(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1) - 1;
	int time = (int)luaL_checkinteger(L, 2);
	int param = (int)luaL_checkinteger(L, 3) - 1;

	TRY(hm_seq_clear_param(seq, channel, time, param));

	CATCH_LUA(, "error clearing param")
	FINALLY_LUA(, 0)
}

int cmd_add_set_patch(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1) - 1;
	int time = (int)luaL_checkinteger(L, 2);
	int patch = (int)luaL_checkinteger(L, 3) - 1;

	TRY(hm_seq_set_patch(seq, channel, time, patch));

	CATCH_LUA(, "error setting patch")
	FINALLY_LUA(, 0)
}

int cmd_clear_set_patch(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	int channel = (int)luaL_checkinteger(L, 1) - 1;
	int time = (int)luaL_checkinteger(L, 2);

	TRY(hm_seq_clear_patch(seq, channel, time));

	CATCH_LUA(, "error clearing patch")
	FINALLY_LUA(, 0)
}

int cmd_get_seq_items(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	lua_newtable(L);
	int n = 1;

	HmSeqItem *items = NULL;
	int numItems;

	TRY(hm_seq_get_items(seq, &items, &numItems));

	for (int i = 0; i < numItems; i++) {
		HmSeqItem *item = &items[i];

		lua_pushinteger(L, n++);
		lua_newtable(L);

		lua_pushliteral(L, "time");
		lua_pushinteger(L, item->time);
		lua_settable(L, -3);

		lua_pushliteral(L, "channel");
		lua_pushinteger(L, item->channel + 1);
		lua_settable(L, -3);

		lua_pushliteral(L, "type");

		switch (item->type) {
			case HM_SEQ_NOTE:
				lua_pushliteral(L, "note");
				lua_settable(L, -3);

				lua_pushliteral(L, "note");
				lua_pushlightuserdata(L, item->data.note.note);
				lua_settable(L, -3);

				lua_pushliteral(L, "length");
				lua_pushinteger(L, item->data.note.data.length);
				lua_settable(L, -3);

				lua_pushliteral(L, "num");
				lua_pushinteger(L, item->data.note.data.num);
				lua_settable(L, -3);

				lua_pushliteral(L, "velocity");
				lua_pushnumber(L, item->data.note.data.velocity);
				lua_settable(L, -3);
				break;

			case HM_SEQ_PITCH:
				lua_pushliteral(L, "pitch");
				lua_settable(L, -3);

				lua_pushliteral(L, "pitch");
				lua_pushnumber(L, item->data.pitch);
				lua_settable(L, -3);

			case HM_SEQ_CONTROL:
				lua_pushliteral(L, "control");
				lua_pushinteger(L, item->data.control.num + 1);
				lua_settable(L, -3);

				lua_pushliteral(L, "value");
				lua_pushnumber(L, item->data.control.value);
				lua_settable(L, -3);
				break;

			case HM_SEQ_PARAM:
				lua_pushliteral(L, "param");
				lua_settable(L, -3);

				lua_pushliteral(L, "param");
				lua_pushinteger(L, item->data.param.num + 1);
				lua_settable(L, -3);

				lua_pushliteral(L, "value");
				lua_pushnumber(L, item->data.param.value);
				lua_settable(L, -3);
				break;

			case HM_SEQ_PATCH:
				lua_pushliteral(L, "patch");
				lua_settable(L, -3);

				lua_pushliteral(L, "patch");
				lua_pushinteger(L, item->data.patch + 1);
				lua_settable(L, -3);
				break;
		}

		lua_settable(L, -3);
	}

	CATCH_LUA(, "error getting seq items");
	FINALLY_LUA(
		free(items);,
		1)
}

int cmd_seq_commit(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmSeq *seq = hm_band_get_seq(band);

	TRY(hm_seq_commit(seq));

	CATCH_LUA(, "error commiting seq changes")
	FINALLY_LUA(, 0)
}
