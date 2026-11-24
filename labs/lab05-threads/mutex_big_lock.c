#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/* Завдання 9: lock навколо всього циклу (велика critical section). */
#define ITERATIONS 1000000

long counter = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *increment(void *arg)
{
    int id = *(int *)arg;

    pthread_mutex_lock(&mutex);
    printf("Thread %d: acquired mutex, counter=%ld\n", id, counter);

    for (long i = 0; i < ITERATIONS; i++) {
        counter++;
    }

    printf("Thread %d: releasing mutex, counter=%ld\n", id, counter);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(void)
{
    pthread_t threads[2];
    int ids[2] = {1, 2};

    for (int i = 0; i < 2; i++) {
        pthread_create(&threads[i], NULL, increment, &ids[i]);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Expected: %ld\n", 2L * ITERATIONS);
    printf("Actual:   %ld\n", counter);

    pthread_mutex_destroy(&mutex);

    return EXIT_SUCCESS;
}
