CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
LDLIBS =

ifeq ($(OS),Windows_NT)
LDLIBS = -lws2_32
endif

all: chatd

chatd: chatd.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f chatd *.o

.PHONY: all clean