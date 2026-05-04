#include "common.h"

void run_producer(int semid, SharedQueue *q) {
    srand(getpid());
    signal(SIGTERM, handle_sig); // Реакция на завершение

    while (keep_running) {
        Message msg;
        create_random_message(&msg);

        sem_wait(semid, SEM_EMPTY);
        if (!keep_running) break;
        sem_wait(semid, SEM_MUTEX);

        queue_push(q, &msg);

        sem_signal(semid, SEM_MUTEX);
        sem_signal(semid, SEM_FULL);

        sleep(rand() % 2 + 1); // Задержка 1-2 сек
    }
    exit(0);
}