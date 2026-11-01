## Задача A. Избавьтесь от легаси или продолжайте жить в проклятом мире, который сами создали

#### Этап 0. Базовый анализ размеров файлов и секций

**Вопросы:**
1. В качестве ответа на этот вопрос предоставьте вывод команды `size`

```bash
root@58b293e08827:/work# size core.o legacy.o hotfix.o main.o
   text	   data	    bss	    dec	    hex	filename
    266	   4040	  50000	  54306	   d422	core.o
    172	      0	      4	    176	     b0	legacy.o
    127	      0	      0	    127	     7f	hotfix.o
    118	      0	      0	    118	     76	main.o
```

2. В одном из файлов столбец `bss` имеет аномально большое значение при малом общем размере файла (`dec`). Объясните, почему так могло произойти и где физически хранится содержимое этой секции на диске

```bash
root@58b293e08827:/work# ls -l *.o
-rw-r--r-- 1 root root 6200 Nov  1 20:08 core.o
-rw-r--r-- 1 root root 1456 Nov  1 20:08 hotfix.o
-rw-r--r-- 1 root root 1608 Nov  1 20:08 legacy.o
-rw-r--r-- 1 root root 1448 Nov  1 20:08 main.o
root@58b293e08827:/work# size -A core.o
core.o  :
section               size   addr
.text                   70      0 # Код функций
.data                 4032      0 # Инициализированные глобальные переменные
.bss                 50000      0 # Неинициализированные или нулевые переменные
.rodata                 76      0 # Данные только для чтения (строки)
.data.rel.local          8      0 # Указатель, которому нужна релокация
.comment                39      0
.note.GNU-stack          0      0
.note.gnu.property     32      0
.eh_frame               88      0
Total                54345
root@58b293e08827:/work# readelf -S -W core.o | grep -E 'Nr|\.data|\.bss|\.rodata'
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [ 3] .data             PROGBITS        0000000000000000 000088 000fc0 00  WA  0   0 32
  [ 4] .bss              NOBITS          0000000000000000 001048 00c350 00  WA  0   0 32
  [ 5] .rodata           PROGBITS        0000000000000000 001048 00004c 00   A  0   0 16
  [ 6] .data.rel.local   PROGBITS        0000000000000000 001098 000008 00  WA  0   0  8
  [ 7] .rela.data.rel.local RELA            0000000000000000 001360 000018 18   I 13   6  8
root@58b293e08827:/work# readelf -s -W core.o | grep -E 'large_buffer|config_table|secret_token|welcome_msg'
     4: 0000000000000000 50000 OBJECT  LOCAL  DEFAULT    4 large_buffer
     8: 0000000000000020  4000 OBJECT  GLOBAL DEFAULT    3 config_table
     9: 0000000000000000     8 OBJECT  GLOBAL DEFAULT    6 welcome_msg
    10: 0000000000000020    30 OBJECT  GLOBAL DEFAULT    5 secret_token
root@58b293e08827:/work# readelf -x .data core.o | head -5

Hex dump of section '.data':
  0x00000000 01000000 00000000 00000000 00000000 ................
  0x00000010 00000000 00000000 00000000 00000000 ................
  0x00000020 01000000 02000000 03000000 00000000 ................
```

> Это `core.o`: bss 50000 байт, а сам файл на диске весит всего 6200 байт. Почти весь bss занимает одна static переменная `large_buffer` на 50000 байт, скорее всего там было что-то вроде `static char large_buffer[50000];` без инициализации
>
> В bss попадают неинициализированные и нулевые глобальные и статические переменные. Хранить в файле 50000 нулей нет смысла, поэтому у секции тип NOBITS: в заголовке записан только размер (0xc350 = 50000), а самих данных в файле нет. Это видно по смещениям, у `.bss` и у следующей за ней `.rodata` одинаковое смещение 0x1048, то есть bss занимает в файле 0 байт. Получается, на диске содержимое bss нигде не хранится, память под неё выделяет загрузчик при запуске программы и сразу заполняет нулями
>
> Для сравнения `config_table` на 4000 байт лежит в `.data`, потому что у неё есть начальные значения (в дампе видно 1, 2, 3 и дальше нули), и все 4000 байт реально записаны в файл. В итоге именно `core.o` больше всех ест память: около 54 КБ данных при 70 байтах кода

