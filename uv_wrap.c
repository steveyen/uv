/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "uv.h"

// Holds a reference to a lua object in the LUA_REGISTRYINDEX.
// E.g., holding a lua function for callbacks from C to lua.
//
typedef struct  {
    lua_State *L;
    int ref;
} lua_ref_t;

static void unref(lua_ref_t *ref) {
    assert(ref && ref->L);
    luaL_unref(ref->L, LUA_REGISTRYINDEX, ref->ref);
    free(ref);
}

static lua_ref_t *ref(lua_State *L) { // Grabs stack's top item.
    lua_ref_t *ref = calloc(1, sizeof(*ref));
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
        luaL_error(L, "ref_function malloc failed");
    }

    return NULL;
}

// Grabs a reference to the lua function at the index.
//
static lua_ref_t *ref_function(lua_State *L, int index) {
    luaL_checktype(L, index, LUA_TFUNCTION);
    lua_pushvalue(L, index);
    return ref(L);
}

// -------------------------------------------------------

typedef struct {
    uv_write_t req;
    uv_buf_t   buf;
    lua_ref_t *cb;
} write_req_t;

static void wrap_uv_on_write(uv_write_t *req, int status) {
    write_req_t *wr = (write_req_t *) req;
    assert(wr && wr->cb && wr->cb->L);
    lua_rawgeti(wr->cb->L, LUA_REGISTRYINDEX, wr->cb->ref);

    lua_pushnumber(wr->cb->L, status);

    if (lua_pcall(wr->cb->L, 1, 1, 0) != 0) {
        printf("wrap_uv_on_write pcall error: %s\n",
               lua_tostring(wr->cb->L, -1));
    }

    unref(wr->cb);
    free(wr->buf.base);
    free(wr);
}

// params: stream, string, callback(status)
//
LUA_API int wrap_uv_write(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p =
        luaL_checkudata(L, 1, "uv_wrap.uv_stream_t_ptr");
    stream = *stream_p;

    size_t slen = 0;
    const char *s = luaL_checklstring(L, 2, &slen);
    if (s == NULL || slen <= 0) {
        lua_pushinteger(L, 0);
        return 1;
    }

    char *sb = malloc(slen + 1);
    if (sb != NULL) {
        memcpy(sb, s, slen);
        sb[slen] = '\0';

        write_req_t *wr = calloc(1, sizeof(*wr));
        if (wr != NULL) {
            wr->buf = uv_buf_init(sb, slen);
            wr->cb = ref_function(L, 3);

            int res = uv_write(&wr->req, stream, &wr->buf, 1,
                               wrap_uv_on_write);
            lua_pushinteger(L, res);
            return 1;
        }

        free(sb);
    }

    luaL_error(L, "wrap_uv_write malloc failed");
    return 0;
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
    assert(ref);
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

// params: stream, callback(nread, string)
//
LUA_API int wrap_uv_read_start(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p =
        luaL_checkudata(L, 1, "uv_wrap.uv_stream_t_ptr");
    stream = *stream_p;

    if (stream->data != NULL) {
        unref(stream->data);
    }
    stream->data = ref_function(L, 2);

    int res = uv_read_start(stream, wrap_uv_on_alloc, wrap_uv_on_read);
    lua_pushinteger(L, res);
    return 1;
}

static void wrap_uv_on_listen(uv_stream_t *server, int status) {
    assert(server);
    lua_ref_t *ref = server->data;
    assert(ref);
    lua_rawgeti(ref->L, LUA_REGISTRYINDEX, ref->ref);

    lua_pushnumber(ref->L, status);

    if (lua_pcall(ref->L, 1, 1, 0) != 0) {
        printf("wrap_uv_on_listen pcall error: %s\n",
               lua_tostring(ref->L, -1));
    }
}

// params: stream, backlog, callback(status)
//
LUA_API int wrap_uv_listen(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p =
        luaL_checkudata(L, 1, "uv_wrap.uv_stream_t_ptr");
    stream = *stream_p;

    int backlog = (int) luaL_checkint(L, 2);

    if (stream->data != NULL) {
        unref(stream->data);
    }
    stream->data = ref_function(L, 3);

    int res = uv_listen(stream, backlog, wrap_uv_on_listen);
    lua_pushinteger(L, res);
    return 1;
}

// params: stream
//
LUA_API int wrap_uv_cleanup(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p =
        luaL_checkudata(L, 1, "uv_wrap.uv_stream_t_ptr");
    stream = *stream_p;

    if (stream->data != NULL) {
        unref(stream->data);
    }
    stream->data = NULL;

    lua_pushinteger(L, 0);
    return 1;
}

