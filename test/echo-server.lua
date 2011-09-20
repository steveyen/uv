#!/usr/bin/env luajit

l = require('uv_wrap')
assert(l and l.UV_OK)

port = 11700
addr = l.uv_ip4_addr("0.0.0.0", port)
assert(addr)

loop = l.uv_default_loop()
assert(loop)

listen_tcp = l.alloc_uv_tcp_t_ptr(loop)
assert(listen_tcp)
assert(0 == l.uv_tcp_init(loop, listen_tcp))
assert(0 == l.uv_tcp_bind(listen_tcp, addr))
listen_stream = l.cast_uv_tcp_t_ptr_to_uv_stream_t_ptr(listen_tcp)

function on_read(nread)
  print("on_read " .. nread)
end

function on_listen(status)
  print("on_listen " .. status)

  client_tcp = l.alloc_uv_tcp_t_ptr(loop)
  assert(client_tcp)
  assert(0 == l.uv_tcp_init(loop, client_tcp))
  client_stream = l.cast_uv_tcp_t_ptr_to_uv_stream_t_ptr(client_tcp)
  assert(0 == l.uv_accept(listen_stream, client_stream))
  assert(0 == l.uv_read_start(client_stream, on_read))
end

assert(0 == l.uv_listen(listen_stream, 128, on_listen))

print("running loop\n")

l.uv_run(loop)

print("OK")
