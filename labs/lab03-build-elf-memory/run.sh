#!/bin/bash
# Відтворює всі експерименти Lab 3 (x86-64 Linux, GCC). Результати — у out/.
cd "$(dirname "$0")"; mkdir -p out; cd out
S=..
ex() { echo; echo "\$ $*"; eval "$@" 2>&1; }
ex gcc -Wall -Wextra $S/program.c -o program
ex ./program
echo "### 1 preprocessing"
ex gcc -E $S/program.c -o program.i
ex file program.i
ex wc -l $S/program.c program.i
ex 'head -n 12 program.i'
ex 'grep -n "int main" program.i'
ex 'grep -n "printf (const" program.i'
echo "### 2 compilation"
ex gcc -S -O0 -masm=intel program.i -o program.s
ex file program.s
ex "grep -vE '^\s*\.(cfi|file|ident|section|size|type|text|globl|p2align)' program.s"
echo "### 3 object"
ex gcc -c program.s -o program.o
ex file program.o
ex 'head -c 64 program.o | od -c | head -4'
ex objdump -d -M intel program.o
echo "### 4 symbols"
ex nm program.o
echo "### 5 link"
ex gcc program.o -o program
ex file program.o
ex file program
ex ./program
echo "### 6 linker error"
ex gcc -c $S/main.c -o main.o
ex gcc main.o -o app
ex gcc -c $S/calculate.c -o calculate.o
ex nm main.o
ex nm calculate.o
ex gcc main.o calculate.o -o app
ex ./app
echo "### 7 ELF header"
ex readelf -h program
echo "### 8 sections"
ex readelf -S -W program
ex "nm program | grep -E ' (main|add|global_value|global_empty)$'"
echo "### 9 sections.c"
ex gcc -Wall -Wextra $S/sections.c -o sections
ex ./sections
ex nm sections
ex "readelf -S -W sections | grep -E '\.(text|rodata|data|bss) '"
ex objdump -s -j .data sections
ex objdump -s -j .rodata sections
ex "readelf -s -W sections | grep -E 'initialized|uninitialized|message'"
echo "### 10 size"
ex size sections
ex ls -l sections
sed 's/^int uninitialized;/int large_array[100000];/; s/           uninitialized,/           large_array[0],/' $S/sections.c > sections_big.c
ex gcc -Wall -Wextra sections_big.c -o sections_big
ex size sections_big
ex ls -l sections sections_big
ex "readelf -S -W sections_big | grep -E '\.bss '"
ex "readelf -l -W sections_big | grep -E 'LOAD'"
echo "### 11 ldd"
ex ldd program
ex 'nm program | grep -i printf'
ex 'ls -l $(gcc -print-file-name=libc.so.6)'
echo "### 12 process"
gcc -Wall -Wextra $S/process.c -o process
./process > process.log & PID=$!
sleep 1; ex cat process.log; ex ps -p $PID -o pid,ppid,stat,etime,cmd
ex "cat /proc/$PID/status | grep -E '^(Name|State|Pid|PPid|VmSize|VmRSS)'"
ex "head -n 8 /proc/$PID/maps"
kill $PID
echo "### 13-14 memory"
ex gcc -Wall -Wextra -O0 $S/memory.c -o memory
for i in 1 2 3; do ex ./memory; done
ex gcc -Wall -Wextra -O0 -fPIE -pie $S/memory.c -o memory_pie
for i in 1 2 3; do ex ./memory_pie; done
ex cat /proc/sys/kernel/randomize_va_space
echo "### 15 stack"
ex gcc -Wall -Wextra -O0 $S/stack.c -o stack
ex ./stack; ex ./stack
echo "### 16 heap"
ex gcc -Wall -Wextra -O0 $S/heap.c -o heap
ex ./heap; ex ./heap
echo "### final"
ex gcc -E $S/memory_map.c -o memory_map.i
ex gcc -S -O0 -masm=intel memory_map.i -o memory_map.s
ex gcc -c memory_map.s -o memory_map.o
ex gcc memory_map.o -o memory_map
for f in $S/memory_map.c memory_map.i memory_map.s memory_map.o memory_map; do ex file $f; done
ex ./memory_map
ex "nm -n memory_map | grep -E ' (square|main|counter_initialized|counter_uninitialized|ratio_initialized|buffer_uninitialized)$'"
ex "readelf -S -W memory_map | grep -E '\.(text|rodata|data|bss) '"
