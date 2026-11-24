# Lab 7. Pipes, Redirection, Signals та Mini-Shell

Робив у Docker на Linux ARM64 (gcc 13.5, strace 6.1), як і попередні три лаби. Ctrl+C перевіряв у псевдотерміналі, щоб сигнал ішов як від справжньої клавіатури. Де треба було два термінали, запускав програму у фоні.

# Частина 1. Pipes

## Завдання 1. pipe_basic

Вивело Read end: 3, Write end: 4, Received: Hello through pipe!. pipefd[0] це кінець для читання, pipefd[1] для запису. Дані йдуть так: write у 4, потім pipe, потім read з 3.

- Pipe це не звичайний файл, а буфер у ядрі без імені на диску. У /proc він виглядає як pipe:[номер].
- read і write працюють, бо pipe теж має file descriptor, як і файли.
- Дескрипторів два, бо pipe працює в один бік, і кінці можна віддати різним процесам.

## Завдання 2. pipe між процесами

Вивело Child received: Message from parent.

Одразу після fork у обох процесів є fd 3 (читання) і fd 4 (запис). Потім parent закриває 3 і пише в 4, а child закриває 4 і читає з 3.

| | Після fork | Після close |
|---|---|---|
| Parent | 3 read, 4 write | 4 write |
| Child | 3 read, 4 write | 3 read |

## Завдання 3. Без close

pipe_no_close.c: child читає в циклі до EOF. Без аргументу child не закриває свій write end, з аргументом close закриває.

Без close програма зависла. Child отримав усі 21 байт, але далі нічого. ps показав, що parent у стані do_wait, а child у anon_pipe_read. У /proc/child/fd було видно 3 -> pipe і 4 -> pipe, тобто в child ще відкритий кінець для запису. А в parent pipe вже не було. Довелось вбити child.

З close все нормально: child отримав дані, read повернув 0 і обидва вийшли, exit 0.

Порожній pipe ще не означає кінець. read просто чекає. EOF буде тільки тоді, коли pipe порожній і всі кінці для запису закриті, навіть у самого читача.

## strace для pipe_processes

```
pipe2([3, 4], 0)                        = 0
clone(...) = 75
[pid    74] close(3)                    = 0
[pid    75] close(4)                    = 0
[pid    74] write(4, "Message from parent\n", 20) = 20
[pid    74] close(4)                    = 0
[pid    75] read(3, "Message from parent\n", 127) = 20
[pid    74] wait4(75, ...)
[pid    75] close(3)                    = 0
```

pipe у ядрі це pipe2, fork це clone. Потім close у обох, write у parent, read у child, wait4 у parent.

# Частина 2. Redirection

## Завдання 4. dup2

dup2_output на екран нічого не вивела, а в redirected.txt опинилось This goes to the file. strace:

```
openat(AT_FDCWD, "redirected.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 3
dup3(3, 1, 0)                           = 1
close(3)                                = 0
write(1, "This goes to the file\n", 22) = 22
```

dup2 робить так, щоб fd 1 вказував туди ж, куди fd 3 (у ядрі видно dup3).

| | fd 1 | fd 3 |
|---|---|---|
| до | термінал | redirected.txt |
| після dup2 | redirected.txt | redirected.txt |
| після close(3) | redirected.txt | закритий |

printf той самий, змінилось тільки, куди веде fd 1.

## Завдання 5. exec і перенаправлення

redirect_exec робить те саме, що ls -l > ls-output.txt: fork, у child open, dup2, close, execvp ls -l. Вивело Parent: child 84 exited with status 0. У ls-output.txt список файлів, і там є сам ls-output.txt з розміром 0. Так вийшло, бо файл створюється ще до того, як запускається ls.

Дескриптори зберігаються після exec, тому ls пише у файл і нічого про це не знає.

## Завдання 6. Pipeline

pipeline вивела 13, і ls -l | wc -l у bash теж 13.

Child 1 робить dup2(pipefd[1], 1), закриває обидва кінці і запускає ls -l. Child 2 робить dup2(pipefd[0], 0), закриває обидва і запускає wc -l. Parent закриває обидва кінці і чекає обох.

| | fd 0 | fd 1 | 3 і 4 |
|---|---|---|---|
| Child 1 (ls) | термінал | pipe write | закриті |
| Child 2 (wc) | pipe read | термінал | закриті |
| Parent | - | - | закриті |

У strace видно pipe2([3, 4]), два clone, у child 1 dup3(4, 1), у child 2 dup3(3, 0), close у всіх, потім execve ls і wc, і wait4 у parent.

Якщо parent не закриє write end, wc ніколи не отримає EOF і програма зависне, як було в pipe_no_close.

# Частина 3. Signals

## Завдання 7. SIGINT

