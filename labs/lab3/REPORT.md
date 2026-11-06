### Задача А. Как пройти в библиотеку?

Создал три файла из задания (копии лежат в `solution/`)

```bash
root@58b293e08827:/work# ls
main.cpp
vector_math.c
vector_math.h
```

#### Этап 1. Статическая библиотека (`.a`)

```bash
root@58b293e08827:/work# gcc -c vector_math.c -o vector_math.o
root@58b293e08827:/work# g++ -c main.cpp -o main.o
root@58b293e08827:/work# ar rcs libmath_static.a vector_math.o
root@58b293e08827:/work# ar t libmath_static.a
vector_math.o
root@58b293e08827:/work# nm -s libmath_static.a

Archive index:
get_lib_version in vector_math.o
dot_product in vector_math.o

vector_math.o:
0000000000000000 d LIB_VERSION
0000000000000011 T dot_product
0000000000000000 T get_lib_version
root@58b293e08827:/work# g++ main.o -L. -lmath_static -o app_static
root@58b293e08827:/work# ./app_static
Library version: v2.4.1-release
Dot product result: 32
```

> Результат верный: 1*4 + 2*5 + 3*6 = 32

**Вопросы:**
1. Коротко опишите, что делают флаги утилиты `ar`: `r`, `c`, `s`. Что произойдёт, если запустить команду второй раз с тем же именем архива?

```bash
root@58b293e08827:/work# ar rcs libmath_static.a vector_math.o
root@58b293e08827:/work# echo $?
0
root@58b293e08827:/work# ar rv libmath_static.a vector_math.o
r - vector_math.o
root@58b293e08827:/work# ar t libmath_static.a
vector_math.o
root@58b293e08827:/work# ar r test.a vector_math.o
ar: creating test.a
root@58b293e08827:/work# ar rcS test_noindex.a vector_math.o
root@58b293e08827:/work# nm -s test_noindex.a

vector_math.o:
0000000000000000 d LIB_VERSION
0000000000000011 T dot_product
0000000000000000 T get_lib_version
root@58b293e08827:/work# g++ main.o test_noindex.a -o app_test
/usr/bin/ld: test_noindex.a: error adding symbols: archive has no index; run ranlib to add one
collect2: error: ld returned 1 exit status
```

> r (replace) добавляет файлы в архив, а если файл с таким именем там уже есть, заменяет его
>
> c (create) создаёт архив, если его ещё нет, и не пишет при этом сообщение `ar: creating ...` (без c оно появляется, видно на test.a)
>
> s записывает или обновляет индекс символов архива, то же самое делает ranlib. По индексу компоновщик быстро находит, в каком .o лежит нужный символ. В GNU ar индекс строится и без s, но если его явно выключить флагом S, компоновщик с таким архивом работать не будет: `archive has no index; run ranlib to add one`
>
> Если запустить команду второй раз с тем же архивом, ничего страшного не будет. Архив не пересоздаётся и файлы в нём не дублируются, vector_math.o просто заменяется новой версией (`ar rv` пишет `r - vector_math.o`), внутри всё так же один файл, индекс обновляется. То есть после изменения vector_math.c достаточно пересобрать .o и выполнить ту же команду. Но удалить так ничего не получится: если убрать какой-то .o из команды, он останется в архиве от прошлой сборки

2. С помощью утилиты `nm` посмотрите, что получилось в `main.o`. Как вы думаете, почему функция `main` не подверглась name mangling (имеет "чистое" имя), хотя файл скомпилирован компилятором C++?

```bash
root@58b293e08827:/work# nm main.o
                 U _GLOBAL_OFFSET_TABLE_
                 U _ZNSolsEPFRSoS_E
                 U _ZNSolsEi
0000000000000027 r _ZNSt8__detail30__integer_to_chars_is_unsignedIjEE
0000000000000028 r _ZNSt8__detail30__integer_to_chars_is_unsignedImEE
0000000000000029 r _ZNSt8__detail30__integer_to_chars_is_unsignedIyEE
                 U _ZSt21ios_base_library_initv
                 U _ZSt4cout
                 U _ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_
                 U _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc
                 U __stack_chk_fail
                 U dot_product
                 U get_lib_version
0000000000000000 T main
root@58b293e08827:/work# nm -C main.o | grep -E 'main|dot|get_lib|cout'
                 U std::cout
                 U dot_product
                 U get_lib_version
0000000000000000 T main
```

