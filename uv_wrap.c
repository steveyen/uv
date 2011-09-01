/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

LUA_API int luaopen_uv_wrap(lua_State *L) {
  return 0;
}

LUA_API int foo(lua_State *L) {
       int n = lua_gettop(L);    /* number of arguments */
       lua_Number sum = 0;
       int i;
       for (i = 1; i <= n; i++) {
         if (!lua_isnumber(L, i)) {
           lua_pushstring(L, "incorrect argument");
           lua_error(L);
         }
         sum += lua_tonumber(L, i);
       }
       lua_pushnumber(L, sum/n);        /* first result */
       lua_pushnumber(L, sum);         /* second result */
       return 2;                   /* number of results */
}
