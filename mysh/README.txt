nnb35
hva6

== Test Plan ==

1. Basic execution
   - Run `echo hello` — expect "hello"
   - Run a nonexistent command — expect "command not found"

2. Built-ins
   - `pwd`            — prints working dir
   - `cd /tmp; pwd`   — shows /tmp
   - `cd`             — returns to HOME
   - `which echo`     — prints /bin/echo
   - `which cd`       — prints nothing, fails
   - `exit`           — shell exits cleanly

3. Redirection
   - `echo hi > out.txt; cat out.txt`
   - `cat < out.txt`
   - `echo a > f; echo b > f; cat f`  (truncation)

4. Pipes
   - `echo hello | cat`
   - `cat /etc/passwd | grep root | cat`

5. Wildcards
   - `echo *.c`    — lists .c files
   - `echo *.xyz`  — no match, passes *.xyz through
   - `echo .*`     — should NOT match hidden files if pat starts with *

6. Batch mode
   - `echo hello | ./mysh`       — no prompt, prints hello
   - `./mysh myscript.sh`        — runs script

7. Interactive mode
   - Run `./mysh` from terminal
   - Check welcome/goodbye messages
   - Check prompt shows ~ for home dir
   - Run a failing command, confirm "Exited with status N"
   - Kill a child with Ctrl-C, confirm "Terminated by signal..."

8. Edge cases
   - Empty line         — no crash
   - Comment-only line  — no crash
   - `< <`              — syntax error, continue
   - `|` at start       — syntax error, continue