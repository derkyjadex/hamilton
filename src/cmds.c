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
	{"send_cc", cmd_send_cc},
	{"get_band_state", cmd_get_band_state},
	{"add_note", cmd_add_note},
	{"add_set_patch", cmd_add_set_patch},
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
