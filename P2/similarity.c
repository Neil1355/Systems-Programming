#include "similarity.h"

#include <math.h>

/* Cosine similarity on sparse linked-list vectors. */
double cosine_similarity(const WordNode *a, const WordNode *b) {
    double dot = 0.0;
    double mag_a = 0.0;
    double mag_b = 0.0;

    const WordNode *cur_a = a;
    while (cur_a != NULL) {
        int count_b = freq_get_count(b, cur_a->word);
        dot += (double)cur_a->count * (double)count_b;
        mag_a += (double)cur_a->count * (double)cur_a->count;
        cur_a = cur_a->next;
    }

    const WordNode *cur_b = b;
    while (cur_b != NULL) {
        mag_b += (double)cur_b->count * (double)cur_b->count;
        cur_b = cur_b->next;
    }

    if (mag_a == 0.0 || mag_b == 0.0) {
        return 0.0;
    }

    return dot / (sqrt(mag_a) * sqrt(mag_b));
}
