#include "mysh.h"

/*
 * Does `name` match the glob pattern `pat`?
 * Pat has the form  prefix*suffix  (exactly one asterisk).
 * Hidden files (starting with '.') are not matched by patterns
 * that start with '*'.
 */
static int match(const char *pat, const char *name) {
    const char *star = strchr(pat, '*');
    if (!star) return strcmp(pat, name) == 0;

    // hidden file rule
    if (star == pat && name[0] == '.') return 0;

    size_t prefix_len = star - pat;
    size_t suffix_len = strlen(star + 1);
    size_t name_len   = strlen(name);

    if (name_len < prefix_len + suffix_len) return 0;
    if (strncmp(pat, name, prefix_len) != 0) return 0;
    if (strcmp(name + name_len - suffix_len, star + 1) != 0) return 0;
    return 1;
}

/*
 * Expand a single glob token. Appends matching names to out[].
 * Returns number of names added, or 0 if no match (caller adds token as-is).
 * Caller owns the returned strings (heap-allocated).
 */
static int expand_one(const char *token, char **out, int max) {
    // Determine directory and pattern
    const char *slash = strrchr(token, '/');
    char dir_buf[4096];
    const char *pat;

    if (slash) {
        size_t dlen = slash - token;
        strncpy(dir_buf, token, dlen);
        dir_buf[dlen] = '\0';
        pat = slash + 1;
    } else {
        strcpy(dir_buf, ".");
        pat = token;
    }

    DIR *d = opendir(dir_buf);
    if (!d) return 0;

    int count = 0;
    struct dirent *ent;
    // collect matches, then sort
    char *matches[MAX_ARGS];
    int   nm = 0;

    while ((ent = readdir(d)) != NULL) {
        if (match(pat, ent->d_name)) {
            char full[4096];
            if (slash)
                snprintf(full, sizeof(full), "%s/%s", dir_buf, ent->d_name);
            else
                snprintf(full, sizeof(full), "%s", ent->d_name);
            matches[nm++] = strdup(full);
            if (nm >= MAX_ARGS - 1) break;
        }
    }
    closedir(d);

    // sort lexicographically
    for (int i = 0; i < nm - 1; i++)
        for (int j = i+1; j < nm; j++)
            if (strcmp(matches[i], matches[j]) > 0) {
                char *tmp = matches[i]; matches[i] = matches[j]; matches[j] = tmp;
            }

    for (int i = 0; i < nm && count < max; i++)
        out[count++] = matches[i];

    return count;
}

/*
 * Expand all wildcard tokens in `tokens[0..ntok-1]`.
 * Result goes into out[], nout is set to the count.
 * Returns 0 on success.
 */
int expand_wildcards(char **tokens, int ntok, char **out, int *nout) {
    int n = 0;
    for (int i = 0; i < ntok; i++) {
        if (strchr(tokens[i], '*')) {
            char *expanded[MAX_ARGS];
            int cnt = expand_one(tokens[i], expanded, MAX_ARGS - n);
            if (cnt == 0) {
                out[n++] = tokens[i]; // no match: pass through
            } else {
                for (int j = 0; j < cnt; j++)
                    out[n++] = expanded[j];
            }
        } else {
            out[n++] = tokens[i];
        }
        if (n >= MAX_ARGS - 1) break;
    }
    out[n] = NULL;
    *nout = n;
    return 0;
}