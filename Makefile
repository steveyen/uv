all: lualuv.so

lualuv.so: uv.a lualuv.o
	$(CC) -shared \
          -o lualuv.so lualuv.o uv.a -llua

lualuv.o: lualuv.c
	$(CC) -fPIC -g -c -Wall lualuv.c