#### Этап 1. Извлечение строк

**Вопросы:**
3. В файле `core.o` присутствует секция, которой нет в `legacy.o` и `hotfix.o` (интуиция подсказывает вам, что это что-то связанное со строками). Найдите её и укажите точное имя

```bash
root@58b293e08827:/work# size -A legacy.o hotfix.o
legacy.o  :
section              size   addr
.text                  52      0
.data                   0      0
.bss                    4      0
.comment               39      0
.note.GNU-stack         0      0
.note.gnu.property     32      0
.eh_frame              88      0
Total                 215


hotfix.o  :
section              size   addr
.text                  39      0
.data                   0      0
.bss                    0      0
.comment               39      0
.note.GNU-stack         0      0
.note.gnu.property     32      0
.eh_frame              56      0
Total                 166


root@58b293e08827:/work# readelf -p .rodata core.o

String dump of section '.rodata':
  [     0]  System Initialized v1.0
  [    20]  FLAG{b1nary_4rch4eology_2026}
  [    3e]  %s, Mode: %d\n

```

> Если сравнить с `size -A core.o` из прошлого вопроса, то в `core.o` есть `.rodata` и `.data.rel.local`, а в `legacy.o` и `hotfix.o` их нет. Строки лежат в секции `.rodata` (read-only data), это и есть ответ. В `.data.rel.local` лежит только 8-байтный указатель `welcome_msg` на строку "System Initialized v1.0", ему нужна релокация, потому что адрес строки станет известен только после компоновки

4. Найдите среди полученных строк секретный токен, оставленный разработчиком. Скопируйте его в отчёт

```bash
root@58b293e08827:/work# strings -a core.o
System Initialized v1.0
FLAG{b1nary_4rch4eology_2026}
%s, Mode: %d
GCC: (Ubuntu 15.2.0-16ubuntu1) 15.2.0
core.c
large_buffer
internal_cleanup
system_mode
config_table
welcome_msg
secret_token
print_status
printf
.symtab
.strtab
.shstrtab
.rela.text
.data
.bss
.rodata
.rela.data.rel.local
.comment
.note.GNU-stack
.note.gnu.property
.rela.eh_frame
```

> Токен: `FLAG{b1nary_4rch4eology_2026}`
>
> Он лежит в `.rodata` по смещению 0x20, ему соответствует символ `secret_token` (тип R, 30 байт, то есть 29 символов и '\0'). Заодно из строк видно, что файлы собирали в GCC 15.2.0, а исходник назывался core.c

#### Этап 2. Извлечение символов

**Вопросы:**
5. Сравните таблицы символов четырёх файлов. Найдите имена, которые встречаются более чем в одном файле. Какие именно имена являются общими для нескольких модулей?

```bash
root@58b293e08827:/work# nm core.o
0000000000000020 D config_table
0000000000000000 t internal_cleanup
0000000000000000 b large_buffer
0000000000000012 T print_status
                 U printf
0000000000000020 R secret_token
0000000000000000 D system_mode
0000000000000000 D welcome_msg
root@58b293e08827:/work# nm legacy.o
000000000000001a T apply_legacy_patch
0000000000000000 b legacy_counter
0000000000000000 t legacy_helper
0000000000000004 C system_mode
root@58b293e08827:/work# nm hotfix.o
0000000000000000 T apply_hotfix
0000000000000004 C system_mode
root@58b293e08827:/work# nm main.o
                 U apply_hotfix
0000000000000000 T main
                 U print_status
root@58b293e08827:/work# nm -A core.o legacy.o hotfix.o main.o | grep -E 'system_mode|print_status|apply_hotfix'
core.o:0000000000000012 T print_status
core.o:0000000000000000 D system_mode
legacy.o:0000000000000004 C system_mode
hotfix.o:0000000000000000 T apply_hotfix
hotfix.o:0000000000000004 C system_mode
main.o:                 U apply_hotfix
main.o:                 U print_status
```

