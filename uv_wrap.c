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

static void wrap_uv_on_connection(uv_stream_t* server, int status) {
    printf("hello from conn\n");

    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, client);

    uv_accept(server, (uv_stream_t *) client);
}

LUA_API int wrap_uv_listen(lua_State *L) {
    uv_stream_t * stream;
    uv_stream_t * *stream_p =
        luaL_checkudata(L, 1, "uv_wrap.uv_stream_t_ptr");
    stream = *stream_p;

    luaL_argcheck (L, stream->data == NULL, 1,
                   "stream->data is not NULL");

    int backlog;
    backlog = (int) luaL_checkint(L, 2);

    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_pushvalue(L, 3);

    lua_ref *ref = malloc(sizeof(lua_ref));
    if (ref == NULL) {
        luaL_error(L, "malloc failed");
    }
    ref->L = L;
    ref->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref->ref == LUA_NOREF ||
        ref->ref == LUA_REFNIL) {
        free(ref);
        luaL_error(L, "luaL_ref failed");
    }

    stream->data = ref;

    printf("wrap_uv_listen %p %p %d\n", stream, stream->loop, backlog);

    int res = (int) uv_listen(stream, backlog, wrap_uv_on_connection);

    printf("wrap_uv_listen %p %p %d %d\n", stream, stream->loop, backlog, res);

    lua_pushinteger(L, res);
    return 1;
}


// ----------------------------------------

// An echo-server for testing.
//
#define ASSERT assert
typedef struct {
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;

static uv_loop_t* loop;

static int server_closed;
static uv_tcp_t tcpServer;
static uv_pipe_t pipeServer;
static uv_handle_t* server;

static void after_write(uv_write_t* req, int status);
static void after_read(uv_stream_t*, ssize_t nread, uv_buf_t buf);
static void on_close(uv_handle_t* peer);
static void on_server_close(uv_handle_t* handle);
static void on_connection(uv_stream_t*, int status);


static void after_write(uv_write_t* req, int status) {
  write_req_t* wr;

  if (status) {
    uv_err_t err = uv_last_error(loop);
    fprintf(stderr, "uv_write error: %s\n", uv_strerror(err));
    ASSERT(0);
  }

  wr = (write_req_t*) req;

  /* Free the read/write buffer and the request */
  free(wr->buf.base);
  free(wr);
}


static void after_shutdown(uv_shutdown_t* req, int status) {
  uv_close((uv_handle_t*)req->handle, on_close);
  free(req);
}


static void after_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
  int i;
  write_req_t *wr;
  uv_shutdown_t* req;

  if (nread < 0) {
    /* Error or EOF */
    ASSERT (uv_last_error(loop).code == UV_EOF);

    if (buf.base) {
      free(buf.base);
    }

    req = (uv_shutdown_t*) malloc(sizeof *req);
    uv_shutdown(req, handle, after_shutdown);

    return;
  }

  if (nread == 0) {
    /* Everything OK, but nothing read. */
    free(buf.base);
    return;
  }

  /*
   * Scan for the letter Q which signals that we should quit the server.
   * If we get QS it means close the stream.
   */
  if (!server_closed) {
    for (i = 0; i < nread; i++) {
      if (buf.base[i] == 'Q') {
        if (i + 1 < nread && buf.base[i + 1] == 'S') {
          free(buf.base);
          uv_close((uv_handle_t*)handle, on_close);
          return;
        } else {
          uv_close(server, on_server_close);
          server_closed = 1;
        }
      }
    }
  }

  wr = (write_req_t*) malloc(sizeof *wr);

  wr->buf = uv_buf_init(buf.base, nread);
  if (uv_write(&wr->req, handle, &wr->buf, 1, after_write)) {
      // FATAL("uv_write failed");
  }
}


static void on_close(uv_handle_t* peer) {
  free(peer);
}


static uv_buf_t echo_alloc(uv_handle_t* handle, size_t suggested_size) {
  return uv_buf_init(malloc(suggested_size), suggested_size);
}


static void on_connection(uv_stream_t* server, int status) {
  uv_stream_t* stream;
  int r;

  if (status != 0) {
    fprintf(stderr, "Connect error %d\n",
        uv_last_error(loop).code);
  }
  ASSERT(status == 0);

    stream = malloc(sizeof(uv_tcp_t));
    ASSERT(stream != NULL);
    uv_tcp_init(loop, (uv_tcp_t*)stream);

  /* associate server with stream */
  stream->data = server;

  r = uv_accept(server, stream);
  ASSERT(r == 0);

  r = uv_read_start(stream, echo_alloc, after_read);
  ASSERT(r == 0);
}


static void on_server_close(uv_handle_t* handle) {
  ASSERT(handle == server);
}


LUA_API int test_helper_func(lua_State *L) {

  loop = uv_default_loop();
  int port = 11800;
  struct sockaddr_in addr = uv_ip4_addr("0.0.0.0", port);
  int r;

  server = (uv_handle_t*)&tcpServer;

  r = uv_tcp_init(loop, &tcpServer);
  if (r) {
    /* TODO: Error codes */
    fprintf(stderr, "Socket creation error\n");
    return 1;
  }

  r = uv_tcp_bind(&tcpServer, addr);
  if (r) {
    /* TODO: Error codes */
    fprintf(stderr, "Bind error\n");
    return 1;
  }

  r = uv_listen((uv_stream_t*)&tcpServer, SOMAXCONN, on_connection);
  if (r) {
    /* TODO: Error codes */
    fprintf(stderr, "Listen error %s\n",
        uv_err_name(uv_last_error(loop)));
    return 1;
  }

  uv_run(loop);


   double trouble = lua_tonumber(L, 1);
   lua_pushnumber(L, 16.0 - trouble);
   return 1;
}

