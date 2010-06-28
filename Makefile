# Makefile for httpsqs
CC=gcc
CFLAGS=-L/usr/local/libevent-1.4.14b-stable/lib/ -levent -L/usr/local/tokyocabinet-1.4.45/lib/ -ltokyocabinet -I/usr/local/libevent-1.4.14b-stable/include/ -I/usr/local/tokyocabinet-1.4.45/include/ -lz -lbz2 -lrt -lpthread -lm -lc -O2

httpsqs: httpsqs.c
	$(CC) -o httpsqs httpsqs.c $(CFLAGS)

clean: httpsqs
	rm -f httpsqs

install: httpsqs
	install $(INSTALL_FLAGS) -m 4755 -o root httpsqs $(DESTDIR)/usr/bin