> Общих имён три: `system_mode` (в core.o, legacy.o и hotfix.o), `print_status` (в core.o и main.o) и `apply_hotfix` (в hotfix.o и main.o)
>
> С `print_status` и `apply_hotfix` всё нормально: функция определена (T) в одном файле и используется (U) в другом, так и должно быть. А вот `system_mode` определена сразу в трёх файлах, это и есть проблема

6. Для каждого общего имени проанализируйте: в каких файлах оно присутствует, какой тип символа в каждом файле, в какой секции (Ndx) оно находится

```bash
root@58b293e08827:/work# readelf -s -W core.o legacy.o hotfix.o main.o | grep -E 'File|system_mode|print_status|apply_hotfix'
File: core.o
     7: 0000000000000000     4 OBJECT  GLOBAL DEFAULT    3 system_mode
    11: 0000000000000012    52 FUNC    GLOBAL DEFAULT    1 print_status
File: legacy.o
     6: 0000000000000004     4 OBJECT  GLOBAL DEFAULT  COM system_mode
File: hotfix.o
     3: 0000000000000001     4 OBJECT  GLOBAL DEFAULT  COM system_mode
     4: 0000000000000000    39 FUNC    GLOBAL DEFAULT    1 apply_hotfix
File: main.o
     4: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND print_status
     5: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND apply_hotfix
```

> `system_mode`:
> - core.o: тип D, Ndx 3 (.data), размер 4 байта, обычное определение инициализированной переменной
> - legacy.o: тип C, Ndx COM, размер 4, выравнивание 4, COMMON символ
> - hotfix.o: тип C, Ndx COM, размер 4, выравнивание 1, COMMON символ (это `char system_mode[4]`)
>
> `print_status`:
> - core.o: тип T, Ndx 1 (.text), функция на 52 байта
> - main.o: тип U, Ndx UND, только вызов
>
> `apply_hotfix`:
> - hotfix.o: тип T, Ndx 1 (.text), функция на 39 байт
> - main.o: тип U, Ndx UND, только вызов
>
> У COMMON символов секции ещё нет, место под них выделит компоновщик. И у них в колонке Value не адрес, а выравнивание: у legacy.o 4 (похоже на int), у hotfix.o 1 (массив char)

7. Сопоставьте ваши наблюдения с исходным кодом `hotfix.c`. Что вы можете сказать о переменной `system_mode` в `core.o` и `legacy.o`, не имея их исходного кода? Какой конфликт это создаёт?

```bash
root@58b293e08827:/work# readelf -x .data core.o | head -3

Hex dump of section '.data':
  0x00000000 01000000 00000000 00000000 00000000 ................
root@58b293e08827:/work# objdump -d -M intel -r core.o | grep -B1 -A1 'system_mode'
  1f:	8b 15 00 00 00 00    	mov    edx,DWORD PTR [rip+0x0]        # 25 <print_status+0x13>
			21: R_X86_64_PC32	system_mode-0x4
  25:	48 8b 05 00 00 00 00 	mov    rax,QWORD PTR [rip+0x0]        # 2c <print_status+0x1a>
root@58b293e08827:/work# objdump -d -M intel -r legacy.o | grep -B1 -A1 'system_mode'
  2e:	00 00 00 
			29: R_X86_64_PC32	system_mode-0x8
  31:	90                   	nop
```

> В core.o `system_mode` это D в `.data`, 4 байта, в начале `.data` лежит `01 00 00 00`. В `print_status` она читается как DWORD в edx и печатается через "%s, Mode: %d". Скорее всего в core.c было `int system_mode = 1;`, то есть сильное определение
>
> В legacy.o это COMMON на 4 байта с выравниванием 4, и в `apply_legacy_patch` туда пишется `mov DWORD PTR ..., 0x2a` (это 42). Похоже на `int system_mode;` без инициализации, собранное с -fcommon, то есть слабое определение
>
> А в hotfix.c это `char system_mode[4];`, тоже COMMON, но уже массив из 4 char
>
> Конфликт в том, что одно глобальное имя определено в трёх модулях, да ещё с разными типами (int и char[4]). Компоновщик склеит их в одну переменную, и `apply_hotfix` будет писать байты 'A', 'B', 'C', '\0' прямо в int, которым пользуется core.o. По стандарту это неопределённое поведение, а на деле тихая порча данных

