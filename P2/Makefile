CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g
TARGET = compare
SRCS = p2compare.c dirwalk.c freqmap.c similarity.c

all: $(TARGET)

$(TARGET): $(SRCS) dirwalk.h freqmap.h similarity.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lm

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean