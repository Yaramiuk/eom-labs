#!/bin/sh
# Відтворює всі експерименти лабораторної роботи (x86-64 Linux, GCC).
set -e
mkdir -p out
for f in arithmetic operations condition loop analyze; do
  gcc -Wall -Wextra -O0 $f.c -o out/$f
  echo "== ./$f"; ./out/$f
  gcc -Wall -Wextra -O0 -S -masm=intel $f.c -o out/${f}_intel.s
done
gcc -Wall -Wextra -O0 -S arithmetic.c -o out/arithmetic.s
gcc -O0 -S -masm=intel operations.c -o out/operations_O0.s
gcc -O2 -S -masm=intel operations.c -o out/operations_O2.s
gcc -O0 -S -masm=intel optimize.c -o out/optimize_O0.s
gcc -O2 -S -masm=intel optimize.c -o out/optimize_O2.s
objdump -d -M intel out/operations > out/operations.objdump
objdump -d -M intel out/analyze > out/analyze.objdump
# Варіанти арифметики
for op in '+' '-' '*'; do
  sed "s/int result = a + b;/int result = a $op b;/" operations.c > out/op_var.c
  gcc -O0 -S -masm=intel out/op_var.c -o "out/op_var_$(echo "$op" | tr '+*-' 'pmn').s"
done
