CC=gcc
CFLAGS=-O3 -Wall
LDFLAGS=-lunwind -lpthread


TARGETS=tau_perf



tau_perf: main.c tracee.c scalls.c conf.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


