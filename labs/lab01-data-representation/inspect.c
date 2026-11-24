#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static void print_binary(uint8_t value)
{
    for (int i = 7; i >= 0; --i) {
        printf("%u", (unsigned int)((value >> i) & 1u));
    }
}

static void inspect_memory(void)
{
    uint32_t value = UINT32_C(0x12345678);
    const unsigned char *bytes = (const unsigned char *)&value;
    printf("\n32-bit value: 0x%08" PRIX32 "\n", value);
    printf("Size: %zu bytes\nMemory (low to high address):\n", sizeof value);
    for (size_t i = 0; i < sizeof value; ++i) {
        printf("  %p  byte %zu: 0x%02X\n",
               (const void *)(bytes + i), i, (unsigned int)bytes[i]);
    }
    if (bytes[0] == 0x78u && bytes[3] == 0x12u) {
        puts("Byte order: Little-endian");
    } else if (bytes[0] == 0x12u && bytes[3] == 0x78u) {
        puts("Byte order: Big-endian");
    } else {
        puts("Byte order: mixed/other");
    }
}

int main(void)
{
    unsigned int input;
    printf("Enter value (0-255): ");
    if (scanf("%u", &input) != 1 || input > 255u) {
        fprintf(stderr, "Error: enter an integer from 0 to 255.\n");
        return 1;
    }
    uint8_t value = (uint8_t)input;
    printf("\nDecimal:      %" PRIu8 "\n", value);
    printf("Hexadecimal:  0x%02" PRIX8 "\n", value);
    printf("Binary:       ");
    print_binary(value);
    putchar('\n');
    if (value >= 32u && value <= 126u) {
        printf("Character:    %c\n", (int)value);
    } else {
        puts("Character:    (non-printable ASCII)");
    }
    inspect_memory();
    return 0;
}
