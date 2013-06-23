/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_BAND_CMDS_H
#define _HAMILTON_BAND_CMDS_H

#include "albase/lua.h"

int cmd_get_synths(lua_State *L);
int cmd_set_synth(lua_State *L);
int cmd_send_cc(lua_State *L);

#endif