Запустив signal_int у псевдотерміналі і три рази натиснув Ctrl+C. Кожного разу вивело SIGINT received, і програма не закрилась. У /proc/PID/status SigCgt було 0000000000000002, тобто перехоплюється сигнал 2.

У handler використовується write, а не printf, бо сигнал може прийти посеред printf, і повторний printf може зламати буфер або зависнути. write вважається async-signal-safe.

## Завдання 8. Сигнал від parent до child

| Режим | Що вийшло |
|---|---|
| без handler | Parent: child was terminated by signal 15 (Terminated) |
| з handler | Child: SIGTERM caught by handler, cleaning up, потім child exited normally, status 7 |

Той самий kill(child, SIGTERM), але без handler ядро вбило child, а з handler child сам вийшов з кодом 7. Тобто kill тільки надсилає сигнал, а що буде далі, залежить від процесу.

## SIGKILL

Надсилав сигнали процесу signal_int з фону:

| Сигнал | Результат | Статус |
|---|---|---|
| kill -INT | перехопився, процес живий | - |
| kill -TERM | завершився | 143 (128 + 15) |
| kill -KILL | Killed | 137 (128 + 9) |

Handler для SIGKILL поставити не можна. Наскільки я зрозумів, це щоб завжди був спосіб вбити будь-який процес, навіть завислий.

| Сигнал | Номер | Звідки | Що робить за замовчуванням |
|---|---|---|---|
| SIGINT | 2 | Ctrl+C | завершує, можна перехопити |
| SIGTERM | 15 | kill PID | завершує, можна перехопити |
| SIGKILL | 9 | kill -9 | завершує одразу, перехопити не можна |
| SIGCHLD | 17 | ядро, коли child завершився | ігнорується, його видно в strace |

# Частина 4. Mini-Shell

## Як влаштований mini_shell

Shell ігнорує SIGINT. У циклі виводить myshell>, читає рядок через fgets і ділить на слова через strtok. Перед цим додає пробіли навколо | і >, щоб ls|wc теж працювало.

- exit і cd виконуються в самому shell без fork.
- Звичайна команда: fork, у child якщо є > то open, dup2 і close, потім повертаю SIGINT за замовчуванням і execvp. Parent чекає через waitpid.
- Команда з |: pipe, два fork, у першому dup2 на stdout, у другому на stdin (і > якщо є), parent закриває обидва кінці і чекає обох.

Лапки, змінні, *, ;, &&, >>, < і кілька pipe не підтримуються, бо в завданні їх не треба. system() не використовую.

Помилки fork, pipe, open, dup2, chdir і waitpid виводяться, і shell працює далі. Якщо exec не вдався, child пише помилку і виходить з кодом 127.

## Чому cd built-in

Якщо зробити cd через fork, то chdir зміниться тільки в child, а child одразу завершиться. Shell залишиться в старій папці, бо поточна папка своя в кожного процесу. Тому chdir треба робити в самому shell. З exit так само: exit у child завершив би тільки child.

## Перевірка

Вивід з псевдотерміналу:

```
myshell> pwd
/lab/out/shtest
myshell> ls -l
total 4
-rw-r--r-- 1 root root 28 Nov 24 10:18 file.txt
myshell> echo Hello Systems
Hello Systems
myshell> cd /tmp
myshell> pwd
/tmp
myshell> cd -nosuch
myshell: cd: -nosuch: No such file or directory
myshell> cd /lab/out/shtest
myshell> echo test > test.txt
myshell> cat test.txt
test
myshell> echo hello > output.txt
myshell> cat output.txt
hello
myshell> ls -l | wc -l
4
myshell> cat file.txt | grep hello
hello world
hello again
myshell> ls|wc -l > count.txt
myshell> cat count.txt
4
myshell> sleep 30
^C
myshell> echo still-running
still-running
myshell> sleep 30 | cat
^C
myshell> echo alive
alive
myshell> ^Ccommand_that_does_not_exist
myshell: command_that_does_not_exist: No such file or directory
myshell> echo x > /nonexistent/f
myshell: /nonexistent/f: No such file or directory
myshell> ls |
myshell: syntax error: missing command after '|'
myshell> | wc
myshell: syntax error: missing command before '|'
myshell> echo hi >
myshell: syntax error: missing file name after '>'
myshell> a | b | c
myshell: only one pipe is supported
myshell>
myshell> echo still alive after errors
still alive after errors
myshell> exit
```

Redirection працює (test.txt і output.txt), pipe теж (ls -l | wc -l і cat | grep). Ctrl+C під час sleep 30 зупинив sleep, а shell залишився. Ctrl+C з sleep 30 | cat теж зупинив обидва. Помилки shell не вбивають.

Ctrl+C працює так, бо його отримують і shell, і child. Shell його ігнорує, а child перед exec повертає SIG_DFL, тому sleep завершується. Повертати треба обов'язково, бо ігнорування сигналу зберігається після exec.

