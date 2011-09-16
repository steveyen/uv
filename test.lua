uv_wrap = require("uv_wrap")

if false then
  for k, v in pairs(uv_wrap) do
    print(k, v)
  end
end

assert(uv_wrap.UV_UNKNOWN == -1)
assert(uv_wrap.UV_OK == 0)
assert(uv_wrap.UV_EOF == 1)

print("OK - basic enums")

d0 = uv_wrap.uv_default_loop()
assert(d0)

d1 = uv_wrap.uv_default_loop()
assert(d1)
assert(d1 ~= d0)

t0 = uv_wrap.uv_now(d0)
assert(t0 > 0)
uv_wrap.uv_update_time(d0)
t1 = uv_wrap.uv_now(d0)
assert(t1 >= t0)

e0 = uv_wrap.uv_last_error(d0)
assert(uv_wrap.uv_err_name(e0) == "OK")

b0 = uv_wrap.uv_buf_init("hello", 5)
assert(b0)

print("OK - simple tests")

if false then
  -- SKIP: libuv's uv_std_handle() still unimplemented.
  --
  stdin = uv_wrap.uv_std_handle(d0, uv_wrap.UV_STDIN)
  assert(stdin)

  stdout = uv_wrap.uv_std_handle(d0, uv_wrap.UV_STDOUT)
  assert(stdout)

  stderr = uv_wrap.uv_std_handle(d0, uv_wrap.UV_STDERR)
  assert(stderr)

  print("OK - stdin/out/err available")
end

print("OK - done")