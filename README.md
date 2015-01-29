## libnsq

async C client library for [NSQ][1]

### Status

This is currently pretty low-level, but functional.  The raw building blocks for communicating
asynchronously via the NSQ TCP protocol are in place as well as a basic "reader" abstraction that facilitates
subscribing and receiving messages via callback.

TODO:

 * maintaining `RDY` count automatically
 * feature negotiation
 * better abstractions for responding to messages in your handlers

### Dependencies

 * [libev][2]
 * [libevbuffsock][3]

[1]: https://github.com/bitly/nsq
[2]: http://software.schmorp.de/pkg/libev
[3]: https://github.com/mreiferson/libevbuffsock

----------------------------Huangyan9188--------------------
on the MAC OSX
if you cannot build the test demo
using following
ar rc libnsq.a command.o reader.o nsqd_connection.o http.o message.o nsqlookupd.o
ranlib libnsq.a
cc -o test.o -c test.c -g -Wall -O2 -DDEBUG -fPIC
cc -o test test.o libnsq.a -lcurl /usr/local/lib/libevbuffsock.a /usr/local/lib/libev.a /usr/local/lib/libjson-c.a

----------------------------OR------------------------------
Just checkout the Makefile, You need to know that the dependencies are
libev
libjson-c
libevbuffsock
libcurl

Please set these ldconfig, or just point out the .a library file
