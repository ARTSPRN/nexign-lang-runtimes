
## Задача B. Наследие

#### Что лежит в артефакте

```bash
root@58b293e08827:/work# ls -l legacy.c
-rw-r--r-- 1 root root 280627 Nov  1 20:09 legacy.c
root@58b293e08827:/work# wc -l legacy.c
4476 legacy.c
root@58b293e08827:/work# sed -n '11,27p' legacy.c

#include<stdio.h>
#include<stdlib.h>
#define A static
#define B malloc
#define C case
#define D(x) sizeof(x)
#define E(x) return x
#define F int
#define G char
#define H fgets
#define I stdin
#define P(x,y) x y
#define R register
#define S struct s
#define T(x, y) x##y
#define U(x, y) x
```

> Файл на 274 КБ, почти весь состоит из `z(z(z(...)))` и закрывающих скобок. В начале комментарий "You are not expected to understand this", это известная цитата из исходников Unix V6. Дальше идут макросы, которые прячут обычные слова C за одной буквой: A это static, E(x) это return x, F это int, S это struct s и так далее. Руками такое читать бесполезно, поэтому я решил отдать это препроцессору, как в первом этапе задачи A

#### Пробую собрать

```bash
root@58b293e08827:/work# gcc legacy.c -o legacy
legacy.c: In function 'df':
legacy.c:40:23: warning: type defaults to 'int' in declaration of 'n' [-Wimplicit-int]
   40 | P(A,P(V,df))(F*p){P(R,n)=128;switch(1024%8){P(C,0):do{*p^=0x63
      |                       ^
legacy.c:23:18: note: in definition of macro 'P'
   23 | #define P(x,y) x y
      |                  ^
root@58b293e08827:/work# clang legacy.c -o legacy_clang
...
legacy.c:40:23: error: type specifier missing, defaults to 'int'; ISO C99 and later do not support implicit int [-Wimplicit-int]
...
legacy.c:60:28: fatal error: bracket nesting level exceeded maximum of 256
   60 | z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(z(
      |                            ^
legacy.c:60:28: note: use -fbracket-depth=N to increase maximum nesting level
1 warning and 2 errors generated.
root@58b293e08827:/work# echo "hello" | ./legacy; echo "exit=$?"
exit=1
```

> gcc собрал с одним предупреждением: у `register n` не указан тип, это старый implicit int. clang собирать отказался, потому что implicit int запрещён с C99, а вложенность скобок больше 256 (потом выяснилось, что самое большое число в массиве 6765, значит и вложенность такая). В gcc 14 и новее implicit int тоже уже ошибка. Так что в таком виде программа собирается только старым компилятором
>
> На экран программа ничего не печатает, только возвращает код. На случайную строку код 1

#### Препроцессинг и разбор

```bash
root@58b293e08827:/work# gcc -E -P legacy.c -o legacy.i
root@58b293e08827:/work# wc -l legacy.i
5208 legacy.i
root@58b293e08827:/work# tail -c 200 legacy.c | tr -cd '!' | wc -c
23
```

Прогнал `legacy.i` через clang-format, вырезал огромный массив, немного ужал и подписал функции. Получилось так:

```c
struct s {
    struct s *s;
};
static struct s *z(struct s *n) {      // добавляет элемент в начало списка
    struct s *t = n;
    n = malloc(sizeof(struct s));
    n->s = t;
    return n;
}
static int pi(struct s *n) {           // длина списка, нигде не вызывается
    int x = 0;
    for (; n; n = n->s, x++)
        ;
    return x;
}
static struct s *ip(int n) {           // число в список длины n
    struct s *s = 0;
    for (int i = 0; i < n; i++) {
        s = z(s);
    }
    return s;
}
static int pc(struct s *n, struct s *o) {   // 1 если длины разные
    for (; n && o; n = n->s, o = o->s)
        ;
    return n || o;
}
static void df(int *p) {               // "шифрование"
    register n = 128;
    switch (1024 % 8) {
    case 0:
        do {
            *p ^= 0x63; p++;
        case 7:
            *p ^= 48;   p++;
        case 6:
        case 5:
            *p <<= 4;   p++;
        case 4:
            *p <<= 2;   p++;
        case 3:
            *p *= 55;   p++;
        case 2:
            *p -= -97;  p++;
        case 1:
            *p += 44;   p++;
        } while (--n > 0);
    }
}
static void dc(int *d, char *s) {      // копирует char[] в int[]
    register int n = (1024 + 7) / 8;
    switch (1024 % 8) {
    case 0:
        do {
            *d++ = *s++;
        case 7: *d++ = *s++;
        case 6: *d++ = *s++;
        case 5: *d++ = *s++;
        case 4: *d++ = *s++;
        case 3: *d++ = *s++;
        case 2: *d++ = *s++;
        case 1: *d++ = *s++;
        } while (--n > 0);
    }
}
static int ch(int *n) {                // проверка
    struct s *a[1024] = {/* огромный массив z(z(z(...))) */};
    struct s *p;
    for (int i = 0; i < 1024; i++) {
        p = ip(i[n]);
        if (pc(p, i[a])) {
            return 0;
        }
    }
    return 1;
}
int main(void) {
    static char b[1024];
    static int i[1024];
    fgets(b, 1024, stdin);
    dc(i, b);
    df(i);
    return !!!!!!!!!!!!!!!!!!!!!!!ch(i);
}
```

