# Lab 3. Побудова програми, ELF та організація пам'яті

Робив так само, як Lab 2: Docker gcc:13 з --platform linux/amd64 (x86-64, GCC 13.5, binutils 2.40).

Там gcc за замовчуванням робить не PIE, тому Type у мене EXEC і адреси починаються з 0x400000. У Codespaces скоріше буде DYN і адреси типу 0x5555..., але секції ті самі.

program.c вивела Result: 30.

## Завдання 1. Preprocessing

gcc -E program.c -o program.i. file каже, що це C source, ASCII text. program.c має 18 рядків, а program.i 756. int main знайшовся на рядку 749, а вище, наприклад на рядку 419, лежить оголошення printf.

- program.i такий великий, бо препроцесор вставив туди весь stdio.h і все, що той підключає.
- Рядка #include <stdio.h> там вже немає, замість нього текст заголовка і службові рядки типу # 1 "/usr/include/stdio.h".
- Це не машинний код, це все ще C.
- CPU його виконати не може.

## Завдання 2. Compilation

gcc -S -O0 -masm=intel program.i -o program.s, file каже assembler source. Там є add: і main:, a + b це add eax, edx. Ще видно, що global_value лежить в .data (.long 10), а global_empty в .bss (.zero 4).

- program.s написаний на Assembly, це текст.
- Це ще не machine code.
- ISA x86-64, видно по регістрах rbp, eax, edi.
- main і add є як мітки.

## Завдання 3. Object file

gcc -c program.s -o program.o. file: ELF 64-bit LSB relocatable, x86-64. cat показує сміття, бо файл двійковий. Перші байти 7f 45 4c 46, тобто 0x7f і ELF.

В objdump -d program.o функції починаються з адреси 0, а в call і там, де звертання до global_value, стоять нулі (e8 00 00 00 00). Як я зрозумів, object file ще не знає справжніх адрес, їх потім підставить linker.

| Файл | Що всередині |
|---|---|
| program.c | мій C-код, текст |
| program.i | C після препроцесора, текст |
| program.s | Assembly, текст |
| program.o | ELF relocatable, машинний код + symbols, двійковий |

## Завдання 4. Symbols

nm program.o:

| Symbol | Тип | Що це |
|---|---|---|
| add | T | код |
| main | T | код |
| global_value | D | ініціалізовані дані |
| global_empty | B | bss |
| printf | U | undefined, тут його немає |

add і main в program.o є, printf немає, тільки посилання на нього. printf знайде linker у libc.

## Завдання 5. Linking

gcc program.o -o program. Тепер file каже: ELF 64-bit LSB executable, dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2. До цього .o був relocatable. ./program знову дає Result: 30.

| Етап | Команда | Що отримуємо |
|---|---|---|
| preprocessing | gcc -E | program.i |
| compilation | gcc -S | program.s |
| assembly | gcc -c | program.o |
| linking | gcc program.o -o program | program |

## Завдання 6. Linker error

