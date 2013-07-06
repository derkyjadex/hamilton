/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include "band_cmds.h"
#include "hamilton/band.h"

int cmd_get_synths(lua_State *L)
{
	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmLib *lib = hm_band_get_lib(band);
	int n;
	const HmSynthType *synths = hm_lib_get_synths(lib, &n);

	lua_newtable(L);
	for (int i = 0; i < n; i++) {
		lua_pushinteger(L, i + 1);
		lua_pushstring(L, synths[i].name);
		lua_settable(L, -3);
	}

	return 1;
}

int cmd_set_synth(lua_State *L)
{
	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	HmLib *lib = hm_band_get_lib(band);
	int channel =luaL_checkint(L, 1) - 1;
	const char *name = luaL_checkstring(L, 2);

	const HmSynthType *synth = hm_lib_get_synth(lib, name);
	if (!synth)
		return luaL_error(L, "no such synth: %s", name);

	int error = hm_band_set_channel_synth(band, channel, synth);
	if (error)
		return luaL_error(L, "error setting synth: %d", error);

	return 0;
}

int cmd_play(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	TRY(hm_band_play(band));

	CATCH_LUA(, "error starting band")
	FINALLY_LUA(, 0)
}

int cmd_pause(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	TRY(hm_band_pause(band));

	CATCH_LUA(, "error pausing band")
	FINALLY_LUA(, 0)
}

int cmd_seek(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));

	lua_Number position = luaL_checknumber(L, 1);

	TRY(hm_band_seek(band, position));

	CATCH_LUA(, "error pausing band")
	FINALLY_LUA(, 0)
}

int cmd_set_looping(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));

	luaL_checkany(L, 1);
	bool looping = lua_toboolean(L, 1);

	TRY(hm_band_set_looping(band, looping));

	CATCH_LUA(, "error setting looping")
	FINALLY_LUA(, 0)
}

int cmd_set_loop(lua_State *L)
{
	BEGIN()

	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));

	lua_Number start = luaL_checknumber(L, 1);
	lua_Number end = luaL_checknumber(L, 2);

	TRY(hm_band_set_loop(band, start, end));

	CATCH_LUA(, "error pausing band")
	FINALLY_LUA(, 0)
}

int cmd_send_cc(lua_State *L)
{
	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));
	int channel = (int)luaL_checkinteger(L, 1);
	int control = (int)luaL_checkinteger(L, 2);
	float value = luaL_checknumber(L, 3);

	hm_band_send_cc(band, channel, control, value);

	return 0;
}

int cmd_get_band_state(lua_State *L)
{
	HmBand *band = lua_touserdata(L, lua_upvalueindex(1));

	HmBandState state;
	hm_band_get_state(band, &state);

	lua_newtable(L);

	lua_pushliteral(L, "playing");
	lua_pushboolean(L, state.playing);
	lua_settable(L, -3);

	lua_pushliteral(L, "position");
	lua_pushinteger(L, state.position);
	lua_settable(L, -3);

	lua_pushliteral(L, "looping");
	lua_pushboolean(L, state.looping);
	lua_settable(L, -3);

	lua_pushliteral(L, "loopStart");
	lua_pushnumber(L, state.loopStart);
	lua_settable(L, -3);

	lua_pushliteral(L, "loopEnd");
	lua_pushnumber(L, state.loopEnd);
	lua_settable(L, -3);

	return 1;
}
