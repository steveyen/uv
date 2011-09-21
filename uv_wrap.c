/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "uv.h"

// Holds a reference to a lua object.  For example, a lua function
// for callbacks from C to lua.
//
typedef struct  {
    lua_State *L;
    int ref;
} lua_ref_t;

typedef struct {
    uv_write_t req;
    lua_ref_t *cb;
} write_req_t;

// Hold a reference to a function.
//
static lua_ref_t *ref_function(lua_State *L, int index) {
    luaL_checktype(L, index, LUA_TFUNCTION);
    lua_pushvalue(L, index);

    lua_ref_t *ref = malloc(sizeof(*ref));
    if (ref != NULL) {
        ref->L = L;
        ref->ref = luaL_ref(L, LUA_REGISTRYINDEX);
        if (ref->ref != LUA_NOREF &&
            ref->ref != LUA_REFNIL) {
            return ref;
        }

        free(ref);
        luaL_error(L, "luaL_ref failed");
    } else {
        luaL_error(L, "ref malloc failed");
    }

    return NULL;
}

LUA_API int wrap_uv_write(lua_State *L) {
    /*
    uv_write_t * req;
    uv_write_t * *req_p =
        luaL_checkudata(L, 1, "uv_wrap.uv_write_t_ptr");
    req = *req_p;
    printf("  wrap_uv_write.req: %p\n", req);

    uv_stream_t * handle;
    uv_stream_t * *handle_p =
        luaL_checkudata(L, 2, "uv_wrap.uv_stream_t_ptr");
    handle = *handle_p;
    printf("  wrap_uv_write.handle: %p\n", handle);

    uv_buf_t bufs[];
    uv_buf_t *bufs[]_p =
        luaL_checkudata(L, 3, "uv_wrap.uv_buf_t");
    bufs[] = *bufs[]_p;

    int bufcnt;
    bufcnt = (int) luaL_checkint(L, 4);

    uv_write_cb cb;
    uv_write_cb *cb_p =
        luaL_checkudata(L, 5, "uv_wrap.uv_write_cb");
    cb = *cb_p;

    int res = (int)
        uv_write(req, handle, bufs[], bufcnt, cb);
    lua_pushinteger(L, res);
    */
    return 1;
}

static uv_buf_t wrap_uv_on_alloc(uv_handle_t *handle,
                                 size_t suggested_size) {
    return uv_buf_init(malloc(suggested_size),
                       suggested_size);
}

static void wrap_uv_on_read(uv_stream_t *stream,
                            ssize_t nread,
                            uv_buf_t buf) {
    assert(stream);

    lua_ref_t *ref = stream->data;
    lua_rawgeti(ref->L, LUA_REGISTRYINDEX, ref->ref);

    lua_pushnumber(ref->L, nread);

    if (nread > 0) {
        lua_pushlstring(ref->L, buf.base, nread);
    } else {
        lua_pushnil(ref->L);
    }

    if (lua_pcall(ref->L, 2, 1, 0) != 0) {
        printf("wrap_uv_on_read pcall error: %s\n",
               lua_tostring(ref->L, -1));
    }

    free(buf.base);
}

LUA_API int wrap_uv_read_start(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p =
        luaL_checkudata(L, 1, "uv_wrap.uv_stream_t_ptr");
    stream = *stream_p;

    luaL_argcheck(L, stream->data == NULL, 1,
                  "stream->data is not NULL");

    stream->data = ref_function(L, 2);

    int res = (int)
        uv_read_start(stream, wrap_uv_on_alloc, wrap_uv_on_read);
    lua_pushinteger(L, res);
    return 1;
}

static void wrap_uv_on_listen(uv_stream_t *server, int status) {
    assert(server);

    lua_ref_t *ref = server->data;
    lua_rawgeti(ref->L, LUA_REGISTRYINDEX, ref->ref);

    lua_pushnumber(ref->L, status);

    if (lua_pcall(ref->L, 1, 1, 0) != 0) {
        printf("wrap_uv_on_listen pcall error: %s\n",
               lua_tostring(ref->L, -1));
    }
}

LUA_API int wrap_uv_listen(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p =
        luaL_checkudata(L, 1, "uv_wrap.uv_stream_t_ptr");
    stream = *stream_p;

    luaL_argcheck(L, stream->data == NULL, 1,
                  "stream->data is not NULL");

    int backlog = (int) luaL_checkint(L, 2);

    stream->data = ref_function(L, 3);

    int res = (int) uv_listen(stream, backlog, wrap_uv_on_listen);
    lua_pushinteger(L, res);

    return 1;
}

