#include "dirwalk.h"
#include "freqmap.h"
#include "similarity.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    const char *path;
    WordFreq wf;
    int valid;
} FileData;

typedef struct {
    const char *path_a;
    const char *path_b;
    size_t combined_words;
    double jsd;
} Comparison;

static int add_argument_path(const char *arg, FileVector *files, int *had_error) {
    struct stat st;
    if (stat(arg, &st) != 0) {
        perror(arg);
        *had_error = 1;
        return 1;
    }

    if (path_has_hidden_basename(arg)) {
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        return collect_text_files_recursive(arg, files, had_error);
    }
    if (S_ISREG(st.st_mode)) {
        if (!filevec_push_unique(files, arg)) {
            return 0;
        }
        return 1;
    }

    fprintf(stderr, "%s: not a regular file or directory\n", arg);
    *had_error = 1;
    return 1;
}

static int cmp_comparisons(const void *a, const void *b) {
    const Comparison *ca = (const Comparison *)a;
    const Comparison *cb = (const Comparison *)b;

    if (ca->combined_words > cb->combined_words) {
        return -1;
    }
    if (ca->combined_words < cb->combined_words) {
        return 1;
    }

    int path_cmp = strcmp(ca->path_a, cb->path_a);
    if (path_cmp != 0) {
        return path_cmp;
    }
    return strcmp(ca->path_b, cb->path_b);
}

static void free_file_data(FileData *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (data[i].valid) {
            freq_free(&data[i].wf);
        }
    }
}

static int build_file_data(const FileVector *files, FileData *data, int *had_error,
                           size_t *valid_count) {
    *valid_count = 0;
    for (size_t i = 0; i < files->len; i++) {
        data[i].path = files->items[i];
        data[i].valid = 0;
        data[i].wf.head = NULL;
        data[i].wf.total_words = 0;

        int status = freq_build_from_file(files->items[i], &data[i].wf);
        if (status < 0) {
            return 0;
        }
        if (status == 0) {
            *had_error = 1;
            continue;
        }

        data[i].valid = 1;
        (*valid_count)++;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file-or-directory> [file-or-directory ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    FileVector files;
    filevec_init(&files);
    int had_error = 0;

    for (int i = 1; i < argc; i++) {
        if (!add_argument_path(argv[i], &files, &had_error)) {
            filevec_free(&files);
            fprintf(stderr, "Out of memory while collecting input files\n");
            return EXIT_FAILURE;
        }
    }

    if (files.len < 2) {
        fprintf(stderr, "Need at least two readable files to compare\n");
        filevec_free(&files);
        return EXIT_FAILURE;
    }

    FileData *data = calloc(files.len, sizeof(FileData));
    if (data == NULL) {
        filevec_free(&files);
        fprintf(stderr, "Out of memory while building file distributions\n");
        return EXIT_FAILURE;
    }

    size_t valid_count = 0;
    if (!build_file_data(&files, data, &had_error, &valid_count)) {
        free_file_data(data, files.len);
        free(data);
        filevec_free(&files);
        fprintf(stderr, "Out of memory while reading input files\n");
        return EXIT_FAILURE;
    }

    if (valid_count < 2) {
        fprintf(stderr, "Need at least two readable files to compare\n");
        free_file_data(data, files.len);
        free(data);
        filevec_free(&files);
        return EXIT_FAILURE;
    }

    size_t max_pairs = valid_count * (valid_count - 1) / 2;
    Comparison *pairs = malloc(max_pairs * sizeof(Comparison));
    if (pairs == NULL) {
        free_file_data(data, files.len);
        free(data);
        filevec_free(&files);
        fprintf(stderr, "Out of memory while preparing comparisons\n");
        return EXIT_FAILURE;
    }

    size_t pair_len = 0;
    for (size_t i = 0; i < files.len; i++) {
        if (!data[i].valid) {
            continue;
        }
        for (size_t j = i + 1; j < files.len; j++) {
            if (!data[j].valid) {
                continue;
            }

            pairs[pair_len].path_a = data[i].path;
            pairs[pair_len].path_b = data[j].path;
            pairs[pair_len].combined_words = data[i].wf.total_words + data[j].wf.total_words;
            pairs[pair_len].jsd = jensen_shannon_distance(&data[i].wf, &data[j].wf);
            pair_len++;
        }
    }

    qsort(pairs, pair_len, sizeof(Comparison), cmp_comparisons);

    for (size_t i = 0; i < pair_len; i++) {
        printf("%.5f %s %s\n", pairs[i].jsd, pairs[i].path_a, pairs[i].path_b);
    }

    free(pairs);
    free_file_data(data, files.len);
    free(data);
    filevec_free(&files);
    return had_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
