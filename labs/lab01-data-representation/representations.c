#include <stdio.h>

int main(void)
{
    const unsigned int values[] = {10u, 65u, 127u, 255u, 256u, 1024u};
    for (size_t i = 0; i < sizeof values / sizeof values[0]; ++i) {
        printf("Decimal: %u  Hexadecimal: 0x%X\n", values[i], values[i]);
    }
    return 0;
}
