# Lab 4. Процеси, системні виклики та fork() / exec() / wait()

Робив у Docker на Linux (Debian 12, gcc 13.5, strace 6.1). Цього разу контейнер ARM64, а не x86-64, бо strace під емуляцією x86-64 в мене не запрацював. Для цієї лаби архітектура не важлива. Єдина різниця: замість fork у strace видно clone.

Другий термінал я не відкривав, а запускав програми у фоні і поки вони спали дивився ps і /proc. Крім обов'язкових файлів додав new_program.c і exec_pid.c (для перевірки PID після exec), zombie_wait.c, fork_twice.c і fork_three.c.

## Завдання 1. Program і Process

file process_info каже ELF 64-bit executable. Програма вивела PID: 96 і PPID: 7. Процес 7 це bash, з якого запускався скрипт.

- До запуску executable просто лежить на диску.
- PID це номер процесу, який дає йому ядро.
- PID не записаний у файлі, бо він з'являється тільки при запуску, і кожен запуск отримує свій.
- PPID це PID батьківського процесу.
- Батько моєї програми це shell (bash).

## Завдання 2. Кілька процесів

| Instance | PID | PPID |
|---|---|---|
| 1 | 96 | 7 |
| 2 | 97 | 7 |
| 3 | 98 | 7 |

Один файл, але три окремі процеси, бо кожен запуск створює новий процес. PPID однаковий, бо всі запускав один і той самий bash.

Program це файл з кодом, а process це запущена програма зі своїм PID і пам'яттю.

## Завдання 3. /proc

| Що | Результат |
|---|---|
| Name | process_info |
| PID | 96 |
| PPID | 7 |
| State | S (sleeping) |
| Executable | /lab/out/process_info |

cmdline показав ./process_info. У maps видно код програми, heap, libc і stack.

- /proc/PID це не сам процес, а файли, через які ядро показує інформацію про нього.
- Коли процес завершився, папка /proc/96 зникла (No such file or directory).
- /proc зручний, бо можна просто прочитати файли і побачити стан процесу.

## Завдання 4. System calls

hello вивела Hello from process!. strace показав 32 системні виклики, більшість від завантаження libc. Сам printf дав один рядок:

```
write(1, "Hello from process!\n", 20) = 20
```

Тут 1 це stdout, далі буфер з текстом, 20 це скільки байтів записати, і повернуло 20, тобто стільки й записалось. Шлях такий: printf у libc, потім write, потім системний виклик, і ядро вже виводить у stdout.

## Завдання 5. printf і write

strace -e write ./output показав два write: один від printf і один мій. Цікаво, що якщо вивід іде в pipe або файл, то першим іде рядок від write, а потім від printf. Коли я запустив через script (тобто з терміналом), порядок став нормальний.

Як я зрозумів, у терміналі printf скидає буфер після кожного \n, а коли вивід у файл чи pipe, він тримає все в буфері до кінця програми. write ж іде в ядро одразу.

printf це функція бібліотеки, яка форматує текст і буферизує. write це майже прямий системний виклик. Тому один printf не обов'язково дорівнює одному write.

## Завдання 6. fork

| | Значення |
|---|---|
| Parent PID | 142 |
| Child PID | 143 |
| Child PPID | 142 |

- До fork був один процес.
- Після fork їх два.
- Parent отримав від fork 143 (PID дитини).
- Child отримав 0.
- fork викликається один раз, а повертається двічі, в обох процесах. Обидва продовжують з того самого місця, тільки pid у них різний, тому if розводить їх по різних гілках.

## Порядок виконання

П'ять разів підряд першим друкував PARENT. Потім я запустив 200 разів: PARENT був першим усі 200 разів, але у 8 запусках child показав PPID=1. Це означає, що parent встиг завершитись раніше, ніж child викликав getppid, і child перейшов до init. В одному з перших прогонів CHILD від попереднього запуску взагалі надрукувався вже після наступної команди.

Тому порядок гарантувати не можна, його вирішує scheduler.

## Завдання 7. fork і пам'ять

Мій прогноз: child не побачить 300, бо в кожного своя копія.

| | value | address |
|---|---|---|
| до fork | 100 | 0x420050 |
| parent | 300 | 0x420050 |
| child | 200 | 0x420050 |

Прогноз підтвердився. Адреса однакова, але це віртуальна адреса, а в кожного процесу своя пам'ять. Як я зрозумів, після fork сторінки спочатку спільні тільки для читання, а коли хтось пише, ядро робить окрему копію (copy-on-write).

## Завдання 8. fork через strace

```
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, ...) = 158
[pid   157] exit_group(0)               = ?
CHILD:  PID=158, PPID=1, fork returned=0
exit_group(0)                           = ?
```

fork у strace виглядає як clone, і він повертає PID дитини 158. Завершення видно як exit_group. Тут parent знову вийшов першим, тому у child PPID=1.

## Завдання 9. wait

wait_basic вивела CHILD: working..., CHILD: finished, PARENT: waiting for child 160, PARENT: child finished і exit status = 42. Працювало 2 секунди. Рядки child вийшли першими, бо вивід ішов у файл і буфер parent ще не скинувся.

1. Під час wait parent просто спить і чекає, поки child завершиться.
2. PARENT: child finished не може з'явитись раніше, бо він стоїть після wait.
3. 42 це код завершення child (return 42).
4. 42 це не PID, PID був 160.
5. WIFEXITED перевіряє, що child завершився нормально, а WEXITSTATUS дістає з status сам код.

## Завдання 10. exec

