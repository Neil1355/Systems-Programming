CC      = gcc
CFLAGS  = -Wall -Wextra -g

SRCS    = mysh.c tokenizer.c wildcard.c builtins.c executor.c
OBJS    = $(SRCS:.c=.o)
TARGET  = mysh

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c mysh.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)