> Числа тут хранятся как связные списки: `z(x)` добавляет один элемент, поэтому `z(z(z(0)))` это список длины 3, то есть просто число 3. Массив `a` это 1024 таких числа. `ip` превращает число в список, `pc` сравнивает длины двух списков
>
> main читает строку через fgets в `b` (он static, так что всё после строки заполнено нулями), копирует символы в `int i[1024]` функцией `dc`, шифрует их функцией `df` и сравнивает с массивом `a` в `ch`
>
> `dc` и `df` написаны через Duff's device, это когда switch прыгает внутрь do-while. Так как 1024 % 8 = 0, прыжок всегда на case 0 и это просто обычные циклы. В `dc` 128 итераций по 8 копирований, то есть 1024 символа. А в `df` у case 6 и case 5 общее тело, поэтому за одну итерацию делается 7 операций, и меняются только первые 128 * 7 = 896 элементов
>
> Какая операция применится к символу, зависит от `i % 7`: ^0x63, ^48, <<4, <<2, *55, +97 (там написано `-= -97`), +44
>
> Восклицательных знаков 23, это нечётное число, значит `!!...!ch(i)` это просто `!ch(i)`. Программа возвращает 0 если строка верная и 1 если нет
>
> Получается, что в массиве `a` лежит зашифрованная правильная строка, и чтобы её найти, надо достать оттуда числа и сделать обратные операции

#### Достаю числа из массива

Считать скобки вручную нереально, поэтому я воспользовался функцией `pi`, которая уже есть в коде, но не используется. Сделал копию `dump.c` и заменил там проверку на печать длины каждого списка:

```bash
root@58b293e08827:/work# cp legacy.c dump.c
root@58b293e08827:/work# diff legacy.c dump.c
4474c4474
< p=ip(i[n]);if(pc(p,i[a])){E(0);}}E(1);}P(F,main)(V){P(P(A,G),b
---
> printf("%d\n",pi(i[a]));}E(1);}P(F,main)(V){P(P(A,G),b
root@58b293e08827:/work# gcc dump.c -o dump 2>/dev/null
root@58b293e08827:/work# echo "" | ./dump > expected.txt
root@58b293e08827:/work# wc -l expected.txt
1024 expected.txt
root@58b293e08827:/work# head -60 expected.txt | tr '\n' ' '; echo
5 92 1552 412 6765 202 139 0 95 1760 460 5775 197 145 17 85 1600 380 5665 202 162 10 94 1648 380 6105 207 152 26 111 1856 416 5555 192 142 10 94 1552 456 6655 192 94 87 4 784 200 3025 145 99 30 58 0 0 0 97 44 99 48 0 0 
root@58b293e08827:/work# sort -n expected.txt | tail -1
6765
```

> Первые примерно 50 чисел разные, а дальше повторяется `0 0 0 97 44 99 48`. Это зашифрованные нули из хвоста буфера: 0<<4 = 0, 0<<2 = 0, 0*55 = 0, 0+97 = 97, 0+44 = 44, 0^0x63 = 99, 0^48 = 48

#### Расшифровка

Каждую операцию из `df` можно сделать в обратную сторону:

```
i % 7 = 0:  x ^= 0x63   ->  x = y ^ 0x63
i % 7 = 1:  x ^= 48     ->  x = y ^ 48
i % 7 = 2:  x <<= 4     ->  x = y >> 4
i % 7 = 3:  x <<= 2     ->  x = y >> 2
i % 7 = 4:  x *= 55     ->  x = y / 55
i % 7 = 5:  x -= -97    ->  x = y - 97
i % 7 = 6:  x += 44     ->  x = y - 44
```

> Проверил на первых числах: 5 ^ 0x63 = 0x66 это 'f', 92 ^ 48 = 108 это 'l', 1552 >> 4 = 97 это 'a', 412 >> 2 = 103 это 'g', 6765 / 55 = 123 это '{'. Начало похоже на флаг, так что написал `solve.c`, который делает это для всех чисел

```bash
root@58b293e08827:/work# gcc -Wall solve.c -o solve
root@58b293e08827:/work# ./solve < expected.txt
flag{i_considered_giving_only_the_binary_24412707}
root@58b293e08827:/work# ./solve < expected.txt > flag.txt
```

#### Проверка на оригинале

```bash
root@58b293e08827:/work# cat flag.txt | ./legacy; echo "exit=$?"
exit=0
root@58b293e08827:/work# echo "flag{i_considered_giving_only_the_binary_24412708}" | ./legacy; echo "exit=$?"
exit=1
```

> Оригинальная программа флаг принимает (код 0), а если поменять последнюю цифру, уже нет. Ещё момент: строка должна заканчиваться переводом строки. fgets кладёт '\n' в буфер, и он тоже есть в массиве (число 58 это '\n' ^ 48), поэтому если передать флаг через `printf '%s'` без \n, проверка не пройдёт

#### Рабочая версия программы

Так как оригинал не собирается clang и новыми gcc, я переписал его в `legacy_clean.c`. Логика та же, но без макросов и списков, ожидаемые значения записаны обычными числами, и программа пишет OK или WRONG. Код возврата как у оригинала

```bash
root@58b293e08827:/work# gcc -Wall -Wextra legacy_clean.c -o legacy_clean
root@58b293e08827:/work# clang -Wall legacy_clean.c -o legacy_clean_clang
root@58b293e08827:/work# cat flag.txt | ./legacy_clean; echo "exit=$?"
OK, flag is correct
exit=0
root@58b293e08827:/work# echo "flag{wrong}" | ./legacy_clean; echo "exit=$?"
WRONG
exit=1
```

> Собирается и gcc, и clang без предупреждений
