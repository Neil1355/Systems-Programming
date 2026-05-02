#include "dirwalk.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void filevec_init(FileVector *v) {
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

static int filevec_contains(const FileVector *v, const char *path) {
    for (size_t i = 0; i < v->len; i++) {
        if (strcmp(v->items[i], path) == 0) {
            return 1;
        }
    }
    return 0;
}

int filevec_push_unique(FileVector *v, const char *path) {
    if (filevec_contains(v, path)) {
        return 1;
    }

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

int path_has_hidden_basename(const char *path) {
    const char *base = strrchr(path, '/');
    base = (base == NULL) ? path : base + 1;
    return base[0] == '.';
}

static int has_txt_suffix(const char *name) {
    size_t n = strlen(name);
    return n >= 4 && strcmp(name + n - 4, ".txt") == 0;
}

static int join_path(const char *root, const char *entry, char **out_full) {
    size_t needed = strlen(root) + 1 + strlen(entry) + 1;
    char *full = malloc(needed);
    if (full == NULL) {
        return 0;
    }
    snprintf(full, needed, "%s/%s", root, entry);
    *out_full = full;
    return 1;
}

static int collect_recursive_impl(const char *root, FileVector *out, int *had_error) {
    DIR *dir = opendir(root);
    if (dir == NULL) {
        perror(root);
        *had_error = 1;
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        char *full = NULL;
        if (!join_path(root, entry->d_name, &full)) {
            closedir(dir);
            return 0;
        }

        struct stat st;
        if (stat(full, &st) != 0) {
            perror(full);
            *had_error = 1;
            free(full);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            int ok = collect_recursive_impl(full, out, had_error);
            free(full);
            if (!ok) {
                closedir(dir);
                return 0;
            }
        } else {
            if (S_ISREG(st.st_mode) && has_txt_suffix(entry->d_name) &&
                !filevec_push_unique(out, full)) {
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

int collect_text_files_recursive(const char *root, FileVector *out, int *had_error) {
    return collect_recursive_impl(root, out, had_error);
}
