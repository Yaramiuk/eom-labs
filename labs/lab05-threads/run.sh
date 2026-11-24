#!/bin/bash
# Відтворює всі експерименти Lab 5 (Linux). Результати — у out/ та у виводі.
#   docker run --rm --init -v "$PWD":/lab -w /lab eom-lab ./run.sh
cd "$(dirname "$0")"; mkdir -p out; cd out; S=..
TIMEFORMAT='real %3R  user %3U  sys %3S'
ex() { echo; echo "\$ $*"; eval "$@" 2>&1; }
echo "CPUs: $(nproc)"
for f in first_thread multiple_threads shared_data thread_stacks race mutex_counter mutex_big_lock race_four mutex_four shared_heap thread_result deadlock deadlock_fixed parallel_sum sequential_sum; do
  gcc -Wall -Wextra -pthread $S/$f.c -o $f || echo "BUILD FAIL $f"
done
echo "### 1"; ex ./first_thread
echo "### 2"; for i in 1 2 3 4 5; do ex ./multiple_threads; done
echo "orderings over 500 runs:"; for i in $(seq 500); do ./multiple_threads | head -4 | cut -c8 | tr -d "\n"; echo; done | sort | uniq -c | sort -rn
echo "### 3"; for i in 1 2 3 4 5; do ex ./shared_data; done
echo "### 4"; for i in 1 2 3; do ex ./thread_stacks; done
echo "### 5 race"; for i in 1 2 3 4 5; do ex ./race; done
echo "### 6 asm"
ex "gcc -Wall -Wextra -pthread -O0 -S $S/race.c -o race_arm64.s"
ex "awk '/^increment:/,/ret/' race_arm64.s | grep -v cfi"
ex "objdump -d race | awk '/<increment>:/,/^\$/'"
echo "### 8 mutex"; for i in 1 2 3 4 5; do ex ./mutex_counter; done
echo "### 9 big lock"; ex ./mutex_big_lock
echo "### 10 time"
for p in race mutex_counter mutex_big_lock; do for i in 1 2 3; do echo -n "$p: "; { time ./$p >/dev/null; } 2>&1; done; done
echo "### 11 four"
for i in 1 2 3 4 5; do ex ./race_four; done
for i in 1 2 3 4 5; do ex ./mutex_four; done
for p in race race_four mutex_counter mutex_four; do for i in 1 2 3; do echo -n "$p: "; { time ./$p >/dev/null; } 2>&1; done; done
echo "### 12 heap"; ex ./shared_heap
echo "### 13 result"; ex ./thread_result
echo "### 15 deadlock"
ex "timeout 5 ./deadlock; echo \"exit code: \$?\""
stdbuf -oL ./deadlock > dl.log & DP=$!; sleep 2
ex cat dl.log
ex "ls /proc/$DP/task"
ex "for t in /proc/$DP/task/*; do echo \$(basename \$t) \$(grep State \$t/status) \$(cat \$t/wchan); done"
ex "ps -L -o pid,lwp,stat,wchan:20,cmd -p $DP"
kill $DP
echo "### 16 fixed"; ex "time timeout 5 ./deadlock_fixed; echo \"exit code: \$?\""
echo "### final"; ex ./parallel_sum; ex ./parallel_sum 1000003
echo "### extra timing"
for n in 1000000 1000000000; do for i in 1 2 3; do
  echo -n "sequential N=$n: "; { time ./sequential_sum $n >/dev/null; } 2>&1
  echo -n "parallel   N=$n: "; { time ./parallel_sum $n >/dev/null; } 2>&1
done; done
ex ./sequential_sum 1000000000; ex ./parallel_sum 1000000000
echo "### extra timing -O1 (чесне порівняння з однаковою якістю коду)"
gcc -O1 $S/sequential_sum.c -o sequential_sum_O1; gcc -O1 -pthread $S/parallel_sum.c -o parallel_sum_O1
for i in 1 2 3; do
  echo -n "sequential -O1 N=1e9: "; { time ./sequential_sum_O1 1000000000 >/dev/null; } 2>&1
  echo -n "parallel   -O1 N=1e9: "; { time ./parallel_sum_O1 1000000000 >/dev/null; } 2>&1
done
