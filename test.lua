f = package.loadlib("lualuv.so", "foo")

avg, sum = f(1,2,3)

assert(avg == 2)
assert(sum == 6)
