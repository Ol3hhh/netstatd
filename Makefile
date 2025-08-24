CC=gcc
CFLAGS=-Wall -O2 -Iinclude
LDFLAGS=-lpcap -lsqlite3

all: bin/daemon bin/cli

bin/daemon: src/daemon.c src/db.c include/db.h
	mkdir -p bin
	$(CC) $(CFLAGS) -o $@ src/daemon.c src/db.c $(LDFLAGS)

bin/cli: src/cli.c
	mkdir -p bin
	$(CC) $(CFLAGS) -o $@ src/cli.c

clean:
	rm -f bin/daemon bin/cli
