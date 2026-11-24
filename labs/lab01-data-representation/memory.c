#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint32_t value = UINT32_C(0x12345678);
    const unsigned char *p = (const unsigned char *)&value;
    printf("value = 0x%08" PRIX32 "\n", value);
    for (size_t i = 0; i < sizeof value; ++i) {
        printf("Address: %p  byte %zu = 0x%02X\n",
               (const void *)(p + i), i, (unsigned int)p[i]);
    }
    return 0;
}
