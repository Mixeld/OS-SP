#include "common.h"

// Определение глобальных переменных
volatile sig_atomic_t keep_running = 1;
ThreadInfo producers[MAX_THREADS];
ThreadInfo consumers[MAX_THREADS];
int p_cnt = 0;
int c_cnt = 0;

void reset_keep_running(void) {
    keep_running = 1;
}

void handle_sigterm(int sig) {
    (void)sig;
    keep_running = 0;
}

void handle_sigint(int sig) {
    (void)sig;
    printf("\n[Parent] Received Ctrl+C, shutting down...\n");
    keep_running = 0;
}

uint16_t calculate_DJB2(Message *msg) {
    unsigned long hash = 5381;
    hash = ((hash << 5) + hash) + msg->type;
    hash = ((hash << 5) + hash) + 0;
    hash = ((hash << 5) + hash) + 0;
    hash = ((hash << 5) + hash) + msg->size;
    
    for (int i = 0; i < (msg->size + 1); i++) {
        hash = ((hash << 5) + hash) + msg->data[i];
    }
    
    return (uint16_t)hash;
}

void create_random_message(Message *msg) {
    msg->type = rand() % 256;
    msg->size = rand() % 256;
    
    int real_len = msg->size + 1;
    int aligned_len = ((msg->size + 4) / 4) * 4;
    
    for (int i = 0; i < aligned_len; i++) {
        msg->data[i] = (i < real_len) ? (rand() % 256) : 0;
    }
    
    msg->hash = calculate_DJB2(msg);
}

void queue_init(SharedQueue *q, int size) {
    q->queue_size = size;
    q->buffer = malloc(sizeof(Message) * size);
    q->head = 0;
    q->tail = 0;
    q->added_count = 0;
    q->extracted_count = 0;
    q->current_load = 0;
    
    pthread_mutex_init(&q->mutex, NULL);
    sem_init(&q->empty, 0, size);
    sem_init(&q->full, 0, 0);
}

void queue_destroy(SharedQueue *q) {
    if (!q) return;
    
    printf("[Queue] Destroying queue buffer...\n");
    
    pthread_mutex_lock(&q->mutex);
    
    if (q->buffer) {
        memset(q->buffer, 0, sizeof(Message) * q->queue_size);
        free(q->buffer);
        q->buffer = NULL;
    }
    
    q->head = 0;
    q->tail = 0;
    q->current_load = 0;
    q->queue_size = 0;
    
    pthread_mutex_unlock(&q->mutex);
    
    pthread_mutex_destroy(&q->mutex);
    sem_destroy(&q->empty);
    sem_destroy(&q->full);
    
    printf("[Queue] Queue destroyed successfully\n");
}

int queue_push(SharedQueue *q, Message *msg) {
    pthread_mutex_lock(&q->mutex);

    if (q->current_load >= q->queue_size) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }

    q->buffer[q->tail] = *msg;
    q->tail = (q->tail + 1) % q->queue_size;
    q->added_count++;
    q->current_load++;

    printf("[Producer %lu] Added message #%d. In queue: %d/%d\n", 
           pthread_self(), q->added_count, q->current_load, q->queue_size);

    pthread_mutex_unlock(&q->mutex);
    return 0;
}

Message queue_pop(SharedQueue *q) {
    pthread_mutex_lock(&q->mutex);

    Message msg = q->buffer[q->head];
    q->head = (q->head + 1) % q->queue_size;
    q->extracted_count++;
    q->current_load--;

    printf("[Consumer %lu] Popped message #%d. In queue: %d/%d\n", 
           pthread_self(), q->extracted_count, q->current_load, q->queue_size);

    pthread_mutex_unlock(&q->mutex);
    return msg;
}

int queue_resize(SharedQueue *q, int new_size) {
    if (new_size < 1 || new_size > 100) {
        printf("Error: Invalid queue size %d (must be 1-100)\n", new_size);
        return -1;
    }
    
    pthread_mutex_lock(&q->mutex);
    
    if (new_size < q->current_load) {
        printf("Error: Cannot shrink queue below current load (%d)\n", q->current_load);
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    
    Message *new_buffer = malloc(sizeof(Message) * new_size);
    if (!new_buffer) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    
    for (int i = 0; i < q->current_load; i++) {
        int old_idx = (q->head + i) % q->queue_size;
        new_buffer[i] = q->buffer[old_idx];
    }
    
    free(q->buffer);
    q->buffer = new_buffer;
    q->head = 0;
    q->tail = q->current_load;
    
    int old_size = q->queue_size;
    q->queue_size = new_size;
    
    printf("\n=== Queue resized: %d → %d (current load: %d) ===\n\n",
           old_size, new_size, q->current_load);
    
    pthread_mutex_unlock(&q->mutex);
    
    sem_destroy(&q->empty);
    sem_destroy(&q->full);
    sem_init(&q->empty, 0, new_size - q->current_load);
    sem_init(&q->full, 0, q->current_load);
    
    return 0;
}