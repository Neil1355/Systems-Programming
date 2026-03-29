CC = gcc
CFLAGS = -Wall -Wextra -g
TARGETS = memtest memgrind

all: $(TARGETS)

memtest: memtest.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) -o memtest memtest.c mymalloc.c

memgrind: memgrind.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) -o memgrind memgrind.c mymalloc.c

clean:
	rm -f $(TARGETS) *.o core

.PHONY: all clean