exec_basic вивела Before exec: PID=161 і Hello from exec!. perror не виконався, бо після успішного exec старого коду вже немає.

Перевірка PID: exec_pid запускає new_program.

| | PID |
|---|---|
| Before exec | 162 |
| After exec | 162 |

Новий процес не створюється, програма нова, PID той самий.

## Завдання 11. fork + exec

fork_exec вивела PARENT: PID=167, CHILD before exec: PID=168, Program started by child, PARENT: created child PID=168, PARENT: child completed.

У strace видно clone, потім wait4 у parent, а в child кілька execve, поки execlp шукає echo в PATH (/usr/local/sbin, /usr/local/bin, /usr/sbin, і тільки /usr/bin спрацював). Потім exit_group у child, wait4 повертає 168 і приходить SIGCHLD.

- fork створює новий процес, копію батька.
- exec у child замінює програму на echo, PID не змінюється.
- wait змушує parent дочекатися, поки child закінчить.

## Завдання 12. launcher

| Команда | Результат |
|---|---|
| ./launcher date | дата, Child exit status: 0 |
| ./launcher whoami | root, статус 0 |
| ./launcher uname | Linux, статус 0 |
| ./launcher nosuchprog | execlp: No such file or directory, статус 1 |
| ./launcher | Usage |

По суті це маленький shell: отримав назву, fork, exec у дитині, wait у батьку.

## Завдання 13. Стани процесу

states спав у sleep(60). /proc показав State: S (sleeping), ps показав TIME 00:00:00, а wchan hrtimer_nanosleep. utime і stime 0.

Під час sleep процес CPU не використовує, він не running, а sleeping. ОС не дає йому CPU, бо йому нічого робити, поки не спрацює таймер. CPU в цей час працює з іншими процесами.

## Завдання 14. Zombie

zombie: parent 1397, child 1399. ps показав:

| PID | PPID | State | CMD |
|---|---|---|---|
| 1397 | 7 | S | ./zombie |
| 1399 | 1397 | Z | [zombie] <defunct> |

/proc/1399/status теж показав State: Z (zombie). Child вже завершився, але parent не зробив wait, тому запис про child залишився. Коли я вбив parent, zombie зник.

З wait(NULL) (zombie_wait.c) child у ps взагалі не з'явився, поки parent спав, бо статус вже забрали.

## Завдання 15. Кількість процесів

Прогноз для двох fork: 4 printf. Вийшло 4 рядки з PID 209-212. Дерево: перший процес робить fork і з'являється другий, потім обидва роблять другий fork, тож разом 4.

Для трьох fork прогноз 8, вийшло 8. У частини рядків PPID=1, бо їхні батьки вже завершились. Тобто кількість росте як 2^n.

## Фінальне завдання process_runner

Програма показує PID батька, робить fork, у child показує PID і PPID і запускає програму через execvp (можна з аргументами). Parent чекає через waitpid і пише, як child завершився. Якщо exec не вдався, child виходить з кодом 127.

| Команда | Результат |
|---|---|
| ./process_runner date | Exit status: 0 |
| ./process_runner ls -l файл | вивід ls, статус 0 |
| ./process_runner false | Exit status: 1 |
| ./process_runner nosuchprog | помилка exec, Exit status: 127 |
| ./process_runner sh -c 'kill -TERM $$' | Terminated by signal 15 |
| ./process_runner | Usage |

Три фрагменти зі strace:

```
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0xffffabca7050) = 251
```

```
[pid   251] execve("/usr/bin/date", ["date"], ...) = 0
```

```
[pid   250] wait4(251,  <unfinished ...>
[pid   251] exit_group(0)               = ?
<... wait4 resumed>[{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 251
```

Перший це створення процесу (fork), другий запуск нової програми (exec), третій очікування і завершення (waitpid і exit).

## Контрольні питання

1. Program це файл, process це запущена програма.
2. PID це номер процесу.
3. PPID це PID батька.
4. Один executable може мати кілька PID, бо кожен запуск створює новий процес.
5. /proc/PID показує інформацію про процес.
6. System call це звернення програми до ядра, наприклад write або fork.
7. Звичайна функція виконується в самій програмі, а system call переходить у ядро, і це дорожче.
8. strace показує, які системні виклики робить програма.
9. fork створює копію процесу.
10. Після fork два процеси.
11. fork повертає 0 у child, щоб child знав, що він child.
12. Parent отримує PID child.
13. Ні, у кожного своя копія value.
14. Адреса однакова, бо вона віртуальна, а фізична пам'ять у кожного своя.
15. exec замінює програму в процесі на іншу.
16. exec новий процес не створює.
17. PID після exec залишається той самий.
18. wait потрібен, щоб дочекатися child і забрати його статус.
19. Exit status це число, з яким завершилась програма, 0 зазвичай означає успіх.
20. Zombie це процес, який завершився, але батько ще не забрав його статус.
21. Zombie нічого не виконує.
22. Running зараз виконується, ready чекає своєї черги на CPU, waiting чекає подію (таймер, введення і т.д.).
23. Порядок parent і child не гарантований, бо це два окремі процеси і їх планує scheduler.
24. Shell якраз так і запускає команди: fork, exec у дитині, wait у батьку.

## Висновок

Я побачив, що один файл може дати кілька процесів з різними PID, і що через /proc видно їхній стан. strace показав, що printf перетворюється на системний виклик write. fork створює копію процесу з окремою пам'яттю, exec міняє програму без зміни PID, а wait дає дочекатися child і отримати статус. Без wait виходить zombie. Порядок виконання parent і child передбачити не можна.
