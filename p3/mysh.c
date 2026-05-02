#include "mysh.h"

extern int find_in_path(const char *name, char *out, size_t outsz);
extern int is_builtin(const char *name);

/* ------------------------------------------------------------------ */
/*  Parser: tokens[] -> Job                                            */
/* ------------------------------------------------------------------ */

int parse_job(char **tokens, int ntok, Job *job) {
    memset(job, 0, sizeof(*job));
    job->num_cmds = 1;
    int ci = 0;
    Command *cur = &job->cmds[ci];
    cur->argc = 0;

    for (int i = 0; i < ntok; i++) {
        char *t = tokens[i];

        if (strcmp(t, "|") == 0) {
            cur->args[cur->argc] = NULL;
            ci++;
            if (ci >= MAX_ARGS) { write(STDERR_FILENO,"syntax error\n",13); return 1; }
            job->num_cmds++;
            cur = &job->cmds[ci];
            cur->argc = 0;
        }
        else if (strcmp(t, "<") == 0) {
            i++;
            if (i >= ntok) { write(STDERR_FILENO,"syntax error: expected filename after <\n",40); return 1; }
            if (strcmp(tokens[i],"<")==0 || strcmp(tokens[i],">")==0) {
                write(STDERR_FILENO,"syntax error\n",13); return 1;
            }
            cur->input_file = tokens[i];
        }
        else if (strcmp(t, ">") == 0) {
            i++;
            if (i >= ntok) { write(STDERR_FILENO,"syntax error: expected filename after >\n",40); return 1; }
            if (strcmp(tokens[i],"<")==0 || strcmp(tokens[i],">")==0) {
                write(STDERR_FILENO,"syntax error\n",13); return 1;
            }
            cur->output_file = tokens[i];
        }
        else {
            cur->args[cur->argc++] = t;
        }
    }
    cur->args[cur->argc] = NULL;

    for (int i = 0; i < job->num_cmds; i++) {
        if (job->cmds[i].argc == 0) {
            write(STDERR_FILENO,"syntax error: empty command\n",28);
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Prompt                                                             */
/* ------------------------------------------------------------------ */

static void print_prompt(void) {
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        write(STDOUT_FILENO, "$ ", 2);
        return;
    }
    const char *home = getenv("HOME");
    char display[4096];
    if (home && strncmp(cwd, home, strlen(home)) == 0) {
        snprintf(display, sizeof(display), "~%s", cwd + strlen(home));
    } else {
        strncpy(display, cwd, sizeof(display));
    }
    write(STDOUT_FILENO, display, strlen(display));
    write(STDOUT_FILENO, "$ ", 2);
}

/* ------------------------------------------------------------------ */
/*  Main read loop                                                     */
/* ------------------------------------------------------------------ */

static int read_line(int fd, char *buf, int maxlen) {
    int n = 0;
    char c;
    while (n < maxlen - 1) {
        int r = read(fd, &c, 1);
        if (r <= 0) {
            if (n == 0) return -1;
            break;
        }
        buf[n++] = c;
        if (c == '\n') break;
    }
    buf[n] = '\0';
    return n;
}

int main(int argc, char *argv[]) {
    int fd = STDIN_FILENO;
    int from_file = 0;

    if (argc > 2) {
        write(STDERR_FILENO, "Usage: mysh [script]\n", 21);
        exit(EXIT_FAILURE);
    }
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
        from_file = 1;
    }

    int interactive = !from_file && isatty(fd);

    if (interactive) {
        write(STDOUT_FILENO, "Welcome to mysh!\n", 17);
    }

    char line[BUF_SIZE];
    int last_status = 0;

    while (1) {
        if (interactive) {
            print_prompt();
        }

        int r = read_line(fd, line, BUF_SIZE);
        if (r < 0) break;

        /* ---- split on ';' and run each segment ---- */
        char *seg = line;
        while (seg && *seg) {
            char *semi = strchr(seg, ';');
            char segment[BUF_SIZE];
            if (semi) {
                size_t len = semi - seg;
                strncpy(segment, seg, len);
                segment[len] = '\0';
                seg = semi + 1;
            } else {
                strncpy(segment, seg, BUF_SIZE);
                seg = NULL;
            }

            char *tokens[MAX_TOKENS];
            int ntok = tokenize(segment, tokens);
            if (ntok == 0) continue;

            char *expanded[MAX_TOKENS];
            int nexp = 0;
            expand_wildcards(tokens, ntok, expanded, &nexp);
            if (nexp == 0) continue;

            Job job;
            if (parse_job(expanded, nexp, &job) != 0) {
                last_status = 1;
                continue;
            }

            if (job.num_cmds == 1 && strcmp(job.cmds[0].args[0], "exit") == 0) {
                if (interactive) write(STDOUT_FILENO, "Exiting mysh.\n", 14);
                goto done;
            }

            last_status = run_job(&job, interactive);

            if (interactive && last_status != 0) {
                char msg[64];
                snprintf(msg, sizeof(msg), "Exited with status %d\n", last_status);
                write(STDOUT_FILENO, msg, strlen(msg));
            }
        }
    }

done:
    if (interactive) {
        write(STDOUT_FILENO, "Exiting mysh.\n", 14);
    }
    if (from_file) close(fd);
    exit(EXIT_SUCCESS);
}