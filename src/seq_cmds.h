/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#ifndef _HAMILTON_SEQ_CMDS_H
#define _HAMILTON_SEQ_CMDS_H

#include "albase/lua.h"

int cmd_add_note(lua_State *L);
int cmd_add_set_patch(lua_State *L);

#endif
