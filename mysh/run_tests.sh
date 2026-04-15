#!/bin/bash
SHELL=./mysh
PASS=0; FAIL=0

check() {
    local desc="$1" expected="$2"
    local actual
    actual=$(echo "$3" | $SHELL 2>&1)
    if echo "$actual" | grep -qF "$expected"; then
        echo "PASS: $desc"
        ((PASS++))
    else
        echo "FAIL: $desc"
        echo "  expected to contain: $expected"
        echo "  got:                 $actual"
        ((FAIL++))
    fi
}

check "echo"          "hello"      "echo hello"
check "pwd"           "/"          "cd /; pwd"
check "which ls"      "/bin/ls"    "which ls"
check "which cd"      ""           "which cd"
check "bad command"   "not found"  "fakecommand"
check "pipe"          "hello"      "echo hello | cat"
check "no glob match" "*.xyz"      "echo *.xyz"
check "comment"       "alive"      "# comment
echo alive"

echo ""
echo "Results: $PASS passed, $FAIL failed"