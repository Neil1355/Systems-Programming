#include "freqmap.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Hriday-owned module: token normalization and word-frequency map logic. */

/* Keep only alphanumeric characters and lowercase them. */
static int normalize_token(const char *raw, char *out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; raw[i] != '\0'; i++) {
        unsigned char c = (unsigned char)raw[i];
        if (isalnum(c)) {
            if (j + 1 >= out_size) {
                break;
            }
            out[j++] = (char)tolower(c);
        }
    }
    out[j] = '\0';
    return j > 0;
}

/* Linear lookup in linked-list frequency map. */
static WordNode *find_node(WordNode *head, const char *word) {
    WordNode *cur = head;
    while (cur != NULL) {
        if (strcmp(cur->word, word) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

/* Insert unseen token or increment existing count. */
static int insert_or_increment(WordNode **head, const char *word) {
    WordNode *node = find_node(*head, word);
    if (node != NULL) {
        node->count++;
        return 1;
    }

    WordNode *new_node = malloc(sizeof(WordNode));
    if (new_node == NULL) {
        return 0;
    }

    size_t len = strlen(word);
    new_node->word = malloc(len + 1);
    if (new_node->word == NULL) {
        free(new_node);
        return 0;
    }

    strcpy(new_node->word, word);
    new_node->count = 1;
    new_node->next = *head;
    *head = new_node;
    return 1;
}

WordNode *freq_build_from_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return NULL;
    }

    WordNode *head = NULL;
    char raw[512];
    char normalized[512];

    while (fscanf(fp, "%511s", raw) == 1) {
        if (!normalize_token(raw, normalized, sizeof(normalized))) {
            continue;
        }
        if (!insert_or_increment(&head, normalized)) {
            freq_free(head);
            fclose(fp);
            return NULL;
        }
    }

    fclose(fp);
    return head;
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
    const WordNode *cur = head;
    while (cur != NULL) {
        if (strcmp(cur->word, word) == 0) {
            return cur->count;
        }
        cur = cur->next;
    }
    return 0;
}
