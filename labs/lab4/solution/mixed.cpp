#include <cstddef>
#include <iostream>

struct Mixed {
    char a;
    int b;
    char c;
    double d;
    short e;
};

struct __attribute__((packed)) PackedMixed {
    char a;
    int b;
    char c;
    double d;
    short e;
};

int main() {
    PackedMixed mixed{'a', 42, 'c', 1.5, 2};
    const double* address = &mixed.d;

    std::cout << "Mixed: " << sizeof(Mixed) << " bytes, alignment " << alignof(Mixed) << '\n';
    std::cout << "PackedMixed: " << sizeof(PackedMixed) << " bytes, alignment "
              << alignof(PackedMixed) << '\n';
    std::cout << "Fields: " << sizeof(mixed.a) + sizeof(mixed.b) + sizeof(mixed.c)
                              + sizeof(mixed.d) + sizeof(mixed.e) << " bytes\n";
    std::cout << "Offset of d: " << offsetof(Mixed, d) << " / "
              << offsetof(PackedMixed, d) << '\n';
    std::cout << "Address of packed d: " << static_cast<const void*>(address) << '\n';
}
