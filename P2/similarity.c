#include "similarity.h"

#include <math.h>
#include <string.h>

double jensen_shannon_distance(const WordFreq *a, const WordFreq *b) {
    if (a->total_words == 0 || b->total_words == 0) {
        return 0.0;
    }

    const WordNode *cur_a = a->head;
    const WordNode *cur_b = b->head;
    double kld_a = 0.0;
    double kld_b = 0.0;

    while (cur_a != NULL || cur_b != NULL) {
        double fa = 0.0;
        double fb = 0.0;

        if (cur_b == NULL || (cur_a != NULL && strcmp(cur_a->word, cur_b->word) < 0)) {
            fa = (double)cur_a->count / (double)a->total_words;
            cur_a = cur_a->next;
        } else if (cur_a == NULL || strcmp(cur_a->word, cur_b->word) > 0) {
            fb = (double)cur_b->count / (double)b->total_words;
            cur_b = cur_b->next;
        } else {
            fa = (double)cur_a->count / (double)a->total_words;
            fb = (double)cur_b->count / (double)b->total_words;
            cur_a = cur_a->next;
            cur_b = cur_b->next;
        }

        double mean = 0.5 * (fa + fb);
        if (fa > 0.0) {
            kld_a += fa * log2(fa / mean);
        }
        if (fb > 0.0) {
            kld_b += fb * log2(fb / mean);
        }
    }

    return sqrt(0.5 * kld_a + 0.5 * kld_b);
}
