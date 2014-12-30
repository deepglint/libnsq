PREFIX=/usr/local
DESTDIR=
LIBDIR=${PREFIX}/lib
INCDIR=${PREFIX}/include

CFLAGS+=-g -Wall -O2 -DDEBUG -fPIC -I $(INCDIR)
LIBS= -lcurl /usr/local/lib/libevbuffsock.a /usr/local/lib/libev.a /usr/local/lib/libjson-c.a
AR=ar
AR_FLAGS=rc
RANLIB=ranlib

all: libnsq test

libnsq: libnsq.a

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

libnsq.a: command.o reader.o nsqd_connection.o http.o message.o nsqlookupd.o
	$(AR) $(AR_FLAGS) $@ $^
	$(RANLIB) $@

test: test.o libnsq.a
	$(CC) -o $@ $^ $(LIBS) -I $(INCDIR)

clean:
	rm -rf libnsq.a test test.dSYM *.o

.PHONY: install clean all

install:
	install -m 755 -d ${DESTDIR}${INCDIR}
	install -m 755 -d ${DESTDIR}${LIBDIR}
	install -m 755 libnsq.a ${DESTDIR}${LIBDIR}/libnsq.a
	install -m 755 nsq.h ${DESTDIR}${INCDIR}/nsq.h
