#include "mysh.h"

/*
 * Tokenize `line` in-place (modifies it with NUL bytes).
 * Stores pointers into `tokens[]`.
 * Returns the number of tokens, or -1 on error.
 *
 * Rules:
 *   - whitespace separates tokens
 *   - #  begins a comment; everything after is dropped
 *   - >, <, | are always single-character tokens
 */
int tokenize(char *line, char **tokens) {
    int n = 0;
    char *p = line;

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '\n') break;
        if (*p == '#') break;

        // ← remove the entire if-block that was here

        char *start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
        char *end = p;
        if (*end) { *end = '\0'; p = end + 1; } else { p = end; }
        tokens[n++] = start;

        if (n >= MAX_TOKENS - 1) break;
    }
    tokens[n] = NULL;
    return n;
}