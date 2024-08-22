#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *thread_test(void *arg) {
    for (int i = 0; i < 10; i++) {
        size_t size = rand() % 1024; // случайный размер блока
        char *ptr = malloc(size);
        if (ptr == NULL) {
            perror("malloc failed");
            pthread_exit(NULL);
        }
        memset(ptr, 0, size); // заполнить блок нулями
        printf("Thread %ld: Allocated %zu bytes\n", (long)arg, size);
        free(ptr);
        printf("Thread %ld: Freed %zu bytes\n", (long)arg, size);
    }
    pthread_exit(NULL);
}

int main() {
    const int NUM_THREADS = 5;
    pthread_t threads[NUM_THREADS];

    // Инициализация генератора случайных чисел
    srand(time(NULL));

    // Создание потоков
    for (long i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, thread_test, (void *)i) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    // Ожидание завершения потоков
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads have finished execution.\n");
    return 0;
}