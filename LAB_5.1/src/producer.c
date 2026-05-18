#include "common.h"

void* run_producer(void *arg) {
    SharedQueue *q = (SharedQueue*)arg;
    srand(time(NULL) ^ pthread_self());
    
    // Разрешаем отмену потока
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    while (keep_running) {
        Message msg;
        create_random_message(&msg);

        // Пытаемся получить свободное место
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;  // Таймаут 1 секунда
        
        if (sem_timedwait(&q->empty, &ts) != 0) {
            if (!keep_running) break;
            continue;  // Таймаут, пробуем снова
        }
        
        if (!keep_running) {
            sem_post(&q->empty);
            break;
        }
        
        pthread_mutex_lock(&q->mutex);
        
        if (q->current_load < q->queue_size) {
            queue_push(q, &msg);
            pthread_mutex_unlock(&q->mutex);
            sem_post(&q->full);
        } else {
            pthread_mutex_unlock(&q->mutex);
            sem_post(&q->empty);
        }

        // Задержка с проверкой флага
        for (int i = 0; i < (rand() % 2 + 1) && keep_running; i++) {
            sleep(1);
        }
    }
    
    printf("[Producer %lu] Shutting down...\n", pthread_self());
    return NULL;
}