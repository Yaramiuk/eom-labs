#!/bin/bash
# Відтворює всі експерименти Lab 7 (Linux). Проміжні файли — у out/.
#   docker run --rm --init --cap-add SYS_PTRACE --security-opt seccomp=unconfined -v "$PWD":/lab -w /lab eom-lab ./run.sh
cd "$(dirname "$0")"; mkdir -p out; cd out; S=..
ex() { echo; echo "\$ $*"; eval "$@" 2>&1; }
for f in pipe_basic pipe_processes pipe_no_close dup2_output redirect_exec pipeline signal_int signal_parent_child mini_shell; do
  gcc -Wall -Wextra $S/$f.c -o $f || echo "BUILD FAIL $f"
done
echo "### 1 pipe_basic"; ex ./pipe_basic
echo "### 2 pipe_processes"; ex ./pipe_processes
echo "### 3 pipe_no_close"
stdbuf -oL ./pipe_no_close > nc.log 2>&1 & NP=$!; sleep 1.5
ex cat nc.log
CH=$(pgrep -P $NP)
ex "ps -o pid,ppid,stat,wchan:20,cmd -p $NP,$CH"
ex "ls -l /proc/$CH/fd"
ex "ls -l /proc/$NP/fd"
kill $CH; wait $NP 2>/dev/null
ex "cat nc.log"
ex "timeout 5 ./pipe_no_close close; echo \"exit: \$?\""
echo "### strace pipe"
ex "strace -f -e trace=pipe,pipe2,clone,fork,vfork,read,write,close,wait4 ./pipe_processes 2>&1 | grep -v 'ld.so\|libc.so\|ELF'"
echo "### 4 dup2"; ex ./dup2_output; ex cat redirected.txt
ex "strace -e trace=openat,dup2,dup3,close,write ./dup2_output 2>&1 | grep -v 'ld.so\|libc.so'"
echo "### 5 redirect_exec"; ex ./redirect_exec; ex "head -5 ls-output.txt"; ex "ls -l | head -5"
echo "### 6 pipeline"; ex ./pipeline; ex "ls -l | wc -l"
ex "strace -f -e trace=pipe2,clone,dup2,dup3,close,execve,wait4 ./pipeline 2>&1 | grep -v 'ld.so\|libc.so\|locale'"
echo "### 7 SIGINT (pty)"
ex "python3 $S/pty_test.py ./signal_int ^C ^C ^C"
echo "### 8 parent/child"
ex ./signal_parent_child
ex ./signal_parent_child handler
echo "### SIGKILL"
./signal_int > si.log & SP=$!; sleep 0.5
ex "kill -INT $SP"; sleep 0.3; ex "ps -o pid,stat,cmd -p $SP"
ex "kill -TERM $SP"; wait $SP; echo "wait status after SIGTERM: $? (128+15)"
./signal_int > si.log & SP=$!; sleep 0.5
ex "kill -KILL $SP"; wait $SP; echo "wait status after SIGKILL: $? (128+9)"
ex "cat si.log"
ex "grep -E 'Sig(Cgt|Ign)' /proc/self/status"
./signal_int & SP=$!; sleep 0.3; ex "grep -E 'SigCgt' /proc/$SP/status"; kill -KILL $SP; wait $SP 2>/dev/null
echo "### mini-shell (pty, Ctrl+C test)"
mkdir -p shtest && cd shtest
printf 'hello world\nfoo\nhello again\n' > file.txt
ex "python3 ../$S/pty_test.py ../mini_shell 'pwd\n' 'ls -l\n' 'echo Hello Systems\n' 'cd /tmp\n' 'pwd\n' 'cd -nosuch\n' 'cd /lab/out/shtest\n' 'echo test > test.txt\n' 'cat test.txt\n' 'echo hello > output.txt\n' 'cat output.txt\n' 'ls -l | wc -l\n' 'cat file.txt | grep hello\n' 'ls|wc -l > count.txt\n' 'cat count.txt\n' 'sleep 30\n' sleep:1 ^C 'echo still-running\n' 'sleep 30 | cat\n' sleep:1 ^C 'echo alive\n' ^C 'command_that_does_not_exist\n' 'echo x > /nonexistent/f\n' 'ls |\n' '| wc\n' 'echo hi >\n' 'a | b | c\n' '\n' 'echo still alive after errors\n' 'exit\n'"
cd ..
echo "### strace mini-shell"
printf 'echo hello > output.txt\nls | wc -l\nexit\n' | strace -f -e trace=process,pipe,pipe2,dup2,dup3,openat,close,read,write ./mini_shell > ms_out.txt 2> ms_trace.txt
ex "cat ms_out.txt"
ex "grep -vE 'ld.so|libc.so|ELF|locale|gconv|\+\+\+ exited|resumed' ms_trace.txt | grep -E 'pipe2|clone|openat\(AT_FDCWD, \"output|dup[23]|execve|close|wait4|exit_group|read\(0|write\(1'"
ex "cat output.txt"
