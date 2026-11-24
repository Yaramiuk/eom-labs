#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    long n = 1000000;

    if (argc > 1) {
        n = strtol(argv[1], NULL, 10);
    }

    long sum = 0;

    for (long i = 1; i <= n; i++) {
        sum += i;
    }

    long expected = n * (n + 1) / 2;

    printf("Sequential result: %ld\n", sum);
    printf("Expected result:   %ld\n", expected);
    printf("Correct: %s\n", sum == expected ? "YES" : "NO");

    return EXIT_SUCCESS;
}