gcc -c main.c пройшло нормально, а gcc main.o -o app дало помилку undefined reference to `calculate'.

Компілятору достатньо прототипу, щоб скомпілювати один файл. А linker має знайти, де calculate реально визначена, і не знаходить її.

Додав calculate.c. nm main.o показує U calculate, а nm calculate.o показує T calculate, тобто один файл потребує функцію, а інший її визначає. gcc main.o calculate.o -o app працює, ./app вивела 30.

## Завдання 7. ELF header

readelf -h program:

| Поле | Значення |
|---|---|
| Class | ELF64 |
| Data | 2's complement, little endian |
| Type | EXEC (Executable file) |
| Machine | Advanced Micro Devices X86-64 |
| Entry point | 0x401040 |

ELF64 означає 64-бітний формат. Machine показує, що це x86-64. Entry point тут не main (main на 0x40113a), а _start, який вже потім викликає main.

## Завдання 8. Секції

readelf -S program:

| Секція | Адреса | Що там у моїй програмі |
|---|---|---|
| .text | 0x401040 | код add, main і _start |
| .rodata | 0x402000 | рядок "Result: %d\n" |
| .data | 0x404008 | int global_value = 10 |
| .bss | 0x40401c | int global_empty |

Через nm перевірив: add 0x401126 і main 0x40113a лежать у .text, global_value 0x404018 у .data, global_empty 0x404020 у .bss. У .bss тип NOBITS, тобто місця у файлі вона не займає.

## Завдання 9. .data і .bss

sections.c вивела 123 0 456 0 Computer Systems.

| Змінна | Де | Як перевірив |
|---|---|---|
| initialized | .data | nm D, у дампі .data є 7b 00 00 00 (123) |
| uninitialized | .bss | nm B |
| static_initialized | .data | nm d, у дампі c8 01 00 00 (456) |
| static_uninitialized | .bss | nm b |
| message | .rodata | nm R, у дампі .rodata є текст Computer Systems |

Мала буква в nm означає static, тобто ім'я видно тільки в цьому файлі.

## Завдання 10. Розмір областей

| | text | data | bss | розмір файлу |
|---|---|---|---|---|
| sections | 1066 | 568 | 16 | 15936 |
| з int large_array[100000] | 1066 | 568 | 400040 | 15952 |

bss виросла на 400 000 байтів, а файл тільки на 16 байтів. Так виходить, бо у файлі для .bss записано тільки розмір, а самі нулі ОС створює вже при запуску. У readelf -l видно, що в сегменті FileSiz 0x238, а MemSiz 0x61ce0.

## Завдання 11. ldd

ldd program показав libc.so.6 і /lib64/ld-linux-x86-64.so.2. nm program показує U printf@GLIBC_2.2.5.

- Повної реалізації printf в моєму файлі немає, вона в libc.so.6.
- .so означає shared object, тобто динамічна бібліотека.
- Кілька програм можуть використовувати одну бібліотеку, бо ОС підключає той самий код у пам'ять кожного процесу.
- Перевага в тому, що програми менші, пам'яті йде менше, а якщо оновити бібліотеку, оновляться всі програми.

## Завдання 12. Executable і process

Запустив process у фоні, він показав PID 102. ps -p 102 показав стан S, тобто він спить у sleep. У /proc/102/maps видно, як шматки файлу process відображені в пам'ять, плюс heap і libc.

Файл process просто лежить на диску. А процес з PID 102 це запущена програма: у неї є своя пам'ять, стан і батьківський процес. З одного файлу можна запустити кілька процесів.

## Завдання 13. Де лежать дані

Мій прогноз: main у code, global_initialized і static_initialized у data, uninitialized у bss, local у stack, malloc у heap.

| Об'єкт | Адреса | Область | Збігся з прогнозом |
|---|---|---|---|
| main | 0x401146 | .text | так |
| global_initialized | 0x404028 | .data | так |
| static_initialized | 0x40402c | .data | так |
| global_uninitialized | 0x404034 | .bss | так |
| static_uninitialized | 0x404038 | .bss | так |
| dynamic | 0x4052a0 | heap | так |
| local | 0x7ffffffca8d4 | stack | так |

- Близько одна до одної лежать глобальні і статичні змінні.
- global_initialized і global_uninitialized поруч, але в різних секціях.
- local дуже далеко від глобальних.
- malloc теж далеко від local: heap внизу після даних, stack зверху.
- Тобто пам'ять процесу поділена на окремі області.

## Завдання 14. Адреси при різних запусках

| | Run 1 | Run 2 | Run 3 |
|---|---|---|---|
| main | 0x401146 | 0x401146 | 0x401146 |
| global_initialized | 0x404028 | 0x404028 | 0x404028 |
| dynamic | 0x4052a0 | 0x4052a0 | 0x4052a0 |
| local | 0x7ffffffca8d4 | 0x7ffffffca8e4 | 0x7ffffffca904 |

У мене змінювалась тільки адреса local. Код і глобальні змінні однакові, бо файл не PIE і завантажується за фіксованою адресою. Я ще зібрав PIE-версію, але там main теж не мінявся (0x555555555159). Думаю, це через емуляцію x86-64 на Mac. На нормальному Linux з ASLR адреси мають змінюватись кожен раз, це робиться для безпеки.

## Завдання 15. Stack

| Функція | Адреса |
|---|---|
| level1 &a | 0x7ffffffca96c |
| level2 &b | 0x7ffffffca94c |
| level3 &c | 0x7ffffffca92c |

Адреси змінюються і кожна наступна менша на 0x20 (32 байти). Стек на x86-64 росте вниз, тож це сходиться. Між запусками адреси інші, але крок той самий.

## Завдання 16. Heap

Думав, що адреси будуть близько, але не обов'язково через 4 байти. Вийшло a 0x4052a0, b 0x4052c0, c 0x4052e0, тобто через 32 байти, хоча я просив 4. Мабуть allocator додає свій заголовок і вирівнювання. Але це не значить, що malloc завжди дає адреси підряд. Головне, що heap зовсім в іншому місці, ніж stack.

## Фінальне завдання memory_map.c

У програмі є функція square, рядок "memory map", глобальні і статичні змінні з ініціалізацією і без, локальна змінна і malloc.

| Об'єкт | Адреса | Область |
|---|---|---|
| square | 0x401146 | .text |
| main | 0x401155 | .text |
| "memory map" | 0x402008 | .rodata |
| counter_initialized | 0x404028 | .data |
| ratio_initialized (static) | 0x404030 | .data |
| counter_uninitialized | 0x404060 | .bss |
| buffer_uninitialized (static) | 0x404080 | .bss |
| heap_value | 0x4052a0 | heap |
| local | 0x7ffffffca9dc | stack |

Карта пам'яті від більших адрес до менших: stack (local) зверху, далі heap (heap_value), потім .bss (counter_uninitialized, buffer_uninitialized), .data (counter_initialized, ratio_initialized), .rodata ("memory map") і внизу .text (square, main).

Pipeline для memory_map.c зробив окремими командами -E, -S, -c і лінкування, потім file:

| Файл | Етап | Що містить | CPU може виконати? |
|---|---|---|---|
| .c | вихідний код | C-текст з #include і коментарями | ні |
| .i | після препроцесора | C-текст із вставленими заголовками | ні |
| .s | після компілятора | Assembly-текст | ні |
| .o | після асемблера | ELF relocatable, машинний код, адреси ще не готові | ні |
| executable | після linker | ELF executable з усіма адресами і entry point | так, коли ОС його запустить |

## Контрольні питання

1. Етапи: preprocessing, compilation, assembly, linking.
2. Препроцесор вставляє #include, розгортає #define, обробляє #if і прибирає коментарі.
3. Компілятор робить Assembly для потрібної ISA.
4. Асемблер переводить Assembly в машинний код і робить object file.
5. В object file є машинний код, дані, таблиця symbols і місця, куди linker має вписати адреси (relocations).
6. Linker з'єднує object files і бібліотеки, знаходить усі symbols і ставить остаточні адреси.
7. undefined reference означає, що linker не знайшов, де визначена функція або змінна.
8. Symbol це ім'я функції або глобальної змінної разом з її адресою.
9. Header містить тільки оголошення для компілятора, а library містить готовий скомпільований код.
10. ELF це формат виконуваних файлів, object files і бібліотек у Linux.
11. У .text лежить код.
12. У .rodata константи і рядки, які не можна змінювати.
13. У .data змінні з початковим значенням, у .bss змінні без нього, вони заповнюються нулями.
14. Велика .bss не збільшує файл, бо там зберігається тільки її розмір.
15. ldd показує, які динамічні бібліотеки потрібні програмі.
16. Executable це файл, а process це запущена програма з PID і своєю пам'яттю.
17. Локальні змінні лежать у stack.
18. Heap потрібен для пам'яті через malloc, розмір якої відомий тільки під час роботи.
19. Stack виділяється і звільняється автоматично при виклику функцій і росте вниз. Heap виділяє і звільняє програміст через malloc і free, він більший і повільніший.
20. Адреси змінюються через ASLR, щоб їх було важче вгадати.
21. Temporal locality: якщо дані щойно використовували, скоро їх, мабуть, використають знову.
22. Spatial locality: якщо звернулися до адреси, скоро, мабуть, звернуться до сусідніх.

## Висновок

Я розбив gcc на окремі кроки і подивився, що виходить на кожному: .i, .s, .o і executable. На linker error побачив, що компіляція і лінкування це різні речі. Розібрався, що лежить у .text, .rodata, .data і .bss, і що велика .bss майже не збільшує файл. По адресах видно, що пам'ять процесу поділена на код, дані, heap і stack, і що stack росте вниз.