> Все C++ имена искажены (std::cout стал `_ZSt4cout`, operator<< стал `_ZNSolsEi` и т.д.), а main нет. Это потому, что main особая функция. Её вызывает не наш код, а стартовый код рантайма (`_start`, потом `__libc_start_main`), который написан на C и ищет символ именно с именем main. Ещё main нельзя перегружать по стандарту C++, так что зашивать типы аргументов в имя незачем, и компилятор всегда оставляет ей обычное сишное имя
>
> `dot_product` и `get_lib_version` тоже не искажены, но тут другая причина: они в vector_math.h обёрнуты в `extern "C"` (это как раз проверяется в этапе 4)

#### Этап 2. Динамическая библиотека (`.so`)

Шаг 1, сборка .so без -fPIC:

```bash
root@58b293e08827:/work# gcc -c vector_math.c -o vector_math_nopic.o
root@58b293e08827:/work# gcc -shared -o libmath_broken.so vector_math_nopic.o
root@58b293e08827:/work# echo $?
0
root@58b293e08827:/work# gcc -v 2>&1 | grep -o -- '--enable-default-pie'
--enable-default-pie
root@58b293e08827:/work# readelf -r vector_math_nopic.o | grep -A3 'rela.text\|rela.data'
Relocation section '.rela.text' at offset 0x270 contains 1 entry:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
00000000000b  000400000002 R_X86_64_PC32     0000000000000000 .data.rel.local - 4

Relocation section '.rela.data.rel.local' at offset 0x288 contains 1 entry:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000000000  000300000001 R_X86_64_64       0000000000000000 .rodata + 0
```

> У меня ошибки не было. Я поискал почему: gcc в Ubuntu собран с `--enable-default-pie` и по умолчанию делает позиционно-независимый код даже без -fPIC. Плюс единственная глобальная переменная `LIB_VERSION` объявлена как static, к ней обращаются относительно rip (релокация R_X86_64_PC32 на локальную секцию), а такое можно разрешить и при сборке .so
>
> Чтобы всё-таки увидеть ошибку, я сделал копию в папке exp и убрал там static у `LIB_VERSION`, чтобы переменная стала глобальной (оригинальный vector_math.c не трогал):

```bash
root@58b293e08827:/work# mkdir -p exp && cp vector_math.c exp/vector_math_global.c
root@58b293e08827:/work# grep -n 'LIB_VERSION =' exp/vector_math_global.c
3:const char* LIB_VERSION = "v2.4.1-release";
root@58b293e08827:/work# gcc -I. -c exp/vector_math_global.c -o exp/vm_global_nopic.o
root@58b293e08827:/work# gcc -shared -o exp/libmath_broken.so exp/vm_global_nopic.o
/usr/bin/ld: exp/vm_global_nopic.o: warning: relocation against `LIB_VERSION' in read-only section `.text'
/usr/bin/ld: exp/vm_global_nopic.o: relocation R_X86_64_PC32 against symbol `LIB_VERSION' can not be used when making a shared object; recompile with -fPIC
/usr/bin/ld: final link failed: bad value
collect2: error: ld returned 1 exit status
root@58b293e08827:/work# gcc -I. -c -fPIC exp/vector_math_global.c -o exp/vm_global_pic.o && gcc -shared -o exp/libmath_ok.so exp/vm_global_pic.o && echo OK
OK
```

Шаги 2 и 3, сборка с -fPIC и линковка:

