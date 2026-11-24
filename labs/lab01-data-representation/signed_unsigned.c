#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static void print_bits(uint8_t value)
{
    for (int i = 7; i >= 0; --i) {
        printf("%u", (unsigned int)((value >> i) & 1u));
    }
}

int main(void)
{
    int8_t signed_value = -1;
    uint8_t unsigned_value = 255;
    uint8_t signed_bits = (uint8_t)signed_value;

    printf("int8_t value:   %" PRId8 "\n", signed_value);
    printf("uint8_t value:  %" PRIu8 "\n", unsigned_value);
    printf("signed hex:     0x%02X\n", (unsigned int)signed_bits);
    printf("unsigned hex:   0x%02X\n", (unsigned int)unsigned_value);
    printf("same bits:      ");
    print_bits(unsigned_value);
    putchar('\n');
    return 0;
}
