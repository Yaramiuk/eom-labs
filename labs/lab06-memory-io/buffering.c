#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("A");
    printf("B");
    printf("C");

#ifdef USE_FFLUSH
    /* Завдання 16: примусово скинути stdio buffer перед write() */
    fflush(stdout);
#endif

    write(
        STDOUT_FILENO,
        "D",
        1
    );

    return 0;
}
