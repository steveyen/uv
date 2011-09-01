LIBUV_DIR = ../libuv

all: uv_wrap.so

test:
	lua test.lua

clean:
	rm -f *.so *.o

uv_wrap.so: ${LIBUV_DIR}/uv.a uv_wrap.o
	$(CC) -shared -o uv_wrap.so uv_wrap.o ${LIBUV_DIR}/uv.a -llua

uv_wrap.o: uv_wrap.c
	$(CC) -fPIC -g -c -Wall uv_wrap.c

