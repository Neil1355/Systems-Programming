#ifndef MYSH_H
#define MYSH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

#define MAX_TOKENS 256
#define MAX_ARGS   256
#define BUF_SIZE   4096

// A single sub-command (one segment of a pipeline)
typedef struct {
    char *args[MAX_ARGS];  // NULL-terminated argument list
    int   argc;
    char *input_file;      // NULL if none
    char *output_file;     // NULL if none
} Command;

// A job is a pipeline of 1..N commands
typedef struct {
    Command cmds[MAX_ARGS];
    int     num_cmds;
} Job;

// tokenizer.c
int  tokenize(char *line, char **tokens);

// wildcard.c
int  expand_wildcards(char **tokens, int ntok, char **out, int *nout);

// builtins.c
int  is_builtin(const char *name);
int  run_builtin(Command *cmd, int *should_exit);

// executor.c
int  run_job(Job *job, int interactive);

// parser (in mysh.c)
int  parse_job(char **tokens, int ntok, Job *job);

#endif