#### Этап 3. Попытка компоновки

**Вопросы:**
8. Выдал ли компоновщик ошибку или предупреждение? Если да, процитируйте его дословно. Если нет, то, как вы думаете, почему компоновщик не сообщил о проблеме?

```bash
root@58b293e08827:/work# gcc -fcommon core.o legacy.o hotfix.o main.o -o app
root@58b293e08827:/work# echo $?
0
root@58b293e08827:/work# ./app
System Initialized v1.0, Mode: 1
System Initialized v1.0, Mode: 4407873
root@58b293e08827:/work# printf '%x\n' 4407873
434241
root@58b293e08827:/work# gcc -fcommon core.o legacy.o hotfix.o main.o -o app -Wl,--warn-common
/usr/bin/ld: legacy.o: warning: common of `system_mode' overridden by definition from core.o
/usr/bin/ld: hotfix.o: warning: common of `system_mode' overridden by definition from core.o
```

> Нет, ни ошибки, ни предупреждения не было, код возврата 0
>
> Так происходит, потому что COMMON символы специально сделаны так, чтобы их можно было "определять" в нескольких файлах (это пришло из старого C и Фортрана). Если есть одно обычное определение и сколько угодно COMMON с тем же именем, компоновщик берёт обычное, а все COMMON просто начинают ссылаться на него. Для него это норма, а не ошибка. Типов переменных компоновщик не знает, для него `system_mode` это просто имя, размер и выравнивание. Размеры везде по 4 байта, так что даже повода предупредить нет. Сам флаг -fcommon здесь ничего не меняет, он влияет только на компиляцию, а у нас уже готовые .o
>
> Если попросить `-Wl,--warn-common`, компоновщик всё-таки пишет, что COMMON из legacy.o и hotfix.o перекрыты определением из core.o
>
> А при запуске видно последствия: после `apply_hotfix` режим стал 4407873. В hex это 0x00434241, то есть байты 'A' (0x41), 'B' (0x42), 'C' (0x43) и '\0', прочитанные как int (little-endian)

#### Этап 4. Анализ исполняемого файла и фикс

**Вопросы:**
9. В какой секции (`.data` или `.bss`) в итоге оказался `system_mode` в финальном исполняемом файле? Как вы думаете, почему компоновщик сделал именно такой выбор?

```bash
root@58b293e08827:/work# nm app | grep system_mode
0000000000004020 D system_mode
root@58b293e08827:/work# readelf -s -W app | grep -E 'Ndx|system_mode'
   Num:    Value          Size Type    Bind   Vis      Ndx Name
   Num:    Value          Size Type    Bind   Vis      Ndx Name
    33: 0000000000004020     4 OBJECT  GLOBAL DEFAULT   25 system_mode
root@58b293e08827:/work# readelf -S -W app | grep -E 'Nr|\.data |\.bss '
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [25] .data             PROGBITS        0000000000004000 003000 000fe8 00  WA  0   0 32
  [26] .bss              NOBITS          0000000000005000 003fe8 00c378 00  WA  0   0 32
```

> `system_mode` оказался в `.data`: тип D, Ndx 25 это `.data`, адрес 0x4020 попадает в её диапазон 0x4000 - 0x4fe8, размер 4 байта
>
> Так вышло, потому что в core.o есть сильное определение с начальным значением 1, а начальное значение можно хранить только в `.data`. Сильное определение важнее COMMON, поэтому компоновщик взял его, а обращения из legacy.o и hotfix.o направил на тот же адрес. Если бы все три были COMMON без инициализации, компоновщик сам выделил бы место в `.bss` по самому большому размеру

Пересобираю с запретом COMMON:

```bash
root@58b293e08827:/work# gcc -fno-common main.o hotfix.o legacy.o core.o -o app_better
root@58b293e08827:/work# echo $?
0
root@58b293e08827:/work# ./app_better
System Initialized v1.0, Mode: 1
System Initialized v1.0, Mode: 4407873
```

