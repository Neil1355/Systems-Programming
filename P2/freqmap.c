#include "freqmap.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int is_word_char(unsigned char c) {
    return isalnum(c) || c == '-';
}

static int grow_token(char **token, size_t *cap, size_t needed) {
    if (needed <= *cap) {
        return 1;
    }

    size_t new_cap = (*cap == 0) ? 32 : *cap;
    while (new_cap < needed) {
        new_cap *= 2;
    }

    char *next = realloc(*token, new_cap);
    if (next == NULL) {
        return 0;
    }

    *token = next;
    *cap = new_cap;
    return 1;
}

static int insert_or_increment_sorted(WordFreq *wf, const char *word) {
    WordNode **cur = &wf->head;
    while (*cur != NULL) {
        int cmp = strcmp((*cur)->word, word);
        if (cmp == 0) {
            (*cur)->count++;
            return 1;
        }
        if (cmp > 0) {
            break;
        }
        cur = &(*cur)->next;
    }

    size_t len = strlen(word);
    WordNode *new_node = malloc(sizeof(WordNode));
    if (new_node == NULL) {
        return 0;
    }

    new_node->word = malloc(len + 1);
    if (new_node->word == NULL) {
        free(new_node);
        return 0;
    }

    strcpy(new_node->word, word);
    new_node->count = 1;
    new_node->next = *cur;
    *cur = new_node;
    return 1;
}

static int finalize_token(WordFreq *wf, char *token, size_t *len) {
    if (*len == 0) {
        return 1;
    }

    token[*len] = '\0';
    if (!insert_or_increment_sorted(wf, token)) {
        return 0;
    }
    wf->total_words++;
    *len = 0;
    return 1;
}

int freq_build_from_file(const char *path, WordFreq *out) {
    out->head = NULL;
    out->total_words = 0;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        return 0;
    }

    char *token = NULL;
    size_t token_len = 0;
    size_t token_cap = 0;
    char buf[4096];

    for (;;) {
        ssize_t nread = read(fd, buf, sizeof(buf));
        if (nread == 0) {
            break;
        }
        if (nread < 0) {
            perror(path);
            free(token);
            freq_free(out);
            close(fd);
            return 0;
        }

        for (ssize_t i = 0; i < nread; i++) {
            unsigned char c = (unsigned char)buf[i];
            if (isspace(c)) {
                if (!finalize_token(out, token, &token_len)) {
                    free(token);
                    freq_free(out);
                    close(fd);
                    return -1;
                }
                continue;
            }

            if (!is_word_char(c)) {
                continue;
            }

            if (!grow_token(&token, &token_cap, token_len + 2)) {
                free(token);
                freq_free(out);
                close(fd);
                return -1;
            }
            token[token_len++] = (char)tolower(c);
        }
    }

    if (!finalize_token(out, token, &token_len)) {
        free(token);
        freq_free(out);
        close(fd);
        return -1;
    }

    free(token);
    close(fd);
    return 1;
}

void freq_free(WordFreq *wf) {
    WordNode *cur = wf->head;
    while (cur != NULL) {
        WordNode *next = cur->next;
        free(cur->word);
        free(cur);
        cur = next;
    }
    wf->head = NULL;
    wf->total_words = 0;
}
