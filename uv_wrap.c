/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "uv.h"

typedef struct lua_ref {
    lua_State *L;
    int ref;
} lua_ref;

void wrap_uv_on_connection(uv_stream_t* server, int status) {
    printf("hello from conn\n");
}

LUA_API int wrap_uv_listen(lua_State *L) {
    uv_stream_t * stream;
    uv_stream_t * *stream_p =
        luaL_checkudata(L, 1, "uv_wrap.uv_stream_t_ptr");
    stream = *stream_p;

    int backlog;
    backlog = (int) luaL_checkint(L, 2);

    luaL_checktype(L, 3, LUA_TFUNCTION);

    lua_ref *ref = malloc(sizeof(lua_ref));
    if (ref == NULL) {
        luaL_error(L, "malloc failed");
    }
    ref->L = L;
    ref->ref = luaL_ref(L, -1);
    if (ref->ref == LUA_NOREF ||
        ref->ref == LUA_REFNIL) {
        free(ref);
        luaL_error(L, "luaL_ref failed");
    }

    assert(stream->data == NULL);
    stream->data = ref;

    int res = (int)
        uv_listen(stream, backlog, wrap_uv_on_connection);
    lua_pushinteger(L, res);
    return 1;
}

LUA_API int test_helper_func(lua_State *L) {
   double trouble = lua_tonumber(L, 1);
   lua_pushnumber(L, 16.0 - trouble);
   return 1;
}

