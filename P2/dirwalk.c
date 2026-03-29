#include "dirwalk.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void filevec_init(FileVector *v) {
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

int filevec_push(FileVector *v, const char *path) {
    if (v->len == v->cap) {
        size_t new_cap = (v->cap == 0) ? 8 : v->cap * 2;
        char **next = realloc(v->items, new_cap * sizeof(char *));
        if (next == NULL) {
            return 0;
        }
        v->items = next;
        v->cap = new_cap;
    }

    v->items[v->len] = malloc(strlen(path) + 1);
    if (v->items[v->len] == NULL) {
        return 0;
    }

    strcpy(v->items[v->len], path);
    v->len++;
    return 1;
}

void filevec_free(FileVector *v) {
    for (size_t i = 0; i < v->len; i++) {
        free(v->items[i]);
    }
    free(v->items);
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

static int has_txt_suffix(const char *name) {
    size_t n = strlen(name);
    return n >= 4 && strcmp(name + n - 4, ".txt") == 0;
}

static int collect_recursive_impl(const char *root, FileVector *out) {
    DIR *dir = opendir(root);
    if (dir == NULL) {
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t needed = strlen(root) + 1 + strlen(entry->d_name) + 1;
        char *full = malloc(needed);
        if (full == NULL) {
            closedir(dir);
            return 0;
        }
        snprintf(full, needed, "%s/%s", root, entry->d_name);

        struct stat st;
        if (stat(full, &st) != 0) {
            free(full);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            int ok = collect_recursive_impl(full, out);
            free(full);
            if (!ok) {
                closedir(dir);
                return 0;
            }
        } else {
            if (has_txt_suffix(entry->d_name) && !filevec_push(out, full)) {
                free(full);
                closedir(dir);
                return 0;
            }
            free(full);
        }
    }

    closedir(dir);
    return 1;
}

int collect_text_files_recursive(const char *root, FileVector *out) {
    return collect_recursive_impl(root, out);
}
