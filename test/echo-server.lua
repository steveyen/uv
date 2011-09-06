l = require('uv_wrap')
assert(l and l.UV_OK)

l.uv_init()

port = 11700
addr = l.uv_ip4_addr("0.0.0.0", port)
assert(addr)

loop = l.uv_default_loop()
assert(loop)

tcp = l.alloc_uv_tcp_t_ptr(loop)
assert(tcp)

assert(0 == l.uv_tcp_init(loop, tcp))
assert(0 == l.uv_tcp_bind(tcp, addr))

function on_connection()
end

tcp_stream = l.cast_uv_tcp_t_ptr_to_uv_stream_t_ptr(tcp)

assert(0 == l.uv_listen(tcp_stream, 1024, on_connection))

l.uv_run(loop)

print("OK")
