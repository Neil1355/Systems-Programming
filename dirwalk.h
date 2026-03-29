#ifndef DIRWALK_H
#define DIRWALK_H

#include <stddef.h>

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} FileVector;

void filevec_init(FileVector *v);
int filevec_push(FileVector *v, const char *path);
void filevec_free(FileVector *v);
int collect_text_files_recursive(const char *root, FileVector *out);

#endif
