#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mymalloc.h"

#define MEMLENGTH 4096

static union {
    char bytes[MEMLENGTH];
    double not_used;
} heap;

static int initialized = 0;

// TODO: Implement mymalloc and myfree
void * mymalloc(size_t size, char *file, int line) {
    return NULL;
}

void myfree(void *ptr, char *file, int line) {
}
