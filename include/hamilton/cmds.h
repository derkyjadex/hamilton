/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_CMDS_H
#define _HAMILTON_CMDS_H

#include "albase/lua.h"
#include "hamilton/band.h"

extern AlLuaKey bandKey;
int luaopen_hamilton(lua_State *L);
void hm_load_cmds(lua_State *L, HmBand *band);

#endif