```bash
root@58b293e08827:/work# gcc -c -fPIC vector_math.c -o vector_math_pic.o
root@58b293e08827:/work# gcc -shared -o libmath_dynamic.so vector_math_pic.o
root@58b293e08827:/work# g++ main.o -L. -lmath_dynamic -o app_dynamic
root@58b293e08827:/work# ./app_dynamic
./app_dynamic: error while loading shared libraries: libmath_dynamic.so: cannot open shared object file: No such file or directory
root@58b293e08827:/work# ldd app_dynamic | grep math
	libmath_dynamic.so => not found
root@58b293e08827:/work# LD_LIBRARY_PATH=. ./app_dynamic
Library version: v2.4.1-release
Dot product result: 32
```

> Сразу программа не запустилась: загрузчик ищет .so только в системных папках, а текущей папки там нет. `-L.` нужен только компоновщику во время сборки, при запуске он уже не работает. Поэтому запускал через `LD_LIBRARY_PATH=.` (ещё можно вшить путь при сборке через `-Wl,-rpath,'$ORIGIN'`)

**Вопросы:**
1. Приведите текст полученной на шаге 1 ошибки

> С кодом из задания ошибки у меня не было (почему, написал выше). Ошибка из эксперимента с глобальной переменной:
> ```
> /usr/bin/ld: exp/vm_global_nopic.o: warning: relocation against `LIB_VERSION' in read-only section `.text'
> /usr/bin/ld: exp/vm_global_nopic.o: relocation R_X86_64_PC32 against symbol `LIB_VERSION' can not be used when making a shared object; recompile with -fPIC
> /usr/bin/ld: final link failed: bad value
> collect2: error: ld returned 1 exit status
> ```
> Как я это понял: код без -fPIC обращается к глобальной переменной напрямую, как будто знает её адрес относительно кода. Но в .so глобальный символ может подменить другой модуль (например сама программа), и настоящий адрес будет известен только при загрузке. А `.text` только для чтения, поправить код при загрузке нельзя, поэтому компоновщик просит пересобрать с -fPIC, тогда обращение пойдёт через таблицу GOT

2. Сравните размеры сегментов `text` для `app_static` и `app_dynamic` (через `size`). Куда делся код функций библиотеки в динамическом варианте?

```bash
root@58b293e08827:/work# size app_static app_dynamic libmath_dynamic.so
   text	   data	    bss	    dec	    hex	filename
   2548	    656	    280	   3484	    d9c	app_static
   2517	    680	    280	   3477	    d95	app_dynamic
   1172	    424	      8	   1604	    644	libmath_dynamic.so
root@58b293e08827:/work# size -A app_static | grep -E '^\.(text|rodata|plt|plt.sec) '
.plt                   80    4128
.plt.sec               64    4224
.text                 595    4288
.rodata                61    8192
root@58b293e08827:/work# size -A app_dynamic | grep -E '^\.(text|rodata|plt|plt.sec) '
.plt                  112    4128
.plt.sec               96    4256
.text                 476    4352
.rodata                46    8192
root@58b293e08827:/work# nm -S app_static | grep -E 'dot_product|get_lib_version'
00000000000012ad 0000000000000066 T dot_product
000000000000129c 0000000000000011 T get_lib_version
root@58b293e08827:/work# nm app_dynamic | grep -E 'dot_product|get_lib_version'
                 U dot_product
                 U get_lib_version
root@58b293e08827:/work# nm -D libmath_dynamic.so | grep -E 'dot_product|get_lib_version'
000000000000110a T dot_product
00000000000010f9 T get_lib_version
root@58b293e08827:/work# readelf -d app_dynamic | grep NEEDED
 0x0000000000000001 (NEEDED)             Shared library: [libmath_dynamic.so]
 0x0000000000000001 (NEEDED)             Shared library: [libstdc++.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
root@58b293e08827:/work# objdump -d -M intel app_dynamic | grep -E 'call.*(dot_product|get_lib_version)'
    124b:	e8 60 fe ff ff       	call   10b0 <get_lib_version@plt>
    129c:	e8 ff fd ff ff       	call   10a0 <dot_product@plt>
```

