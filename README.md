# uv - lua wrapping of libuv + more

Uses the (included) swiglite tool to wrap libuv.

## Building

### Prerequisites

The swiglite tool uses ruby, rubygems and ERB.

### Easy Approach

The easy way is to have libuv as a sibling directory.
For example...

    git clone git://github.com/joyent/libuv.git
    cd libuv && make
    cd ..
    git clone git://github.com/steveyen/uv.git
    cd uv && make

## LICENSE

Apache License Version 2.0

