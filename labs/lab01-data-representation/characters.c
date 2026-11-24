#include <stdio.h>

int main(void)
{
    const char values[] = {'A', 'B', 'a', '0', '9'};
    for (size_t i = 0; i < sizeof values / sizeof values[0]; ++i) {
        unsigned char code = (unsigned char)values[i];
        printf("Character: %c  Decimal: %u  Hex: 0x%02X\n",
               values[i], (unsigned int)code, (unsigned int)code);
    }
    return 0;
}
