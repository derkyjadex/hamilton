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
	int time = (int)luaL_checkinteger(L, 2);
	int length = (int)luaL_checkinteger(L, 3);
	int num = (int)luaL_checkinteger(L, 4);
	float velocity = luaL_checknumber(L, 5);

	HmNoteData note = {
		.time = time,
		.length = length,
		.num = num,
		.velocity = velocity
	};

	TRY(hm_seq_add_note(seq, channel, &note));

	CATCH_LUA(, "Error adding note")
	FINALLY_LUA(, 0)
}
