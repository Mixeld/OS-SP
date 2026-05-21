#include "common.h"

void cancel_all_threads(void) {
    printf("\n[Cleanup] Cancelling all threads...\n");
    
    for (int i = 0; i < p_cnt; i++) {
        if (producers[i].active) {
            if (pthread_cancel(producers[i].thread) == 0) {
                printf("[Cleanup] Cancelled producer %d (thread: %lu)\n", 
                       i, producers[i].thread);
            } else {
                printf("[Cleanup] Failed to cancel producer %d\n", i);
            }
        }
    }
    
    for (int i = 0; i < c_cnt; i++) {
        if (consumers[i].active) {
            if (pthread_cancel(consumers[i].thread) == 0) {
                printf("[Cleanup] Cancelled consumer %d (thread: %lu)\n", 
                       i, consumers[i].thread);
            } else {
                printf("[Cleanup] Failed to cancel consumer %d\n", i);
            }
        }
    }
}

void join_all_threads(void) {
    printf("[Cleanup] Joining all threads (preventing zombies)...\n");
    
    for (int i = 0; i < p_cnt; i++) {
        if (producers[i].active) {
            void *retval;
            if (pthread_join(producers[i].thread, &retval) == 0) {
                printf("[Cleanup] Joined producer %d (thread: %lu)\n", 
                       i, producers[i].thread);
                producers[i].active = 0;
            } else {
                printf("[Cleanup] Failed to join producer %d\n", i);
            }
        }
    }
    
    for (int i = 0; i < c_cnt; i++) {
        if (consumers[i].active) {
            void *retval;
            if (pthread_join(consumers[i].thread, &retval) == 0) {
                printf("[Cleanup] Joined consumer %d (thread: %lu)\n", 
                       i, consumers[i].thread);
                consumers[i].active = 0;
            } else {
                printf("[Cleanup] Failed to join consumer %d\n", i);
            }
        }
    }
    
    p_cnt = 0;
    c_cnt = 0;
}

void cleanup_all(SharedQueue *q) {
    if (!q) return;
    
    printf("\n[Cleanup] Starting full cleanup...\n");
    
    keep_running = 0;
    
    printf("[Cleanup] Unlocking semaphores...\n");
    for (int i = 0; i < MAX_THREADS * 2; i++) {
        sem_post(&q->empty);
        sem_post(&q->full);
    }
    
    usleep(100000);
    
    cancel_all_threads();
    usleep(50000);
    join_all_threads();
    
    queue_destroy(q);
    
    printf("[Cleanup] All resources cleaned up successfully!\n");
}