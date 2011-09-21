# uv - simple lua wrapping of libuv.

libuv is the O/S independent platform core of node.js and handles
issues like networking and file I/O.  Now, the goodness of libuv is
now available for lua.  See http://github.com/joyent/libuv for more
information on libuv.

The libuv wrappers in this project are rather low-level, where the
mechanical exposure of libuv API's to lua are pretty true to libuv's
original C API's.

The wrappings are tested against (at least) lua 5.1.4 and
luajit 2.0.0-beta8.

## Building

### Prerequisites

The swiglite tool uses ruby, rubygems and ERB.

And, you'll need lua 5.1.4 'make install'ed.

### Easy Approach

The easy way is to have libuv as a sibling directory.
For example...

    git clone git://github.com/joyent/libuv.git
    cd libuv && make
    cd ..
    git clone git://github.com/steveyen/uv.git
    cd uv && make

Then, you can test the included echo server...

    lua test/echo_server.lua

Or, you can also use luajit instead of lua...

    luajit test/echo_server.lua

In a separate terminal, a client will receive echo'ed strings...

    telnet 127.0.0.1 11700
    > hello
    < hello

## Development notes

The uv wrappers use the (included) "swiglite" tool, which is
implemented in ruby.

## LICENSE

Apache License Version 2.0

