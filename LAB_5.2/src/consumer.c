#include "common.h"

void* run_consumer(void *arg) {
    SharedQueue *q = (SharedQueue*)arg;
    
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    long consumed = 0;
    
    while (1) {
        // Проверяем флаг завершения
        pthread_mutex_lock(&global_mutex);
        int running = keep_running;
        pthread_mutex_unlock(&global_mutex);
        
        if (!running) break;
        
        Message msg = queue_pop(q);
        
        // Проверяем, не пустое ли сообщение (при завершении)
        if (msg.size == 0 && msg.type == 0 && msg.hash == 0 && !keep_running) {
            break;
        }
        
        consumed++;
        
        uint16_t check = calculate_DJB2(&msg);
        if (check == msg.hash)
            printf("[Consumer %lu] OK: Hash match\n", pthread_self());
        else
            printf("[Consumer %lu] ERROR: Hash mismatch! (calc=%u, stored=%u)\n", 
                   pthread_self(), check, msg.hash);
        
        // Имитация разной скорости работы
        for (int i = 0; i < (rand() % 2 + 1) && keep_running; i++) {
            sleep(1);
        }
    }
    
    // Сохраняем статистику
    for (int i = 0; i < MAX_THREADS; i++) {
        if (consumers[i].active && consumers[i].thread == pthread_self()) {
            consumers[i].consumed_count = consumed;
            break;
        }
    }
    
    printf("[Consumer %lu] Shutting down... Consumed: %ld messages\n", 
           pthread_self(), consumed);
    return NULL;
}