> У меня эта команда прошла без ошибки. Причина та же, что в вопросе 8: -fno-common влияет на то, какие символы генерирует компилятор, а при компоновке готовых .o уже ничего не делает, в hotfix.o символ так и остался C. Поэтому я пересобрал hotfix.c с этим флагом:

```bash
root@58b293e08827:/work# nm hotfix.o
0000000000000000 T apply_hotfix
0000000000000004 C system_mode
root@58b293e08827:/work# gcc -fno-common -c hotfix.c -o hotfix.o
root@58b293e08827:/work# nm hotfix.o
0000000000000000 T apply_hotfix
0000000000000000 B system_mode
root@58b293e08827:/work# gcc -fno-common main.o hotfix.o legacy.o core.o -o app_better
/usr/bin/ld: warning: alignment 1 of normal symbol `system_mode' in hotfix.o is smaller than 4 used by the common definition in legacy.o
/usr/bin/ld: warning: NOTE: alignment discrepancies can cause real problems.  Investigation is advised.
/usr/bin/ld: core.o:(.data+0x0): multiple definition of `system_mode'; hotfix.o:(.bss+0x0): first defined here
collect2: error: ld returned 1 exit status
```

**Вопросы:**
10. Какое сообщение об ошибке теперь выдает компоновщик? Как, не меняя имена переменных, можно исправить `hotfix.c`, чтобы эта сборка прошла успешно?

> Ошибка:
> ```
> /usr/bin/ld: core.o:(.data+0x0): multiple definition of `system_mode'; hotfix.o:(.bss+0x0): first defined here
> collect2: error: ld returned 1 exit status
> ```
> С -fno-common `char system_mode[4];` стала обычным сильным определением в `.bss` (тип B). Теперь сильных определений два, в core.o и в hotfix.o, и такое компоновщик уже не пропускает. Перед ошибкой он ещё ругается, что выравнивание в hotfix.o (1) меньше, чем у COMMON из legacy.o (4), это тоже намёк на то, что типы разные
>
> Исправить можно, если превратить определение в hotfix.c в объявление через `extern`. Тогда hotfix.c будет пользоваться переменной из core.o (файл `solution/hotfix_fixed.c`):

```c
extern char system_mode[4];

void apply_hotfix(void) {
  system_mode[0] = 'A';
  system_mode[1] = 'B';
  system_mode[2] = 'C';
  system_mode[3] = '\0';
}
```

```bash
root@58b293e08827:/work# gcc -fno-common -c hotfix_fixed.c -o hotfix.o
root@58b293e08827:/work# nm hotfix.o
0000000000000000 T apply_hotfix
                 U system_mode
root@58b293e08827:/work# gcc -fno-common main.o hotfix.o legacy.o core.o -o app_better
root@58b293e08827:/work# echo $?
0
root@58b293e08827:/work# ./app_better
System Initialized v1.0, Mode: 1
System Initialized v1.0, Mode: 4407873
```

> Теперь в hotfix.o `system_mode` имеет тип U, определение осталось одно в core.o, и сборка проходит
>
> Но `extern` чинит только сборку, сама ошибка в логике остаётся: hotfix.c всё ещё считает переменную массивом char, а в core.o это int, поэтому режим так же портится в 4407873. По-хорошему нужен общий заголовок с `extern int system_mode;`, чтобы компилятор проверял тип во всех файлах, и `apply_hotfix` должна записывать число, а не строку
>
> Ещё один вариант без переименования это `static char system_mode[4];` (файл `solution/hotfix_static.c`). Тогда у hotfix.c будет своя переменная, которую другие модули не видят (тип b), конфликт пропадает, и режим в core.o больше не портится. Но тогда и сам hotfix ни на что не влияет, так что какой вариант правильный, зависит от того, что хотел сделать автор патча

```bash
root@58b293e08827:/work# gcc -fno-common -c hotfix_static.c -o hotfix_static.o && nm hotfix_static.o
0000000000000000 T apply_hotfix
0000000000000000 b system_mode
root@58b293e08827:/work# gcc -fno-common main.o hotfix_static.o legacy.o core.o -o app_static_var && ./app_static_var
System Initialized v1.0, Mode: 1
System Initialized v1.0, Mode: 1
```
