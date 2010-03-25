# Makefile for httpsqs_client
CC=gcc

client_httpsqs: client_httpsqs.c
	$(CC) client_httpsqs.c -o client_httpsqs

clean: client_httpsqs
	rm -f client_httpsqs

install: client_httpsqs
	install -m 4755 -o root client_httpsqs $(DESTDIR)/usr/bin
