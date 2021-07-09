CC=gcc
CFLAGS=-O3 -Wall
LDFLAGS=t -lpthread


TARGETS=perf


perf: main.c tracee.c scalls.c conf.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


