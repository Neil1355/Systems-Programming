#include "mysh.h"

extern int find_in_path(const char *name, char *out, size_t outsz);
extern int is_builtin(const char *name);
extern int run_builtin(Command *cmd, int *should_exit);

static const char *SEARCH_DIRS[] = {
    "/usr/local/bin", "/usr/bin", "/bin", NULL
};

/*
 * Resolve the executable path for args[0].
 * Returns 1 and fills `out` on success, 0 on failure.
 */
static int resolve_path(const char *name, char *out, size_t outsz) {
    if (strchr(name, '/')) {
        strncpy(out, name, outsz);
        return access(out, X_OK) == 0;
    }
    return find_in_path(name, out, outsz);
}

/*
 * Run an entire job (pipeline of >= 1 commands).
 * `interactive`: 1 if running in interactive mode.
 * Returns 0 if the last command succeeds, 1 otherwise.
 */
int run_job(Job *job, int interactive) {
    int n = job->num_cmds;

    // For each adjacent pair we need a pipe
    // pipes[i] connects cmd[i] -> cmd[i+1]
    int pipes[MAX_ARGS][2];
    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return 1;
        }
    }

    pid_t pids[MAX_ARGS];

    for (int i = 0; i < n; i++) {
        Command *cmd = &job->cmds[i];

        // Built-ins run in the shell process (unless in a pipeline)
        // If n==1 and it's a builtin, run directly
        if (n == 1 && is_builtin(cmd->args[0])) {
            int should_exit = 0;
            int ret = run_builtin(cmd, &should_exit);
            if (should_exit) exit(EXIT_SUCCESS);
            return ret;
        }

        // For builtins inside a pipeline: fork and run builtin in child
        char exe_path[4096];
        int  is_bin = is_builtin(cmd->args[0]);

        if (!is_bin && !resolve_path(cmd->args[0], exe_path, sizeof(exe_path))) {
            fprintf(stderr, "%s: command not found\n", cmd->args[0]);
            // close all pipes we opened
            for (int k = 0; k < n-1; k++) { close(pipes[k][0]); close(pipes[k][1]); }
            return 1;
        }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }

        if (pid == 0) {
            // --- child ---

            // Hook up pipeline ends
            if (i > 0) {       // read from previous pipe
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if (i < n - 1) {   // write to next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipe fds in child
            for (int k = 0; k < n-1; k++) {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }

            // Input redirection (only for first in pipeline per spec)
            if (cmd->input_file) {
                int fd = open(cmd->input_file, O_RDONLY);
                if (fd < 0) { perror(cmd->input_file); exit(1); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            } else if (!interactive && i == 0) {
                // batch mode: default stdin is /dev/null
                int fd = open("/dev/null", O_RDONLY);
                if (fd >= 0) { dup2(fd, STDIN_FILENO); close(fd); }
            }

            // Output redirection (only for last in pipeline per spec)
            if (cmd->output_file) {
                int fd = open(cmd->output_file, O_WRONLY|O_CREAT|O_TRUNC, 0640);
                if (fd < 0) { perror(cmd->output_file); exit(1); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            if (is_bin) {
                int should_exit = 0;
                int ret = run_builtin(cmd, &should_exit);
                exit(ret);
            }

            execv(exe_path, cmd->args);
            perror(exe_path);
            exit(1);
        }

        pids[i] = pid;
    }

    // Parent: close all pipe fds
    for (int k = 0; k < n-1; k++) {
        close(pipes[k][0]);
        close(pipes[k][1]);
    }

    // Wait for all children; track last exit status
    int last_status = 0;
    for (int i = 0; i < n; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == n-1) last_status = status;
    }

    if (WIFEXITED(last_status))   return WEXITSTATUS(last_status);
    if (WIFSIGNALED(last_status)) return 1;
    return 1;
}