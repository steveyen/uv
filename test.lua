uv_wrap = require("uv_wrap")

f = package.loadlib("uv_wrap.so", "foo")

avg, sum = f(1,2,3)

assert(avg == 2)
assert(sum == 6)

assert(uv_wrap.UV_OK == 0)
assert(uv_wrap.UV_EOF == 10)
