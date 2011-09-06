/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

LUA_API int test_helper_func(lua_State *L) {
   double trouble = lua_tonumber(L, 1);
   lua_pushnumber(L, 16.0 - trouble);
   return 1;
}
