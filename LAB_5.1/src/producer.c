#include "common.h"

void* run_producer(void *arg) {
    SharedQueue *q = (SharedQueue*)arg;
    srand(time(NULL) ^ (unsigned long)pthread_self());

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    while (keep_running) {
        Message msg;
        create_random_message(&msg);

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;

        if (sem_timedwait(&q->empty, &ts) != 0) {
            if (!keep_running) break;
            continue;
        }

        if (!keep_running) {
            sem_post(&q->empty);
            break;
        }

        if (queue_push(q, &msg) != 0) {
            sem_post(&q->empty);
        } else {
            sem_post(&q->full);
        }

        for (int i = 0; i < (rand() % 2 + 1) && keep_running; i++) {
            sleep(1);
        }
    }

    printf("[Producer %lu] Shutting down...\n", pthread_self());
    return NULL;
}