uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

LIBUV_DIR = ../libuv

CFLAGS = -I${LIBUV_DIR}/include -g

ifeq (Darwin,$(uname_S))
LINKFLAGS+=-framework CoreServices
endif

all: uv_wrap.so

test: all
	lua test.lua

clean:
	rm -f *.so *.o uv_wrap_gen.c

uv_wrap.so: ${LIBUV_DIR}/uv.a uv_wrap.o uv_wrap_gen.o
	$(CC) -shared -o uv_wrap.so \
        uv_wrap.o uv_wrap_gen.o \
        ${LIBUV_DIR}/uv.a -llua $(LINKFLAGS)

uv_wrap_gen.c:
	grep -v "#include" ${LIBUV_DIR}/include/uv.h | cpp | \
        sed -E "s,[A-Z_]+_PRIVATE_[A-Z_]+,,g" | \
            ./swiglite uv_wrap uv.h > uv_wrap_gen.c
