#include "mysh.h"

static const char *SEARCH_DIRS[] = {
    "/usr/local/bin", "/usr/bin", "/bin", NULL
};

int is_builtin(const char *name) {
    return (strcmp(name,"cd")==0 || strcmp(name,"pwd")==0 ||
            strcmp(name,"which")==0 || strcmp(name,"exit")==0);
}

/*
 * Find `name` in the search path. Writes full path into `out`.
 * Returns 1 if found, 0 otherwise.
 */
int find_in_path(const char *name, char *out, size_t outsz) {
    for (int i = 0; SEARCH_DIRS[i]; i++) {
        snprintf(out, outsz, "%s/%s", SEARCH_DIRS[i], name);
        if (access(out, X_OK) == 0) return 1;
    }
    return 0;
}

/*
 * Run a built-in command.
 * Output goes to cmd->output_file if set, else fd 1.
 * Sets *should_exit = 1 if the command is `exit`.
 * Returns 0 on success, 1 on failure.
 */
int run_builtin(Command *cmd, int *should_exit) {
    *should_exit = 0;
    const char *name = cmd->args[0];

    // Determine output fd
    int out_fd = STDOUT_FILENO;
    int opened = 0;
    if (cmd->output_file) {
        out_fd = open(cmd->output_file, O_WRONLY|O_CREAT|O_TRUNC, 0640);
        if (out_fd < 0) { perror(cmd->output_file); return 1; }
        opened = 1;
    }

    int ret = 0;

    if (strcmp(name, "exit") == 0) {
        *should_exit = 1;
        // exit can also have output redirected — nothing to print
    }
    else if (strcmp(name, "pwd") == 0) {
        char buf[4096];
        if (getcwd(buf, sizeof(buf)) == NULL) { perror("pwd"); ret = 1; }
        else { write(out_fd, buf, strlen(buf)); write(out_fd, "\n", 1); }
    }
    else if (strcmp(name, "cd") == 0) {
        if (cmd->argc > 2) {
            write(STDERR_FILENO, "cd: too many arguments\n", 23);
            ret = 1;
        } else {
            const char *dest = (cmd->argc == 1) ? getenv("HOME") : cmd->args[1];
            if (chdir(dest) != 0) { perror("cd"); ret = 1; }
        }
    }
    else if (strcmp(name, "which") == 0) {
        if (cmd->argc != 2) {
            write(STDERR_FILENO, "which: wrong number of arguments\n", 33);
            ret = 1;
        } else {
            const char *prog = cmd->args[1];
            if (is_builtin(prog)) {
                // print nothing, fail
                ret = 1;
            } else {
                char path[4096];
                if (find_in_path(prog, path, sizeof(path))) {
                    write(out_fd, path, strlen(path));
                    write(out_fd, "\n", 1);
                } else {
                    ret = 1;
                }
            }
        }
    }

    if (opened) close(out_fd);
    return ret;
}