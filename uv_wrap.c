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
// E.g., holding a lua function/closure for callbacks from C to lua.
//
typedef struct  {
    lua_State *L;
    int ref;
} lua_ref_t;

static void unref(lua_ref_t *ref) {
    if (ref && ref->L) {
        luaL_unref(ref->L, LUA_REGISTRYINDEX, ref->ref);
        free(ref);
    }
}

static lua_ref_t *ref(lua_State *L) { // Grabs stack's top item.
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

// Grabs a reference to the lua function/closure at the index.
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
    lua_ref_t *ref_buf;
    lua_ref_t *ref_cb;
} write_req_t;

static void wrap_uv_on_write(uv_write_t *req, int status) {
    write_req_t *wr = (write_req_t *) req;
    assert(wr && wr->ref_cb && wr->ref_cb->L);
    lua_rawgeti(wr->ref_cb->L, LUA_REGISTRYINDEX, wr->ref_cb->ref);
    lua_pushnumber(wr->ref_cb->L, status);
    if (lua_pcall(wr->ref_cb->L, 1, 0, 0) != 0) {
        printf("wrap_uv_on_write pcall error: %s\n",
               lua_tostring(wr->ref_cb->L, -1));
    }

    unref(wr->ref_buf);
    unref(wr->ref_cb);
    free(wr);
}

// params: stream, stringToWrite, afterWriteCallback(status)
//
LUA_API int wrap_uv_write(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p = luaL_checkudata(L, 1, "uv_stream_t_ptr");
    stream = *stream_p;

    size_t slen = 0;
    char *s = (char *) luaL_checklstring(L, 2, &slen);
    if (s == NULL || slen <= 0) {
        lua_pushinteger(L, 0);
        return 1;
    }

    write_req_t *wr = malloc(sizeof(*wr));
    if (wr != NULL) {
        lua_pushvalue(L, 2);
        wr->ref_buf = ref(L);
        wr->ref_cb = ref_function(L, 3);
        if (wr->ref_buf && wr->ref_cb) {
            wr->buf = uv_buf_init(s, slen);

            int res = uv_write(&wr->req, stream, &wr->buf, 1,
                               wrap_uv_on_write);
            lua_pushinteger(L, res);
            return 1;
        }

        unref(wr->ref_buf);
        unref(wr->ref_cb);
        free(wr);
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
    if (ref != NULL) {
        lua_rawgeti(ref->L, LUA_REGISTRYINDEX, ref->ref);
        lua_pushnumber(ref->L, nread);
        if (nread > 0) {
            lua_pushlstring(ref->L, buf.base, nread);
        } else {
            lua_pushnil(ref->L);
        }
        if (lua_pcall(ref->L, 2, 0, 0) != 0) {
            printf("wrap_uv_on_read pcall error: %s\n",
                   lua_tostring(ref->L, -1));
        }
    }

    free(buf.base);
}

// params: stream, afterReadCallback(nread, string)
//
LUA_API int wrap_uv_read_start(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p = luaL_checkudata(L, 1, "uv_stream_t_ptr");
    stream = *stream_p;

    stream->data = ref_function(L, 2);

    int res = uv_read_start(stream, wrap_uv_on_alloc, wrap_uv_on_read);
    lua_pushinteger(L, res);
    return 1;
}

static void wrap_uv_on_listen(uv_stream_t *server, int status) {
    assert(server);

    lua_ref_t *ref = server->data;
    if (ref != NULL) {
        lua_rawgeti(ref->L, LUA_REGISTRYINDEX, ref->ref);
        lua_pushnumber(ref->L, status);
        if (lua_pcall(ref->L, 1, 0, 0) != 0) {
            printf("wrap_uv_on_listen pcall error: %s\n",
                   lua_tostring(ref->L, -1));
        }
    }
}

// params: stream, listenBacklogCount, listenCallback(status)
//
LUA_API int wrap_uv_listen(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p = luaL_checkudata(L, 1, "uv_stream_t_ptr");
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

// As part of this stream cleanup function, de-link the reference from
// C to the lua callback closure.  Any fired callbacks after cleanup()
// will become no-op, and lua can also GC appropriately.
//
// params: stream
//
LUA_API int wrap_uv_cleanup(lua_State *L) {
    uv_stream_t *stream;
    uv_stream_t **stream_p = luaL_checkudata(L, 1, "uv_stream_t_ptr");
    stream = *stream_p;

    if (stream->data != NULL) {
        unref(stream->data);
    }
    stream->data = NULL;

    lua_pushinteger(L, 0);
    return 1;
}

static void on_work(uv_work_t *wr) {
    assert(wr);

    lua_ref_t *cb = wr->data;
    assert(cb);

    lua_rawgeti(cb->L, LUA_REGISTRYINDEX, cb->ref);
    if (lua_pcall(cb->L, 0, 0, 0) != 0) {
        printf("wrap_uv_on_write pcall error: %s\n",
               lua_tostring(cb->L, -1));
    }
}

static void on_work_after(uv_work_t *wr) {
    assert(wr);

    lua_ref_t *cb = wr->data;
    assert(cb);

    unref(cb);
    free(wr);
}

// The wrapped lua API for uv_queue_work() is simpler than
// the C API because lua has closures to handle user data.
//
// params: loop, callback()
//
LUA_API int wrap_uv_queue_work(lua_State *L) {
    uv_loop_t *loop;
    uv_loop_t **loop_p = luaL_checkudata(L, 1, "uv_loop_t_ptr");
    loop = *loop_p;

    uv_work_t *wr = calloc(1, sizeof(*wr));
    if (wr != NULL) {
        wr->data = ref_function(L, 2);

        int res = uv_queue_work(loop, wr, on_work, on_work_after);
        lua_pushinteger(L, res);
        return 1;
    }

    luaL_error(L, "wrap_uv_queue_work malloc failed");
    return 0;
}
