# Makefile for httpsqs
CC=gcc
CFLAGS=-Wl,-rpath,/usr/local/libevent-2.0.12-stable/lib/:/usr/local/tokyocabinet-1.4.47/lib/ -L/usr/local/libevent-2.0.12-stable/lib/ -levent -L/usr/local/tokyocabinet-1.4.47/lib/ -ltokyocabinet -I/usr/local/libevent-2.0.12-stable/include/ -I/usr/local/tokyocabinet-1.4.47/include/ -lz -lbz2 -lrt -lpthread -lm -lc -O2 -g

httpsqs: httpsqs.c
	$(CC) -o httpsqs httpsqs.c prename.c $(CFLAGS)
	@echo ""
	@echo "httpsqs build complete."
	@echo ""	

clean: httpsqs
	rm -f httpsqs

install: httpsqs
	install $(INSTALL_FLAGS) -m 4755 -o root httpsqs $(DESTDIR)/usr/bin
