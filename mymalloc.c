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

typedef size_t Header;

#define HEADER_SIZE        sizeof(Header)
#define HDR_SIZE(h)        (*(h) & ~(size_t)0x7)
#define HDR_ALLOC(h)       (*(h) &  (size_t)0x1)
#define HDR_PACK(sz, al)   ((sz) | (size_t)(al))

#define ROUND8(n)  (((n) + 7) & ~(size_t)7)
#define MIN_CHUNK  (HEADER_SIZE + 8)

static Header *first_chunk(void) { return (Header *)heap.bytes; }

static Header *next_chunk(Header *h) {
    return (Header *)((char *)h + HDR_SIZE(h));
}

static int in_heap(void *p) {
    return (char *)p >= heap.bytes && (char *)p < heap.bytes + MEMLENGTH;
}

static void leak_detector(void) {
    int count = 0; size_t total = 0;
    for (Header *h = first_chunk(); in_heap(h); h = next_chunk(h)) {
        if (HDR_ALLOC(h)) { count++; total += HDR_SIZE(h) - HEADER_SIZE; }
    }
    if (count > 0)
        fprintf(stderr, "mymalloc: %zu bytes leaked in %d objects.\n", total, count);
}

static void init_heap(void) {
    *first_chunk() = HDR_PACK(MEMLENGTH, 0);
    initialized = 1;
    atexit(leak_detector);
}

void *mymalloc(size_t size, char *file, int line) {
    if (!initialized) init_heap();
    if (size == 0) {
        fprintf(stderr, "malloc: Unable to allocate 0 bytes (%s:%d)\n", file, line);
        return NULL;
    }
    size_t needed = HEADER_SIZE + ROUND8(size);
    for (Header *h = first_chunk(); in_heap(h); h = next_chunk(h)) {
        if (!HDR_ALLOC(h) && HDR_SIZE(h) >= needed) {
            size_t leftover = HDR_SIZE(h) - needed;
            if (leftover >= MIN_CHUNK) {
                *(Header *)((char *)h + needed) = HDR_PACK(leftover, 0);
                *h = HDR_PACK(needed, 1);
            } else {
                *h = HDR_PACK(HDR_SIZE(h), 1);
            }
            return (char *)h + HEADER_SIZE;
        }
    }
    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
    return NULL;
}

void myfree(void *ptr, char *file, int line) {
    if (!initialized) init_heap();
    if (!in_heap(ptr)) {
        fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
        exit(2);
    }
    Header *target = NULL;
    for (Header *h = first_chunk(); in_heap(h); h = next_chunk(h)) {
        if ((char *)h + HEADER_SIZE == (char *)ptr) { target = h; break; }
    }
    if (!target) {
        fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
        exit(2);
    }
    if (!HDR_ALLOC(target)) {
        fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
        exit(2);
    }
    *target = HDR_PACK(HDR_SIZE(target), 0);

    for (Header *nxt = next_chunk(target); in_heap(nxt) && !HDR_ALLOC(nxt);
         nxt = next_chunk(target))
        *target = HDR_PACK(HDR_SIZE(target) + HDR_SIZE(nxt), 0);

    for (Header *h = first_chunk(); in_heap(h); h = next_chunk(h)) {
        Header *n = next_chunk(h);
        if (n == target && !HDR_ALLOC(h)) {
            *h = HDR_PACK(HDR_SIZE(h) + HDR_SIZE(target), 0);
            for (Header *nxt = next_chunk(h); in_heap(nxt) && !HDR_ALLOC(nxt);
                 nxt = next_chunk(h))
                *h = HDR_PACK(HDR_SIZE(h) + HDR_SIZE(nxt), 0);
            break;
        }
        if (n > target) break;
    }
}