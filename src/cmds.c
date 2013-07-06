/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "hamilton/cmds.h"
#include "hamilton/lib.h"
#include "hamilton/band.h"

#include "band_cmds.h"
#include "seq_cmds.h"

AlLuaKey bandKey;

static const luaL_Reg lib[] = {
	{"get_synths", cmd_get_synths},
	{"set_synth", cmd_set_synth},

	{"play", cmd_play},
	{"pause", cmd_pause},
	{"seek", cmd_seek},
	{"set_looping", cmd_set_looping},
	{"set_loop", cmd_set_loop},

	{"send_cc", cmd_send_cc},
	{"get_band_state", cmd_get_band_state},

	{"add_note", cmd_add_note},
	{"remove_note", cmd_remove_note},
	{"update_note", cmd_update_note},
	{"add_set_pitch", cmd_add_set_pitch},
	{"clear_set_pitch", cmd_clear_set_pitch},
	{"add_set_control", cmd_add_set_control},
	{"clear_set_control", cmd_clear_set_control},
	{"add_set_param", cmd_add_set_param},
	{"clear_set_param", cmd_clear_set_param},
	{"add_set_patch", cmd_add_set_patch},
	{"clear_set_patch", cmd_clear_set_patch},

	{"get_seq_messages", cmd_get_seq_messages},
	{NULL, NULL}
};

int luaopen_hamilton(lua_State *L)
{
	luaL_newlibtable(L, lib);
	lua_pushlightuserdata(L, &bandKey);
	lua_gettable(L, LUA_REGISTRYINDEX);
	luaL_setfuncs(L, lib, 1);

	return 1;
}

void hm_load_cmds(lua_State *L, HmBand *band)
{
	lua_pushlightuserdata(L, &bandKey);
	lua_pushlightuserdata(L, band);
	lua_settable(L, LUA_REGISTRYINDEX);
	luaL_requiref(L, "hamilton", luaopen_hamilton, false);
}
