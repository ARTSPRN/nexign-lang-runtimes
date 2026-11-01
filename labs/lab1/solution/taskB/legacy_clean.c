#include <stdio.h>

#define SIZE 1024
#define ENCODED 896 // df делает 128 итераций по 7 операций

// ожидаемые значения для первых 51 символа (флаг и '\n'),
static const int expected_start[] = {
    5,    92,   1552, 412,  6765, 202,  139,  0,    95,   1760, 460,
    5775, 197,  145,  17,   85,   1600, 380,  5665, 202,  162,  10,
    94,   1648, 380,  6105, 207,  152,  26,   111,  1856, 416,  5555,
    192,  142,  10,   94,   1552, 456,  6655, 192,  94,   87,   4,
    784,  200,  3025, 145,  99,   30,   58};

#define START_LEN ((int)(sizeof(expected_start) / sizeof(expected_start[0])))

// то же, что df() делает с одним элементом
int encode(int x, int pos) {
    if (pos >= ENCODED)
        return x; // последние 128 элементов df не трогает
    switch (pos % 7) {
    case 0: return x ^ 0x63;
    case 1: return x ^ 48;
    case 2: return x << 4;
    case 3: return x << 2;
    case 4: return x * 55;
    case 5: return x + 97; // в оригинале *p -= -97
    case 6: return x + 44;
    }
    return x;
}

// дальше флага в массиве a[] лежат просто зашифрованные нули
int expected(int pos) {
    if (pos < START_LEN)
        return expected_start[pos];
    return encode(0, pos);
}

int main(void) {
    static char buf[SIZE]; // static, чтобы был заполнен нулями, как в оригинале

    if (fgets(buf, SIZE, stdin) == NULL)
        buf[0] = '\0';

    for (int i = 0; i < SIZE; i++) {
        int x = encode(buf[i], i);
        if (x < 0) // в оригинале ip() для отрицательного числа даёт пустой список, то есть 0
            x = 0;
        if (x != expected(i)) {
            printf("WRONG\n");
            return 1;
        }
    }

    printf("OK, flag is correct\n");
    return 0;
}