> text у app_static 2548 байт, у app_dynamic 2517, разница всего 31 байт. Она маленькая, потому что сами функции маленькие: get_lib_version 0x11 = 17 байт, dot_product 0x66 = 102 байта, вместе 119. Ровно на столько уменьшилась секция `.text` (595 и 476), а `.rodata` стала меньше на 15 байт, это строка "v2.4.1-release" с '\0'. Зато в динамической версии появились заглушки для вызова этих функций (`.plt` и `.plt.sec` выросли на 32 + 32 байта) и записи в таблицах динамических символов, поэтому в сумме text почти не поменялся
>
> Код функций в динамическом варианте переехал в libmath_dynamic.so (там они T), а в app_dynamic остались только неопределённые ссылки U, запись NEEDED libmath_dynamic.so и вызовы через PLT (`call dot_product@plt`). Настоящие адреса подставит загрузчик ld-linux при запуске. На большой библиотеке экономия была бы заметнее, и ещё библиотеку можно обновить без пересборки программы

#### Этап 3. Ошибка порядка компоновки

```bash
root@58b293e08827:/work# g++ -L. -lmath_dynamic main.o -o app_wrong_order
/usr/bin/ld: main.o: in function `main':
main.cpp:(.text+0x63): undefined reference to `get_lib_version'
/usr/bin/ld: main.cpp:(.text+0xb4): undefined reference to `dot_product'
collect2: error: ld returned 1 exit status
root@58b293e08827:/work# g++ main.o -L. -lmath_dynamic -o app_correct
root@58b293e08827:/work# LD_LIBRARY_PATH=. ./app_correct
Library version: v2.4.1-release
Dot product result: 32
root@58b293e08827:/work# g++ -L. -lmath_static main.o -o app_wrong_static
/usr/bin/ld: main.o: in function `main':
main.cpp:(.text+0x63): undefined reference to `get_lib_version'
/usr/bin/ld: main.cpp:(.text+0xb4): undefined reference to `dot_product'
collect2: error: ld returned 1 exit status
root@58b293e08827:/work# g++ -v -L. -lmath_dynamic main.o -o app_wrong_order 2>&1 | grep -o -- '--as-needed' | head -1
--as-needed
root@58b293e08827:/work# g++ -Wl,--no-as-needed -L. -lmath_dynamic main.o -o app_wrong_order_noasneeded && echo OK && rm app_wrong_order_noasneeded
OK
```

**Вопросы:**
1. Шаг 1 упадёт с ошибкой `undefined reference`. Объясните, почему это происходит, используя концепцию множеств компоновщика: `U` (неразрешённые ссылки), `D` (определённые символы) и `E` (извлечённые файлы)

> Компоновщик проходит по файлам один раз слева направо и ведёт три множества: E это файлы, которые попадут в программу, U это символы, которые где-то используются, но ещё не определены, D это уже определённые символы. Объектный файл всегда добавляется в E, а его символы обновляют U и D. А из библиотеки берётся только то, что определяет что-то из текущего U, остальное пропускается, и назад к библиотеке компоновщик не возвращается
>
> При порядке `-lmath_dynamic main.o` получается так. Сначала идут служебные файлы рантайма вроде crt1.o, после них в U есть main и ещё пара символов, но dot_product и get_lib_version там нет. Дальше идёт libmath_dynamic, она определяет обе функции, но в U их нет, поэтому библиотека никому не нужна и в E не попадает. Потом main.o: он добавляется в E, main переходит в D, а get_lib_version, dot_product и функции из libstdc++ добавляются в U. libstdc++ и libc g++ сам добавляет в конец, поэтому их символы находятся. А libmath_dynamic уже пройдена, и в конце в U остаются get_lib_version и dot_product, отсюда undefined reference
>
> При правильном порядке `main.o -lmath_dynamic` к моменту, когда компоновщик доходит до библиотеки, обе функции уже лежат в U, библиотека нужна, она подключается, и U становится пустым
>
> Для статической библиотеки это обычное правило, у app_wrong_static такая же ошибка. Для .so по теории библиотека подключается целиком всегда, но в Ubuntu g++ по умолчанию передаёт компоновщику `--as-needed`. С ним .so, которая в момент обработки ничего не закрывает из U, просто выкидывается, то есть ведёт себя как архив. С `-Wl,--no-as-needed` неправильный порядок у меня собрался. Вывод такой, что библиотеки надо писать после файлов, которые их используют

