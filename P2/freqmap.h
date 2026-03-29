#ifndef FREQMAP_H
#define FREQMAP_H

#include <stddef.h>

typedef struct WordNode {
    char *word;
    size_t count;
    struct WordNode *next;
} WordNode;

typedef struct {
    WordNode *head;
    size_t total_words;
} WordFreq;

/*
 * Return values:
 *   1  success
 *   0  recoverable I/O error (already reported via perror)
 *  -1  unrecoverable allocation failure
 */
int freq_build_from_file(const char *path, WordFreq *out);
void freq_free(WordFreq *wf);

#endif
