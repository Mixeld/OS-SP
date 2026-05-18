#include "common.h"

void* run_consumer(void *arg) {
    SharedQueue *q = (SharedQueue*)arg;
    
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    while (keep_running) {
        // Пытаемся получить сообщение с таймаутом
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;  // Таймаут 1 секунда
        
        if (sem_timedwait(&q->full, &ts) != 0) {
            if (!keep_running) break;
            continue;  // Таймаут, пробуем снова
        }
        
        if (!keep_running) {
            sem_post(&q->full);
            break;
        }
        
        pthread_mutex_lock(&q->mutex);
        
        Message msg = queue_pop(q);
        
        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->empty);

        // Проверка целостности сообщения по хешу
        uint16_t check = calculate_DJB2(&msg);
        if (check == msg.hash) 
            printf("[Consumer %lu] OK: Hash match\n", pthread_self());
        else 
            printf("[Consumer %lu] ERROR: Hash mismatch!\n", pthread_self());

        // Задержка с проверкой флага
        for (int i = 0; i < (rand() % 2 + 1) && keep_running; i++) {
            sleep(1);
        }
    }
    
    printf("[Consumer %lu] Shutting down...\n", pthread_self());
    return NULL;
}