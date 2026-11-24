#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREADS 4

typedef struct {
    int  id;
    long start;
    long end;
    long result;
} Task;

/* Кожен thread рахує суму свого діапазону в локальну змінну sum
 * (на власному stack) і лише в кінці один раз записує її у свій Task. */
void *calculate_sum(void *arg)
{
    Task *task = arg;

    long sum = 0;

    for (long i = task->start; i <= task->end; i++) {
        sum += i;
    }

    task->result = sum;

    return NULL;
}

int main(int argc, char *argv[])
{
    long n = 1000000;

    if (argc > 1) {
        n = strtol(argv[1], NULL, 10);
        if (n < THREADS) {
            fprintf(stderr, "N must be >= %d\n", THREADS);
            return EXIT_FAILURE;
        }
    }

    pthread_t threads[THREADS];
    Task tasks[THREADS];

    long chunk = n / THREADS;

    for (int i = 0; i < THREADS; i++) {
        tasks[i].id = i + 1;
        tasks[i].start = i * chunk + 1;
        /* останній thread забирає залишок, якщо N не ділиться на 4 */
        tasks[i].end = (i == THREADS - 1) ? n : (i + 1) * chunk;
        tasks[i].result = 0;

        int rc = pthread_create(&threads[i], NULL, calculate_sum, &tasks[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", rc);
            return EXIT_FAILURE;
        }
    }

    long total = 0;

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);

        /* результат читаємо лише ПІСЛЯ join — thread гарантовано завершився */
        printf("Thread %d: %ld ... %ld -> %ld\n",
               tasks[i].id, tasks[i].start, tasks[i].end, tasks[i].result);

        total += tasks[i].result;
    }

    long expected = n * (n + 1) / 2;

    printf("\nParallel result: %ld\n", total);
    printf("Expected result: %ld\n", expected);
    printf("\nCorrect: %s\n", total == expected ? "YES" : "NO");

    return total == expected ? EXIT_SUCCESS : EXIT_FAILURE;
}
