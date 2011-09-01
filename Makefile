LIBUV_DIR = ../libuv

all: lualuv.so

clean:
	rm -f *.so *.o

lualuv.so: ${LIBUV_DIR}/uv.a lualuv.o
	$(CC) -shared -o lualuv.so lualuv.o ${LIBUV_DIR}/uv.a -llua

lualuv.o: lualuv.c
	$(CC) -fPIC -g -c -Wall lualuv.c

