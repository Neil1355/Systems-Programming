#include "dirwalk.h"
#include "freqmap.h"
#include "similarity.h"

#include <stdio.h>

static void compare_all_pairs(const FileVector *files) {
    for (size_t i = 0; i < files->len; i++) {
        WordNode *map_i = freq_build_from_file(files->items[i]);
        if (map_i == NULL) {
            fprintf(stderr, "Could not read %s\n", files->items[i]);
            continue;
        }

        for (size_t j = i + 1; j < files->len; j++) {
            WordNode *map_j = freq_build_from_file(files->items[j]);
            if (map_j == NULL) {
                fprintf(stderr, "Could not read %s\n", files->items[j]);
                continue;
            }

            double sim = cosine_similarity(map_i, map_j);
            printf("%.6f  %s  %s\n", sim, files->items[i], files->items[j]);
            freq_free(map_j);
        }

        freq_free(map_i);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    FileVector files;
    filevec_init(&files);

    if (!collect_text_files_recursive(argv[1], &files)) {
        fprintf(stderr, "Failed to scan directory: %s\n", argv[1]);
        filevec_free(&files);
        return 1;
    }

    if (files.len < 2) {
        fprintf(stderr, "Need at least two .txt files under %s\n", argv[1]);
        filevec_free(&files);
        return 1;
    }

    compare_all_pairs(&files);
    filevec_free(&files);
    return 0;
}
