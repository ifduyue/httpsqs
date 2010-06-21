# Makefile for httpsqs
CC=gcc
CFLAGS=-O2 -Wall -levent -ltokyocabinet -lz -lrt -lpthread -lm -lc

httpsqs: httpsqs.c
	$(CC) $(CFLAGS) httpsqs.c -o httpsqs

clean: httpsqs
	rm -f httpsqs

install: httpsqs
	install $(INSTALL_FLAGS) -m 4755 -o root httpsqs $(DESTDIR)/usr/bin
