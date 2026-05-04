#include "common.h"

void run_consumer(int semid, SharedQueue *q) {
    signal(SIGTERM, handle_sig);

    while (keep_running) {
        sem_wait(semid, SEM_FULL);
        if (!keep_running) break;
        sem_wait(semid, SEM_MUTEX);

        Message msg = queue_pop(q);

        sem_signal(semid, SEM_MUTEX);
        sem_signal(semid, SEM_EMPTY);

        uint16_t check = calculate_DJB2(&msg);
        if (check == msg.hash) 
            printf("[Consumer %d] OK: Hash match\n", getpid());
        else 
            printf("[Consumer %d] ERROR: Hash mismatch!\n", getpid());

        sleep(rand() % 2 + 1);
    }
    exit(0);
}