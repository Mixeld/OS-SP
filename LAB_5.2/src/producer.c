#include "common.h"

void* run_producer(void *arg) {
    SharedQueue *q = (SharedQueue*)arg;
    srand(time(NULL) ^ (unsigned long)pthread_self());
    
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    long produced = 0;
    
    while (1) {
        // Проверяем флаг завершения
        pthread_mutex_lock(&global_mutex);
        int running = keep_running;
        pthread_mutex_unlock(&global_mutex);
        
        if (!running) break;
        
        Message msg;
        create_random_message(&msg);
        
        if (queue_push(q, &msg) == 0) {
            produced++;
        }
        
        // Имитация разной скорости работы
        for (int i = 0; i < (rand() % 2 + 1) && keep_running; i++) {
            sleep(1);
        }
    }
    
    // Сохраняем статистику
    for (int i = 0; i < MAX_THREADS; i++) {
        if (producers[i].active && producers[i].thread == pthread_self()) {
            producers[i].produced_count = produced;
            break;
        }
    }
    
    printf("[Producer %lu] Shutting down... Produced: %ld messages\n", 
           pthread_self(), produced);
    return NULL;
}