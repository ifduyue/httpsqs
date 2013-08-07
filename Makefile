# Makefile for httpsqs
libevent?=/usr/local/libevent-2.0.12-stable/lib
tokyocabinet?=/usr/local/tokyocabinet-1.4.47
CC=gcc
CFLAGS=-Wl,-rpath,$(libevent)/lib/:$(tokyocabinet)/lib/ -L$(libevent)/lib/ -levent -L$(tokyocabinet)/lib/ -ltokyocabinet -I$(libevent)/include/ -I$(tokyocabinet)/include/ -lz -lbz2 -lrt -lpthread -lm -lc -O2 -g

httpsqs: httpsqs.c
	$(CC) -o httpsqs httpsqs.c prename.c $(CFLAGS)
	@echo ""
	@echo "httpsqs build complete."
	@echo ""	

clean: httpsqs
	rm -f httpsqs

install: httpsqs
	install $(INSTALL_FLAGS) -m 4755 -o root httpsqs $(DESTDIR)/usr/bin
