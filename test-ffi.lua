local ffi = require("ffi")

ffi.cdef[[
int printf(const char *fmt, ...);
]]

ffi.C.printf("Hello %s!\n", "world")

------------------------------------------------

ffi.cdef[[
void Sleep(int ms);
int poll(struct pollfd *fds, unsigned long nfds, int timeout);
]]

if false then
local sleep
function sleep(s)
  ffi.C.poll(nil, 0, s*1000)
end

for i=1,160 do
  io.write("."); io.flush()
  sleep(0.01)
end
io.write("\n")
end

------------------------------------------------

ffi.cdef[[
unsigned long compressBound(unsigned long sourceLen);
int compress2(uint8_t *dest, unsigned long *destLen,
	      const uint8_t *source, unsigned long sourceLen, int level);
int uncompress(uint8_t *dest, unsigned long *destLen,
	       const uint8_t *source, unsigned long sourceLen);
]]
local zlib = ffi.load(ffi.os == "Windows" and "zlib1" or "z")

local function compress(txt)
  local n = zlib.compressBound(#txt)
  local buf = ffi.new("uint8_t[?]", n)
  local buflen = ffi.new("unsigned long[1]", n)
  local res = zlib.compress2(buf, buflen, txt, #txt, 9)
  assert(res == 0)
  return ffi.string(buf, buflen[0])
end

local function uncompress(comp, n)
  local buf = ffi.new("uint8_t[?]", n)
  local buflen = ffi.new("unsigned long[1]", n)
  local res = zlib.uncompress(buf, buflen, comp, #comp)
  assert(res == 0)
  return ffi.string(buf, buflen[0])
end

-- Simple test code.
local txt = string.rep("abcd", 1000)
print("Uncompressed size: ", #txt)
local c = compress(txt)
print("Compressed size: ", #c)
local txt2 = uncompress(c, #txt)
assert(txt2 == txt)

------------------------------------------------

ffi.cdef[[
typedef struct uv_loop_s uv_loop_t;
uv_loop_t* uv_default_loop();
]]

local u = ffi.load("uv")

print(u)
print(u.uv_default_loop())
