#!/bin/bash
# Відтворює всі експерименти Lab 6 (Linux). Проміжні файли — у out/.
#   docker run --rm --init --cap-add SYS_PTRACE --security-opt seccomp=unconfined -v "$PWD":/lab -w /lab eom-lab ./run.sh
cd "$(dirname "$0")"; mkdir -p out; cd out; S=..
ex() { echo; echo "\$ $*"; eval "$@" 2>&1; }
for f in memory_layout memory_maps page_size heap_growth invalid_memory standard_fds fd_observer open_file write_file read_file file_offset streams buffering file_copy; do
  gcc -Wall -Wextra $S/$f.c -o $f || echo "BUILD FAIL $f"
done
gcc -Wall -Wextra -DUSE_LSEEK $S/file_offset.c -o file_offset_lseek
gcc -Wall -Wextra -DUSE_FFLUSH $S/buffering.c -o buffering_fflush
ex uname -srm
ex getconf PAGE_SIZE
echo "### 1 layout"; for i in 1 2 3; do ex ./memory_layout; done
echo "### 2 maps"
stdbuf -oL ./memory_maps > mm.log & MP=$!; sleep 1
ex cat mm.log
ex "cat /proc/$MP/maps"
kill $MP
echo "### 3 page"; ex ./page_size
echo "### 4 heap growth"
rm -f in.fifo; mkfifo in.fifo
stdbuf -oL ./heap_growth < in.fifo > hg.log & HP=$!
exec 3> in.fifo; sleep 1
ex cat hg.log
ex "grep -E '^(VmSize|VmRSS|RssAnon)' /proc/$HP/status"
ex "grep -B1 -A3 \"^\$(printf '%x' 0)\" /dev/null; awk '\$6==\"\" && \$2 ~ /rw/' /proc/$HP/maps | head -5"
echo >&3; sleep 1
ex cat hg.log
ex "grep -E '^(VmSize|VmRSS|RssAnon)' /proc/$HP/status"
echo >&3; exec 3>&-; wait $HP
echo "### 5 invalid"
ex "./invalid_memory; echo \"exit status: \$?\""
ex "strace ./invalid_memory 2>&1 | tail -6"
echo "### 7 fds"
ex ./standard_fds
stdbuf -oL ./fd_observer > fo.log & FP=$!; sleep 0.5
ex cat fo.log; ex "ls -l /proc/$FP/fd"; kill $FP
script -qc "./fd_observer & sleep 0.5; ls -l /proc/\$!/fd; kill \$!" /dev/null > fo_tty.log
echo; echo "\$ (у pseudo-terminal) ls -l /proc/PID/fd"; cat fo_tty.log | tr -d '\r'
echo "### 8 open"
stdbuf -oL ./open_file > of.log & OP=$!; sleep 0.5
ex cat of.log; ex "ls -l /proc/$OP/fd"; kill $OP
echo "### 9 write"; ex ./write_file; ex cat output.txt; ex "od -c output.txt | head -3"
echo "### 10 read"
printf 'Computer Systems\nOperating Systems\n' > input.txt
ex ./read_file; ex wc -c input.txt
echo "### 11-12 offset"; ex ./file_offset; ex ./file_offset_lseek
echo "### 13 strace read"; ex strace -e trace=openat,read,write,close ./read_file
echo "### 14 streams"
ex ./streams
ex "./streams > normal.txt"; ex cat normal.txt
ex "./streams 2> errors.txt"; ex cat errors.txt
ex "strace -e trace=write ./streams > normal.txt"
echo "### 15-16 buffering"
ex "./buffering; echo"
ex "strace -e write ./buffering; echo"
ex "script -qc 'strace -e write ./buffering' /dev/null | tr -d '\r'; echo"
ex "./buffering_fflush; echo"
ex "strace -e write ./buffering_fflush; echo"
echo "### final file_copy"
printf 'Operating Systems\nComputer Architecture\n' > source.txt
ex ./file_copy source.txt destination.txt
ex "cmp source.txt destination.txt; echo \"cmp status: \$?\""
ex cat destination.txt
ex ./file_copy file_copy copy_of_file_copy
ex "cmp file_copy copy_of_file_copy; echo \"cmp status: \$?\""
ex "ls -l file_copy copy_of_file_copy"
head -c 10000000 /dev/urandom > random.bin
ex ./file_copy random.bin random_copy.bin
ex "cmp random.bin random_copy.bin; echo \"cmp status: \$?\"; sha256sum random.bin random_copy.bin"
ex "strace -c -e trace=read,write ./file_copy random.bin random_copy.bin 2>&1 | tail -6"
: > empty.txt; ex ./file_copy empty.txt empty_copy.txt; ex "cmp empty.txt empty_copy.txt; echo \"cmp status: \$?\""
echo "--- errors"
ex "./file_copy; echo \"exit: \$?\""
ex "./file_copy only_one; echo \"exit: \$?\""
ex "./file_copy nosuchfile.txt x.txt; echo \"exit: \$?\""
ex "./file_copy source.txt /nonexistent_dir/x.txt; echo \"exit: \$?\""
ex "./file_copy . x.txt; echo \"exit: \$?\""
ex "./file_copy source.txt /dev/full; echo \"exit: \$?\""
ex "strace -e trace=openat,close ./file_copy source.txt /nonexistent_dir/x.txt 2>&1 | grep -v 'ld.so\|libc'"
echo "--- strace"
ex "strace -e trace=openat,read,write,close ./file_copy source.txt destination.txt 2>&1 | grep -v 'ld.so\|libc.so'"