Один недолік: якщо натиснути Ctrl+C на порожньому prompt, shell його просто ігнорує, і ^C залишається в рядку. Bash у такому разі малює новий prompt, але для нашої спрощеної версії цього не вимагали.

## strace mini_shell

Запустив echo hello > output.txt, ls | wc -l і exit. Для першої команди:

```
clone(..., SIGCHLD) = 148
[pid   147] wait4(148,  <unfinished ...>
[pid   148] openat(AT_FDCWD, "output.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 3
[pid   148] dup3(3, 1, 0)               = 1
[pid   148] close(3)                    = 0
[pid   148] execve("/usr/bin/echo", ["echo", "hello"], ...) = 0
[pid   148] write(1, "hello\n", 6)      = 6
```

Для pipeline:

```
pipe2([3, 4], 0)                      = 0
clone(...) = 149
[pid   147] clone(...) = 150
[pid   147] close(3)
[pid   147] close(4)
[pid   147] wait4(149, ...)
[pid   149] dup3(4, 1, 0)
[pid   150] dup3(3, 0, 0)
[pid   149] execve("/usr/bin/ls", ["ls"], ...)
[pid   150] execve("/usr/bin/wc", ["wc", "-l"], ...)
[pid   150] write(1, "17\n", 3)
```

Тут видно все, що я описав вище: clone це fork, openat і dup3 для >, pipe2 і два clone для |, execve і wait4. Для exit жодного clone немає, бо це built-in.

Загальна схема така: користувач вводить рядок, shell його розбирає і або сам виконує cd чи exit, або робить fork і exec для команди, або pipe і два fork для pipeline. Далі все через системні виклики йде в ядро, яке керує процесами, файлами і pipe.

## Контрольні питання

1. IPC це способи обміну даними між процесами (pipe, сигнали, сокети і т.д.).
2. pipe() створює канал у ядрі і два дескриптори.
3. pipefd[0] це кінець для читання.
4. pipefd[1] це кінець для запису.
5. pipe працює через read і write, бо в нього є file descriptor.
6. Після fork child отримує копії всіх дескрипторів батька.
7. Непотрібні кінці треба закривати, бо інакше не буде EOF і програма може зависнути.
8. read з pipe повертає 0, коли pipe порожній і всі кінці для запису закриті.
9. Якщо parent залишить write end відкритим, читач не отримає EOF і все зависне.
10. dup2 робить один дескриптор копією іншого.
11. > робиться через open, dup2 на fd 1, close і exec.
12. Програмі не треба знати про перенаправлення, вона просто пише в fd 1.
13. program1 | program2: pipe, fork, у першому stdout в pipe, у другому stdin з pipe, exec обох, parent закриває кінці і чекає.
14. У першої програми до pipe підключається stdout.
15. У другої програми stdin.
16. Signal це повідомлення процесу про подію.
17. Сигнал передає тільки номер, а pipe передає дані.
18. kill надсилає сигнал процесу.
19. SIGTERM можна перехопити, SIGKILL ні, він вбиває одразу.
20. SIGKILL не можна перехопити, щоб завжди можна було вбити процес.
21. SIGINT приходить, коли натискаєш Ctrl+C.
22. printf у handler поганий вибір, бо може зламатись, якщо сигнал прийшов під час іншого printf.
23. Async-signal-safe функція це та, яку можна безпечно викликати в handler, наприклад write.
24. Shell це звичайна програма, бо він тільки використовує fork, exec і т.д., як і інші програми. Якщо він впаде, ядро не постраждає.
25. fork у shell створює процес для команди.
26. exec запускає саму команду в цьому процесі.
27. waitpid чекає, поки команда закінчиться.
28. cd built-in, бо папку треба змінити саме в shell.
29. exit робить сам shell, бо він має завершити себе.
30. system() не підходить, бо він сам запускає інший shell і все ховає.
31. Після exec дескриптори залишаються відкритими.
32. pipe створює канал, fork дає його двом процесам, dup2 підключає його до stdout і stdin.
33. Процеси pipeline працюють одночасно, бо це окремі процеси, а pipe сам їх синхронізує.
34. Якщо читач завершився, а писач пише далі, писач отримає SIGPIPE.
35. Після створення pipeline parent має закрити обидва кінці pipe.

## Висновок

Я розібрався, що pipe це буфер у ядрі з двома кінцями, і що EOF буде тільки тоді, коли закриті всі кінці для запису. Без цього програма в мене зависла. dup2 дозволяє перенаправити stdout у файл або pipe, і це зберігається після exec. Через це вдалося зробити > і ls -l | wc -l. З сигналами побачив, що SIGINT і SIGTERM можна перехопити, а SIGKILL ні. Наприкінці зібрав усе в mini_shell, який виконує команди, cd, exit, > і одну |, а від Ctrl+C зупиняється тільки команда, сам shell працює далі.
