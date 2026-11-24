#include <stdio.h>
#include <stdlib.h>

/* .data */
int counter_initialized = 7;
/* .bss */
int counter_uninitialized;

/* .data (static, видимий лише в цьому файлі) */
static double ratio_initialized = 1.5;
/* .bss (static) */
static char buffer_uninitialized[64];

/* .text */
int square(int x)
{
    return x * x;
}

int main(void)
{
    /* string literal — .rodata */
    const char *label = "memory map";

    /* stack */
    int local = square(counter_initialized);

    /* heap */
    int *heap_value = malloc(sizeof(int));
    if (heap_value == NULL) {
        return 1;
    }
    *heap_value = local;

    printf("Code/function (square):  %p\n", (void *)square);
    printf("Code/function (main):    %p\n", (void *)main);
    printf("String literal:          %p  \"%s\"\n", (void *)label, label);
    printf("Global initialized:      %p\n", (void *)&counter_initialized);
    printf("Static initialized:      %p\n", (void *)&ratio_initialized);
    printf("Global uninitialized:    %p\n", (void *)&counter_uninitialized);
    printf("Static uninitialized:    %p\n", (void *)buffer_uninitialized);
    printf("Heap allocation:         %p\n", (void *)heap_value);
    printf("Stack/local:             %p\n", (void *)&local);

    free(heap_value);
    return 0;
}
