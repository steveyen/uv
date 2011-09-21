#!/usr/bin/env luajit

l = require('uv_wrap')
assert(l and l.UV_OK)

port = 11700
addr = assert(l.uv_ip4_addr("0.0.0.0", port))
loop = assert(l.uv_default_loop())

listen_tcp = assert(l.alloc_uv_tcp_t_ptr(loop))
assert(0 == l.uv_tcp_init(loop, listen_tcp))
assert(0 == l.uv_tcp_bind(listen_tcp, addr))
listen_stream = assert(l.cast_uv_tcp_t_ptr_to_uv_stream_t_ptr(listen_tcp))

function on_listen(status)
  print("on_listen " .. status)

  local client_tcp = assert(l.alloc_uv_tcp_t_ptr(loop))
  assert(0 == l.uv_tcp_init(loop, client_tcp))
  local client_stream = assert(l.cast_uv_tcp_t_ptr_to_uv_stream_t_ptr(client_tcp))
  assert(0 == l.uv_accept(listen_stream, client_stream))

  function on_write(status)
    print("on_write " .. status)
  end

  function on_read(nread, s)
    if nread > 0 then
      print("on_read " .. nread .. ": " .. s)
      l.uv_write(client_stream, s, on_write)
    else
      print("on_read " .. nread)
    end
  end

  assert(0 == l.uv_read_start(client_stream, on_read))
end

assert(0 == l.uv_listen(listen_stream, 128, on_listen))

print("running loop\n")

l.uv_run(loop)

print("OK")
