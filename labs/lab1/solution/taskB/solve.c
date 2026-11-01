// solve.c - достаёт флаг из чисел массива a[] из legacy.c
// числа берутся из expected.txt (их печатает dump.c), по одному в строке
// запуск: ./solve < expected.txt
#include <stdio.h>

// df() делает с i-м символом одну из 7 операций по кругу,
// тут то же самое, только наоборот
int undo(int y, int pos) {
    switch (pos % 7) {
    case 0: return y ^ 0x63;  // было x ^= 0x63
    case 1: return y ^ 48;    // было x ^= 48
    case 2: return y >> 4;    // было x <<= 4
    case 3: return y >> 2;    // было x <<= 2
    case 4: return y / 55;    // было x *= 55
    case 5: return y - 97;    // было x -= -97
    case 6: return y - 44;    // было x += 44
    }
    return 0;
}

int main(void) {
    int y;
    int pos = 0;
    while (scanf("%d", &y) == 1) {
        int c = undo(y, pos);
        if (c == 0 || c == '\n') // строка кончилась
            break;
        putchar(c);
        pos++;
    }
    putchar('\n');
    return 0;
}
