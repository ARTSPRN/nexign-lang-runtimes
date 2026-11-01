## Задача A. Do It Yourself

#### Этап 1. Препроцессинг (simple.i)

**Вопросы:**
1. Сколько строк было в исходном файле и сколько стало в `.i`? 

```bash
root@58b293e08827:/work# wc -l simple.cpp simple.i
   23 simple.cpp
 1073 simple.i
 1096 total
```

2. Найдите в `simple.i` функцию `main`. Что находится **над** ней в файле? Почему там так много строк?

> Над main находятся раскрытые подключения заголовочных файлов, в первую очередь содержимое cstdio, внутренние заголовки стандартной библиотеки, объявления типов, макросов, функций, служебные директивы препроцессора. Строк много, потому что один маленький cstdio тянет за собой цепочку системных заголовков, а препроцессор подставляет их целиком прямо в файл

3. Найдите в `simple.i` строку `"Result: %d, Counter: %d, Magic: %d\n"`. Где она оказалась и что с ней сделал препроцессор?

> Она осталась внутри main, то есть препроцессор её не изменял. Препроцессор cначала раскрывает include, затем раскрывает макросы, затем убирает комментарии, затем добавляет служебную информацию о строках, но не модифицирует строковые литералы внутри кода

#### Этап 2. Компиляция и оптимизации (simple.s)

**Вопросы:**
4. Сравните размеры файлов `-O0` и `-O2`. Во сколько раз оптимизированный код короче?

```bash
root@58b293e08827:/work# wc -l simple_O0.s simple_O2.s
 144 simple_O0.s
  97 simple_O2.s
 241 total
root@58b293e08827:/work# O0=$(wc -l < simple_O0.s)
O2=$(wc -l < simple_O2.s)
awk -v o0="$O0" -v o2="$O2" 'BEGIN { printf "O0=%d, O2=%d, ratio=%.2f\n", o0, o2, o0/o2 }'
O0=144, O2=97, ratio=1.48
```

> Обычно -O2 делает код короче за счёт инлайн подстановки, удаления лишнего кода, сворачивания констант и упрощения вызовов

5. Откройте файл, полученный после сборки без оптимизации. Найдите метку `square:`. Какие инструкции там используются для умножения?

```bash
root@58b293e08827:/work# sed -n '/square/,/ret/p' simple_O0.s | head -n 30
	.globl	_Z6squarei
	.type	_Z6squarei, @function
_Z6squarei:
.LFB2:
	.cfi_startproc
	endbr64
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	DWORD PTR -4[rbp], edi
	mov	eax, DWORD PTR -4[rbp]
	imul	eax, eax
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.size	_Z6squarei, .-_Z6squarei
	.globl	_Z14sum_of_squaresii
	.type	_Z14sum_of_squaresii, @function
_Z14sum_of_squaresii:
.LFB3:
	.cfi_startproc
	endbr64
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	push	rbx
```

> Для умножения была использована инструкция imul. По определению asm это знаковое умножение для целых чисел

6. Найдите метку `main:`. Какие регистры используются для передачи аргументов в вызов `sum_of_squares`? Как это связано с соглашениями о вызовах?

```bash
root@58b293e08827:/work# sed -n '/^main:/,/ret/p' simple_O0.s | less
main:
.LFB5:
        .cfi_startproc
        endbr64
        push    rbp
        .cfi_def_cfa_offset 16
        .cfi_offset 6, -16
        mov     rbp, rsp
        .cfi_def_cfa_register 6
        sub     rsp, 16
        mov     esi, 4
        mov     edi, 3
        call    _Z14sum_of_squaresii
        mov     DWORD PTR -4[rbp], eax
        call    _Z12bump_counterv
        mov     edx, DWORD PTR global_counter[rip]
        mov     eax, DWORD PTR -4[rbp]
        mov     ecx, 42
        mov     esi, eax
        lea     rax, .LC0[rip]
        mov     rdi, rax
        mov     eax, 0
        call    printf@PLT
        mov     eax, 0
        leave
        .cfi_def_cfa 7, 8
        ret
```

> В функции main перед вызовом sum_of_squares аргументы передаются через регистры edi и esi: сначала выполняется mov edi, 3, затем mov esi, 4. Это соответствует соглашению о вызовах System V AMD64 ABI, используемому в 64-битном Linux. По этому соглашению первые целочисленные аргументы передаются через регистры rdi, rsi, rdx, rcx, r8, r9. Так как передаются значения типа int, используются 32-битные части этих регистров: edi и esi. Возвращаемое значение функции возвращается через регистр eax

7. Во втором файле (с `-O2`) функция `square` скорее всего **исчезла**. Почему? 

> Потому что компилятор на -O2 видит, что функция маленькая и может подставить её тело прямо в место вызова, может вычислить результат заранее, удаляет лишние вызовы. Эти процессы называются: inlining, constant folding, constant propagation, dead code elimination. В итоге отдельная функция square может стать не нужна, и компилятор её не оставляет в ассемблере

#### Этап 3. Ассемблирование в объектный файл

**Вопросы:**
8. Почему `MAGIC` имеет тип `R` (read-only data), а не `D` (data)?

> Потому что MAGIC объявлена как константа (aka данные только для чтения). В объектном файле такие данные попадают в секцию rodata. Поэтому nm и помечает символ как R - read-only data. А тип D обычно означает обычные инициализированные изменяемые данные

9. Что показывает команда `size`? Какие секции занимают больше всего места и почему?

```bash
root@58b293e08827:/work# size -A simple.o
simple.o  :
section              size   addr
.text                 172      0 # Машинный код функций
.data                   0      0 # Инициализированные изменяемые глобальные переменные
.bss                    4      0 # Неинициализированные или нулевые глобальные переменные
.rodata                44      0 # Секция данных только для чтения
.comment               46      0 # Служебная секция с комментарием компилятора
.note.GNU-stack         0      0 # Служебная секция, связанная с атрибутами стека
.note.gnu.property     32      0 # Служебная секция с свойствами бинарника
.eh_frame             152      0 # Информация для раскрутки стека и обработки исключений
Total                 450
```

> Больше всего места занимает секция `.text` - 172 байта, потому что в ней находится машинный код всех функций программы: `square`, `sum_of_squares`,`bump_counter` и `main`. Второй по размеру является служебная секция `.eh_frame` - 152 байта. Она содержит информацию для обработки исключений и раскрутки стека. Секция `.rodata` занимает 44 байта и хранит данные только для чтения: строку формата для `printf` и, вероятно, константу `MAGIC`. Секция `.bss` занимает 4 байта, потому что в ней размещается глобальная переменная `global_counter` типа `int`. Секция `.data` имеет размер 0, потому что переменная `global_counter` инициализирована нулём и компилятор помещает её в `.bss`, а не в `.data` 

#### Этап 4. Компоновка и запуск

**Вопросы:**
10. Какой результат вывела программа? 

```bash
root@58b293e08827:/work# g++ simple.o -o simple
root@58b293e08827:/work# file simple
simple: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=8fcd188558fe5da4213c93062d6b2d47f313c9b5, for GNU/Linux 3.2.0, not stripped
root@58b293e08827:/work# ./simple
Result: 25, Counter: 1, Magic: 42
```