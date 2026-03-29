#ifndef FREQMAP_H
#define FREQMAP_H

typedef struct WordNode {
    char *word;
    int count;
    struct WordNode *next;
} WordNode;

WordNode *freq_build_from_file(const char *path);
void freq_free(WordNode *head);
int freq_get_count(const WordNode *head, const char *word);

#endif
