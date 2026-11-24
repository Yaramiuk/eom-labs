#include <stdio.h>

int main(void)
{
    printf("char:      %zu bytes (%zu bits)\n", sizeof(char), sizeof(char) * 8u);
    printf("short:     %zu bytes (%zu bits)\n", sizeof(short), sizeof(short) * 8u);
    printf("int:       %zu bytes (%zu bits)\n", sizeof(int), sizeof(int) * 8u);
    printf("long:      %zu bytes (%zu bits)\n", sizeof(long), sizeof(long) * 8u);
    printf("long long: %zu bytes (%zu bits)\n", sizeof(long long), sizeof(long long) * 8u);
    printf("float:     %zu bytes (%zu bits)\n", sizeof(float), sizeof(float) * 8u);
    printf("double:    %zu bytes (%zu bits)\n", sizeof(double), sizeof(double) * 8u);
    printf("void *:    %zu bytes (%zu bits)\n", sizeof(void *), sizeof(void *) * 8u);
    return 0;
}
