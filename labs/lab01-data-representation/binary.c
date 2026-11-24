#include <stdio.h>

void print_binary(unsigned char value)
{
    for (int i = 7; i >= 0; --i) {
        printf("%u", (unsigned int)((value >> i) & 1u));
    }
}

int main(void)
{
    const unsigned int values[] = {0u, 1u, 10u, 65u, 127u, 128u, 255u};
    for (size_t i = 0; i < sizeof values / sizeof values[0]; ++i) {
        unsigned char value = (unsigned char)values[i];
        printf("Decimal: %3u  Hex: 0x%02X  Binary: ", (unsigned int)value,
               (unsigned int)value);
        print_binary(value);
        putchar('\n');
    }
    return 0;
}
