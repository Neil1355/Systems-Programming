#include "freqmap.h"

#include <stdlib.h>

WordNode *freq_build_from_file(const char *path) {
    (void)path;
    /* TODO(hriday): implement token parsing and frequency map construction. */
    return NULL;
}

void freq_free(WordNode *head) {
    WordNode *cur = head;
    while (cur != NULL) {
        WordNode *next = cur->next;
        free(cur->word);
        free(cur);
        cur = next;
    }
}

int freq_get_count(const WordNode *head, const char *word) {
    (void)head;
    (void)word;
    /* TODO(hriday): implement lookup by word in frequency map. */
    return 0;
}
