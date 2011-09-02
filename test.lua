uv_wrap = require("uv_wrap")

f = package.loadlib("uv_wrap.so", "foo")

avg, sum = f(1,2,3)

assert(avg == 2)
assert(sum == 6)

if false then
  for k, v in pairs(uv_wrap) do
    print(k, v)
  end
end

assert(uv_wrap.UV_UNKNOWN == -1)
assert(uv_wrap.UV_OK == 0)
assert(uv_wrap.UV_EOF == 1)

print("OK")