#### Этап 4. Name mangling и `extern "C"`

Сделал копию заголовка и закомментировал в нём блок `extern "C"`:

```bash
root@58b293e08827:/work# cp vector_math.h vector_math.h.bak
root@58b293e08827:/work# cat vector_math.h
#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

// #ifdef __cplusplus
// extern "C" {
// #endif

extern const char* get_lib_version(void);
int dot_product(const int* a, const int* b, int n);

// #ifdef __cplusplus
// }
// #endif

#endif
root@58b293e08827:/work# g++ -c main.cpp -o main.o
root@58b293e08827:/work# g++ main.o -L. -lmath_dynamic -o app_dynamic
/usr/bin/ld: main.o: in function `main':
main.cpp:(.text+0x63): undefined reference to `get_lib_version()'
/usr/bin/ld: main.cpp:(.text+0xb4): undefined reference to `dot_product(int const*, int const*, int)'
collect2: error: ld returned 1 exit status
```

**Вопросы:**
1. Линковка упадёт с ошибкой `undefined reference`. Выполните `nm main.o | grep dot` и `nm libmath_dynamic.so | grep dot`. Как теперь выглядят имена функций `dot_product` и `get_lib_version` в объектном файле C++?

```bash
root@58b293e08827:/work# nm main.o | grep dot
                 U _Z11dot_productPKiS0_i
root@58b293e08827:/work# nm libmath_dynamic.so | grep dot
000000000000110a T dot_product
root@58b293e08827:/work# nm main.o | grep get_lib
                 U _Z15get_lib_versionv
root@58b293e08827:/work# nm libmath_dynamic.so | grep get_lib
00000000000010f9 T get_lib_version
root@58b293e08827:/work# echo _Z11dot_productPKiS0_i _Z15get_lib_versionv | c++filt
dot_product(int const*, int const*, int) get_lib_version()
```

> Без `extern "C"` компилятор C++ считает их обычными C++ функциями и искажает имена: dot_product стал `_Z11dot_productPKiS0_i`, а get_lib_version стал `_Z15get_lib_versionv`
>
> Разобрал, как это читается: `_Z` значит что имя искажено, `11dot_product` это длина имени и само имя, дальше идут типы параметров. `PKi` это const int*, `S0_` значит повторить тип, который уже был (второй const int*), `i` это int. У get_lib_version в конце `v`, то есть void, параметров нет. Так C++ поддерживает перегрузку: у функций с одним именем, но разными параметрами получаются разные символы
>
> А библиотека собрана компилятором C (gcc, там `__cplusplus` не определён), поэтому в ней имена простые: dot_product и get_lib_version. Компоновщик сравнивает имена побайтово, `_Z11dot_productPKiS0_i` не равно `dot_product`, поэтому undefined reference. Кстати, в тексте ошибки ld уже показывает имена с типами, `dot_product(int const*, int const*, int)`, это тоже подсказка, что ищется C++ функция
>
> Блок `#ifdef __cplusplus extern "C" { ... }` как раз говорит компилятору C++, что это C функции и имена искажать не надо. А компилятор C этот блок не видит, так что один заголовок подходит для обоих языков

Вернул заголовок обратно, всё снова собирается:

```bash
root@58b293e08827:/work# mv vector_math.h.bak vector_math.h
root@58b293e08827:/work# g++ -c main.cpp -o main.o
root@58b293e08827:/work# g++ main.o -L. -lmath_dynamic -o app_dynamic
root@58b293e08827:/work# LD_LIBRARY_PATH=. ./app_dynamic
Library version: v2.4.1-release
Dot product result: 32
```
