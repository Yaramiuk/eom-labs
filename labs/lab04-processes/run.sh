#!/bin/bash
# Відтворює всі експерименти Lab 4 у Linux. Результати — у out/.
# Запуск у Docker (strace потребує ptrace):
#   docker run --rm --init --cap-add SYS_PTRACE --security-opt seccomp=unconfined -v "$PWD":/lab -w /lab eom-lab ./run.sh
cd "$(dirname "$0")"; mkdir -p out; cd out; S=..
ex() { echo; echo "\$ $*"; eval "$@" 2>&1; }
for f in process_info hello output fork_basic fork_memory wait_basic exec_basic new_program exec_pid fork_exec launcher states zombie zombie_wait fork_twice fork_three process_runner; do
  gcc -Wall -Wextra $S/$f.c -o $f || echo "BUILD FAIL $f"
done
echo "### 1-2 process_info x3"
ex file process_info
for i in 1 2 3; do stdbuf -oL ./process_info > pi_$i.log & eval P$i=$!; done
sleep 1
ex "ps -o pid,ppid,stat,cmd -p $P1,$P2,$P3"
ex "ps -o pid,ppid,cmd -p $$"
echo "### 3 /proc"
ex "ls /proc/$P1 | tr '\n' ' '"; echo
ex "tr '\0' ' ' < /proc/$P1/cmdline; echo"
ex "grep -E '^(Name|State|Pid|PPid|Threads|VmRSS)' /proc/$P1/status"
ex "readlink /proc/$P1/exe"
ex "head -n 20 /proc/$P1/maps"
kill $P1 $P2 $P3; wait $P1 $P2 $P3 2>/dev/null
ex "ls /proc/$P1 2>&1 | head -1"
ex "cat pi_1.log pi_2.log pi_3.log"
echo "### 4 syscalls"
ex ./hello
ex "strace ./hello 2>&1 | head -40"
ex "strace -c ./hello 2>&1 | tail -25"
ex strace -e write ./hello
echo "### 5 printf vs write"
ex strace -e write ./output
ex "strace -e write ./output > /dev/null"
ex "./output | cat"
ex "script -qc \"strace -e write ./output\" /dev/null"
echo "### 6 fork"
for i in 1 2 3 4 5; do ex ./fork_basic; done
echo "order stats (200 runs, stdout=pipe):"
for i in $(seq 200); do ./fork_basic | sed -n 2p | cut -c1-5; done | sort | uniq -c
for i in $(seq 200); do ./fork_basic; done | grep -c "PPID=1,"
echo "### 7 fork memory"
ex ./fork_memory
echo "### 8 strace fork"
ex strace -f -e trace=process ./fork_basic
echo "### 9 wait"
ex "time ./wait_basic"
echo "### 10 exec"
ex ./exec_basic
ex ./exec_pid
ex strace -e trace=execve ./exec_pid
echo "### 11 fork_exec"
ex ./fork_exec
ex strace -f -e trace=process ./fork_exec
echo "### 12 launcher"
ex ./launcher date; ex ./launcher whoami; ex ./launcher uname; ex ./launcher nosuchprog; ex ./launcher
echo "### 13 states"
./states > st.log & SP=$!; sleep 1
ex cat st.log
ex "grep State /proc/$SP/status"
ex "ps -o pid,ppid,state,time,cmd -p $SP"
ex "cat /proc/$SP/wchan; echo"
ex "awk '{print \"utime=\"\$14, \"stime=\"\$15}' /proc/$SP/stat"
kill $SP
echo "### 14 zombie"
stdbuf -oL ./zombie > z.log & ZP=$!; sleep 1
ex cat z.log
CP=$(grep -o 'child=[0-9]*' z.log | cut -d= -f2)
ex "ps -o pid,ppid,state,cmd -p $ZP,$CP"
ex "grep -E '^(Name|State|Pid|PPid)' /proc/$CP/status"
kill $ZP; sleep 0.5
ex "ps -o pid,state,cmd -p $CP || echo 'child gone after parent exit'"
stdbuf -oL ./zombie_wait > zw.log & ZW=$!; sleep 1
ex cat zw.log
CW=$(grep -o 'child=[0-9]*' zw.log | cut -d= -f2)
ex "ps -o pid,ppid,state,cmd -p $ZW,$CW"
kill $ZW
echo "### 15 forks"
ex ./fork_twice
ex "./fork_twice | wc -l"
ex "./fork_three | sort"
ex "./fork_three | wc -l"
echo "### final"
ex ./process_runner date
ex ./process_runner ls -l /lab/process_runner.c
ex ./process_runner false
ex ./process_runner nosuchprog
ex "./process_runner sh -c 'kill -TERM \$\$'"
ex ./process_runner
ex strace -f -e trace=process ./process_runner date
