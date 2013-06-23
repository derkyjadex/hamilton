/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "hamilton/cmds.h"
#include "hamilton/lib.h"
#include "hamilton/band.h"

#include "band_cmds.h"

AlLuaKey bandKey;

static const luaL_Reg lib[] = {
	{"get_synths", cmd_get_synths},
	{"set_synth", cmd_set_synth},
	{"send_cc", cmd_send_cc},
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
