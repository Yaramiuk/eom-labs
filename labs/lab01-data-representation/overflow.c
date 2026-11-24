#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint8_t value = 254;
    printf("254       = %" PRIu8 "\n", value);
    ++value;
    printf("255       = %" PRIu8 "\n", value);
    ++value;
    printf("255 + 1   = %" PRIu8 "\n", value);
    ++value;
    printf("255 + 2   = %" PRIu8 "\n", value);
    return 0;
}
