CC=gcc
CFLAGS=-O3 -Wall
LDFLAGS=-lpthread


TARGETS=perf


perf: main.c tracee.c scalls.c conf.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


