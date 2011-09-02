LIBUV_DIR = ../libuv

all: uv_wrap.so

test:
	lua test.lua

clean:
	rm -f *.so *.o uv_wrap_gen.c

uv_wrap.so: ${LIBUV_DIR}/uv.a uv_wrap.o uv_wrap_gen.c
	$(CC) -shared -o uv_wrap.so uv_wrap.o ${LIBUV_DIR}/uv.a -llua

uv_wrap.o: uv_wrap.c
	$(CC) -fPIC -g -c -Wall uv_wrap.c

uv_wrap_gen.c:
	grep -v "#include" ${LIBUV_DIR}/include/uv.h | cpp | ./swigl > uv_wrap_gen.c

